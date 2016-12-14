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

#ifndef LIBU8_U8SIGNALS_H
#define LIBU8_U8SIGNALS_H 1
#define LIBU8_U8SIGNALS_H_VERSION __FILE__

/** \file u8signals.h
    These functions support the transformation of signals into
      u8_raise calls.
 **/

#include <signal.h>

U8_EXPORT u8_condition u8_UnknownSignal,
  u8_SIGHUP,
  u8_SIGINT,
  u8_SIGQUIT,
  u8_SIGILL,
  u8_SIGABRT,
  u8_SIGFPE,
  u8_SIGKILL,
  u8_SIGSEGV,
  u8_SIGPIPE,
  u8_SIGALRM,
  u8_SIGTERM,
  u8_SIGUSR1,
  u8_SIGUSR2,
  u8_SIGCHLD,
  u8_SIGCONT,
  u8_SIGSTOP,
  u8_SIGTSTP,
  u8_SIGTTIN,
  u8_SIGTTOU,
  u8_SIGBUS,
  u8_SIGPOLL,
  u8_SIGPROF,
  u8_SIGSYS,
  u8_SIGTRAP,
  u8_SIGURG,
  u8_SIGVTALRM,
  u8_SIGXCPU,
  u8_SIGXFSZ,
  u8_SIGIOT,
  u8_SIGSTKFLT,
  u8_SIGIO,
  u8_SIGINFO,
  u8_SIGWINCH,
  u8_SIGUNUSED;

/** Gets a programmer-readable name for a signal
   @param signum an signal number as passed to a signal handler
   @returns a statically allocated u8_condition string
**/
U8_EXPORT u8_string u8_signal_name(int signum);

/** Gets a signal number for a name
   @param name the name of a signal (e.g. SIGSEGV)
   @returns a signal (1-31) or -1
**/
U8_EXPORT int u8_name2signal(u8_string name);

/** Raises a u8_condition based on a signal number.
    This is designed to be used as a signal handler with the
    signal() call. Note that it is usually preferred to use sigaction,
    where the handler is u8_siginfo_raise.
   @param signum an signal number as passed to a signal handler
   @returns void
   Raises an informative condition.
**/
U8_EXPORT void u8_signal_raise(int signum);

/** Raises a u8_condition based on received signal
    This is designed to be used as a signal handler with the
    sigaction() call.
   @param siginfo a siginfo_t structure with information about the signal
   @param ptr the void* pointer which passed to the sigaction handler
   @returns void
   Raises an informative condition.
**/
U8_EXPORT void u8_sigaction_raise(siginfo_t *info,void *ptr);

#endif /* ndef LIBU8_U8EXCEPTIONS_H */
