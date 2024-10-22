
#ifndef DEFSTRUCT
#define DEFSTRUCT

#define MAXCHAR 3000

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
} DATETIME;


typedef struct
{
    int NO;
    int h;
    int d;
    int m;
    int y;
    double Qobs;
    double Qsim;
} ST_DATA;

#endif