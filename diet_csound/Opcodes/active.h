#ifndef ACTIVE_H
#define ACTIVE_H
/*
    active.h:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neub�cker,
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

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        MYFLT   *ins;
        MYFLT   *opt;
        MYFLT   *norel;
} INSTCNT;

int32_t instcount(CSOUND *, INSTCNT *p);
int32_t instcount_S(CSOUND *, INSTCNT *p);

#endif /* ACTIVE_H */
