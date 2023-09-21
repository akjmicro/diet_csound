/*
    spectra.c:

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
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"
#include <inttypes.h>

#define LOGTWO  (0.69314718056)


static const char *outstring[] = {"mag", "db", "mag sqrd", "root mag"};


#define S       sizeof

static OENTRY spectra_localops[] = {
{ "pitch", S(PITCH),   0,  3,    "kk", "aiiiiqooooojo",
                                             (SUBR)pitchset, (SUBR)pitch },
{ "maca", S(SUM),      0,  3,  "a", "y",    (SUBR)macset, (SUBR)maca  },
{ "mac", S(SUM),       0,  3,  "a", "Z",    (SUBR)macset, (SUBR)mac   },
{ "clockon", S(CLOCK), 0,  3,  "",  "i",    (SUBR)clockset, (SUBR)clockon, NULL  },
{ "clockoff", S(CLOCK),0,  3,  "",  "i",    (SUBR)clockset, (SUBR)clockoff, NULL },
{ "readclock", S(CLKRD),0, 1,  "i", "i",    (SUBR)clockread, NULL, NULL          },
{ "readscratch", S(SCRATCHPAD),0, 1, "i", "o", (SUBR)scratchread, NULL, NULL     },
{ "writescratch", S(SCRATCHPAD),0, 1, "", "io", (SUBR)scratchwrite, NULL, NULL   },
{ "pitchamdf",S(PITCHAMDF),0,3,"kk","aiioppoo",
                                     (SUBR)pitchamdfset, (SUBR)pitchamdf },
{ "hsboscil",S(HSBOSC), TR, 3, "a", "kkkiiioo",

                                    (SUBR)hsboscset,(SUBR)hsboscil },
{ "phasorbnk", S(PHSORBNK),0,3,"a", "xkio",
                                (SUBR)phsbnkset, (SUBR)phsorbnk },
{ "phasorbnk.k", S(PHSORBNK),0,3,"k", "xkio",
                                (SUBR)phsbnkset, (SUBR)kphsorbnk, NULL},
{ "adsynt",S(HSBOSC), TR, 3,  "a", "kkiiiio", (SUBR)adsyntset, (SUBR)adsynt },
{ "mpulse", S(IMPULSE), 0, 3,  "a", "kko",
                                    (SUBR)impulse_set, (SUBR)impulse },
{ "lpf18", S(LPF18), 0, 3,  "a", "axxxo",  (SUBR)lpf18set, (SUBR)lpf18db },
{ "waveset", S(BARRI), 0, 3,  "a", "ako",  (SUBR)wavesetset, (SUBR)waveset},
{ "pinkish", S(PINKISH), 0, 3, "a", "xoooo", (SUBR)pinkset, (SUBR)pinkish },
{ "noise",  S(VARI), 0, 3,  "a", "xk",   (SUBR)varicolset, (SUBR)varicol },
{ "transeg", S(TRANSEG),0, 3,  "k", "iiim",
                                           (SUBR)trnset,(SUBR)ktrnseg, NULL},
{ "transeg.a", S(TRANSEG),0, 3,  "a", "iiim",
                                           (SUBR)trnset,(SUBR)trnseg},
{ "transegb", S(TRANSEG),0, 3,  "k", "iiim",
                              (SUBR)trnset_bkpt,(SUBR)ktrnseg,(SUBR)NULL},
{ "transegb.a", S(TRANSEG),0, 3,  "a", "iiim",
                              (SUBR)trnset_bkpt,(SUBR)trnseg    },
{ "transegr", S(TRANSEG),0, 3, "k", "iiim",
                              (SUBR)trnsetr,(SUBR)ktrnsegr,(SUBR)NULL },
{ "transegr.a", S(TRANSEG),0, 3, "a", "iiim",
                              (SUBR)trnsetr,(SUBR)trnsegr      },
{ "clip", S(CLIP),      0, 3,  "a", "aiiv", (SUBR)clip_set, (SUBR)clip  },
{ "cpuprc", S(CPU_PERC),0, 1,     "",     "Si",   (SUBR)cpuperc_S, NULL, NULL   },
{ "maxalloc", S(CPU_PERC),0, 1,   "",     "Si",   (SUBR)maxalloc_S, NULL, NULL  },
{ "cpuprc", S(CPU_PERC),0, 1,     "",     "ii",   (SUBR)cpuperc, NULL, NULL   },
{ "maxalloc", S(CPU_PERC),0, 1,   "",     "ii",   (SUBR)maxalloc, NULL, NULL  },
{ "active", 0xffff                                                          },
{ "active.iS", S(INSTCNT),0,1,    "i",    "Soo",   (SUBR)instcount_S, NULL, NULL },
{ "active.kS", S(INSTCNT),0,2,    "k",    "Soo",   NULL, (SUBR)instcount_S, NULL },
{ "active.i", S(INSTCNT),0,1,     "i",    "ioo",   (SUBR)instcount, NULL, NULL },
{ "active.k", S(INSTCNT),0,2,     "k",    "koo",   NULL, (SUBR)instcount, NULL },
{ "p.i", S(PFUN),        0,1,     "i",    "i",     (SUBR)pfun, NULL, NULL     },
{ "p.k", S(PFUNK),       0,3,     "k",    "k",
                                          (SUBR)pfunk_init, (SUBR)pfunk, NULL },
{ "mute", S(MUTE), 0,1,           "",      "So",   (SUBR)mute_inst_S             },
{ "mute.i", S(MUTE), 0,1,         "",      "io",   (SUBR)mute_inst             },
{ "median", S(MEDFILT), 0, 3,     "a", "akio", (SUBR)medfiltset, (SUBR)medfilt },
{ "mediank", S(MEDFILT), 0,3,     "k", "kkio", (SUBR)medfiltset, (SUBR)kmedfilt},
};

LINKAGE_BUILTIN(spectra_localops)
