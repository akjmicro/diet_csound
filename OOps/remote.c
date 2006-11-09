/*
    remote.c:

    Copyright (C) 2006 by Barry Vercoe

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"
#include "remote.h"

#ifdef HAVE_SOCKETS
#ifndef WIN32
#include <sys/ioctl.h>
#ifdef LINUX
#include <linux/if.h>
#endif
#include <net/if.h>
#endif
#endif /* HAVE_SOCKETS */

#define MAXREMOTES 10

#define ST(x)   (((REMOTE_GLOBALS*) ((CSOUND*)csound)->remoteGlobals)->x)

void remote_Cleanup(CSOUND *csound);

void remoteRESET(CSOUND *csound)
{
    csound->remoteGlobals = NULL;
}

#ifdef HAVE_SOCKETS

 /* get the IPaddress of this machine */
static int getIpAddress(char *ipaddr, char *ifname)
{
    int ret = 1;
#ifdef WIN32
    /* VL 12/10/06: something needs to go here */
    /* gethostbyname is the real answer; code below id unsafe */
    char hostname[1024];
    struct hostent *he;
    struct sockaddr_in sin;
    gethostname(hostname, sizeof(hostname));
    he = gethostbyname(hostname);

    memset(&sin, 0, sizeof (struct sockaddr_in));
    memmove(&sin.sin_addr, he->h_addr_list[0], he->h_length);
    strcpy(ipaddr, inet_ntoa (sin.sin_addr));
    ret = 0;

#else
    struct ifreq ifr;
    int fd;

    fd = socket(AF_INET,SOCK_DGRAM, 0);
    if (fd >= 0) {
      strcpy(ifr.ifr_name, ifname);

      if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        char *local;
        local = inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr);
        strcpy(ipaddr, local);
        ret = 0;
      }
    }
    close(fd);
#endif
    return ret;
}

char remoteID(CSOUND *csound)
{
    int len = strlen(ST(ipadrs));
    return ST(ipadrs)[len-1];
}

static int callox(CSOUND *csound)
{
    if (csound->remoteGlobals == NULL) {
      csound->remoteGlobals = csound->Calloc(csound, sizeof(REMOTE_GLOBALS));
      if (csound->remoteGlobals == NULL) {
        csound->Message(csound, Str("insufficient memory to initialize remote"
                        " globals."));
        goto error;
      }
/*       ST(remote_port) = REMOT_PORT; */
    }

    ST(socksout) = (SOCK*)csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(SOCK));
    if (ST(socksout) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize outgoing "
                      "socket table."));
      goto error;
    }

    ST(socksin) = (int*) csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(int));
    if (ST(socksin) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize incoming "
                      "socket table."));
      goto error;
    }

    ST(insrfd_list) =
      (int*) csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(int));
    if (ST(insrfd_list) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize "
                      "insrfd_list."));
      goto error;
    }

    ST(chnrfd_list) =
      (int*) csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(int));
    if (ST(chnrfd_list) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize "
                      "chnrfd_list."));
      goto error;
    }

    ST(insrfd) = (int*) csound->Calloc(csound,(size_t)129 * sizeof(int));
    if (ST(insrfd) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize "
                      "insrfd table."));
      goto error;
    }

    ST(chnrfd) = (int*) csound->Calloc(csound,(size_t)17 * sizeof(int));
    if (ST(chnrfd) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize "
                      "chnrfd table."));
      goto error;
    }

    ST(ipadrs) = (char*) csound->Calloc(csound,(size_t)15 * sizeof(char));
    if (ST(ipadrs) == NULL) {
      csound->Message(csound, Str("insufficient memory to initialize "
                      "local ip address."));
      goto error;
    }

    /* get IP adrs of this machine */
    /* FIXME: don't hardcode eth0 */
    if (getIpAddress(ST(ipadrs), "eth0") < 0) {
      csound->Message(csound, Str("unable to get local ip address."));
      goto error;
    }

    return 0;

error:
    /* Clean up anything we may have allocated before running out of memory */
    remote_Cleanup(csound);
    return -1;
}

/* Cleanup the above; called from musmon csoundCleanup */
void remote_Cleanup(CSOUND *csound)
{
    int fd;
    if (csound->remoteGlobals == NULL) return;
    if (ST(socksout) != NULL) {
      SOCK *sop = ST(socksout), *sop_end = sop + MAXREMOTES;
      for ( ; sop < sop_end; sop++)
        if ((fd = sop->rfd) > 0)
          close(fd);
      csound->Free(csound,(char *)ST(socksout));
      ST(socksout) = NULL;
    }
    if (ST(socksin) != NULL) {
      int *sop = ST(socksin), *sop_end = sop + MAXREMOTES;
      for ( ; sop < sop_end; sop++)
        if ((fd = *sop) > 0)
          close(fd);
      csound->Free(csound,(char *)ST(socksin));
      ST(socksin) = NULL;
    }
    if (ST(insrfd_list) != NULL) {
      csound->Free(csound, ST(insrfd_list));   ST(insrfd_list) = NULL;
    }
    if (ST(chnrfd_list) != NULL) {
      csound->Free(csound, ST(chnrfd_list));   ST(chnrfd_list) = NULL; }
    if (ST(insrfd) != NULL) {
      csound->Free(csound, ST(insrfd)); ST(insrfd) = NULL; }
    if (ST(chnrfd) != NULL) {
      csound->Free(csound, ST(chnrfd)); ST(chnrfd) = NULL; }
    if (ST(ipadrs) != NULL) {
      csound->Free(csound, ST(ipadrs)); ST(ipadrs) = NULL; }
    ST(insrfd_count) = ST(chnrfd_count) = 0;
    csound->Free(csound, csound->remoteGlobals);
    csound->remoteGlobals = NULL;
}

static int CLopen(CSOUND *csound, char *ipadrs)     /* Client -- open to send */
{
    int rfd, i;

    SOCK *sop = ST(socksout), *sop_end = sop + MAXREMOTES;
    do {
      if (ipadrs == sop->adr)                      /* if socket already exists */
        return sop->rfd;                           /*   return with that   */
    } while (++sop < sop_end);

    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    if (( rfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      csound->InitError(csound, Str("could not open remote port"));
      return NOTOK;
    }
    memset(&(ST(to_addr)), 0, sizeof(ST(to_addr)));        /* clear sock mem */
    ST(to_addr).sin_family = AF_INET;                  /* set as INET address */
    /* server IP adr, netwk byt order */
#ifdef WIN32
    ST(to_addr).sin_addr.S_un.S_addr = inet_addr((const char *)ipadrs);
#else
    inet_aton((const char *)ipadrs, &(ST(to_addr).sin_addr));
#endif
    ST(to_addr).sin_port = htons((int) REMOT_PORT);    /* port we will listen on,
                                                      network byte order */
/*     ST(to_addr).sin_port = htons((int) ST(remote_port)); */
    for (i=0; i<10; i++){
      if (connect(rfd, (struct sockaddr *) &ST(to_addr), sizeof(ST(to_addr))) < 0)
        csound->Message(csound, Str("---> Could not connect \n"));
      else goto conok;
    }
    csound->InitError(csound, Str("---> Failed all attempts to connect. \n"));
    return NOTOK;
 conok:
    csound->Message(csound, Str("--->  Connected. \n"));
    for (sop = ST(socksout); sop < sop_end; sop++)
      if (sop->adr == NULL) {
        sop->adr = ipadrs;                         /* record the new socket */
        sop->rfd = rfd;
        break;
      }
    return rfd;
}

int CLsend(CSOUND *csound, int conn, void *data, int length)
{
    int nbytes;
    if ((nbytes = write(conn, data, length)) <= 0) {
      csound->PerfError(csound, Str("write to socket failed"));
      return NOTOK;
    }
    /*    csound->Message(csound, "nbytes sent: %d \n", nbytes); */
    return OK;
}

static int SVopen(CSOUND *csound, char *ipadrs_local)
           /* Server -- open to receive */
{
    int conn, socklisten,opt;
    char ipadrs[15];
    int *sop = ST(socksin), *sop_end = sop + MAXREMOTES;
#ifdef WIN32
    int clilen;
#else
    socklen_t clilen;
#endif
    opt = 1;

    if ((socklisten = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      csound->InitError(csound, Str("creating socket\n"));
      return NOTOK;
    }
    else csound->Message(csound, Str("created socket \n"));
    /* set the addresse to be reusable */
    if ( setsockopt(socklisten, SOL_SOCKET, SO_REUSEADDR,
#ifdef WIN32
                    (const char *)&opt,
#else
                    &opt,
#endif
                    sizeof(opt)) < 0 )

      csound->InitError(csound,
                        Str("setting socket option to reuse the addresse \n"));

    memset(&(ST(to_addr)), 0, sizeof(ST(to_addr)));    /* clear sock mem */
    ST(local_addr).sin_family = AF_INET;               /* set as INET address */
    /* our adrs, netwrk byt order */
#ifdef WIN32
    ST(to_addr).sin_addr.S_un.S_addr = inet_addr((const char *)ipadrs);
#else
    inet_aton((const char *)ipadrs, &(ST(local_addr).sin_addr));
#endif
    ST(local_addr).sin_port = htons((int)REMOT_PORT); /* port we will listen on,
                                                         netwrk byt order */
/*     ST(local_addr).sin_port = htons((int) ST(remote_port)); */
    /* associate the socket with the address and port */
    if (bind (socklisten,
              (struct sockaddr *) &ST(local_addr),
              sizeof(ST(local_addr))) < 0) {
      csound->InitError(csound, Str("bind failed"));
      return NOTOK;
    }
    if (listen(socklisten, 5) < 0) {    /* start the socket listening
                                           for new connections -- may wait */
      csound->InitError(csound, Str("listen failed"));
      return NOTOK;
    }
    clilen = sizeof(ST(local_addr));  /* FIX THIS FOR MULTIPLE CLIENTS !!!!!!!*/
    conn = accept(socklisten, (struct sockaddr *) &ST(local_addr), &clilen);
    if (conn < 0) {
      csound->InitError(csound, Str("accept failed"));
      return NOTOK;
    }
    else {
      csound->Message(csound, Str("accepted, conn=%d \n"), conn);
      for (sop = ST(socksin); sop < sop_end; sop++)
        if (*sop == 0) {
          *sop = conn;                       /* record the new connection */
          break;
        }
    }
    return OK;
}

int SVrecv(CSOUND *csound, int conn, void *data, int length)
{
    struct sockaddr from;
#ifdef WIN32 /* VL, 12/10/06: I'm guessing here. If someone knows better, fix it */
#define MSG_DONTWAIT  0
   int clilen = sizeof(from);
#else
    socklen_t clilen = sizeof(from);
#endif
    size_t n;
    n = recvfrom(conn, data, length, MSG_DONTWAIT, &from, &clilen);
    /*  if (n>0) csound->Message(csound, "nbytes received: %d \n", (int)n); */
    return (int)n;
}

/* /////////////  INSTR 0 opcodes ///////////////////// */

int insremot(CSOUND *csound, INSREMOT *p)
/* declare certain instrs for remote Csounds */
{   /*      INSTR 0 opcode  */
    short nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (callox(csound) < 0) {
        csound->InitError(csound, Str("failed to initialize remote globals."));
        return 0;
      }
    }
    if (nargs < 3) {
      csound->InitError(csound, Str("missing instr nos"));
      return 0;
    }
    csound->Message(csound, Str("*** str1: %s own:%s\n"),
                    (char *)p->str1 , ST(ipadrs));
    if (strcmp(ST(ipadrs), (char *)p->str1) == 0) {  /* if client is this adrs */
      MYFLT   **argp = p->insno;
      int rfd = 0;
      if ((rfd = CLopen(csound, (char *)p->str2)) <= 0) /* open port to remote */
        return NOTOK;
      for (nargs -= 2; nargs--; ) {
        short insno = (short)**argp++;     /* & for each insno */
        if (insno <= 0 || insno > 128) {
          csound->InitError(csound, Str("illegal instr no"));
          return NOTOK;
        }
        if (ST(insrfd)[insno]) {
          csound->InitError(csound, Str("insno already remote"));
          return NOTOK;
        }
        ST(insrfd)[insno] = rfd;   /*  record file descriptor   */
      }
      ST(insrfd_list)[ST(insrfd_count)++] = rfd;   /*  and make a list    */
    }
    else if (!strcmp(ST(ipadrs),(char *)p->str2)) { /* if server is this adrs*/
      csound->Message(csound, Str("*** str2: %s own:%s\n"),
                      (char *)p->str2 , ST(ipadrs));
      if (SVopen(csound, (char *)p->str2) == NOTOK){ /* open port to listen */
        csound->InitError(csound, Str("Failed to open port to listen"));
        return NOTOK;
      }
    }
    return OK;
}

int insglobal(CSOUND *csound, INSGLOBAL *p)
/* declare certain instrs global remote Csounds */
{   /*      INSTR 0 opcode  */
    short nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (callox(csound) < 0) {
        csound->InitError(csound, Str("failed to initialize remote globals."));
        return 0;
      }
    }
    if (nargs < 2) {
      csound->InitError(csound, Str("missing instr nos"));
      return NOTOK;
    }
    csound->Message(csound, Str("*** str1: %s own:%s\n"),
                    (char *)p->str1 , ST(ipadrs));
    if (strcmp(ST(ipadrs), (char *)p->str1) == 0) { /* if client is this adrs */
      MYFLT   **argp = p->insno;
      for (nargs -= 1; nargs--; ) {
        short insno = (short)**argp++;             /* for each insno */
        if (insno <= 0 || insno > 128) {
          csound->InitError(csound, Str("illegal instr no"));
          return NOTOK;
        }
        if (ST(insrfd)[insno]) {
          csound->InitError(csound, Str("insno already specific remote"));
          return NOTOK;
        }
        ST(insrfd)[insno] = GLOBAL_REMOT;             /*  mark as GLOBAL   */
      }
    }
    return OK;
}

int midremot(CSOUND *csound, MIDREMOT *p)    /* declare certain channels for
                                                remote Csounds */
{                                            /* INSTR 0 opcode  */
    short nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (callox(csound) < 0) {
        csound->InitError(csound, Str("failed to initialize remote globals."));
        return 0;
      }
    }
    if (nargs < 3) {
      csound->InitError(csound, Str("missing channel nos"));
      return NOTOK;
    }
    if (strcmp(ST(ipadrs), (char *)p->str1) == 0) {  /* if client is this adrs */
      MYFLT   **argp = p->chnum;
      int  rfd;
      if ((rfd = CLopen(csound, (char *)p->str2)) <= 0) /* open port to remot */
        return NOTOK;
      for (nargs -= 2; nargs--; ) {
        short chnum = (short)**argp++;               /* & for each channel   */
        if (chnum <= 0 || chnum > 16) {              /* THESE ARE MIDCHANS+1 */
          csound->InitError(csound, Str("illegal channel no"));
          return NOTOK;
        }
        if (ST(chnrfd)[chnum]) {
          csound->InitError(csound, Str("channel already remote"));
          return NOTOK;
        }
        ST(chnrfd)[chnum] = rfd;                      /* record file descriptor */
                }
                ST(chnrfd_list)[ST(chnrfd_count)++] = rfd;   /* and make a list */
    }
    else if (!strcmp(ST(ipadrs), (char *)p->str2)) { /* if server is this adrs */
      if(SVopen(csound, (char *)p->str2) == NOTOK){  /* open port to listen */
        csound->InitError(csound, Str("Failed to open port to listen"));
        return NOTOK;
      }
      csound->oparms->RMidiin = 1;            /* & enable rtevents in */
    }
    return OK;
}

int midglobal(CSOUND *csound, MIDGLOBAL *p)
/* declare certain chnls global remote Csounds */
{                                         /*       INSTR 0 opcode  */
    short nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (callox(csound) < 0) {
        csound->InitError(csound, Str("failed to initialize remote globals."));
        return 0;
      }
    }
    if (nargs < 2) {
      csound->InitError(csound, Str("missing channel nos"));
      return NOTOK;
    }
    csound->Message(csound, Str("*** str1: %s own:%s\n"),
                    (char *)p->str1 , ST(ipadrs));
    if (strcmp(ST(ipadrs), (char *)p->str1) == 0) { /* if client is this adrs */
      MYFLT   **argp = p->chnum;
      for (nargs -= 1; nargs--; ) {
        short chnum = (short)**argp++;             /* for each channel */
        if (chnum <= 0 || chnum > 16) {
          csound->InitError(csound, Str("illegal channel no"));
          return NOTOK;
        }
        if (ST(chnrfd)[chnum]) {
          csound->InitError(csound, Str("channel already specific remote"));
          return NOTOK;
        }
        ST(chnrfd)[chnum] = GLOBAL_REMOT;              /*  mark as GLOBAL   */
                }
    }
    return OK;
}

/* ////////////////       MUSMON SERVICES //////////////// */

int insSendevt(CSOUND *csound, EVTBLK *evt, int rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    EVTBLK *cpp = (EVTBLK *)bp->data;       /* align an EVTBLK struct */
    int nn;
    MYFLT *f, *g;
    cpp->strarg = NULL;                     /* copy the initial header */
    cpp->opcod = evt->opcod;
    cpp->pcnt = evt->pcnt;
    f = &evt->p2orig;
    g = &cpp->p2orig;
    for (nn = evt->pcnt + 3; nn--; )        /* copy the remaining data */
      *g++ = *f++;
    bp->type = SCOR_EVT;                    /* insert type and len */
    bp->len = (char *)g - (char *)bp;
    if (CLsend(csound, rfd, (void *)bp, (int)bp->len) < 0) {
      csound->PerfError(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

int insGlobevt(CSOUND *csound, EVTBLK *evt)  /* send an event to all remote fd's */
{
    int nn;
    for (nn = 0; nn < ST(insrfd_count); nn++) {
      if (insSendevt(csound, evt, ST(insrfd_list)[nn]) == NOTOK)
        return NOTOK;
    }
    return OK;
}

int MIDIsendevt(CSOUND *csound, MEVENT *evt, int rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    MEVENT *mep = (MEVENT *)bp->data;       /* align an MEVENT struct */
    *mep = *evt;                            /*      & copy the data   */
    bp->type = MIDI_EVT;                    /* insert type and len    */
    bp->len = sizeof(int) * 2 + sizeof(MEVENT);

    if (CLsend(csound, rfd, (void *)bp, (size_t)bp->len) < 0) {
      csound->PerfError(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

int MIDIGlobevt(CSOUND *csound, MEVENT *evt) /* send an Mevent to all remote fd's */
{
    int nn;
    for (nn = 0; nn < ST(chnrfd_count); nn++) {
      if (MIDIsendevt(csound, evt, ST(chnrfd_list)[nn]) == NOTOK)
        return NOTOK;
    }
    return OK;
}

int MIDIsend_msg(CSOUND *csound, MEVENT *evt, int rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    MEVENT *mep = (MEVENT *)bp->data;       /* align an MEVENT struct */
    *mep = *evt;                            /*      & copy the data   */
    bp->type = MIDI_MSG;                    /* insert type and len    */
    bp->len = sizeof(int) * 2 + sizeof(MEVENT);

    if (CLsend(csound, rfd, (void *)bp, (size_t)bp->len) < 0) {
      csound->PerfError(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

/* send an M chnmsg to all remote fd's */
int MIDIGlob_msg(CSOUND *csound, MEVENT *evt)
{
    int nn;
    for (nn = 0; nn < ST(chnrfd_count); nn++) {
      if (MIDIsend_msg(csound, evt, ST(chnrfd_list)[nn]) == NOTOK)
        return NOTOK;
    }
    return OK;
}

int getRemoteInsRfd(CSOUND *csound, int insno)
{
    if (csound->remoteGlobals && ST(insrfd))
        return ST(insrfd)[insno];
    else return 0;
}

int getRemoteInsRfdCount(CSOUND *csound)
{
    if (csound->remoteGlobals)  return ST(insrfd_count);
    else return 0;
}

int getRemoteChnRfd(CSOUND *csound, int chan)
{
    if (csound->remoteGlobals && ST(chnrfd))
        return ST(chnrfd)[chan];
    else return 0;
}

int* getRemoteSocksIn(CSOUND *csound)
{
    if (csound->remoteGlobals)
       return ST(socksin);
    else return 0;
}

#else /* HAVE_SOCKETS not defined */

char remoteID(CSOUND *csound)
{
    return (char)0;
}

/* Cleanup the above; called from musmon csoundCleanup */
void remote_Cleanup(CSOUND *csound)
{
    csound->remoteGlobals = NULL;
    return;
}

int SVrecv(CSOUND *csound, int conn, void *data, int length)
{
    return 0;
}

/*  INSTR 0 opcodes  */

int insremot(CSOUND *csound, INSREMOT *p)
/* declare certain instrs for remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int insglobal(CSOUND *csound, INSGLOBAL *p)
/* declare certain instrs global remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int midremot(CSOUND *csound, MIDREMOT *p)
/* declare certain channels for remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int midglobal(CSOUND *csound, MIDGLOBAL *p)
/* declare certain chnls global remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

/*  MUSMON SERVICES  */

int insSendevt(CSOUND *csound, EVTBLK *evt, int rfd)
{
    return OK;
}

int insGlobevt(CSOUND *csound, EVTBLK *evt)
/* send an event to all remote fd's */
{
    return OK;
}

int MIDIsendevt(CSOUND *csound, MEVENT *evt, int rfd)
{
    return OK;
}

int MIDIGlobevt(CSOUND *csound, MEVENT *evt)
/* send an Mevent to all remote fd's */
{
    return OK;
}

int getRemoteInsRfd(CSOUND *csound, int insno)
{
    return 0;
}

int getRemoteInsRfdCount(CSOUND *csound)
{
    return 0;
}

int getRemoteChnRfd(CSOUND *csound, int chan)
{
    return 0;
}

int* getRemoteSocksIn(CSOUND *csound)
{
    return NULL;
}

#endif /* HAVE_SOCKETS */
