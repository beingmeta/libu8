/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2013 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8LOGGING_H
#define LIBU8_U8LOGGING_H 1
#define LIBU8_U8LOGGING_H_VERSION __FILE__

/** \file u8logging.h
    These functions provide various ways of logging program events.
 **/

#if HAVE_SYSLOG
#include <syslog.h>
#else
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

/* Our own little addition */
#ifndef LOG_DETAIL
#define LOG_DETAIL 8
#endif
#ifndef LOG_DELUGE
#define LOG_DELUGE 9
#endif

/* Common mistypes/misremembers/mnemonics, might as well define them */
#define LOG_WARN LOG_WARNING
#define LOGWARN LOG_WARNING
#define LOG_NOTIFY LOG_NOTICE
#define LOGNOTIFY LOG_NOTICE
#define LOGNOTICE LOG_NOTICE
#define LOG_ERROR LOG_ERR
#define LOGERR LOG_ERR
#define LOG_CRITICAL LOG_CRIT
#define LOGCRIT LOG_CRIT
#define LOG_EMERGENCY LOG_EMERG
#define LOG_PANIC LOG_EMERG
#define LOGPANIC LOG_EMERG
#define LOG_DBG LOG_DEBUG
#define LOGDBG LOG_DEBUG
#define LOGDEBUG LOG_DEBUG
#define LOG_DETAILS LOG_DETAIL
#define LOGDETAILS LOG_DETAIL
#define LOGDETAIL LOG_DETAIL
#define LOG_DETAILED LOG_DETAIL
#define LOGDELUGE LOG_DELUGE

#define U8_MAX_LOGLEVEL 9

/* This is the overall log level and messages with with priorities
   above this should not produced output. */
U8_EXPORT int u8_loglevel, u8_logging_initialized;
#define U8_DEFAULT_LOGLEVEL LOG_NOTICE

/* When logging to stdio, these dtermine which errors go to
   the stdout, which go to stderr, and which go to syslog.
   Messages can go to multiple destinations.
   Note that these thresholds are applied after u8_loglevel so,
   for example, debug messages won't go to the standard output
   unless the overall loglevel is lower than debug.
*/
U8_EXPORT int u8_stdout_loglevel, u8_stderr_loglevel, u8_syslog_loglevel;
#define U8_DEFAULT_STDOUT_LOGLEVEL LOG_DEBUG
#define U8_DEFAULT_STDERR_LOGLEVEL LOG_ERROR
#define U8_DEFAULT_SYSLOG_LOGLEVEL LOG_CRIT

U8_EXPORT u8_string u8_loglevels[];

/* This determines what context is delivered along with messages.  Clock
   time is always shown. */
U8_EXPORT int u8_log_show_date, u8_log_show_elapsed;
U8_EXPORT int u8_log_show_procinfo, u8_log_show_threadinfo;
/* Whether or not syslog has been initialized. */
U8_EXPORT int u8_logging_initialized;

U8_EXPORT u8_string u8_logprefix, u8_logsuffix;

typedef int (*u8_logfn)(int loglevel,u8_condition condition,u8_string message);

/** Possibly generates a log message for an (optional) condition.
     Whether and where this produces output depends on how the
      program is linked and configured.
   @param priority an (int [-1,7]) priority level
   @param c a string describing the condition (possibly NULL)
   @param message the content of the message to be emmited
   @returns 1 if the call actually produced output somewhere
**/      
U8_EXPORT int u8_logger(int priority,u8_condition c,u8_string message);

/** Possibly formats a log message for an (optional) condition.
     Whether and where this produces output depends on how the
      program is linked and configured.
   @param priority an (int [-1,7]) priority level
   @param c a string describing the condition (possibly NULL)
   @param format_string a utf-8 printf-like format string
   @param ... arguments for the format string
   @returns 1 if the call actually produced output somewhere
**/      
U8_EXPORT int u8_log(int priority,u8_condition c,u8_string format_string,...);
U8_EXPORT int u8_message(u8_string format_string,...);

/** Sets the function used for log messages
   @param logfn
   @returns the previous logfn
**/      
U8_EXPORT u8_logfn u8_set_logfn(u8_logfn);

/** Sets the prefix and suffix string for non-syslog log messages
   @param prefix
   @param suffix
   @returns void
**/      
U8_EXPORT void u8_set_logixes(u8_string prefix, u8_string suffix);

/** Generates a message prefix into the given buffer, 
     with output including process information controlled
     by the various u8_log_show variables.
    If buflen doesn't allow all the information to be shown,
     some information may be removed.
   @param buf a byte buffer in which to compose the prefix
   @param buflen the number of bytes in the buffer
   @returns a pointer to the buffer it was given, filled
      with context information and null terminated.
**/
U8_EXPORT u8_string u8_message_prefix(u8_byte *buf,int buflen);

/** Formats and outputs a string using syslog and the given priority level.
     This does its output if u8_loglevel is greater than 0.
   @param priority an (int) priority level passed to syslog
   @param format_string a utf-8 printf-like format string
   @param ... arguments for the format string
   @returns void
**/
U8_EXPORT void u8_syslog(int priority,u8_string format_string,...);

/** Control whether syslog is used together with stdio output.
     If flag is 1, syslog is used for notifications and warnings, together
      with the stderr.
    @param flag int
**/
U8_EXPORT void u8_use_syslog(int flag);

#endif /* ndef LIBU8_U8LOGGING_H */
