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
   "SIG25", "SIG26", "SIG27", "SIG28", "SIG29", "SIG30", "SIG31", "SIG32",
   "SIG33", "SIG34", "SIG35", "SIG36", "SIG37", "SIG38", "SIG39", "SIG40",
   "SIG41", "SIG42", "SIG43", "SIG44", "SIG45", "SIG46", "SIG47", "SIG48",
   "SIG49", "SIG50", "SIG51", "SIG52", "SIG53", "SIG54", "SIG55", "SIG56",
   "SIG57", "SIG58", "SIG59", "SIG60", "SIG61", "SIG62", "SIG63", "SIG64",
   "SIG65", "SIG66", "SIG67", "SIG68", "SIG69", "SIG70", "SIG71", "SIG72",
   "SIG73", "SIG74", "SIG75", "SIG76", "SIG77", "SIG78", "SIG79", "SIG80",
   "SIG81", "SIG82", "SIG83", "SIG84", "SIG85", "SIG86", "SIG87", "SIG88",
   "SIG89", "SIG90", "SIG91", "SIG92", "SIG93", "SIG94", "SIG95", "SIG96",
   "SIG97", "SIG98", "SIG99", "SIG100", "SIG101", "SIG102", "SIG103", "SIG104",
   "SIG105", "SIG106", "SIG107", "SIG108", "SIG109", "SIG110", "SIG111", "SIG112",
   "SIG113", "SIG114", "SIG115", "SIG116", "SIG117", "SIG118", "SIG119", "SIG120",
   "SIG121", "SIG122", "SIG123", "SIG124", "SIG125", "SIG126", "SIG127"};

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
    if (signum<128)
      return signum_names[signum];
    else return u8_UnknownSignal;
  }
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
