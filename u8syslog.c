/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 beingmeta, LLC
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

#include "libu8/libu8io.h"

/* Direct syslog call */

U8_EXPORT int syslog_logger(int priority,u8_condition c,u8_string message)
{
  u8_byte buf[512];
  unsigned int epriority = (priority < 0) ? (-priority) : (priority);
  u8_string context = u8_log_context;
  if (u8_logger_initialized==0) u8_init_logger();
  if (priority > u8_loglevel)
    return 0;
  else NO_ELSE;
  if (epriority <= u8_syslog_loglevel)
    return 0;

  u8_string prefix=u8_message_prefix(buf,512);
  if (prefix==NULL) prefix="";
  if ((c)&&(context))
    syslog(priority,"%s (%s) %s %s%s%s",
           prefix,context,c,message,
           (u8_log_context) ? ("\n\t@ ") : (""),
           (u8_log_context) ? (u8_log_context) : (U8S("")));
  else if (c)
    syslog(priority,"%s (%s) %s%s%s",prefix,c,message,
           (u8_log_context) ? ("\n\t@ ") : (""),
           (u8_log_context) ? (u8_log_context) : (U8S("")));
  else if (prefix)
    syslog(priority,"%s -- %s%s%s",prefix,message,
           (u8_log_context) ? ("\n\t@ ") : (""),
           (u8_log_context) ? (u8_log_context) : (U8S("")));
  else if (c)
    syslog(priority,"(%s) %s%s%s",prefix,message,
           (u8_log_context) ? ("\n\t@ ") : (""),
           (u8_log_context) ? (u8_log_context) : (U8S("")));
  else syslog(priority,"%s%s%s",message,
              (u8_log_context) ? ("\n\t@ ") : (""),
              (u8_log_context) ? (u8_log_context) : (U8S("")));
  return 1;
}

U8_EXPORT void u8_use_syslog(int flag)
{
  if (flag)
    u8_set_logfn(syslog_logger);
  else u8_set_logfn(NULL);
}

U8_EXPORT void u8_syslog(int priority,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args;
  int epriority=priority;
  if (priority<0) epriority=(-priority)-2;
  if (epriority > LOG_DEBUG) epriority=LOG_DEBUG;
  U8_INIT_STATIC_OUTPUT(out,512);
  out.u8_streaminfo |= U8_HUMAN_OUTPUT;
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  syslog(epriority,"%s",out.u8_outbuf);
  u8_free(out.u8_outbuf);
}

static void raisefn(u8_condition ex,u8_context cxt,u8_string details)
{
  U8_OUTPUT out;
  U8_INIT_STATIC_OUTPUT(out,512);
  out.u8_streaminfo |= U8_HUMAN_OUTPUT;
  if (details)
    if (cxt)
      u8_printf(&out,"Aborting due to %m@%s: %m",ex,cxt,details);
    else u8_printf(&out,"Aborting due to %m: %m",ex,details);
  else if (cxt)
    u8_printf(&out,"Aborting due to %m@%s",ex,cxt);
  else u8_printf(&out,"Aborting due to %m",ex);
  if (u8_logger_initialized==0) u8_init_logger();
  syslog(LOG_ALERT,"%s",out.u8_outbuf);
  exit(1);
}

/* Initialization functions */

U8_EXPORT void u8_initialize_syslog()
{
  u8_string app=u8_appid();
  if (app==NULL) app="daemon";
  openlog(app,LOG_PID|LOG_CONS|LOG_NDELAY,LOG_DAEMON);
  u8_set_error_handler(raisefn);
  u8_set_logfn(syslog_logger);
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
