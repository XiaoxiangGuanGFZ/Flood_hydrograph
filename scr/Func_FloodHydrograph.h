#ifndef Func_FloodHydrograph
#define Func_FloodHydrograph

#define EPS 1e-9

void Gradient_discharge(
    double *data_Q,
    double **data_G,
    size_t dimLen
);

void Flood_AddNoise(
    double **data_Q,
    size_t dimLen,
    double Q_threshold
);


void Flood_peaks(
    double *data_Q,
    double *data_G,
    int **flag_peak,
    int **index_peak,
    size_t *n_peaks,
    size_t dimLen
);

void Gradient_percentile(
    double *data_G,
    double position_th,
    double *percentile,
    size_t dimLen
);

int peak_independent(
    double *data_Q,
    double *data_G,
    int id1,
    int id2
);

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
);

#endif