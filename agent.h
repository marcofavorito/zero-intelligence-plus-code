//
// agent.h: general global constants and structures
//
// Dave Cli, August 1996
//
#define NULL_EQ -1 /*signals no equilibrium*/

// symbolic constants for agent type, shout type, and whether shout is accepted or rejected
#define   BUY 1
#define   SELL 0
#define   BID 1
#define   OFFER 0
#define   DEAL 1
#define   NO_DEAL 0
#define   END_DAY 2

typedef struct an_agent {
    int job;     /*BUYing or SELLing*/
    int active;  /*still in the market?*/
    int n;       /*number of deals done*/
    int willing; /*want to make a trade at this price?*/
    int able;    /*allowed to trade at this price?*/
    Real limit;   /*the bottom-line price for this agent*/
    Real profit;  /*profit coefficient in determinining bid/offer price*/
    Real beta;    /*coefficient for changing profit over time (learning rate)*/
    Real momntm;  /*momentum in changing profit*/
    Real last_d;  /*last change*/
    Real price;   /*what the agent will actually bid*/
    Real quant;   /*how much of this commodity*/
    Real bank;    /*how much money this agent has in the bank*/
    Real a_gain;  /*actual gain*/
    Real t_gain;  /*theoretical gain*/
    Real sum;     /*in determining average reward*/
    Real avg;     /*average reward*/
} Agent;

void set_price(Agent *);

void shout_update(int deal_type, int status,
                  int n_sell, Agent sellers[], int n_buy, Agent buyers[], Real price,
                  int verbose);

void buy_init(Agent b[], int verbose);

void sell_init(Agent s[], int verbose);

int willing_trade(Agent *a, Real price);

void profit_alter(Agent *a, Real price, int verbose);

