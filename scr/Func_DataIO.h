#ifndef FUNC_DATAIO
#define FUNC_DATAIO


void Data_import_ascii(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t *dimLen
);

void Flood_event_write(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    ST_DATA *p_data
);

void Data_import_ascii_daily(
    char fp_data[],
    double **data,
    ST_DATA **p_data,
    size_t *dimLen
);

void Flood_event_write_daily(
    FILE *p_out,
    double *data_Q,
    int id_start,
    int id_end,
    int event_id,
    ST_DATA *p_data
);

#endif