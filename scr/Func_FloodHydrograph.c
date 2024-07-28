#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include "def_struct.h"
#include "Func_FloodHydrograph.h"


void Gradient_discharge(
    double *data_Q,
    double **data_G,
    size_t dimLen
)
{
    *data_G = (double *)malloc(sizeof(double) * dimLen);
    for (size_t i = 0; i < dimLen - 1; i++)
    {
        *(*data_G + i) = *(data_Q + i) - *(data_Q + i + 1);
    }
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
    int N_p;
    N_p = (int) dimLen / 4;
    *flag_peak = (int *)malloc(sizeof(int) * dimLen);
    *(*flag_peak + 0) = 0;          // the first step
    *(*flag_peak + dimLen - 1) = 0; // the last step

    *index_peak = (int *)malloc(sizeof(int) * N_p);

    int id = 0;
    for (size_t i = 1; i < dimLen - 1; i++)
    {
        if (*(data_G + i - 1) < 0 && *(data_G + i) >= 0)
        {
            *(*flag_peak + i) = 1;
            *(*index_peak + id) = i;
            id += 1;
            if (id >= N_p)
            {
                printf("The number of flood peaks overflows!\n");
                exit(1);
            }
            
        } else if (
            *(data_G + i - 1) > 0 && *(data_G + i) < 0
        ) {
            *(*flag_peak + i) = -1;
        } else {
            *(*flag_peak + i) = 0;
        }
    }
    *n_peaks = id;
}

void Gradient_percentile(
    double *data_G,
    double position_th,
    double *percentile,
    size_t dimLen
)
{
    double *data_Q_abs;
    data_Q_abs = (double *)malloc(sizeof(double) * dimLen);
    for (size_t i = 0; i < dimLen; i++)
    {
        *(data_Q_abs + i) = abs(*(data_G + i));
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
    
    int id_percentile;
    id_percentile = (int)((1 - position_th / 100) * dimLen);
    *percentile = *(data_Q_abs + id_percentile);
    free(data_Q_abs);
}


void Flood_event_identify(
    double *data_Q,
    double *data_G,
    int *flag_peak,
    int id_peak,
    int *id_start,
    int *id_end,
    int time_lag_days,
    double Q_threshold,
    double G_threshold,
    int dimLen
)
{
    int time_lag_steps;
    time_lag_steps = time_lag_days * 24;
    
    int id_peak_tmp;
    id_peak_tmp = id_peak;
    int i;
    int n_low;  // number of gradients lower than a predefined threshold in a continuous manner

    /************ start of the event ********/
    n_low = 0;
    i = id_peak;  
    *id_start = -1;
    while (i > 0 && n_low < time_lag_steps)
    {
        i -= 1;   // trace back, prior to the peak step
        if (*(flag_peak + i) == 1) {
            // it is peak
            if (abs(id_peak_tmp - i) >= time_lag_steps &&
                peak_independent(data_Q, data_G, i, id_peak) == 1 )
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
                    } else {
                        id_find += 1;
                    }
                }
                break;
            } else {
                if (abs(*(data_G + i)) <= G_threshold)
                {
                    n_low += 1;
                }
                else
                {
                    n_low = 0; // refresh number of gradient lower than a predefined threshold [G_threshold]
                }
            }
        } else {
            //  not a peak
            if (abs(*(data_G + i)) <= G_threshold)
            {
                n_low += 1;
            }
            else
            {
                n_low = 0; // refresh number of gradient lower than a predefined threshold [G_threshold]
            }
        }
    }
    if (*id_start == -1 && i >= 0)
    {
        /*******
         * independent peak is not reached during start of event back-tracing
         * ****/
        *id_start = i + n_low - 1;
    }
    
    /************ end of the event ********/
    i = id_peak;
    n_low = 0;
    *id_end = -1;
    while (i > 0 && n_low < time_lag_steps)
    {
        i += 1;  // looking forward from the peak
        if (*(flag_peak + i) == 1) {
            // it is peak
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
                if (abs(*(data_G + i)) <= G_threshold)
                {
                    n_low += 1;
                }
                else
                {
                    n_low = 0; // refresh number of gradient lower than a predefined threshold [G_threshold]
                }
            }
        } else {
            //  not a peak
            if (abs(*(data_G + i)) <= G_threshold)
            {
                n_low += 1;
            }
            else
            {
                n_low = 0; // refresh number of gradient lower than a predefined threshold [G_threshold]
            }
        }
    }
    if (*id_end == -1 && i >= 0)
    {
        /*******
         * independent peak is not reached during start of event back-tracing
         * ****/
        *id_end = i + n_low - 1;
    }
}

int peak_independent(
    double *data_Q,
    double *data_G,
    int id1,
    int id2
)
{
    
    if (id1 < id2)
    {
        double Qmin;
        double Qpeak1, Qpeak2;
        double Qmax1, Qmax2;
        Qpeak1 = *(data_Q + id1);
        Qpeak2 = *(data_Q + id2);
        if (Qpeak1 > Qpeak2)
        {
            Qmax1 = Qpeak1;
            Qmax2 = Qpeak2;
        }
        else
        {
            Qmax1 = Qpeak2;
            Qmax2 = Qpeak1;
        }
        Qmin = Qmax1;
        for (size_t i = id1 + 1; i < id2; i++)
        {
            if (Qmin > *(data_Q + i))
            {
                Qmin = *(data_Q + i);
            }
        }
        if (Qmin < 0.7 * Qmax2 && Qmin < 0.2 * Qmax1 && Qmax2 > 0.2 * Qmax1)
        {
            return 1;  // independent peak
        } else {
            return 0;  // connected peak
        }
    } else {
        printf("Error in peak_independent() \n");
        exit(1);
    }
}