/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2018 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#define U8_INLINE_IO 1

#include "libu8/u8source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8streamio.h"
#include "libu8/u8ctype.h"
#include "libu8/u8stringfns.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#if HAVE_STRING_H
#include <string.h>
#endif

u8_condition u8_NullString=_("Null UTF-8 string");

#define CHECK_NULL_STRING(arg,cxt,rv)		\
  if (U8_EXPECT_FALSE(arg==NULL)) {		\
    u8_seterr(u8_NullString,cxt,NULL);		\
    return rv;}

/* For debugging */

U8_EXPORT void u8_utf8_warning
(u8_condition message,u8_string string,u8_string lim)
{
  char buf[UTF8_BUGWINDOW];
  int n_bytes=UTF8_BUGWINDOW;
  if ((lim)&&((lim-string)<n_bytes)) n_bytes=lim-string;
  u8_grab_bytes(string,n_bytes,buf);
  u8_log(LOG_WARN,message,_("bytes: %s"),buf);
}

/* Core functions */

int _u8_sgetc(const u8_byte **s)
{
  return u8_sgetc(s);
}

int _u8_sgetc_lim(const u8_byte **s,const u8_byte *lim)
{
  return u8_sgetc_lim(s,lim);
}

u8_byteoff _u8_byteoffset(u8_string s,u8_charoff offset,u8_byteoff max)
{
  return u8_byteoffset(s,offset,max);
}

u8_charoff _u8_charoffset(u8_string s,u8_byteoff i)
{
  return u8_charoffset(s,i);
}

u8_buf u8_strncpy(u8_buf dest,u8_string src,size_t n)
{
  u8_string scan=src, lim=src+n, end;
  while ((*scan) && (scan < lim)) {
    int c=u8_sgetc(&scan);
    // TODO: Handle combining characters
    if (c<0) break;
    end=scan;}
  memmove(dest,src,end-src); dest[lim-src]='\0';
  return dest;
}

u8_buf _u8_strdup(u8_string s)
{
  CHECK_NULL_STRING(s,"u8_strdup",NULL);
  int len=strlen(s); int newlen=len+1;
  newlen=(((newlen%4)==0)?(newlen):(((newlen/4)+1)*4));
  u8_byte *nstring=u8_malloc(newlen);
  strncpy(nstring,s,len); nstring[len]='\0';
  return (u8_buf)nstring;
}
u8_buf u8_strndup(u8_string s,int len)
{
  CHECK_NULL_STRING(s,"u8_strndup",NULL);
  int newlen=len+1;
  newlen=(((newlen%4)==0)?(newlen):(((newlen/4)+1)*4));
  u8_byte *nstring=u8_malloc(newlen);
  strncpy(nstring,s,len); nstring[len]='\0';
  return (u8_buf)nstring;
}

U8_EXPORT
/* u8_strlen_x:
    Arguments: a pointer to a UTF-encoded string and a length
    Returns: an integer indicating the number of unicode characters
in the string it represents
*/
ssize_t u8_strlen_x(u8_string str,size_t slen)
{
  CHECK_NULL_STRING(str,"u8_strlen_x",-1);
  const u8_byte *scan=str, *limit=str+slen; int len=0, rv=0;
  while ((rv>=0)&&(scan < limit)) {
    len++;
    if (*scan<0x80) scan++;
    else rv=u8_sgetc(&scan);}
  if (rv<-1) return rv;
  else return len;
}

U8_EXPORT
/* u8_strlen:
    Arguments: a pointer to a mull-terminated UTF-encoded string
    Returns: an integer indicating the number of unicode characters
in the string it represents
*/
size_t u8_strlen(u8_string str)
{
  CHECK_NULL_STRING(str,"u8_strlen",-1);
  const u8_byte *scan=str;
  int ch=u8_sgetc(&scan), len=0;
  while (ch>=0) {
    len++;
    ch=u8_sgetc(&scan);}
  if (ch<-1)
    return ch;
  else return len;
}

U8_EXPORT
/* u8_substring:
    Arguments: a pointer to a UTF-encoded string and an integer
    Returns: the substring starting at the interger-th character

*/
u8_string u8_substring(u8_string str,int index)
{
  CHECK_NULL_STRING(str,"u8_substring",NULL);
  const u8_byte *scan=str, *last=scan; int count=index;
  while ((count > 0) && (u8_sgetc(&scan) >= 0)) {
    last=scan; count--;}
  if (count == 0) return last;
  else return NULL;
}

U8_EXPORT
/* u8_slice:
    Arguments: two pointers into a UTF8-encoded string
    Returns: the substring between the pointers
*/
u8_string u8_slice(const u8_byte *start,const u8_byte *end)
{
  CHECK_NULL_STRING(start,"u8_slice",NULL);
  if (end==NULL)
    return u8_strdup(start);
  else if (end<start)
    return NULL;
  else if (end-start>65536*8)
    return NULL;
  else {
    unsigned int newlen=(end-start)+1;
    u8_byte *slice;
    newlen=(((newlen%4)==0)?(newlen):(((newlen/4)+1)*4));
    slice=u8_malloc(newlen);
    strncpy(slice,start,(end-start));
    slice[end-start]='\0';
    return (u8_string)slice;}
}


U8_EXPORT
int _u8_char_len(u8_string s)
{
  CHECK_NULL_STRING(s,"u8_char_len",-1);
  return u8_char_len(s);
}

U8_EXPORT u8_string u8_char2bytes(int character,u8_byte buf[8])
{
  struct U8_OUTPUT tmp;
  U8_INIT_STATIC_OUTPUT_BUF(tmp,8,buf);
  if (character<=0)
    tmp.u8_outbuf[0]='\0';
  else {u8_putc(&tmp,character);}
  return tmp.u8_outbuf;
}

U8_EXPORT
/* u8_string_ref:
    Arguments: a pointer to a UTF-encoded string
    Returns: returns the first unicode character in the string

*/
int u8_string_ref(u8_string str)
{
  CHECK_NULL_STRING(str,"u8_string_ref",-1);
  int c=u8_sgetc(&str);
  return c;
}

/** UTF-8 lengths and validation **/

static int get_utf8_size(u8_byte s1)
{
  if (s1 < 0x80) return 1;
  else if (s1 < 0xC0) return -1;
  else if (s1 < 0xE0) return 2;
  else if (s1 < 0xF0) return 3;
  else if (s1 < 0xF8) return 4;
  else if (s1 < 0xFC) return 5;
  else if (s1 < 0xFE) return 6;
  else return -1;
}

static int check_utf8_ptr(u8_string s,int size)
{
  CHECK_NULL_STRING(s,"check_utf8_ptr",-1);
  int i=1;
  if (size<0)
    return 0;
  else if (size == 1)
    return size;
  /* Now check that the string is valid */
  while (i < size)
    if (s[i] < 0x80) return -i;
    else if (s[i] > 0xc0) return -i;
    else i++;
  return size;
}

static int valid_utf8p(u8_string s)
{
  int sz = check_utf8_ptr(s,get_utf8_size(*s));
  while (sz > 0)
    if (*s == '\0')
      return 1;
    else {
      s=s+sz;
      sz=check_utf8_ptr(s,get_utf8_size(*s));}
  return 0;
}

U8_EXPORT
/* u8_validptr:
    Arguments: a possible utf8 string
    Returns: 1 if the string is valid, 0 otherwise.
*/
int u8_validptr(const u8_byte *s)
{
  CHECK_NULL_STRING(s,"u8_validptr",-1);
  int sz=get_utf8_size(*s);
  if (sz>0)
    return (check_utf8_ptr(s,sz)>0);
  else return 0;
}

U8_EXPORT
/* u8_validp:
    Arguments: a possible utf8 string
    Returns: 1 if the string is valid, 0 otherwise.
*/
int u8_validp(u8_string s)
{
  CHECK_NULL_STRING(s,"u8_validp",-1);
  return valid_utf8p(s);
}

U8_EXPORT
/* u8_validate:
    Arguments: a possible utf8 string
    Returns: the number of bytes which are valid
*/
ssize_t u8_validate(u8_string s,size_t len)
{
  CHECK_NULL_STRING(s,"u8_validate",-1);
  int sz=get_utf8_size(*s);
  const u8_byte *limit=s+len, *start=s;
  while ((s<limit) && ((sz=get_utf8_size(*s))>0))
    if (s+sz>limit)
      return s-start;
    else s=s+sz;
  return s-start;
}

U8_EXPORT
u8_string u8_getvalid(u8_string str,u8_string limit)
{
  if (limit)
    while ( (str<limit) && (*str>=0x80) && (*str<0xC0) ) str++;
  else while ( (*str>=0x80) && (*str<0xC0) ) str++;
  return str;
}

U8_EXPORT
/* u8_valid_copy:
     Input: a string which should be UTF-8 encoded
     Output: a utf-8 encoding string
Copies its argument, converting invalid UTF-8 sequences into
sequences of latin-1 characters. This always returns a valid UTF8
string. */
u8_string u8_valid_copy(u8_string s)
{
  CHECK_NULL_STRING(s,"u8_valid_copy",NULL);
  size_t len = strlen(s);
  U8_OUTPUT out; U8_INIT_STATIC_OUTPUT(out,len+2);
  while (*s)
    if (*s<0x80) u8_putc(&out,*s++);
    else if (check_utf8_ptr(s,get_utf8_size(*s))>0) {
      int c=u8_sgetc(&s); u8_putc(&out,c);}
    else while (*s>=0x80) u8_putc(&out,*s++);
  return out.u8_outbuf;
}

U8_EXPORT
/** Copies at most *len* bytes of *string* into *buf*, making sure
    that the copy doesn't terminate inside of a UTF-8 multi-byte
    representation.
    @param string a UTF-8 string
    @param buf a pointer to a byte array of at least *len* bytes
    @param len the length of the byte array
    @returns an int between 1 and 7 inclusive or -1
**/
u8_string u8_string2buf(u8_string string,u8_byte *buf,size_t len)
{
  CHECK_NULL_STRING(string,"u8_string2buf",NULL);
  u8_string scan=string;
  struct U8_OUTPUT tmpout;
  unsigned int margin = (len<17) ? (2) : (5);
  int c = u8_sgetc(&scan);
  U8_INIT_FIXED_OUTPUT(&tmpout,len,buf);
  while ((*scan) && (c>0) && (u8_outbuf_space(&tmpout)>margin)) {
    u8_putc(&tmpout,c);
    c=u8_sgetc(&scan);}
  if ((tmpout.u8_streaminfo)&(U8_STREAM_OVERFLOW)) {
    if (margin<=0) {}
    else if (margin<=2)
      u8_puts(&tmpout,"");
    else u8_puts(&tmpout,".!.!");}
  return buf;
}

U8_EXPORT
/* u8_convert_crlfs:
     Input: a supposedly UTF-8 string
     Output: a valid UTF-8 string \r\n converted to \n
 */
u8_string u8_convert_crlfs(u8_string s)
{
  CHECK_NULL_STRING(s,"u8_convert_crlfs",NULL);
  size_t len = strlen(s);
  U8_OUTPUT out; U8_INIT_STATIC_OUTPUT(out,len+1);
  while (*s)
    if (*s=='\r')
      if (s[1]=='\n') {u8_putc(&out,'\n'); s=s+2;}
      else u8_putc(&out,*s++);
    else if (*s<0x80) u8_putc(&out,*s++);
    else if (check_utf8_ptr(s,get_utf8_size(*s))>0) {
      int c=u8_sgetc(&s); u8_putc(&out,c);}
    else while (*s>=0x80) u8_putc(&out,*s++);
  return (u8_string)out.u8_outbuf;
}

/* Additional functions */

U8_EXPORT
/* u8_downcase:
    Arguments: a null-terminated utf-8 C string
    Returns: a copy of the string in lowercase
*/
u8_string u8_downcase (u8_string string)
{
  CHECK_NULL_STRING(string,"u8_downcase",NULL);
  if (U8_EXPECT_FALSE(string==NULL)) return string;
  const u8_byte *scan=string;
  struct U8_OUTPUT ss; int c;
  U8_INIT_STATIC_OUTPUT(ss,32);
  while (*scan) {
    if (*scan < 0x80) c=tolower(*scan++);
    else c=u8_tolower(u8_sgetc(&scan));
    u8_putc(&ss,c);}
  return (u8_string) ss.u8_outbuf;
}

U8_EXPORT
/* u8_upcase:
    Arguments: a null-terminated utf-8 C string
    Returns: a copy of the string in uppercase
*/
u8_string u8_upcase (u8_string string)
{
  CHECK_NULL_STRING(string,"u8_upcase",NULL);
  const u8_byte *scan=string;
  struct U8_OUTPUT ss; int c;
  U8_INIT_STATIC_OUTPUT(ss,32);
  while (*scan) {
    if (*scan < 0x80) c=toupper(*scan++);
    else c=u8_toupper(u8_sgetc(&scan));
    u8_putc(&ss,c);}
  return (u8_string)ss.u8_outbuf;
}

U8_EXPORT
/* u8_string_append:
    Arguments: a series of strings, terminated by a NULL pointer
    Returns: the concatenation of the strings
*/
u8_string u8_string_append(u8_string first_string,...)
{
  struct U8_OUTPUT out; va_list args; u8_string each;
  va_start(args,first_string);
  U8_INIT_STATIC_OUTPUT(out,512);
  if (first_string) {u8_puts(&out,first_string);}
  while ((each=va_arg(args,u8_string))) {
    if (each[0])  u8_puts(&out,each);}
  va_end(args);
  return out.u8_outbuf;
}

U8_EXPORT
/* u8_decompose:
    Arguments: a UTF-8 string
    Returns: the decomposed string representation
*/
u8_string u8_decompose(u8_string string)
{
  CHECK_NULL_STRING(string,"u8_decompose",NULL);
  struct U8_OUTPUT out;
  const u8_byte *scan=string; int c;
  U8_INIT_STATIC_OUTPUT(out,512);
  while ((c=u8_sgetc(&scan))>0) {
    if (c<0x80) u8_putc(&out,c);
    u8_string str=u8_decompose_char(c);
    if (str) u8_puts(&out,str);
    else u8_putc(&out,c);}
  return (u8_string)out.u8_outbuf;
}

U8_EXPORT
/* u8_string_subst:
    Arguments: returns a copy of its argument with all occurrences of one string replaced with another
    Returns: the concatenation of the strings
*/
u8_string u8_string_subst(u8_string input,u8_string key,u8_string replace)
{
  CHECK_NULL_STRING(input,"u8_string_subst/input",NULL);
  CHECK_NULL_STRING(key,"u8_string_subst/key",NULL);
  if (U8_EXPECT_FALSE (replace==NULL) ) replace="";
  const u8_byte *scan=input, *next=strstr(scan,key);
  if (next==NULL) return u8_strdup(input);
  else {
    struct U8_OUTPUT out;
    int key_len=u8_bytelen(key), replace_len=u8_bytelen(replace);
    U8_INIT_STATIC_OUTPUT(out,u8_bytelen(input)+(replace_len*4)+2);
    while (next) {
      u8_putn(&out,scan,next-scan);
      u8_putn(&out,replace,replace_len);
      scan=next+key_len;
      next=strstr(scan,key);}
    u8_puts(&out,scan);
    return (u8_string) out.u8_outbuf;}
}

U8_EXPORT int u8_has_prefix(u8_string string,u8_string prefix,int casefold)
{
  CHECK_NULL_STRING(string,"u8_has_prefix/string",-1);
  CHECK_NULL_STRING(prefix,"u8_has_prefix/prefix",-1);
  if ((casefold>0)&&(strncasecmp(string,prefix,u8_strlen(prefix))==0))
    return 1;
  else if ((casefold<=0)&&(strncmp(string,prefix,u8_strlen(prefix))==0))
    return 1;
  else return 0;
}

U8_EXPORT int u8_has_suffix(u8_string string,u8_string suffix,int casefold)
{
  CHECK_NULL_STRING(string,"u8_has_suffix/string",-1);
  CHECK_NULL_STRING(suffix,"u8_has_suffix/xuffix",-1);
  int stringlen=u8_strlen(string), suffixlen=u8_strlen(suffix);
  if (suffixlen>stringlen) return 0;
  else if ((casefold>0)&&(strcasecmp(string+(stringlen-suffixlen),suffix)==0))
    return 1;
  else if ((casefold<=0)&&(strcmp(string+(stringlen-suffixlen),suffix)==0))
    return 1;
  else return 0;
}

/* Converting strings to ascii-ish, mostly for debugging */

U8_EXPORT
/* u8_grab_bytes:
    Arguments: a pointer to a string
    Returns: an ASCII string representation of some number of bytes from a string
*/
char *u8_grab_bytes(u8_string s,int n,char *buf)
{
  CHECK_NULL_STRING(s,"u8_grab_bytes",NULL);
  char *result=((buf)?(buf):(u8_malloc(n))), *write=result, *lim=result+(n-1);
  const u8_byte *scan=s; int c=*scan++;
  while ((c>0)&&(write<lim)) {
    if ((c<128)&&((isprint((char)c))||(c==32))) *write++=c;
    else if ((write+2)>lim) break;
    else if ((c=='\n')||(c=='\t')||(c=='\r')) {
      char *s;
      if (c=='\n') s="\\n"; else if (c=='\t') s="\\t"; else s="\\r";
      memcpy(write,s,3); write=write+2;}
    else if ((write+3)>lim) break;
    else {
      char buf[8]; sprintf(buf,"%%%02x",c);
      memcpy(write,buf,4); write=write+3;}
    c=*scan++;}
  *write++='\0';
  return result;
}

/* Inserts a string at the beginning and before every line break
   in another string */
U8_EXPORT u8_string u8_indent_text(u8_string input,u8_string indent)
{
  CHECK_NULL_STRING(input,"u8_indent_text/input",NULL);
  CHECK_NULL_STRING(indent,"u8_indent_text/indent",NULL);
  int len=strlen(input), n_lines=1, indent_len=strlen(indent);
  const u8_byte *scan=input; u8_byte *output;
  while ((scan=strchr(scan+1,'\n'))) n_lines++;
  output=u8_malloc(len+(n_lines*indent_len)+1);
  if (output==NULL) {
    errno=0; return NULL;}
  else {
    const u8_byte *read=input; scan=read;
    u8_byte *write=output;
    while ((scan=strchr(read,'\n'))) {
      int n_bytes=scan-read;
      strncpy(write,read,n_bytes); write=write+n_bytes; *write++='\n';
      strncpy(write,indent,indent_len); write=write+indent_len;
      read=scan+1;}
    strcpy(write,read);
    return (u8_string)output;}
}

/* Comparing strings */

U8_EXPORT int u8_strcmp(u8_string s1,u8_string s2,int cmpflags)
{
  u8_string scan1 = s1, scan2 = s2;
  while ( (*scan1) && (*scan2) ) {
    if ( (*scan1 < 0x80 ) && (*scan2 < 0x80 ) ) {
      int c1, c2;
      if (cmpflags & U8_STRCMP_CI) {
	c1 = u8_tolower(*scan1);
	c2 = u8_tolower(*scan2);}
      else {
	c1 = *scan1;
	c2 = *scan2;}
      if (c1 < c2)
	return -1;
      else if (c1 > c2)
	return 1;
      else {}
      scan1++;
      scan2++;}
    else {
      int c1 = u8_sgetc(&scan1);
      int c2 = u8_sgetc(&scan2);
      if (cmpflags & U8_STRCMP_CI) {
	c1 = u8_tolower(c1);
	c2 = u8_tolower(c2);}
      if (c1<c2)
	return -1;
      else if (c1>c2)
	return 1;
      else {}}}
  if (*scan1)
    return 1;
  else if (*scan2)
    return -1;
  else return 0;
}

/* Searching strings */

U8_EXPORT u8_string u8_strchrs(u8_string s,u8_string chars,int order)
{
  CHECK_NULL_STRING(s,"u8_strchrs/s",NULL);
  CHECK_NULL_STRING(chars,"u8_indent_text/chars",NULL);
  u8_string result=NULL;
  const u8_byte *scan=chars; while (*scan) {
    u8_byte _buf[12], *needle=NULL;
    const u8_byte *start=scan;
    int c=u8_sgetc(&scan);
    const u8_byte *end=scan; 
    if (end>start+1) {
      memcpy(_buf,start,end-start);
      _buf[end-start]='\0';
      needle=_buf;}
    u8_string match=(c<128)? (strchr(s,c)) : (strstr(s,needle));
    if (match==NULL) {}
    else if (order==0)
      return match;
    else if (result==NULL)
      result=match;
    else if ((order>0)?(match>result):(match<result))
      result=match;
    else {}}
  return result;
}

static u8_string _u8_strchr(u8_string s,int needle,u8_string *end)
{
  CHECK_NULL_STRING(s,"u8_strchr",NULL);
  if (needle<0x80) {
    u8_string found=strchr(s,needle), last=NULL;
    while (found) {
      last=found; found=strchr(found+1,needle);}
    return last;}
  else {
    const u8_byte *scan=s, *last=scan;
    int c=u8_sgetc(&scan);
    while ((c>=0)&&(c!=needle)) {
      last=scan; c=u8_sgetc(&scan);}
    if (c==needle) {
      if (end) *end=scan;
      return last;}
    else return NULL;}
}

U8_EXPORT u8_string u8_strchr(u8_string haystack,int needle,int n)
{
  CHECK_NULL_STRING(haystack,"u8_strchr",NULL);
  u8_string result=NULL;
  if (n==1) {
    return _u8_strchr(haystack,needle,NULL);}
  else if (n>=0) {
    const u8_byte *end=NULL, *last=NULL;
    const u8_byte *match=_u8_strchr(haystack,needle,&end);
    while (match) {
      if (n==0) return match;
      else {
	last=match;
	match=_u8_strchr(end,needle,&end);
	n--;}}
    return last;}
  /* There should be a good way to handle the n<0 case, but I can't
     come up with it now. */
  else if (n==-1) {
    const u8_byte *last=NULL, *end=NULL;
    const u8_byte *match=_u8_strchr(haystack,needle,&end);
    while (match) {
      last=match;
      match= (end) ? (_u8_strchr(end,needle,&end)) : (NULL);}
    return last;}
  else return NULL;
}

U8_EXPORT u8_string u8_strstr(u8_string haystack,u8_string needle,int n)
{
  CHECK_NULL_STRING(haystack,"u8_strstr/haystack",NULL);
  CHECK_NULL_STRING(needle,"u8_strstr/needle",NULL);
  u8_string result=NULL;
  size_t needle_len=strlen(needle);
  if (n>=0) {
    const u8_byte *end=NULL, *last=NULL;
    const u8_byte *match=strstr(haystack,needle);
    while (match) {
      if (n==0) return match;
      else {
	last=match;
	match=strstr(match+needle_len,needle);
	n--;}}
    if (n==0) return last; else return NULL;}
  /* There should be a good way to handle the n<0 case, but I can't
     come up with it now. */
  else if (n==-1) {
    const u8_byte *last=NULL, *end=NULL;
    const u8_byte *match=strstr(haystack,needle);
    while (match) {
      last=match;
      match=strstr(match+needle_len,needle);}
    return last;}
  else return NULL;
}

#define lsgetc(string)  (u8_tolower(u8_sgetc(&(string))))
#define bsgetc(string)  (u8_base_char(u8_sgetc(&(string))))
#define lbsgetc(string) (u8_tolower(u8_base_char(u8_sgetc(&(string)))))

U8_EXPORT u8_string u8_strcasestr(u8_string haystack,u8_string needle)
{
  CHECK_NULL_STRING(haystack,"u8_strcasestr/haystack",NULL);
  CHECK_NULL_STRING(needle,"u8_strcasestr/needle",NULL);
  u8_string match_needle=needle, scan=haystack;
  u8_char start_char=lsgetc(match_needle);
  while (*scan) {
    u8_string start=scan;
    u8_char ch=lsgetc(scan);
    if (ch == start_char) {
      u8_string needle_scan=match_needle;
      while ( (*scan) && (*needle_scan) ) {
	u8_char match_char=lsgetc(needle_scan);
	ch=lsgetc(scan);
	if (ch != match_char) break;
	else if (*needle_scan=='\0')
	  /* If you're at the end of the needle, you've found it. */
	  return start;
	else continue;}
      /* If you left the loop, it failed, so go back, swallow the
	 character and advance. */
      scan=start; u8_sgetc(&scan);
      continue;}}
  return NULL;
}

U8_EXPORT u8_string u8_strbasestr(u8_string haystack,u8_string needle)
{
  CHECK_NULL_STRING(haystack,"u8_strcasestr/haystack",NULL);
  CHECK_NULL_STRING(needle,"u8_strcasestr/needle",NULL);
  u8_string match_needle=needle, scan=haystack;
  u8_char start_char=bsgetc(match_needle);
  while (*scan) {
    u8_string start=scan;
    u8_char ch=bsgetc(scan);
    if (ch == start_char) {
      u8_string needle_scan=match_needle;
      while ( (*scan) && (*needle_scan) ) {
	u8_char match_char=bsgetc(needle_scan);
	ch=bsgetc(scan);
	if (ch != match_char) break;
	else if (*needle_scan=='\0')
	  /* If you're at the end of the needle, you've found it. */
	  return start;
	else continue;}
      /* If you left the loop, it failed, so go back, swallow the
	 character and advance. */
      scan=start; u8_sgetc(&scan);
      continue;}}
  return NULL;
}

U8_EXPORT u8_string u8_strcasebasestr(u8_string haystack,u8_string needle)
{
  CHECK_NULL_STRING(haystack,"u8_strcasestr/haystack",NULL);
  CHECK_NULL_STRING(needle,"u8_strcasestr/needle",NULL);
  u8_string match_needle=needle, scan=haystack;
  u8_char start_char=lbsgetc(match_needle);
  while (*scan) {
    u8_string start=scan;
    u8_char ch=lbsgetc(scan);
    if (ch == start_char) {
      u8_string needle_scan=match_needle;
      while ( (*scan) && (*needle_scan) ) {
	u8_char match_char=lbsgetc(needle_scan);
	ch=lbsgetc(scan);
	if (ch != match_char) break;
	else if (*needle_scan=='\0')
	  /* If you're at the end of the needle, you've found it. */
	  return start;
	else continue;}
      /* If you left the loop, it failed, so go back, swallow the
	 character and advance. */
      scan=start; u8_sgetc(&scan);
      continue;}}
  return NULL;
}

U8_EXPORT u8_string u8_strstrs(u8_string haystack,u8_string needles[],int order)
{
  CHECK_NULL_STRING(haystack,"u8_strstrs/haystack",NULL);
  u8_string result=NULL;
  u8_string *scan=needles; while (*scan) {
    u8_string needle=*needles++;
    u8_string match=strstr(haystack,needle);
    if (match==NULL) {}
    else if (order == 0)
      return match;
    else if (result==NULL)
      result=match;
    else if ((order>0)?(match>result):(match<result))
      result=match;
    else {}}
  return result;
}

/* Portable ITOA for use in signal handling */

U8_EXPORT char *itoa_helper(unsigned long long int num,char *buf,
			    int base,const char *digits,
			    unsigned long long int mult)
{
  char *write=buf;
  if (num==0) {
    *write++='0'; *write++='\0';
    return buf;}
  unsigned long long int reduce=num;
  int started=0; while (mult>0) {
    int weight=reduce/mult;
    if (started) *write++=digits[weight];
    else if (weight) {
      *write++=digits[weight];
      started=1;}
    else {}
    reduce=reduce%mult;
    mult=mult/base;}
  *write++='\0';
  return buf;
}

U8_EXPORT char *u8_itoa10(long long int num,char outbuf[32])
{
  char *write=outbuf;
  if (num<0) {*write++='-'; num=-num;}
  return itoa_helper(num,write,10,"0123456789",
		     1000000000000000000LL);
}

U8_EXPORT char *u8_uitoa10(unsigned long long int num,char outbuf[32])
{
  return itoa_helper(num,outbuf,10,"0123456789",
		     1000000000000000000LL);
}

U8_EXPORT char *u8_uitoa8(unsigned long long int num,char outbuf[32])
{
  return itoa_helper(num,outbuf,8,"01234567",
		     01000000000000000000000LL);
}

U8_EXPORT char *u8_uitoa16(unsigned long long int num,char outbuf[32])
{
  return itoa_helper(num,outbuf,16,"0123456789ABCDEF",
		     0x1000000000000000LL);
}

/* Initialization function (just records source file info) */

U8_EXPORT void u8_init_stringfns_c()
{
  u8_register_source_file(_FILEINFO);
}

