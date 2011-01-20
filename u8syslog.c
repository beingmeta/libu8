/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2011 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#include "libu8/libu8.h"

static char versionid[] MAYBE_UNUSED=
  "$Id$";

#include "libu8/libu8io.h"

/* Direct syslog call */

U8_EXPORT void u8_use_syslog(int flag)
{
}

U8_EXPORT int syslog_logger(int priority,u8_condition c,u8_string message)
{
  u8_byte buf[512];
  if (priority>u8_loglevel) return 0;
  if (u8_syslog_initialized==0) u8_init_syslog();
  u8_string prefix=u8_message_prefix(buf,512);
  if ((prefix) && (c))
    syslog(priority,"%s (%s) %s",prefix,c,message);
  else if (prefix)
    syslog(priority,"%s -- %s",prefix,message);
  else if (c)
    syslog(priority,"(%s) %s",prefix,message);
  else syslog(priority,"%s",message);
  return 1;
}

U8_EXPORT void u8_syslog(int priority,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args; 
  U8_INIT_OUTPUT(&out,512);
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  syslog(priority,"%s",out.u8_outbuf);
  u8_free(out.u8_outbuf);
}

static void raisefn(u8_condition ex,u8_context cxt,u8_string details)
{
  U8_OUTPUT out;
  U8_INIT_OUTPUT(&out,512);
  if (details)
    if (cxt)
      u8_printf(&out,"Aborting due to %m@%s: %m",ex,cxt,details);
    else u8_printf(&out,"Aborting due to %m: %m",ex,details);  
  else if (cxt)
    u8_printf(&out,"Aborting due to %m@%s",ex,cxt);
  else u8_printf(&out,"Aborting due to %m",ex);
  if (u8_syslog_initialized==0) u8_init_syslog();
  syslog(LOG_ALERT,"%s",out.u8_outbuf);
  exit(1);
}

#if 0
static void message(u8_string msg)
{
  u8_byte buf[512];
  if (u8_syslog_initialized==0) u8_init_syslog();
  u8_string prefix=u8_message_prefix(buf,512);
  if (prefix)
    syslog(LOG_INFO,"%s %s",prefix,msg);
  else syslog(LOG_INFO,"%s",msg);
}

static void notice(u8_string msg)
{
  u8_byte buf[512];
  if (u8_syslog_initialized==0) u8_init_syslog();
  u8_string prefix=u8_message_prefix(buf,512);
  if (prefix)
    syslog(LOG_NOTICE,"%s %s",prefix,msg);
  else syslog(LOG_NOTICE,"%s",msg);
}

static void warn(u8_string msg)
{
  u8_byte buf[512];
  if (u8_syslog_initialized==0) u8_init_syslog();
  u8_string prefix=u8_message_prefix(buf,512);
  if (prefix)
    syslog(LOG_WARNING,"%s %s",prefix,msg);
  else syslog(LOG_WARNING,"%s",msg);
}
#endif

/* Initialization functions */

U8_EXPORT void u8_initialize_syslog()
{
  u8_string app=u8_appid();
  if (app==NULL) app="daemon";
  openlog(app,LOG_PID|LOG_CONS|LOG_NDELAY,LOG_DAEMON);
  u8_set_error_handler(raisefn);
  u8_set_logfn(syslog_logger);
}

