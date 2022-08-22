//
// random.h
//
// Dave Cli, May 1991
//
// Some general random routines
// Copied or adapted from Numerical recipies in C by Press, Flannery, Teukolsky, and Vetterling, (CUP, 1988).
//

#define Real double

void rseed(int *); /*reseed random number generator*/
Real randval(Real); /*return a (near)uniform distributed random number 2 [0; limit]*/
int irand(int); /*return a random integer 2 f0; : : : ; limit ? 1g */
Real gaussrand(void); /*returns a N (0; 1) random deviate*/

// NB: abs(gaussrand()) will be > 3 about once in 400 trials (the 3 ?  rule).

Real exprand(Real); /*exponential distribution with specifed mean*/
