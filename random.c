//
// random.c
//
// Dave Cli , May 1991
//

#include   <math.h>
#include   <time.h>
#include   <stdio.h>
#include   "random.h"

// ran1 from the Numerical Recipes in C Book { it's the slowest but (?) best
// ******************** ran1 ******************

#define   M1 259200
#define   IA1 7141
#define   IC1 54773
#define   RM1 (1.0/M1)
#define   M2 134456
#define   IA2 8121
#define   IC2 28411
#define   RM2 (1.0/M2)
#define   M3 243000
#define   IA3 4561
#define   IC3 51349

float ran1(int *idum) {
    static long ix1, ix2, ix3;
    static float r[98];
    float temp;
    static int iff = 0;
    int j;
    void nrerror();

    if (*idum < 0 || iff == 0) {
        iff = 1;
        ix1 = (IC1 - (*idum)) % M1;
        ix1 = (IA1 * ix1 + IC1) % M1;
        ix2 = ix1 % M2;
        ix1 = (IA1 * ix1 + IC1) % M1;
        ix3 = ix1 % M3;
        for (j = 1; j <= 97; j++) {
            ix1 = (IA1 * ix1 + IC1) % M1;
            ix2 = (IA2 * ix2 + IC2) % M2;
            r[j] = (ix1 + ix2 * RM2) * RM1;
        }
        *idum = 1;
    }
    ix1 = (IA1 * ix1 + IC1) % M1;
    ix2 = (IA2 * ix2 + IC2) % M2;
    ix3 = (IA3 * ix3 + IC3) % M3;
    j = 1 + ((97 * ix3) / M3);
    if (j > 97 || j < 1)
        /* nrerror("RAN1: This cannot happen."); */
        fprintf(stderr, "RAN1: This cannot happen.");
    temp = r[j];
    r[j] = (ix1 + ix2 * RM2) * RM1;
    return temp;
}

#undef   M1
#undef   IA1
#undef   IC1
#undef   RM1
#undef   M2
#undef   IA2
#undef   IC2
#undef   RM2
#undef   M3
#undef   IA3
#undef   IC3


// **********************************************
// NB ran1 is not exported { it's masked by the following routines

// rseed: reseed the random number generator from the system clock if (*s)=0 then the system clock is used, otherwise the (*s) is used
void rseed(int *s) {
    time_t tseed;
    int seed;

    if ((*s) == 0) {
        time(&tseed);
        seed = (int) (tseed % 32767);
        *s = seed;
    } else seed = *s;
    fprintf(stdout, "\n: Seed is %d\n", seed);
    /* srandom(seed); */
    seed = seed * -1;
    ran1(&seed);
}

// randval: return a (near)uniform distributed random number in the range 0..limit, as a Real
Real randval(Real limit) {
    float rv;
    int i = 1;
    /*get a random value in the range   0..1*/
    rv = ran1(&i);
    return (limit * ((Real) rv));
}

// irand: return a random integer in [0..limit-1]
int irand(int limit) {
    int ir;
    /*while loop is used to trap the exceptional case where   the underlying deviate in [0,1] actually returns 1.00*/
    ir = limit;
    while (ir == limit) { ir = (int) (floor(randval((Real) limit))); }
    return (ir);
}

// gaussrand: return a N (0; 1) deviate
Real gaussrand(void) {
    static int iset = 0;
    static Real gset;
    Real fac, r, v1, v2;

    if (iset == 0) {
        do {
            v1 = 2.0 * randval(1.0) - 1.0;
            v2 = 2.0 * randval(1.0) - 1.0;
            r = (v1 * v1) + (v2 * v2);
        } while (r >= 1.0);
        fac = sqrt(-2.0 * log(r) / r);
        gset = v1 * fac;
        iset = 1;
        return (v2 * fac);
    } else {
        iset = 0;
        return (gset);
    }
}


// exprand: exponentially distributed variable: also from numerical recipes.
Real exprand(Real mean) {
    Real r;

    r = 0.0;
    while (r == 0.0) { r = randval(1.0); }
    return ((-log(r)) * mean);
}

