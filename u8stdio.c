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

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8stringfns.h"
#include "libu8/u8streamio.h"
#include "libu8/u8logging.h"
#include "libu8/libu8io.h"
#include "libu8/u8stdio.h"

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <unistd.h>
#include <time.h>
#include <errno.h>

#if HAVE_SYSLOG
#include <syslog.h>
#endif

U8_EXPORT void u8_check_stdio(void);
U8_EXPORT void u8_initialize_logging(void);

u8_condition stdio_error=_("STDIO error");

static int stdoutISstderr=-1;

static void do_output(FILE *out,u8_string prefix,
		      u8_string level,u8_condition c,
		      u8_string body);

static int stdio_logger(int priority,u8_condition c,u8_string msg)
{
  unsigned int epriority = (priority < 0) ? (-priority) : (priority);
  if ( (priority >= 0) &&
       (! ( (priority <= u8_syslog_loglevel) ||
	    (priority <= u8_loglevel) ) ) )
    return 0;
  u8_byte buf[512]; int output=0; u8_string indented=NULL;
  u8_string prefix = u8_message_prefix(buf,512);
  u8_string level = (epriority < U8_MAX_LOGLEVEL) ? (u8_loglevels[epriority]) : (NULL);
  if (prefix == NULL) prefix="";
  if ((u8_logindent)&&(u8_logindent[0])&&(strchr(msg,'\n')))
    indented=u8_indent_text(msg,u8_logindent);
  if (!(indented)) indented=msg;
  if (u8_logger_initialized==0) u8_init_logger();
#if HAVE_SYSLOG
  if ((priority>=0) && (priority<=u8_syslog_loglevel)) {
    syslog(priority,"%s",indented);}
  else NO_ELSE;
#endif
  if (priority>u8_loglevel) {
    if ((indented)&&(msg!=indented)) u8_free(indented);
    return 0;}
  if (stdoutISstderr<0) u8_check_stdio();
  if (stdoutISstderr) {
    if ((priority<0) || (epriority <= u8_stderr_loglevel) ) {
      do_output(stderr,prefix,level,c,indented); output=1;}
    else if (priority <= u8_stdout_loglevel) {
      do_output(stdout,prefix,level,c,indented); output=1;}
    else NO_ELSE;
    if ((indented)&&(msg!=indented)) u8_free(indented);
    return output;}
  if ((priority<0) || (epriority <= u8_stderr_loglevel) ) {
    do_output(stderr,prefix,level,c,indented); output=1;}
  if (epriority <= u8_stdout_loglevel) {
    do_output(stdout,prefix,level,c,indented);
    fflush(stdout);
    output=1;}
  if ((indented)&&(msg!=indented)) u8_free(indented);
  return output;
}

static void do_output(FILE *out,u8_string prefix,
		      u8_string level,u8_condition c,
		      u8_string body)
{
  if (prefix == NULL) prefix = "";
  if ((c) && (level))
    fprintf(out,"%s%s %s (%s) %s",u8_logprefix,prefix,level,c,body);
  else if (c)
    fprintf(out,"%s%s (%s) %s",u8_logprefix,prefix,c,body);
  else if (level)
    fprintf(out,"%s%s %s %s",u8_logprefix,prefix,level,body);
  else fprintf(out,"%s%s %s",u8_logprefix,prefix,body);
  if ( u8_log_context ) {
    if ( (u8_logindent) && (u8_logindent[0]) )
      fprintf(out,"\n%s@ %s",u8_logindent,u8_log_context);
    else fprintf(out,"\n\t@ %s",u8_log_context);}
  if (u8_logsuffix) fputs(u8_logsuffix,out);
}

#if (!(U8_WITH_STDIO))
U8_EXPORT int u8_default_logger(int priority_level,u8_condition c,
				u8_string msg)
{
  return stdio_logger(priority,c,msg);
}
#endif

static void raisefn(u8_condition ex,u8_context cxt,u8_string details)
{
  U8_OUTPUT out;
  U8_INIT_STATIC_OUTPUT(out,512);
  if (details)
    if (cxt)
      u8_printf(&out,"Aborting due to %m@%s: %m",ex,cxt,details);
    else u8_printf(&out,"Aborting due to %m: %m",ex,details);
  else if (cxt)
    u8_printf(&out,"Aborting due to %m@%s",ex,cxt);
  else u8_printf(&out,"Aborting due to %m",ex);

  fprintf(stderr,"%s\n",out.u8_outbuf);
  exit(1);
}

U8_EXPORT void u8_check_stdio()
{
  if (getenv("U8_STDIOISSTDERR")) stdoutISstderr=1;
#if (HAVE_ISATTY)
  else if ((isatty(1)) && (isatty(2))) {
    char *n1=ttyname(1), *n2=ttyname(2);
    if ((n1==NULL)||(n2==NULL)||(n1==n2))
      stdoutISstderr=1;
    else stdoutISstderr=(strcmp(n1,n2)==0);}
  else if ((isatty(1)) || (isatty(2)))
    stdoutISstderr=0;
#endif
#if ((HAVE_SYS_STAT_H) && (HAVE_FSTAT))
  else {
    struct stat outstat, errstat;
    if (fstat(1,&outstat)<0) stdoutISstderr=0;
    if (fstat(2,&errstat)<0) stdoutISstderr=0;
    else if ((outstat.st_dev==errstat.st_dev) &&
             (outstat.st_ino==errstat.st_ino))
      stdoutISstderr=1;
    else stdoutISstderr=0;}
#else
  else stdoutISstderr=1;
#endif
  U8_CLEAR_ERRNO();
}

U8_EXPORT void u8_stdoutISstderr(int flag)
{
  stdoutISstderr=flag;
}

/* u8_fprintf */

/* STDIO output with three encodings */

/* These are minimal functions which should usually be replaced
   by use of XFILEs. */

#define UTF8_ENC 0
#define LATIN1_ENC 1
#define ASCII_ENC 2

static int lite_encoding=UTF8_ENC;

U8_EXPORT void u8_ascii_stdio()
{
  lite_encoding=ASCII_ENC;
}
U8_EXPORT void u8_latin1_stdio()
{
  lite_encoding=LATIN1_ENC;
}
U8_EXPORT void u8_utf8_stdio()
{
  lite_encoding=UTF8_ENC;
}

U8_EXPORT void u8_fputs(u8_string s,FILE *f)
{
  if (lite_encoding==UTF8_ENC) fputs(s,f);
  else {
    long int nbytes;
    unsigned int maxchar=
      ((lite_encoding==LATIN1_ENC) ? (0x100) : (0x80));
    const u8_byte *scan=s, *start=s;
    while (*scan)
      if (*scan>=0x80) {
        int c, retval=0;
        if (scan>start) retval=fwrite(start,1,scan-start,f);
        if (retval<0) {
          u8_log(LOG_WARN,"u8_fputs",u8_strdup(strerror(errno)));
          errno=0;}
        c=u8_sgetc(&scan); start=scan;
        if (c<maxchar) fputc(c,f);
        else fprintf(f,"\\u%04x",c);}
      else scan++;
    nbytes=fwrite(start,1,scan-start,f);
    if (nbytes<0) {
      u8_log(LOG_CRIT,stdio_error,"Error writing file output");
      u8_graberrno("u8_fputs",NULL);}}
}

U8_EXPORT void u8_fprintf(FILE *f,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args;
  U8_INIT_STATIC_OUTPUT(out,512);
  va_start(args,format_string);
  u8_do_printf(&out,format_string,&args);
  va_end(args);
  u8_fputs(out.u8_outbuf,f);
  u8_free(out.u8_outbuf);
}

/* Initialization functions */

U8_EXPORT void u8_initialize_u8stdio()
{
  u8_register_source_file(_FILEINFO);
  u8_set_error_handler(raisefn);
  u8_set_logfn(stdio_logger);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
