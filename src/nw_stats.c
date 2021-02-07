/**
 * @file
 *
 * @author jeffrey.daily@gmail.com
 *
 * Copyright (c) 2015 Battelle Memorial Institute.
 */
#include "config.h"

#include <stdint.h>
#include <stdlib.h>

#include "parasail.h"
#include "parasail/memory.h"

#define NEG_INF_32 (INT32_MIN/2)
#define MAX(a,b) ((a)>(b)?(a):(b))

#ifdef PARASAIL_TABLE
#define ENAME parasail_nw_stats_table
#else
#ifdef PARASAIL_ROWCOL
#define ENAME parasail_nw_stats_rowcol
#else
#define ENAME parasail_nw_stats
#endif
#endif

parasail_result_t* ENAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix)
{
    /* declare local variables */
    parasail_result_t *result = NULL;
    int * restrict s1 = NULL;
    int * restrict s2 = NULL;
    int * restrict H = NULL;
    int * restrict HM = NULL;
    int * restrict HS = NULL;
    int * restrict HL = NULL;
    int * restrict F = NULL;
    int * restrict FM = NULL;
    int * restrict FS = NULL;
    int * restrict FL = NULL;
    int i = 0;
    int j = 0;

    /* validate inputs */
    PARASAIL_CHECK_NULL(_s1);
    PARASAIL_CHECK_GT0(s1Len);
    PARASAIL_CHECK_NULL(_s2);
    PARASAIL_CHECK_GT0(s2Len);
    PARASAIL_CHECK_GE0(open);
    PARASAIL_CHECK_GE0(gap);
    PARASAIL_CHECK_NULL(matrix);

    /* initialize stack variables */

    /* initialize result */
#ifdef PARASAIL_TABLE
    result = parasail_result_new_table3(s1Len, s2Len);
#else
#ifdef PARASAIL_ROWCOL
    result = parasail_result_new_rowcol3(s1Len, s2Len);
#else
    result = parasail_result_new_stats();
#endif
#endif
    if (!result) return NULL;

    /* set known flags */
    result->flag |= PARASAIL_FLAG_NW | PARASAIL_FLAG_NOVEC
        | PARASAIL_FLAG_STATS
        | PARASAIL_FLAG_BITS_INT | PARASAIL_FLAG_LANES_1;
#ifdef PARASAIL_TABLE
    result->flag |= PARASAIL_FLAG_TABLE;
#endif
#ifdef PARASAIL_ROWCOL
    result->flag |= PARASAIL_FLAG_ROWCOL;
#endif

    /* initialize heap variables */
    s1 = parasail_memalign_int(16, s1Len);
    s2 = parasail_memalign_int(16, s2Len);
    H = parasail_memalign_int(16, s2Len+1);
    HM = parasail_memalign_int(16, s2Len+1);
    HS = parasail_memalign_int(16, s2Len+1);
    HL = parasail_memalign_int(16, s2Len+1);
    F = parasail_memalign_int(16, s2Len+1);
    FM = parasail_memalign_int(16, s2Len+1);
    FS = parasail_memalign_int(16, s2Len+1);
    FL = parasail_memalign_int(16, s2Len+1);

    /* validate heap variables */
    if (!s1) return NULL;
    if (!s2) return NULL;
    if (!H) return NULL;
    if (!HM) return NULL;
    if (!HS) return NULL;
    if (!HL) return NULL;
    if (!F) return NULL;
    if (!FM) return NULL;
    if (!FS) return NULL;
    if (!FL) return NULL;

    for (i=0; i<s1Len; ++i) {
        s1[i] = matrix->mapper[(unsigned char)_s1[i]];
    }
    for (j=0; j<s2Len; ++j) {
        s2[j] = matrix->mapper[(unsigned char)_s2[j]];
    }

    /* upper left corner */
    H[0] = 0;
    HM[0] = 0;
    HS[0] = 0;
    HL[0] = 0;
    F[0] = NEG_INF_32;
    FM[0] = 0;
    FS[0] = 0;
    FL[0] = 0;
    
    /* first row */
    for (j=1; j<=s2Len; ++j) {
        H[j] = -open - (j-1)*gap;
        HM[j] = 0;
        HS[j] = 0;
        HL[j] = 0;
        F[j] = NEG_INF_32;
        FM[j] = 0;
        FS[j] = 0;
        FL[j] = 0;
    }

    /* iter over first sequence */
    for (i=1; i<=s1Len; ++i) {
        const int * const restrict matrow = &matrix->matrix[matrix->size*s1[i-1]];
        /* init first column */
        int NH = H[0];
        int NHM = HM[0];
        int NHS = HS[0];
        int NHL = HL[0];
        int WH = -open - (i-1)*gap;
        int WHM = 0;
        int WHS = 0;
        int WHL = 0;
        int E = NEG_INF_32;
        int EM = 0;
        int ES = 0;
        int EL = 0;
        H[0] = WH;
        HM[0] = WHM;
        HS[0] = WHS;
        HL[0] = WHL;
        for (j=1; j<=s2Len; ++j) {
            int H_dag;
            int H_new;
            int E_opn;
            int E_ext;
            int F_opn;
            int F_ext;
            int NWH = NH;
            int NWM = NHM;
            int NWS = NHS;
            int NWL = NHL;
            NH = H[j];
            NHM = HM[j];
            NHS = HS[j];
            NHL = HL[j];
            F_opn = NH - open;
            F_ext  = F[j] - gap;
            F[j]  = MAX(F_opn, F_ext);
            E_opn = WH - open;
            E_ext = E - gap;
            E = MAX(E_opn, E_ext);
            H_dag = NWH + matrow[s2[j-1]];
            H_new = MAX(H_dag, E);
            H_new = MAX(H_new, F[j]);
            if (F_opn > F_ext) {
                FM[j] = NHM;
                FS[j] = NHS;
                FL[j] = NHL;
            }
            FL[j] += 1;
            if (E_opn > E_ext) {
                EM = WHM;
                ES = WHS;
                EL = WHL;
            }
            EL += 1;
            WH = H_new;
            if (H_new == H_dag) {
                WHM  = NWM + (s1[i-1] == s2[j-1]);
                WHS  = NWS + (matrow[s2[j-1]] > 0);
                WHL  = NWL + 1;
            }
            else if (H_new == F[j]) {
                WHM  = FM[j];
                WHS  = FS[j];
                WHL  = FL[j];
            }
            else {
                WHM  = EM;
                WHS  = ES;
                WHL  = EL;
            }
            H[j] = WH;
            HM[j] = WHM;
            HS[j] = WHS;
            HL[j] = WHL;
#ifdef PARASAIL_TABLE
            result->stats->tables->score_table[1LL*(i-1)*s2Len + (j-1)] = WH;
            result->stats->tables->matches_table[1LL*(i-1)*s2Len + (j-1)] = WHM;
            result->stats->tables->similar_table[1LL*(i-1)*s2Len + (j-1)] = WHS;
            result->stats->tables->length_table[1LL*(i-1)*s2Len + (j-1)] = WHL;
#endif
        }
#ifdef PARASAIL_ROWCOL
        result->stats->rowcols->score_col[i-1] = WH;
        result->stats->rowcols->matches_col[i-1] = WHM;
        result->stats->rowcols->similar_col[i-1] = WHS;
        result->stats->rowcols->length_col[i-1] = WHL;
#endif
    }
#ifdef PARASAIL_ROWCOL
    for (j=1; j<=s2Len; ++j) {
        result->stats->rowcols->score_row[j-1] = H[j];
        result->stats->rowcols->matches_row[j-1] = HM[j];
        result->stats->rowcols->similar_row[j-1] = HS[j];
        result->stats->rowcols->length_row[j-1] = HL[j];
    }
#endif

    result->score = H[s2Len];
    result->end_query = s1Len-1;
    result->end_ref = s2Len-1;
    result->stats->matches = HM[s2Len];
    result->stats->similar = HS[s2Len];
    result->stats->length = HL[s2Len];

    parasail_free(FL);
    parasail_free(FS);
    parasail_free(FM);
    parasail_free(F);
    parasail_free(HL);
    parasail_free(HS);
    parasail_free(HM);
    parasail_free(H);
    parasail_free(s2);
    parasail_free(s1);

    return result;
}

