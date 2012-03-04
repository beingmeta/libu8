/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2012 beingmeta, inc.
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

#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include <stdarg.h>
#include <stdio.h> /* for sprintf */

int u8_log_show_date=0; /* If zero, show the date together with the time. */
int u8_log_show_procinfo=0; /* If zero, this overrides u8_log_show_threadinfo. */
int u8_log_show_elapsed=0; /* This tells displays to show elapsed time (fine grained). */
int u8_log_show_appid=1;
int u8_log_show_threadinfo=0;

u8_string u8_loglevels[9]={
  _("Emergency!!!"),_("Alert!!"),_("Critical"),_("Error"),_("Warning"),
  _("Note"),_("Info"),_("Debug"),NULL};

/* Generic logging */
/*  This assumes that it is linked with code that defines u8_logger. */

int u8_loglevel=U8_DEFAULT_LOGLEVEL;
int u8_stdout_loglevel=U8_DEFAULT_STDOUT_LOGLEVEL;
int u8_stderr_loglevel=U8_DEFAULT_STDERR_LOGLEVEL;
int u8_syslog_loglevel=U8_DEFAULT_SYSLOG_LOGLEVEL;

static u8_logfn logfn=NULL;
U8_EXPORT int u8_default_logger(int loglevel,u8_condition c,u8_string message);

#if U8_WITH_STDIO
U8_EXPORT int u8_default_logger(int loglevel,u8_condition c,u8_string message)
{
  u8_byte buf[128]; u8_string prefix; 
  if (loglevel>7) {
    fprintf(stderr,"[!! Logging call with invalid priority %d (%s)]\n",loglevel,c);
    return 0;}
  else if (loglevel>u8_loglevel) return 0;
  prefix=u8_message_prefix(buf,128);
  if (loglevel<0) {
    if (c)
      fprintf(stdout,"[%s (%s) %s]\n",prefix,c,message);
    else fprintf(stdout,"[%s %s]\n",prefix,message);
    fflush(stdout);
    return 1;}
  if (loglevel<=u8_stdout_loglevel) {
    if (c)
      fprintf(stdout,"[%s %s (%s): %s]\n",
	      prefix,u8_loglevels[loglevel],
	      c,message);
    else fprintf(stdout,"[%s %s: %s]\n",
		 prefix,u8_loglevels[loglevel],message);}
  if (loglevel<=u8_stderr_loglevel) {
    if (c)
      fprintf(stderr,"[%s %s (%s): %s]\n",
	      prefix,u8_loglevels[loglevel],
	      c,message);
    else fprintf(stderr,"[%s %s: %s]\n",
		 prefix,u8_loglevels[loglevel],message);}
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
  if (logfn) retval=logfn(-1,NULL,out.u8_outbuf);
  else retval=u8_default_logger(-1,NULL,out.u8_outbuf);
  if ((out.u8_streaminfo)&(U8_STREAM_OWNS_BUF)) u8_free(out.u8_outbuf);
  return retval;
}

/* Using syslog if it's there */

#if HAVE_SYSLOG
static u8_mutex syslog_init_lock;

int u8_syslog_initialized=0;

U8_EXPORT void u8_init_syslog()
{
  u8_string app;
  u8_lock_mutex(&syslog_init_lock);
  if (u8_syslog_initialized) {
    u8_unlock_mutex(&syslog_init_lock);
    return;}
  app=u8_appid();
  openlog(app,LOG_PID|LOG_CONS|LOG_NDELAY|LOG_PERROR,LOG_DAEMON);
  u8_syslog_initialized=1;
  u8_unlock_mutex(&syslog_init_lock);
}
#else
U8_EXPORT void u8_init_syslog()
{
  u8_syslog_initialized=1;
}
#endif

/* Figuring out the prefix for log messages */

U8_EXPORT u8_string u8_message_prefix(u8_byte *buf,int buflen)
{
  time_t nowval=time(NULL);
  struct tm *now=localtime(&nowval);
  char clockbuf[64], timebuf[64], procid[128]; u8_string appid=NULL;
  if (u8_log_show_date)
    strftime(clockbuf,32,"%H:%M:%S(%d%b%y)",now);
  else strftime(clockbuf,32,"%H:%M:%S",now);
  if (u8_log_show_elapsed) 
    sprintf(timebuf,"%s(%f)",clockbuf,u8_elapsed_time());
  else sprintf(timebuf,"%s",clockbuf);
  if (u8_log_show_procinfo) {
#if ((HAVE_GETPID) && (HAVE_PTHREAD_SELF))
    if (u8_log_show_threadinfo)
      sprintf(procid,"%lu:%lx",
	      (unsigned long)getpid(),
	      (unsigned long)pthread_self());
    else sprintf(procid,"%lu",(unsigned long)getpid());
#elif (HAVE_GETPID)
    sprintf(procid,"%lu",(unsigned long)getpid());
#else
    sprintf(procid,"noinfo");
#endif
  }
  else strcpy(procid,"");
  if ((u8_log_show_procinfo) && (u8_log_show_appid)) appid=u8_appid();
  if (!(u8_log_show_procinfo)) {
    sprintf(buf,"%s",timebuf);
    return buf;}
  else if ((u8_log_show_procinfo) && (u8_log_show_appid) && (appid!=NULL) &&
	   ((strlen(timebuf)+strlen(procid)+strlen(appid)+5)<buflen))
    sprintf(buf,"%s <%s:%s>",timebuf,appid,procid);
  else if ((u8_log_show_procinfo) &&
	   ((strlen(timebuf)+strlen(procid)+5)<buflen))
    sprintf(buf,"%s <%s>",timebuf,procid);
  else if ((strlen(timebuf)+2)<buflen)
    sprintf(buf,"%s ",timebuf);
  else return NULL;
  return buf;
}





