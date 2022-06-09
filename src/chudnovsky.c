/*
* Compute pi to a certain number of decimal digits, and print it.
*
*   gcc -O2 -Wall -o chudnovsky chudnovsky.c -lgmp
*
* WARNING: This is a demonstration program only, is not optimized for
* speed, and should not be used for serious work!
*
* The Chudnovsky Algorithm:
*                               _____
*                     426880 * /10005
*  pi = ---------------------------------------------
*         _inf_
*         \     (6*k)! * (13591409 + 545140134 * k)
*          \    -----------------------------------
*          /     (3*k)! * (k!)^3 * (-640320)^(3*k)
*         /____
*          k=0
*
* http://en.wikipedia.org/wiki/Pi#Rapidly_convergent_series
*
* First million digits: http://www.piday.org/million.php
*
* Copyright (c) 2012 Brian "Beej Jorgensen" Hall <beej@beej.us>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <gmp.h>
#include <pthread.h>

// how many to display if the user doesn't specify:
#define DEFAULT_DIGITS 60

// how many decimal digits the algorithm generates per iteration:
#define DIGITS_PER_ITERATION 14.1816474627254776555

typedef struct {
    mpz_t       a;
    mpz_t       b
    mpz_t       c;
    mpz_t       d;
    mpz_t       e;

    mpf_t *     A;
    mpf_t *     B;

    uint32_t    iterations;
}
THREAD_PARMS;

void * numeratorThread(void * p)
{
	uint32_t        k;
    THREAD_PARMS *  tp;

    tp = (THREAD_PARMS *)p;

	for (k = 0; k < tp->iterations; k++) {
		mpz_fac_ui(tp->a, (6 * k));  // (6k)!

		mpz_set_ui(b, 545140134); // 13591409 + 545140134k
		mpz_mul_ui(b, b, k);
		mpz_add_ui(b, b, 13591409);

		mpz_mul(a, a, b);
		mpf_set_z(A, a);
	}

    return NULL;
}

void * denominatorThread(void * p)
{
	uint32_t        k;
    THREAD_PARMS *  tp;
    uint32_t        threek;

    tp = (THREAD_PARMS *)p;

	for (k = 0; k < tp->iterations; k++) {
		threek = (3 * k);

		mpz_fac_ui(c, threek);  // (3k)!

		mpz_fac_ui(d, k);  // (k!)^3
		mpz_pow_ui(d, d, 3);

		mpz_ui_pow_ui(e, 640320, threek); // -640320^(3k)
		
        if ((threek & 1) == 1) { 
            mpz_neg(e, e);
        }

		mpz_mul(c, c, d);
		mpz_mul(c, c, e);
		mpf_set_z(B, c);
	}

    return NULL;
}

/**
 * Compute pi to the specified number of decimal digits using the
 * Chudnovsky Algorithm.
 *
 * http://en.wikipedia.org/wiki/Pi#Rapidly_convergent_series
 *
 * NOTE: this function returns a malloc()'d string!
 *
 * @param digits number of decimal digits to compute
 *
 * @return a malloc'd string result (with no decimal marker)
 */
char * chudnovsky(uint32_t digits)
{
	mpf_t           result, con, A, B, F, sum;
    THREAD_PARMS    threadParms;
	char *          output;
	mp_exp_t        exp;
	double          bits_per_digit;
	uint32_t        k;
    uint32_t        threek;
	uint32_t        precision_bits;
    pthread_t       numeratorPID;
    pthread_t       denominatorPID;

    threadParms.iterations = (uint32_t)(((double)digits / (double)DIGITS_PER_ITERATION) + (double)1.0);

    threadParms.A = (mpf_t *)malloc(sizeof(mpf_t) * 1000);
    threadParms.A = (mpf_t *)malloc(sizeof(mpf_t) * 1000);

	// roughly compute how many bits of precision we need for
	// this many digit:
	bits_per_digit = 3.32192809488736234789; // log2(10)
	precision_bits = (digits * bits_per_digit) + 1;

	mpf_set_default_prec(precision_bits);

	// allocate GMP variables
	mpf_inits(result, con, A, B, F, sum, NULL);
	mpz_inits(threadParms.a, threadParms.b, threadParms.c, threadParms.d, threadParms.e, NULL);

	mpf_set_ui(sum, 0); // sum already zero at this point, so just FYI

	// first the constant sqrt part
	mpf_sqrt_ui(con, 10005);
	mpf_mul_ui(con, con, 426880);

    pthread_create(numeratorPID, NULL, numeratorThread, &threadParms);
    pthread_create(denominatorPID, NULL, denominatorThread, &threadParms);

	// now the fun bit
	for (k = 0; k < iterations; k++) {
		threek = (3 * k);

		mpz_fac_ui(a, (6 * k));  // (6k)!

		mpz_set_ui(b, 545140134); // 13591409 + 545140134k
		mpz_mul_ui(b, b, k);
		mpz_add_ui(b, b, 13591409);

		mpz_fac_ui(c, threek);  // (3k)!

		mpz_fac_ui(d, k);  // (k!)^3
		mpz_pow_ui(d, d, 3);

		mpz_ui_pow_ui(e, 640320, threek); // -640320^(3k)
		
        if ((threek & 1) == 1) { 
            mpz_neg(e, e);
        }

		// numerator (in A)
		mpz_mul(a, a, b);
		mpf_set_z(A, a);

		// denominator (in B)
		mpz_mul(c, c, d);
		mpz_mul(c, c, e);
		mpf_set_z(B, c);

		// result
		mpf_div(F, A, B);

		// add on to sum
		mpf_add(sum, sum, F);
	}

	// final calculations (solve for pi)
	mpf_ui_div(sum, 1, sum); // invert result
	mpf_mul(sum, sum, con); // multiply by constant sqrt part

	// get result base-10 in a string:
	output = mpf_get_str(NULL, &exp, 10, digits, sum); // calls malloc()

	// free GMP variables
	mpf_clears(result, con, A, B, F, sum, NULL);
	mpz_clears(a, b, c, d, e, NULL);

	return output;
}

/**
 * Print a usage message and exit
 */
void usage_exit(void)
{
	fprintf(stderr, "usage: chudnovsky [digits]\n");
	exit(1);
}

/**
 * MAIN
 *
 * See usage_exit() for usage.
 */
int main(int argc, char **argv)
{
	char *      pi;
    char *      endptr;
	uint32_t    digits = 0;

	switch (argc) {
		case 1:
			digits = DEFAULT_DIGITS;
			break;

		case 2:
			digits = (uint32_t)strtoul(argv[1], &endptr, 10);
			
            if (*endptr != '\0') { 
                usage_exit();
            }
			break;

		default:
			usage_exit();
	}

	if (digits < 1) { 
        usage_exit();
    }

	pi = chudnovsky(digits);

	// since there's no decimal point in the result, we'll print the
	// first digit, then the rest of it, with the expectation that the
	// decimal will appear after "3", as per usual:
	printf("%.1s.%s\n", pi, pi+1);

	// chudnovsky() malloc()s the result string, so let's be proper:
	free(pi);

	return 0;
}
