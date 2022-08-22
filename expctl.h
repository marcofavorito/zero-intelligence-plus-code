// expctl.h: header file for experiment control data struct and i/o etc
// Dave Cli, Aug 1996

// Agent-sched: data associated with one agent's buy/sell limits etc
typedef struct an_agent_sched {
    int n_units;           /*how many units the agent         has/wants*/
    Real limit[MAX_UNITS]; /*limit price of each unit*/
} Agent_sched;

// SD-sched: data associated with a supply or demand schedule
typedef struct sd_sched {
    int n_agents;                        /*how many agents involved*/
    int first_day;                       /*first day this schedule applies to*/
    int last_day;                        /*last day this schedule applies to*/
    int can_shout;                       /*boolean: 0=>silent traders; 1=>can    shout*/
    Agent_sched agents[MAX_AGENTS];      /*details of individual agents*/
} SD_sched;

// Expctl: experiment control parameters
typedef struct a_expctl {
    char id[MAX_ID];                    /*id characters for output files*/
    int n_days;                         /*number of trading periods to run for*/
    int min_trades;                     /*minimum number of trades per day*/
    int max_trades;                     /*maximum number of trades per day*/
    int random;                         /*boolean: 0=> ZIP; 1=>ZI-C*/
    int nyse;                           /*boolean: 0=>NYSE o ; 1=>NYSE on*/
    int n_dem_sched;                    /*number of demand schedules*/
    SD_sched dem_sched[MAX_SCHED];      /*details of demand schedules*/
    int d_sched;                        /*index of currently active demand schedule*/
    int n_sup_sched;                    /*number of supply schedules*/
    SD_sched sup_sched[MAX_SCHED];      /*details of supply schedules*/
    int s_sched;                        /*index of currently active supply schedule*/
} Expctl;

void expctl_in(char [], Expctl *, int);
