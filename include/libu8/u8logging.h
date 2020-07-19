/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
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
#endif

#ifndef LOG_EMERG
#define LOG_EMERG       0       /* system is unusable */
#endif

#ifndef LOG_ALERT
#define LOG_ALERT       1       /* action must be taken immediately */
#endif

#ifndef LOG_CRIT
#define LOG_CRIT        2       /* critical conditions */
#endif

#ifndef LOG_ERR
#define LOG_ERR         3       /* error conditions */
#endif

#ifndef LOG_WARNING
#define LOG_WARNING     4       /* warning conditions */
#endif

#ifndef LOG_NOTICE
#define LOG_NOTICE      5       /* normal but significant condition */
#endif

#ifndef LOG_INFO
#define LOG_INFO        6       /* informational */
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG       7       /* debug-level messages */
#endif

/* Our own little additions */
#ifndef LOG_DETAIL
#define LOG_DETAIL 8
#endif
#ifndef LOG_GLUT
#define LOG_GLUT 9
#endif
#ifndef LOG_DELUGE
#define LOG_DELUGE 10
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
#define LOGINFO LOG_INFO
#define LOG_DBG LOG_DEBUG
#define LOGDBG LOG_DEBUG
#define LOGDEBUG LOG_DEBUG
#define LOG_DETAILS LOG_DETAIL
#define LOGDETAILS LOG_DETAIL
#define LOGDETAIL LOG_DETAIL
#define LOG_DETAILED LOG_DETAIL
#define LOGGLUT LOG_GLUT
#define LOGDELUGE LOG_DELUGE

#define LOGF_WARN     (-(LOG_WARN))
#define LOGF_NOTICE   (-(LOG_NOTIFY))
#define LOGF_NOTIFY   (-(LOG_NOTIFY))
#define LOGF_ERR      (-(LOG_ERR))
#define LOGF_ERROR    (-(LOG_ERR))
#define LOGF_CRIT     (-(LOG_CRIT))
#define LOGF_EMERG    (-(LOG_EMERG))
#define LOGF_INFO     (-(LOG_INFO))
#define LOGF_DEBUG    (-(LOG_DEBUG))
#define LOGF_DETAILS  (-(LOG_DETAILS))
#define LOGF_DELUGE   (-(LOG_DELUGE))
#define LOGF_GLUT     (-(LOG_GLUT))

/* With U8 prefixes, just in case */
#define U8_LOG_WARNING LOG_WARNING
#define U8_LOG_WARN LOG_WARNING
#define U8_LOGWARN LOG_WARNING
#define U8_LOG_NOTICE LOG_NOTICE
#define U8_LOG_NOTIFY LOG_NOTICE
#define U8_LOG_NOTIFY LOG_NOTICE
#define U8_LOGNOTIFY LOG_NOTICE
#define U8_LOGNOTICE LOG_NOTICE
#define U8_LOG_ERR LOG_ERR
#define U8_LOG_ERROR LOG_ERR
#define U8_LOGERR LOG_ERR
#define U8_LOG_CRIT LOG_CRIT
#define U8_LOG_CRITICAL LOG_CRIT
#define U8_LOGCRIT LOG_CRIT
#define U8_LOG_EMERG LOG_EMERG
#define U8_LOG_EMERGENCY LOG_EMERG
#define U8_LOG_PANIC LOG_EMERG
#define U8_LOGPANIC LOG_EMERG
#define U8_LOG_DEBUG LOG_DEBUG
#define U8_LOG_DBG LOG_DEBUG
#define U8_LOGDBG LOG_DEBUG
#define U8_LOGDEBUG LOG_DEBUG
#define U8_LOG_DETAIL LOG_DETAIL
#define U8_LOG_DETAILS LOG_DETAIL
#define U8_LOGDETAILS LOG_DETAIL
#define U8_LOGDETAIL LOG_DETAIL
#define U8_LOG_DETAILED LOG_DETAIL
#define U8_LOG_GLUT LOG_GLUT
#define U8_LOGGLUT LOG_GLUT
#define U8_LOG_DELUGE LOG_DELUGE
#define U8_LOGDELUGE LOG_DELUGE

typedef int (*u8_logtestfn)(int loglevel,u8_condition c);

U8_EXPORT u8_logtestfn u8_logbreakp;

#define U8_MAX_LOGLEVEL 11

/* This is negative, so it's always output, and it's got a loglevel
   name of NULL, so no loglevel appears. */
#define U8_LOG_MSG (-11)

/* This is defined to change the effective log level for u8_logf */
#ifndef U8_LOGLEVEL
#define U8_LOGLEVEL u8_loglevel
#endif

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
U8_EXPORT int u8_log_show_date, u8_log_show_elapsed, u8_log_show_appid;
U8_EXPORT int u8_log_show_procinfo, u8_log_show_threadinfo;
/* Whether or not syslog has been initialized. */
U8_EXPORT int u8_logging_initialized;

U8_EXPORT u8_string u8_logprefix, u8_logsuffix, u8_logindent;

typedef int (*u8_logfn)(int loglevel,u8_condition condition,u8_string message);
typedef int (*u8_log_callback)(int loglevel,u8_condition condition,
			       u8_string message,
			       void *data);

U8_EXPORT void u8_bind_logfn(u8_logfn f);

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

/** Possibly formats a log message for an (optional) condition.  This
     macro uses u8_log so the destination of any output will depend on
     how the program is linked and configured. This macro uses the CPP
     definition of U8_LOGLEVEL to determine whether to call
     u8_log(). To specify a log level setting in a C file, define
     U8_LOGLEVEL to an expression to be used as the current loglevel.
   @param priority an (int [-1,7]) priority level
   @param condition a string describing the condition (possibly NULL)
   @param format_string a utf-8 printf-like format string
   @param ... arguments for the format string
   @returns 1 if the call actually produced output somewhere
**/
#define u8_logf(priority,condition,format_string,...) \
  ( ( (priority) <= U8_LOGLEVEL ) ? \
    (u8_log(-(priority),condition,format_string, ##__VA_ARGS__)) : \
    (0) )

/** A macro which expands int a loglevel defaulting to u8_loglevel.
   @param level an integer describing a loglevel or -1
   @returns the loglevel to use, which is u8_loglevel if the argument is <= 0
**/
#define u8_getloglevel(level) ( ( (level) > 0) ? (level) : (u8_loglevel) )
#define u8_merge_loglevels(local,module) \
  ( ( (local) > 0) ? (local) : (module > 0) ? (module) : (u8_loglevel) )

/** Sets the function used for log messages
   @param logfn
   @returns the previous logfn
**/
U8_EXPORT u8_logfn u8_set_logfn(u8_logfn logfn);

/** Sets the function and data arg used for log messages
   @param logfn a function for handling log messages
   @param logdata void * data pointr to pass the *logfn*
   @returns the previous logger
**/
U8_EXPORT u8_log_callback u8_set_logger(u8_log_callback logfn,void* logdata);

/** Sets the prefix and suffix string for non-syslog log messages
   @param prefix
   @param suffix
   @returns void
**/
U8_EXPORT void u8_set_logixes(u8_string prefix, u8_string suffix);

/** Sets the indentation string for log messages
   @param indent a string to be used as the indentation for log messages
   @returns void
**/
U8_EXPORT void u8_set_logindent(u8_string indent);

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

/* Called when a log message is either over the set loglevel or
   matches one of the watched conditions. This is a good place to set
   a debugger breakpoint.
   @param loglevel the loglevel of the log call
   @param condition the condition for the log call
   @returns void
*/
U8_EXPORT void u8_log_break(int loglevel,u8_condition c);

/* Getting the logger context */

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_log_context_key;
#define u8_log_context ((u8_string)(u8_tld_get(u8_log_context_key)))
#define u8_set_log_context(cxt) \
  u8_tld_set(u8_log_context_key,((void *)cxt))
#elif (U8_USE__THREAD)
U8_EXPORT __thread u8_string u8_log_context;
#define u8_set_log_context(cxt) u8_log_context=cxt
#else
U8_EXPORT u8_string u8_log_context;
#define u8_set_log_context(ex) u8_log_context=cxt
#endif



#endif /* ndef LIBU8_U8LOGGING_H */
