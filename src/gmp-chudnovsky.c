/* Pi computation using Chudnovsky's algortithm.
** Copyright 2002, 2005 Hanhong Xue (macroxue at yahoo dot com)
** Slightly modified 2005 by Torbjorn Granlund to allow more than 2G
** digits to be computed.
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 1. Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright notice,
** this list of conditions and the following disclaimer in the documentation
** and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
** EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include "gmp.h"

#define A                   13591409
#define B                   545140134
#define C                   640320
#define D                   12

#define BITS_PER_DIGIT      3.32192809488736234787
#define DIGITS_PER_ITER     14.1816474627254776555
#define DOUBLE_PREC         53

// how many to display if the user doesn't specify:
#define DEFAULT_DIGITS      100

static char *   prog_name;

#if CHECK_MEMUSAGE
#undef CHECK_MEMUSAGE
#define CHECK_MEMUSAGE						\
    do {									\
        char buf[100];						\
        snprintf(                           \
                buf,                        \
                100,						\
                "ps aguxw | grep '[%c]%s'", \
                prog_name[0],               \
                prog_name + 1);	            \
        system (buf);						\
    }                                       \
    while (0)
#else
#undef CHECK_MEMUSAGE
#define CHECK_MEMUSAGE
#endif


/* Return user CPU time measured in milliseconds.  */

#if !defined (__sun)    && \
    (defined (USG)      || \
     defined (__SVR4)   || \
     defined (_UNICOS)  || \
     defined (__hpux))
static int cputime(void) {  
    return (int) ((double) clock () * 1000 / CLOCKS_PER_SEC);
}
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

static int cputime(void) {
    struct rusage rus;

    getrusage (0, &rus);

    return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}
#endif

/*///////////////////////////////////////////////////////////////////////////*/

static mpf_t        t1;
static mpf_t        t2;

/*
** r = sqrt(x)
*/
static void my_sqrt_ui(mpf_t r, uint64_t x) {
    uint64_t        prec;
    uint64_t        bits;
    uint64_t        prec0;

    prec0 = mpf_get_prec(r);

    if (prec0 <= DOUBLE_PREC) {
        mpf_set_d(r, sqrt(x));
        return;
    }

    bits = 0;

    for (prec = prec0;prec > DOUBLE_PREC;) {
        int bit = prec & 1;

        prec = (prec + bit) >> 1;
        bits = (bits << 1) + bit;
    }

    mpf_set_prec_raw(t1, DOUBLE_PREC);
    mpf_set_d(t1, (1 / sqrt(x)));

    while (prec < prec0) {
        prec <<= 1;

        if (prec < prec0) {
            /* t1 = t1+t1*(1-x*t1*t1)/2; */
            mpf_set_prec_raw(t2, prec);
            mpf_mul(t2, t1, t1);         /* half x half -> full */
            mpf_mul_ui(t2, t2, x);
            mpf_ui_sub(t2, 1, t2);
            mpf_set_prec_raw(t2, (prec >> 1));
            mpf_div_2exp(t2, t2, 1);
            mpf_mul(t2, t2, t1);         /* half x half -> half */
            mpf_set_prec_raw(t1, prec);
            mpf_add(t1, t1, t2);
        }
        else {
            break;
        }
        
        prec -= (bits & 1);
        bits >>= 1;
    }

    /* t2=x*t1, t1 = t2+t1*(x-t2*t2)/2; */
    mpf_set_prec_raw(t2, prec0 >> 1);
    mpf_mul_ui(t2, t1, x);
    mpf_mul(r, t2, t2);          /* half x half -> full */
    mpf_ui_sub(r, x, r);
    mpf_mul(t1, t1, r);          /* half x half -> half */
    mpf_div_2exp(t1, t1, 1);
    mpf_add(r, t1, t2);
}

/* r = y/x   WARNING: r cannot be the same as y. */
#if __GMP_MP_RELEASE >= 50001
#define my_div mpf_div
#else
static void my_div(mpf_t r, mpf_t y, mpf_t x) {
    uint64_t        prec;
    uint64_t        bits;
    uint64_t        prec0;

    prec0 = mpf_get_prec(r);

    if (prec0 <= DOUBLE_PREC) {
        mpf_set_d(r, (mpf_get_d(y) / mpf_get_d(x)));
        return;
    }

    bits = 0;

    for (prec=prec0; prec>DOUBLE_PREC;) {
        int bit = prec & 1;

        prec = (prec + bit) >> 1;

        bits = (bits << 1) + bit;
    }

    mpf_set_prec_raw(t1, DOUBLE_PREC);
    mpf_ui_div(t1, 1, x);

    while (prec<prec0) {
        prec <<= 1;

        if (prec < prec0) {
            /* t1 = t1+t1*(1-x*t1); */
            mpf_set_prec_raw(t2, prec);
            mpf_mul(t2, x, t1);          /* full x half -> full */
            mpf_ui_sub(t2, 1, t2);
            mpf_set_prec_raw(t2, (prec >> 1));
            mpf_mul(t2, t2, t1);         /* half x half -> half */
            mpf_set_prec_raw(t1, prec);
            mpf_add(t1, t1, t2);
        }
        else {
            prec = prec0;

            /* t2=y*t1, t1 = t2+t1*(y-x*t2); */
            mpf_set_prec_raw(t2, (prec >> 1));
            mpf_mul(t2, t1, y);          /* half x half -> half */
            mpf_mul(r, x, t2);           /* full x half -> full */
            mpf_sub(r, y, r);
            mpf_mul(t1, t1, r);          /* half x half -> half */
            mpf_add(r, t1, t2);
            break;
        }

        prec -= (bits & 1);
        bits >>= 1;
    }
}
#endif

/*///////////////////////////////////////////////////////////////////////////*/

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

typedef struct {
    uint64_t        max_facs;
    uint64_t        num_facs;
    uint64_t *      fac;
    uint64_t *      pow;
}
fac_t[1];

typedef struct {
    int64_t         fac;
    int64_t         pow;
    int64_t         nxt;
}
sieve_t;

static sieve_t *        sieve;
static int64_t          sieve_size;
static fac_t            ftmp;
static fac_t            fmul;

#define INIT_FACS       32

static void fac_show(fac_t f) {
    int64_t           i;

    for (i = 0; i < f[0].num_facs; i++) {
        if (f[0].pow[i] == 1) {
            printf("%llu ", f[0].fac[i]);
        }
        else {
            printf("%llu ^ %llu ", f[0].fac[i], f[0].pow[i]);
        }
    }

    printf("\n");
}

static void fac_reset(fac_t f) {
    f[0].num_facs = 0;
}

static void fac_init_size(fac_t f, long int s) {
    if (s < INIT_FACS) {
        s = INIT_FACS;
    }

    f[0].fac  = malloc(s * sizeof(uint64_t) * 2);
    f[0].pow  = f[0].fac + s;
    f[0].max_facs = s;

    fac_reset(f);
}

static void fac_init(fac_t f) {
    fac_init_size(f, INIT_FACS);
}

static void fac_clear(fac_t f) {
    free(f[0].fac);
}

static void fac_resize(fac_t f, long int s) {
    if (f[0].max_facs < s) {
        fac_clear(f);
        fac_init_size(f, s);
    }
}

/* f = base^pow */
static void fac_set_bp(fac_t f, uint64_t base, long int pow) {
    int64_t         i;

    assert(base < sieve_size);

    for (i = 0, base >>= 1; base > 0; i++, base = sieve[base].nxt) {
        f[0].fac[i] = sieve[base].fac;
        f[0].pow[i] = sieve[base].pow*pow;
    }

    f[0].num_facs = i;
    
    assert(i <= f[0].max_facs);
}

/* r = f*g */
static void fac_mul2(fac_t r, fac_t f, fac_t g) {
    int64_t     i;
    int64_t     j;
    int64_t     k;

    for (i = j = k = 0; i < f[0].num_facs && j<g[0].num_facs; k++) {
        if (f[0].fac[i] == g[0].fac[j]) {
            r[0].fac[k] = f[0].fac[i];
            r[0].pow[k] = f[0].pow[i] + g[0].pow[j];
            
            i++;
            j++;
        }
        else if (f[0].fac[i] < g[0].fac[j]) {
            r[0].fac[k] = f[0].fac[i];
            r[0].pow[k] = f[0].pow[i];
            
            i++;
        }
        else {
            r[0].fac[k] = g[0].fac[j];
            r[0].pow[k] = g[0].pow[j];
            
            j++;
        }
    }

    for (; i < f[0].num_facs; i++, k++) {
        r[0].fac[k] = f[0].fac[i];
        r[0].pow[k] = f[0].pow[i];
    }

    for (; j < g[0].num_facs; j++, k++) {
        r[0].fac[k] = g[0].fac[j];
        r[0].pow[k] = g[0].pow[j];
    }

    r[0].num_facs = k;

    assert(k <= r[0].max_facs);
}

/* f *= g */
static void fac_mul(fac_t f, fac_t g) {
    fac_t       tmp;

    fac_resize(fmul, f[0].num_facs + g[0].num_facs);
    fac_mul2(fmul, f, g);

    tmp[0]  = f[0];
    f[0]    = fmul[0];
    fmul[0] = tmp[0];
}

/* f *= base^pow */
static void fac_mul_bp(fac_t f, uint64_t base, uint64_t pow) {
    fac_set_bp(ftmp, base, pow);
    fac_mul(f, ftmp);
}

/* remove factors of power 0 */
static void fac_compact(fac_t f) {
    int64_t         i;
    int64_t         j;

    for (i = 0, j = 0; i < f[0].num_facs; i++) {
        if (f[0].pow[i] > 0) {
            if (j < i) {
                f[0].fac[j] = f[0].fac[i];
                f[0].pow[j] = f[0].pow[i];
            }

            j++;
        }
    }

    f[0].num_facs = j;
}

/* convert factorized form to number */
static void bs_mul(mpz_t r, int64_t a, int64_t b) {
    int64_t         i;
    int64_t         j;

    if (b - a <= 32) {
        mpz_set_ui(r, 1);

        for (i = a; i < b; i++) {
            for (j = 0; j < fmul[0].pow[i]; j++) {
                mpz_mul_ui(r, r, fmul[0].fac[i]);
            }
        }
    }
    else {
        mpz_t           r2;

        mpz_init(r2);

        bs_mul(r2, a, (a + b) >> 1);
        bs_mul(r, (a + b) >> 1, b);

        mpz_mul(r, r, r2);
        mpz_clear(r2);
    }
}

static mpz_t            gcd;

#if HAVE_DIVEXACT_PREINV
static mpz_t    mgcd;
void mpz_invert_mod_2exp (mpz_ptr, mpz_srcptr);
void mpz_divexact_pre (mpz_ptr, mpz_srcptr, mpz_srcptr, mpz_srcptr);
#endif

/* f /= gcd(f,g), g /= gcd(f,g) */
static void fac_remove_gcd(mpz_t p, fac_t fp, mpz_t g, fac_t fg) {
    int64_t         i;
    int64_t         j;
    int64_t         k;
    int64_t         c;

    fac_resize(fmul, min(fp->num_facs, fg->num_facs));

    for (i = j = k = 0; i < fp->num_facs && j < fg->num_facs; ) {
        if (fp->fac[i] == fg->fac[j]) {
            c = min(fp->pow[i], fg->pow[j]);

            fp->pow[i] -= c;
            fg->pow[j] -= c;
            fmul->fac[k] = fp->fac[i];
            fmul->pow[k] = c;
            
            i++;
            j++;
            k++;
        } 
        else if (fp->fac[i] < fg->fac[j]) {
            i++;
        }
        else {
            j++;
        }
    }

    fmul->num_facs = k;

    assert(k <= fmul->max_facs);

    if (fmul->num_facs) {
        bs_mul(gcd, 0, fmul->num_facs);

        #if HAVE_DIVEXACT_PREINV
        mpz_invert_mod_2exp (mgcd, gcd);
        mpz_divexact_pre (p, p, gcd, mgcd);
        mpz_divexact_pre (g, g, gcd, mgcd);
        #else
        #define SIZ(x) x->_mp_size
        mpz_divexact(p, p, gcd);
        mpz_divexact(g, g, gcd);
        #endif
        
        fac_compact(fp);
        fac_compact(fg);
    }
}

/*///////////////////////////////////////////////////////////////////////////*/

static int          out = 0;
static mpz_t *      pstack;
static mpz_t *      qstack;
static mpz_t *      gstack;
static fac_t *      fpstack;
static fac_t *      fgstack;
static int64_t      top = 0;
static int64_t      gcd_time = 0;
static double       progress = 0;
static double       percent;

#define p1 (pstack[top])
#define q1 (qstack[top])
#define g1 (gstack[top])
#define fp1 (fpstack[top])
#define fg1 (fgstack[top])

#define p2 (pstack[top+1])
#define q2 (qstack[top+1])
#define g2 (gstack[top+1])
#define fp2 (fpstack[top+1])
#define fg2 (fgstack[top+1])

/* binary splitting */
static void bs(uint64_t a, uint64_t b, uint64_t gflag, int64_t level) {
    uint64_t      i;
    uint64_t      mid;
    int           ccc;

    if (b - a == 1) {
        /*
        ** g(b-1,b) = (6b-5)(2b-1)(6b-1)
        ** p(b-1,b) = b^3 * C^3 / 24
        ** q(b-1,b) = (-1)^b*g(b-1,b)*(A+Bb).
        */
        mpz_set_ui(p1, b);
        mpz_mul_ui(p1, p1, b);
        mpz_mul_ui(p1, p1, b);
        mpz_mul_ui(p1, p1, (C / 24) * (C / 24));
        mpz_mul_ui(p1, p1, C * 24);

        mpz_set_ui(g1, (2 * b) - 1);
        mpz_mul_ui(g1, g1, (6 * b) - 1);
        mpz_mul_ui(g1, g1, (6 * b) - 5);

        mpz_set_ui(q1, b);
        mpz_mul_ui(q1, q1, B);
        mpz_add_ui(q1, q1, A);
        mpz_mul   (q1, q1, g1);

        if (b % 2) {
            mpz_neg(q1, q1);
        }

        i = b;

        while ((i & 1) == 0) {
            i >>= 1;
        }

        fac_set_bp(fp1, i, 3);	/*  b^3 */
        fac_mul_bp(fp1, 3 * 5 * 23 * 29, 3);

        fp1[0].pow[0]--;

        fac_set_bp(fg1, (2 * b) - 1, 1);	/* 2b-1 */
        fac_mul_bp(fg1, (6 * b) - 1, 1);	/* 6b-1 */
        fac_mul_bp(fg1, (6 * b) - 5, 1);	/* 6b-5 */

        if (b > (int)(progress)) {
            printf(".");
            fflush(stdout);

            progress += percent * 2;
        }
    }
    else {
        /*
        ** p(a,b) = p(a,m) * p(m,b)
        ** g(a,b) = g(a,m) * g(m,b)
        ** q(a,b) = q(a,m) * p(m,b) + q(m,b) * g(a,m)
        */
        mid = a + ((b - a) * 0.5224);     /* tuning parameter */
        bs(a, mid, 1, level + 1);

        top++;

        bs(mid, b, gflag, level + 1);

        top--;

        if (level == 0) {
            puts ("");
        }

        ccc = (level == 0);

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        if (level >= 4) {           /* tuning parameter */
            #if 0
            long t = cputime();
            #endif

            fac_remove_gcd(p2, fp2, g1, fg1);
            
            #if 0
            gcd_time += cputime()-t;
            #endif
        }

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        mpz_mul(p1, p1, p2);

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        mpz_mul(q1, q1, p2);

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        mpz_mul(q2, q2, g1);

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        mpz_add(q1, q1, q2);

        if (ccc) {
            CHECK_MEMUSAGE;
        }

        fac_mul(fp1, fp2);

        if (gflag) {
            mpz_mul(g1, g1, g2);
            fac_mul(fg1, fg2);
        }
    }

    if (out & 2) {
        printf("p(%llu, %llu) = ", a, b);
        fac_show(fp1);

        if (gflag) {
            printf("g(%llu, %llu) = ", a ,b);
            fac_show(fg1);
        }
    }
}

static void build_sieve(long int n, sieve_t *s) {
    int64_t           m;
    int64_t           i;
    int64_t           j;
    int64_t           k;

    sieve_size = n;
    m = (int64_t)sqrt(n);
    memset(s, 0, sizeof(sieve_t) * n / 2);

    s[1 / 2].fac = 1;
    s[1 / 2].pow = 1;

    for (i = 3; i <= n; i += 2) {
        if (s[i/2].fac == 0) {
            s[i/2].fac = i;
            s[i/2].pow = 1;

            if (i <= m) {
                for (j = i * i, k = i >> 1; j <= n; j += i + i, k++) {
                    if (s[j/2].fac == 0) {
                        s[j/2].fac = i;

                        if (s[k].fac == i) {
                            s[j/2].pow = s[k].pow + 1;
                            s[j/2].nxt = s[k].nxt;
                        }
                        else {
                            s[j/2].pow = 1;
                            s[j/2].nxt = k;
                        }
                    }
                }
            }
        }
    }
}

static void printUsage(void) {
	printf("\n Usage: chudnovsky [OPTIONS]\n\n");
	printf("  Options:\n");
	printf("   -h/?                 Print this help\n");
	printf("   -digits num_digits   Number of pi digits to compute\n");
	printf("   -f output_file       The output file\n");
	printf("\n");
}

int main(int argc, char *argv[]) {
    char *          endptr;
    char *          pszOutputFile;
	uint64_t        digits = DEFAULT_DIGITS;
    FILE *          fptrOut;
    mpf_t           pi;
    mpf_t           qi;
    int64_t         i;
    int64_t         depth = 1;
    int64_t         terms;
    uint64_t        psize;
    uint64_t        qsize;
    int64_t         begin;
    int64_t         mid0;
    int64_t         mid1;
    int64_t         mid2;
    int64_t         mid3;
    int64_t         mid4;
    int64_t         end;

    prog_name = argv[0];

	if (argc > 1) {
		for (i = 1;i < argc;i++) {
			if (argv[i][0] == '-') {
				if (strncmp(&argv[i][1], "digits", 6) == 0) {
                    digits = strtoul(&argv[++i][0], &endptr, 10);

                    if (*endptr != '\0') { 
                        printUsage();
                        return -1;
                    }
				}
				else if (strncmp(&argv[i][1], "f", 1) == 0) {
					pszOutputFile = strdup(&argv[++i][0]);
				}
				else if (argv[i][1] == 'h' || argv[i][1] == '?') {
					printUsage();
                    return 0;
				}
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

    terms = digits / DIGITS_PER_ITER;

    while ((1L << depth) < terms) {
        depth++;
    }

    depth++;
    
    percent = (double)terms / 100.0;

    printf("#terms=%lld, depth=%lld\n", terms, depth);

    begin = cputime();

    printf("sieve   ");
    fflush(stdout);

    sieve_size = max(3 * 5 * 23 * 29 + 1, terms * 6);
    sieve = (sieve_t *)malloc(sizeof(sieve_t) * sieve_size / 2);

    build_sieve(sieve_size, sieve);

    mid0 = cputime();
    
    printf("time = %6.3f\n", (double)(mid0 - begin) / 1000.0);

    /* allocate stacks */
    pstack =    malloc(sizeof(mpz_t) * depth);
    qstack =    malloc(sizeof(mpz_t) * depth);
    gstack =    malloc(sizeof(mpz_t) * depth);
    fpstack =   malloc(sizeof(fac_t) * depth);
    fgstack =   malloc(sizeof(fac_t) * depth);

    for (i = 0; i < depth; i++) {
        mpz_init(pstack[i]);
        mpz_init(qstack[i]);
        mpz_init(gstack[i]);

        fac_init(fpstack[i]);
        fac_init(fgstack[i]);
    }

    mpz_init(gcd);
    
    #if HAVE_DIVEXACT_PREINV
    mpz_init(mgcd);
    #endif
    
    fac_init(ftmp);
    fac_init(fmul);

    /* begin binary splitting process */
    if (terms <= 0) {
        mpz_set_ui(p2, 1);
        mpz_set_ui(q2, 0);
        mpz_set_ui(g2, 1);
    }
    else {
        bs(0, terms, 0, 0);
    }

    mid1 = cputime();

    printf("\n");
    printf("bs      time = %6.3f\n", (double)(mid1 - mid0) / 1000.0);
    printf("gcd     time = %6.3f\n", (double)(gcd_time) / 1000.0);

    /* free some resources */
    free(sieve);

    #if HAVE_DIVEXACT_PREINV
    mpz_clear(mgcd);
    #endif

    mpz_clear(gcd);
    fac_clear(ftmp);
    fac_clear(fmul);

    for (i = 1; i < depth; i++) {
        mpz_clear(pstack[i]);
        mpz_clear(qstack[i]);
        mpz_clear(gstack[i]);

        fac_clear(fpstack[i]);
        fac_clear(fgstack[i]);
    }

    mpz_clear(gstack[0]);

    fac_clear(fpstack[0]);
    fac_clear(fgstack[0]);

    free(gstack);
    free(fpstack);
    free(fgstack);

    /* prepare to convert integers to floats */
    mpf_set_default_prec((int64_t)((digits * BITS_PER_DIGIT) + 16));

    /*
    p*(C/D)*sqrt(C)
    pi = -----------------
        (q+A*p)
    */

    psize = mpz_sizeinbase(p1, 10);
    qsize = mpz_sizeinbase(q1, 10);

    mpz_addmul_ui(q1, p1, A);
    mpz_mul_ui(p1, p1, C / D);

    mpf_init(pi);
    mpf_set_z(pi, p1);
    mpz_clear(p1);

    mpf_init(qi);
    mpf_set_z(qi, q1);
    mpz_clear(q1);

    free(pstack);
    free(qstack);

    mid2 = cputime();

    /* initialize temp float variables for sqrt & div */
    mpf_init(t1);
    mpf_init(t2);

    /* final step */
    printf("div     ");
    fflush(stdout);
    
    my_div(qi, pi, qi);
    
    mid3 = cputime();
    
    printf("time = %6.3f\n", (double)(mid3 - mid2) / 1000.0);

    printf("sqrt    ");  fflush(stdout);
    my_sqrt_ui(pi, C);
    mid4 = cputime();
    printf("time = %6.3f\n", (double)(mid4-mid3)/1000);

    printf("mul     ");  fflush(stdout);
    mpf_mul(qi, qi, pi);
    end = cputime();
    printf("time = %6.3f\n", (double)(end-mid4)/1000);

    printf("total   time = %6.3f\n", (double)(end-begin)/1000);
    fflush(stdout);

    printf(
        "   P size = %llu digits (%f)\n   Q size = %llu digits (%f)\n",
        psize, 
        (double)psize / (double)digits, 
        qsize, 
        (double)qsize / (double)digits);

    fptrOut = fopen(pszOutputFile, "wt");

    if (fptrOut == NULL) {
        fprintf(stderr, "Could not open output file '%s': %s\n", pszOutputFile, strerror(errno));
        mpf_clear(qi);
        return -1;
    }

    /* output Pi and timing statistics */
    printf("pi[0..%lld]\n", terms);
    mpf_out_str(fptrOut, 10, digits + 2, qi);

    fclose(fptrOut);

    /* free float resources */
    mpf_clear(pi);
    mpf_clear(qi);

    mpf_clear(t1);
    mpf_clear(t2);

    return 0;
}
