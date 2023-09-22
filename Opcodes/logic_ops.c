/*
    logic_ops.c:

    Copyright (C) 2001 William 'Pete' Moss

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

                                                        /* ugmoss.c */
#include "stdopcod.h"
#include "aops.h"
#include <math.h>

/******************************************************************************
  all this code was written by william 'pete' moss. <petemoss@petemoss.org>
  no copyright, since it seems silly to copyright algorithms.
  do what you want with the code, and credit me if you get the chance
******************************************************************************/


static int32_t and_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    int32_t input2 = MYFLT2LRND64(*p->b);
    *p->r = (MYFLT)(input1 & input2);
    return OK;
}

static int32_t and_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r    = p->r;
    MYFLT *in1  = p->a;
    MYFLT *in2  = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t   n, nsmps = CS_KSMPS;
    int32_t  input1, input2;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT) (input1 & input2);
    }
    return OK;
}

static int32_t and_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2 = MYFLT2LRND64(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int32_t and_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2, input1 = MYFLT2LRND64(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int32_t or_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    int32_t input2 = MYFLT2LRND64(*p->b);
    *p->r = (MYFLT)(input1 | input2);
    return OK;
}

static int32_t or_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2, input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t or_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2 = MYFLT2LRND64(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t or_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2, input1 = MYFLT2LRND64(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t xor_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    int32_t input2 = MYFLT2LRND64(*p->b);
    *p->r = (MYFLT)(input1 ^ input2);
    return OK;
}

static int32_t xor_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2, input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int32_t xor_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2 = MYFLT2LRND64(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int32_t xor_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input2, input1 = MYFLT2LRND64(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND64(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int32_t shift_left_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    int32_t input2 = (int32_t) MYFLT2LRND64(*p->b);
    *p->r = (MYFLT) (input1 << input2);
    return OK;
}

static int32_t shift_left_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1, input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(p->a[n]);
      input2 = (int32) MYFLT2LRND64(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int32_t shift_left_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1;
    int32_t  input2 = MYFLT2LRND64(*p->b);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(p->a[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int32_t shift_left_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1 = MYFLT2LRND64(*p->a), input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND64(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int32_t shift_right_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    int32_t  input2 = (int32_t) MYFLT2LRND64(*p->b);
    *p->r = (MYFLT) (input1 >> input2);
    return OK;
}

static int32_t shift_right_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1, input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(p->a[n]);
      input2 = (int32_t) MYFLT2LRND64(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int32_t shift_right_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1;
    int32_t  input2 = MYFLT2LRND64(*p->b);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(p->a[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int32_t shift_right_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    int32_t  input1 = MYFLT2LRND64(*p->a), input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND64(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int32_t not_k(CSOUND *csound, AOP *p)    /* Added for completeness by JPff */
{
    IGN(csound);
    int32_t input1 = MYFLT2LRND64(*p->a);
    *p->r = (MYFLT)(~input1);
    return OK;
}

static int32_t not_a(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
     for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND64(in1[n]);
      r[n] = (MYFLT)(~input1);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY logic_localops[] = {
    { "##and.ii",  S(AOP),  0,1, "i", "ii", (SUBR)and_kk                },
    { "##and.kk",  S(AOP),  0,2, "k", "kk", NULL,   (SUBR)and_kk        },
    { "##and.ka",  S(AOP),  0,2, "a", "ka", NULL,   (SUBR)and_ka        },
    { "##and.ak",  S(AOP),  0,2, "a", "ak", NULL,   (SUBR)and_ak        },
    { "##and.aa",  S(AOP),  0,2, "a", "aa", NULL,   (SUBR)and_aa        },
    { "##or.ii",   S(AOP),  0,1, "i", "ii", (SUBR)or_kk                 },
    { "##or.kk",   S(AOP),  0,2, "k", "kk", NULL,   (SUBR)or_kk         },
    { "##or.ka",   S(AOP),  0,2, "a", "ka", NULL,   (SUBR)or_ka         },
    { "##or.ak",   S(AOP),  0,2, "a", "ak", NULL,   (SUBR)or_ak         },
    { "##or.aa",   S(AOP),  0,2, "a", "aa", NULL,   (SUBR)or_aa         },
    { "##xor.ii",  S(AOP),  0,1, "i", "ii", (SUBR)xor_kk                },
    { "##xor.kk",  S(AOP),  0,2, "k", "kk", NULL,   (SUBR)xor_kk        },
    { "##xor.ka",  S(AOP),  0,2, "a", "ka", NULL,   (SUBR)xor_ka        },
    { "##xor.ak",  S(AOP),  0,2, "a", "ak", NULL,   (SUBR)xor_ak        },
    { "##xor.aa",  S(AOP),  0,2, "a", "aa", NULL,   (SUBR)xor_aa        },
    { "##not.i",   S(AOP),  0,1, "i", "i",  (SUBR)not_k                 },
    { "##not.k",   S(AOP),  0,2, "k", "k",  NULL,   (SUBR)not_k         },
    { "##not.a",   S(AOP),  0,2, "a", "a",  NULL,   (SUBR)not_a         },
    { "##shl.ii",  S(AOP),  0,1, "i", "ii", (SUBR) shift_left_kk        },
    { "##shl.kk",  S(AOP),  0,2, "k", "kk", NULL, (SUBR) shift_left_kk  },
    { "##shl.ka",  S(AOP),  0,2, "a", "ka", NULL, (SUBR) shift_left_ka  },
    { "##shl.ak",  S(AOP),  0,2, "a", "ak", NULL, (SUBR) shift_left_ak  },
    { "##shl.aa",  S(AOP),  0,2, "a", "aa", NULL, (SUBR) shift_left_aa  },
    { "##shr.ii",  S(AOP),  0,1, "i", "ii", (SUBR) shift_right_kk       },
    { "##shr.kk",  S(AOP),  0,2, "k", "kk", NULL, (SUBR) shift_right_kk },
    { "##shr.ka",  S(AOP),  0,2, "a", "ka", NULL, (SUBR) shift_right_ka },
    { "##shr.ak",  S(AOP),  0,2, "a", "ak", NULL, (SUBR) shift_right_ak },
    { "##shr.aa",  S(AOP),  0,2, "a", "aa", NULL, (SUBR) shift_right_aa }
};

int32_t logic_localops_init(CSOUND *csound)
{
    (void) csound;
    return (int32_t) (sizeof(logic_localops) / sizeof(OENTRY));
}
