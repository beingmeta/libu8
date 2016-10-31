/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2016 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#include "libu8/u8source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8exceptions.h"
#include "libu8/u8signals.h"
#include <signal.h>

u8_condition u8_UnknownSignal=_("Unknown signal"),
  u8_SIGHUP=_("SIGHUP"),
  u8_SIGINT=_("SIGINT (keyboard interrupt)"),
  u8_SIGQUIT=_("SIGQUIT (keyboard quit)"),
  u8_SIGILL=_("SIGILL (illegal instruction)"),
  u8_SIGABRT=_("SIGABRT"),
  u8_SIGFPE=_("SIGFPE"),
  u8_SIGKILL=_("SIGKILL"),
  u8_SIGSEGV=_("SIGSEGV"),
  u8_SIGPIPE=_("SIGPIPE"),
  u8_SIGALRM=_("SIGALRM"),
  u8_SIGTERM=_("SIGTERM"),
  u8_SIGUSR1=_("SIGUSR1"),
  u8_SIGUSR2=_("SIGUSR2"),
  u8_SIGCHLD=_("SIGCHLD"),
  u8_SIGCONT=_("SIGCONT"),
  u8_SIGSTOP=_("SIGSTOP"),
  u8_SIGTSTP=_("SIGTSTP"),
  u8_SIGTTIN=_("SIGTTIN (background input)"),
  u8_SIGTTOU=_("SIGTTOU (background output)"),
  u8_SIGBUS=_("SIGBUS (bus error)"),
  u8_SIGPOLL=_("SIGPOLL"),
  u8_SIGPROF=_("SIGPROF"),
  u8_SIGSYS=_("SIGSYS (bad argument)"),
  u8_SIGTRAP=_("SIGTRAP"),
  u8_SIGURG=_("SIGURG (socket condition)"),
  u8_SIGVTALRM=_("SIGVTALRM (virtual alarm)"),
  u8_SIGXCPU=_("SIGXCPU"),
  u8_SIGXFSZ=_("SIGXFSZ"),
  u8_SIGIOT=_("SIGIOT (~SIGABRT)"),
  u8_SIGEMT=_("SIGEMT"),
  u8_SIGSTKFLT=_("SIGSTKFLT"),
  u8_SIGIO=_("SIGIO (~SIGPOLL)"),
  u8_SIGCLD=_("SIGCLD (~SIGCHLD)"),
  u8_SIGPWR=_("SIGPWR"),
  u8_SIGINFO=_("SIGINFO (~SIGPWR)"),
  u8_SIGLOST=_("SIGLOST (file lock lost)"),
  u8_SIGWINCH=_("SIGWINCH (window changed)"),
  u8_SIGUNUSED=_("SIGUNUSED");


static u8_condition signum_names[]=
  {"SIG0", "SIG1", "SIG2", "SIG3", "SIG4", "SIG5", "SIG6", "SIG7", "SIG8",
   "SIG9", "SIG10", "SIG11", "SIG12", "SIG13", "SIG14", "SIG15", "SIG16", 
   "SIG17", "SIG18", "SIG19", "SIG20", "SIG21", "SIG22", "SIG23", "SIG24",
   "SIG25", "SIG26", "SIG27", "SIG28", "SIG29", "SIG30", "SIG31", "SIG32"};

U8_EXPORT u8_string u8_signal_name(int signum)
{
  switch (signum) {
  case SIGHUP: return u8_SIGHUP;
  case SIGINT: return u8_SIGINT;
  case SIGQUIT: return u8_SIGQUIT;
  case SIGILL: return u8_SIGILL;
  case SIGABRT: return u8_SIGABRT;
  case SIGFPE: return u8_SIGFPE;
  case SIGKILL: return u8_SIGKILL;
  case SIGSEGV: return u8_SIGSEGV;
  case SIGPIPE: return u8_SIGPIPE;
  case SIGALRM: return u8_SIGALRM;
  case SIGTERM: return u8_SIGTERM;
  case SIGUSR1: return u8_SIGUSR1;
  case SIGUSR2: return u8_SIGUSR2;
  case SIGCHLD: return u8_SIGCHLD;
  case SIGCONT: return u8_SIGCONT;
  case SIGSTOP: return u8_SIGSTOP;
  case SIGTSTP: return u8_SIGTSTP;
  case SIGTTIN: return u8_SIGTTIN;
  case SIGTTOU: return u8_SIGTTOU;
#if SIGBUS
  case SIGBUS: return u8_SIGBUS;
#endif
#if SIGPOLL
  case SIGPOLL: return u8_SIGPOLL;
#endif
#if SIGPROF
  case SIGPROF: return u8_SIGPROF;
#endif
#if SIGSYS
  case SIGSYS: return u8_SIGSYS;
#endif
#if SIGTRAP
  case SIGTRAP: return u8_SIGTRAP;
#endif
#if SIGURG
  case SIGURG: return u8_SIGURG;
#endif
#if SIGVTALRM
  case SIGVTALRM: return u8_SIGVTALRM;
#endif
#if SIGXCPU
  case SIGXCPU: return u8_SIGXCPU;
#endif
#if SIGXFSZ
  case SIGXFSZ: return u8_SIGXFSZ;
#endif
#if (defined(SIGIOT) && (SIGIOT!=SIGABRT))
  case SIGIOT: return u8_SIGIOT;
#endif
#if SIGEMT
  case SIGEMT: return u8_SIGEMT;
#endif
#if SIGSTKFLT
  case SIGSTKFLT: return u8_SIGSTKFLT;
#endif
#if (defined(SIGIO) && (SIGIO!=SIGPOLL))
  case SIGIO: return u8_SIGIO;
#endif
#if (defined(SIGCLD) && (SIGCLD!=SIGCHLD))
  case SIGCLD: return u8_SIGCLD;
#endif
#if SIGPWR
  case SIGPWR: return u8_SIGPWR;
#endif
#if SIGINFO
  case SIGINFO: return u8_SIGINFO;
#endif
#if SIGLOST
  case SIGLOST: return u8_SIGLOST;
#endif
#if SIGWINCH
  case SIGWINCH: return u8_SIGWINCH;
#endif
#if (defined(SIGUNUSED) && (SIGUNUSED!=SIGSYS))
  case SIGUNUSED: return u8_SIGUNUSED;
#endif
  default:
    if (signum<32)
      return signum_names[signum];
    else return u8_UnknownSignal;
  }
}

U8_EXPORT int u8_name2signal(u8_string name)
{
  int off=0;
  if (strncasecmp(name,"sig",3)==0) off=3;
  if (strcasecmp(name+off,"HUP")==0)
    return SIGHUP;
  else if (strcasecmp(name+off,"INT")==0)
    return SIGINT;
  else if (strcasecmp(name+off,"QUIT")==0)
    return SIGQUIT;
  else if (strcasecmp(name+off,"ILL")==0)
    return SIGILL;
  else if (strcasecmp(name+off,"ABRT")==0)
    return SIGABRT;
  else if (strcasecmp(name+off,"FPE")==0)
    return SIGFPE;
  else if (strcasecmp(name+off,"KILL")==0)
    return SIGKILL;
  else if (strcasecmp(name+off,"SEGV")==0)
    return SIGSEGV;
  else if (strcasecmp(name+off,"PIPE")==0)
    return SIGPIPE;
  else if (strcasecmp(name+off,"ALRM")==0)
    return SIGALRM;
  else if (strcasecmp(name+off,"TERM")==0)
    return SIGTERM;
  else if (strcasecmp(name+off,"USR1")==0)
    return SIGUSR1;
  else if (strcasecmp(name+off,"USR2")==0)
    return SIGUSR2;
  else if (strcasecmp(name+off,"CHLD")==0)
    return SIGCHLD;
  else if (strcasecmp(name+off,"CONT")==0)
    return SIGCONT;
  else if (strcasecmp(name+off,"STOP")==0)
    return SIGSTOP;
  else if (strcasecmp(name+off,"TSTP")==0)
    return SIGTSTP;
  else if (strcasecmp(name+off,"TTIN")==0)
    return SIGTTIN;
  else if (strcasecmp(name+off,"TTOU")==0)
    return SIGTTOU;
#ifdef SIGBUS
  else if (strcasecmp(name+off,"BUS")==0)
    return SIGBUS;
#endif
#ifdef SIGPOLL
  else if (strcasecmp(name+off,"POLL")==0)
    return SIGPOLL;
#endif
#ifdef SIGPROF
  else if (strcasecmp(name+off,"PROF")==0)
    return SIGPROF;
#endif
#ifdef SIGSYS
  else if (strcasecmp(name+off,"SYS")==0)
    return SIGSYS;
#endif
#ifdef SIGTRAP
  else if (strcasecmp(name+off,"TRAP")==0)
    return SIGTRAP;
#endif
#ifdef SIGURG
  else if (strcasecmp(name+off,"URG")==0)
    return SIGURG;
#endif
#ifdef SIGVTALRM
  else if (strcasecmp(name+off,"VTALRM")==0)
    return SIGVTALRM;
#endif
#ifdef SIGXCPU
  else if (strcasecmp(name+off,"XCPU")==0)
    return SIGXCPU;
#endif
#ifdef SIGXFSZ
  else if (strcasecmp(name+off,"XFSZ")==0)
    return SIGXFSZ;
#endif
#if (defined(SIGIOT) && (SIGIOT!=SIGABRT))
  else if (strcasecmp(name+off,"IOT")==0)
    return SIGIOT;
#endif
#ifdef SIGEMT
  else if (strcasecmp(name+off,"EMT")==0)
    return SIGEMT;
#endif
#ifdef SIGSTKFLT
  else if (strcasecmp(name+off,"STKFLT")==0)
    return SIGSTKFLT;
#endif
#if (defined(SIGIO) && (SIGIO!=SIGPOLL))
  else if (strcasecmp(name+off,"IO")==0)
    return SIGIO;
#endif
#if (defined(SIGCLD) && (SIGCLD!=SIGCHLD))
  else if (strcasecmp(name+off,"CLD")==0)
    return SIGCLD;
#endif
#ifdef SIGPWR
  else if (strcasecmp(name+off,"PWR")==0)
    return SIGPWR;
#endif
#ifdef SIGINFO
  else if (strcasecmp(name+off,"INFO")==0)
    return SIGINFO;
#endif
#ifdef SIGLOST
  else if (strcasecmp(name+off,"LOST")==0)
    return SIGLOST;
#endif
#ifdef SIGWINCH
  else if (strcasecmp(name+off,"WINCH")==0)
    return SIGWINCH;
#endif
#if (defined(SIGUNUSED) && (SIGUNUSED!=SIGSYS))
  else if (strcasecmp(name+off,"UNUSED")==0)
    return SIGUNUSED;
#endif
  else return -1;
}

U8_EXPORT void u8_signal_raise(int signum)
{
  u8_raise(u8_signal_name(signum),NULL,NULL);
}

U8_EXPORT void u8_sigaction_raise(siginfo_t *info,void *ptr)
{
  u8_raise(u8_signal_name(info->si_signo),u8_strerror(info->si_errno),NULL);
}

/* Initialization */

U8_EXPORT void u8_init_signals_c()
{
  u8_register_source_file(_FILEINFO);
}
