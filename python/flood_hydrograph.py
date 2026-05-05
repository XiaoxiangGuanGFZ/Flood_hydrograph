"""
flood_hydrograph.py
-------------------
Python translation of the C flood hydrograph separation program.

Original author: Xiaoxiang Guan (guan@gfz.de)
Python translation: Heiko Apel with the help of Claude

Usage:
    hourly discharge:
    python flood_hydrograph.py -s 1.6 -q 177.9 -t 3 -r h -i hourly.csv -o events_hourly.csv
    or daily: 
    python flood_hydrograph.py -s 1.6 -q 177.9 -t 3 -r d -i daily.csv -o events_daily.csv
    

Arguments:
    -s  gradient threshold (default: 0.1)
    -q  flood peak threshold (required)
    -t  time lag between independent peaks in days (default: 3)
    -r  temporal resolution of input discharge [h/d]: hourly or daily
    -i  input CSV file path (required)
    -o  output CSV file path (required)

Input CSV format:
    Header row required.
    Hourly:
        Year, Month, Day, Hour, Discharge
    Daily:
        Year, Month, Day, Discharge

Reference:
    Guse et al. (2020), Hydrol. Earth Syst. Sci., 24(4), 1633-1648.
"""

import argparse
import sys
import numpy as np
import pandas as pd


# ---------------------------------------------------------------------------
# Data I/O
# ---------------------------------------------------------------------------

def data_import_ascii(fp_data: str, resolution: str) -> pd.DataFrame:
    """
    Read a CSV discharge time series file.

    Parameters
    ----------
    fp_data : str
        Path to the CSV file. Expected columns: Year, Month, Day, Hour, Discharge.
        First row is treated as header.
    
    resolution : str
        'h' for hourly data: expected columns Year, Month, Day, Hour, Discharge.
        'd' for daily data:  expected columns Year, Month, Day, Discharge.
    
    Returns
    -------
    pd.DataFrame
        DataFrame with columns: y, m, d, [h,] Q
    """
    try:
        df = pd.read_csv(fp_data, header=0)
    except FileNotFoundError:
        print(f"Cannot open data file: {fp_data}")
        sys.exit(1)

    # Normalise column names: use positional assignment to stay robust
    # regardless of the actual header text in the CSV
    
    if resolution == "h":
        if df.shape[1] != 5:
            print("Hourly input must have 5 columns: Year, Month, Day, Hour, Discharge")
            sys.exit(1)
        df.columns = ["y", "m", "d", "h", "Q"]

    elif resolution == "d":
        if df.shape[1] != 4:
            print("Daily input must have 4 columns: Year, Month, Day, Discharge")
            sys.exit(1)
        df.columns = ["y", "m", "d", "Q"]
        df["h"] = 0
        df = df[["y", "m", "d", "h", "Q"]]

    else:
        print(f"Invalid resolution: {resolution}. Use 'h' or 'd'.")
        sys.exit(1)
    
    return df


def flood_event_write(p_out, df: pd.DataFrame, data_Q: np.ndarray,
                      id_start: int, id_end: int, event_id: int,
                      resolution: str) -> None:
    """
    Write a single flood event time series to an already-open output file.
    For hourly data:
        event_id,y,m,d,h,discharge

    For daily data:
        event_id,y,m,d,discharge

    Parameters
    ----------
    p_out : file object
        Open output file (write mode).
    df : pd.DataFrame
        Full time series DataFrame (for date/time columns).
    data_Q : np.ndarray
        Discharge array.
    id_start : int
        Start index of the flood event (inclusive).
    id_end : int
        End index of the flood event (inclusive).
    event_id : int
        Flood event identifier.
    """
    for i in range(id_start, id_end + 1):
        row = df.iloc[i]
        if resolution == "h":
            p_out.write(
                f"{event_id},{int(row.y)},{int(row.m)},{int(row.d)},"
                f"{int(row.h)},{data_Q[i]:.6f}\n"
            )
        else:
            p_out.write(
                f"{event_id},{int(row.y)},{int(row.m)},{int(row.d)},"
                f"{data_Q[i]:.6f}\n"
            )


# ---------------------------------------------------------------------------
# Flood hydrograph functions
# ---------------------------------------------------------------------------

def gradient_discharge(data_Q: np.ndarray) -> np.ndarray:
    """
    Compute the discrete forward-difference gradient of a discharge time series.

    data_G[i] = data_Q[i] - data_Q[i+1]
    The last element is set to 0.

    Parameters
    ----------
    data_Q : np.ndarray
        Discharge time series.

    Returns
    -------
    np.ndarray
        Gradient array (same length as data_Q).
    """
    data_G = np.empty(len(data_Q))
    data_G[:-1] = data_Q[:-1] - data_Q[1:]
    data_G[-1] = 0.0
    return data_G


def flood_peaks(data_Q: np.ndarray, data_G: np.ndarray):
    """
    Detect local peaks and valleys in a discharge time series.

    A peak (local maximum) is detected where the gradient changes from
    negative to positive (data_G[i-1] < 0 and data_G[i] >= 0).
    A valley (local minimum) is detected where gradient changes from
    positive to negative.

    Parameters
    ----------
    data_Q : np.ndarray
        Discharge time series (not used directly, kept for interface consistency).
    data_G : np.ndarray
        Gradient of discharge.

    Returns
    -------
    flag_peak : np.ndarray of int
        Array of same length as data_Q:
         1  → local maximum (flood peak)
        -1  → local minimum (valley)
         0  → neither
    index_peak : np.ndarray of int
        Indices where flag_peak == 1.
    n_peaks : int
        Number of detected peaks.
    """
    n = len(data_Q)
    flag_peak = np.zeros(n, dtype=int)

    for i in range(1, n - 1):
        if data_G[i - 1] < 0 and data_G[i] >= 0:
            flag_peak[i] = 1       # local maximum
        elif data_G[i - 1] > 0 and data_G[i] < 0:
            flag_peak[i] = -1      # local minimum

    index_peak = np.where(flag_peak == 1)[0]
    n_peaks = len(index_peak)
    return flag_peak, index_peak, n_peaks


def gradient_percentile(data_G: np.ndarray, position_th: float) -> float:
    """
    Compute a percentile of the absolute gradient values.

    Parameters
    ----------
    data_G : np.ndarray
        Gradient time series.
    position_th : float
        Percentile position (e.g., 90 for the 90th percentile).

    Returns
    -------
    float
        The percentile value.
    """
    return float(np.percentile(np.abs(data_G), position_th))


def peak_independent(data_Q: np.ndarray, id1: int, id2: int) -> bool:
    """
    Determine whether two peaks belong to independent flood events.

    Independence criteria (Guse et al. 2020):
        1. Qmin < 0.7 * smaller peak
        2. Qmin < 0.2 * larger peak
        3. smaller peak > 0.2 * larger peak

    Parameters
    ----------
    data_Q : np.ndarray
        Discharge time series.
    id1 : int
        Index of the first (earlier) peak.
    id2 : int
        Index of the second (later) peak. Must be > id1.

    Returns
    -------
    bool
        True if the peaks are independent, False otherwise.
    """
    if id1 >= id2:
        print("Error in peak_independent(): id1 must be < id2")
        sys.exit(1)

    Qpeak1 = data_Q[id1]
    Qpeak2 = data_Q[id2]
    Qmax1 = max(Qpeak1, Qpeak2)   # larger peak
    Qmax2 = min(Qpeak1, Qpeak2)   # smaller peak

    Qmin = data_Q[id1:id2 + 1].min()

    return (Qmin < 0.7 * Qmax2) and (Qmin < 0.2 * Qmax1) and (Qmax2 > 0.2 * Qmax1)


def flood_event_identify(
    data_Q: np.ndarray,
    data_G: np.ndarray,
    flag_peak: np.ndarray,
    id_peak: int,
    time_lag_steps: int,
    Q_threshold: float,
    G_threshold: float,
) -> tuple:
    """
    Identify the start and end indices of a flood event around a given peak.

    The search traces backward (for start) and forward (for end) from the peak.
    It stops when either:
      - an independent adjacent peak is found (use the valley between them), or
      - a sufficiently long period of low gradient is found.

    Parameters
    ----------
    data_Q : np.ndarray
        Discharge time series.
    data_G : np.ndarray
        Gradient of discharge.
    flag_peak : np.ndarray of int
        Peak/valley flags (1, -1, 0).
    id_peak : int
        Index of the target peak.
    time_lag_steps : int
        Minimum separation between independent peaks, expressed as number of rows/time steps.
        For hourly data: days * 24.
        For daily data: days * 1.
    Q_threshold : float
        Discharge threshold (currently unused, kept for interface consistency).
    G_threshold : float
        Gradient threshold to define stable / low-change flow.

    Returns
    -------
    id_start : int
        Start index of the flood event.
    id_end : int
        End index of the flood event.
    """
    dimLen = len(data_Q)
    # time_lag_steps = time_lag_days * 24  # hourly data → steps per day = 24

    # ---- backward search for event start ----
    n_low = 0
    i = id_peak
    id_start = -1

    while i > 0 and n_low < time_lag_steps:
        i -= 1

        if flag_peak[i] == 1:
            # Another peak encountered
            if (abs(id_peak - i) >= time_lag_steps and
                    peak_independent(data_Q, i, id_peak)):
                # Independent peak found → find valley between them
                id_find = i + 1
                while id_find < id_peak:
                    if flag_peak[id_find] == -1:
                        id_start = id_find
                        break
                    id_find += 1
                break
            else:
                # Not independent → check gradient condition
                if abs(data_G[i]) <= G_threshold:
                    n_low += 1
                else:
                    n_low = 0
        else:
            if abs(data_G[i]) <= G_threshold:
                n_low += 1
            else:
                n_low = 0

    if id_start == -1 and i >= 0:
        id_start = i + n_low - 1

    # ---- forward search for event end ----
    i = id_peak
    n_low = 0
    id_end = -1

    while i < dimLen - 1 and n_low < time_lag_steps:
        i += 1
        if i >= dimLen:
            break

        if flag_peak[i] == 1:
            if (abs(id_peak - i) >= time_lag_steps and
                    peak_independent(data_Q, id_peak, i)):
                # Independent peak found → find valley between them
                id_find = id_peak + 1
                while id_find < i:
                    if flag_peak[id_find] == -1:
                        id_end = id_find
                        break
                    id_find += 1
                break
            else:
                if abs(data_G[i]) <= G_threshold:
                    n_low += 1
                else:
                    n_low = 0
        else:
            if abs(data_G[i]) <= G_threshold:
                n_low += 1
            else:
                n_low = 0

    if id_end == -1 and i >= 0:
        id_end = i - n_low + 1

    return id_start, id_end


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Flood hydrograph separation tool."
    )
    parser.add_argument("-s", type=float, default=0.1,
                        help="Discharge gradient threshold (default: 0.1)")
    parser.add_argument("-q", type=float, required=True,
                        help="Flood peak discharge threshold")
    parser.add_argument("-t", type=int, default=3,
                        help="Time lag between independent peaks in days (default: 3)")
    parser.add_argument("-r", choices=["h", "d"], default="h",
                        help="Input time resolution: 'h' for hourly, 'd' for daily (default: h)")
    parser.add_argument("-i", required=True, help="Input CSV file path")
    parser.add_argument("-o", required=True, help="Output CSV file path")
    return parser.parse_args()


def main():
    args = parse_args()

    s_value = args.s    # gradient threshold
    q_value = args.q    # peak flow threshold
    t_value = args.t    # time lag in days
    
    resolution = args.r # temporal resolution of input discharge, h or d
    if resolution == "h":
        steps_per_day = 24
    else:
        steps_per_day = 1
    
    print("-------------- command-line arguments:")
    print(f"  low gradient threshold:                    {s_value:.3f}")
    print(f"  peak flow threshold:                       {q_value:.3f}")
    print(f"  time lag between independent peaks (days): {t_value}")
    print(f"  input:                                     {args.i}")
    print(f"  input resolution:                          {resolution}")
    print(f"  output:                                    {args.o}")
    
    # --- Data import ---
    print("-------------- data import (preview of first 10 rows):")
    df = data_import_ascii(args.i, resolution)
    data_Q = df["Q"].to_numpy(dtype=float)

    print(f"{'y':>6} {'m':>6} {'d':>6} {'h':>6} {'value':>8}")
    for i in range(min(10, len(df))):
        row = df.iloc[i]
        print(f"{int(row.y):>6} {int(row.m):>6} {int(row.d):>6} {int(row.h):>6} {row.Q:>8.3f}")
    print("-------------- discharge data import: Done!")

    # --- Gradient computation ---
    data_G = gradient_discharge(data_Q)
    print("-------------- discharge gradient computed: Done!")

    G_threshold = s_value

    # --- Peak detection ---
    flag_peak, index_peak, n_peaks = flood_peaks(data_Q, data_G)
    print("-------------- flood peaks extraction: Done!")

    # --- Flood event separation ---
    try:
        p_out = open(args.o, "w")
    except OSError:
        print(f"Cannot create / open output file: {args.o}")
        sys.exit(1)

    if resolution == "h":
        p_out.write("event_id,y,m,d,h,discharge\n")
    else:
        p_out.write("event_id,y,m,d,discharge\n")

    Q_threshold = q_value
    # time_lag_days = t_value
    time_lag_steps = t_value * steps_per_day
    print("-------------- flood event hydrograph separation: ...")
    print(f"{'Event_id':>8} {'id_start':>9} {'id_end':>9} {'Q_peak':>8}")

    event_id = 0
    i = 0
    while i < n_peaks:
        id_pk = index_peak[i]

        if data_Q[id_pk] >= Q_threshold:
            id_start, id_end = flood_event_identify(
                data_Q, data_G, flag_peak,
                id_pk, time_lag_steps, Q_threshold, G_threshold
            )

            flood_event_write(p_out, df, data_Q, id_start, id_end, event_id, resolution)
            print(f"{event_id:>8} {id_start:>9} {id_end:>9} {data_Q[id_pk]:>8.2f}")
            event_id += 1

            # Skip peaks that fall within the current event's end
            while i < n_peaks and id_end >= index_peak[i]:
                i += 1
        else:
            i += 1

    p_out.close()
    print("-------------- flood events separation: Done!")


if __name__ == "__main__":
    main()
