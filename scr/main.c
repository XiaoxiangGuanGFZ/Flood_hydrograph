#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netcdf.h>
#include <time.h>
#include "def_struct.h"
#include "Func_DataIO.h"
#include "Func_FloodHydrograph.h"
#include "Func_Datetime.h"


void print_usage() {
    printf("Usage: ./app.exe -s <value> -i <input_file> -o <output_file>\n");
}

void print_usage();

int main(int argc, char *argv[])
{
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;
    double s_value = 0;
    double q_value = 100.0;
    int s_flag = 0, q_flag, i_flag = 0, o_flag = 0;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:q:i:o:")) != -1) {
        switch (opt) {
            case 's':
                s_value = atof(optarg);
                s_flag = 1;
                break;
            case 'q':
                q_value = atof(optarg);
                q_flag = 1;
                break;
            case 'i':
                input_file = optarg;
                i_flag = 1;
                break;
            case 'o':
                output_file = optarg;
                o_flag = 1;
                break;
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    // Check that all required flags were provided
    if (!s_flag || !i_flag || !o_flag) {
        print_usage();
        return EXIT_FAILURE;
    }
    printf("-------------- command-line arguments: \n");
    // Print the parsed values (for debugging)
    printf("%s: %.3f\n", "- low flow threshold", s_value);
    printf("%s: %.3f\n", "- peak flow threshold", q_value);
    printf("%s: %s\n", "- input", input_file);
    printf("%s: %s\n", "- output", output_file);
    
    /***************
     * time series data import
     * *************/
    printf("-------------- data import (preview of first 10 rows): \n");
    size_t dimLen;   // the total length of the time series
    double *data_Q;  // time series
    dimLen = 1000000;
    int fileformat = 0;
    fileformat = check_file_extension(input_file);
    if (fileformat == 0) {
        ST_DATA *p_data;
        Data_import_ascii(input_file, &data_Q, &p_data, dimLen);
        printf("%6s %6s %6s %6s %6s %6s \n", "y", "m", "d", "h", "Qsim", "Qobs");
        for (size_t i = 0; i < 10; i++)
        {
            printf("%6d %6d %6d %6d %6.3f %6.3f \n",
                   (p_data + i)->y, (p_data + i)->m, (p_data + i)->d, (p_data + i)->h,
                   (p_data + i)->Qsim, (p_data + i)->Qobs);
        }
    } else if (fileformat == 1) {
        char varName[30] = "Qsim_0000000101";
        Data_import(input_file, varName, &data_Q, &dimLen);
        for (size_t i = 0; i < 10; i++)
        {
            printf("%6.3f \n",*(data_Q + i));
        }
    }
    printf("-------------- discharge data import: Done!\n");


    /****************
     * discharge process gradient derivation for the entire series
     * **************/
    double *data_G;  // gradient of discharge
    Gradient_discharge(data_Q, &data_G, dimLen);
    printf("-------------- discharge gradient computed: Done!\n");
    
    // double position_th = 90;
    double G_percentile;
    // Gradient_percentile(data_G, position_th, &G_percentile, dimLen);
    // G_percentile = 0.5667546;
    G_percentile = s_value;
    //           90%       92%       95%       97% 
    //     0.5667546 0.7167893 1.1169979 1.6636920 
    // printf("90th percentile of discharge gradient: %5.1f\n", G_percentile);

    /*****************
     * extract all the discharge peaks
     * **************/
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
    printf("-------------- discharge peaks extraction: Done!\n");

    /**************
     * based on the detected flood peaks,
     * look foreward and backward for the start and end point of the event
     * ************/
    FILE *p_out;
    if ((p_out = fopen(output_file, "w")) == NULL)
    {
        printf("Cannot create / open output file: %s\n", output_file);
        exit(1);
    }
    fprintf(p_out, "event_id,datetime_index,time_series\n");
    DATETIME DT_start = {1950, 1, 1, 0}; 
    double Q_threshold; // = 177.0
    Q_threshold = q_value;

    printf("-------------- flood event hydrograph extraction: ...\n");
    int id_start, id_end;
    int time_lag_days = 3; // time lag between two peaks; 3 days for catchment with area less than 1000km2
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
    printf("-------------- flood events extraction: Done!\n");
    return 0;
}

