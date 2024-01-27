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
#include "active.h"
#include "cwindow.h"
#include <inttypes.h>

#define S       sizeof


int32_t instcount(CSOUND *csound, INSTCNT *p)
{
    int32_t n;

    if (csound->ISSTRCOD(*p->ins)) {
      char *ss = get_arg_string(csound,*p->ins);
      n = csound->strarg2insno(csound,ss,1);
    }
    else n = *p->ins;

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int32_t tot = 1;
      for (n=1; n<csound->engineState.maxinsno; n++)
        if (csound->engineState.instrtxtp[n]) /* If it exists */
          tot += ((*p->opt) ? csound->engineState.instrtxtp[n]->instcnt :
                              csound->engineState.instrtxtp[n]->active);
      *p->cnt = (MYFLT)tot;
    }
    else {
      //csound->Message(csound, "Instr %p \n", csound->engineState.instrtxtp[n]);
      *p->cnt = ((*p->opt) ?
                 (MYFLT) csound->engineState.instrtxtp[n]->instcnt :
                 (MYFLT) csound->engineState.instrtxtp[n]->active);
      if (*p->norel)
        *p->cnt -= csound->engineState.instrtxtp[n]->pending_release;
    }

    return OK;
}

int32_t instcount_S(CSOUND *csound, INSTCNT *p)
{


    int32_t n = csound->strarg2insno(csound, ((STRINGDAT *)p->ins)->data, 1);

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int32_t tot = 1;
      for (n=1; n<csound->engineState.maxinsno; n++)
        if (csound->engineState.instrtxtp[n]) /* If it exists */
          tot += ((*p->opt) ? csound->engineState.instrtxtp[n]->instcnt :
                              csound->engineState.instrtxtp[n]->active);
      *p->cnt = (MYFLT)tot;
    }
    else {
      *p->cnt = ((*p->opt) ?
                 (MYFLT) csound->engineState.instrtxtp[n]->instcnt :
                 (MYFLT) csound->engineState.instrtxtp[n]->active);
      if (*p->norel)
        *p->cnt -= csound->engineState.instrtxtp[n]->pending_release;
    }

    return OK;
}


static OENTRY active_localops[] = {
    { "active",    0xffff                                                                  },
    { "active.iS", S(INSTCNT),0,1, "i",  "Soo",  (SUBR)instcount_S, NULL,             NULL },
    { "active.kS", S(INSTCNT),0,2, "k",  "Soo",  NULL,             (SUBR)instcount_S, NULL },
    { "active.i",  S(INSTCNT),0,1, "i",  "ioo",  (SUBR)instcount,  NULL,              NULL },
    { "active.k",  S(INSTCNT),0,2, "k",  "koo",  NULL,             (SUBR)instcount,   NULL },
};

LINKAGE_BUILTIN(active_localops)
