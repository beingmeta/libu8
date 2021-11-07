/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2021 beingmeta, LLC
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
#include "libu8/u8elapsed.h"

#include <ctype.h>

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include <stdarg.h>
#if U8_WITH_STDIO
#include <stdio.h>
#endif

#if (U8_USE_TLS)
u8_tld_key u8_log_context_key;
#elif (U8_USE__THREAD)
__thread u8_string u8_log_context=NULL;
#else
u8_string u8_log_context=NULL;
#endif

int u8_log_show_date=0; /* If zero, show the date together with the time. */
int u8_log_show_procinfo=0;
int u8_log_show_threadinfo=0;
int u8_log_show_elapsed=0; /* This tells displays to show elapsed time (fine grained). */
int u8_log_show_appid=1;

u8_string u8_loglevels[12]={
  _("Emergency!!!"),
  _("Alert!!"),
  _("Critical!"),
  _("Error"),
  _("Warning"),
  _("Notice"),
  _("Info"),
  _("Debug"),
  _("Detail"),
  _("Glut"),
  _("Deluge"),
  NULL};

/* Generic logging */
/*  This assumes that it is linked with code that defines u8_logger. */

int u8_loglevel=U8_DEFAULT_LOGLEVEL;
int u8_stdout_loglevel=U8_DEFAULT_STDOUT_LOGLEVEL;
int u8_stderr_loglevel=U8_DEFAULT_STDERR_LOGLEVEL;
int u8_syslog_loglevel=U8_DEFAULT_SYSLOG_LOGLEVEL;

int u8_breakpoint_loglevel=-1;
u8_logtestfn u8_logbreakp=NULL;

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

static u8_log_callback logger=NULL;
static void *logger_data=NULL;

#define strmatch(s,v) (strcasecmp(s,v)==0)

U8_EXPORT int u8_get_loglevel(u8_string spec)
{
  if (spec == NULL) return -1;
  else if (isdigit(spec[0]))
    return strtol(spec,NULL,10);
  else if (strncasecmp(spec,"log_",4)==0)
    spec=spec+4;
  else NO_ELSE;
  if ( (strmatch(spec,"swamp")) || (strmatch(spec,"deluge")) ||
       (strmatch(spec,"verydetailed")) )
    return 9;
  else if ( (strmatch(spec,"details")) || (strmatch(spec,"detail")) ||
            (strmatch(spec,"detailed")) )
    return 8;
  else if ( (strmatch(spec,"dbg")) || (strmatch(spec,"debug")) )
    return 7;
  else if ( (strmatch(spec,"info")) || (strmatch(spec,"status")) ||
            (strmatch(spec,"information")) )
    return 6;
  else if ( (strmatch(spec,"notice")) || (strmatch(spec,"notify")) )
    return 5;
  else if ( (strmatch(spec,"warning")) || (strmatch(spec,"warn")) )
    return 4;
  else if ( (strmatch(spec,"error")) || (strmatch(spec,"err")) )
    return 3;
  else if ( (strmatch(spec,"critical")) || (strmatch(spec,"crit")) ||
            (strmatch(spec,"danger")) )
    return 2;
  else if  ( (strmatch(spec,"alert")) || (strmatch(spec,"attention")) )
    return 1;
  else if  ( (strmatch(spec,"panic")) || (strmatch(spec,"emerg")) ||
             (strmatch(spec,"emergency")) )
    return 0;
  else return -1;
}

#if U8_WITH_STDIO
static void do_output(FILE *out,u8_string prefix,
		      u8_string level,u8_condition c,
		      u8_string body)
{
  if (prefix == NULL) prefix = "";
  if ((c) && (level))
    fprintf(out,"%s%s %s (%s) %s%s",
	    u8_logprefix,prefix,level,c,body,u8_logsuffix);
  else if (c)
    fprintf(out,"%s%s %s %s%s",u8_logprefix,prefix,c,body,u8_logsuffix);
  else if (level)
    fprintf(out,"%s%s %s %s%s",u8_logprefix,prefix,level,body,u8_logsuffix);
  else fprintf(out,"%s%s %s%s",u8_logprefix,prefix,body,u8_logsuffix);

}

U8_EXPORT int u8_default_logger(int priority,u8_condition c,u8_string message)
{
  u8_byte buf[128]; u8_string prefix, indented=NULL;
  /* We provide a way for callers to force the message to be output
   while specifying a descriptive loglevel. In particular, if the log
   level is negative, it is always output using an effective loglevel
   which is the positive complement of the negative loglevel. */
  unsigned int rv = 0, epriority = (priority < 0) ? (-priority) : (priority);
  u8_string level = (epriority < U8_MAX_LOGLEVEL) ?
    (u8_loglevels[epriority]) :
    (NULL);
  if (priority > U8_MAX_LOGLEVEL) {
    fprintf(stderr,"%s!! Logging call with invalid priority %d (%s)%s",
            u8_logprefix,priority,c,u8_logsuffix);
    return 0;}
  else if (priority > u8_loglevel) return 0;
  else NO_ELSE;;
  prefix=u8_message_prefix(buf,128);
  if ((u8_logindent)&&(u8_logindent[0])&&(strchr(message,'\n')))
    indented=u8_indent_text(message,u8_logindent);
  if (!(indented)) indented=message;
  if ((priority<0)||(epriority<=u8_stderr_loglevel)) {
    do_output(stderr,prefix,level,c,indented); rv=1;}
  if (epriority<=u8_stdout_loglevel) {
    do_output(stdout,prefix,level,c,indented); rv=1;}
  if ((indented)&&(indented!=message)) u8_free(indented);
  return rv;
}
#endif

U8_EXPORT int u8_logger(int priority,u8_condition c,u8_string msg)
{
  if (logger)
    return logger(priority,c,msg,logger_data);
  else if (logfn)
    return logfn(priority,c,msg);
  else return u8_default_logger(priority,c,msg);
}

U8_EXPORT int u8_log(int priority,u8_condition c,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args; int retval;
  u8_byte msgbuf[1000];
  U8_INIT_STATIC_OUTPUT_BUF(out,1000,msgbuf);
  out.u8_streaminfo |= U8_HUMAN_OUTPUT;
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  if (logger)
    retval=logger(priority,c,out.u8_outbuf,logger_data);
  else if (logfn)
    retval=logfn(priority,c,out.u8_outbuf);
  else retval=u8_default_logger(priority,c,out.u8_outbuf);
  if ((out.u8_streaminfo)&(U8_STREAM_OWNS_BUF))
    u8_free(out.u8_outbuf);
  if ( ( (u8_breakpoint_loglevel>=0) &&
	 ( ( (priority>=0) && (priority <= u8_breakpoint_loglevel) ) ||
	   ( (priority<0) && (-priority <= u8_breakpoint_loglevel) ) ) ) ||
       ( (u8_logbreakp) && (u8_logbreakp(priority,c)) ) )
    u8_log_break(priority,c);
  return retval;
}

U8_EXPORT u8_logfn u8_set_logfn(u8_logfn newfn)
{
  u8_logfn oldfn=logfn;
  logfn=newfn;
  return oldfn;
}

U8_EXPORT u8_log_callback u8_set_logger(u8_log_callback newfn,void *newdata)
{
  u8_log_callback oldfn=logger;
  logger=newfn;
  logger_data=newdata;
  return oldfn;
}

U8_EXPORT int u8_message(u8_string format_string,...)
{
  u8_byte msgbuf[256];
  struct U8_OUTPUT out; va_list args; int retval;
  U8_INIT_STATIC_OUTPUT_BUF(out,256,msgbuf);
  out.u8_streaminfo |= U8_HUMAN_OUTPUT;
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  if (logger)
    retval=logger(U8_LOG_MSG,NULL,out.u8_outbuf,logger_data);
  else if (logfn)
    retval=logfn(U8_LOG_MSG,NULL,out.u8_outbuf);
  else retval=u8_default_logger(U8_LOG_MSG,NULL,out.u8_outbuf);
  if ((out.u8_streaminfo)&(U8_STREAM_OWNS_BUF)) u8_free(out.u8_outbuf);
  return retval;
}

U8_EXPORT void u8_log_break(int loglevel,u8_condition c)
{
  int v=2;
  /* This is a good place to set a breakpoint */
  v = v*2;
}

/* Figuring out the prefix for log messages */

U8_EXPORT u8_string u8_message_prefix(u8_byte *buf,int buflen)
{
  time_t now_t=time(NULL);
  char clockbuf[64], timebuf[128], procbuf[128];
  u8_string appid=NULL, procid=procbuf;
#if HAVE_LOCALTIME_R
  struct tm _now, *now=&_now;
  u8_zero_struct(_now); now=localtime_r(&now_t,&_now);
#else
  struct tm *now=localtime(&now_t);
#endif
  memset(buf,0,buflen); u8_zero_array(clockbuf);
  u8_zero_array(timebuf); u8_zero_array(procbuf);
  if (u8_log_show_date)
    strftime(clockbuf,32,"(%d%b%y)%H:%M:%S",now);
  else strftime(clockbuf,32,"%H:%M:%S",now);
  if (u8_log_show_elapsed)
    u8_sprintf(timebuf,128,"%s(%f)",clockbuf,u8_elapsed_time());
  else u8_sprintf(timebuf,128,"%s",clockbuf);
  if (!((u8_log_show_procinfo)||(u8_log_show_threadinfo))) {
    strcpy(buf,timebuf);
    return buf;}
  else procid=NULL;
  if (u8_log_show_appid) appid=u8_appid();
#if (HAVE_GETPID)
  if (u8_log_show_threadinfo)
    procid=u8_sprintf(procbuf,128,"%lld:%lld",
		      (long long)getpid(),
		      (long long)u8_threadid());
  else {
    u8_write_long_long((long long)getpid(),procbuf,128);
    procid=procbuf;}
#else
  if (u8_log_show_procinfo) strcpy(procbuf,"nopid");
#endif
  if ( (appid) && (procid) )
    u8_sprintf(buf,buflen,"%s <%s:%s>",timebuf,appid,procid);
  else if (appid)
    u8_sprintf(buf,buflen,"%s <%s>",timebuf,appid);
  else u8_sprintf(buf,buflen,"%s <%s>",timebuf,procid);
  return buf;
}

static u8_mutex logger_init_lock;
int u8_logger_initialized=0;

static int logging_c_initialized=0;

void init_logging_c()
{
  if (logging_c_initialized>0) return;
  else logging_c_initialized=1;
  u8_init_mutex(&logger_init_lock);
#if ((U8_THREADS_ENABLED) && (U8_USE_TLS))
  u8_new_threadkey(&u8_log_context_key,NULL);
#endif

  if (getenv("U8_LOGLEVEL")) {
    u8_string spec = u8_getenv("U8_LOGLEVEL");
    int loglevel = u8_get_loglevel(spec);
    if (loglevel>0) u8_loglevel = loglevel;}

  u8_register_source_file(_FILEINFO);
}

U8_EXPORT void u8_init_logger()
{
  u8_lock_mutex(&logger_init_lock);
  if (u8_logger_initialized) {
    u8_unlock_mutex(&logger_init_lock);
    return;}
  else u8_logger_initialized=1;

#if HAVE_SYSLOG
  u8_string app=u8_appid();
  openlog(app,LOG_PID|LOG_CONS|LOG_NDELAY|LOG_PERROR,LOG_DAEMON);
#endif
  u8_unlock_mutex(&logger_init_lock);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
