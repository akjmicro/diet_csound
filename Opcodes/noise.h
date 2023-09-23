#ifndef NOISE_H
#define NOISE_H
/*
    noise.h:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neubäcker,
                       rasmus ekman, Phil Burk

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

                        /*                                      NOISE.H */
#include "uggab.h"

#define GRD_MAX_RANDOM_ROWS   (32)

typedef struct {
    OPDS        h;
    MYFLT       *aout;
    MYFLT       *xin, *imethod, *iparam1, *iseed, *iskip;
    int32       ampinc;         /* Scale output to range */
    uint32      randSeed;     /* Used by local random generator */
                                /* for Paul Kellet's filter bank */
    double      b0, b1, b2, b3, b4, b5, b6;
                                /* for Gardner method */
    int32       grd_Rows[GRD_MAX_RANDOM_ROWS];
    int32       grd_NumRows;    /* Number of rows (octave bands of noise) */
    int32       grd_RunningSum; /* Used to optimize summing of generators. */
    int32_t         grd_Index;      /* Incremented each sample. */
    int32_t         grd_IndexMask;  /* Index wrapped by ANDing with this mask. */
    MYFLT       grd_Scalar;     /* Used to scale to normalize generated noise. */
} PINKISH;

typedef struct {
        OPDS    h;
        MYFLT   *ar;
        MYFLT   *amp, *freq, *offset;
        uint32_t     next;
} IMPULSE;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *kamp, *beta;
        MYFLT   last, lastbeta, sq1mb2, ampmod;
        int32_t     ampinc;
} VARI;

#endif
