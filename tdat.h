//
// tdat.h: header for routines that record and display data in one trading session
//
// Dave Cli, Aug 1996
//

typedef struct trade_data {
    Real deal_p; /*price at which deal succeeds*/
    int deal_t;  /*type of deal accepted (bid or ask)*/
    Real t_eq_p; /*theoretical equilibrium price*/
    int t_eq_q;  /*theoretical equilibrium quantity*/
    Real a_eq_p; /*actual equilibrium price*/
    int a_eq_q;  /*actual equilibrium quantity*/
} Trade_data;

void xg_trades_graph(Trade_data tdat[][MAX_TRADES],
                     Day_data *, int, int, char [], int);
