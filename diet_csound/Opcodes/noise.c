/*
    noise.c:

    Copyright (C) 1995 Barry Vercoe

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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"
#include <math.h>
#include "cwindow.h"
#include "noise.h"
#include "uggab.h"
#include <inttypes.h>

/* pinkish: Two methods for pink-type noise generation
   The Moore/Gardner method, coded by Phil Burke, optimised by James McCartney;
   Paul Kellet's  -3dB/octave white->pink filter bank, "refined" version;
   Paul Kellet's  -3dB/octave white->pink filter bank, "economy" version

   The Moore/Gardner method output seems to have bumps in the low-mid and
   mid-high ranges.
   The Kellet method (refined) has smooth spectrum, but goes up slightly
   at the far high end.
 */

#define rand_31(x) (x->Rand31(&(x->randSeed1)) - 1)

#define GARDNER_PINK        FL(0.0)
#define KELLET_PINK         FL(1.0)
#define KELLET_CHEAP_PINK   FL(2.0)

int32_t GardnerPink_init(CSOUND *csound, PINKISH *p);
int32_t GardnerPink_perf(CSOUND *csound, PINKISH *p);

int32_t pinkset(CSOUND *csound, PINKISH *p)
{
        /* Check valid method */
    if (UNLIKELY(*p->imethod != GARDNER_PINK && *p->imethod != KELLET_PINK
                 && *p->imethod != KELLET_CHEAP_PINK)) {
      return csound->InitError(csound, Str("pinkish: Invalid method code"));
    }
    /* User range scaling can be a- or k-rate for Gardner, a-rate only
       for filter */
    if (IS_ASIG_ARG(p->xin)) {
      p->ampinc = 1;
    }
    else {
      /* Cannot accept k-rate input with filter method */
      if (UNLIKELY(*p->imethod != FL(0.0))) {
        return csound->InitError(csound, Str(
                      "pinkish: Filter method requires a-rate (noise) input"));
      }
      p->ampinc = 0;
    }
    /* Unless we're reinitializing a tied note, zero coefs */
    if (*p->iskip != FL(1.0)) {
      if (*p->imethod == GARDNER_PINK)
        GardnerPink_init(csound,p);
      else                                      /* Filter method */
        p->b0 = p->b1 = p->b2 = p->b3 = p->b4 = p->b5 = p->b6 = FL(0.0);
    }
    return OK;
}

int32_t pinkish(CSOUND *csound, PINKISH *p)
{
    MYFLT       *aout, *ain;
    double      c0, c1, c2, c3, c4, c5, c6, nxtin, nxtout;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    aout = p->aout;
    ain = p->xin;
    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (*p->imethod == GARDNER_PINK) {  /* Gardner method (default) */
      GardnerPink_perf(csound,p);
    }
    else if (*p->imethod == KELLET_PINK) {
      /* Paul Kellet's "refined" pink filter */
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;
      c3 = p->b3; c4 = p->b4; c5 = p->b5; c6 = p->b6;
      for (n=offset;n<nsmps;n++) {
        nxtin = (double)ain[n];
        c0 = c0 * 0.99886 + nxtin * 0.0555179;
        c1 = c1 * 0.99332 + nxtin * 0.0750759;
        c2 = c2 * 0.96900 + nxtin * 0.1538520;
        c3 = c3 * 0.86650 + nxtin * 0.3104856;
        c4 = c4 * 0.55000 + nxtin * 0.5329522;
        c5 = c5 * -0.7616 - nxtin * 0.0168980;
        nxtout = c0 + c1 + c2 + c3 + c4 + c5 + c6 + nxtin * 0.5362;
        aout[n] = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
        c6 = nxtin * 0.115926;
      }
      /* Store back filter coef states */
      p->b0 = c0; p->b1 = c1; p->b2 = c2;
      p->b3 = c3; p->b4 = c4; p->b5 = c5; p->b6 = c6;
    }
    else if (*p->imethod == KELLET_CHEAP_PINK) {
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;

      for (n=offset;n<nsmps;n++) {      /* Paul Kellet's "economy" pink filter */
        nxtin = (double)ain[n];
        c0 = c0 * 0.99765 + nxtin * 0.0990460;
        c1 = c1 * 0.96300 + nxtin * 0.2965164;
        c2 = c2 * 0.57000 + nxtin * 1.0526913;
        nxtout = c0 + c1 + c2 + nxtin * 0.1848;
        aout[n] = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
      }

      /* Store back filter coef states */
      p->b0 = c0; p->b1 = c1; p->b2 = c2;
    }
    return OK;
}

/************************************************************/
/*
        GardnerPink_init() and GardnerPink_perf()

        Generate Pink Noise using Gardner method.
        Optimization suggested by James McCartney uses a tree
        to select which random value to replace.

    x x x x x x x x x x x x x x x x
     x   x   x   x   x   x   x   x
       x       x       x       x
           x               x
                   x

    Tree is generated by counting trailing zeros in an increasing index.
        When the index is zero, no random number is selected.

    Author: Phil Burk, http://www.softsynth.com

        Revision History:
                Csound version by rasmus ekman May 2000
                Several changes, some marked "(re)"

    Copyright 1999 Phil Burk - No rights reserved.
*/

/************************************************************/

/* Yet another pseudo-random generator. Could probably be changed
   for any of the other available ones in Csound */

#define PINK_RANDOM_BITS       (24)
/* Left-shift one bit less 24 to allow negative values (re) */
#define PINK_RANDOM_SHIFT      (7)

/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static int32 GenerateRandomNumber(uint32 randSeed)
{
    randSeed = ((uint32_t) randSeed * 196314165U) + 907633515UL;
    return (int32) ((int32_t) ((uint32_t) randSeed));
}

/************************************************************/

/* Set up for user-selected number of bands of noise generators. */
int32_t GardnerPink_init(CSOUND *csound, PINKISH *p)
{
    int32_t i;
    MYFLT pmax;
    int32 numRows;

    /* Set number of rows to use (default to 20) */
    if (*p->iparam1 >= 4 && *p->iparam1 <= GRD_MAX_RANDOM_ROWS)
      p->grd_NumRows = (int32)*p->iparam1;
    else {
      p->grd_NumRows = 20;
      /* Warn if user tried but failed to give sensible number */
      if (UNLIKELY(*p->iparam1 != FL(0.0)))
        csound->Warning(csound, Str("pinkish: Gardner method requires 4-%d bands. "
                                    "Default %"PRIi32" substituted for %d.\n"),
                        GRD_MAX_RANDOM_ROWS, p->grd_NumRows,
                        (int32_t) *p->iparam1);
    }

    /* Seed random generator by user value or by time (default) */
    if (*p->iseed != FL(0.0)) {
      if (*p->iseed > -1.0 && *p->iseed < 1.0)
        p->randSeed = (uint32) (*p->iseed * (MYFLT)0x80000000);
      else p->randSeed = (uint32) *p->iseed;
    }
    else p->randSeed = (uint32) csound->GetRandomSeedFromTime();

    numRows = p->grd_NumRows;
    p->grd_Index = 0;
    if (numRows == 32) p->grd_IndexMask = 0xFFFFFFFF;
    else p->grd_IndexMask = (1<<numRows) - 1;

    /* Calculate reasonable maximum signed random value. */
    /* Tweaked to get sameish peak value over all numRows values (re) */
    pmax = (MYFLT)((numRows + 30) * (1<<(PINK_RANDOM_BITS-2)));
    p->grd_Scalar = FL(1.0) / pmax;

/* Warm up by filling all rows (re) (original zeroed all rows, and runningSum) */
    {
      int32 randSeed, newRandom, runningSum = 0;
      randSeed = p->randSeed;
      for (i = 0; i < numRows; i++) {
        randSeed = GenerateRandomNumber(randSeed);
        newRandom = randSeed >> PINK_RANDOM_SHIFT;
        runningSum += newRandom;
        p->grd_Rows[i] = newRandom;
      }
      p->grd_RunningSum = runningSum;
      p->randSeed = randSeed;
    }
    return OK;
}

/* Generate numRows octave-spaced white bands and sum to pink noise. */
int32_t GardnerPink_perf(CSOUND *csound, PINKISH *p)
{
    IGN(csound);
    MYFLT *aout, *amp, scalar;
    int32 *rows, rowIndex, indexMask, randSeed, newRandom;
    int32 runningSum, sum, ampinc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS - p->h.insdshead->ksmps_no_end;

    aout        = p->aout;
    amp         = p->xin;
    ampinc      = p->ampinc;    /* Used to increment user amp if a-rate */
    scalar      = p->grd_Scalar;
    rowIndex    = p->grd_Index;
    indexMask   = p->grd_IndexMask;
    runningSum  = p->grd_RunningSum;
    rows        = &(p->grd_Rows[0]);
    randSeed    = p->randSeed;

    for (n=offset; n<nsmps;n++) {
      /* Increment and mask index. */
      rowIndex = (rowIndex + 1) & indexMask;

      /* If index is zero, do not update any random values. */
      if ( rowIndex != 0 ) {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int32_t numZeros = 0;
        int32_t n = rowIndex;
        while( (n & 1) == 0 ) {
          n = n >> 1;
          numZeros++;
        }

        /* Replace the indexed ROWS random value.
         * Subtract and add back to RunningSum instead of adding all
         * the random values together. Only one changes each time.
         */
        runningSum -= rows[numZeros];
        randSeed = GenerateRandomNumber(randSeed);
        newRandom = randSeed >> PINK_RANDOM_SHIFT;
        runningSum += newRandom;
        rows[numZeros] = newRandom;
      }

      /* Add extra white noise value. */
      randSeed = GenerateRandomNumber(randSeed);
      newRandom = randSeed >> PINK_RANDOM_SHIFT;
      sum = runningSum + newRandom;

      /* Scale to range of +/-p->xin (user-selected amp) */
      aout[n] = *amp * sum * scalar;
      amp += ampinc;            /* Increment if amp is a-rate */
    }

    p->grd_RunningSum = runningSum;
    p->grd_Index = rowIndex;
    p->randSeed = randSeed;
    return OK;
}

int32_t impulse_set(CSOUND *csound, IMPULSE *p)
{
    p->next = (uint32_t)MYFLT2LONG(*p->offset * CS_ESR);
    return OK;
}

int32_t impulse(CSOUND *csound, IMPULSE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t next = p->next;
    MYFLT *ar = p->ar;
    if (next<0) next = -next;
    if (UNLIKELY(next < (int32)nsmps)) { /* Impulse in this frame */
      MYFLT frq = *p->freq;     /* Freq at k-rate */
      int32_t sfreq;                /* Converted to samples */
      if (frq == FL(0.0)) sfreq = INT_MAX; /* Zero means infinite */
      else if (frq < FL(0.0)) sfreq = -(int32_t)frq; /* Negative cnts in sample */
      else sfreq = (int32_t)(frq*CS_ESR); /* Normal case */
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset;n<nsmps;n++) {
        if (UNLIKELY(next-- == 0)) {
          ar[n] = *p->amp;
          next = sfreq - 1;     /* Note can be less than k-rate */
        }
        else ar[n] = FL(0.0);
      }
    }
    else {                      /* Nothing this time so just fill */
      memset(ar, 0, nsmps*sizeof(MYFLT));
      next -= nsmps;
    }
    p->next = next;
    return OK;
}

int32_t varicolset(CSOUND *csound, VARI *p)
{
   IGN(csound);
    p->last = FL(0.0);
    p->lastbeta = *p->beta;
    p->sq1mb2 = SQRT(FL(1.0)-p->lastbeta * p->lastbeta);
    p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    p->ampinc = IS_ASIG_ARG(p->kamp) ? 1 : 0;
    return OK;
}

int32_t varicol(CSOUND *csound, VARI *p)
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       beta = *p->beta;
    MYFLT       sq1mb2 = p->sq1mb2;
    MYFLT       lastx = p->last;
    MYFLT       ampmod = p->ampmod;
    MYFLT       *kamp = p->kamp;
    int32_t         ampinc = p->ampinc;
    MYFLT       *rslt = p->rslt;

    if (beta != p->lastbeta) {
       beta = p->lastbeta = *p->beta;
       sq1mb2 = p->sq1mb2 =  SQRT(FL(1.0)-p->lastbeta * p->lastbeta);
       ampmod = p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    }

    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT rnd = FL(2.0) * (MYFLT) rand_31(csound) / FL(2147483645) - FL(1.0);
      lastx = lastx * beta + sq1mb2 * rnd;
      rslt[n] = lastx * *kamp * ampmod;
      kamp += ampinc;
    }
    p->last = lastx;
    return OK;
}

#define S       sizeof

static OENTRY noise_localops[] = {
    { "mpulse",  S(IMPULSE), 0, 3, "a", "kko",   (SUBR)impulse_set, (SUBR)impulse },
    { "pinkish", S(PINKISH), 0, 3, "a", "xoooo", (SUBR)pinkset,     (SUBR)pinkish },
    { "noise",   S(VARI),    0, 3, "a", "xk",    (SUBR)varicolset,  (SUBR)varicol },
};

LINKAGE_BUILTIN(noise_localops)
