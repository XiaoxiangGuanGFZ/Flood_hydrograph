## Flood hydrograph

## Introduction

The program is designed to extract the hydrograph of independent flood events with its peak flow exceeding a defined threshold.

An independent peak is identified if it fulfills the criteria following Guse et al. (2020): 

1. the lowest discharge between two peaks is smaller than 70% of the smaller peak 
2. the smaller peak is greater than 20% of the annual maximum peak
3. the minimum flow between two peaks drops below 20% of the annual maximum flow 
4. the time lag between two peaks is at least 7 d

These criteria were empirically derived to prevent the identification of oscillatory peaks as independent flood events.

Determination of the start and end point:

- the gradient in discharge between 2 consecutive days is first calculated.
- The start point of the flood event is then identified by tracing back the gradient prior to the peak flow. 
- If the gradient is lower than a predefined threshold for 7 consecutive days, the starting date is set to the latest date in this time window. 
- If no starting point is detected within 40 d prior to the peak flow, the lowest discharge value in this time window is selected. 
- The event end point is analogously determined by looking forward from the peak.

The gradient threshold is usually empirically identified using a trial-and-error procedure and visual inspection. 90th percentile of all gradients could be selected. 

The time lag parameter is given in days. Its conversion to time steps depends on the temporal resolution of the input data:
- hourly data → 1 day = 24 time steps
- daily data → 1 day = 1 time step

The time lag of 7 days is based on literature and is suitable for medium to large catchments. However, this value is not universally optimal. For smaller catchments (e.g., < 1000 km²) with fast runoff response, a shorter time lag may be more appropriate. In such cases, a value of around 3 days can be considered to better capture independent flood events and avoid merging distinct peaks.

## How-to-use

### Program preparation

The program is composed in C. Nagivate to the directory and then compile the program with `CMake` tool.

```shell
cd scr
mkdir build
cd build
cmake ..
make
```

Call the compiled executable and run the program in command-line:

```shell
.\Flood_hydrograph.exe -s 1.6 -q 177.9 -t 3 -r h -i .\hourly.csv -o .\out.csv
```

or for daily data:

```shell
.\Flood_hydrograph.exe -s 1.6 -q 177.9 -t 3 -r d -i .\daily.csv -o .\out.csv
```

- `-s`: the gradient threshold
- `-q`: flood peak threshold
- `-t`: time lag between independent peaks, in days
- `-r`: temporal resolution of input discharge
    - h = hourly
    - d = daily
- `-i`: file path and name of the input, see example in `data` directory: `discharge.csv`
- `-o`: file path and name of the input


### Input

The program supports both **hourly** and **daily** discharge time series in CSV format.

An examplary CSV-format data file with long-term **hourly** discharge observation: `.\data\discharge.csv`. In total, there are 5 columns, standing for the date and value series. 

```shell
Year,Month,Day,Hour,Discharge
2000,1,1,0,52.3
2000,1,1,1,48.7
```

The C program **doesn't** know how to deal with missing or non-numeric values in the input file. 

### Output

The separated flood hydrographs are stored in `.\out.csv`, where the first column `event_id` indicates the index of identified independent flood event. The following columns store the time series of hydrograph (discharge series). 

## Reference

Guse, B., Merz, B., Wietzke, L., Ullrich, S., Viglione, A. and Vorogushyn, S.  2020.  The role of flood wave superposition in the severity of large floods. Hydrol. Earth Syst. Sci., 24(4), 1633-1648. doi: https://doi.org/10.5194/hess-24-1633-2020.

## Author

[Xiaoxiang Guan](guan@gfz.de)


