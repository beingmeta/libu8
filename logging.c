/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2014 beingmeta, inc.
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

#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include <stdarg.h>
#include <stdio.h> /* for sprintf */

int u8_log_show_date=0; /* If zero, show the date together with the time. */
int u8_log_show_procinfo=0; /* If zero, this overrides u8_log_show_threadinfo. */
int u8_log_show_elapsed=0; /* This tells displays to show elapsed time (fine grained). */
int u8_log_show_appid=1;
int u8_log_show_threadinfo=0;

u8_string u8_loglevels[11]={
  _("Emergency!!!"),
  _("Alert!!"),
  _("Critical"),
  _("Error"),
  _("Warning"),
  _("Note"),
  _("Info"),
  _("Debug"),
  _("Detail"),
  _("-deluge-"),
  NULL};

/* Generic logging */
/*  This assumes that it is linked with code that defines u8_logger. */

int u8_loglevel=U8_DEFAULT_LOGLEVEL;
int u8_stdout_loglevel=U8_DEFAULT_STDOUT_LOGLEVEL;
int u8_stderr_loglevel=U8_DEFAULT_STDERR_LOGLEVEL;
int u8_syslog_loglevel=U8_DEFAULT_SYSLOG_LOGLEVEL;

u8_string u8_logprefix="[";
u8_string u8_logsuffix="]\n";
u8_string u8_logindent=(u8_string)NULL;

U8_EXPORT u8_string u8_indent_text(u8_string input,u8_string indent);

U8_EXPORT void u8_set_logixes(u8_string pre, u8_string post)
{
  u8_logprefix=pre; u8_logsuffix=post;
}

U8_EXPORT void u8_set_logindent(u8_string indent)
{
  u8_logindent=indent;
}

static u8_logfn logfn=NULL;
U8_EXPORT int u8_default_logger(int loglevel,u8_condition c,u8_string message);

#if U8_WITH_STDIO
U8_EXPORT int u8_default_logger(int loglevel,u8_condition c,u8_string message)
{
  u8_byte buf[128]; u8_string prefix, indented=NULL;
  int eloglevel=loglevel; u8_string level;
  if (loglevel>U8_MAX_LOGLEVEL) {
    fprintf(stderr,"%s!! Logging call with invalid priority %d (%s)%s",
	    u8_logprefix,loglevel,c,u8_logsuffix);
    return 0;}
  else if (loglevel>u8_loglevel) return 0;
  else if (loglevel<0) eloglevel=(-loglevel);
  else {}
  level=((loglevel>=(-(U8_MAX_LOGLEVEL)))?
	 (u8_loglevels[eloglevel]):
	 ((u8_string)""));
  prefix=u8_message_prefix(buf,128);
  if ((u8_logindent)&&(u8_logindent[0])&&(strchr(message,'\n')))
    indented=u8_indent_text(message,u8_logindent);
  if (!(indented)) indented=message;
  if ((loglevel<0)||(eloglevel<=u8_stdout_loglevel)) {
    if ((c)&&(*level))
      fprintf(stdout,"%s%s %s (%s) %s%s",
	      u8_logprefix,prefix,level,c,indented,u8_logsuffix);
    else if (c)
      fprintf(stdout,"%s%s (%s) %s%s",
	      u8_logprefix,prefix,c,indented,u8_logsuffix);
    else if (*level)
      fprintf(stdout,"%s%s %s: %s%s",
	      u8_logprefix,prefix,level,indented,u8_logsuffix);
    else fprintf(stdout,"%s%s %s%s",
		 u8_logprefix,prefix,indented,u8_logsuffix);
    if ((indented)&&(indented!=message)) u8_free(indented);
    fflush(stdout);
    return 1;}
  if (eloglevel<=u8_stderr_loglevel) {
    if ((c)&&(*level))
      fprintf(stderr,"%s%s%s (%s): %s%s",
	      u8_logprefix,prefix,level,c,indented,u8_logsuffix);
    else if (c)
      fprintf(stdout,"%s%s (%s) %s%s",
	      u8_logprefix,prefix,c,indented,u8_logsuffix);
    else if (*level)
      fprintf(stderr,"%s%s %s: %s%s",
	      u8_logprefix,prefix,level,indented,u8_logsuffix);
    else fprintf(stderr,"%s%s %s%s",
		 u8_logprefix,prefix,indented,u8_logsuffix);}
  if ((indented)&&(indented!=message)) u8_free(indented);
  return 0;
}
#endif

U8_EXPORT int u8_logger(int loglevel,u8_condition c,u8_string msg)
{
  if (logfn) return logfn(loglevel,c,msg);
  else return u8_default_logger(loglevel,c,msg);
}

U8_EXPORT int u8_log(int loglevel,u8_condition c,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args; int retval;
  u8_byte msgbuf[512]; U8_INIT_OUTPUT_BUF(&out,512,msgbuf);
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  if (logfn) retval=logfn(loglevel,c,out.u8_outbuf);
  else retval=u8_default_logger(loglevel,c,out.u8_outbuf);
  if ((out.u8_streaminfo)&(U8_STREAM_OWNS_BUF))
    u8_free(out.u8_outbuf);
  return retval;
}

U8_EXPORT u8_logfn u8_set_logfn(u8_logfn newfn)
{
  u8_logfn oldfn=logfn;
  logfn=newfn;
  return oldfn;
}

U8_EXPORT int u8_message(u8_string format_string,...)
{
  u8_byte msgbuf[256];
  struct U8_OUTPUT out; va_list args; int retval;
  U8_INIT_OUTPUT_BUF(&out,256,msgbuf);
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  if (logfn) retval=logfn(-10,NULL,out.u8_outbuf);
  else retval=u8_default_logger(-10,NULL,out.u8_outbuf);
  if ((out.u8_streaminfo)&(U8_STREAM_OWNS_BUF)) u8_free(out.u8_outbuf);
  return retval;
}

/* Figuring out the prefix for log messages */

U8_EXPORT u8_string u8_message_prefix(u8_byte *buf,int buflen)
{
  time_t nowval=time(NULL);
  struct tm *now=localtime(&nowval);
  char clockbuf[64], timebuf[64], procbuf[128];
  u8_string appid=NULL, procid=procbuf;
  if (u8_log_show_date)
    strftime(clockbuf,32,"%H:%M:%S(%d%b%y)",now);
  else strftime(clockbuf,32,"%H:%M:%S",now);
  if (u8_log_show_elapsed) 
    sprintf(timebuf,"%s(%f)",clockbuf,u8_elapsed_time());
  else sprintf(timebuf,"%s",clockbuf);
  if (!(u8_log_show_procinfo)) {
    sprintf(buf,"%s",timebuf);
    return buf;}
if (u8_log_show_appid) appid=u8_appid();
#if (HAVE_GETPID)
  if (u8_log_show_threadinfo)
    procid=u8_procinfo(procbuf);
  else sprintf(procbuf,"%lu",(unsigned long)getpid());
#else
  if (u8_log_show_procinfo) sprintf(procbuf,"nopid");
#endif
  if ((appid!=NULL)&&((strlen(timebuf)+strlen(procid)+strlen(appid)+5)<buflen))
    sprintf(buf,"%s <%s:%s>",timebuf,appid,procid);
  else if ((u8_log_show_procinfo) &&
	   ((strlen(timebuf)+strlen(procid)+5)<buflen))
    sprintf(buf,"%s <%s>",timebuf,procid);
  else if ((strlen(timebuf)+2)<buflen)
    sprintf(buf,"%s ",timebuf);
  else return NULL;
  return buf;
}

/* Using syslog if it's there */

#if HAVE_SYSLOG
static u8_mutex logging_init_lock;

int u8_logging_initialized=0;

U8_EXPORT void u8_initialize_logging()
{
  u8_string app;
  u8_lock_mutex(&logging_init_lock);
  if (u8_logging_initialized) {
    u8_unlock_mutex(&logging_init_lock);
    return;}
  app=u8_appid();
  openlog(app,LOG_PID|LOG_CONS|LOG_NDELAY|LOG_PERROR,LOG_DAEMON);
  u8_logging_initialized=1;
  u8_unlock_mutex(&logging_init_lock);
  u8_register_source_file(_FILEINFO);
}
#else
U8_EXPORT void u8_initialize_logging()
{
  if (u8_logging_initialized) return;
  u8_logging_initialized=1;
  u8_register_source_file(_FILEINFO);
}
#endif





