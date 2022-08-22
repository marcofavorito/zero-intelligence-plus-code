//
// expctl.c: read experiment control parameters from a file
//
// Dave Cli, Aug 1996
//
// This does some validity checks but still need to be careful that the data-file it reads from is structured correctly.
// When there is more than one schedule for supply or demand, they must be listed in the data-file in the order they 
// are to become active. The first-day for the 0th schedule is set to zero, whatever value is given in the data-file

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "random.h"
#include "max.h"
#include "expctl.h"

#define LLEN 1024 /*max. no. of characters in a line*/

// get-non-comment-line: read to start of next line that doesn't start with `#'
int get_non_comment_line(FILE *fp) {
    int c, reading = 1;
    char s[LLEN];

    while (reading) { /*get to first non-whitespace char*/
        c = ' ';
        while (isspace(c)) {
            c = fgetc(fp);
            if (c == EOF) return (EOF);
        }

        /*is this a comment line?*/
        if (c == '#') { /*yes: read the rest of this line*/
            ungetc(c, fp);
            fgets(s, LLEN, fp);
        } else { /*no: put the char back and exit*/
            ungetc(c, fp);
            return (EOF - 1); /*i.e. something that     isn't EOF*/
        }
    }
}

// read-sched: read a supply or demand schedule
int read_sched(FILE *fp, SD_sched *sched, int verbose) {
    int i, *pi, a, u;
    float f, *pf;

    pi = &i;
    pf = &f;

    /*read number of agents*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            sched->n_agents = (*pi);
            if ((sched->n_agents < 1) || (sched->n_agents > MAX_AGENTS)) {
                fprintf(stderr, "\nFail: # agents must be in range {1,...,%d}\n",
                        MAX_AGENTS);
                exit(0);
            }
            if (verbose) {
                fprintf(stdout, " %d agents: ", sched->n_agents);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read # agents \n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading # agents\n");
        exit(0);
    }

    /*read start day*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            sched->first_day = (*pi);
            if (verbose) {
                fprintf(stdout, "from day %d ", sched->first_day);
                fflush(stdout);
            }
        }
    }

    /*read end day*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            sched->last_day = (*pi);
            if (sched->last_day < sched->first_day) {
                fprintf(stderr, "\nFail: last_day(%d)<first_day(%d)\n",
                        sched->last_day, sched->first_day);
                exit(0);
            }
            if (verbose) {
                fprintf(stdout, "to day %d\n", sched->last_day);
                fflush(stdout);
            }
        }
    }

    /*read shout ag*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            sched->can_shout = (*pi);
            if ((sched->can_shout < 0) || (sched->can_shout > 1)) {
                fprintf(stderr, "\nFail: can_shout not Boolean (%d)\n",
                        sched->can_shout);
                exit(0);
            }
            if (verbose) {
                if (sched->can_shout) {
                    fprintf(stdout, "(These traders CAN SHOUT)\n");
                    fflush(stdout);
                }
                else {
                    fprintf(stdout, "(These traders are SILENT)\n");
                    fflush(stdout);
                }
            }
        }
    }

    /*read agent pricing specs*/
    for (a = 0; a < sched->n_agents; a++) {
        if (get_non_comment_line(fp) != EOF) {
            if (fscanf(fp, "%d", pi) != EOF) {
                sched->agents[a].n_units = (*pi);
                if ((sched->agents[a].n_units < 1) || (sched->agents[a].n_units > MAX_UNITS)) {
                    fprintf(stderr, "\nFail: # units must be inrange {1,...,%d}\n",
                            MAX_UNITS);
                    exit(0);
                }

                if (verbose) {
                    fprintf(stdout, "     Agent %2d, %d units: ", a, sched->agents[a].n_units);
                    fflush(stdout);
                }

                for (u = 0; u < sched->agents[a].n_units; u++) {
                    if (fscanf(fp, "%f", pf) != EOF) {
                        sched->agents[a].limit[u] = (Real) (*pf);
                        if (sched->agents[a].limit[u] < 0.0) {
                            fprintf(stderr, "\nFail: negative price (%f)\n", *pf);
                            exit(0);
                        }
                        if (verbose) {
                            fprintf(stdout, "%f ", sched->agents[a].limit[u]);
                            fflush(stdout);
                        }
                    }
                }

                if (verbose) {
                    fprintf(stdout, "\n");
                    fflush(stdout);
                }
            }
        }
    } /*end of reading the agent data*/
}

// expctl-in: read expctl data from a specified file
void expctl_in(char filename[], Expctl *ec, int verbose) {
    int *pi, i, sched;
    float f, *pf;
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "\nFAIL: can't open \"%s\" as expctl input file\n", filename);
        exit(0);
    }

    pi = &i;
    pf = &f;

    /*read id string*/
    if (get_non_comment_line(fp) != EOF) { /*copy id string up to but not including the newline*/
        fscanf(fp, "%s\n", &(ec->id));
        if (verbose) {
            fprintf(stdout, "ID: %s\n", ec->id);
            fflush(stdout);
        }
    }


    /*read number of days*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->n_days = (*pi);
            if ((ec->n_days < 1) || (ec->n_days > MAX_N_DAYS)) {
                fprintf(stderr, "\nFail: # trading days must be in range {1,...,%d}\n",
                        MAX_N_DAYS);
                exit(0);
            }
            if (verbose) {
                fprintf(stdout, "%d days: ", ec->n_days);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read number of days\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading number of days\n");
        exit(0);
    }

    /*read min number of trades per day*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->min_trades = (*pi);
            if ((ec->min_trades < 1) || (ec->min_trades > MAX_TRADES)) {
                fprintf(stderr, "\nFail: min # trades must be in range {1,...,%d}\n",
                        MAX_TRADES);
                exit(0);
            }
            if (verbose) {
                fprintf(stdout, "min_trades=%d ", ec->min_trades);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read min_trades\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading min_trades\n");
        exit(0);
    }

    /*read max number of trades per day*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->max_trades = (*pi);
            if ((ec->max_trades < ec->min_trades) || (ec->max_trades > MAX_TRADES)) {
                fprintf(stderr, "\nFail: max # trades muts be in range {%d,...,%d}\n",
                        ec->min_trades, MAX_TRADES);
                exit(0);
            }
            if (verbose) {
                fprintf(stdout, "max_trades=%d\n", ec->max_trades);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read max_trades\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading max_trades\n");
        exit(0);
    }

    /*read random ag*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->random = (*pi);
            switch (ec->random) {
                case 1:
                    if (verbose) fprintf(stdout, "Random (ZI-C) traders; ");
                    break;

                case 0:
                    if (verbose) fprintf(stdout, "Intelligent traders; ");
                    break;


                default:
                    fprintf(stderr, "\nFail: random flag must be boolean\n");
                    exit(0);
            }
            if (verbose) fflush(stdout);
        } else {
            fprintf(stderr, "\nFail: can't read random flag\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading random flag\n");
        exit(0);
    }

    /*read nyse ag*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->nyse = (*pi);
            switch (ec->nyse) {
                case 1:
                    if (verbose) fprintf(stdout, "NYSE trading rules\n");
                    break;

                case 0:
                    if (verbose) fprintf(stdout, "no NYSE rules\n");
                    break;

                default:
                    fprintf(stderr, "\nFail: NYSE flag must be boolean\n");
                    exit(0);
            }
            if (verbose) fflush(stdout);
        } else {
            fprintf(stderr, "\nFail: can't read nyse flag\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading nyse flag\n");
        exit(0);
    }

    /*read number of demand schedules*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->n_dem_sched = (*pi);
            if ((ec->n_dem_sched < 1) || (ec->n_dem_sched > MAX_SCHED)) {
                fprintf(stderr, "\nFail: # demand scheds must be in range {1,...,%d}\n",
                        MAX_SCHED);
                exit(0);
            }

            if (verbose) {
                fprintf(stdout, "%d demand schedules:\n", ec->n_dem_sched);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read # demand schedules\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading # demand schedules\n");
        exit(0);
    }

    /*read the schedules*/
    for (sched = 0; sched < ec->n_dem_sched; sched++) {
        if (verbose) fprintf(stdout, " Demand schedule %d:\n", sched);
        if (read_sched(fp, &(ec->dem_sched[sched]), verbose) == EOF) {
            fprintf(stderr, "\nFail: no more demand schedules\n");
            exit(0);
        }
    }

    ec->d_sched = 0;
    ec->dem_sched[ec->d_sched].first_day = 0;


    /*read number of supply schedules*/
    if (get_non_comment_line(fp) != EOF) {
        if (fscanf(fp, "%d", pi) != EOF) {
            ec->n_sup_sched = (*pi);
            if ((ec->n_sup_sched < 1) || (ec->n_sup_sched > MAX_SCHED)) {
                fprintf(stderr, "\nFail: # supply scheds must be in range {1,...,%d}\n",
                        MAX_SCHED);
                exit(0);
            }

            if (verbose) {
                fprintf(stdout, "%d supply schedules:\n", ec->n_sup_sched);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nFail: can't read # supply schedules\n");
            exit(0);
        }
    } else {
        fprintf(stderr, "\nFail: EOF reading # supply schedules\n");
        exit(0);
    }

    /*read the schedules*/
    for (sched = 0; sched < ec->n_sup_sched; sched++) {
        if (verbose) fprintf(stdout, " Supply schedule %d:\n", sched);
        if (read_sched(fp, &(ec->sup_sched[sched]), verbose) == EOF) {
            fprintf(stderr, "\nFail: no more supply schedules\n");
            exit(0);
        }
    }

    ec->s_sched = 0;
    ec->sup_sched[ec->s_sched].first_day = 0;

    fclose(fp);
}


