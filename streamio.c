/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
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
#include "libu8/u8stringfns.h"
#include "libu8/u8ctype.h"

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
/* Just for sprintf */
#include <stdio.h>

/* Utility functions */

U8_EXPORT void _U8_SETUP_OUTPUT(u8_output s,size_t sz,
                                unsigned char *buf,
                                unsigned char *write,
                                int flags)
{
  U8_SETUP_OUTPUT(s,sz,buf,write,flags);
}


U8_EXPORT void _U8_INIT_OUTPUT_X(u8_output s,size_t sz,
                                 unsigned char *buf,
                                 unsigned char *write,
                                 int flags)
{
  U8_INIT_OUTPUT_X(s,sz,buf,flags);
}

U8_EXPORT void _U8_INIT_STRING_INPUT(u8_input s,size_t n,u8_byte *buf)
{
  U8_INIT_STRING_INPUT(s,n,buf);
}

U8_EXPORT void _U8_INIT_INPUT_X(u8_input s,size_t n,u8_byte *buf,int bits)
{
  U8_INIT_INPUT_X(s,n,buf,bits);
}

U8_EXPORT
/* u8_grow_stream:
     Arguments: a pointer to a string stream and a number of u8_inbuf
     Returns: int

  Grows the data structures for the string stream to include delta
more u8_inbuf
*/
ssize_t u8_grow_stream(struct U8_STREAM *stream,ssize_t delta_arg)
{
  ssize_t delta=(delta_arg<U8_BUF_MIN_GROW)?(U8_BUF_MIN_GROW):(delta_arg);
  ssize_t cur_size=stream->u8_bufsz;
  ssize_t new_size=(stream->u8_streaminfo&U8_OUTPUT_STREAM)?
    u8_grow_output_stream((u8_output)stream,(delta<0)?(-1):(cur_size+delta)):
    u8_grow_input_stream((u8_input)stream,(delta<0)?(-1):(cur_size+delta));
  return new_size;
}

U8_EXPORT
/* u8_grow_input_stream:
     Arguments: a pointer to an input stream and max buffer size
     Returns: the size of the new buffer

     Doubles the size of the input buffer
*/
ssize_t u8_grow_input_stream(struct U8_INPUT *in,ssize_t to_size)
{
  if ((in->u8_streaminfo)&(U8_FIXED_STREAM)) return 0;

  int owns_buf = ((in->u8_streaminfo)&(U8_STREAM_OWNS_BUF));
  size_t max = in->u8_bufsz,
    bytes_buffered = in->u8_inlim - in->u8_read,
    bytes_read = in->u8_read - in->u8_inbuf;
  size_t new_max = ((max>=U8_BUF_THROTTLE_POINT)?
                    (max+(U8_BUF_THROTTLE_POINT/2)):
                    (max*2));

  if (to_size<0) to_size=new_max;
  else if ((to_size+U8_BUF_MIN_GROW)<max)
    return max;
  else NO_ELSE;
  while (new_max<to_size)
    new_max = ((new_max>=U8_BUF_THROTTLE_POINT)?
               (new_max+(U8_BUF_THROTTLE_POINT/2)):
               (new_max*2));
  u8_byte *buf = in->u8_inbuf, *new_buf=NULL;
  /* Reset buffer */
  if (bytes_read>0) {
    memcpy(buf, in->u8_read, bytes_buffered);
    in->u8_read = buf;
    in->u8_inlim -= bytes_read;
    *(in->u8_inlim) = '\0';}
  /* Try to allocate a new buffer */
  new_buf=(owns_buf)?(u8_realloc(buf,new_max)):(u8_malloc(new_max));
  if (new_buf==NULL) {
    size_t shrink_by = (new_max-max)/16;
    while (new_buf==NULL) {
      /* The alloc/realloc call failed, so try smaller sizes */
      errno=0; /* Reset errno */
      /* Shrink until it works or the buffer size is smaller than the
         current value */
      new_max=new_max-shrink_by;
      if (new_max<=max) {
        u8_log(LOGCRIT,"MallocFailed/grow_output",
               "Couldn't grow buffer for input stream");
        return max;}
      u8_log(LOGCRIT,"MemoryRestricted",
             "Couldn't grow u8_input_stream buffer (x%llx) "
             "to %lld bytes, trying %lld",
             ((long long)(U8_PTR2INT(in))),
             new_max+shrink_by,new_max);
      new_buf=(owns_buf)?(u8_realloc(in->u8_inbuf,new_max)):
        (u8_malloc(new_max));}}
  if (!(owns_buf)) {
    memcpy(new_buf,buf,bytes_buffered);
    in->u8_streaminfo|=U8_STREAM_OWNS_BUF;}
  in->u8_inbuf=new_buf;
  in->u8_read=new_buf;
  in->u8_inlim=new_buf+bytes_buffered;
  in->u8_bufsz=new_max;
  return new_max;
}

U8_EXPORT
/* u8_grow_output_stream:
     Arguments: a pointer to an input stream and max buffer size
     Returns: the size of the new buffer

     Doubles the size of the input buffer
*/
ssize_t u8_grow_output_stream(struct U8_OUTPUT *out,ssize_t to_size)
{
  int owns_buf = ((out->u8_streaminfo)&(U8_STREAM_OWNS_BUF));
  size_t max = out->u8_bufsz;
  size_t new_max = ((max>=U8_BUF_THROTTLE_POINT)?
                    (max+(U8_BUF_THROTTLE_POINT/2)):
                    (max*2)),
    bytes_buffered = out->u8_write - out->u8_outbuf;
  if (to_size<0) to_size=new_max;
  else if ((to_size+U8_BUF_MIN_GROW)<max)
    return max;
  else NO_ELSE;
  while (new_max<to_size)
    new_max = ((new_max>=U8_BUF_THROTTLE_POINT)?
               (new_max+(U8_BUF_THROTTLE_POINT/2)):
               (new_max*2));
  u8_byte *buf = out->u8_outbuf, *new_buf=NULL;
  /* Try to allocate a new buffer */
  new_buf=(owns_buf)?
    (u8_zrealloc(buf,new_max,max)):
    (u8_zmalloc(new_max));
  if (new_buf==NULL) {
    size_t shrink_by = (new_max-max)/16;
    while (new_buf==NULL) {
      /* The alloc/realloc call failed, so try smaller sizes */
      /* Shrink until it works or the buffer size is smaller than the
         current value */
      new_max=new_max-shrink_by;
      if (new_max<=max) {
        u8_log(LOGCRIT,"MallocFailed/grow_output",
               "Couldn't grow buffer for output stream");
        return max;}
      errno=0; /* Reset errno */
      u8_log(LOGCRIT,"MemoryRestricted",
             "Couldn't grow u8_output_stream buffer (x%llx) "
             "to %lld bytes, trying %lld",
             ((long long)U8_PTR2INT(out)),
             new_max+shrink_by,new_max);
      new_buf=(owns_buf)?(u8_realloc(out->u8_outbuf,new_max)):
        (u8_malloc(new_max));}}
  if (!(owns_buf)) {
    memcpy(new_buf,buf,bytes_buffered);
    out->u8_streaminfo|=U8_STREAM_OWNS_BUF;}
  out->u8_outbuf=new_buf;
  out->u8_write=new_buf+bytes_buffered;
  out->u8_outlim=new_buf+new_max;
  out->u8_bufsz=new_max;
  return new_max;
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
  if (f->u8_write+size+1>=f->u8_outlim) {
    /* Need space */
    int rv = (f->u8_flushfn) ? (f->u8_flushfn(f)) : (0);
    if (rv<0) {
      u8_graberrno("u8_putc/flush",NULL);
      return -1;}
    else if (f->u8_write+size+1>=f->u8_outlim) {
      /* Still need space */
      if ((f->u8_streaminfo)&(U8_FIXED_STREAM))
        return 0;
      else {
        size_t cur_size = f->u8_bufsz;
        size_t new_size = cur_size+size+U8_BUF_MIN_GROW;
        u8_grow_output_stream(f,new_size);}}}
  if (f->u8_write+size+1>=f->u8_outlim) {
    if (errno) u8_graberrno("u8_putc",NULL);
    u8_seterr("NoSpaceInStream","u8_putc",NULL);
    return -1;}
  else {
    shift=(size-1)*6;
    write=f->u8_write;
    *write++=off[size-1]|(masks[size-1]&(ch>>shift));
    shift=shift-6;
    size--;
    while (size) {
      *write++=0x80|((ch>>shift)&0x3F);
      shift=shift-6; size--;}
    *write='\0'; f->u8_write=write;
    return size;}
}
U8_EXPORT int _u8_putn(struct U8_OUTPUT *f,u8_string data,int len)
{
  if (U8_EXPECT_FALSE(len==0))
    return 0;
  else if (f->u8_write+len+1>=f->u8_outlim) {
    /* Need space */
    int rv = (f->u8_flushfn) ? (f->u8_flushfn(f)) : (0);
    if (rv<0) {
      u8_graberrno("u8_putn/flush",NULL);
      return -1;}
    else if (f->u8_write+len+1>=f->u8_outlim) {
      /* Still need space */
      if ((f->u8_streaminfo)&(U8_FIXED_STREAM))
        return 0;
      else {
        size_t cur_size = f->u8_bufsz;
        size_t new_size = cur_size+len+U8_BUF_MIN_GROW;
        u8_grow_output_stream(f,new_size);}}}
  else NO_ELSE;
  if (f->u8_write+len+1>=f->u8_outlim) {
    if (errno) u8_graberrno("u8_putn",NULL);
    u8_seterr("NoSpaceInStream","u8_putn",NULL);
    return -1;}
  else {
    memcpy(f->u8_write,data,len);
    f->u8_write=f->u8_write+len;
    *(f->u8_write)='\0';
    return len;}
}
U8_EXPORT int _u8_output_needs(u8_output out,size_t n_bytes)
{
  if (u8_outbuf_space(out)>=n_bytes)
    return 1;
  else if ( ( (out->u8_streaminfo) & (U8_FIXED_STREAM) ) &&
            (out->u8_flushfn == NULL) )
    return 0;
  else {
    /* Need space */
    int rv = (out->u8_flushfn) ? (out->u8_flushfn(out)) : (0);
    if (rv<0) u8_graberrno("u8_putn/flush",NULL);
    if ( (u8_outbuf_space(out)) < n_bytes+1 )
      return 1;
    else if ( (out->u8_streaminfo) & (U8_FIXED_STREAM) )
      return 0;
    else {
      /* Still needing space */
      size_t cur_size = out->u8_bufsz;
      size_t new_size = cur_size+n_bytes+U8_BUF_MIN_GROW;
      u8_grow_output_stream(out,new_size);
      return (u8_outbuf_space(out)>=n_bytes);}}
}
U8_EXPORT int _u8_getc(struct U8_INPUT *f)
{
  int i, ch, byte, size;
  const u8_byte *scan;
  if (f->u8_read>=f->u8_inlim) {
    /* Try to get more data */
    if (f->u8_fillfn) f->u8_fillfn(f);
    /* If you can't, just return */
    if (f->u8_read>=f->u8_inlim) return -1;}
  byte=*(f->u8_read);
  if (byte < 0x80) {
    f->u8_read++; return byte;}
  else if (U8_EXPECT_FALSE(((byte < 0xc0)||(byte >= 0xFE)))) {
    /* Bad start char */
    if ((u8_utf8err)||
        ((f->u8_streaminfo&U8_STREAM_UTF8ERR)==U8_STREAM_UTF8ERR)) {
      char *details=u8_grab_bytes(f->u8_read,UTF8_BUGWINDOW,NULL);
      u8_log(LOG_WARN,u8_BadUTF8Start,"bytes: '%s'",details);
      u8_seterr(u8_BadUTF8Start,"u8_getc",details);
      (f->u8_read)++;
      return -2;}
    else if ((u8_utf8warn)||
             ((f->u8_streaminfo&U8_STREAM_UTF8WARN)==U8_STREAM_UTF8WARN))
      u8_utf8_warning(u8_BadUTF8Start,f->u8_read,f->u8_inlim);
    else NO_ELSE;
    (f->u8_read)++;
    return 0xFFFD;}
  /* Otherwise, figure out the size and initial byte fragment */
  else if (byte < 0xE0) {size=2; ch=byte&0x1F;}
  else if (byte < 0xF0) {size=3; ch=byte&0x0F;}
  else if (byte < 0xF8) {size=4; ch=byte&0x07;}
  else if (byte < 0xFC) {size=5; ch=byte&0x3;}
  else {
    assert(byte < 0xFE);
    size=6; ch=byte&0x1;}
  /* Now, we now how many u8_inbuf we need, so we check if we have
     that much data. */
  if (f->u8_read+size>f->u8_inlim) /* Not enough data */
    if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_read;
      while (n_u8_inbuf<size) {
        if (f->u8_fillfn(f)==0) return -1;
        else n_u8_inbuf=f->u8_inlim-f->u8_read;}}
    else return -1;
  else NO_ELSE;
  /* We have enough data, so now we just do a UTF-8 read. */
  i=size-1; f->u8_read++; scan=f->u8_read;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      if ((u8_utf8err)||
          ((f->u8_streaminfo&U8_STREAM_UTF8ERR)==U8_STREAM_UTF8ERR)) {
        char *details=u8_grab_bytes(scan,UTF8_BUGWINDOW,NULL);
        u8_log(LOG_WARN,u8_BadUTF8,
               _("Truncated UTF-8 sequence: '%s'"),details);
        u8_seterr(u8_TruncatedUTF8,"u8_getc",details);
        f->u8_read=(u8_byte *)scan; /* Consume the bad sequence */
        return -2;}
      else if ((u8_utf8warn)||(f->u8_streaminfo&U8_STREAM_UTF8WARN))
        u8_utf8_warning(u8_TruncatedUTF8,f->u8_read,f->u8_inlim);
      else NO_ELSE;
      f->u8_read=(u8_byte *)scan; /* Consume the truncated byte sequence */
      return 0xFFFD;}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  /* And now we update the data structure */
  f->u8_read=(u8_byte *)scan;
  return ch;
}

static int peekc(struct U8_INPUT *f,int fill)
{
  int i, ch, byte, size;
  const u8_byte *start=f->u8_read, *scan=start;
  if (f->u8_read>=f->u8_inlim) {
    if (fill) {
      /* Try to get more data */
      if (f->u8_fillfn) f->u8_fillfn(f);
      /* If you can't, just return */
      if (f->u8_read>=f->u8_inlim) return -1;}
    else return -1;}
  byte=*(f->u8_read);
  if (byte < 0x80) return byte;
  else if (byte < 0xc0) {   /* Catch this error */
    if ((u8_utf8err)||
        ((f->u8_streaminfo&U8_STREAM_UTF8ERR)==U8_STREAM_UTF8ERR)) {
      char *details=u8_grab_bytes(f->u8_read,U8_UTF8BUG_WINDOW,NULL);
      u8_seterr(u8_BadUTF8,"u8_getc",details);
      return -2;}
    else if ((u8_utf8warn)||(f->u8_streaminfo&U8_STREAM_UTF8WARN))
      u8_utf8_warning(u8_TruncatedUTF8,start,f->u8_inlim);
    else NO_ELSE;
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
  if (f->u8_read+size>f->u8_inlim) /* Not enough data */
    if (!(fill)) return -1;
    else if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_read;
      while (n_u8_inbuf<size) {
        if (f->u8_fillfn(f)==0) return -1;
        else n_u8_inbuf=f->u8_inlim-f->u8_read;}}
    else return -1;
  else NO_ELSE;
  /* We have enough data, so now we just do a UTF-8 read. */
  i=size=1; scan=f->u8_read;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      f->u8_read=(u8_byte *)scan;
      if ((u8_utf8err)||
	  ((f->u8_streaminfo&U8_STREAM_UTF8ERR)==U8_STREAM_UTF8ERR)) {
	return u8_reterr(u8_BadUTF8,"u8_getc",u8_strdup(start));}
      else return 0xFFFD;}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  return ch;
}
U8_EXPORT int u8_probec(struct U8_INPUT *f) { return peekc(f,1); }
U8_EXPORT int u8_peekc(struct U8_INPUT *f) { return peekc(f,0); }

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
  if (f->u8_read+n>f->u8_inlim) /* Not enough data */
    if (f->u8_fillfn) {
      /* Try to fill the buffer */
      int n_u8_inbuf=f->u8_inlim-f->u8_read;
      while (n_u8_inbuf<n) {
        if (f->u8_fillfn(f)==0) return -1;
        else n_u8_inbuf=f->u8_inlim-f->u8_read;}}
    else return -1;
  else NO_ELSE;
  /* Check if it worked */
  if (f->u8_read+n>f->u8_inlim) return -1;
  /* We have enough data, so now we need to get the size of
     the largest valid UTF-8 string less than *n* u8_inbuf */
  n=u8_validate(f->u8_read,n);
  /* Now we just copy the u8_inbuf over */
  strncpy(ptr,f->u8_read,n); ptr[n]='\0'; f->u8_read=f->u8_read+n;
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
                    ssize_t *sizep)
{
  const u8_byte *found=NULL, *start=f->u8_read;
  while (((found=strstr(start,eos))==NULL)||(found>f->u8_inlim)) {
    int start_pos=f->u8_inlim-f->u8_read, retval=0;
    /* Quit if we have length constraints which
       we are already past. */
    if (f->u8_fillfn) retval=f->u8_fillfn(f);
    if (retval==0) break;
    else if (retval<0) {
      if (sizep) *sizep=retval;
      return NULL;}
    start=f->u8_read+start_pos;}
  if (found) {
    ssize_t size=(found-f->u8_read);
    if (sizep) *sizep=size;
    /* No data, return NULL */
    if ((buf) && (size>=len)) return NULL;
    else if (buf==NULL) buf=u8_malloc(size+1);
    if (size)
      u8_getn(buf,size,f);
    else buf[0]='\0';
    /* Advance past the separator */
    f->u8_read=f->u8_read+strlen(eos);
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
    if ((f->u8_read>f->u8_inbuf) && (f->u8_read[-1]==ch)) {
      f->u8_read--;
      return ch;}
    else {
      char buf[32];
      sprintf(buf,"\\U%08x",ch);
      u8_seterr(u8_BadUNGETC,"u8_ungetc",u8_strdup(buf));
      return -1;}
  else {
    struct U8_OUTPUT tmpout; u8_byte buf[16]; int size;
    U8_INIT_FIXED_OUTPUT(&tmpout,16,buf);
    u8_putc(&tmpout,ch); size=tmpout.u8_write-tmpout.u8_outbuf;
    if ((f->u8_read >= f->u8_inbuf+size) &&
        (strncmp(f->u8_read-size,tmpout.u8_outbuf,size)==0)) {
      f->u8_read=f->u8_read-size;
      return ch;}
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
  const u8_byte *semi=strchr(f->u8_read,';'), *comma=strchr(f->u8_read,',');
  while (((semi==NULL)||(semi>=f->u8_inlim))&&
         ((comma==NULL)||(comma>=f->u8_inlim))&&
         (f->u8_fillfn)) {
    f->u8_fillfn(f);
    semi=strchr(f->u8_read,';');
    comma=strchr(f->u8_read,',');}
  if (f->u8_read==f->u8_inlim)
    return -1;
  else {
    const u8_byte *start=f->u8_read; u8_string end=NULL;
    int code=u8_parse_entity(start,&end);
    if ((code<0) && (end) && (f->u8_fillfn)) {
      /* If code<0 and end was set, that meant it got started but
         didn't finish, so we call the u8_fillfn and try again. */
      f->u8_fillfn(f);
      code=u8_parse_entity(start,&end);}
    else NO_ELSE;
    if (code>=0) {
      f->u8_read=(u8_byte *)end;
      return code;}
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
  f->u8_inlim=f->u8_read=f->u8_inbuf;
}

U8_EXPORT
/* u8_close_input:
     Arguments: a size
     Returns: an input stream

This mallocs and initializes an input stream to read from a particular string.
*/
int u8_close_input(u8_input in)
{
  if (in->u8_closefn)
    return in->u8_closefn(in);
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

U8_EXPORT
/* u8_reset_output:
     Arguments: an output stream
     Returns: the number of bytes available in the reset stream
 Discards the buffered output on the stream
*/
ssize_t u8_reset_output(u8_output out)
{
  out->u8_write = out->u8_outbuf;
  out->u8_outbuf[0]='\0';
  return out->u8_bufsz;
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

/* Input ports */

U8_EXPORT
u8_string u8_get_input_context(struct U8_INPUT *in,
                               size_t n_before,size_t n_after,
                               u8_string sep)
{
  u8_string read=in->u8_read, buf=in->u8_inbuf;

  u8_string before=
    u8_getvalid( ( (u8_inbuf_read(in)<n_before) ? (buf) : (read-n_before) ),
                 ( in->u8_inlim) );
  u8_string after=
    u8_getvalid( ( (u8_inbuf_ready(in)<n_after) ?
                   (in->u8_inlim) : (read+n_after) ),
                 ( in->u8_inlim));
  if (before>=after) {
    u8_log(LOGWARN,"InternalError",
           "The impossible happened in u8_get_input_context, sorry!");
    if (sep) return u8_strdup(sep); else return u8_strdup("<>");}
  else {
    size_t sep_len=(sep) ? (strlen(sep)) : (0);
    u8_byte *context=u8_zmalloc((after-before)+sep_len+1);
    
    strncpy(context,before,read-before); context[read-before]='\0';
    strcat(context,sep);
    strncat(context,read,after-read);
    
    return (u8_string) context;}
}

U8_EXPORT
u8_string u8_get_output_context(struct U8_OUTPUT *out,size_t n_before)
{
  u8_string buf=out->u8_outbuf, write=out->u8_write;

  u8_string before=
    u8_getvalid( ( (u8_outbuf_written(out)<n_before) ?
                   (buf) : (write-n_before) ),
                 ( out->u8_outlim) );

  return u8_strdup(before);
}

/* Default output ports */

u8_output u8_global_output=NULL;

U8_EXPORT void u8_set_global_output(u8_output out)
{
  u8_global_output=out;
}

#if U8_USE_TLS
u8_tld_key u8_default_output_key;
U8_EXPORT U8_OUTPUT *u8_get_default_output()
{
  U8_OUTPUT *f=(U8_OUTPUT *)u8_tld_get(u8_default_output_key);
  if (f) return f;
  else return u8_global_output;
}
U8_EXPORT void u8_set_default_output(U8_OUTPUT *f)
{
  u8_tld_set(u8_default_output_key,f);
}
U8_EXPORT void u8_reset_default_output(U8_OUTPUT *f)
{
  if ((f)&&(f==u8_global_output))
    u8_tld_set(u8_default_output_key,NULL);
  else u8_tld_set(u8_default_output_key,f);
}
#else /* Assume single threaded */
#if U8_USE__THREAD
__thread U8_OUTPUT *u8_default_output;
#else
U8_OUTPUT *u8_default_output=NULL;
#endif
U8_EXPORT U8_OUTPUT *u8_get_default_output()
{
  if (u8_default_output) return u8_default_output;
  else return u8_global_output;
}
U8_EXPORT void u8_set_default_output(U8_OUTPUT *f)
{
  u8_default_output=f;
}
U8_EXPORT void u8_reset_default_output(U8_OUTPUT *f)
{
  if ((f)&&(f==u8_global_output))
    u8_default_output=NULL;
  else u8_default_output=f;
}
#endif

/* Default input ports */

u8_input u8_global_input=NULL;

U8_EXPORT void u8_set_global_input(u8_input out)
{
  u8_global_input=out;
}

#if U8_USE_TLS
u8_tld_key u8_default_input_key;
U8_EXPORT U8_INPUT *u8_get_default_input()
{
  U8_INPUT *f=(U8_INPUT *)u8_tld_get(u8_default_input_key);
  if (f) return f;
  else return u8_global_input;
}
U8_EXPORT void u8_set_default_input(U8_INPUT *f)
{
  u8_tld_set(u8_default_input_key,f);
}
U8_EXPORT void u8_reset_default_input(U8_INPUT *f)
{
  if ((f)&&(f==u8_global_input))
    u8_tld_set(u8_default_input_key,NULL);
  else u8_tld_set(u8_default_input_key,f);
}
#else /* Assume single threaded */
#if U8_USE__THREAD
__thread U8_INPUT *u8_default_input;
#else
U8_INPUT *u8_default_input=NULL;
#endif
U8_EXPORT U8_INPUT *u8_get_default_input()
{
  if (u8_default_input) return u8_default_input;
  else return u8_global_input;
}
U8_EXPORT void u8_set_default_input(U8_INPUT *f)
{
  u8_default_input=f;
}
U8_EXPORT void u8_reset_default_input(U8_INPUT *f)
{
  if ((f)&&(f==u8_global_input))
    u8_default_input=NULL;
  else u8_default_input=f;
}
#endif

/* Initialization function (just records source file info) */

static int streamio_init_done=0;

void init_streamio_c()
{
  if (streamio_init_done) return;
  else streamio_init_done=1;
#if (U8_USE_TLS)
  u8_new_threadkey(&u8_default_output_key,NULL);
  u8_new_threadkey(&u8_default_input_key,NULL);
#endif

  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
