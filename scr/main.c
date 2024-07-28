#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <time.h>
#include "def_struct.h"
#include "Func_DataIO.h"
#include "Func_FloodHydrograph.h"
#include "Func_Datetime.h"

int main()
{
    /************
     * data import:
     * hourly simulated discharge
     * *************/
    char varName[30] = "Qsim_0000000101";
    char FP[] = "D:/CPFF/Flood_hydrograph/subdaily_discharge.nc";
    size_t dimLen;
    double *data_Q;
    Data_import(FP, varName, &data_Q, &dimLen);
    
    double *data_G;  // gradient of discharge
    Gradient_discharge(data_Q, &data_G, dimLen);
    double position_th = 90;
    double G_percentile;
    // Gradient_percentile(data_G, position_th, &G_percentile, dimLen);
    G_percentile = 0.5667546;
    //           90%       92%       95%       97% 
    //     0.5667546 0.7167893 1.1169979 1.6636920 
    printf("90th percentile of discharge gradient: %5.1f\n", G_percentile);
    int *flag_peak;
    int *index_peak;
    int n_peaks;
    Flood_peaks(
        data_Q,
        data_G,
        &flag_peak,
        &index_peak,
        &n_peaks,
        dimLen);

    // for (size_t i = 0; i < 100; i++)
    // {
    //     printf("%5.2f, %5.2f, %d\n", *(data_Q + i), *(data_G + i), *(flag_peak + i));
    // }

    // for (size_t i = 0; i < n_peaks; i++)
    // {
    //     printf("%d,", *(index_peak + i));
    // }
    FILE *p_out;
    if ((p_out = fopen("D:/CPFF/Flood_hydrograph/floodevent_out.csv", "w")) == NULL)
    {
        printf("Cannot create / open SSIM file: \n");
        exit(1);
    }
    DATETIME DT_start = {1950, 1, 1, 0}; 

    double Q_threshold = 177.0;

    int id_start, id_end;
    int time_lag_days = 7;
    int event_id = 0;
    printf("%8s%9s%9s%8s\n", "Event_id", "id_start", "id_end", "Q_peak");
    for (size_t i = 0; i < n_peaks; i++)
    {
        if (*(data_Q + *(index_peak + i)) >= Q_threshold)
        {
            Flood_event_identify(
                data_Q,
                data_G,
                flag_peak,
                *(index_peak + i),
                &id_start,
                &id_end,
                time_lag_days,
                Q_threshold,
                G_percentile,
                dimLen);
            /*******
             * write the flood event:
             * - start and end datetime (datatime series), 
             * - hourly discharge
             * ******/
            Flood_event_write(
                p_out,
                data_Q,
                id_start,
                id_end,
                event_id,
                DT_start);
            printf("%8d%9d%9d%8.2f\n", event_id, id_start, id_end, *(data_Q + *(index_peak + i)));
            event_id++;

            while (id_end >= *(index_peak + i) && i < n_peaks)
            {
                i += 1;
            }
        }
    }
    
    return 0;
}

