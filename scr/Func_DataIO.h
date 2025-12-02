#ifndef FUNC_DATAIO
#define FUNC_DATAIO

int check_file_extension(const char *filename);

void Data_import(
    char *FP_NC,
    char *varName,
    double **data,
    size_t *dimLen
);

size_t Data_import_ascii(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t dimLen
);

void Flood_event_write(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    DATETIME DT_start
);

#endif