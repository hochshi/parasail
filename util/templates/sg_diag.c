/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2014 Battelle Memorial Institute.
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#include "config.h"

#include <stdlib.h>

%(HEADER)s

#include "parasail.h"
#include "parasail/memory.h"
#include "parasail/internal_%(ISA)s.h"

#define NEG_INF %(NEG_INF)s
%(FIXES)s

#ifdef PARASAIL_TABLE
static inline void arr_store_si128(
        int *array,
        %(VTYPE)s vWscore,
        %(INDEX)s i,
        %(INDEX)s s1Len,
        %(INDEX)s j,
        %(INDEX)s s2Len)
{
%(PRINTER)s
}
#endif


#ifdef PARASAIL_TABLE
#define FNAME %(NAME_TABLE)s
#else
#define FNAME %(NAME)s
#endif

parasail_result_t* FNAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix)
{
    const %(INDEX)s N = %(LANES)s; /* number of values in vector */
    const %(INDEX)s PAD = N-1;
    const %(INDEX)s PAD2 = PAD*2;
    const %(INDEX)s s1Len_PAD = s1Len+PAD;
    const %(INDEX)s s2Len_PAD = s2Len+PAD;
    %(INT)s * const restrict s1 = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s1Len+PAD);
    %(INT)s * const restrict s2B= parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _tbl_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _del_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict s2 = s2B+PAD; /* will allow later for negative indices */
    %(INT)s * const restrict tbl_pr = _tbl_pr+PAD;
    %(INT)s * const restrict del_pr = _del_pr+PAD;
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table1(s1Len, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif
    %(INDEX)s i = 0;
    %(INDEX)s j = 0;
    %(INT)s score = NEG_INF;
    %(VTYPE)s vNegInf = %(VSET1)s(NEG_INF);
    %(VTYPE)s vNegInf0 = %(VRSHIFT)s(vNegInf, %(BYTES)s); /* shift in a 0 */
    %(VTYPE)s vOpen = %(VSET1)s(open);
    %(VTYPE)s vGap  = %(VSET1)s(gap);
    %(VTYPE)s vOne = %(VSET1)s(1);
    %(VTYPE)s vN = %(VSET1)s(N);
    %(VTYPE)s vNegOne = %(VSET1)s(-1);
    %(VTYPE)s vI = %(VSET)s(%(DIAG_I)s);
    %(VTYPE)s vJreset = %(VSET)s(%(DIAG_J)s);
    %(VTYPE)s vMaxScore = vNegInf;
    %(VTYPE)s vILimit = %(VSET1)s(s1Len);
    %(VTYPE)s vILimit1 = %(VSUB)s(vILimit, vOne);
    %(VTYPE)s vJLimit = %(VSET1)s(s2Len);
    %(VTYPE)s vJLimit1 = %(VSUB)s(vJLimit, vOne);
    %(SATURATION_CHECK_INIT)s

    /* convert _s1 from char to int in range 0-23 */
    for (i=0; i<s1Len; ++i) {
        s1[i] = matrix->mapper[(unsigned char)_s1[i]];
    }
    /* pad back of s1 with dummy values */
    for (i=s1Len; i<s1Len_PAD; ++i) {
        s1[i] = 0; /* point to first matrix row because we don't care */
    }

    /* convert _s2 from char to int in range 0-23 */
    for (j=0; j<s2Len; ++j) {
        s2[j] = matrix->mapper[(unsigned char)_s2[j]];
    }
    /* pad front of s2 with dummy values */
    for (j=-PAD; j<0; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }
    /* pad back of s2 with dummy values */
    for (j=s2Len; j<s2Len_PAD; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }

    /* set initial values for stored row */
    for (j=0; j<s2Len; ++j) {
        tbl_pr[j] = 0;
        del_pr[j] = NEG_INF;
    }
    /* pad front of stored row values */
    for (j=-PAD; j<0; ++j) {
        tbl_pr[j] = NEG_INF;
        del_pr[j] = NEG_INF;
    }
    /* pad back of stored row values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        tbl_pr[j] = NEG_INF;
        del_pr[j] = NEG_INF;
    }

    /* iterate over query sequence */
    for (i=0; i<s1Len; i+=N) {
        %(VTYPE)s vNscore = vNegInf0;
        %(VTYPE)s vWscore = vNegInf0;
        %(VTYPE)s vIns = vNegInf;
        %(VTYPE)s vDel = vNegInf;
        %(VTYPE)s vJ = vJreset;
        %(DIAG_MATROW_DECL)s
        %(VTYPE)s vIltLimit = %(VCMPLT)s(vI, vILimit);
        %(VTYPE)s vIeqLimit1 = %(VCMPEQ)s(vI, vILimit1);
        /* iterate over database sequence */
        for (j=0; j<s2Len+PAD; ++j) {
            %(VTYPE)s vMat;
            %(VTYPE)s vNWscore = vNscore;
            vNscore = %(VRSHIFT)s(vWscore, %(BYTES)s);
            vNscore = %(VINSERT)s(vNscore, tbl_pr[j], %(LAST_POS)s);
            vDel = %(VRSHIFT)s(vDel, %(BYTES)s);
            vDel = %(VINSERT)s(vDel, del_pr[j], %(LAST_POS)s);
            vDel = %(VMAX)s(
                    %(VSUB)s(vNscore, vOpen),
                    %(VSUB)s(vDel, vGap));
            vIns = %(VMAX)s(
                    %(VSUB)s(vWscore, vOpen),
                    %(VSUB)s(vIns, vGap));
            vMat = %(VSET)s(
                    %(DIAG_MATROW_USE)s
                    );
            vNWscore = %(VADD)s(vNWscore, vMat);
            vWscore = %(VMAX)s(vNWscore, vIns);
            vWscore = %(VMAX)s(vWscore, vDel);
            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                %(VTYPE)s cond = %(VCMPEQ)s(vJ,vNegOne);
                vWscore = %(VANDNOT)s(cond, vWscore);
                vDel = %(VBLEND)s(vDel, vNegInf, cond);
                vIns = %(VBLEND)s(vIns, vNegInf, cond);
            }
            %(SATURATION_CHECK_MID)s
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vWscore, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vWscore,0);
            del_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vDel,0);
            /* as minor diagonal vector passes across the i or j limit
             * boundary, extract the last value of the column or row */
            {
                %(VTYPE)s vJeqLimit1 = %(VCMPEQ)s(vJ, vJLimit1);
                %(VTYPE)s vJgtNegOne = %(VCMPGT)s(vJ, vNegOne);
                %(VTYPE)s vJltLimit = %(VCMPLT)s(vJ, vJLimit);
                %(VTYPE)s cond_j = %(VAND)s(vIltLimit, vJeqLimit1);
                %(VTYPE)s cond_i = %(VAND)s(vIeqLimit1,
                        %(VAND)s(vJgtNegOne, vJltLimit));
                %(VTYPE)s cond_max = %(VCMPGT)s(vWscore, vMaxScore);
                %(VTYPE)s cond_all = %(VAND)s(cond_max,
                        %(VOR)s(cond_i, cond_j));
                vMaxScore = %(VBLEND)s(vMaxScore, vWscore, cond_all);
            }
            vJ = %(VADD)s(vJ, vOne);
        }
        vI = %(VADD)s(vI, vN);
    }

    /* max in vMaxScore */
    for (i=0; i<N; ++i) {
        %(INT)s value;
        value = (%(INT)s) %(VEXTRACT)s(vMaxScore, %(LAST_POS)s);
        if (value > score) {
            score = value;
        }
        vMaxScore = %(VSHIFT)s(vMaxScore, %(BYTES)s);
    }

    %(SATURATION_CHECK_FINAL)s

    result->score = score;

    parasail_free(_del_pr);
    parasail_free(_tbl_pr);
    parasail_free(s2B);
    parasail_free(s1);

    return result;
}

