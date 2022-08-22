//
// ddat.c: code for handling data/stats compiled at end of each trading day
//
// Dave Cli, Sept 1996
//

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "random.h"
#include "ddat.h"
#include "max.h"

#define SMALLREAL 0.0000001 /*used to dodge rounding errors on sqrt */
#define DD_ALPHA  0
#define DD_QUANT  1
#define DD_EFFIC  2
#define DD_PRICE  3
#define DD_PDISP  4
#define DD_VOLTY  5

// rstat-zero: set everything to zero in one Real-stat structure
void rstat_zero(Real_stat *r) {
    r->sum = 0.0;
    r->sumsq = 0.0;
    r->n = 0;
}

// ddat-init: initialise day data
void ddat_init(Day_data *ddat) {
    rstat_zero(&(ddat->alpha));
    rstat_zero(&(ddat->quant));
    rstat_zero(&(ddat->effic));
    rstat_zero(&(ddat->price));
    rstat_zero(&(ddat->pdisp));
    rstat_zero(&(ddat->volty));
}

// ddat-update: update day data.
void ddat_update(Day_data *dd, int n_deals,
                 Real sum_price, Real alpha, Real pdisp, Real effic, Real pdiff) {
    Real v;
    if (n_deals > 0) {
        (dd->price.sum) += (sum_price / n_deals);
        (dd->price.sumsq) += ((sum_price / n_deals) * (sum_price / n_deals));
        (dd->price.n)++;

        v = sqrt(pdiff / n_deals); /*root     mean square di erence*/
        (dd->volty.sum) += v;
        (dd->volty.sumsq) += (v * v);
        (dd->volty.n)++;

        (dd->alpha.sum) += alpha;
        (dd->alpha.sumsq) += (alpha * alpha);
        (dd->alpha.n)++;

        (dd->effic.sum) += effic;
        (dd->effic.sumsq) += (effic * effic);
        (dd->effic.n)++;

        (dd->quant.sum) += n_deals;
        (dd->quant.sumsq) += (n_deals * n_deals);
        (dd->quant.n)++;

        (dd->pdisp.sum) += pdisp;
        (dd->pdisp.sumsq) += (pdisp * pdisp);
        (dd->pdisp.n)++;
    }
}

// ddat-meanpmsd: plot mean plus and minus one standard deviation
void ddat_meanpmsd(FILE *fp, int field, int n_days, int n_exps, Day_data dd[]) {
    int d, n[MAX_N_DAYS];
    Real mean, meansq, diff, sum[MAX_N_DAYS], sumsq[MAX_N_DAYS];
    char fieldstr[30];

    if (n_days > MAX_N_DAYS) {
        fprintf(stderr, "\nFAIL: MAX_N_DAYS too small in ddat.c: recompile\n");
        exit(0);
    }

    switch (field) {
        case DD_ALPHA:
            strcpy(fieldstr, "Alpha");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].alpha.sum;
                sumsq[d] = dd[d].alpha.sumsq;
                n[d] = dd[d].alpha.n;
            }
            break;

        case DD_QUANT:
            strcpy(fieldstr, "Quantity");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].quant.sum;
                sumsq[d] = dd[d].quant.sumsq;
                n[d] = dd[d].quant.n;
            }
            break;

        case DD_EFFIC:
            strcpy(fieldstr, "Efficiency");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].effic.sum;
                sumsq[d] = dd[d].effic.sumsq;
                n[d] = dd[d].effic.n;
            }
            break;

        case DD_PRICE:
            strcpy(fieldstr, "Price");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].price.sum;
                sumsq[d] = dd[d].price.sumsq;
                n[d] = dd[d].price.n;
            }
            break;

        case DD_PDISP:
            strcpy(fieldstr, "Dispersion");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].pdisp.sum;
                sumsq[d] = dd[d].pdisp.sumsq;
                n[d] = dd[d].pdisp.n;
            }
            break;
        case DD_VOLTY:
            strcpy(fieldstr, "Volatility");
            for (d = 0; d < n_days; d++) {
                sum[d] = dd[d].volty.sum;
                sumsq[d] = dd[d].volty.sumsq;
                n[d] = dd[d].volty.n;
            }
            break;

        default:
            fprintf(stderr, "\nFAIL: bad field in ddat_meanpmsd (%d)\n", field);
            exit(0);
    }

    fprintf(fp, "\" %s (mean)\n", fieldstr);
    for (d = 0; d < n_days; d++) fprintf(fp, "%d %f\n", d + 1, sum[d] / n[d]);
    fprintf(fp, "\n");

    fprintf(fp, "\" %s (-1s.d.)\n", fieldstr);
    for (d = 0; d < n_days; d++) {
        mean = sum[d] / n[d];
        meansq = mean * mean;
        diff = (sumsq[d] / n[d]) - meansq;
        if (diff < SMALLREAL) diff = 0.0;
        fprintf(fp, "%d %f\n", d + 1, mean - sqrt(diff));
    }
    fprintf(fp, "\n");

    fprintf(fp, "\" %s (+1s.d.)\n", fieldstr);
    for (d = 0; d < n_days; d++) {
        mean = sum[d] / n[d];
        meansq = mean * mean;
        diff = (sumsq[d] / n[d]) - meansq;
        if (diff < SMALLREAL) diff = 0.0;
        fprintf(fp, "%d %f\n", d + 1, mean + sqrt(diff));
    }
    fprintf(fp, "\n");
}

// ddat-xgraph: plot the daily stats in xgraph format
void xg_daily_graph(Day_data dd[], int n_days, int n_exps, char *fname) {
    int d;
    FILE *fp;

    fp = fopen(fname, "w");

    fprintf(fp, "TitleText: %s: n=%d\n\n", fname, n_exps);

    if (n_exps < 2) { /*no sense in    calculating SD*/
        fprintf(fp, "\" Alpha\n");
        for (d = 0; d < n_days; d++) fprintf(fp, "%d %f\n", d + 1, dd[d].alpha.sum);
        fprintf(fp, "\n");

        fprintf(fp, "\" Efficiency\n");
        for (d = 0; d < n_days; d++) fprintf(fp, "%d %f\n", d + 1, dd[d].effic.sum);
        fprintf(fp, "\n");
        fprintf(fp, "\" Quantity\n");
        for (d = 0; d < n_days; d++) fprintf(fp, "%d %f\n", d + 1, dd[d].quant.sum);
        fprintf(fp, "\n");

        fprintf(fp, "\" Dispersion\n");
        for (d = 0; d < n_days; d++) fprintf(fp, "%d %f\n", d + 1, dd[d].pdisp.sum);
        fprintf(fp, "\n");
    } else { /*plot mean and s.d. for the daily stats*/
        ddat_meanpmsd(fp, DD_PRICE, n_days, n_exps, dd);
        ddat_meanpmsd(fp, DD_ALPHA, n_days, n_exps, dd);
        ddat_meanpmsd(fp, DD_EFFIC, n_days, n_exps, dd);
        ddat_meanpmsd(fp, DD_QUANT, n_days, n_exps, dd);
        ddat_meanpmsd(fp, DD_PDISP, n_days, n_exps, dd);
        ddat_meanpmsd(fp, DD_VOLTY, n_days, n_exps, dd);
    }
    fclose(fp);
}

