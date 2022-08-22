//
// sd.c: code for working with stepped supply and demand curves, producing output in x g format
//
// Dave Cli, Sept 1996
//

#include    <math.h>
#include    <stdio.h>
#include    "random.h"
#include    "agent.h"
#include    "max.h"
#include    "sd.h"

#define MAX_PRICES ((MAX_AGENTS)*MAX_UNITS)

// max number of points in a polyline
#define MAX_POINTS (MAX_PRICES*3)

// constants for X g drawing
#define SCALE_FACTOR 1200 /*pixels per inch*/
#define      X_INCHES 7
#define      Y_INCHES 6
#define      LMARGIN_X ((int)(0.15*X_INCHES*SCALE_FACTOR))
#define      LMARGIN_Y ((int)(0.15*Y_INCHES*SCALE_FACTOR))
#define      GRAPH_X    ((int)(0.70*X_INCHES*SCALE_FACTOR))
#define      GRAPH_Y    ((int)(0.70*Y_INCHES*SCALE_FACTOR))
#define      Y_EQ_0     LMARGIN_Y+GRAPH_Y
#define      X_EQ_0     LMARGIN_X+GRAPH_X
#define      AX_THICK 1                       /*thickness of axis lines*/
#define      AX_PTS 18                        /*pointsize for labelling axes*/
#define      TICK_X ((int)(0.025*GRAPH_Y)) /*x tick height*/
#define      TICK_Y ((int)(0.025*GRAPH_X)) /*y tick length*/
#define      X_TICKS 10 /*aim at this number of ticks on the x axis*/
#define      Y_TICKS 10 /*aim at this number of ticks on the y axis*/

#define      MAX_LABELLEN 80 /*maximum number of characters in a label*/
#define      PL_SOLID 0    /*polyline solid linestyle*/
#define      PL_DASHED 1   /*polyline dashed linestyle*/
#define      PL_DOTTED 2   /*polyline dotted linestyle*/
#define      TRI_UP 0
#define      TRI_DOWN 1

#define NULL_EQ -1 /*signifies no equilibrium price/quantity*/


//sort: just bubble
void sort(int order, int field, int n, Real l[][2]) {
    int i, j, swap;
    Real t;

    if ((field < 0) || (field > 1)) {
        fprintf(stderr, "\nFail: bad field=%d in sort\n", field);
        exit(0);
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < i; j++) {
            if (order) { if (l[i][field] > l[j][field]) swap = 1; else swap = 0; }
            else { if (l[i][field] < l[j][field]) swap = 1; else swap = 0; }

            if (swap) {
                t = l[i][0];
                l[i][0] = l[j][0];
                l[j][0] = t;
                t = l[i][1];
                l[i][1] = l[j][1];
                l[j][1] = t;
            }
        }
    }
}

// xf-polyline: draw a polyline in x g
void xf_polyline(FILE *fp, int lstyle, int lthick, Real dlen, int npoints,
                 int coords[MAX_POINTS][2]) {
    int p;

    fprintf(fp, "2 1 %d %d -1 7 0 0 -1 %6.3f 0 0 -1 0 0 %d\n",
            lstyle, lthick, dlen, npoints);
    for (p = 0; p < npoints; p++) { fprintf(fp, "        %d %d\n", coords[p][0], coords[p][1]); }

}

// xf-text: draw some text in x g
void xf_text(FILE *fp, int points, Real angle, int x, int y, char text[]) {
    fprintf(fp, "4 0 -1 0 0 0 %d %f 4 195 135 %d %d %s\\001\n",
            points, angle, x, y, text);
}

// xf-triangle: draw a shaded triangle
void xf_triangle(FILE *fp, int base_x, int base_y, int peak_y, int dx) {
    int shade;

    if (base_y > peak_y) { shade = 15; } /*pointing down*/
    else { shade = 5; } /*pointing up*/

    fprintf(fp, "2 3 0 1 -1 7 0 0 %d 0.000 0 0 -1 0 0 4\n", shade);
    /*peak*/
    fprintf(fp, "         %d %d ", base_x + dx / 2, peak_y);
    /*base*/
    fprintf(fp, "%d %d %d %d ", base_x, base_y, base_x + dx, base_y);
    /*back to the peak*/
    fprintf(fp, "%d %d\n", base_x + dx / 2, peak_y);
}

// setcoords: load values into a coordinate pair
void setcoords(int c[][2], int p, int x, int y) {
    if (p >= MAX_POINTS) {
        fprintf(stderr, "\nFAIL: p=%d >= MAX_POINTS=%d\n", p, MAX_POINTS);
        exit(0);
    }
    c[p][0] = x;
    c[p][1] = y;
}

// neat-ticks: find a "neat" inter-tick interval for axis labelling
int neat_ticks(int range, int max_ticks) {
    int tick_step,
            tmp = 1;
    if (range > max_ticks) {
        tick_step = (int) (floor(0.5 + (range / ((Real) (max_ticks)))));
        /*pick a nice stepsize: this is a bit of a kludge*/
        tmp = 2;
        if (tick_step > 2) tmp = 5;
        if (tick_step > 7) tmp = 10;
        if (tick_step > 11) tmp = 15;
        if (tick_step > 17) tmp = 20;
        if (tick_step > 22) tmp = 25;
        if (tick_step > 30) tmp = 50;
        if (tick_step > 60) tmp = 100;
        if (tick_step > 120) tmp = 150;
        if (tick_step > 170) tmp = 200;
        if (tick_step > 220) tmp = 250;
        if (tick_step > 300) tmp = 500;
        if (tick_step > 600) tmp = 1000;
    }
    tick_step = tmp;
    return (tick_step);
}

// draw-axes: do the price and quantity axes dx and dy are returned with the number of pixels in a unit-step
//   miny is baseline y value, maxy is max y value on graph
void draw_axes(FILE *fp, int min_q, int max_q, Real min_p, Real max_p,
               int eq_p, int eq_q, int surplus, char fname[],
               int *dx, int *dy, int *miny, int *maxy) {
    int t, p,
            tick,
            start,
            tick_step,
            delta,
            imin_p, imax_p,
            coords[MAX_POINTS][2],
            range;
    char labelstr[MAX_LABELLEN];

    /*draw the axes*/
    p = 0;
    setcoords(coords, p++, LMARGIN_X, LMARGIN_Y);
    setcoords(coords, p++, LMARGIN_X, Y_EQ_0);
    setcoords(coords, p++, X_EQ_0, Y_EQ_0);
    xf_polyline(fp, PL_SOLID, AX_THICK, 0.00, p, coords);

    /*horizontal axis: quantity*/
    range = (max_q - min_q);
    tick_step = neat_ticks(range, X_TICKS);
    delta = GRAPH_X / (1 + (range / tick_step));
    (*dx) = delta / tick_step;
    start = min_q - 1;
    if (start < 0) start = 0;

    for (t = start; t <= max_q; t += tick_step) {
        tick = LMARGIN_X + (((t - start) / tick_step) * delta);
        p = 0;
        setcoords(coords, p++, tick, Y_EQ_0);
        setcoords(coords, p++, tick, Y_EQ_0 - TICK_X);
        xf_polyline(fp, PL_SOLID, AX_THICK, 0.00, p, coords);
        if (t > min_q - 1) {
            sprintf(labelstr, "%d", t);
            xf_text(fp, AX_PTS, 0.0, tick, Y_EQ_0 + 2 * TICK_X, labelstr);
        }
    }

    sprintf(labelstr, "Quantity");
    xf_text(fp, AX_PTS, 0.0, LMARGIN_X + (int) (GRAPH_X * 0.4), Y_EQ_0 + 4 * TICK_X, labelstr);

    /*vertical axis:price*/
    imin_p = (int) (100 * min_p);
    imax_p = (int) (100 * max_p);
    range = imax_p - imin_p;
    tick_step = neat_ticks(range, Y_TICKS);

    /* fiddle imin-p and i-maxp to make them integer multiples of tick-step*/
    imin_p = (imin_p / tick_step) * tick_step;         /*integer division: lower bound*/
    imax_p = (1 + (imax_p / tick_step)) * tick_step; /*integer division: upper bound*/
    range = imax_p - imin_p;
    *miny = imin_p;
    *maxy = imax_p;
    delta = GRAPH_Y / (range / tick_step);
    *dy = GRAPH_Y / range;
    delta = (*miny) * (*dy);

    for (t = imin_p; t <= imax_p; t += tick_step) {
        tick = Y_EQ_0 - (t * (*dy)) + delta;
        p = 0;
        setcoords(coords, p++, LMARGIN_X, tick);
        setcoords(coords, p++, LMARGIN_X + TICK_Y, tick);
        xf_polyline(fp, PL_SOLID, AX_THICK, 0.00, p, coords);
        sprintf(labelstr, "%d", t);
        xf_text(fp, AX_PTS, 0.0, LMARGIN_X - 4 * TICK_Y, tick, labelstr);
    }

    /*annotate with key values*/
    if (eq_q == NULL_EQ) {
        sprintf(labelstr,
                "Price                        Eq.Price=<->    Eq.Quant=%2d     Surplus=%4d",
                0, 0);
    } else {
        sprintf(labelstr,
                "Price                         Eq.Price=%3d    Eq.Quant=%2d     Surplus=%4d",
                eq_p, eq_q, surplus);
    }

    xf_text(fp, AX_PTS, 0.0, LMARGIN_X - 4 * TICK_Y, tick - 4 * TICK_Y, labelstr);
    /*add the lename for reference, but in small text*/
    sprintf(labelstr, "%s", fname);
    xf_text(fp, AX_PTS / 2, 0.0, LMARGIN_X - 4 * TICK_Y, tick - 6 * TICK_Y, fname);
}

// supdem: from a list of supplier prices and a list of demander prices, return values: equilibrium price,
// equilibrium quantity, max surplus. The max surplus figure is integrated from 1 to max-trades, rather than 1 to max(nb,ns)
// - the maximum total profit that could have been earned is dependent on how many trades are allowed.
void supdem(int ns, Agent sellers[], int nb, Agent buyers[], int max_trades,
            Real *ep, int *iq, Real *surplus, int field, char fname[],
            Real *bounds, int verbose) {
    int maxn, a, s, b, no_intersect, not_found,
            q; /*quantity*/
    Real sp[MAX_PRICES][2], /*seller limit and quote prices*/
    bp[MAX_PRICES][2], /*buyer limit and quote prices*/
    profit, tot_surp,
            maxprice, minprice;
    FILE *fp;

    /*these declarations are for the xg drawing stuff */
    int p, /*point index*/
    min_q, max_q, /*minimum and maximum quantities on graph*/
    dx, dy, tx, ty, miny, fy, maxy,
            coords[MAX_POINTS][2]; /*coordinate points in polyline etc*/
    char labelstr[MAX_LABELLEN];

    *ep = -1.0;
    *iq = NULL_EQ;

    if (((nb * MAX_UNITS) > MAX_PRICES) || ((ns * MAX_UNITS) > MAX_PRICES)) {
        fprintf(stderr, "\nFail: too many units in supdem() -- recompile\n");
        exit(0);
    }

    s = 0;
    for (a = 0; a < ns; a++) {
        if (sellers[a].active) {
            for (q = 0; q < sellers[a].quant; q++) {
                sp[s][0] = sellers[a].limit;
                sp[s][1] = sellers[a].price;
                if (s == 0) {
                    maxprice = sellers[a].price;
                    minprice = sellers[a].limit;
                }
                else { /*for sellers, limit<=price*/
                    if (sellers[a].price > maxprice) maxprice = sellers[a].price;
                    if (sellers[a].limit < minprice) minprice = sellers[a].limit;
                }
                s++;
            }
        }
    }

    b = 0;
    for (a = 0; a < nb; a++) {
        if (buyers[a].active) {
            for (q = 0; q < buyers[a].quant; q++) {
                bp[b][0] = buyers[a].limit;
                bp[b][1] = buyers[a].price;
                /*for buyers, limit>=price*/
                if (buyers[a].limit > maxprice) maxprice = buyers[a].limit;
                if (buyers[a].price < minprice) minprice = buyers[a].price;
                b++;
            }
        }
    }

    sort(1, field, b, bp);
    sort(0, field, s, sp);

    maxn = (s > b ? s : b);
    min_q = 1;
    max_q = maxn;

    if (verbose) {
        fprintf(stdout, "Max_trades=%d\n", max_trades);
        fprintf(stdout, "Minprice=%f maxprice=%f min_q=%d max_q=%d\n",
                minprice, maxprice, min_q, max_q);
    }

    if (bounds != NULL) { /*autoscaling is OFF*/
        min_q = (int) (*bounds);
        max_q = (int) (*(bounds + 1));
        minprice = *(bounds + 2);
        maxprice = *(bounds + 3);
        if (verbose) {
            fprintf(stdout, "Autoscaling is OFF. Bounds are:\n");
            fprintf(stdout, "Minprice=%f maxprice=%f min_q=%d max_q=%d\n",
                    minprice, maxprice, min_q, max_q);
        }
    }

    tot_surp = 0.0;
    if (sp[0][field] > bp[0][field]) { /*lowest selling price is larger than   highest buying price*/
        no_intersect = 1;
    } else { /* find intersect point*/
        no_intersect = 0;
        not_found = 1;

        for (q = 0; q < maxn; q++) { /*intersection?*/
            profit = bp[q][field] - sp[q][field];
            if (not_found) {
                if (sp[q][field] > bp[q][field]) { /*straightforward intersect*/
                    *ep = (sp[q - 1][field] + bp[q - 1][field]) / 2.0;
                    *iq = q;
                    not_found = 0;
                } else {
                    if ((q + 1 == s) && (q + 1 == b)) { /*last buyer and seller*/
                        *ep = (sp[q][field] + bp[q][field]) / 2.0;
                        *iq = q + 1;
                        if (q < max_trades) tot_surp += profit;
                        not_found = 0;
                    } else {
                        if ((q + 1) == s) { /*run out of active sellers but still some buyers*/
                            *ep = (bp[q][field] + bp[q + 1][field]) / 2.0;
                            *iq = q + 1;
                            if (q < max_trades) tot_surp += profit;
                            not_found = 0;
                        } else {
                            if ((q + 1) == b) { /*run out of active buyers but still some sellers*/
                                (*ep) = (sp[q][field] + sp[q + 1][field]) / 2.0;
                                (*iq) = q + 1;
                                if (q < max_trades) tot_surp += profit;
                                not_found = 0;
                            }
                        }
                    }
                }

                if (not_found) {
                    if (q < max_trades) {
                        tot_surp += profit;
                    }
                }
            }

            if (verbose) {
                fprintf(stdout, "quantity %2d ", q + 1);
                if (q < s) fprintf(stdout, "supply=%5.3f ", sp[q][field]);
                else fprintf(stdout, "             ");

                if (q < b) fprintf(stdout, "demand=%5.3f ", bp[q][field]);
                else fprintf(stdout, "             ");

                fprintf(stdout, "profit=%f cum.surp=%f ", profit, tot_surp);
                fprintf(stdout, "\n");
            }
        }
    }

    *surplus = tot_surp;

    if (verbose) {
        switch (field) {
            case EQ_THEORY:
                fprintf(stdout, "Theoretical");
                break;
            case EQ_ACTUAL:
                fprintf(stdout, "Actual");
                break;
            default:
                fprintf(stderr, "\nFail: bad field=%d in supdem\n", field);
                exit(0);
        }
        fprintf(stdout, " equilibrium price=%f at %d; max surplus=%f\n",
                *ep, *iq, *surplus);
    }

    if (fname[0] != '\0') { /*write an x g le*/
        fp = fopen(fname, "w");

        /*do the preamble*/
        fprintf(fp, "#FIG 3.1\nLandscape\nCenter\nInches\n1200 2\n");

        /*do the axes tickmarks and labelling*/
        draw_axes(fp, min_q, max_q, minprice, maxprice, (int) floor((*ep * 100) + 0.5),
                  *iq, (int) floor((*surplus * 100) + 0.5), fname,
                  &dx, &dy, &miny, &maxy);

        /*do the supply triangles and build the supply curve*/
        p = 0;
        for (q = 0; q < s; q++) {
            tx = LMARGIN_X + (q * dx);
            xf_triangle(fp, tx, Y_EQ_0 - (int) (sp[q][0] * dy * 100) + (miny * dy),
                        Y_EQ_0 - (int) (sp[q][1] * dy * 100) + (miny * dy), dx);
            fy = Y_EQ_0 - (int) (sp[q][field] * dy * 100) + (miny * dy);
            setcoords(coords, p++, tx, fy);
            setcoords(coords, p++, tx + dx, fy);
        }
        setcoords(coords, p++, tx + dx, Y_EQ_0 - (maxy * dy) + (miny * dy));
        /*do the supply curve*/
        xf_polyline(fp, PL_SOLID, AX_THICK, 0.00, p, coords);

        /*do the demand triangles and build the demand curve*/
        p = 0;
        for (q = 0; q < b; q++) {
            tx = LMARGIN_X + (q * dx);
            xf_triangle(fp, tx, Y_EQ_0 - (int) (bp[q][0] * dy * 100) + (miny * dy),
                        Y_EQ_0 - (int) (bp[q][1] * dy * 100) + (miny * dy), dx);
            fy = Y_EQ_0 - (int) (bp[q][field] * dy * 100) + (miny * dy);
            setcoords(coords, p++, tx, fy);
            setcoords(coords, p++, tx + dx, fy);
        }
        setcoords(coords, p++, tx + dx, Y_EQ_0);
        xf_polyline(fp, PL_SOLID, AX_THICK, 0.00, p, coords);

        /*equilibrium price and quantity*/
        if (!no_intersect) {
            p = 0;
            setcoords(coords, p++, LMARGIN_X, Y_EQ_0 - ((*ep) * dy * 100) + (miny * dy));
            setcoords(coords, p++, tx + dx, Y_EQ_0 - ((*ep) * dy * 100) + (miny * dy));
            xf_polyline(fp, PL_DASHED, AX_THICK, 4.00, p, coords);

            p = 0;
            setcoords(coords, p++, LMARGIN_X + ((*iq) * dx), Y_EQ_0 - ((*ep) * dy * 100) + (miny * dy));
            setcoords(coords, p++, LMARGIN_X + ((*iq) * dx), Y_EQ_0);
            xf_polyline(fp, PL_DASHED, AX_THICK, 4.00, p, coords);
        }

        fclose(fp);
    }
}

