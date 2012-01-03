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

#include "libu8/libu8io.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include <stdarg.h>

#ifndef PRINTF_CHUNK_SIZE
#define PRINTF_CHUNK_SIZE 128
#endif

u8_condition u8_BadPrintFormat=_("u8_printf: Bad format code");

#if ((U8_THREADS_ENABLED) && (HAVE_GETTEXT))
static u8_mutex textdomains_lock;
#endif

/* Message lookup */

#if HAVE_GETTEXT
static struct U8_TEXTDOMAIN {
  const char *domain;
  u8_xlatefn xlate;
  struct U8_TEXTDOMAIN *next;} *textdomains=NULL;

static u8_string getmessage(u8_string msg)
{
  struct U8_TEXTDOMAIN *scan=textdomains;
  while (scan) {
    u8_string newstring=
      ((scan->xlate) ? (scan->xlate(msg)) :
       ((u8_string)(dgettext(scan->domain,msg))));
    if (newstring==msg)
      scan=scan->next;
    else return newstring;}
  return dgettext("libu8",msg);
}

U8_EXPORT
/* u8_register_textdomain:
   Arguments: a gettext domain and an encoding
   Returns: void
 Records the text domain for message translation by u8_printf.
*/
void u8_register_textdomain(char *domain)
{
}

U8_EXPORT
/* u8_register_xlatefn:
   Arguments: a translation function
   Returns: void
 Adds a translation function to the lookup list.
*/
void u8_register_xlatefn(u8_xlatefn fn)
{
}

U8_EXPORT
/* u8_getmessage:
   Arguments: a string
   Returns: a string
 Searches all registered text domains for translations for a particular
  message string.
*/
u8_string u8_getmessage(u8_string msg)
{
  return getmessage(msg);
}
#else
#define getmessage(x) (x)
#endif

/* Extended printf */

u8_printf_handler u8_printf_handlers[128];

#define printf_codep(c) ((c<128)&&(u8_printf_handlers[(int)c]!=NULL))

/* This is the key internal function for normal printf, which checks
   for a translation of the format string, calls message_printf if there
   is one, and otherwise just processes the format string and arguments
   in order. */
int u8_do_printf(u8_output s,u8_string fstring,va_list *args)
{
  char *format_string=getmessage(fstring);
  int n_directives=0;
  unsigned char *scan=format_string, *fmt=strchr(scan,'%');
  while (fmt) {
    unsigned char cmd[16]; int i=0, code; u8_string to_free=NULL;
    /* First, output everything leading up to the % sign */
    u8_putn(s,scan,fmt-scan); scan=fmt;
    /* Read the percent sign */
    cmd[i++]=*scan++;
    /* Read in the format code */
    while ((i<16) && (*scan) && (!(printf_codep(*scan))))
      cmd[i++]=*scan++; 
    /* Read the final byte */
    cmd[i++]=code=*scan++; cmd[i]='\0'; n_directives++;
    if (code == '%') u8_putc(s,'%');
    else {
      char buf[PRINTF_CHUNK_SIZE], *string=buf;
      if ((code == 'd') || (code == 'i') ||
	  (code == 'u') || (code == 'o') ||
	  (code == 'x')) {
	if (strstr(cmd,"ll")) {
	  long long i=va_arg(*args,long long);
	  sprintf(buf,cmd,i);}
	else if (strstr(cmd,"l")) {
	  long i=va_arg(*args,long);
	  sprintf(buf,cmd,i);}
	else {
	  int i=va_arg(*args,int);
	  sprintf(buf,cmd,i);}}
      else if ((code == 'f') || (code == 'g') || (code == 'e')) {
	double f=va_arg(*args,double);
	sprintf(buf,cmd,f);}
      else if (code == 's') {
	string=va_arg(*args,char *);
	/* A - modifer on s indicates that the string arg should be freed
	   after use. */
	if (string==NULL) string="(null)";
	else if (strchr(cmd,'-')) to_free=string;}
      else if (code == 'm') {
	/* The m conversion is like s but passes its argument through the
	   message catalog. */
	u8_string arg=va_arg(*args,char *);
	string=getmessage(arg);}
      else if ((code<128) && (u8_printf_handlers[(int)code]))
	/* We pass the pointer args because some stdarg implementations
	   work better that way. */
	string=u8_printf_handlers[(int)code]
	  (s,cmd,buf,PRINTF_CHUNK_SIZE,args);
      else return u8_reterr(u8_BadPrintFormat,"u8_do_printf",u8_strdup(cmd));
      if (string == NULL) {} else u8_puts(s,string);
      if (to_free) u8_free(to_free);}
    fmt=strchr(scan,'%');}
  u8_puts(s,scan); /* At the end, output the tail of the format string */
  return n_directives;
}

U8_EXPORT
/* u8_printf
    Arguments: a string stream, a format string, and other args
    Returns: void

  Outputs a string to string stream generated from the format string and
using the provided arguments.  Much like printf (surprise). */
int u8_printf(u8_output s,u8_string format_string,...)
{
  va_list args; int retval;
  va_start(args,format_string);
  retval=u8_do_printf(s,format_string,&args);
  va_end(args);
  return retval;
}

U8_EXPORT
/* u8_mkstring
    Arguments: a format string, and other args
    Returns: a malloc'd string whose contains are generated from the arguments
*/
u8_string u8_mkstring(u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args; int retval=0;
  U8_INIT_OUTPUT(&out,128);
  va_start(args,format_string);
  if ((retval=u8_do_printf(&out,format_string,&args))<0) {
    u8_free(out.u8_outbuf); return NULL;}
  else return out.u8_outbuf;
}

static u8_string default_printf_handler
  (u8_output s,char *cmd,u8_string buf,int bufsiz,va_list *args)
{
  return NULL;
}

U8_EXPORT void u8_init_printf_c()
{
  /* These are all really no-ops, but they help with parsing. */
  u8_printf_handlers['s']=default_printf_handler;
  u8_printf_handlers['m']=default_printf_handler;
  u8_printf_handlers['d']=default_printf_handler;
  u8_printf_handlers['u']=default_printf_handler;
  u8_printf_handlers['o']=default_printf_handler;
  u8_printf_handlers['x']=default_printf_handler;
  u8_printf_handlers['e']=default_printf_handler;
  u8_printf_handlers['f']=default_printf_handler;
  u8_printf_handlers['g']=default_printf_handler;
  u8_printf_handlers['%']=default_printf_handler;
#if ((U8_THREADS_ENABLED) && (HAVE_GETTEXT))
  u8_init_mutex(&textdomains_lock);
#endif
}
