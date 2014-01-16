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

#include "libu8/libu8io.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8filefns.h"
#include "libu8/u8ctype.h"
#include <stdio.h>
#include <ctype.h>

#include "charmaps.h"

u8_condition u8_UnRepresentedCharacter=_("Encoding can't represent character");
u8_condition u8_UnknownEncoding=_("Can't find named encoding");
u8_condition u8_BadHexString=_("Bad hexadecimal representation (length)");
u8_condition u8_BadHexChar=_("Bad hexadecimal representation (character)");

#ifndef U8_ENCODINGS_DIR
#define U8_ENCODINGS_DIR "/usr/share/libu8/encodings"
#endif

struct U8_TEXT_ENCODING *encodings, *utf8_encoding=NULL, *ascii_encoding=NULL;
struct U8_TEXT_ENCODING *latin0_encoding, *latin1_encoding=NULL;

static char *encname_aliases="+LATIN0:ISO885915;+ISOLATIN0:ISO885915;+LATIN1:ISO88591;+ISOLATIN1:ISO88591;+LATIN2:ISO88592;+ISOLATIN2:ISO88592;+LATIN3:ISO88593;+ISOLATIN3:ISO88593;+ISOLATIN3:ISO88593;+LATIN4:ISO88594;+ISOLATIN4:ISO88594;+ISOLATIN4:ISO88594;+CYRILLIC:ISO88595;+ARABIC:ISO88596;+GREEK:ISO88597;+HEBREW:ISO88598;+ISOHEBREW:ISO88598;LATIN6:ISO885910;ISOLATIN6:ISO885910;ISOLATIN7:ISO885913;LATIN8:ISO885914;ISOLATIN8:ISO885914;LATIN9:ISO885915;ISOLATIN9:ISO885915;";
 
typedef int xchar;

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

/* Looking up encoding names */

/* encoding names are compared by ignoring non-alphabetic
   characters and matching regardless of case. */
static int compare_encoding_names(char *name1,char *name2)
{
  char *scan1=name1, *scan2=name2;
  while ((*scan1) && (*scan2))
    if (*scan1 == *scan2) {scan1++; scan2++;}
    else if (tolower(*scan1) == tolower(*scan2)) {scan1++; scan2++;}
    else if ((*scan1 == '-') || (*scan1 == '_') || (*scan1 == '/') || (*scan1 == ' '))
      scan1++;
    else if ((*scan2 == '-') || (*scan2 == '_') ||
	     (*scan2 == '/') || (*scan2 == ' '))
      scan2++;
    else return 0;
  while ((*scan1) && (isspace(*scan1))) scan1++;
  while ((*scan2) && (isspace(*scan2))) scan2++;
  if (*scan1 == *scan2) return 1;
  else return 0;
}

static struct U8_TEXT_ENCODING *lookup_encoding_name(char *name)
{
  struct U8_TEXT_ENCODING *scan=encodings;
  if (name == NULL) return NULL;
  else while (scan) {
    char **names=scan->names;
    while (*names)
      if (compare_encoding_names(*names,name)) return scan;
      else names++;
    scan=scan->next;}
  return NULL;
}

/* This assumes that the name does not contain any internal NULs */
u8_string standardize_encoding_name(u8_string string)
{
  struct U8_OUTPUT out; int c;
  U8_INIT_OUTPUT(&out,128);
  while ((c=u8_sgetc(&string))>0)
    if ((c<0x80) && (isalnum(c))) {
      int upper=toupper(c);
      u8_putc(&out,upper);}
  if (out.u8_outptr-out.u8_outbuf<64) {
    char skey[200], *alias;
    sprintf(skey,"+%s:",out.u8_outbuf);
    alias=strstr(encname_aliases,skey);
    if (alias) {
      char *start=strchr(alias,':')+1, *end=strchr(start,';');
      strncpy(out.u8_outbuf,start,(end-start));
      out.u8_outbuf[end-start]='\0';}}
  return out.u8_outbuf;
}

/* Defining encodings */

static int compute_flags(struct U8_MB_MAP *chset,int size);

static int charset_order(const void *vx,const void *vy)
{
  struct U8_MB_MAP *x=(struct U8_MB_MAP *)vx;
  struct U8_MB_MAP *y=(struct U8_MB_MAP *)vy;
  int ix=x->from, iy=y->from;
  if (ix == iy) return 0;
  else if (ix < iy) return -1;
  else return 1;
}

static void sort_charset(struct U8_MB_MAP *chset,int size)
{
  qsort(chset,size,sizeof(struct U8_MB_MAP),charset_order);
}

static struct U8_MB_MAP *invert_charset(struct U8_MB_MAP *input,int size)
{
  struct U8_MB_MAP *inv=u8_alloc_n(size,struct U8_MB_MAP);
  int i=0; while (i < size) {
    inv[i].to=input[i].from; inv[i].from=input[i].to; i++;}
  sort_charset(inv,size);
  return inv;
}

static int compute_flags(struct U8_MB_MAP *chset,int size)
{
  int includes_ascii=1, linear=1, i=1, j=1;
  sort_charset(chset,size);
  if (size >= 256) {
    while (i < 128) {
      if ((chset[i].to != i)||(chset[i].from != i)) {
	includes_ascii=0; break;}
      else i++;}}
  while (j < size)
    if (chset[j].from != j) {
      linear=0; break;}
    else j++;
  if ((includes_ascii) && (linear))
    return ((U8_ENCODING_INCLUDES_ASCII) |
	    (U8_ENCODING_IS_LINEAR));
  else if (includes_ascii) return (U8_ENCODING_INCLUDES_ASCII);
  else if (linear) return (U8_ENCODING_IS_LINEAR);
  else return 0;
}

/** Defining new encodings **/

static void add_alias(u8_encoding e,char *name)
{
  char **names=e->names; int len=0;
  while (*names)
    if (compare_encoding_names(*names,name)) return;
    else {names++; len++;}
  names=u8_realloc(e->names,sizeof(char *)*(len+2));
  e->names=names;
  names[len]=u8_strdup(name); names[len+1]=NULL;
}

U8_EXPORT
/* u8_define_encoding:
     Arguments: a name, a pointer to a charset, 
        a wide-char to multi-byte conversion function,
        a multi-byte to wide-char conversion function,
        and a set of flags.
     Returns: 1 if the map was used, zero if it wasn't (mapping was already defined)

Defines an encoding with a name and the associated properties.  If an
encoding with the give properties already exists, the name is added to
that encoding structure. */
u8_encoding u8_define_encoding
  (char *name,struct U8_MB_MAP *charset,int size,
   uc2mb_fn uc2mb,mb2uc_fn mb2uc,int flags)
{
  struct U8_TEXT_ENCODING *scan=encodings;
  while (scan) 
    if ((scan->charset == charset) && (scan->flags == flags) &&
	(scan->uc2mb == uc2mb) && (scan->mb2uc == mb2uc)) {
      add_alias(scan,name);
      return scan;}
    else scan=scan->next;
  scan=lookup_encoding_name(name);
  if (scan) {
    u8_log(LOG_WARNING,NULL,_("Text encoding `%s' already exists"),name);
    return scan;}
  scan=u8_alloc(struct U8_TEXT_ENCODING);
  scan->names=u8_alloc_n(2,char *);
  scan->names[0]=u8_strdup(name); scan->names[1]=NULL;
  if (size) {
    scan->charset=charset; scan->charset_size=size;
    sort_charset(charset,size);
    scan->charset_inv=invert_charset(charset,size);}
  else {scan->charset=NULL; scan->charset_inv=NULL;}
  scan->uc2mb=uc2mb; scan->mb2uc=mb2uc;
  scan->flags=flags; scan->next=encodings; encodings=scan;
  return scan;
}

/* Loading encodings */

/* This loads a unicode consortium format character encoding */
u8_encoding load_unicode_consortium_encoding(char *name,FILE *f)
{
  char buf[512];
  struct U8_MB_MAP *map=u8_alloc_n(256,struct U8_MB_MAP);
  int size=0, limit=256;
  while (fgets(buf,512,f) != NULL) {
    int from, to;
    if (sscanf(buf,"0x%x\t0x%x",&from,&to) == 2) {
      if (size >=limit) {
	map=u8_realloc_n(map,limit+256,struct U8_MB_MAP);
	limit=limit+256;}
      map[size].from=from; map[size].to=to; size++;}}
  u8_fclose(f);
  return
    u8_define_encoding(name,map,size,NULL,NULL,compute_flags(map,size));
}

static unsigned int parse_seq(char *start,char *end);

u8_encoding load_charmap_encoding(char *name,FILE *f)
{
  /* We don't actually parse the headers */
  char buf[512]; char **aliases=u8_alloc_n(64,char *);
  struct U8_MB_MAP *map=u8_alloc_n(256,struct U8_MB_MAP);
  int size=0, limit=256, n_aliases=0, max_aliases=64;
  /* Find where the charmap starts */
  while (fgets(buf,512,f) != NULL)
    if (strcmp(buf,"CHARMAP\n") == 0) break;
    else if (strncmp(buf,"<code_set_name> ",16) == 0) {
      char *copy=u8_strdup(buf+16); int len=strlen(copy);
      if ((strcmp(name,buf+16)) == 0) {
	if (n_aliases >= max_aliases) {
	  aliases=u8_realloc_n(aliases,max_aliases*2,u8_charstring);
	  max_aliases=max_aliases*2;}
	if (copy[len] == '\n') copy[len]=0;
	aliases[n_aliases++]=copy;}
      else u8_free(copy);}
    else if (strncmp(buf,"% alias ",8) == 0) {
      char *copy=u8_strdup(buf+8); int len=strlen(copy);
      if (n_aliases >= max_aliases) {
	aliases=u8_realloc_n(aliases,max_aliases*2,u8_charstring);
	max_aliases=max_aliases*2;}
      if (copy[len] == '\n') copy[len]=0;
      aliases[n_aliases++]=copy;}
    else continue;
  /* Read the entries */
  while (fgets(buf,512,f) != NULL) {
    char *seq_start=strstr(buf,"/x"), *seq_end=NULL, *code_start=NULL;
    unsigned int from, to;
    if (seq_start) {
      char *ns=strchr(seq_start,' '), *nt=strchr(seq_start,'\t');;
      if (ns == NULL) seq_end=nt;
      else if (nt == NULL) seq_end=ns;
      else if (ns < nt) seq_end=ns; else seq_end=nt;}
    if (seq_end) code_start=strstr(buf,"<U");
    if (code_start == NULL) {
      if (strncmp(buf,"END CHARMAP",11) == 0) break;
      else continue;}
    from=parse_seq(seq_start,seq_end);
    sscanf(code_start+2,"%x>",&to);
    if (size >=limit) {
      map=u8_realloc_n(map,limit+256,struct U8_MB_MAP);
      limit=limit+256;}
    map[size].from=from; map[size].to=to; size++;}
  u8_fclose(f);
  {
    int i=0, flags=compute_flags(map,size);
    u8_encoding e=u8_define_encoding(name,map,size,NULL,NULL,flags);
    while (i < n_aliases) {
      add_alias(e,aliases[i]); u8_free(aliases[i]); i++;}
    u8_free(aliases);
    return e;}
}

/* This reads the byte sequence for a charmap file */
static unsigned int parse_seq(char *start,char *end)
{
  if (end-start == 4) {
    int one; sscanf(start,"/x%2x",&one); return one;}
  else if (end-start == 8) {
    int one, two;
    sscanf(start,"/x%2x/x%2x",&one,&two);
    return (one<<8)+two;}
  else if (end-start == 12) {
    int one, two, three;
    sscanf(start,"/x%2x/x%2x/x%2x",&one,&two,&three);
    return (one<<16)+(two<<8)+three;}
  else if (end-start == 16) {
    int one, two, three, four;
    sscanf(start,"/x%2x/x%2x/x%2x/x%2x",&one,&two,&three,&four);
    return (one<<24)+(two<<16)+(three<<8)+four;}
  else {
    fprintf(stderr,"Too many bytes in charmap entry");
    return 0;}
}

U8_EXPORT
/* u8_load_encoding:
     Arguments: a name and a filename
     Returns: void

Defines a text encoding based on a text file of byte sequence to
unicode mappings.  This interprets the standard mappings files provided
by the Unicode consortium at ftp://ftp.unicode.org/Public/MAPPINGS/.
*/
u8_encoding u8_load_encoding(char *name,char *file)
{
  FILE *f=fopen(file,"r"); char buf[512], *rbuf; 
  if (f == NULL) return NULL;
  rbuf=fgets(buf,512,f);
  if (rbuf==NULL) {fclose(f); return NULL;}
  fseek(f,0,SEEK_SET);
  if (strncmp(buf,"<code_set_name>",strlen("<code_set_name")) == 0)
    return load_charmap_encoding(name,f);
  else return load_unicode_consortium_encoding(name,f);
}

static struct U8_TEXT_ENCODING *try_to_load_encoding(char *name)
{
  struct U8_TEXT_ENCODING *e=NULL;
  char *envpath=getenv("U8_ENCODINGS");
  char *std=standardize_encoding_name(name);
  if (envpath) {
    char *path=u8_malloc(strlen(std)+strlen(envpath)+2);
    strcpy(path,envpath); strcat(path,"/"); strcat(path,std);
    if (u8_file_existsp(path)) {
      e=u8_load_encoding(name,path);
      if (e == NULL)
	fprintf(stderr,"The file '%s' failed to provide the encoding %s\n",
		path,name);
      else add_alias(e,name);
      u8_free(path);}}
  if (e) {u8_free(std); return e;}
  else {
    char *path=u8_malloc(strlen(std)+strlen(U8_ENCODINGS_DIR)+2);
    strcpy(path,U8_ENCODINGS_DIR); strcat(path,"/"); strcat(path,std);
    if (u8_file_existsp(path)) {
      e=u8_load_encoding(name,path);
      if (e == NULL)
	fprintf(stderr,"The file '%s' failed to provide the encoding %s\n",
		path,name);
      else add_alias(e,name);}
    u8_free(path); u8_free(std);
    if (e) return e;
    else if (u8_file_existsp(name))
      e=u8_load_encoding(name,name);
    if (e) return e;
    else {
      fprintf(stderr,"Can't locate encoding %s\n",name);
      return e;}}
}

U8_EXPORT
/* u8_get_encoding:
    Arguments: an ASCII string
    Returns: a pointer to an U8_TEXT_ENCODING struct

  This gets the structure describing a particular encoding given
its ASCII name.
*/
struct U8_TEXT_ENCODING *u8_get_encoding(char *name)
{
  if (name == NULL) return NULL;
  else {
    struct U8_TEXT_ENCODING *encoding=lookup_encoding_name(name);
    if (encoding) return encoding;
    else return try_to_load_encoding(name);}
}

static struct U8_TEXT_ENCODING *default_encoding=NULL;

U8_EXPORT
/* u8_get_default_encoding:
    Arguments: none
    Returns: a pointer to an U8_TEXT_ENCODING struct
*/
struct U8_TEXT_ENCODING *u8_get_default_encoding()
{
  char *from_env;
  if (default_encoding) return default_encoding;
  else from_env=getenv("LC_CTYPE");
  if (from_env) {
    struct U8_TEXT_ENCODING *encoding=u8_get_encoding(from_env);
    if (encoding) {
      default_encoding=encoding;
      return encoding;}}
  from_env=getenv("LANG");
  if (from_env) {
    char *encstart=strchr(from_env,'.');
    if (encstart) {
      struct U8_TEXT_ENCODING *encoding=u8_get_encoding(encstart+1);
      if (encoding) {
	default_encoding=encoding;
	return encoding;}}}
  default_encoding=utf8_encoding;
  return default_encoding;
}

U8_EXPORT int u8_set_default_encoding(char *name)
{
  struct U8_TEXT_ENCODING *e=u8_get_encoding(name);
  if (e==NULL) {
    u8_seterr(u8_UnknownEncoding,"u8_set_default_encoding",u8_strdup(name));
    return -1;}
  else if (e==default_encoding) return 0;
  else {
    default_encoding=e;
    return 1;}
}


/* Guessing encodings */

U8_EXPORT struct U8_TEXT_ENCODING *u8_guess_encoding(unsigned char *buf)
{
  u8_byte *code_start, *code_end, codename[128];
  if ((code_start=strstr(buf,"coding:"))) 
    code_start=code_start+7;
  else if ((code_start=strstr(buf,"charset=")))
    code_start=code_start+8;
  else return NULL;
  while ((*code_start)&&(isspace(*code_start))) code_start++;
  if (!(isalpha(*code_start))) return NULL;
  else {
    u8_byte *scan=code_start; int c=u8_sgetc(&scan);
    while (u8_isspace(c)) {
      code_start=scan; c=u8_sgetc(&scan);}
    if (c<0) return NULL;
    code_end=scan;
    while ((c>0)&&((u8_isalnum(c))||(c=='-')||(c=='_')||(c=='.'))) {
      code_end=scan;  c=u8_sgetc(&scan);}}
  strncpy(codename,code_start,code_end-code_start);
  codename[code_end-code_start]='\0';
  return u8_get_encoding(codename);
}

/** Charset primitives **/

static int mb_lookup_code(int code,struct U8_MB_MAP *map,int size)
{
  int bot=0, top=size-1;
  while (top >= bot) {
    int split=bot+(top-bot)/2;
    if (code == map[split].from) return map[split].to;
    else if (code < map[split].from) top=split-1;
    else bot=split+1;}
  return -1;
}

/* This function uses a lookup table to convert an external encoding
   into a unicode code point. */
static int table_mb2uc
  (xchar *o,unsigned char *s,size_t n,struct U8_TEXT_ENCODING *e)
{
  /* The simplest and most common case (most or all of the latin and ISO-8859 encodings) */
  if ((e->flags)&(U8_ENCODING_IS_LINEAR)) {
    *o=(e->charset)[*s].to; return 1;}
  else {
    /* The more complicated case tries to read a word */
    int i=0, size=0, code=0, try, n_bytes=((n > 4) ? 4 : n);
    while (i < n_bytes) {
      code=(code<<8)|(*s); s++; size++;
      try=mb_lookup_code(code,e->charset,e->charset_size);
      if (try >= 0) {*o=try; return size;}
      else i++;}
    return -1;}
}
  
/* This function uses a lookup table to convert a unicode code point
   to an external encoding. */
static int table_uc2mb
  (unsigned char *o,xchar ch,struct U8_TEXT_ENCODING *e)
{
  int code=mb_lookup_code(ch,e->charset_inv,e->charset_size);
  if (code < 0) return -1;
  else if (code < 0x100) {*o=code; return 1;}
  else if (code < 0x10000) {
    o[0]=(code&0xFF00)>>8; o[1]=(code&0xFF); return 2;}
  else if (code < 0x1000000) {
    o[0]=(code&0xFF0000)>>16; o[1]=(code&0xFF00)>>8; o[2]=(code&0xFF);
    return 3;}
  else {
    o[0]=(code&0xFF000000)>>24; o[1]=(code&0xFF0000)>>16;
    o[2]=(code&0xFF00)>>8; o[3]=(code&0xFF);
    return 4;}
}


/* MB Interpret */

/* This function reads an encoded unicode code point from a buffer, converting
   as neccessary. */
static int encgetc(struct U8_TEXT_ENCODING *e,
		   struct U8_MB_MAP *charset,
		   int is_linear,int includes_ascii,
		   unsigned char **scan,unsigned char *end)
{
  /* If we have no encoding, we assume UTF-8, which is either ascii
     or uses u8_sgetc */
  if ((e==NULL) || (e==utf8_encoding))
    if (**scan<0x80) return *((*scan)++);
    else if (get_utf8_size(**scan)>(end-*scan))
      return -1;
    else if (e) return u8_sgetc_lim(scan,end);
    else if (u8_validptr(*scan)) return u8_sgetc_lim(scan,end);
    else
      /* If the string isn't valid UTF-8, return the byte values as though
	 it were latin1 */
      return *((*scan)++);
  /* Linear table lookup just uses the byte of input as an offset
     into the character map, and also handles the special case
     of ASCII subsets. */
  else if ((e->charset)&&(e->flags&(U8_ENCODING_IS_LINEAR)))
    if ((**scan<0x80) && (e->flags&(U8_ENCODING_INCLUDES_ASCII)))
      return *((*scan)++);
    else {
      int byte=*((*scan)++);
      return e->charset[byte].to;}
  /* Non-linear table lookup. */
  else if (e->charset) {
    int len=end-*scan;
    xchar c, l;
    if (len>16) len=16;
    l=table_mb2uc(&c,*scan,len,e);
    if (l<0) return l;
    *scan=*scan+l;
    return c;}
  else if (e->mb2uc) {
    int len=end-*scan;
    xchar c; int l;
    if (len>16) len=16;
    l=e->mb2uc(&c,*scan,16);
    if (l<0) return l;
    *scan=*scan+l;
    return c;}
  else return *((*scan)++);
}

U8_EXPORT
/* u8_convert:
     Arguments: a string stream, start and end pointers to an 8BIT text string,
and a pointer to the text encoding for the string
     Returns: the number of characters read
*/
int u8_convert
  (struct U8_TEXT_ENCODING *e,int convert_crlfs,
   struct U8_OUTPUT *out,unsigned char **scan,unsigned char *end)
{
  u8_byte *start=*scan;
  struct U8_MB_MAP *charset=((e) ? (e->charset) : (NULL));
  int includes_ascii=((e) ? (e->flags&U8_ENCODING_INCLUDES_ASCII) : (1));
  int is_linear=((e) ? (e->flags&U8_ENCODING_IS_LINEAR) : (0));
  int chars_read=0;
  if (end == NULL) end=start+strlen(start);
  while (*scan<end) {
    u8_byte *last_scan=*scan; int retval=0;
    int c=encgetc(e,charset,includes_ascii,is_linear,scan,end);
    if (c<0) return c;
    else if ((convert_crlfs) && (c=='\r')) {
      int nc=encgetc(e,charset,includes_ascii,is_linear,scan,end);
      if (nc<0) {*scan=last_scan; break;}
      else if (nc=='\n') u8_putc(out,'\n');
      else {retval=u8_putc(out,'\r'); retval=u8_putc(out,nc);}}
    else retval=u8_putc(out,c);
    if (retval<0) return retval;
    chars_read++;}
  return chars_read;
}

U8_EXPORT
/* u8_make_string:
     Arguments: start and end pointers to an 8BIT string representation
       and a text encoding
     Returns: a utf8 encoded string
If the end pointer is NULL, it is set to the end of the string.
*/
u8_string u8_make_string
  (struct U8_TEXT_ENCODING *e,u8_byte *start,u8_byte *end)
{
  struct U8_OUTPUT out;
  U8_INIT_OUTPUT(&out,((end-start)+(end-start)/4));
  u8_convert(e,1,&out,&start,end);
  return out.u8_outbuf;
}

U8_EXPORT
/* u8_localize:
     Arguments: a utf8 encoded string and a text encoding
     Returns: a regular string

  Returns an 8BIT string encoded using the text encoding.
*/
unsigned char *u8_localize
  (struct U8_TEXT_ENCODING *e,
   u8_byte **scanner,u8_byte *end,
   int escape_char,int crlf,u8_byte *buf,int *size_loc)
{
  struct U8_MB_MAP *inv; int outsize;
  unsigned char *write, *write_limit;
  u8_byte *scan=*scanner;
  int u8len, buf_mallocd, bufsiz, in_crlf=0;
  if (end==NULL) {
    u8len=strlen(scan); end=scan+u8len;}
  else u8len=end-scan;
  if (buf) {
    write=buf; bufsiz=*size_loc; write_limit=write+bufsiz;
    buf_mallocd=0;}
  else {
    bufsiz=2*u8len;
    write=buf=u8_malloc(bufsiz);
    write_limit=buf+bufsiz;
    buf_mallocd=1;}
  while ((scan<end) && ((buf_mallocd) || (write+8<write_limit))) {
    u8_byte *last=scan;
    int ch;
    /* Here's the trick.  If you hit a newline and crlf is non-zero,
       either you're in the middle of outputting a crlf sequence or you
       should start one.  If in_crlf is true, clear it and output a \n,
       advancing the scanner; if in_crlf is false, set it, output an \r,
       and reset the pointer.  Note that you have to reset the pointer
       to stay in the loop if you're at the end of the string. */
    if (crlf==0) ch=u8_sgetc_lim(&scan,end);
    else if (in_crlf) {ch='\n'; in_crlf=0; scan++;}
    else {
      ch=u8_sgetc_lim(&scan,end);
      if (ch == '\n') {ch='\r'; in_crlf=1; scan=last;}}
    /* Grow the buffer if you can and if its neccessary.
       Note that if you can't grow the buffer and it is neccessary,
       you would have dropped out of the loop. */
    if ((buf_mallocd) && ((write+8)>=write_limit)) {
      int write_off=write-buf;
      buf=u8_realloc(buf,bufsiz+1024); bufsiz=bufsiz+1024;
      write_limit=buf+bufsiz; write=buf+write_off;}
    if (ch<0) {ch=0; scan++;}
    else if ((e==NULL) || (e == utf8_encoding)) 
      if (ch<0x80) {*write++=ch; *write=0;}
      else {
	int n_bytes=scan-last;
	memcpy(write,last,n_bytes);
	write=write+n_bytes; *write=0;}
    else if ((ch<0x80)&&((e->flags)&(U8_ENCODING_INCLUDES_ASCII))) {
      *write++=ch; *write=0;}
    else if ((inv=e->charset_inv)&&
	     ((outsize=table_uc2mb(write,(xchar)ch,e))>=0)) {
      write=write+outsize;}
    else if ((e->uc2mb)&&
	     ((outsize=e->uc2mb(write,(xchar)ch))>=0)) {
      write=write+outsize;}
    /* If we get here, we know we don't have a native encoding. */
    else if (((escape_char == '\\') ||(escape_char == '&') ||
	      (escape_char == 'x')) &&
	     ((e->flags)&(U8_ENCODING_INCLUDES_ASCII))) {
      if (escape_char == '&') {
	sprintf(write,"&#%d;",ch);
	write=write+strlen(write);}
      else if (escape_char == '\\')
	if (ch < 0x8000) {
	  sprintf(write,"\\u%04x",ch);
	  write=write+6;}
	else {
	  sprintf(write,"\\U%08x",ch);
	  write=write+10;}
      else if (escape_char == 'x') {
	sprintf(write,"\\x%x;",ch);
	write=write+strlen(write);}
      else {}}
    /* There is another case we could handle here, which is encoding escapes
       in character sets which don't include ASCII but do have representations
       of all the characters used for the encoding.  But we don't currently
       do that. */
    else {
      uc2mb_fn uc2mb=e->uc2mb; int l;
      if (uc2mb == NULL) uc2mb=(uc2mb_fn)wctomb;
      l=uc2mb(write,(xchar)ch); write=write+l;}}
  if (size_loc) *size_loc=write-buf;
  *write++='\0'; /* Null terminate it */
  *scanner=scan;
  return buf;
}
  
unsigned char *u8_localize_string
  (struct U8_TEXT_ENCODING *e,u8_byte *start,u8_byte *end)
{
  return u8_localize(e,&start,end,0,0,NULL,NULL);
}

/** some standard encodings **/

/* UTF-8 encoding */
static int utf8towc(xchar *o,u8_byte *s,size_t n)
{
  u8_byte *start=s;
  int size=get_utf8_size(*s);
  if (size == 1) {*o=*s; return 1;}
  else {
    int ch=u8_sgetc(&s);
    if (ch<0) return ch;
    *o=ch;
    return s-start;}
}

static int wctoutf8(u8_byte *o,xchar ch)
{
  u8_byte off[6]={0x00,0xC0,0xE0,0xF0,0xF8,0xFC};
  int i, size=0; u8_byte buf[6];
  if (ch < 0x80) {size=1; buf[0]=(unsigned char)ch;}
  else {
    buf[size++]=(ch&0x3F)|0x80; ch=ch>>6;
    while (ch) {
      buf[size++]=(ch&0x3F)|0x80; ch=ch>>6;}
    buf[size-1]=off[size-1]|buf[size-1];}
  i=size-1; while (i>=0) *o++=buf[i--];
  return size;
}

/* UTF-16 encoding */
static int utf16towc(xchar *o,u8_byte *i,size_t n)
{
  *o=(i[0]<<8)|(i[1]);
  return 2;
}

static int wctoutf16(u8_byte *buf,xchar ch)
{
  buf[0]=(ch>>8); buf[1]=ch&0xff; return 2;
}

static u8_encoding declare_charset(char *name,struct U8_MB_MAP *chset)
{
  int size=chset[0].from;
  sort_charset(chset+1,size);
  return u8_define_encoding
    (name,chset+1,size,NULL,NULL,compute_flags(chset+1,size));
}


/* Mime Conversion */

U8_EXPORT
/* u8_read_quoted_printable
     Arguments: two string pointers and an int pointer
     Returns: a character string
  Converts a quoted_printable string into bytes and deposits its length
  in a size pointer. */
char *u8_read_quoted_printable(char *from,char *to,int *sizep)
{
  char *result=u8_malloc(to-from+1), *read=from, *write=result;
  int size=0;
  while (read < to) 
    if (*read == '=')
      if (read[1] == '\n') read=read+2;
      else if ((read[1] == '\r') && (read[2] == '\n')) read=read+3;
      else {
	char buf[4]; int coded_char;
	buf[0]=read[1]; buf[1]=read[2]; buf[2]=0;
	sscanf(buf,"%x",&coded_char); size++;
	*write++=coded_char; read=read+3;}
    else {*write++=*read++; size++;}
  *write=0; *sizep=size;
  return result;
}

static int encode_base64_byte(char c)
{
  if ((c>='A') && (c<='Z')) return c-'A';
  else if ((c>='a') && (c<='z')) return 26+(c-'a');
  else if ((c>='0') && (c<='9')) return 52+(c-'0');
  else if (c == '+') return 62;
  else if (c == '/') return 63;
  else if (c == '=') return 0;
  else return -1;
}

static int encode_base64_block(char *c)
{
  int first_byte=encode_base64_byte(c[0]);
  if (first_byte<0) return -1;
  else return
	 (first_byte<<18)|
	 ((encode_base64_byte(c[1]))<<12)|
	 ((encode_base64_byte(c[2]))<<6)|
	 ((encode_base64_byte(c[3]))<<0);
}

U8_EXPORT
/* u8_read_base64
     Arguments: two string pointers and an int pointer
     Returns: a character string
  Converts a base64 string into bytes and deposits its length
  in a size pointer. */
unsigned char *u8_read_base64(char *from,char *to,int *sizep)
{
  char *result=u8_malloc(to-from+1), *read=from, *write=result;
  int size=0;
  while (read < to) {
    int bytes=encode_base64_block(read);
    if (bytes < 0) read++;
    else {
      *write++=bytes>>16; *write++=(bytes>>8)&0xFF; *write++=(bytes)&0xFF;
      if (read[2] == '=') {size=size+1; break;}
      else if (read[3] == '=') {size=size+2; break;}
      else {size=size+3; read=read+4;}}}
  *sizep = size;
  return result;
}

#define hexweight(c) \
  ((U8_EXPECT_TRUE(isxdigit(c))) ? \
   ((isdigit(c)) ? (c-'0') : (isupper(c)) ? ((c-'A')+10) : ((c-'a')+10)) : \
   (-1))

U8_EXPORT
/* u8_read_base16
     Arguments: a data pointer and a length (int)
     Returns: an ASCII character string (malloc'd)
  Converts the data into a hex base16 representation,
   returning the representation as a NUL-terminated string.  */
unsigned char *u8_read_base16(char *data,int len_arg,int *result_len)
{
  unsigned int len=((len_arg<0) ? (strlen(data)) : (len_arg));
  if (len==0) {
    *result_len=len;
    return NULL;}
  else {
    unsigned char *scan=data, *limit=data+len;
    unsigned char *result=u8_malloc(len/2);
    unsigned char *write=result;
    *result_len=-1; /* Initial error value */
    if (U8_EXPECT_FALSE(result==NULL)) {
      u8_seterr(u8_MallocFailed,"u8_read_base16",NULL);
      *result_len=-1; return NULL;}
    else while (scan<limit) {
	if ((isspace(*scan))||(ispunct(*scan))) {scan++; continue;}
	else if ((scan+1)<limit) {
	  int hival=hexweight(scan[0]), loval=hexweight(scan[1]);
	  if ((hival<0) || (loval<0)) {
	    u8_seterr(u8_BadHexString,"u8_read_base16",u8_slice(data,data+len));
	    u8_free(result);
	    return NULL;}
	  *write=(hival<<4)|loval;
	  scan=scan+2; write++;}
	else {
	  u8_seterr(u8_BadHexString,"u8_read_base16",u8_slice(data,data+len));
	  u8_free(result);
	  return NULL;}}
    *result_len=len/2;
    return result;}
}

static char base64_codes[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

U8_EXPORT
/* u8_write_base64
     Arguments: a data pointer, a length (int), and a pointer to a result length
     Returns: an ASCII character string
  Converts the data into a base64 representation, returning the representation
   as a NUL-terminated string and storing the length (without the NUL) in
   the provided result length pointer.  */
char *u8_write_base64(unsigned char *data,int len,int *result_len)
{
  unsigned int sixbit;
  unsigned char *scan=data, *limit=data+len;
  unsigned char *result=u8_malloc((((len/3)+1)*4)+1);
  unsigned char *write=result;
  int leftover=len%3;
  while (scan<limit) {
    sixbit=0;
    sixbit=sixbit|((*scan++)<<16);
    if (scan<limit) sixbit=sixbit|((*scan++)<<8);
    if (scan<limit) sixbit=sixbit|((*scan++));
    *write++=base64_codes[((sixbit>>18)&0x3F)];
    *write++=base64_codes[((sixbit>>12)&0x3F)];
    *write++=base64_codes[((sixbit>>6)&0x3F)];
    *write++=base64_codes[((sixbit)&0x3F)];}
  switch (leftover) {
  case 0: break;
  case 1: {
    write[-1]='='; write[-2]='=';}
  case 2: {
    write[-1]='=';}}
  *result_len=write-result; *write++='\0';
  return result;
}

static char base16_codes[]="0123456789ABCDEF";

U8_EXPORT
/* u8_write_base16
     Arguments: a data pointer and a length (int)
     Returns: an ASCII character string (malloc'd)
  Converts the data into a hex base16 representation,
   returning the representation as a NUL-terminated string.  */
char *u8_write_base16(unsigned char *data,int len_arg)
{
  unsigned int len=((len_arg<0) ? (strlen(data)) : (len_arg));
  unsigned char *scan=data, *limit=data+len;
  unsigned char *result=u8_malloc((len*2)+1);
  unsigned char *write=result;
  while (scan<limit) {
    unsigned int ival=*scan++;
    *write++=base16_codes[(ival>>4)&0xF];
    *write++=base16_codes[ival&0xF];}
  *write++='\0';
  return result;
}

static void convert_mime_header_text
  (U8_OUTPUT *out,unsigned char *start,unsigned char *end)
{
  char charset[64], *chstart=start+2, *chend=strchr(chstart,'?');
  unsigned char enc_code=chend[1], *data_start=chend+3;
  unsigned char *rawdata, *rawend, *scan;
  u8_encoding encoding;
  int startlen=out->u8_outptr-out->u8_outbuf, raw_bytes;
  if ((chend-chstart)>=64) encoding=NULL;
  else if (strchr("QqBb",enc_code)==NULL) encoding=NULL;
  else {
    strncpy(charset,chstart,chend-chstart);
    charset[chend-chstart]='\0';
    encoding=u8_get_encoding(charset);}
  if (encoding==NULL) {
    u8_putn(out,start,end-start);
    return;}
  if ((enc_code=='Q') || (enc_code=='q'))
    rawdata=u8_read_quoted_printable(data_start,end,&raw_bytes);
  else rawdata=u8_read_base64(data_start,end,&raw_bytes);
  scan=rawdata; rawend=rawdata+raw_bytes;
  u8_convert(encoding,1,out,&scan,rawend);
  if (scan<end) {
    /* This was malformed, so we don't do the conversion. */
    out->u8_outptr=out->u8_outbuf+startlen; out->u8_outbuf[startlen]='\0';
    u8_putn(out,start,end-start);
    return;}
  /* Free the encoded data we converted. */
  u8_free(rawdata);
}

U8_EXPORT
/* u8_mime_convert:
     Arguments: two string pointers
     Returns: a utf8 string
  Converts character escapes in mime data. */
u8_string u8_mime_convert(char *start,char *end)
{
  U8_OUTPUT out;
  char *scan=start;
  U8_INIT_OUTPUT(&out,256);
  while (scan<end) {
    if ((*scan=='=') && (scan[1]=='?')) {
      char *code_end=strstr(scan,"?=");
      if ((code_end) && (code_end<end)) {
	convert_mime_header_text(&out,scan,code_end);
	scan=code_end+1;}
      else {u8_putc(&out,*scan); scan++;}}
    else {u8_putc(&out,*scan); scan++;}}
  return out.u8_outbuf;
}


/* Initialization */

void u8_init_convert_c()
{
  u8_define_encoding
    ("ASCII",NULL,0,NULL,NULL,U8_ENCODING_INCLUDES_ASCII);
  u8_define_encoding
    ("US-ASCII",NULL,0,NULL,NULL,U8_ENCODING_INCLUDES_ASCII);
  declare_charset("latin0",iso_8859_15_map);
  declare_charset("iso-latin0",iso_8859_15_map);
  declare_charset("iso-8859/15",iso_8859_15_map);
  declare_charset("latin1",iso_8859_1_map);
  declare_charset("iso-latin1",iso_8859_1_map);
  declare_charset("iso-8859/1",iso_8859_1_map);
  declare_charset("latin2",iso_8859_2_map);
  declare_charset("iso-latin2",iso_8859_2_map);
  declare_charset("iso-8859/2",iso_8859_2_map);
  u8_define_encoding("UTF-8",NULL,0,wctoutf8,utf8towc,
		     U8_ENCODING_INCLUDES_ASCII);
  u8_define_encoding("UTF/8",NULL,0,wctoutf8,utf8towc,
		     U8_ENCODING_INCLUDES_ASCII);
  u8_define_encoding("UTF/16",NULL,0,wctoutf16,utf16towc,0);
  u8_define_encoding("UTF-16",NULL,0,wctoutf16,utf16towc,0);
  u8_define_encoding("UCS-2",NULL,0,wctoutf16,utf16towc,0);
  u8_define_encoding("UCS/2",NULL,0,wctoutf16,utf16towc,0);
  
  ascii_encoding=u8_get_encoding("ASCII");
  utf8_encoding=u8_get_encoding("UTF-8");
  latin1_encoding=u8_get_encoding("LATIN-1");
  latin0_encoding=u8_get_encoding("LATIN-0");
  default_encoding=u8_get_encoding("UTF-8");

  u8_register_source_file(_FILEINFO);

}
