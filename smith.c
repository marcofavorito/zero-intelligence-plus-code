//
// smith.c: the master program
// 
// Dave Cli, Sept 1996
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include   "max.h"
#include   "random.h"
#include   "agent.h"
#include   "sd.h"
#include   "ddat.h"
#include   "tdat.h"
#include   "expctl.h"

// reward: monetary reward for a deal
Real reward(Agent *a, Real price) {
    Real r;
    if ((a->job) == SELL) { r = ((price - (a->limit))); }
    else { r = (((a->limit) - price)); }

    if (r < 0.0) r = 0.0;

    return (r);
}

// get-price: get a price from an agent)
Real get_price(Agent *a, int id, int random, int verbose) {
    Real price;
    Real rmin = 0.01, rmax = 4.0; /*bounds on random prices*/

    if (random) { /*agent price is generated at random*/
        if (rmax < a->limit) {
            fprintf(stderr, "\nFail: rmax too low in get_price()\n");
            exit(0);
        }

        if (a->job == BUY) price = rmin + randval((a->limit) - rmin);
        else price = (a->limit) + randval(rmax - (a->limit));
        price = (floor(0.5 + (price * 100))) / 100;
        a->price = price;
    } else price = a->price;

    if (verbose) {
        if (a->job == BUY)
            fprintf(stdout, "Buyer %d bids at %5.3f (reward=%5.3f)\n",
                    id, price, reward(a, price));
        else
            fprintf(stdout, "Seller %d offers at %5.3f (reward=%5.3f)\n",
                    id, price, reward(a, price));
    }
    return (price);
}

// get-willing: form a list of agents willing to deal
int get_willing(Real price, Agent agents[], int n, int ilist[], char *s, int random,
                int verbose) {
    int willing = 0, a;
    Real r_price, p;

    p = price;

    for (a = 0; a < n; a++) {
        if (random) { /*agent generates a price at random, compares it   to given price*/
            /*and is willing if random price makes a profit*/
            agents[a].willing = 0;

            if (agents[a].active) {
                r_price = get_price(agents + a, a, random, verbose);
                if (agents[a].job == BUY) {
                    if (r_price > price) {
                        agents[a].willing = 1;
                        p = r_price;
                    }
                } else {
                    if (r_price < price) {
                        agents[a].willing = 1;
                        p = r_price;
                    }
                }
            }
        } else { /*use some intelligence*/
            willing_trade(agents + a, price);
        }

        if (agents[a].willing) {
            ilist[willing] = a;
            willing++;

            if (verbose) {
                fprintf(stdout, "%s%2d willing (r)price=%5.3f reward=%5.3f\n",
                        s, a, p, reward(agents + a, price));
            }
        }
    }
    if (verbose) fprintf(stdout, "%d traders willing to deal\n", willing);

    return (willing);
}

// get-able: form a list of agents able to deal
int get_able(Real price, Agent agents[], int n, int ilist[], char *s, int verbose) {
    int able = 0, a;

    for (a = 0; a < n; a++) {
        if (agents[a].able) {
            ilist[able] = a;
            able++;
            if (verbose) {
                fprintf(stdout, "%s%2d able (reward=%5.3f)\n",
                        s, a, reward(agents + a, price));
            }
        }
    }
    return (able);
}

// bank: adjust bank balances of buyer and seller in a deal
void bank(Agent *s, Agent *b, Real price, Real *surplus, int verbose) {
    Real r;

    /*seller*/
    r = reward(s, price);
    (s->bank) += r;
    (s->a_gain) += r;
    (*surplus) += (r);

    (s->quant)--;
    if (s->quant < 1) s->active = 0;
    if (verbose) {
        fprintf(stdout, "Seller: limit=%f reward=%f bank=%f quant=%d (surp=%f)\n",
                s->limit, r, s->bank, s->quant, *surplus);
    }

    /*buyer*/
    r = reward(b, price);
    (b->bank) += r;
    (b->a_gain) += r;
    (*surplus) += (r);

    (b->quant)--;
    if (b->quant < 1) b->active = 0;
    if (verbose) {
        fprintf(stdout, "Buyer: limit=%f reward=%f bank=%f quant=%d (surp=%f)\n",
                b->limit, r, b->bank, b->quant, *surplus);
    }
}


// day-init: initialise all data structures for start of day
void day_init(int exp_number, int day_number, Day_data *ddat, Expctl *ec,
              Agent sellers[], Agent buyers[],
              Real *p_0, Real *max_surplus, int verbose) {
    int b, s, q_0, s_sched, d_sched, n_buy, n_sell;
    Real eq_profit;
    char filename[40];

    /*initialise the buyers*/
    if (day_number == 0) { /* first day: read the first demand schedule*/
        ec->d_sched = 0;
    } else if ((day_number - 1) ==
               (ec->dem_sched[ec->d_sched].last_day)) { /*previous day was last day on that demand schedule: update*/
        (ec->d_sched)++;
        if (ec->d_sched == ec->n_dem_sched) {
            fprintf(stderr, "\nFail: ran out of demand schedules on day %d\n",
                    day_number);
            exit(0);
        }
    }
    d_sched = ec->d_sched;
    n_buy = ec->dem_sched[d_sched].n_agents;

    /*mark all buyers active, set quantities and limit prices*/
    for (b = 0; b < n_buy; b++) {
        buyers[b].quant = ec->dem_sched[d_sched].agents[b].n_units;
        buyers[b].active = 1;
        buyers[b].a_gain = 0.0;
        /*NOTE: ONLY ALLOWS FOR ONE LIMIT PRICE*/
        buyers[b].limit = ec->dem_sched[d_sched].agents[b].limit[0];
        set_price(buyers + b);
        if (verbose) fprintf(stdout, "buyer %d price %f\n", b, buyers[b].price);
    }

    /*initialise the sellers*/
    if (day_number == 0) { /* first day: read the first demand schedule*/
        ec->s_sched = 0;
    } else if ((day_number - 1) ==
               (ec->sup_sched[ec->s_sched].last_day)) { /*previous day was last day on that supply schedule: update*/
        (ec->s_sched)++;
        if (ec->s_sched == ec->n_sup_sched) {
            fprintf(stderr, "\nFail: ran out of supply schedules on day %d\n",
                    day_number);
            exit(0);
        }
    }
    s_sched = ec->s_sched;
    n_sell = ec->sup_sched[s_sched].n_agents;

    /*mark all sellers active, set quantities and limit prices*/
    for (s = 0; s < n_sell; s++) {
        sellers[s].quant = ec->sup_sched[s_sched].agents[s].n_units;
        sellers[s].active = 1;
        sellers[s].a_gain = 0.0;
        /*NOTE: ONLY ALLOWS FOR ONE LIMIT PRICE*/
        sellers[s].limit = ec->sup_sched[s_sched].agents[s].limit[0];
        set_price(sellers + s);
        if (verbose) fprintf(stdout, "seller %d price %f\n", s, sellers[s].price);
    }

    /* find theoretical equilibrium price*/
    if (exp_number == 0) sprintf(filename, "%ssd%02d_000.fig", ec->id, day_number + 1);
    else sprintf(filename, "\0");
    supdem(n_sell, sellers, n_buy, buyers,
           ec->max_trades, p_0, &q_0, max_surplus, EQ_THEORY, filename,
           NULL, verbose);

    /*set theoretical gains for buyers and sellers*/
    for (b = 0; b < n_buy; b++) {
        eq_profit = buyers[b].quant * (buyers[b].limit - (*p_0));
        if (eq_profit < 0.0) eq_profit = 0.0;
        buyers[b].t_gain = eq_profit;
    }
    for (s = 0; s < n_sell; s++) {
        eq_profit = sellers[s].quant * ((*p_0) - sellers[s].limit);
        if (eq_profit < 0.0) eq_profit = 0.0;
        sellers[s].t_gain = eq_profit;
    }
}

// trade: see if a buyer and a seller can be found who will enter into a trade
void trade(Trade_data *tdat, Agent sellers[], Agent buyers[], Expctl *ec,
           Real max_surplus, Real *surplus, int *stat, int verbose) {
    int b, s,        /*buyer and seller indices*/
    dt,         /*deal type*/
    status,     /*what's happening*/
    eq_q,       /*equilibrium quantity*/
    n_willing, /*number of agents willing to trade at a given price*/
    n_able,     /*number of agents able to trade at a given price*/
    n_fails,    /*number of failed/declined bids/offers*/
    n_buy,      /*number of buyers*/
    n_sell,     /*number of sellers*/
    active_b,   /*number of active buyers*/
    active_s,   /*number of active sellers*/
    sell_shout, /*can sellers shout offers?*/
    buy_shout, /*can buyers shout bids?*/
    traders,    /*number of traders to choose from when generating shout*/
    first_offer,/* ag raised until an opening offer is made*/
    first_bid, /*falg raised unitl an opening bid is made*/
    ilist[MAX_AGENTS]; /*list of indices*/

    Real eq_p,      /*equilibrium price*/
    cur_surp, /*current actual max surplus*/
    best_offer,/*used in NYSE rules*/
    best_bid, /*used in NYSE rules*/
    price;     /*price of bid/ask*/

    n_buy = ec->dem_sched[ec->d_sched].n_agents;
    n_sell = ec->sup_sched[ec->s_sched].n_agents;
    sell_shout = ec->sup_sched[ec->s_sched].can_shout;
    buy_shout = ec->dem_sched[ec->d_sched].can_shout;

    if ((sell_shout == 0) && (buy_shout == 0)) {
        fprintf(stderr, "\nFAIL: Can't have both buyers AND sellers silent\n");
        exit(0);
    }

    /* find the theoretical equilibrium price*/
    supdem(n_sell, sellers, n_buy, buyers, ec->max_trades,
           &eq_p, &eq_q, &cur_surp, EQ_THEORY,
           "\0", NULL, verbose);
    if (eq_q != NULL_EQ) {
        tdat->t_eq_p = eq_p;
        tdat->t_eq_q = eq_q;
    } else tdat->t_eq_q = NULL_EQ;

    /* find the actual equilibrium price*/
    supdem(n_sell, sellers, n_buy, buyers, ec->max_trades,
           &eq_p, &eq_q, &cur_surp, EQ_ACTUAL,
           "\0", NULL, verbose);
    if (eq_q != NULL_EQ) {
        tdat->a_eq_p = eq_p;
        tdat->a_eq_q = eq_q;
    } else tdat->a_eq_q = NULL_EQ;

    n_fails = 0;
    status = NO_DEAL;
    first_offer = 1;
    first_bid = 1;
    while ((status == NO_DEAL) && (n_fails < MAX_FAILS)) {
        /*count active agents and mark them as able to bid*/
        active_b = 0;
        for (b = 0; b < n_buy; b++) {
            if (buyers[b].active) {
                buyers[b].able = 1;
                active_b++;
            } else buyers[b].able = 0;
        }
        active_s = 0;
        for (s = 0; s < n_sell; s++) {
            if (sellers[s].active) {
                sellers[s].able = 1;
                active_s++;
            } else sellers[s].able = 0;
        }

        traders = 0;
        if (sell_shout) traders += active_s;
        if (buy_shout) traders += active_b;

        if (verbose)
            fprintf(stdout, "%d traders: active_s=%d active_b=%d\n",
                    traders, active_s, active_b);

        if (irand(traders) < active_s) { /*is there a seller able to make   an offer?*/
            dt = OFFER;

            if (ec->nyse && (!first_offer)) {
                if (ec->random) { /*any seller with a limit price higher than best offer can't deal*/
                    for (s = 0; s < n_sell; s++) { if (sellers[s].limit > best_offer) sellers[s].able = 0; }
                } else { /*any seller with an equal or higher price can't offer*/
                    for (s = 0; s < n_sell; s++) { if (sellers[s].price >= best_offer) sellers[s].able = 0; }
                }
            }
            n_able = get_able(0.0, sellers, n_sell, ilist, "S", verbose);

            if (n_able > 0) { /*an able seller makes an offer*/
                s = ilist[irand(n_able)];
                /*get price for seller*/
                price = get_price(sellers + s, s, ec->random, verbose);
                if (ec->nyse) {
                    if (first_offer) {
                        best_offer = price;
                        first_offer = 0;
                    } else { if (price < best_offer) best_offer = price; }
                }

                /*get willing buyers*/
                n_willing = get_willing(price, buyers, n_buy, ilist, "B", ec->random, verbose);
                if (n_willing > 0) status = DEAL;
            } else {
                if (verbose) fprintf(stdout, "No sellers able to offer\n");
                n_fails = MAX_FAILS;
                status = END_DAY;
            }
        } else { /*is there a buyer able to make a bid?*/
            dt = BID;
            if (ec->nyse && (!first_bid)) {
                if (ec->random) { /*any buyer with limit lower than best bid can't deal*/
                    for (b = 0; b < n_buy; b++) { if (buyers[b].limit < best_bid) buyers[b].able = 0; }
                } else { /*any buyer with an equal or lower price can't bid*/
                    for (b = 0; b < n_buy; b++) { if (buyers[b].price <= best_bid) buyers[b].able = 0; }
                }
            }
            n_able = get_able(0.0, buyers, n_buy, ilist, "B", verbose);

            if (n_able > 0) { /*an able buyer makes a bid*/
                b = ilist[irand(n_able)];

                /*get price for buyer*/
                price = get_price(buyers + b, b, ec->random, verbose);
                if (ec->nyse) {
                    if (first_bid) {
                        best_bid = price;
                        first_bid = 0;
                    } else { if (price > best_bid) best_bid = price; }
                }

                /*get willing selllers*/
                n_willing = get_willing(price, sellers, n_sell, ilist, "S", ec->random, verbose);
                if (n_willing > 0) status = DEAL;
            } else {
                if (verbose) fprintf(stdout, "No buyers able to bid\n");
                n_fails = MAX_FAILS;
                status = END_DAY;
            }
        }

        if (status == DEAL) { /*DEAL*/
            if (dt == OFFER) { /*select the willing buyer for this offer*/
                b = ilist[irand(n_willing)];
                if (verbose) {
                    fprintf(stdout,
                            "Seller %d sells to Buyer %d (reward=%5.3f)\n",
                            s, b, reward(buyers + b, price));
                }
            } else { /*select the willing seller for this bid*/
                s = ilist[irand(n_willing)];
                if (verbose) {
                    fprintf(stdout,
                            "Buyer %d buys from Seller %d (reward=%5.3f)\n",
                            b, s, reward(sellers + s, price));
                }
            }

            /*record what happened*/
            tdat->deal_p = price;
            tdat->deal_t = dt;

            /*update trading strategies of buyers and sellers*/
            shout_update(dt, status, n_sell, sellers, n_buy, buyers, price, verbose);

            /*update bank accounts of buyer and seller*/
            bank(sellers + s, buyers + b, price, surplus, verbose);
        } else { /*NO DEAL or END DAY*/
            n_fails++;
            if (verbose) fprintf(stdout, "No willing takers (fails=%d)\n", n_fails);
            tdat->deal_p = -1.0; /*negative price => no deal*/

            /*update trading strategies of buyers and sellers*/
            shout_update(dt, status, n_sell, sellers, n_buy, buyers, price, verbose);
        }
    }
    *stat = status;
}


int main(int argc, char *argv[]) {
    int n_trans,    /*number of transactions on a day*/
    d,          /*day*/
    status,     /*how things are going*/
    rs,         /*random seed*/
    s, b,        /*seller, buyer, loop indices*/
    n_buy,      /*number of buyers*/
    n_sell,     /*number of sellers*/
    n_trades,   /*number of trades done in a day*/
    n_exps,     /*number of experiments to run*/
    max_trades, /*maxmimum number of trades in a session*/
    verbose = 1,
            ats_n[MAX_TRADES], /*counts of entries in ats[]*/
    eq,         /*equilibrium quantity*/
    dummy_i,    /*dummy integer*/
    e,          /*experiment number*/
    t;          /*transaction number within a day*/
    Real price, p_0, sigmasum, alpha, ep, last_price, sum_price_diff,
            dummy_r1, dummy_r2,
            sum_price,
            diff, pd, pdisp, pds,
            alphatrans,       /*alpha over transaction sequence (cf G+S g6)*/
    ats[MAX_TRADES], /*alpha over transaction sequence (cf G+S g6)*/
    bounddata[4],    /*can be used to inhibit autoscaling on supdem*/
    *bounds,
            max_surplus, surplus, efficiency;
    char fname[60];
    Day_data ddat[MAX_N_DAYS];
    Trade_data tdat[MAX_N_DAYS][MAX_TRADES];
    Real_stat ats_e[MAX_TRADES]; /*for summarising ats[] over experiments*/
    Agent buyers[MAX_AGENTS],
            sellers[MAX_AGENTS];
    Expctl expctl;
    FILE *fp;

    if (argc < 3) {
        fprintf(stderr, "\nUsage: smith <n_exps> <datafilename>\n");
        exit(0);
    }
    sscanf(argv[1], "%d", &n_exps);
    fprintf(stdout, "%d experiments, data-file=%s\n", n_exps, argv[2]);

    if (n_exps == 1) rs = 0;
    else rs = 999;

    rseed(&rs);

    expctl_in(argv[2], &expctl, 1);

    /*initialise daily data records*/
    for (d = 0; d < expctl.n_days; d++) ddat_init(ddat + d);

    for (t = 0; t < MAX_TRADES; t++) {
        ats_n[t] = 0;
        ats[t] = 0.0;
        ats_e[t].n = 0;
        ats_e[t].sum = 0.0;
        ats_e[t].sumsq = 0.0;
    }

    for (e = 0; e < n_exps; e++) { /*do one experiment*/

        buy_init(buyers, verbose);
        sell_init(sellers, verbose);

        for (d = 0; d < expctl.n_days; d++) { /*one trading period or "day"*/

            /*set maximum number of trades in this day*/
            max_trades = expctl.max_trades;
            if (verbose) fprintf(stdout, "\nday %d: %d trades\n", d + 1, max_trades);

            /*set things up for the start of the day*/
            day_init(e, d, ddat + d, &expctl, sellers, buyers, &p_0, &max_surplus, verbose);
            n_buy = expctl.dem_sched[expctl.d_sched].n_agents;
            n_sell = expctl.sup_sched[expctl.s_sched].n_agents;

            surplus = 0.0;
            n_trades = 0;
            sigmasum = 0.0;
            sum_price = 0.0;
            sum_price_diff = 0.0;

            bounds = NULL;

            bounddata[0] = 1;
            bounddata[1] = 12;
            bounddata[2] = 0.0;
            bounddata[3] = 3.75;
            bounds = &(bounddata[0]);
            if (e == 0) /* first experiment?*/
            { /*write a figure of the actual supply and demand curves*/
                sprintf(fname, "%ssd%02d_%03d_000.fig", expctl.id, d + 1, n_trades + 1);
                supdem(n_sell, sellers, n_buy, buyers, max_trades,
                       &dummy_r1, &dummy_i, &dummy_r2,
                       EQ_ACTUAL, fname, bounds, verbose);
            }

            for (t = 0; t < max_trades; t++) { /*one trading session: either   a trade occurs or a fail is recorded*/
                if (verbose) fprintf(stdout, "\nday %d trade %d\n", d, t + 1);

                trade(&(tdat[d][t]), sellers, buyers, &expctl,
                      max_surplus, &surplus, &status, verbose);

                /*this can generate *lots* of data-files*/
                if ((verbose > 0) && (e == 0)) /* first experiment?*/
                { /*print a figure of the actual supply and demand curves*/
                    sprintf(fname,
                            "%ssd%02d_%03d_%03d.fig", expctl.id, d + 1, n_trades + 1, t + 1);
                    fprintf(stdout, "Writing %s\n", fname);
                    supdem(n_sell, sellers, n_buy, buyers, max_trades,
                           &dummy_r1, &dummy_i, &dummy_r2,
                           EQ_ACTUAL, fname, bounds, verbose);
                }

                /*calculate stats*/
                if (status == DEAL) {
                    if (t > 0) last_price = price;
                    price = tdat[d][t].deal_p;
                    if (t > 0) sum_price_diff += ((price - last_price) * (price - last_price));

                    pds = ((price - p_0) * (price - p_0));
                    (ats[n_trades]) += pds;
                    (ats_n[n_trades])++;
                    n_trades++;
                    sum_price += price;
                    sigmasum += pds;
                    alpha = (100 * sqrt(sigmasum / n_trades)) / p_0;
                    efficiency = (surplus / max_surplus) * 100;
                    if (verbose) {
                        fprintf(stdout, "Day %d deal %d alpha=%f efficiency=%f\n",
                                d, n_trades, alpha, efficiency);
                    }
                } else {
                    if (status == END_DAY) /*give up*/
                        t = max_trades;
                }
            } /*end of the trading session*/

            /*update the data for this day*/
            /*profit dispersion*/
            pd = 0.0;
            for (b = 0; b < n_buy; b++) {
                diff = ((buyers[b].a_gain) - (buyers[b].t_gain));
                pd += (diff * diff);
            }
            for (s = 0; s < n_sell; s++) {
                diff = ((sellers[s].a_gain) - (sellers[s].t_gain));
                pd += (diff * diff);
            }
            pdisp = sqrt((1 / ((Real) (n_buy + n_sell))) * pd);
            if (verbose) fprintf(stdout, "Dispersion=%f\n", pdisp);

            ddat_update(ddat + d, n_trades, sum_price, alpha, pdisp, efficiency,
                        sum_price_diff);

        } /*end   of the day loop*/
        if (e == 0) { /*plot the trade stats in xgraph format*/
            sprintf(fname, "%sresults.xg", expctl.id);
            xg_trades_graph(tdat, ddat, expctl.n_days, max_trades, fname, n_exps);

            /*plot this exp's per-trans rms deviation of deal price from equilib*/
            sprintf(fname, "%sres_rms.xg", expctl.id);
            fp = fopen(fname, "w");
            for (t = 0; t < max_trades; t++) {
                if (ats_n[t] > 0) {
                    fprintf(fp, "%d ", t + 1);
                    fprintf(fp, "%f \n", sqrt(ats[t] / ats_n[t]));
                }
            }
            fclose(fp);
        }

        for (t = 0; t < max_trades; t++) {
            if (ats_n[t] > 0) {
                alphatrans = sqrt(ats[t] / ats_n[t]);
                (ats_e[t].sum) += alphatrans;
                (ats_e[t].sumsq) += (alphatrans * alphatrans);

                (ats_e[t].n)++;
            }
        }

        fprintf(stdout, "experiment %d done\n", e);

    } /*end of the experiment loop*/
    /*plot the end-of-day stats in xgraph format*/
    sprintf(fname, "%sres_day.xg", expctl.id);
    xg_daily_graph(ddat, expctl.n_days, n_exps, fname);

    /*plot per-trans rms deviation of deal price from equilib, over exps*/
    sprintf(fname, "%sres_rms_avg.xg", expctl.id);
    fp = fopen(fname, "w");
    fprintf(fp, "TitleText: %s: n=%d\n\n", fname, n_exps);
    /*mean*/
    fprintf(fp, "\"Mean\n");
    for (t = 0; t < max_trades; t++) {
        if (ats_e[t].n > 0) {
            fprintf(fp, "%d ", t + 1);
            fprintf(fp, "%f \n", ats_e[t].sum / ats_e[t].n);
        }
    }
    fprintf(fp, "\n");
    /*+1 standard dev*/
    fprintf(fp, "\"Mean+1sd\n");
    for (t = 0; t < max_trades; t++) {
        if (ats_e[t].n > 0) {
            fprintf(fp, "%d ", t + 1);
            alphatrans = ats_e[t].sum / ats_e[t].n;
            fprintf(fp, "%f \n",
                    alphatrans + sqrt((ats_e[t].sumsq / ats_e[t].n) - (alphatrans * alphatrans)));
        }
    }
    fprintf(fp, "\n");
    /*-1 standard dev*/
    fprintf(fp, "\"Mean-1sd\n");
    for (t = 0; t < max_trades; t++) {
        if (ats_e[t].n > 0) {
            fprintf(fp, "%d ", t + 1);
            alphatrans = ats_e[t].sum / ats_e[t].n;
            fprintf(fp, "%f \n",
                    alphatrans - sqrt((ats_e[t].sumsq / ats_e[t].n) - (alphatrans * alphatrans)));
        }
    }
    fprintf(fp, "\n");
    /*n as a proportion of nexps*/
    fprintf(fp, "\"n/n_exps\n");
    for (t = 0; t < max_trades; t++) {
        if (ats_e[t].n > 0) {
            fprintf(fp, "%d ", t + 1);
            fprintf(fp, "%f \n", ((Real) ats_e[t].n) / n_exps);
        }
    }
    fclose(fp);
    return (1);
}
