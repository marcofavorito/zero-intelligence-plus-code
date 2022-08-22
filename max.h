// max.h: maxima for array bounds etc


#define MAX_N_DAYS 30
#define MAX_TRADES 100
#define TOT_TRADES (MAX_N_DAYS*MAX_TRADES)
#define MAX_FAILS 100 /*maximum numbers of bids/offers al lowed to fail before day's trading closes*/
#define MAX_BUYERS 100
#define MAX_SELLERS 100
#define MAX_AGENTS (MAX_BUYERS>MAX_SELLERS?MAX_BUYERS:MAX_SELLERS)
#define MAX_UNITS 3 /*max no. of units an agent can sell/buy*/
#define MAX_SCHED 2 /*max no. of supply or demand schedules in an experiment*/
#define MAX_ID 30 /*max no. of chars in id tag used for output*/