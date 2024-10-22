#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include "def_struct.h"
#include "Func_Datetime.h"
#include "Func_DataIO.h"

int check_file_extension(const char *filename) {
    // Find the last occurrence of the dot in the filename
    const char *dot = strrchr(filename, '.');
    
    // Check if dot exists and there is something after the dot
    if (!dot || dot == filename) {
        return 0; // No extension found
    }
    
    // Compare the extension to "nc"
    if (strcmp(dot + 1, "nc") == 0) {
        return 1; // Extension is "nc"
    } else {
        return 0; // Other extension
    }
}


void Data_import(
    char *FP_NC,
    char *varName,
    double **data,
    size_t *dimLen
)
{
    int ncID, varID, dimID;
    nc_open(FP_NC, NC_NOWRITE, &ncID);
    nc_inq_varid(ncID, varName, &varID);
    nc_inq_dimid(ncID, "time", &dimID);
    nc_inq_dimlen(ncID, dimID, dimLen);
    
    *data = (double *)malloc(sizeof(double) * *dimLen);
    nc_get_var_double(ncID, varID, *data);

}

void Data_import_ascii(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t dimLen
)
{
    *data = (double *)malloc(sizeof(double) * dimLen);
    *p_data = (ST_DATA *)malloc(sizeof(ST_DATA) * dimLen);

    FILE *fp;
    if ((fp = fopen(fp_data, "r")) == NULL)
    {
        printf("Cannot open data file: %s\n", fp_data);
        exit(1);
    }
    char *token;
    char row[MAXCHAR];
    char row_first[MAXCHAR];
    int i = 0; // record the number of rows in the data file
    fgets(row_first, MAXCHAR, fp); // skip the first row
    // printf("%s\n", row_first);
    while (fgets(row, MAXCHAR, fp) != NULL && i < dimLen)
    {
        (*p_data + i)->NO = atoi(strtok(row, " "));
        (*p_data + i)->h = atoi(strtok(NULL, " ")); 
        (*p_data + i)->d = atoi(strtok(NULL, " "));
        (*p_data + i)->m = atoi(strtok(NULL, " "));
        (*p_data + i)->y = atoi(strtok(NULL, " "));
        (*p_data + i)->Qobs = atof(strtok(NULL, " "));
        (*p_data + i)->Qsim = atof(strtok(NULL, " "));
        i++;
    }
    fclose(fp);
    if (i > dimLen)
    {
        printf("conflict numbers of lines in data file: %s\n", fp_data);
        exit(1);
    }
    for (size_t i = 0; i < dimLen; i++)
    {
        *(*data + i) = (*p_data + i)->Qsim;
    }
    
}


void Flood_event_write(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    DATETIME DT_start
)
{
    DATETIME DT_ite;
    // time_t t;
    // t = convert_to_time_t(DT_start);

    for (size_t i = id_start; i < id_end + 1; i++)
    {
        /*********
         * date time processing: 
         * add the time lapse from observation beginning time
         * ****/
        //  write to text file
        // fprintf(
        //     p_out,
        //     "%d,%d-%02d-%02d %02d:00,%f\n",
        //     event_id,
        //     DT_ite.year, DT_ite.month, DT_ite.day, DT_ite.hour,
        //     *(data_Q + i)
        // );
        fprintf(
            p_out,
            "%d,%ld,%f\n", event_id, i, *(data_Q + i)
        );
    }
}