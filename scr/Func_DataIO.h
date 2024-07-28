#ifndef FUNC_DATAIO
#define FUNC_DATAIO

void Data_import(
    char *FP_NC,
    char *varName,
    double **data,
    size_t *dimLen
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