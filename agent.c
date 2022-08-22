//
// agent.c: de nes an agent, how it adapts, etc.
//
// Dave Cli, Aug 1996
//

#include <math.h>
#include <stdio.h>
#include "random.h"
#include "max.h"
#include "agent.h"

#define BONUS 0.00

#define MARKUP 1.1
#define MARKDOWN 0.9
#define MARK 0.05

// set-price: set the price of an agent from its limit and profit values
void set_price(Agent *a) {
    a->price = (a->limit) * (1 + a->profit);
    /*normalise to one-cent precision*/
    a->price = (floor((a->price * 100) + 0.5)) / 100;
}

// agent-init: initialise the common elements of an agent (buyer or seller)
void agent_init(Agent *a, int verbose) {
    a->beta = 0.1 + randval(0.4);
    a->bank = 0.0;
    a->n = 0;
    a->sum = 0.0;
    a->last_d = 0.0;
    a->momntm = 0.2 + randval(0.6);
    a->momntm = randval(0.1);
    a->active = 1;
    if (verbose) {
        fprintf(stdout, "prof=%+5.3f beta=%5.3f mom=%5.3f bank=%5.2f\n",
                a->profit, a->beta, a->momntm, a->bank);
    }
}

// buy-init: initialize the buyers
void buy_init(Agent b[MAX_AGENTS], int verbose) {
    int a;

    for (a = 0; a < MAX_AGENTS; a++) {
        b[a].job = BUY;
        b[a].profit = -1.0 * (0.05 + randval(0.3));
        if (verbose) fprintf(stdout, "B%2d ", a);
        agent_init(b + a, verbose);
    }
}

// sell-init: initialize the sellers
void sell_init(Agent s[MAX_AGENTS], int verbose) {
    int a;

    for (a = 0; a < MAX_AGENTS; a++) {
        s[a].job = SELL;
        s[a].profit = 0.05 + randval(0.3);
        if (verbose) fprintf(stdout, "S%2d ", a);
        agent_init(s + a, verbose);
    }
}

// willing-trade: is an agent willing to trade at given price?
int willing_trade(Agent *a, Real price) {
    if (a->job == BUY) { /*willing to buy at this price?*/
        if ((a->active) && (a->price >= price)) { a->willing = 1; }
        else { a->willing = 0; }
    } else { /*willing to sell at this price?*/
        if ((a->active) && (a->price <= price)) { a->willing = 1; }
        else { a->willing = 0; }
    }
    return (a->willing);
}

// profit-alter: update profit margin on basis of sale price using Widrow-Hoff style update with learning rate .
void profit_alter(Agent *a, Real price, int verbose) {
    Real c, diff, change, newprofit;

    if (verbose)
        fprintf(stdout, "lim=%5.3f prof=%5.3f price=%5.2f",
                a->limit, a->profit, a->price);

    diff = (price - (a->price));
    change = ((1.0 - (a->momntm)) * (a->beta) * diff) + ((a->momntm) * (a->last_d));

    if (verbose)
        fprintf(stdout, " last_d=%5.3f diff=%5.2f chng=%+5.3f",
                a->last_d, diff, change);

    a->last_d = change;

    /*set new prices by altering profit margin*/
    newprofit = ((a->price + change) / a->limit) - 1.0;

    if (a->job == SELL) {
        if (newprofit > 0.0) a->profit = newprofit;
    } else {
        if (newprofit < 0.0) a->profit = newprofit;
    }

    set_price(a);
    if (verbose) { fprintf(stdout, " nu_prof=%5.3f nu_price=%5.2f", a->profit, a->price); }
}

// shout-update: update strategies of buyers and sellers after a shout
void shout_update(int deal_type, int status, int n_sell,
                  Agent sellers[], int n_buy, Agent buyers[], Real price,
                  int verbose) {
    int b, s;
    Real target_price;
    /*any seller whose price is less than or equal to the deal price raises profit margin*/
    /*(this is an attempt to increase profits next time around)*/

    for (s = 0; s < n_sell; s++) {
        if (verbose) fprintf(stdout, "S%02d(%d) ", s, sellers[s].active);

        if (status == DEAL) {
            if (sellers[s].price <= price) { /*could get more? { try raising margin*/
                target_price = (price * (1.0 + randval(MARK))) + randval(0.05);
                profit_alter(sellers + s, target_price, verbose);
            } else { /*wouldn't have got this deal, so mark the price down*/
                if ((deal_type == BID) &&
                    (!willing_trade(sellers + s, price)) &&
                    (sellers[s].active)
                        ) {
                    target_price = (price * (1.0 - randval(MARK))) - randval(0.05);
                    profit_alter(sellers + s, target_price, verbose);
                }
            }
        } else /*NO DEAL*/
        {
            if (deal_type == OFFER)
                if ((sellers[s].price >= price) &&
                    (sellers[s].active)) { /*would have asked for more and lost the deal, so reduce profit*/
                    target_price = (price * (1.0 - randval(MARK))) - randval(0.05);
                    profit_alter(sellers + s, target_price, verbose);
                }
        }
        if (verbose)fprintf(stdout, "\n");
    }

    for (b = 0; b < n_buy; b++) {
        if (verbose) fprintf(stdout, "B%02d(%d) ", b, buyers[b].active);

        if (status == DEAL) {
            if (buyers[b].price >= price) { /*could get lower price? { try raising margin (i.e. cutting price)*/
                target_price = (price * (1.0 - randval(MARK))) - randval(0.05);
                profit_alter(buyers + b, target_price, verbose);
            } else { /*wouldn't have got this deal, so mark the price up (reduce profit)*/
                if ((deal_type == OFFER) &&
                    (!willing_trade(buyers + b, price)) &&
                    (buyers[b].active)
                        ) {
                    target_price = (price * (1.0 + randval(MARK))) + randval(0.05);
                    profit_alter(buyers + b, target_price, verbose);
                }
            }
        } else /*NO-DEAL*/
        {
            if (deal_type == BID)
                if ((buyers[b].price <= price) &&
                    (buyers[b].active)) { /*would have bid less and also lost the deal, so reduce profit*/
                    target_price = (price * (1.0 + randval(MARK))) + randval(0.05);
                    profit_alter(buyers + b, target_price, verbose);
                }
        }
        if (verbose)fprintf(stdout, "\n");
    }
}

