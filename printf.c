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

#include "libu8/u8source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8io.h"
#include "libu8/u8streamio.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8printf.h"

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include <stdarg.h>

#ifndef PRINTF_CHUNK_SIZE
#define PRINTF_CHUNK_SIZE 1234
#endif

u8_condition u8_BadPrintFormat=_("u8_printf: Bad format code");

#if ((U8_THREADS_ENABLED) && (HAVE_GETTEXT))
static u8_mutex textdomains_lock;
static u8_mutex gettext_lock;
#endif

/* Message lookup */

#if HAVE_GETTEXT
static struct U8_TEXTDOMAIN {
  const char *domain;
  u8_mutex gettext_lock;
  u8_xlatefn xlate;
  struct U8_TEXTDOMAIN *next;} *textdomains=NULL;

static u8_string getmessage(u8_string msg)
{
  u8_string result=msg;
  struct U8_TEXTDOMAIN *scan=textdomains;
  while (scan) {
    u8_string newstring;
    if (scan->xlate)
      newstring=scan->xlate(msg);
    else {
      u8_lock_mutex(&(scan->gettext_lock));
      newstring=(u8_string)dgettext(scan->domain,msg);
      u8_unlock_mutex(&(scan->gettext_lock));}
    if ( (newstring==NULL) || (newstring==msg) )
      scan=scan->next;
    else return newstring;}
  u8_lock_mutex(&(gettext_lock));
  result=dgettext("libu8",msg);
  u8_unlock_mutex(&(gettext_lock));
  return result;
}

U8_EXPORT
/* u8_register_textdomain:
   Arguments: a gettext domain and an encoding
   Returns: void
 Records the text domain for message translation by u8_printf.
*/
void u8_register_textdomain(char *domain)
{
  struct U8_TEXTDOMAIN *scan, *new;
  u8_lock_mutex(&textdomains_lock);
  scan=textdomains;
  while (scan) {
    if (strcmp(scan->domain,domain)==0) {
      u8_unlock_mutex(&textdomains_lock);
      return;}
    else scan=scan->next;}
  new=u8_alloc(struct U8_TEXTDOMAIN);
  new->domain=u8_strdup(domain);
  new->xlate=NULL;
  u8_init_mutex(&(new->gettext_lock));
  new->next=textdomains;
  textdomains=new;
  u8_unlock_mutex(&textdomains_lock);
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
int u8_do_printf(u8_output s,u8_string format_string,va_list *args)
{
  int n_directives=0;
  const unsigned char *scan=format_string, *fmt=strchr(scan,'%');
  while (fmt) {
    unsigned char cmd[16]; int i=0, code; u8_string to_free=NULL;
    /* First, output everything leading up to the % sign */
    if (fmt>scan) {
      u8_putn(s,scan,fmt-scan); scan=fmt;}
    /* Read the percent sign */
    cmd[i++]=*scan++;
    /* Read in the format code */
    while ((i<16) && (*scan) && (!(printf_codep(*scan))))
      cmd[i++]=*scan++;
    /* Read the final byte */
    cmd[i++]=code=*scan++; cmd[i]='\0'; n_directives++;
    if (code == '%') u8_putc(s,'%');
    else {
      unsigned char buf[PRINTF_CHUNK_SIZE], *string=buf;
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
      else if ((code == 's')||(code == 'm')) {
	char *prefix=NULL;
	char *arg=va_arg(*args,char *); string=arg;
	/* A - modifer on s indicates that the string arg should be
	   freed after use.  A modifier # indicates that the argument
	   is a libc string and will need to be converted to UTF-8.
	   Modifiers l and u indicates upper and lower case conversions.
	   Further :/,.? modifiers indicate a prefix character which
	   precedes the string when the string is not empty. */
	if (strchr(cmd,'/')) prefix="/";
	else if (strchr(cmd,':')) prefix=":";
	else if (strchr(cmd,'.')) prefix=".";
	else if (strchr(cmd,',')) prefix=", ";
	else NO_ELSE;
	if (strchr(cmd,'-')) to_free=arg;
	/* The m conversion is like s but passes its argument through the
	   message catalog. */
	if ((arg)&&(code=='m'))
	  arg=(u8_byte *)getmessage(arg);
	if ((string) && (strchr(cmd,'#'))) {
	  string=(u8_byte *)u8_fromlibc(string);
	  if (to_free) u8_free(to_free);
	  to_free=string;}
	if (arg==NULL) {
	  if ((prefix)||(strchr(cmd,'?')))
	    string="";
	  else string="(null)";}
	else if (strchr(cmd,'l')) {
	  string=(u8_byte *)u8_downcase(string);
	  if (to_free) u8_free(to_free);
	  to_free=string;}
	else if (strchr(cmd,'u')) {
	  string=(u8_byte *)u8_upcase(string);
	  if (to_free) u8_free(to_free);
	  to_free=string;}
	else NO_ELSE;
	if ((arg)&&(prefix)) {
	  string=(u8_byte *)u8_string_append(prefix,string,NULL);
	  if (to_free) u8_free(to_free);
	  to_free=string;}}
      else if (code == 'v') {
	unsigned char *start=va_arg(*args,unsigned char *);
	unsigned char *scan=start, *limit;
	ssize_t len=
	  ((strchr(cmd,'l'))?(va_arg(*args,ssize_t)):(va_arg(*args,int)));
	if (len>=0) limit=start+len;
	else {limit=start; while (*limit) limit++;}
	while (scan<limit) {
	  int ch=*scan++; char buf[8]; 
	  if (scan<limit) sprintf(buf,"%02x:",ch);
	  else sprintf(buf,"%02x",ch);
	  u8_puts(s,buf);}
	string=NULL;}
      else if (code == 'c') {
	unsigned int codepoint = va_arg(*args,int);
	u8_putc(s,codepoint); string = NULL; }
      else if ((code<128) && (u8_printf_handlers[(int)code]))
	/* We pass the pointer args because some stdarg implementations
	   work better that way. */
	string=(u8_byte *)u8_printf_handlers[(int)code]
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
  U8_INIT_STATIC_OUTPUT(out,128);
  va_start(args,format_string);
  if ((retval=u8_do_printf(&out,format_string,&args))<0) {
    va_end(args);
    u8_free(out.u8_outbuf); return NULL;}
  else {
    va_end(args);
    return out.u8_outbuf;}
}

U8_EXPORT
/* u8_sprintf:
    Arguments: a string buffer, its length, a format string, and other args
    Returns: a malloc'd string whose contains are generated from the arguments
*/
u8_string u8_sprintf
(unsigned char *buf_arg,size_t buflen,u8_string format_string,...)
{
  struct U8_OUTPUT out; va_list args; int retval=0;
  u8_byte *buf = (buf_arg) ? (buf_arg) : (u8_malloc(buflen));
  U8_INIT_FIXED_OUTPUT(&out,buflen,buf);
  va_start(args,format_string);
  if ((retval=u8_do_printf(&out,format_string,&args))<0) {
    va_end(args);
    u8_free(out.u8_outbuf); return NULL;}
  else {
    va_end(args);
    return out.u8_outbuf;}
}

static u8_string default_printf_handler
  (u8_output s,char *cmd,u8_byte *buf,int bufsiz,va_list *args)
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
  u8_printf_handlers['v']=default_printf_handler;
  u8_printf_handlers['%']=default_printf_handler;
#if ((U8_THREADS_ENABLED) && (HAVE_GETTEXT))
  u8_init_mutex(&textdomains_lock);
  u8_init_mutex(&gettext_lock);
#endif
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
