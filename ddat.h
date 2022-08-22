//
// ddat.h: header for ddat.c routines
//
// Dave Cli, Sept 1996
//

#include <stdlib.h>

// datatype for Real sum and sum of squares, used in calculuating mean and s.d.
typedef struct real_stat {
    Real sum, sumsq;
    int n;
} Real_stat;

// data and stats for a day's trading
typedef struct day_data {
    Real_stat alpha; /*Smith's alpha*/
    Real_stat quant; /*Quantity*/
    Real_stat effic; /*Eciency*/
    Real_stat price; /*price*/
    Real_stat pdisp; /*profit dispersal*/
    Real_stat volty; /*transaction price    volatility*/
} Day_data;

// ddat-init: initialise daily data
void ddat_init(Day_data *);

// ddat-update: update daily data
void ddat_update(Day_data *, int, Real, Real, Real, Real, Real);

// ddat-xgraph: plot the daily stats in xgraph format
void xg_daily_graph(Day_data dd[], int, int, char *);
