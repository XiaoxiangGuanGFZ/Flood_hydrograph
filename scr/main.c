#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "def_struct.h"
#include "Func_DataIO.h"
#include "Func_FloodHydrograph.h"
#include "Func_Datetime.h"


void print_usage() {
    printf("Usage: ./app.exe -s <value> -q <value> -t <value> -r <h|d> -i <input_file> -o <output_file>\n");
    // printf("Usage: ./app.exe -s <value> -q <value> -t <value> -i <input_file> -o <output_file>\n");
}

void print_usage();

int main(int argc, char *argv[])
{
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;
    double s_value = 0.1;    // discharge gradient threshold
    double q_value = 100.0;  // flood peak threshold
    char resolution = 'h';   // default hourly
    int steps_per_day = 24;
    int t_value = 3;            // time lag (days) between indepedent peaks
    
    int s_flag = 0, q_flag;     // flag for flood separation argument
    int t_flag = 0;             // flag for time lap between flood peaks
    int i_flag = 0, o_flag = 0; // input and output flag
    int r_flag = 0;             // flag for temporal resolution of input discharge

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:q:t:r:i:o:")) != -1) {
        switch (opt) {
            case 's':
                s_value = atof(optarg);
                s_flag = 1;
                break;
            case 'q':
                q_value = atof(optarg);
                q_flag = 1;
                break;
            case 't':
                t_value = atoi(optarg);
                t_flag = 1;
                break;
            case 'r':
                resolution = optarg[0];   // take first char
                r_flag = 1;
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
    if (!q_flag || !i_flag || !o_flag || !r_flag) {
        print_usage();
        return EXIT_FAILURE;
    }

    printf("-------------- command-line arguments: \n");
    // Print the parsed values (for debugging)
    printf("%s: %.3f\n", "- low gradient threshold", s_value);
    printf("%s: %.3f\n", "- peak flow threshold", q_value);
    printf("%s: %d\n", "- time lag between independent peaks (days)", t_value);
    if (resolution == 'h') {
        printf("%s: %s\n", "- resolution", "hourly");
        steps_per_day = 24;
    } else if (resolution == 'd') {
        printf("%s: %s\n", "- resolution", "daily");
        steps_per_day = 1;
    } else {
        printf("Invalid resolution: %c (use 'h' or 'd')\n", resolution);
        return EXIT_FAILURE;
    }
    
    printf("%s: %s\n", "- input", input_file);
    printf("%s: %s\n", "- output", output_file);
    
    /***************
     * read time series
     * *************/
    printf("-------------- data import (preview of first 10 rows): \n");
    size_t dimLen;   // the total length of the time series
    double *data_Q;  // time series of discharge
    dimLen = 1000000;
    ST_DATA *p_data;
    if (resolution == 'h') {
        Data_import_ascii(input_file, &data_Q, &p_data, &dimLen);
    }
    else {
        Data_import_ascii_daily(input_file, &data_Q, &p_data, &dimLen);
    }

    printf("number of rows: %ld \n", dimLen);
    printf("%6s %6s %6s %6s %6s \n", "y", "m", "d", "h", "value");
    for (size_t i = 0; i < 10; i++)
    {
        printf("%6d %6d %6d %6d %6.3f \n",
                (p_data + i)->y, (p_data + i)->m, (p_data + i)->d, (p_data + i)->h,
                (p_data + i)->Q);
    }
    printf("-------------- discharge data import: Done!\n");
    
    /****************
     * discharge process: gradient derivation for the entire series
     * **************/
    double *data_G;  // gradient of discharge
    Gradient_discharge(data_Q, &data_G, dimLen);
    printf("-------------- discharge gradient computed: Done!\n");
    
    double G_percentile;
    G_percentile = s_value;

    /*****************
     * extract all the flood discharge peaks
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
    printf("-------------- flood peaks extraction: Done!\n");

    /**************
     * based on the detected flood peaks,
     * trace foreward and backward for the start and end point of the event
     * ************/
    FILE *p_out;
    if ((p_out = fopen(output_file, "w")) == NULL)
    {
        printf("Cannot create / open output file: %s\n", output_file);
        exit(1);
    }
    if (resolution == 'h') {
        fprintf(p_out, "event_id,y,m,d,h,discharge\n");
    } else {
        fprintf(p_out, "event_id,y,m,d,discharge\n");
    }
    
    double Q_threshold; // = 177.0
    Q_threshold = q_value;

    printf("-------------- flood event hydrograph separation: ...\n");
    int id_start, id_end;
    int time_lag_steps;
    time_lag_steps = t_value * steps_per_day;

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
                time_lag_steps,
                Q_threshold,
                G_percentile,
                dimLen);
            
            if (resolution == 'h') {
                /*******
                 * write the flood event:
                 * - event id,
                 * - year, month, day, hour,
                 * - hourly discharge
                 * ******/
                Flood_event_write(p_out, data_Q, id_start, id_end, event_id, p_data);
            } else {
                Flood_event_write_daily(p_out, data_Q, id_start, id_end, event_id, p_data);
            }
            printf("%8d%9d%9d%8.2f\n", event_id, id_start, id_end, *(data_Q + *(index_peak + i)));
            event_id++;

            while (id_end >= *(index_peak + i) && i < n_peaks)
            {
                i += 1;
            }
        }
    }
    printf("-------------- flood events separation: Done!\n");
    return 0;
}

