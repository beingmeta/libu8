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

#define U8_INLINE_IO 1

#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8streamio.h"
#include "libu8/u8ctype.h"

#include <limits.h>
/* Just for sprintf */
#include <stdio.h>

#ifndef U8_BUF_THROTTLE_POINT
#define U8_BUF_THROTTLE_POINT (1024*1024)
#endif

u8_condition u8_UnexpectedEOD=_("Unexpected EOD"), 
  u8_BadUTF8=_("Invalid UTF-8 encoded text"),
  u8_BadUnicodeChar=_("Invalid Unicode Character"),
  u8_BadUNGETC=_("UNGETC error"),
  u8_NoZeroStreams=_("No zero-length string streams");

int u8_utf8warn=1;

/* Utility functions */

U8_EXPORT void _U8_INIT_OUTPUT_X(u8_output s,int sz,char *buf,int flags)
{
  U8_INIT_OUTPUT_X(s,sz,buf,flags);
}

U8_EXPORT void _U8_INIT_STRING_INPUT(u8_input s,int n,u8_byte *buf)
{
  U8_INIT_STRING_INPUT(s,n,buf);
}

U8_EXPORT void _U8_INIT_INPUT_X(u8_input s,int n,u8_byte *buf,int bits)
{
  U8_INIT_INPUT_X(s,n,buf,bits);
}

U8_EXPORT
/* u8_grow_stream:
     Arguments: a pointer to a string stream and a number of u8_inbuf
     Returns: void

  Grows the data structures for the string stream to include delta
more u8_inbuf
*/
int u8_grow_stream(struct U8_OUTPUT *f,int need)
{
  int n_current=f->u8_outptr-f->u8_outbuf, n_needed=n_current+need+1;
  int current_max=f->u8_bufsz, new_max=current_max;
  while (new_max<=n_needed)
    if (new_max>=U8_BUF_THROTTLE_POINT)
      new_max=new_max+U8_BUF_THROTTLE_POINT;
    else new_max=new_max*2;
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) {
    u8_byte *newptr=u8_realloc(f->u8_outbuf,new_max);
    if (newptr==NULL)
      return f->u8_outlim-f->u8_outptr;
    f->u8_outbuf=newptr;}
  else {
    u8_byte *newu8_inbuf=u8_malloc(new_max);
    if (newu8_inbuf==NULL)
      return f->u8_outlim-f->u8_outptr;
    strcpy(newu8_inbuf,f->u8_outbuf);
    f->u8_streaminfo=f->u8_streaminfo|U8_STREAM_OWNS_BUF;
    f->u8_outbuf=newu8_inbuf;}
  f->u8_outptr=f->u8_outbuf+n_current;
  f->u8_outlim=f->u8_outbuf+new_max;
  f->u8_bufsz=new_max;
  return f->u8_outlim-f->u8_outptr;
}

/* Operations which may flush or fill */

U8_EXPORT int _u8_putc(struct U8_OUTPUT *f,int ch)
{
  unsigned char off[6]={0x00,0xC0,0xE0,0xF0,0xF8,0xFC};
  unsigned char masks[6]={0x7f,0x1F,0x0f,0x07,0x03,0x01};
  int size=0, shift=0;
  u8_byte *write;
  if (ch == 0) size=2;
  else if (ch < 0x80) size=1; 
  else if (ch < 0x800) size=2;
  else if (ch < 0x10000) size=3;
  else if (ch < 0x200000) size=4;
  else if (ch < 0x4000000) size=5;
  else if (ch < 0x80000000) size=6;
  else return u8_reterr(u8_BadUnicodeChar,"u8_putc",NULL);
  if ((f->u8_outptr+size+1>=f->u8_outlim) && (f->u8_flushfn)) 
    f->u8_flushfn(f);
  if (f->u8_outptr+size+1>=f->u8_outlim) 
    u8_grow_stream(f,size);
  if (f->u8_outptr+size+1>=f->u8_outlim) {
    u8_graberr(-1,"u8_putc",NULL);
    return -1;}
  shift=(size-1)*6; write=f->u8_outptr;
  *write++=off[size-1]|(masks[size-1]&(ch>>shift));
  shift=shift-6; size--;
  while (size) {
    *write++=0x80|((ch>>shift)&0x3F); shift=shift-6; size--;}
  *write='\0'; f->u8_outptr=write;
  return 1;
}
U8_EXPORT int _u8_putn(struct U8_OUTPUT *f,u8_string data,int len)
{
  if ((f->u8_outptr+len+1>=f->u8_outlim) && (f->u8_flushfn)) 
    f->u8_flushfn(f);
  if (f->u8_outptr+len+1>=f->u8_outlim) 
    u8_grow_stream(f,len);
  if (f->u8_outptr+len+1>=f->u8_outlim) return -1;
  memcpy(f->u8_outptr,data,len);
  f->u8_outptr=f->u8_outptr+len; *(f->u8_outptr)='\0';
  return len;
}
U8_EXPORT int _u8_getc(struct U8_INPUT *f)
{
  int i, ch, byte, size;
  u8_byte *scan, *start=f->u8_inptr;
  if (f->u8_inptr>=f->u8_inlim) {
    /* Try to get more data */
    if (f->u8_fillfn) f->u8_fillfn(f);
    /* If you can't, just return */
    if (f->u8_inptr>=f->u8_inlim) return -1;}
  byte=*(f->u8_inptr);
  if (byte < 0x80) {
    f->u8_inptr++; return byte;}
  else if (byte < 0xc0) {
    /* Unexpected continuation byte */
    if ((u8_utf8warn)||(f->u8_streaminfo&U8_STREAM_UTF8WARN))
      u8_log(LOG_WARN,u8_BadUTF8,_("Unexpected continuation byte: 0x%2x"),byte);
    (f->u8_inptr)++;
    return 0xFFFD;}
  /* Otherwise, figure out the size and initial byte fragment */
  else if (byte < 0xE0) {size=2; ch=byte&0x1F;}
  else if (byte < 0xF0) {size=3; ch=byte&0x0F;}
  else if (byte < 0xF8) {size=4; ch=byte&0x07;}
  else if (byte < 0xFC) {size=5; ch=byte&0x3;}     
  else if (byte < 0xFE) {size=6; ch=byte&0x1;}
  else { /* Bad data, return the character */
    if ((u8_utf8warn)||(f->u8_streaminfo&U8_STREAM_UTF8WARN))
      u8_log(LOG_WARN,u8_BadUTF8,_("Illegal UTF-8 byte: 0x%2x"),byte);
    f->u8_inptr++;  /* Consume the byte */
    return 0xFFFD;}
  /* Now, we now how many u8_inbuf we need, so we check if we have
     that much data. */
  if (f->u8_inptr+size>f->u8_inlim) /* Not enough data */
    if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_inptr;
      while (n_u8_inbuf<size) {
	if (f->u8_fillfn(f)==0) return -1;
	else n_u8_inbuf=f->u8_inlim-f->u8_inptr;}}
    else return -1;
  else {}
  /* We have enough data, so now we just do a UTF-8 read. */
  i=size-1; f->u8_inptr++; scan=f->u8_inptr;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      if ((u8_utf8warn)||(f->u8_streaminfo&U8_STREAM_UTF8WARN)) {
	char buf[256];
	int j=0; buf[0]='\0'; while (j<size) {
	  char tmp[8]; sprintf(tmp,"%2x",start[j]);
	  if (j>0) strcat(buf," ");
	  strcat(buf,tmp); j++;}
	u8_log(LOG_WARN,u8_BadUTF8,_("Truncated UTF-8 sequence: 0x%s"),buf);}
      f->u8_inptr=scan; /* Consume the truncated byte sequence */
      return 0xFFFD;}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  /* And now we update the data structure */
  f->u8_inptr=scan;
  return ch;
}

U8_EXPORT int u8_probec(struct U8_INPUT *f)
{
  int i, ch, byte, size;
  u8_byte *scan, *start=f->u8_inptr;
  if (f->u8_inptr>=f->u8_inlim) {
    /* Try to get more data */
    if (f->u8_fillfn) f->u8_fillfn(f);
    /* If you can't, just return */
    if (f->u8_inptr>=f->u8_inlim) return -1;}
  byte=*(f->u8_inptr);
  if (byte < 0x80) return byte;
  else if (byte < 0xc0) {   /* Catch this error */
    char buf[16]; sprintf(buf,"0x%02x",byte);
    u8_log(LOG_WARN,u8_BadUTF8,buf);
    return 0xFFFD;}
  /* Otherwise, figure out the size and initial byte fragment */
  else if (byte < 0xE0) {size=2; ch=byte&0x1F;}
  else if (byte < 0xF0) {size=3; ch=byte&0x0F;}
  else if (byte < 0xF8) {size=4; ch=byte&0x07;}
  else if (byte < 0xFC) {size=5; ch=byte&0x3;}     
  else if (byte < 0xFE) {size=6; ch=byte&0x1;}
  else { /* Bad data, return the character */
    return 0xFFFD;}
  /* Now, we now how many u8_inbuf we need, so we check if we have
     that much data. */
  if (f->u8_inptr+size>f->u8_inlim) /* Not enough data */
    if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_inptr;
      while (n_u8_inbuf<size) {
	if (f->u8_fillfn(f)==0) return -1;
	else n_u8_inbuf=f->u8_inlim-f->u8_inptr;}}
    else return -1;
  else {}
  /* We have enough data, so now we just do a UTF-8 read. */
  i=size=1; scan=f->u8_inptr;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      f->u8_inptr=scan;
      return u8_reterr(u8_BadUTF8,"u8_getc",u8_strdup(start));}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  return ch;
}

U8_EXPORT
/* u8_getn:
    Arguments: an input stream, a byte count, and a pointer to a buffer
    Returns: the number of u8_inbuf actually read

    Reads N u8_inbuf of UTF-8 from the input stream and null-terminates it,
      returns the number of u8_inbuf deposited in the buffer, null-terminated.
*/
int _u8_getn(u8_byte *ptr,int n,struct U8_INPUT *f)
{
  /* First, check if we need to get more data into the buffer. */
  if (f->u8_inptr+n>f->u8_inlim) /* Not enough data */
    if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_inptr;
      while (n_u8_inbuf<n) {
	if (f->u8_fillfn(f)==0) return -1;
	else n_u8_inbuf=f->u8_inlim-f->u8_inptr;}}
    else return -1;
  else {}
  /* Check if it worked */
  if (f->u8_inptr+n>f->u8_inlim) return -1;
  /* We have enough data, so now we need to get the size of
     the largest valid UTF-8 string less than *n* u8_inbuf */
  n=u8_validate(f->u8_inptr,n);
  /* Now we just copy the u8_inbuf over */
  strncpy(ptr,f->u8_inptr,n); ptr[n]='\0'; f->u8_inptr=f->u8_inptr+n;
  return n;
}

U8_EXPORT
/* u8_gets_x:
    Arguments: a pointer to a character buffer, it size, an input stream,
               a terminating string, and a pointer to a size value.
    Returns: a string or NULL.

    Reads UTF-8 u8_inbuf UTF-8 from an input stream up until *eos* (which might be,
    for instance, a newline "\n") or stream end and null-terminates the result.
    If buf is provided and has enough space (according to len), it is used
    for the data.  Otherwise, the function returns NULL.  If buf is NULL,
    a string is allocated with u8_malloc and used to store the results.
    In either case, the number of u8_inbuf needed is stored on the indicated
    size pointer.
*/
u8_string u8_gets_x(u8_byte *buf,int len,
		    struct U8_INPUT *f,u8_string eos,
		    int *sizep)
{
  u8_byte *found=NULL, *start=f->u8_inptr; int eof=0;
  *(f->u8_inlim)='\0';
  while ((found=strstr(start,eos))==NULL) {
    int start_pos=f->u8_inlim-f->u8_inptr, retval=0;
    /* Quit if we have length constraints which
       we are already past. */
    if (f->u8_fillfn) retval=f->u8_fillfn(f);
    if (retval==0) {eof=1; break;}
    else if (retval<0) {
      if (sizep) *sizep=retval;
      return NULL;}
    start=f->u8_inptr+start_pos;}
  if (found) {
    int size=(found-f->u8_inptr);
    if (sizep) *sizep=size;
    if ((buf) && (size>len)) return NULL;
    else if (buf==NULL) buf=u8_malloc(size+1);
    u8_getn(buf,size,f);
    /* Advance past the separator */
    f->u8_inptr=f->u8_inptr+strlen(eos);
    return buf;}
  else return NULL;
}

U8_EXPORT
/* u8_ungetc:
    Arguments: an input stream and a unicode character (int)
    Returns: the charcter shoved back or -1 if it fails.

    Puts a character back in the an input stream, so that the next
     read will retrieve it.
*/

int u8_ungetc(struct U8_INPUT *f,int ch)
{
  /* Note that this implementation assumes that the stream has
     not had its buffer compacted.  This is consistent with
     the assumption that the last thing we did to it was a read
     operation buffer which returned after any buffer compaction. */
  if (ch<0x80)
    if ((f->u8_inptr>f->u8_inbuf) && (f->u8_inptr[-1]==ch)) {
      f->u8_inptr--;
      return ch;}
    else {
      char buf[32]; 
      sprintf(buf,"\\U%08x",ch);
      u8_seterr(u8_BadUNGETC,"u8_ungetc",u8_strdup(buf));
      return -1;}
  else {
    struct U8_OUTPUT tmpout; u8_byte buf[16]; int size;
    U8_INIT_FIXED_OUTPUT(&tmpout,16,buf);
    u8_putc(&tmpout,ch); size=tmpout.u8_outptr-tmpout.u8_outbuf;
    if ((f->u8_inptr>f->u8_inbuf+size) &&
	(strncmp(f->u8_inptr-size,tmpout.u8_outbuf,size)==0)) {
      f->u8_inptr=f->u8_inptr-size; return ch;}
    else {
      char buf[32]; 
      sprintf(buf,"\\U%08x",ch);
      u8_seterr(u8_BadUNGETC,"u8_ungetc",u8_strdup(buf));
      return -1;}}
}

U8_EXPORT
/* u8_get_entity:
     Arguments: an input stream
     Result: a unicode pointer or -1
*/
int u8_get_entity(struct U8_INPUT *f)
{
  if ((f->u8_inptr==f->u8_inlim) && (f->u8_fillfn)) f->u8_fillfn(f);
  if (f->u8_inptr==f->u8_inlim) return -1;
  else {
    u8_byte *start=f->u8_inptr, *end=NULL;
    int code=u8_parse_entity(start,&end);
    if ((code<0) && (end) && (f->u8_fillfn)) {
      /* If code<0 and end was set, that meant it got started but
	 didn't finish, so we call the u8_fillfn and try again. */
      f->u8_fillfn(f);
      code=u8_parse_entity(start,&end);}
    else {}
    if (code>=0) {
      f->u8_inptr=end; return code;}
    else return -1;}
}


/* Opening string input and output streams */

U8_EXPORT
/* u8_open_input_string:
     Arguments: a utf-8 string
     Returns: an input stream

This mallocs and initializes an input stream to read from a particular string. 
*/
U8_INPUT *u8_open_input_string(u8_string input)
{
  U8_INPUT *instream=u8_alloc(struct U8_INPUT);
  U8_INIT_STRING_INPUT(instream,-1,input);
  return instream;
}

U8_EXPORT void u8_flush_input(struct U8_INPUT *f)
{
  f->u8_inlim=f->u8_inptr=f->u8_inbuf;
}

U8_EXPORT
/* u8_close_input:
     Arguments: a size
     Returns: an input stream

This mallocs and initializes an input stream to read from a particular string. 
*/
int u8_close_input(u8_input in)
{
  if (in->u8_closefn) return in->u8_closefn(in);
  else return 0;
}

U8_EXPORT int _u8_close_soutput(struct U8_OUTPUT *f)
{
  if (f->u8_flushfn) f->u8_flushfn((U8_OUTPUT *)f);
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_outbuf);
  if (f->u8_streaminfo&U8_STREAM_MALLOCD) u8_free(f);
  return 1;
}

U8_EXPORT
/* u8_open_output_string:
     Arguments: a size
     Returns: an input stream

This mallocs and initializes an input stream to read from a particular string. 
*/
U8_OUTPUT *u8_open_output_string(int initial_size)
{
  U8_OUTPUT *outstream=u8_alloc(struct U8_OUTPUT);
  U8_INIT_OUTPUT(outstream,initial_size);
  outstream->u8_streaminfo=outstream->u8_streaminfo|U8_STREAM_MALLOCD;
  return outstream;
}

U8_EXPORT
/* u8_close_output:
     Arguments: a size
     Returns: an input stream

This mallocs and initializes an input stream to read from a particular string. 
*/
int u8_close_output(u8_output out)
{
  if (out->u8_closefn) return out->u8_closefn(out);
  else return 0;
}

U8_EXPORT void u8_flush(struct U8_OUTPUT *f)
{
  if (f->u8_flushfn) f->u8_flushfn(f);
}

U8_EXPORT int _u8_close_sinput(struct U8_INPUT *f)
{
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_inbuf);
  if (f->u8_streaminfo&U8_STREAM_MALLOCD) u8_free(f);
  return 1;
}

U8_EXPORT
/* u8_close:
    Arguments: an input or output stream
   Closes an open input or output stream. */
int u8_close(U8_STREAM *stream)
{
  if ((stream->u8_streaminfo)&(U8_OUTPUT_STREAM))
    return u8_close_output((U8_OUTPUT *)stream);
  else return u8_close_input((U8_INPUT *)stream);
}

/* Initialization function (just records source file info) */

U8_EXPORT void u8_init_streamio_c()
{
  u8_register_source_file(_FILEINFO);
}
