//
// tdat.c: routine for manipulating data and stats recorded during a single trade, i.e., the auction leading to one deal
//
// Dave Cli, Aug 1996
//

#include <stdio.h>
#include <math.h>

#include "random.h"
#include "agent.h"
#include "max.h"
#include "ddat.h"
#include "tdat.h"

// xg-trades-graph: plot stats concerning individual trades in xgraph format
void xg_trades_graph(Trade_data tdat[MAX_N_DAYS][MAX_TRADES],
                     Day_data *ddat,
                     int n_days,int max_trades,
                     char filename[],int n_exps)
{ Real gx,dgx,q;
    int t,d;
    FILE *fp;

    fp=fopen(filename,"w");

    fprintf(fp,"TitleText: %s: n=%d\n\n",filename,n_exps);

    dgx=(1.0/((Real)(max_trades)));

    fprintf(fp,"\"Price\n");
    for(d=0;d<n_days;d++)
    { gx=d+1;
        q=(ddat+d)->quant.sum;
        for(t=0;t<q;t++)
        { if(tdat[d][t].deal_p>=0.0) fprintf(fp,"%f %f\n",gx,tdat[d][t].deal_p);
            gx+=dgx;
        }
    }

    fprintf(fp,"\n\"Actual EqP\n");
    for(d=0;d<n_days;d++)
    { gx=d+1;
        q=(ddat+d)->quant.sum;
        for(t=0;t<q;t++)
        { if(tdat[d][t].a_eq_q!=NULL_EQ) fprintf(fp,"%f %f\n",gx,tdat[d][t].a_eq_p);
            gx+=dgx;
        }
    }

    fprintf(fp, "\n\"Theoretical EqP\n");
    for(d=0;d<n_days;d++)
    { gx=d+1;
        q=(ddat+d)->quant.sum;
        for(t=0;t<q;t++)
        { if(tdat[d][t].t_eq_q!=NULL_EQ) fprintf(fp,"%f %f\n",gx,tdat[d][t].t_eq_p);
            gx+=dgx;
        }
    }

    fclose(fp);
}
