#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "def_struct.h"
#include "Func_FloodHydrograph.h"



void Gradient_discharge(
    double *data_Q,
    double **data_G,
    size_t dimLen
)
{
    /******************
     *  Function: Gradient_discharge
        Purpose:
        Computes the discrete gradient (difference) of a discharge time series.
        The gradient is calculated as the difference between consecutive values.

        Inputs:
        data_Q  - input array of discharge values
        data_G  - pointer to output array (will store gradient values)
        dimLen  - length of the input time series

        Output:
        data_G[i] = data_Q[i] - data_Q[i+1]
        The last value is set to 0 (no forward value available)

        Notes:
        - Uses forward difference scheme
        - Memory for data_G is allocated inside the function
        - Caller is responsible for freeing allocated memory
     *************/

    // Allocate memory for gradient array
    *data_G = (double *)malloc(sizeof(double) * dimLen);
    for (size_t i = 0; i < dimLen - 1; i++)
    {
        // Compute gradient using forward difference
        // Difference between current and next value
        *(*data_G + i) = *(data_Q + i) - *(data_Q + i + 1);
    }
    // Handle last element (no next value available)
    *(*data_G + dimLen - 1) = 0;
}



void Flood_peaks(
    double *data_Q,
    double *data_G,
    int **flag_peak,
    int **index_peak,
    int *n_peaks,
    size_t dimLen
)
{
    /******************
    Function: Flood_peaks
    Purpose:
    Detects local peaks in a discharge time series using
    the gradient (data_G). Peaks are identified by sign changes in the gradient.

    Inputs:
    data_Q     - discharge time series (not directly used here but kept for context)
    data_G     - gradient of discharge (e.g., from Gradient_discharge)
    flag_peak  - output array marking peak/trough/normal points
    index_peak - output array storing indices of detected peaks
    n_peaks    - output: number of detected peaks
    dimLen     - length of the time series

    Output:
    flag_peak[i]:
        1  -> local maximum (flood peak)
        -1  -> local minimum (valley)
        0  -> neither

    index_peak:
        stores indices where flag_peak == 1

    Method:
    A peak is detected when gradient changes from negative to positive:
        data_G[i-1] < 0 AND data_G[i] >= 0

    A valley is detected when gradient changes from positive to negative:
        data_G[i-1] > 0 AND data_G[i] < 0

    Notes:
    - Memory is allocated inside the function
    - Caller must free flag_peak and index_peak
    - Maximum number of peaks is limited to dimLen/4 (safety assumption)

     * ***************** */

    // Estimate maximum number of peaks (heuristic)
    int N_p;
    N_p = (int) dimLen / 4;

    // Allocate memory for flags (same length as data)
    *flag_peak = (int *)malloc(sizeof(int) * dimLen);

    // First and last points cannot be peaks (boundary condition)
    *(*flag_peak + 0) = 0;          // the first step
    *(*flag_peak + dimLen - 1) = 0; // the last step

    // Allocate memory for storing peak indices
    *index_peak = (int *)malloc(sizeof(int) * N_p);

    int id = 0;  // counter for detected peaks

    // Loop through time series (excluding boundaries)
    for (size_t i = 1; i < dimLen - 1; i++)
    {
        // Case 1: Local maximum (flood peak)
        // Gradient changes from negative to positive
        if (*(data_G + i - 1) < 0 && *(data_G + i) >= 0)
        {
            *(*flag_peak + i) = 1;      // mark as peak
            *(*index_peak + id) = i;    // store index
            id += 1;

            // Check for overflow of allocated peak storage
            if (id >= N_p)
            {
                printf("The number of flood peaks overflows!\n");
                exit(1);
            }
        } 
        // Case 2: Local minimum (valley)
        // Gradient changes from positive to negative
        else if (
            *(data_G + i - 1) > 0 && *(data_G + i) < 0
        ){
            *(*flag_peak + i) = -1;
        } 
        // Case 3: No significant feature
        else {
            *(*flag_peak + i) = 0;
        }
    }
    // Store total number of detected peaks
    *n_peaks = id;
}



void Gradient_percentile(
    double *data_G,
    double position_th,
    double *percentile,
    size_t dimLen
)
{
    /*********************
    Function: Gradient_percentile
    Purpose:
    Computes a percentile threshold of the absolute gradient values.
    This can be used to identify significant changes in discharge.

    Inputs:
    data_G       - gradient time series (can contain positive/negative values)
    position_th  - percentile position (e.g., 90 for 90th percentile)
    percentile   - output: computed percentile value
    dimLen       - length of the time series

    Method:
    1. Take absolute value of gradient (magnitude of change)
    2. Sort values in descending order
    3. Select value corresponding to desired percentile

    Notes:
    - Uses simple (inefficient) bubble-sort-like algorithm: O(n²)
    - Percentile is computed from descending order
    - Caller provides pointer for output value
     * *****************/

    // Allocate memory for absolute gradient values
    double *data_Q_abs;
    data_Q_abs = (double *)malloc(sizeof(double) * dimLen);
    for (size_t i = 0; i < dimLen; i++)
    {
        // Compute absolute values of gradient
        *(data_Q_abs + i) = fabs(*(data_G + i));
    }
    
    // sort the absolute gradients in decreasing order
    double tmp;
    for (size_t i = 0; i < dimLen - 1; i++)
    {
        for (size_t j = i + 1; j < dimLen; j++)
        {
            if (*(data_Q_abs + i) < *(data_Q_abs + j))
            {
                tmp = *(data_Q_abs + i);
                *(data_Q_abs + i) = *(data_Q_abs + j);
                *(data_Q_abs + j) = tmp;
            }
        }
    }
    
    // Compute index corresponding to desired percentile
    // Example: position_th = 90 → selects top 10% threshold
    int id_percentile;
    id_percentile = (int)((1 - position_th / 100) * dimLen);
    // Assign percentile value

    *percentile = *(data_Q_abs + id_percentile);
    free(data_Q_abs); // Free allocated memory
}



void Flood_event_identify(
    double *data_Q,
    double *data_G,
    int *flag_peak,
    int id_peak,
    int *id_start,
    int *id_end,
    int time_lag_steps,
    double Q_threshold,
    double G_threshold,
    int dimLen
)
{
    /*********************
    Function: Flood_event_identify
    Purpose:
    Identifies the start and end indices of a flood event around a given peak.

    Inputs:
    data_Q         - discharge time series
    data_G         - gradient of discharge
    flag_peak      - array marking peaks (1), valleys (-1), others (0)
    id_peak        - index of the target peak
    id_start       - output: start index of the event
    id_end         - output: end index of the event
    time_lag_steps - minimum separation steps between independent peaks (in hours)
    Q_threshold    - (currently unused) discharge threshold
    G_threshold    - gradient threshold to define "stable/low change"
    dimLen         - length of time series

    Method:
    - Convert time lag from days → time steps (hours)
    - Search backward from peak to find event start
    - Search forward from peak to find event end
    - Stop when:
        (1) an independent peak is found → use valley between peaks
        (2) sufficiently long period of low gradient is found

    Hydrological idea:
    A flood event begins/ends when:
        - flow stabilizes (low gradient), OR
        - another independent event separates it

     ******************/

    
    // store original peak index
    int id_peak_tmp;
    id_peak_tmp = id_peak;
    int i;

    // counter for consecutive low-gradient steps
    int n_low;  // number of gradients lower than a predefined threshold in a continuous manner


    /************ START OF THE EVENT (backward search) ************/
    n_low = 0;
    i = id_peak;  
    *id_start = -1; // initialize as "not found"

    // Move backward in time
    while (i > 0 && n_low < time_lag_steps)
    {
        i -= 1;   // trace back, prior to the peak step

        // Case 1: encountered another peak
        if (*(flag_peak + i) == 1) {
            // it is also peak,
            // Check if this is an independent peak
            if (abs(id_peak_tmp - i) >= time_lag_steps &&
                peak_independent(data_Q, data_G, i, id_peak) == 1)
            {
                /********
                 * it is an independent peak:
                 * the searching of start of the event is stopped.
                 * *****/
                int id_find;
                id_find = i + 1;
                while (id_find < id_peak)
                {
                    if (*(flag_peak + id_find) == -1)  
                    {
                        /****
                         * lowest flow between two independent peaks:
                         * peak i and peak id_peak
                         * **/ 
                        *id_start = id_find;
                        break;
                    }
                    id_find += 1;
                }
                break;
            } 
            // Case 2: check gradient condition (applies to all points)
            else {
                if (fabs(*(data_G + i)) <= G_threshold)
                {
                    n_low += 1;  // stable flow → possible boundary
                }
                else
                {
                    n_low = 0;   // reset counter if gradient increases
                }
            }
        } else {
            //  not a peak
            if (fabs(*(data_G + i)) <= G_threshold)
            {
                n_low += 1;
            }
            else
            {
                n_low = 0; // reset counter
            }
        }
    }

    // If no independent peak found → use low-gradient condition
    if (*id_start == -1 && i >= 0)
    {
        /*******
         * independent peak is not reached during start of event back-tracing
         * ****/
        *id_start = i + n_low - 1;
    }
    
    /************ END OF THE EVENT (forward search) ************/
    i = id_peak;
    n_low = 0;
    *id_end = -1;

    // Move forward in time
    while (i < dimLen - 1 && n_low < time_lag_steps)
    {
        i += 1;  // looking forward from the peak
        if (*(flag_peak + i) == 1) {
            // it is peak,
            // check if this is an independent peak
            if (abs(id_peak_tmp - i) >= time_lag_steps &&
                peak_independent(data_Q, data_G, id_peak, i) == 1 )
            {
                /********
                 * it is an independent peak:
                 * the searching of start of the event is stopped.
                 * *****/
                int id_find;
                id_find = id_peak + 1;
                while (id_find < i)
                {
                    if (*(flag_peak + id_find) == -1)  
                    {
                        /****
                         * lowest flow between two independent peaks:
                         * peak i and peak id_peak
                         * **/ 
                        *id_end = id_find;
                        break;
                    } else {
                        id_find += 1;
                    }
                }
                break;
            } else {
                if (fabs(*(data_G + i)) <= G_threshold)
                {
                    n_low += 1;
                }
                else
                {
                    n_low = 0; // reset counter
                }
            }
        } else {
            //  not a peak
            if (fabs(*(data_G + i)) <= G_threshold)
            {
                n_low += 1;
            }
            else
            {
                n_low = 0; // reset counter
            }
        }
    }
    if (*id_end == -1 && i >= 0)
    {
        /*******
         * independent peak is not reached during start of event back-tracing
         * ****/
        *id_end = i - n_low + 1;
    }
}



int peak_independent(
    double *data_Q,
    double *data_G,
    int id1,
    int id2
)
{
    /**********************
    Function: peak_independent
    Purpose:
    Determines whether two detected peaks belong to independent flood events
    or are part of the same event.

    Inputs:
    data_Q - discharge time series
    data_G - gradient (not used here, but kept for consistency/interface)
    id1    - index of first peak
    id2    - index of second peak (must be > id1)

    Output:
    returns:
        1 → peaks are independent (separate flood events)
        0 → peaks are connected (same event)

    Method:
    - Find the minimum discharge (Qmin) between the two peaks
    - Compare Qmin with the magnitudes of the two peaks
    - Apply heuristic thresholds to decide independence

    Hydrological idea:
    If the flow between peaks drops sufficiently, they are independent events.
     ***********************/

    // Ensure indices are in correct order
    if (id1 < id2)
    {
        double Qmin;  // lowest flow between two peaks
        double Qpeak1, Qpeak2;
        double Qmax1, Qmax2; // bigger and smaller flow between these two peaks

        // Get peak values
        Qpeak1 = *(data_Q + id1);
        Qpeak2 = *(data_Q + id2);

        // Determine which peak is larger
        if (Qpeak1 > Qpeak2)
        {
            Qmax1 = Qpeak1;  // larger peak
            Qmax2 = Qpeak2;  // smaller peak
        }
        else
        {
            Qmax1 = Qpeak2;
            Qmax2 = Qpeak1;
        }

        // Initialize Qmin as first peak (will be reduced)
        Qmin = Qpeak1;
        // Find minimum discharge between the two peaks
        for (size_t i = id1 + 1; i < id2; i++)
        {
            if (Qmin > *(data_Q + i))
            {
                Qmin = *(data_Q + i);
            }
        }

        /*********
         * Independence criteria:
         *
         * 1) Qmin < 0.7 * smaller peak
         * 2) Qmin < 0.2 * larger peak
         * 3) smaller peak is not too small (at least 20% of larger peak)
         *
         * These ensure:
         *   - a significant drop between peaks
         *   - both peaks are meaningful
         *********/

        if (Qmin < 0.7 * Qmax2 && Qmin < 0.2 * Qmax1 && Qmax2 > 0.2 * Qmax1)
        {
            return 1;  // independent peak (separate events)
        } else {
            return 0;  // connected peak
        }
    } else {
        printf("Error in peak_independent() \n");
        exit(1);
    }
}

