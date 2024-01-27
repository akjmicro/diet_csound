/*
    vco2.c:

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

#include "vco2.h"
#include <math.h>

static inline STDOPCOD_GLOBALS *get_oscbnk_globals(CSOUND *csound)
{
    return ((STDOPCOD_GLOBALS*) csound->stdOp_Env);
}

static void oscbnk_flen_setup(int32 flen, uint32 *mask,
                              uint32 *lobits, MYFLT *pfrac)
{
    uint32       n;

    n = (uint32) flen;
    *lobits = 0UL; *mask = 1UL; *pfrac = FL(0.0);
    if (n < 2UL) return;
    while (n < OSCBNK_PHSMAX) {
      n <<= 1; *mask <<= 1; (*lobits)++;
    }
    *pfrac = FL(1.0) / (MYFLT) *mask; (*mask)--;
}

/* ---- vco2init, vco2ft, and vco2 opcodes by Istvan Varga, Sep 2002 ---- */

/* table arrays for vco2 opcode */
/*   0: sawtooth                */
/*   1: 4 * x * (1 - x)         */
/*   2: pulse (not normalised)  */
/*   3: square                  */
/*   4: triangle                */
/*   5 and above: user defined  */

#define VCO2_MAX_NPART  4096    /* maximum number of harmonic partials */

typedef struct {
    int32_t     waveform;           /* waveform number (< 0: user defined)       */
    int32_t     w_npart;            /* nr of partials in user specified waveform */
    double  npart_mul;          /* multiplier for number of partials         */
    int32_t     min_size, max_size; /* minimum and maximum table size            */
    MYFLT   *w_fftbuf;          /* FFT of user specified waveform            */
} VCO2_TABLE_PARAMS;

/* remove table array for the specified waveform */

static void vco2_delete_table_array(CSOUND *csound, int32_t w)
{
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    int32_t               j;

    /* table array does not exist: nothing to do */
    if (pp->vco2_tables == (VCO2_TABLE_ARRAY**) NULL ||
        w >= pp->vco2_nr_table_arrays ||
        pp->vco2_tables[w] == (VCO2_TABLE_ARRAY*) NULL)
      return;
#ifdef VCO2FT_USE_TABLE
    /* free number of partials -> table list, */
    csound->Free(csound, pp->vco2_tables[w]->nparts_tabl);
#else
    /* free number of partials list, */
    csound->Free(csound, pp->vco2_tables[w]->nparts);
#endif
    /* table data (only if not shared as standard Csound ftables), */
    for (j = 0; j < pp->vco2_tables[w]->ntabl; j++) {
      if (pp->vco2_tables[w]->base_ftnum < 1)
        csound->Free(csound, pp->vco2_tables[w]->tables[j].ftable);
    }
    /* table list, */
    csound->Free(csound, pp->vco2_tables[w]->tables);
    /* and table array structure */
    csound->Free(csound, pp->vco2_tables[w]);
    pp->vco2_tables[w] = NULL;
}

/* generate a table using the waveform specified in tp */

static void vco2_calculate_table(CSOUND *csound,
                                 VCO2_TABLE *table, VCO2_TABLE_PARAMS *tp)
{
    MYFLT   scaleFac;
    MYFLT   *fftbuf;
    int32_t     i, minh;

    if (UNLIKELY(table->ftable == NULL)) {
      csound->InitError(csound, "%s",
                        Str("function table is NULL, check that ibasfn is "
                            "available\n"));
      return;
    }

    /* allocate memory for FFT */
    fftbuf = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (table->size + 2));
    if (tp->waveform >= 0) {                        /* no DC offset for   */
      minh = 1; fftbuf[0] = fftbuf[1] = FL(0.0);    /* built-in waveforms */
    }
    else
      minh = 0;
    scaleFac = csound->GetInverseRealFFTScale(csound, (int32_t) table->size);
    scaleFac *= (FL(0.5) * (MYFLT) table->size);
    switch (tp->waveform) {
      case 0: scaleFac *= (FL(-2.0) / PI_F);          break;
      case 1: scaleFac *= (FL(-4.0) / (PI_F * PI_F)); break;
      case 3: scaleFac *= (FL(-4.0) / PI_F);          break;
      case 4: scaleFac *= (FL(8.0) / (PI_F * PI_F));  break;
    }
    /* calculate FFT of the requested waveform */
    for (i = minh; i <= (table->size >> 1); i++) {
      fftbuf[i << 1] = fftbuf[(i << 1) + 1] = FL(0.0);
      if (i > table->npart) continue;
      switch (tp->waveform) {
      case 0:                                   /* sawtooth */
        fftbuf[(i << 1) + 1] = scaleFac / (MYFLT) i;
        break;
      case 1:                                   /* 4 * x * (1 - x) */
        fftbuf[i << 1] = scaleFac / ((MYFLT) i * (MYFLT) i);
        break;
      case 2:                                   /* pulse */
        fftbuf[i << 1] = scaleFac;
        break;
      case 3:                                   /* square */
        fftbuf[(i << 1) + 1] = (i & 1 ? (scaleFac / (MYFLT) i) : FL(0.0));
        break;
      case 4:                                   /* triangle */
        fftbuf[(i << 1) + 1] = (i & 1 ? ((i & 2 ? scaleFac : (-scaleFac))
                                         / ((MYFLT) i * (MYFLT) i))
                                        : FL(0.0));
        break;
      default:                                  /* user defined */
        if (i <= tp->w_npart) {
          fftbuf[i << 1] = scaleFac * tp->w_fftbuf[i << 1];
          fftbuf[(i << 1) + 1] = scaleFac * tp->w_fftbuf[(i << 1) + 1];
        }
      }
    }
    /* inverse FFT */
    fftbuf[1] = fftbuf[table->size];
    fftbuf[table->size] = fftbuf[(int32_t) table->size + 1] = FL(0.0);
    csound->InverseRealFFT(csound, fftbuf, (int32_t) table->size);
    /* copy to table */
    for (i = 0; i < table->size; i++)
      table->ftable[i] = fftbuf[i];
    /* write guard point */
    table->ftable[table->size] = fftbuf[0];
    /* free memory used by temporary buffers */
    csound->Free(csound, fftbuf);
}

/* set default table parameters depending on waveform */

static void vco2_default_table_params(int32_t w, VCO2_TABLE_PARAMS *tp)
{
    tp->waveform = w;
    tp->w_npart = -1;
    tp->npart_mul = 1.05;
    tp->min_size = (w == 2 ? 256 : 128);
    tp->max_size = (w == 2 ? 16384 : 8192);
    tp->w_fftbuf = NULL;
}

/* return number of partials for next table */

static void vco2_next_npart(double *npart, VCO2_TABLE_PARAMS *tp)
{
    double  n;
    n = *npart * tp->npart_mul;
    if ((n - *npart) < 1.0)
      (*npart)++;
    else
      *npart = n;
}

/* return optimal table size for a given number of partials */

static int32_t vco2_table_size(int32_t npart, VCO2_TABLE_PARAMS *tp)
{
    int32_t     n;

    if (npart < 1)
      return 16;        /* empty table, size is always 16 */
    else if (npart == 1)
      n = 1;
    else if (npart <= 4)
      n = 2;
    else if (npart <= 16)
      n = 4;
    else if (npart <= 64)
      n = 8;
    else if (npart <= 256)
      n = 16;
    else if (npart <= 1024)
      n = 32;
    else
      n = 64;
    /* set table size according to min and max value */
    n *= tp->min_size;
    if (n > tp->max_size) n = tp->max_size;

    return n;
}

/* Generate table array for the specified waveform (< 0: user defined).  */
/* The tables can be accessed also as standard Csound ftables, starting  */
/* from table number "base_ftable" if it is greater than zero.           */
/* The return value is the first ftable number that is not allocated.    */

static int32_t vco2_tables_create(CSOUND *csound, int32_t waveform,
                                  int32_t base_ftable,
                                  VCO2_TABLE_PARAMS *tp)
{
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    int32_t               i, npart, ntables;
    double            npart_f;
    VCO2_TABLE_ARRAY  *tables;
    VCO2_TABLE_PARAMS tp2;

    /* set default table parameters if not specified in tp */
    if (tp == NULL) {
      if (waveform < 0) return -1;
      vco2_default_table_params(waveform, &tp2);
      tp = &tp2;
    }
    waveform = (waveform < 0 ? 4 - waveform : waveform);
    if (waveform >= pp->vco2_nr_table_arrays) {
      /* extend space for table arrays */
      ntables = ((waveform >> 4) + 1) << 4;
      pp->vco2_tables = (VCO2_TABLE_ARRAY**)
        csound->ReAlloc(csound, pp->vco2_tables, sizeof(VCO2_TABLE_ARRAY*)
                                                 * ntables);
      for (i = pp->vco2_nr_table_arrays; i < ntables; i++)
        pp->vco2_tables[i] = NULL;
      pp->vco2_nr_table_arrays = ntables;
    }
    /* clear table array if already initialised */
    if (pp->vco2_tables[waveform] != NULL) {
      vco2_delete_table_array(csound, waveform);
      csound->Warning(csound,
                      Str("redefined table array for waveform %d\n"),
                      (waveform > 4 ? 4 - waveform : waveform));
    }
    /* calculate number of tables */
    i = tp->max_size >> 1;
    if (i > VCO2_MAX_NPART) i = VCO2_MAX_NPART; /* max number of partials */
    npart_f = 0.0; ntables = 0;
    do {
      ntables++;
      vco2_next_npart(&npart_f, tp);
    } while (npart_f <= (double) i);
    /* allocate memory for the table array ... */
    tables = pp->vco2_tables[waveform] =
      (VCO2_TABLE_ARRAY*) csound->Calloc(csound, sizeof(VCO2_TABLE_ARRAY));
    /* ... and all tables */
#ifdef VCO2FT_USE_TABLE
    tables->nparts_tabl =
      (VCO2_TABLE**) csound->Malloc(csound, sizeof(VCO2_TABLE*)
                                            * (VCO2_MAX_NPART + 1));
#else
    tables->nparts =
        (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (ntables * 3));
    for (i = 0; i < ntables; i++) {
      tables->nparts[i] = FL(-1.0);     /* padding for number of partials */
      tables->nparts[(ntables << 1) + i] = FL(1.0e24);  /* list */
    }
#endif
    tables->tables =
        (VCO2_TABLE*) csound->Calloc(csound, sizeof(VCO2_TABLE) * ntables);
    /* generate tables */
    tables->ntabl = ntables;            /* store number of tables */
    tables->base_ftnum = base_ftable;   /* and base ftable number */
    npart_f = 0.0; i = 0;
    do {
      /* store number of partials, */
      npart = tables->tables[i].npart = (int32_t) (npart_f + 0.5);
#ifndef VCO2FT_USE_TABLE
      tables->nparts[ntables + i] = (MYFLT) npart;
#endif
      /* table size, */
      tables->tables[i].size = vco2_table_size(npart, tp);
      /* and other parameters */
      oscbnk_flen_setup((int32) tables->tables[i].size,
                        &(tables->tables[i].mask),
                        &(tables->tables[i].lobits),
                        &(tables->tables[i].pfrac));
      /* if base ftable was specified, generate empty table ... */
      if (base_ftable > 0) {
        csound->FTAlloc(csound, base_ftable, (int32_t) tables->tables[i].size);
        csoundGetTable(csound, &(tables->tables[i].ftable), base_ftable);
        base_ftable++;                /* next table number */
      }
      else    /* ... else allocate memory (cannot be accessed as a       */
        tables->tables[i].ftable =      /* standard Csound ftable) */
          (MYFLT*) csound->Malloc(csound, sizeof(MYFLT)
                                          * (tables->tables[i].size + 1));
      /* now calculate the table */
      vco2_calculate_table(csound, &(tables->tables[i]), tp);
      /* next table */
      vco2_next_npart(&npart_f, tp);
    } while (++i < ntables);
#ifdef VCO2FT_USE_TABLE
    /* build table for number of harmonic partials -> table lookup */
    i = npart = 0;
    do {
      tables->nparts_tabl[npart++] = &(tables->tables[i]);
      if (i < (ntables - 1) && npart >= tables->tables[i + 1].npart) i++;
    } while (npart <= VCO2_MAX_NPART);
#endif

    return base_ftable;
}

/* ---- vco2init opcode ---- */

static int32_t vco2init(CSOUND *csound, VCO2INIT *p)
{
    int32_t     waveforms, base_ftable, ftnum, i, w;
    VCO2_TABLE_PARAMS   tp;
    FUNC    *ftp;
    uint32_t j;
    /* check waveform number */
    waveforms = (int32_t) MYFLT2LRND(*(p->iwaveforms));
    if (UNLIKELY(waveforms < -1000000 || waveforms > 31)) {
      return csound->InitError(csound,
                               Str("vco2init: invalid waveform number: %f"),
                               *(p->iwaveforms));
    }
    /* base ftable number (required by user defined waveforms except -1) */
    ftnum = base_ftable = (int32_t) MYFLT2LONG(*(p->iftnum));
    if (ftnum < 1) ftnum = base_ftable = -1;
    if (UNLIKELY((waveforms < -1 && ftnum < 1) || ftnum > 1000000)) {
      return csound->InitError(csound,
                               Str("vco2init: invalid base ftable number"));
    }
    *(p->ift) = (MYFLT) ftnum;
    if (!waveforms) return OK;     /* nothing to do */
    w = (waveforms < 0 ? waveforms : 0);
    do {
    /* set default table parameters, */
      vco2_default_table_params(w, &tp);
    /* and override with user specified values (if there are any) */
    if (*(p->ipmul) > FL(0.0)) {
      if (UNLIKELY(*(p->ipmul) < FL(1.00999) || *(p->ipmul) > FL(2.00001))) {
        return csound->InitError(csound, Str("vco2init: invalid "
                                             "partial number multiplier"));
      }
      tp.npart_mul = (double) *(p->ipmul);
    }
    if (*(p->iminsiz) > FL(0.0)) {
      i = (int32_t) MYFLT2LONG(*(p->iminsiz));
      if (UNLIKELY(i < 16 || i > 262144 || (i & (i - 1)))) {
        return csound->InitError(csound,
                                 Str("vco2init: invalid min table size"));
      }
      tp.min_size = i;
    }
    if (*(p->imaxsiz) > FL(0.0)) {
      i = (int32_t) MYFLT2LONG(*(p->imaxsiz));
      if (UNLIKELY(i < 16 || i > 16777216 || (i & (i - 1)) || i < tp.min_size)) {
        return csound->InitError(csound,
                                 Str("vco2init: invalid max table size"));
      }
      tp.max_size = i;
    }
    else {
      tp.max_size = tp.min_size << 6;           /* default max size */
      if (tp.max_size > 16384) tp.max_size = 16384;
      if (tp.max_size < tp.min_size) tp.max_size = tp.min_size;
    }

      if (w >= 0) {             /* built-in waveforms */
        if (waveforms & (1 << w)) {
          ftnum = vco2_tables_create(csound, w, ftnum, &tp);
          if (UNLIKELY(base_ftable > 0 && ftnum <= 0)) {
            return csound->InitError(csound, Str("ftgen error"));
          }
        }
      }
      else {                      /* user defined, requires source ftable */
        if (UNLIKELY((ftp = csound->FTFind(csound, p->isrcft)) == NULL ||
                     ftp->flen < 4)) {
          return csound->InitError(csound,
                                   Str("vco2init: invalid source ftable"));
        }
        /* analyze source table, and store results in table params structure */
        i = ftp->flen;
        tp.w_npart = i >> 1;
        tp.w_fftbuf = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (i + 2));
        for (j = 0; j < ftp->flen; j++)
          tp.w_fftbuf[j] = ftp->ftable[j] / (MYFLT) (ftp->flen >> 1);
        csound->RealFFT(csound, tp.w_fftbuf, (int32_t) ftp->flen);
        tp.w_fftbuf[ftp->flen] = tp.w_fftbuf[1];
        tp.w_fftbuf[1] = tp.w_fftbuf[(int32_t) ftp->flen + 1] = FL(0.0);
        /* generate table array */
        ftnum = vco2_tables_create(csound,waveforms, ftnum, &tp);
        /* free memory used by FFT buffer */
        csound->Free(csound, tp.w_fftbuf);
        if (UNLIKELY(base_ftable > 0 && ftnum <= 0)) {
          return csound->InitError(csound, Str("ftgen error"));
        }
      }
      *(p->ift) = (MYFLT) ftnum;
      w++;
    } while (w > 0 && w < 5);
    return OK;
}

/* ---- vco2ft / vco2ift opcode (initialisation) ---- */

static int32_t vco2ftp(CSOUND *, VCO2FT *);

static int32_t vco2ftset(CSOUND *csound, VCO2FT *p)
{
    int32_t     w;

    if (p->vco2_nr_table_arrays == NULL || p->vco2_tables == NULL) {
      STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
      p->vco2_nr_table_arrays = &(pp->vco2_nr_table_arrays);
      p->vco2_tables = &(pp->vco2_tables);
    }
    w = (int32_t) MYFLT2LRND(*(p->iwave));
    if (w > 4) w = 0x7FFFFFFF;
    if (w < 0) w = 4 - w;
    if (UNLIKELY(w >= *(p->vco2_nr_table_arrays) || (*(p->vco2_tables))[w] == NULL
                 || (*(p->vco2_tables))[w]->base_ftnum < 1)) {
      return csound->InitError(csound, Str("vco2ft: table array "
                                           "not found for this waveform"));
    }
#ifdef VCO2FT_USE_TABLE
    p->nparts_tabl = (*(p->vco2_tables))[w]->nparts_tabl;
    p->tab0 = (*(p->vco2_tables))[w]->tables;
#else
    /* address of number of partials list (with offset for padding) */
    p->nparts = (*(p->vco2_tables))[w]->nparts
                + (*(p->vco2_tables))[w]->ntabl;
    p->npart_old = p->nparts + ((*(p->vco2_tables))[w]->ntabl >> 1);
#endif
    p->base_ftnum = (*(p->vco2_tables))[w]->base_ftnum;
    if (*(p->inyx) > FL(0.5))
      p->p_scl = FL(0.5) * CS_ESR;
    else if (*(p->inyx) < FL(0.001))
      p->p_scl = FL(0.001) * CS_ESR;
    else
      p->p_scl = *(p->inyx) * CS_ESR;
    p->p_min = p->p_scl / (MYFLT) VCO2_MAX_NPART;
    /* in case of vco2ift opcode, find table number now */
    if (!strcmp(p->h.optext->t.opcod, "vco2ift"))
      vco2ftp(csound, p);
    else                                /* else set perf routine to avoid */
      p->h.opadr = (SUBR) vco2ftp;      /* "not initialised" error */
    return OK;
}

/* ---- vco2ft opcode (performance) ---- */

static int32_t vco2ftp(CSOUND *csound, VCO2FT *p)
{
    IGN(csound);
#ifdef VCO2FT_USE_TABLE
    MYFLT   npart;
    int32_t     n;
#else
    MYFLT   npart, *nparts;
    int32_t     nn;
#endif

    npart = (MYFLT)fabs(*(p->kcps)); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
    n = (int32_t) (p->nparts_tabl[(int32_t) (p->p_scl / npart)] - p->tab0);
    *(p->kft) = (MYFLT) (n + p->base_ftnum);
#else
    npart = p->p_scl / npart;
    nparts = p->npart_old;
    if (npart < *nparts) {
      do {
        nparts--; nn = 1;
        while (npart < *(nparts - nn)) {
          nparts = nparts - nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    else if (npart >= *(nparts + 1)) {
      do {
        nparts++; nn = 1;
        while (npart >= *(nparts + nn + 1)) {
          nparts = nparts + nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    p->npart_old = nparts;
    *(p->kft) = (MYFLT) ((int32_t) (nparts - p->nparts) + p->base_ftnum);
#endif
    return OK;
}

static int32_t vco2ft(CSOUND *csound, VCO2FT *p)
{
    return csound->PerfError(csound, &(p->h),
                             Str("vco2ft: not initialised"));
}

/* ---- vco2 opcode (initialisation) ---- */

static int32_t vco2set(CSOUND *csound, VCO2 *p)
{
    int32_t     mode, tnum;
    int32_t     tnums[8] = { 0, 0, 1, 2, 1, 3, 4, 5 };
    int32_t     modes[8] = { 0, 1, 2, 0, 0, 0, 0, 0 };
    MYFLT   x;
    uint32_t min_args;

    if (p->vco2_nr_table_arrays == NULL || p->vco2_tables == NULL) {
      STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
      p->vco2_nr_table_arrays = &(pp->vco2_nr_table_arrays);
      p->vco2_tables = &(pp->vco2_tables);
    }
    /* check number of args */
    if (UNLIKELY(p->INOCOUNT > 6)) {
      return csound->InitError(csound, Str("vco2: too many input arguments"));
    }
    mode = (int32_t) MYFLT2LONG(*(p->imode)) & 0x1F;
    if (mode & 1) return OK;               /* skip initialisation */
    /* more checks */
    min_args = 2;
    if ((mode & 14) == 2 || (mode & 14) == 4) min_args = 4;
    if (mode & 16) min_args = 5;
    if (UNLIKELY(p->INOCOUNT < min_args)) {
      return csound->InitError(csound,
                               Str("vco2: insufficient required arguments"));
    }

    //FIXME

//    if (UNLIKELY(p->XINCODE)) {
//      return csound->InitError(csound, Str("vco2: invalid argument type"));
//    }

    /* select table array and algorithm, according to waveform */
    tnum = tnums[(mode & 14) >> 1];
    p->mode = modes[(mode & 14) >> 1];
    /* initialise tables if not done yet */
    if (tnum >= *(p->vco2_nr_table_arrays) ||
        (*(p->vco2_tables))[tnum] == NULL) {
      if (LIKELY(tnum < 5))
        vco2_tables_create(csound, tnum, -1, NULL);
      else {
        return csound->InitError(csound, Str("vco2: table array not found for "
                                             "user defined waveform"));
      }
    }
#ifdef VCO2FT_USE_TABLE
    p->nparts_tabl = (*(p->vco2_tables))[tnum]->nparts_tabl;
#else
    /* address of number of partials list (with offset for padding) */
    p->nparts = (*(p->vco2_tables))[tnum]->nparts
                + (*(p->vco2_tables))[tnum]->ntabl;
    p->npart_old = p->nparts + ((*(p->vco2_tables))[tnum]->ntabl >> 1);
    p->tables = (*(p->vco2_tables))[tnum]->tables;
#endif
    /* set misc. parameters */
    p->init_k = 1;
    p->pm_enabled = (mode & 16 ? 1 : 0);
    if ((mode & 16) || (p->INOCOUNT < 5))
      p->phs = 0UL;
    else {
      x = *(p->kphs); x -= (MYFLT) ((int32) x);
      p->phs = OSCBNK_PHS2INT(x);
    }
    p->f_scl = csound->onedsr;
    x = (p->INOCOUNT < 6 ? FL(0.5) : *(p->inyx));
    if (x < FL(0.001)) x = FL(0.001);
    if (x > FL(0.5)) x = FL(0.5);
    p->p_min = x / (MYFLT) VCO2_MAX_NPART;
    p->p_scl = x;
    return OK;
}

/* ---- vco2 opcode (performance) ---- */

static int32_t vco2(CSOUND *csound, VCO2 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t      n;
    VCO2_TABLE      *tabl;
    uint32  phs, phs2, frq, frq2, lobits, mask;
#ifdef VCO2FT_USE_TABLE
    MYFLT   f, f1, npart, pfrac, v, *ftable, kamp, *ar;
    if (UNLIKELY(p->nparts_tabl == NULL)) {
#else
    MYFLT   f, f1, npart, *nparts, pfrac, v, *ftable, kamp, *ar;
    if (UNLIKELY(p->tables == NULL)) {
#endif
      return csound->PerfError(csound, &(p->h),
                               Str("vco2: not initialised"));
    }
    /* if 1st k-cycle, initialise now */
    if (p->init_k) {
      p->init_k = 0;
      if (p->pm_enabled) {
        f = p->kphs_old = *(p->kphs); f -= (MYFLT) ((int32) f);
        p->phs = OSCBNK_PHS2INT(f);
      }
      if (p->mode) {
        p->kphs2_old = -(*(p->kpw));
        f = p->kphs2_old; f -= (MYFLT) ((int32) f);
        p->phs2 = (p->phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
      }
    }
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* calculate frequency (including phase modulation) */
    f = *(p->kcps) * p->f_scl;
    frq = OSCBNK_PHS2INT(f);
    if (p->pm_enabled) {
      f1 = (MYFLT) ((double) *(p->kphs) - (double) p->kphs_old)
        / (nsmps-offset);
      p->kphs_old = *(p->kphs);
      frq = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      f += f1;
    }
    /* find best table for current frequency */
    npart = (MYFLT)fabs(f); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
    tabl = p->nparts_tabl[(int32_t) (p->p_scl / npart)];
#else
    npart = p->p_scl / npart;
    nparts = p->npart_old;
    if (npart < *nparts) {
      do {
        nparts--; nn = 1;
        while (npart < *(nparts - nn)) {
          nparts = nparts - nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    else if (npart >= *(nparts + 1)) {
      do {
        nparts++; nn = 1;
        while (npart >= *(nparts + nn + 1)) {
          nparts = nparts + nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    p->npart_old = nparts;
    tabl = p->tables + (int32_t) (nparts - p->nparts);
#endif
    /* copy object data to local variables */
    kamp = *(p->kamp);
    phs = p->phs;
    lobits = tabl->lobits; mask = tabl->mask; pfrac = tabl->pfrac;
    ftable = tabl->ftable;

    if (!p->mode) {                     /* - mode 0: simple table playback - */
      for (nn=offset; nn<nsmps; nn++) {
        n = phs >> lobits;
        v = ftable[n++];
        v += (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
        phs = (phs + frq) & OSCBNK_PHSMSK;
        ar[nn] = v * kamp;
      }
    }
    else {
      v = -(*(p->kpw));                                 /* pulse width */
      f1 = (MYFLT) ((double) v - (double) p->kphs2_old) / (nsmps-offset);
      f = p->kphs2_old; f -= (MYFLT) ((int32) f); if (f < FL(0.0)) f++;
      p->kphs2_old = v;
      phs2 = p->phs2;
      frq2 = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      if (p->mode == 1) {               /* - mode 1: PWM - */
        /* DC correction offset */
        f = FL(1.0) - FL(2.0) * f;
        f1 *= FL(-2.0);
        for (nn=offset; nn<nsmps; nn++) {
          n = phs >> lobits;
          v = ftable[n++];
          ar[nn] = v + (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((int32) (phs2 & mask)) * pfrac;
          ar[nn] = (ar[nn] - v + f) * kamp;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        }
      }
      else {                            /* - mode 2: saw / triangle ramp - */
        for (nn=offset; nn<nsmps; nn++) {
          n = phs >> lobits;
          v = ftable[n++];
          ar[nn] = v + (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((int32) (phs2 & mask)) * pfrac;
          ar[nn] = (ar[nn] - v) * (FL(0.25) / (f - f * f)) * kamp;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        }
      }
      p->phs2 = phs2;
    }
    /* save oscillator phase */
    p->phs = phs;
    return OK;
}

/* ---- denorm opcode ---- */

#ifndef USE_DOUBLE
#define DENORM_RND  ((MYFLT) ((*seed = (*seed * 15625 + 1) & 0xFFFF) - 0x8000) \
                             * FL(1.0e-24))
#else
#define DENORM_RND  ((MYFLT) ((*seed = (*seed * 15625 + 1) & 0xFFFF) - 0x8000) \
                             * FL(1.0e-60))
#endif

static int32_t denorms(CSOUND *csound, DENORMS *p)
{
    MYFLT   r, *ar, **args = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t     n = p->INOCOUNT, *seed;

    seed = p->seedptr;
    if (seed == NULL) {
      STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
      seed = p->seedptr = &(pp->denorm_seed);
    }
    if (UNLIKELY(early)) nsmps -= early;
    do {
      r = DENORM_RND;
      ar = *args++;
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      for (nn=offset; nn<nsmps; nn++) {
        ar[nn] += r;
      }
    } while (--n);
    return OK;
}

/* ---- delayk and vdel_k opcodes ---- */

static int32_t delaykset(CSOUND *csound, DELAYK *p)
{
    int32_t npts, mode = (int32_t) MYFLT2LONG(*p->imode) & 3;

    if (mode & 1) return OK;            /* skip initialisation */
    p->mode = mode;
    /* calculate delay time */
    npts = (int32_t) (*p->idel * CS_EKR + FL(1.5));
    if (UNLIKELY(npts < 1))
      return csound->InitError(csound, Str("delayk: invalid delay time "
                                           "(must be >= 0)"));
    p->readp = 0; p->npts = npts;
    /* allocate space for delay buffer */
    if (p->aux.auxp == NULL ||
        (uint32_t)(npts * sizeof(MYFLT)) > p->aux.size) {
      csound->AuxAlloc(csound, (int32) (npts * sizeof(MYFLT)), &p->aux);
    }
    p->init_k = npts - 1;
    return OK;
}

static int32_t delayk(CSOUND *csound, DELAYK *p)
{
    MYFLT   *buf = (MYFLT*) p->aux.auxp;

    if (UNLIKELY(!buf))
      return csound->PerfError(csound, &(p->h),
                               Str("delayk: not initialised"));
    buf[p->readp++] = *(p->ksig);           /* write input signal to buffer */
    if (p->readp >= p->npts)
      p->readp = 0;                         /* wrap index */
    if (p->init_k) {
      *(p->ar) = (p->mode & 2 ? *(p->ksig) : FL(0.0));  /* initial delay */
      p->init_k--;
    }
    else
      *(p->ar) = buf[p->readp];             /* read output signal */
    return OK;
}

static int32_t vdelaykset(CSOUND *csound, VDELAYK *p)
{
    int32_t npts, mode = (int32_t) MYFLT2LONG(*p->imode) & 3;

    if (mode & 1)
      return OK;                /* skip initialisation */
    p->mode = mode;
    /* calculate max. delay time */
    npts = (int32_t) (*p->imdel * CS_EKR + FL(1.5));
    if (UNLIKELY(npts < 1))
      return csound->InitError(csound, Str("vdel_k: invalid max delay time "
                                           "(must be >= 0)"));
    p->wrtp = 0; p->npts = npts;
    /* allocate space for delay buffer */
    if (p->aux.auxp == NULL ||
        (uint32_t)(npts * sizeof(MYFLT)) > p->aux.size) {
      csound->AuxAlloc(csound, (int32) (npts * sizeof(MYFLT)), &p->aux);
    }
    p->init_k = npts;           /* not -1 this time ! */
    return OK;
}

static int32_t vdelayk(CSOUND *csound, VDELAYK *p)
{
    MYFLT   *buf = (MYFLT*) p->aux.auxp;
    int32_t     n, npts = p->npts;

    if (UNLIKELY(!buf))
      return csound->PerfError(csound, &(p->h),
                               Str("vdel_k: not initialised"));
    buf[p->wrtp] = *(p->ksig);              /* write input signal to buffer */
                                            /* calculate delay time */
    n = (int32_t) MYFLT2LONG(*(p->kdel) * CS_EKR);
    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, &(p->h),
                               Str("vdel_k: invalid delay time "
                                           "(must be >= 0)"));
    n = p->wrtp - n;
    if (++p->wrtp >= npts) p->wrtp = 0;         /* wrap index */
    if (p->init_k) {
      if (p->mode & 2) {
        if (npts == p->init_k)
          p->frstkval = *(p->ksig);             /* save first input value */
        *(p->ar) = (n < 0 ? p->frstkval : buf[n]);      /* initial delay */
      }
      else {
        *(p->ar) = (n < 0 ? FL(0.0) : buf[n]);
      }
      p->init_k--;
    }
    else {
      while (n < 0) n += npts;
      *(p->ar) = buf[n];                        /* read output signal */
    }
    return OK;
}

/* ------------ rbjeq opcode ------------ */

/* original algorithm by Robert Bristow-Johnson */
/* Csound orchestra version by Josep M Comajuncosas, Aug 1999 */
/* ported to C (and optimised) by Istvan Varga, Dec 2002 */

/* ar rbjeq asig, kfco, klvl, kQ, kS[, imode] */

/* IV - Dec 28 2002: according to the original version by JMC, the formula */
/*   alpha = sin(omega) * sinh(1 / (2 * Q))                                */
/* should be used to calculate Q. However, according to my tests, it seems */
/* to be wrong with low Q values, where this simplified code               */
/*   alpha = sin(omega) / (2 * Q)                                          */
/* was measured to be more accurate. It also makes the Q value for no      */
/* resonance exactly sqrt(0.5) (as it would be expected), while the old    */
/* version required a Q setting of about 0.7593 for no resonance.          */
/* With Q >= 1, there is not much difference.                              */
/* N.B.: the above apply to the lowpass and highpass filters only. For     */
/* bandpass, band-reject, and peaking EQ, the modified formula is          */
/*   alpha = tan(omega / (2 * Q))                                          */

/* Defining this macro selects the revised version, while commenting it    */
/* out enables the original.                                               */

/* #undef IV_Q_CALC */
#define IV_Q_CALC 1

static int32_t rbjeqset(CSOUND *csound, RBJEQ *p)
{
     IGN(csound);
    int32_t mode = (int32_t) MYFLT2LONG(*p->imode) & 0xF;

    if (mode & 1)
      return OK;                /* skip initialisation */
    /* filter type */
    p->ftype = mode >> 1;
    /* reset filter */
    p->old_kcps = p->old_klvl = p->old_kQ = p->old_kS = FL(-1.12123e35);
    p->b0 = p->b1 = p->b2 = p->a1 = p->a2 = FL(0.0);
    p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = FL(0.0);
    return OK;
}

static int32_t rbjeq(CSOUND *csound, RBJEQ *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     new_frq;
    MYFLT   b0, b1, b2, a1, a2, tmp;
    MYFLT   xnm1, xnm2, ynm1, ynm2;
    MYFLT   *ar, *asig;
    double  dva0;

    if (*(p->kcps) != p->old_kcps) {
      /* frequency changed */
      new_frq = 1;
      p->old_kcps = *(p->kcps);
      /* calculate variables that depend on freq., and are used by all modes */
      p->omega = (double) p->old_kcps * TWOPI / (double) CS_ESR;
      p->cs = cos(p->omega);
      p->sn = sqrt(1.0 - p->cs * p->cs);
      //printf("**** (%d) p->cs = %f\n", __LINE__, p->cs);
    }
    else
      new_frq = 0;
    /* copy object data to local variables */
    ar = p->ar; asig = p->asig;
    xnm1 = p->xnm1; xnm2 = p->xnm2; ynm1 = p->ynm1; ynm2 = p->ynm2;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->ftype) {
    case 0:                                     /* lowpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = p->sn * 0.5 / (double) p->old_kQ;       /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (0.5 * (dva0 - dva0 * p->cs));
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp + xnm1 + xnm1 + xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 1:                                     /* highpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = p->sn * 0.5 / (double) p->old_kQ;       /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (0.5 * (dva0 + dva0 * p->cs));
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp - xnm1 - xnm1 + xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 2:                                     /* bandpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (dva0 * alpha);
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp - xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 3:                                     /* band-reject (notch) filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) dva0;
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp + xnm2) - a1 * (ynm1 - xnm1) - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 4:                                     /* peaking EQ */
      if (new_frq || *(p->kQ) != p->old_kQ || *(p->klvl) != p->old_klvl) {
        double  sq, alpha, tmp1, tmp2;
        p->old_kQ = *(p->kQ);
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        //printf("*** (%d) p->old_klvl\n", __LINE__, p->old_klvl);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        tmp1 = alpha / sq;
        dva0 = 1.0 / (1.0 + tmp1);
        tmp2 = alpha * sq * dva0;
        p->b0 = (MYFLT) (dva0 + tmp2);
        p->b2 = (MYFLT) (dva0 - tmp2);
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * tmp1);
      }
      b0 = p->b0; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b2 * xnm2 - a1 * (ynm1 - xnm1) - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 5:                                     /* low shelf */
      if (new_frq || *(p->klvl) != p->old_klvl || *(p->kS) != p->old_kS) {
        double sq, beta, tmp1, tmp2, tmp3, tmp4;
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        p->old_kS = *(p->kS);
        beta = p->sn * sqrt(((double) p->old_klvl + 1.0) / p->old_kS
                            - (double) p->old_klvl + sq + sq - 1.0);
        /* recalculate all coeffs */
        tmp1 = sq + 1.0;
        tmp2 = sq - 1.0;
        tmp3 = tmp1 * p->cs;
        tmp4 = tmp2 * p->cs;
        dva0 = 1.0 / (tmp1 + tmp4 + beta);
        p->a1 = (MYFLT) (-2.0 * dva0 * (tmp2 + tmp3));
        p->a2 = (MYFLT) (dva0 * (tmp1 + tmp4 - beta));
        dva0 *= sq;
        p->b0 = (MYFLT) (dva0 * (tmp1 - tmp4 + beta));
        p->b1 = (MYFLT) ((dva0 + dva0) * (tmp2 - tmp3));
        p->b2 = (MYFLT) (dva0 * (tmp1 - tmp4 - beta));
      }
      b0 = p->b0; b1 = p->b1; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b1 * xnm1 + b2 * xnm2 - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 6:                                     /* high shelf */
      if (new_frq || *(p->klvl) != p->old_klvl || *(p->kS) != p->old_kS) {
        double sq, beta, tmp1, tmp2, tmp3, tmp4;
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        p->old_kS = *(p->kS);
        beta = p->sn * sqrt(((double) p->old_klvl + 1.0) / p->old_kS
                            - (double) p->old_klvl + sq + sq - 1.0);
        /* recalculate all coeffs */
        tmp1 = sq + 1.0;
        tmp2 = sq - 1.0;
        tmp3 = tmp1 * p->cs;
        tmp4 = tmp2 * p->cs;
        dva0 = 1.0 / (tmp1 - tmp4 + beta);
        p->a1 = (MYFLT) ((dva0 + dva0) * (tmp2 - tmp3));
        p->a2 = (MYFLT) (dva0 * (tmp1 - tmp4 - beta));
        dva0 *= sq;
        p->b0 = (MYFLT) (dva0 * (tmp1 + tmp4 + beta));
        p->b1 = (MYFLT) (-2.0 * dva0 * (tmp2 + tmp3));
        p->b2 = (MYFLT) (dva0 * (tmp1 + tmp4 - beta));
      }
      b0 = p->b0; b1 = p->b1; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b1 * xnm1 + b2 * xnm2 - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    default:
      return csound->PerfError(csound, &(p->h),
                               Str("rbjeq: invalid filter type"));
      break;
    }
    /* save filter state */
    p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
    return OK;
}

/* ------------------------------------------------------------------------- */

static const OENTRY vco2_localops[] = {
    { "vco2init",   sizeof(VCO2INIT),   TW, 1,      "i",    "ijjjjj",
            (SUBR) vco2init, (SUBR) NULL, (SUBR) NULL                   },
    { "vco2ift",    sizeof(VCO2FT),     TW, 1,      "i",    "iov",
            (SUBR) vco2ftset, (SUBR) NULL, (SUBR) NULL                  },
    { "vco2ft",     sizeof(VCO2FT),     TW, 3,      "k",    "kov",
            (SUBR) vco2ftset, (SUBR) vco2ft, (SUBR) NULL                },
    { "vco2",       sizeof(VCO2),       TR, 3,      "a",    "kkoOOo",
     (SUBR) vco2set, (SUBR) vco2                    },
    { "denorm",     sizeof(DENORMS),   WI,  2,      "",     "y",
            (SUBR) NULL, (SUBR) denorms, NULL  },
    { "delayk",     sizeof(DELAYK),    0,  3,      "k",    "kio",
            (SUBR) delaykset, (SUBR) delayk, (SUBR) NULL                },
    { "vdel_k",     sizeof(VDELAYK),   0,  3,      "k",    "kkio",
            (SUBR) vdelaykset, (SUBR) vdelayk, (SUBR) NULL              },
    { "rbjeq",      sizeof(RBJEQ),     0,  3,      "a",    "akkkko",
            (SUBR) rbjeqset, (SUBR) rbjeq                  }
};

LINKAGE_BUILTIN(vco2_localops)
