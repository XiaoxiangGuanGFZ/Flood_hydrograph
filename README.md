## Flood hydrograph

The program is designed to extract the hydrograph of  independent flood events with its peak flow exceeding a defined threshold.

An independent peak is identified if it fulfils the criteria following Guse et al. (2020): 

1. the lowest discharge between two peaks is smaller than 70% of the smaller peak 
2. the smaller peak is greater than 20% of the annual maximum peak
3. the minimum flow between two peaks drops below 20% of the annual maximum flow 
4. the time lag between two peaks is at least 7 d

These criteria were empirically derived to prevent the identification of oscillatory peaks as independent flood events.

To estimate the start and end point, the gradient in discharge between 2 consecutive days is first calculated. The start point of the flood event is then identified by tracing back the gradient prior to the peak flow. If the gradient is lower than a predefined threshold for 7 consecutive days, the starting date is set to the latest date in this time window. 

The gradient threshold is usually empirically identified using a trial-and-error procedure and visual inspection. 90th percentile of all gradients could be selected. 

If no starting point is detected within 40 d prior to the peak flow, the lowest discharge value in this time window is selected. The event end point is analogously determined by looking forward from the peak.


## How-to-use

### Program preparation

The program is composed in C, together with the corresponding netcdf library. Nagivate to the directory where the source codes are store and then compile the program with `CMake` tool.

```
cd scr
mkdir build
cd build
cmake ..
make
```

After successfully compiling the program and get the executable, call the app and feed the appropriate command-line arguments to run the program:

```
.\Flood_hydrograph.exe -s 1.6 -q 177.9 -i .\subdaily_discharge.nc -o .\floodevent_out.csv
```

- `-s`: the gradient threshold
- `-q`: flood peak threshold
- `-i`: file path and name of the input, see example in `data` directory: `subdaily_discharge.nc` or `subdaily_discharge.out`
- `-o`: file path and name of the input

### Output

The result file has three columns indicating the detected event id, the date index and the series value respectively.


## Reference

Guse, B., Merz, B., Wietzke, L., Ullrich, S., Viglione, A. and Vorogushyn, S.  2020.  The role of flood wave superposition in the severity of large floods. Hydrol. Earth Syst. Sci., 24(4), 1633-1648. doi: 10.5194/hess-24-1633-2020.

## Author

[Xiaoxiang Guan](https://www.gfz-potsdam.de/staff/guan.xiaoxiang/sec44)
