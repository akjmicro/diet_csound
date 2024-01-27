/*
    streson.c:

    Copyright (C) 1996 John ffitch
                  1998 Victor Lazzarini

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "stdopcod.h"
#include "streson.h"

/*******************************************************/
/* streson.c : string resonator opcode                 */
/*             takes one input and passes it through   */
/*             emulates the resonance                  */
/*             of a string tuned to a kfun fundamental */
/*          Victor Lazzarini, 1998                     */
/*******************************************************/

static int32_t stresonset(CSOUND *csound, STRES *p)
{
    p->size = (int32_t) (CS_ESR/20);   /* size of delay line */
    csound->AuxAlloc(csound, p->size*sizeof(MYFLT), &p->aux);
    p->Cdelay = (MYFLT*) p->aux.auxp; /* delay line */
    p->LPdelay = p->APdelay = FL(0.0); /* reset the All-pass and Low-pass delays */
    p->wpointer = p->rpointer = 0; /* reset the read/write pointers */
    memset(p->Cdelay, '\0', p->size*sizeof(MYFLT));
    return OK;
}

static int32_t streson(CSOUND *csound, STRES *p)
{
    MYFLT *out = p->result;
    MYFLT *in = p->ainput;
    MYFLT g = *p->ifdbgain;
    MYFLT freq;
    double a, s, w, sample, tdelay, fracdelay;
    int32_t delay;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t rp = p->rpointer, wp = p->wpointer;
    int32_t size = p->size;
    MYFLT       APdelay = p->APdelay;
    MYFLT       LPdelay = p->LPdelay;
    int32_t         vdt;

    freq = *p->afr;
    if (UNLIKELY(freq < FL(20.0))) freq = FL(20.0);   /* lowest freq is 20 Hz */
    tdelay = CS_ESR/freq;
    delay = (int32_t) (tdelay - 0.5); /* comb delay */
    fracdelay = tdelay - (delay + 0.5); /* fractional delay */
    vdt = size - delay;       /* set the var delay */
    a = (1.0-fracdelay)/(1.0+fracdelay);   /* set the all-pass gain */
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      /* GetSample(p); */
      MYFLT tmpo;
      rp = (vdt + wp);
      if (UNLIKELY(rp >= size)) rp -= size;
      tmpo = p->Cdelay[rp];
      w = in[n] + tmpo;
      s = (LPdelay + w)*0.5;
      LPdelay = w;
      out[n] = sample = APdelay + s*a;
      APdelay = s - (sample*a);
      p->Cdelay[wp] = sample*g;
      wp++;
      if (UNLIKELY(wp == size)) wp=0;
    }
    p->rpointer = rp; p->wpointer = wp;
    p->LPdelay = LPdelay; p->APdelay = APdelay;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY streson_localops[] =
{
    { "streson", S(STRES),    0, 3, "a",  "akk",  (SUBR)stresonset, (SUBR)streson}
};

LINKAGE_BUILTIN(streson_localops)
