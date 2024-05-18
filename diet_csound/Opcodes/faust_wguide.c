/* ------------------------------------------------------------
name: "faust_wguide"
Code generated with Faust 2.66.5 (https://faust.grame.fr)
------------------------------------------------------------ */

#include "csoundCore.h"
#include <math.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct {
    OPDS   h;
    MYFLT  *ar, *ain, *param_freq, *param_res;
    int    fSampleRate;
    MYFLT  fConst0;
    MYFLT  fConst1;
    int    IOTA0;
    MYFLT  fVec0[2048];
    MYFLT  fRec0[2];
    MYFLT  fRec1[2];
} FAUST_WGUIDE;


static int faust_wguide_init(CSOUND* csound, FAUST_WGUIDE *p)
{
    p->fSampleRate = (int)csound->GetSr(csound);
    p->fConst0 = fmin(1.92e+05, fmax(1.0, (MYFLT)(p->fSampleRate)));
    p->fConst1 = 1.0 / p->fConst0;
    p->IOTA0 = 0;
    /* C99 loop */
    {
      int l0;
      for (l0 = 0; l0 < 2048; l0 = l0 + 1) {
        p->fVec0[l0] = 0.0;
      }
    }
    /* C99 loop */
    {
      int l1;
      for (l1 = 0; l1 < 2; l1 = l1 + 1) {
        p->fRec0[l1] = 0.0;
      }
    }
    /* C99 loop */
    {
      int l2;
      for (l2 = 0; l2 < 2; l2 = l2 + 1) {
        p->fRec1[l2] = 0.0;
      }
    }
    return OK;
}

static int faust_wguide(CSOUND* csound, FAUST_WGUIDE* p)
{
    // interface with struct
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = (uint32_t) csound->GetKsmps(csound);
    MYFLT *in = p->ain;
    MYFLT *out = p->ar;
    MYFLT param_freq = *p->param_freq;
    MYFLT param_res = *p->param_res;
    // initial calc setup
    MYFLT f0 = 0.001 * param_res;
    int i1 = fabs(f0) < 2.220446049250313e-16;
    MYFLT f2 = 0.0 - ((i1) ? 0.0 : exp(0.0 - p->fConst1 / ((i1) ? 1.0 : f0)));
    MYFLT f3 = p->fConst0 / param_freq;
    MYFLT f4 = f3 + -1.0;
    int i5 = (int)(f4);
    int i6 = (int)fmin(1025, fmax(0, i5 + 1));
    MYFLT f7 = floor(f4);
    MYFLT f8 = f3 + (-1.0 - f7);
    int i9 = (int)fmin(1025, fmax(0, i5));
    MYFLT f10 = f7 + (2.0 - f3);
    /*
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    */
    /* C99 loop */
    {
        int i0;
        for (i0 = 0; i0 < nsmps; i0++) {
            MYFLT fTemp0 = (MYFLT)(in[i0]) - p->fRec0[1] * f2;
            p->fVec0[p->IOTA0 & 2047] = fTemp0;
            p->fRec0[0] = f10 * p->fVec0[(p->IOTA0 - i9) & 2047] + f8 * p->fVec0[(p->IOTA0 - i6) & 2047];
            p->fRec1[0] = fTemp0;
            out[i0] = (MYFLT)(p->fRec1[1]);
            p->IOTA0 = p->IOTA0 + 1;
            p->fRec0[1] = p->fRec0[0];
            p->fRec1[1] = p->fRec1[0];
        }
    }
    return OK;
}

static OENTRY faustwguide_localops[] = {
    { "faust_wguide", sizeof(FAUST_WGUIDE), 0, 3, "a", "aJJ", (SUBR)faust_wguide_init, (SUBR)faust_wguide }
};

LINKAGE_BUILTIN(faustwguide_localops)
