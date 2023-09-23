/*
    vco2.h:

    Copyright (C) 2002, 2005 Istvan Varga

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

#ifndef CSOUND_VCO2_H
#define CSOUND_VCO2_H

#include "csoundCore.h"
#include "stdopcod.h"

#define OSCBNK_PHSMAX   0x80000000UL    /* max. phase   */
#define OSCBNK_PHSMSK   0x7FFFFFFFUL    /* phase mask   */
#define OSCBNK_PHS2INT(x)                                                     \
    ((uint32) MYFLT2LRND((x) * (MYFLT) OSCBNK_PHSMAX) & OSCBNK_PHSMSK)

/* ---- vco2init, vco2ft, and vco2 opcodes by Istvan Varga, Sep 2002 ---- */

/* Select algorithm to be used for finding table numbers */
/* Define this macro to use simple table lookup (slower  */
/* at high control rate, due to float->int32_t cast), or     */
/* comment it out to use a search algorithm (slower with */
/* very fast changes in frequency)                       */

#define VCO2FT_USE_TABLE    1

typedef struct {
    int32_t     npart;              /* number of harmonic partials (may be zero) */
    int32_t     size;               /* size of the table (not incl. guard point) */
    uint32               /* parameters needed for reading the table,  */
            lobits, mask;       /*   and interpolation                       */
    MYFLT   pfrac;
    MYFLT   *ftable;            /* table data (size + 1 floats)              */
} VCO2_TABLE;

struct VCO2_TABLE_ARRAY_ {
    int32_t     ntabl;              /* number of tables                          */
    int32_t     base_ftnum;         /* base ftable number (-1: none)             */
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl;  /* table ptrs for all numbers of partials    */
#else
    MYFLT   *nparts;            /* number of partials list                   */
#endif
    VCO2_TABLE  *tables;        /* array of table structures                 */
};

typedef struct {
    OPDS    h;
    MYFLT   *ift, *iwaveforms, *iftnum, *ipmul, *iminsiz, *imaxsiz, *isrcft;
} VCO2INIT;

typedef struct {
    OPDS    h;
    MYFLT   *ar, *kamp, *kcps, *imode, *kpw, *kphs, *inyx;
    MYFLT   *dummy[9];
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl;  /* table ptrs for all numbers of partials    */
#endif
    int32_t     init_k;             /* 1 in first k-cycle, 0 otherwise           */
    int32_t     mode;               /* algorithm (0, 1, or 2)                    */
    int32_t     pm_enabled;         /* phase modulation enabled (0: no, 1: yes)  */
#ifdef VCO2FT_USE_TABLE
    MYFLT   f_scl, p_min, p_scl, kphs_old, kphs2_old;
#else
    MYFLT   f_scl, p_min, p_scl, *npart_old, *nparts, kphs_old, kphs2_old;
    VCO2_TABLE  *tables;        /* pointer to array of tables                */
#endif
    uint32  phs, phs2;  /* oscillator phase                          */
    VCO2_TABLE_ARRAY  ***vco2_tables;
    int32_t             *vco2_nr_table_arrays;
} VCO2;

typedef struct {
    OPDS    h;
    MYFLT   *kft, *kcps, *iwave, *inyx;
    MYFLT   p_min, p_scl;
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl, *tab0;
#else
    MYFLT   *npart_old, *nparts;
#endif
    VCO2_TABLE_ARRAY    ***vco2_tables;
    int32_t                 *vco2_nr_table_arrays;
    int32_t                 base_ftnum;
} VCO2FT;

typedef struct {                /* denorm a1[, a2[, a3[, ... ]]] */
    OPDS    h;
    MYFLT   *ar[256];
    int32_t     *seedptr;
} DENORMS;

typedef struct {                /* kr delayk ksig, idel[, imode] */
    OPDS    h;
    MYFLT   *ar, *ksig, *idel, *imode;
    int32_t     npts, init_k, readp, mode;
    AUXCH   aux;
} DELAYK;

typedef struct {                /* kr vdel_k ksig, kdel, imdel[, imode] */
    OPDS    h;
    MYFLT   *ar, *ksig, *kdel, *imdel, *imode;
    int32_t     npts, init_k, wrtp, mode;
    MYFLT   frstkval;
    AUXCH   aux;
} VDELAYK;

typedef struct {                /* ar rbjeq asig, kfco, klvl, kQ, kS[, imode] */
        OPDS    h;
        MYFLT   *ar, *asig, *kcps, *klvl, *kQ, *kS, *imode;     /* args */
        /* internal variables */
        MYFLT   old_kcps, old_klvl, old_kQ, old_kS;
        double  omega, cs, sn;
        MYFLT   xnm1, xnm2, ynm1, ynm2;
        MYFLT   b0, b1, b2, a1, a2;
        int32_t
        ftype;
} RBJEQ;

#endif          /* CSOUND_VCO2_H */
