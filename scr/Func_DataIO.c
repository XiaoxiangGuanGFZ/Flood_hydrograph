#include <stdio.h>
#include <netcdf.h>
#include <time.h>
#include "def_struct.h"
#include "Func_Datetime.h"

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