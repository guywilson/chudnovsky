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
#include <errno.h>
#include <gmp.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_CORES                       4UL
#define THREAD_ITERATION_THESHOLD       1000
#define THREAD_SLEEP_CYCLES             10

// how many to display if the user doesn't specify:
#define DEFAULT_DIGITS                  60

// how many decimal digits the algorithm generates per iteration:
#define DIGITS_PER_ITERATION            14.1816474627254776555f

typedef struct {
    uint64_t        startk;
    uint64_t        endk;

    mpf_t           result;
}
thread_parms_t;

// static void test(int numThreads, uint64_t digits) {
//     int                 i;
//     uint64_t            iterations;
//     uint64_t            startCounter = 0UL;
//     thread_parms_t *    thread_parms;

//     iterations = (uint64_t)((double)digits / (double)DIGITS_PER_ITERATION) + 1UL;

//     /*
//     ** Add 10%...
//     */
//     iterations += (iterations / 10);

//     printf("Num iterations = %llu\n", iterations);

//     thread_parms = (thread_parms_t *)malloc(sizeof(thread_parms_t) * numThreads);

//     if (thread_parms == NULL) {
//         fprintf(stderr, "Failed to allocate memory for thread paramaters\n");
//         exit(-1);
//     }

//     /*
//     ** Iterations are split by thread, the first thread gets
//     ** 50% of the load (where k is relatively small), 66% when 
//     ** we have just 2 threads, then each subsequent thread gets 
//     ** 50% of the remainder, with the final thread getting the 
//     ** remainder...
//     */
//     for (i = 0;i < numThreads;i++) {
//         if (numThreads == 1) {
//             thread_parms[i].startk = 0;
//             thread_parms[i].endk = iterations - 1UL;
//         }
//         else if (numThreads == 2) {
//             thread_parms[0].startk = 0UL;
//             thread_parms[0].endk = (uint64_t)((float)iterations * 0.66f);
//             thread_parms[1].startk = thread_parms[0].endk + 1UL;
//             thread_parms[1].endk = iterations - 1UL;
//         }
//         else {
//             thread_parms[i].startk = startCounter;

//             if (i == (numThreads - 1)) {
//                 /*
//                 ** Last thread...
//                 */
//                 thread_parms[i].endk = iterations - 1UL;
//             }
//             else {
//                 thread_parms[i].endk = 
//                     startCounter + 
//                     (uint64_t)(((float)iterations - (float)startCounter) * 0.5f) - 1UL;
//             }

//             startCounter = thread_parms[i].endk + 1UL;
//         }

//         printf("Starting thread, startk = %llu, endk = %llu\n", thread_parms[i].startk, thread_parms[i].endk);
//     }

//     free(thread_parms);
// }

void * chudnovsky_thread(void * p) {
    int                 i;
    mpf_t               A;
    mpf_t               B;
    mpf_t               F;
	mpz_t               a;
    mpz_t               b;
    mpz_t               c;
    mpz_t               d;
    mpz_t               e;
    uint64_t            k;
    uint64_t            threek;
    uint64_t            sixk;
    thread_parms_t *    parms;
    
    parms = (thread_parms_t *)p;

	// allocate GMP variables
	mpf_inits(A, B, F, NULL);
	mpz_inits(a, b, c, d, e, NULL);

    i = 0;

	for (k = parms->startk; k < parms->endk; k++) {
        if (i == THREAD_SLEEP_CYCLES) {
            i = 0;

            usleep(100U);
        }

		threek = 3 * k;
        sixk = threek << 1;

		mpz_fac_ui(a, sixk);  // (6k)!

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
		mpf_add(parms->result, parms->result, F);

        i++;
	}

	// free GMP variables
	mpf_clears(A, B, F, NULL);
	mpz_clears(a, b, c, d, e, NULL);

    pthread_exit(NULL);
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
 * @param numCores number of cpu cores to run on
 *
 * @return a malloc'd string result (with no decimal marker)
 */
static char * chudnovsky(uint64_t digits, int numCores) {
    int                 i;
    int                 numThreads = numCores;
    pthread_t *         tids;
    mpf_t               con;
    mpf_t               sum;
	mp_exp_t            exp;
	uint64_t            iterations;
    uint64_t            startCounter = 0;
	uint64_t            precision_bits;
	char *              output;
	double              bits_per_digit;
    thread_parms_t *    thread_parms;

    iterations = (uint64_t)((double)digits / (double)DIGITS_PER_ITERATION) + 1UL;

    /*
    ** Add 10%...
    */
    iterations += (iterations / 10);

    printf("Num iterations = %llu\n", iterations);

    /*
    ** If we've got an odd number of threads, scale up to an even number...
    */
    if ((numThreads % 2) == 1) {
        numThreads++;
    }

    if (iterations < THREAD_ITERATION_THESHOLD) {
        numThreads = 1;
    }

    thread_parms = (thread_parms_t *)malloc(sizeof(thread_parms_t) * numThreads);

    if (thread_parms == NULL) {
        fprintf(stderr, "Failed to allocate memory for thread paramaters\n");
        exit(-1);
    }

    tids = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);

    if (tids == NULL) {
        fprintf(stderr, "Failed to allocate memory for thread IDs\n");
        exit(-1);
    }

	// roughly compute how many bits of precision we need for
	// this many digit:
	bits_per_digit = 3.32192809488736234789; // log2(10)
	precision_bits = (digits * bits_per_digit) + 1UL;

	mpf_set_default_prec(precision_bits);

	// allocate GMP variables
	mpf_inits(con, sum, NULL);

	mpf_set_ui(sum, 0); // sum already zero at this point, so just FYI

	// first the constant sqrt part
	mpf_sqrt_ui(con, 10005);
	mpf_mul_ui(con, con, 426880);

    /*
    ** Iterations are split by thread, the first thread gets
    ** 50% of the load (where k is relatively small), 66% when 
    ** we have just 2 threads, then each subsequent thread gets 
    ** 50% of the remainder, with the final thread getting the 
    ** remainder...
    */
    for (i = 0;i < numThreads;i++) {
        if (numThreads == 1) {
            thread_parms[i].startk = 0;
            thread_parms[i].endk = iterations - 1UL;
        }
        else if (numThreads == 2) {
            thread_parms[0].startk = 0UL;
            thread_parms[0].endk = (uint64_t)((float)iterations * 0.66f);
            thread_parms[1].startk = thread_parms[0].endk + 1UL;
            thread_parms[1].endk = iterations - 1UL;
        }
        else {
            thread_parms[i].startk = startCounter;

            if (i == (numThreads - 1)) {
                /*
                ** Last thread...
                */
                thread_parms[i].endk = iterations - 1UL;
            }
            else {
                thread_parms[i].endk = 
                    startCounter + 
                    (uint64_t)(((float)iterations - (float)startCounter) * 0.5f) - 1UL;
            }

            startCounter = thread_parms[i].endk + 1UL;
        }

        printf("Starting thread, startk = %llu, endk = %llu\n", thread_parms[i].startk, thread_parms[i].endk);

        mpf_init(thread_parms[i].result);
        mpf_set_ui(thread_parms[i].result, 0);

        pthread_create(&tids[i], NULL, chudnovsky_thread, (void *)&thread_parms[i]);
    }

    for (i = 0;i < numThreads;i++) {
        pthread_join(tids[i], NULL);

        // add on to sum
        mpf_add(sum, sum, thread_parms[i].result);
        mpf_clear(thread_parms[i].result);
    }

    free(thread_parms);
    free(tids);

	// final calculations (solve for pi)
	mpf_ui_div(sum, 1, sum); // invert result
	mpf_mul(sum, sum, con); // multiply by constant sqrt part

	// get result base-10 in a string:
	output = mpf_get_str(NULL, &exp, 10, digits, sum); // calls malloc()

	// free GMP variables
	mpf_clears(con, sum, NULL);

	return output;
}

/**
 * Print a usage message and exit
 */
static void printUsage(void) {
	printf("\n Usage: chudnovsky [OPTIONS]\n\n");
	printf("  Options:\n");
	printf("   -h/?                 Print this help\n");
	printf("   -digits num_digits   Number of pi digits to compute\n");
	printf("   -cores num_cores     How many cores to run on\n");
	printf("   -f output_file       The output file\n");
	printf("\n");
}

/**
 * MAIN
 *
 * See usage_exit() for usage.
 */
int main(int argc, char **argv) {
    int             i;
    int             numCores = NUM_CORES;
	char *          pi;
    char *          endptr;
    char *          pszOutputFile;
	long            digits = DEFAULT_DIGITS;
    FILE *          fptrOut;

	if (argc > 1) {
		for (i = 1;i < argc;i++) {
			if (argv[i][0] == '-') {
				if (strncmp(&argv[i][1], "digits", 6) == 0) {
                    digits = strtol(&argv[++i][0], &endptr, 10);

                    if (*endptr != '\0') { 
                        printUsage();
                        return -1;
                    }
				}
				else if (strncmp(&argv[i][1], "cores", 5) == 0) {
					numCores = atoi(&argv[++i][0]);
				}
				else if (strncmp(&argv[i][1], "f", 1) == 0) {
					pszOutputFile = strdup(&argv[++i][0]);
				}
				else if (argv[i][1] == 'h' || argv[i][1] == '?') {
					printUsage();
                    return 0;
				}
				// else if (strncmp(&argv[i][1], "test", 4) == 0) {
				// 	test(numCores, digits);
                //     return -1;
				// }
				else {
					printf("Unknown argument '%s'", &argv[i][0]);
					printUsage();
                    return -1;
				}
			}
		}
	}
	else {
		printUsage();
        return -1;
	}

	if (digits < 1) { 
        printUsage();
        return -1;
    }

	pi = chudnovsky(digits, numCores);

    fptrOut = fopen(pszOutputFile, "wt");

    if (fptrOut == NULL) {
        fprintf(stderr, "Could not open output file '%s': %s\n", pszOutputFile, strerror(errno));
        free(pi);
        return -1;
    }

	// since there's no decimal point in the result, we'll print the
	// first digit, then the rest of it, with the expectation that the
	// decimal will appear after "3", as per usual:
	fprintf(fptrOut, "%.1s.%s\n", pi, pi + 1);

    fclose(fptrOut);

	// chudnovsky() malloc()s the result string, so let's be proper:
	free(pi);

	return 0;
}
