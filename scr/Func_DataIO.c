#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include "def_struct.h"
#include "Func_Datetime.h"
#include "Func_DataIO.h"

void Data_import_ascii(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t *dimLen
)
{
    /********
     * // Function: Data_import_ascii
        // Purpose: 
        //   Reads time series data from a CSV file and stores it into two structures:
        //   1) ST_DATA array (structured data: year, month, day, hour, discharge)
        //   2) double array (only discharge values for further processing)
        //
        // Inputs:
        //   fp_data  - path to input CSV file
        //   data     - pointer to array of doubles (output: Qsim values)
        //   p_data   - pointer to array of ST_DATA structs (output: full records)
        //   dimLen   - expected number of data rows
        //
        // Notes:
        //   - Assumes CSV format: Year,Month,Day,Hour,Discharge
        //   - First row is skipped (header)
        //   - Uses strtok for parsing (modifies input buffer)
     * *****/

    // Allocate memory for output arrays
    *data = (double *)malloc(sizeof(double) * *dimLen);
    *p_data = (ST_DATA *)malloc(sizeof(ST_DATA) * *dimLen);

    // Open file for reading
    FILE *fp;
    if ((fp = fopen(fp_data, "r")) == NULL)
    {
        printf("Cannot open data file: %s\n", fp_data);
        exit(1);
    }
    char *token;            // pointer for tokenized strings (unused but kept for clarity)
    char row[MAXCHAR];      // buffer for each line
    char row_first[MAXCHAR];// buffer for header line
    
    int i = 0; // record the number of rows in the data file, counter for number of rows read
    
    fgets(row_first, MAXCHAR, fp); // Skip header line (column names)
    
    while (fgets(row, MAXCHAR, fp) != NULL && i < *dimLen)
    {
        // Parse CSV fields (comma-separated)
        // strtok modifies 'row', so order matters

        (*p_data + i)->y = atoi(strtok(row, ","));
        (*p_data + i)->m = atoi(strtok(NULL, ","));
        (*p_data + i)->d = atoi(strtok(NULL, ","));
        (*p_data + i)->h = atoi(strtok(NULL, ",")); 
        (*p_data + i)->Q = atof(strtok(NULL, ","));
        i++;  // move to next record
    }
    fclose(fp);

    // Check if more rows were read than expected (sanity check)
    if (i > *dimLen)
    {
        printf("conflict numbers of lines in data file: %s\n", fp_data);
        exit(1);
    }

    *dimLen = i; // number of rows of valid observation
    
    // Extract only Qsim values into separate double array
    for (size_t j = 0; j < *dimLen; j++)
    {
        *(*data + j) = (*p_data + j)->Q;
    }
    
}


void Flood_event_write(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    ST_DATA *p_data
)
{
    /***********************
     *  Function: Flood_event_write
        Purpose:
        Writes a detected flood event time series to an output file.
        Each row contains event ID, timestamp (year, month, day, hour),
        and corresponding discharge value.
        
        Inputs:
        p_out     - file pointer to output file (already opened)
        data_Q    - array of discharge values (e.g., Qsim)
        id_start  - starting index of the flood event (inclusive)
        id_end    - ending index of the flood event (inclusive)
        event_id  - identifier for the flood event
        p_data    - array of ST_DATA structs containing date-time info
        
        Output format (CSV):
        event_id, year, month, day, hour, discharge
        
        Notes:
        - Assumes id_start <= id_end
        - Assumes indices are within bounds of allocated arrays
        - Uses pointer arithmetic for performance

     *********************/


    // Loop through all time steps belonging to the flood event
    for (size_t i = id_start; i < id_end + 1; i++)
    {
        
        fprintf(
            p_out,
            "%d,%d,%d,%d,%d,%f\n", 
            event_id, 
            (p_data + i)->y, (p_data + i)->m, (p_data + i)->d, (p_data + i)->h, 
            *(data_Q + i)
        );
    }
}

void Data_import_ascii_daily(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t *dimLen
)
{
    /********
     * Function: Data_import_ascii_daily
     * Purpose:
     *   Reads daily discharge time series data from a CSV file.
     *   Stores the data in two forms:
     *     1) ST_DATA array (structured date + discharge)
     *     2) double array (discharge values only)
     *
     * Input:
     *   fp_data  - path to input CSV file
     *   data     - pointer to output array (discharge values)
     *   p_data   - pointer to output array (structured data)
     *   dimLen   - maximum expected number of rows (input),
     *              updated to actual number of rows (output)
     *
     * Expected CSV format:
     *   Year,Month,Day,Discharge
     *
     * Notes:
     *   - First row is assumed to be header and skipped
     *   - Hour field is set to 0 (since data is daily)
     *   - Uses strtok(), which modifies the input row buffer
     *   - Memory is allocated inside the function
     *********/
    *data = malloc(sizeof(double) * *dimLen);
    *p_data = malloc(sizeof(ST_DATA) * *dimLen);

    FILE *fp = fopen(fp_data, "r");
    if (fp == NULL) {
        printf("Cannot open data file: %s\n", fp_data);
        exit(1);
    }

    char row[MAXCHAR];
    char header[MAXCHAR];
    int i = 0;

    fgets(header, MAXCHAR, fp);

    while (fgets(row, MAXCHAR, fp) != NULL && i < *dimLen) {
        (*p_data + i)->y = atoi(strtok(row, ","));
        (*p_data + i)->m = atoi(strtok(NULL, ","));
        (*p_data + i)->d = atoi(strtok(NULL, ","));
        (*p_data + i)->h = 0;   // daily data: fixed hour
        (*p_data + i)->Q = atof(strtok(NULL, ","));
        i++;
    }

    fclose(fp);
    *dimLen = i;

    for (size_t j = 0; j < *dimLen; j++) {
        *(*data + j) = (*p_data + j)->Q;
    }
}


void Flood_event_write_daily(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    ST_DATA *p_data
)
{
    /********
     * Function: Flood_event_write_daily
     * Purpose:
     *   Writes a detected flood event (daily resolution)
     *   to an output file in CSV format.
     *
     * Output format:
     *   event_id,year,month,day,discharge
     *
     * Input:
     *   p_out     - pointer to output file (already opened)
     *   data_Q    - array of discharge values
     *   id_start  - start index of the event (inclusive)
     *   id_end    - end index of the event (inclusive)
     *   event_id  - identifier for the flood event
     *   p_data    - structured time data (year, month, day)
     *
     * Notes:
     *   - Hour field is omitted (daily data)
     *   - Assumes id_start <= id_end and valid indices
     *   - Uses pointer arithmetic for efficiency
     *********/
    for (size_t i = id_start; i < id_end + 1; i++) {
        fprintf(
            p_out,
            "%d,%d,%d,%d,%f\n",
            event_id,
            (p_data + i)->y,
            (p_data + i)->m,
            (p_data + i)->d,
            *(data_Q + i)
        );
    }
}

