#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <math.h>
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
    *(*data_G + dimLen - 1) = 0.0;
}

void Flood_AddNoise(
    double **data_Q,
    size_t dimLen,
    double Q_threshold
)
{
    double Q1, Q2;
    for (size_t i = 0; i < dimLen - 1; i++)
    {
        Q1 = *(*data_Q + i);
        Q2 = *(*data_Q + i + 1);
        if (fabs(Q1 - Q2) <= EPS && Q1 >= Q_threshold / 2.0){ // * fmax(fabs(Q1), fabs(Q2))
            *(*data_Q + i + 1) += 1e-4;
            // printf("Q1: %8f Q2: %8f Q2_new: %8f\n", Q1, Q2, *(*data_Q + i + 1));
        }
    }
}

void Flood_peaks(
    double *data_Q,
    double *data_G,
    int **flag_peak,
    int **index_peak,
    size_t *n_peaks,
    size_t dimLen
)
{
    /*************
     * detect all peaks in the time series
     * -----------
     * flag_peak: a list of integers, flag variable: 
     * -1: local minima 
     * 0: others
     * 1: local maxima
     * 
     * index_peak: index of detected peaks (local maxima)
     * n_peaks: number of peaks
     * ********/
    size_t N_p;
    N_p = dimLen; //  / 4;
    *flag_peak = (int *)malloc(sizeof(int) * dimLen);
    *(*flag_peak + 0) = 0;          // the first step
    *(*flag_peak + dimLen - 1) = 0; // the last step

    *index_peak = (int *)malloc(sizeof(int) * N_p);

    size_t id = 0;
    for (size_t i = 1; i < dimLen - 1; i++)
    {
        if (*(data_G + i - 1) < EPS && *(data_G + i) > EPS) // >=0 
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
            *(data_G + i - 1) > EPS && *(data_G + i) < EPS
        ) {
            *(*flag_peak + i) = -1;
        } else {
            *(*flag_peak + i) = 0.0;
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
    /***************
     * derive the gradient threshold
     * this function is deprecated. 
     * **************/
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
    size_t dimLen
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
        if (*(flag_peak + i) == 1) { // it is a peak (local maxima)

            if (abs(id_peak_tmp - i) >= time_lag_steps &&
                (peak_independent(data_Q, data_G, i, id_peak) == 1 ||
                 *(data_Q + i) >= Q_threshold))
            {
                /********
                 * it is an independent peak:
                 * the searching of start of the event should be stopped.
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
                // it is close to the evaluated peak, distance < time_lag_steps; 
                // or it is a dependent peak
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
            // *(flag_peak + i) either 0 or -1
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
         * no independent peak is run into during start of event back-tracing,
         * thus *id_start was not changed. 
         * ****/
        *id_start = i + n_low - 1;
    }
    
    /************ end of the event ********/
    i = id_peak;
    n_low = 0;
    *id_end = -1;
    while (i < dimLen && n_low < time_lag_steps)
    {
        i += 1;  // looking forward from the peak
        if (*(flag_peak + i) == 1) {
            // it is a peak at i
            if (abs(id_peak_tmp - i) >= time_lag_steps &&
                (peak_independent(data_Q, data_G, id_peak, i) == 1 ||
                *(data_Q + i) >= Q_threshold)
                )
            {
                /********
                 * it is an independent peak at i:
                 * the searching of start of the event shoud be terminated.
                 * *****/
                int id_find;
                
                id_find = i - 1;
                while (id_find > id_peak)
                {
                    if (*(flag_peak + id_find) == -1)  
                    {
                        /****
                         * the first local minima before peak i
                         * **/ 
                        *id_end = id_find;
                        break;
                    } else {
                        id_find -= 1;
                    }
                }
                break;
            } else {
                // it is not an independent peak;
                // it should be included in this detected event. 
                if (abs(*(data_G + i)) <= G_threshold)
                {
                    n_low += 1;
                }
                else
                {
                    n_low = 0; // refresh number of gradients lower than a predefined threshold [G_threshold]
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
                n_low = 0; // refresh number of gradients lower than a predefined threshold [G_threshold]
            }
        }
    }
    if (*id_end == -1 && i < dimLen)
    {
        /*******
         * no independent peak is run into during start of event back-tracing
         * ****/
        // *id_end = i - n_low + 1;
        // *id_end = i + n_low - 1;
        *id_end = i;
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
        double Qmin;  // lowest flow between two peaks
        double Qpeak1, Qpeak2;
        double Qmax1, Qmax2; // bigger and smaller flow between these two peaks
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
        Qmin = Qpeak1;
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