#ifndef FUNC_DATETIME
#define FUNC_DATETIME

time_t custom_timegm(struct tm *tm);

time_t convert_to_time_t(DATETIME dt);
DATETIME convert_to_datetime(time_t t);
char *DateString(
    time_t *tm
);

#endif
