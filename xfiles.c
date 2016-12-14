/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2016 beingmeta, inc.
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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8io.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"
#include "libu8/xfiles.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Define these flags if you don't have them */
#if WIN32
#define XFILE_OPEN_PERMS (S_IRUSR|S_IWUSR)
#else
#define XFILE_OPEN_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)
#endif

u8_condition
  u8_nopos=_("XFILE does not allow positioning"),
  u8_nowrite=_("XFILE only allows writing"),
  u8_noread=_("XFILE only allows reading");

static u8_condition InvalidBufsize=_("Invalid buffer size for xfile");
static u8_condition SpecialXBUF=_("Stream XBUF is static");

/* Utility functions */

U8_EXPORT int u8_writeall(int sock,const unsigned char *data,int len)
{
  int bytes_to_write=len;
  while (bytes_to_write>0) {
    int delta=write(sock,data,bytes_to_write);
    if (delta<0)
      if (errno==EAGAIN) continue;
      else return delta;
    else if (delta==0)
      if (errno==EAGAIN) continue;
      else if (bytes_to_write>0) return -1;
      else return 0;
    else {
      data=data+delta;
      bytes_to_write=bytes_to_write-delta;}}
  return bytes_to_write;
}

/* Input */

/* This returns the number of bytes added */
static int fill_xinput(struct U8_XINPUT *xf)
{
  /* There are three basic steps:
       * Overwriting what we've already read with the data we have.
       * Reading data from the file/socket
       * Writing that data as UTF-8 into the buffer
       * Updating all the various pointers in the structure.
  */
  struct U8_OUTPUT tmpout; int convert_val;
  int unread_bytes=xf->u8_inlim-xf->u8_inptr;
  int bytes_read, bytes_converted;
  const unsigned char *reader, *limit;
  u8_byte *start=(u8_byte *)xf->u8_inbuf, *cur=(u8_byte *)xf->u8_inptr;
  u8_byte *end=(u8_byte *)xf->u8_inlim;
  if (cur>start) {
    /* First, if we've read anything at all, overwrite it, compressing the
       input buffer to make more space. */
    memmove(start,cur,unread_bytes);
    /* Now we're reading form the front of the buffer. */
    xf->u8_inptr=xf->u8_inbuf;
    /* And the limit of valid data is just unread_bytes further on */
    end= xf->u8_inlim= xf->u8_inbuf+unread_bytes;
    /* Null terminate it, if there's space (there has to be) */
    start[unread_bytes]='\0';
    cur=start+unread_bytes;}
  /* Now, fill the read buffer from the input socket */
  bytes_read=read(xf->fd,xf->xbuf+xf->xbuflen,xf->xbuflim-xf->xbuflen);
  /* If you had trouble or didn't get any data, return zero or the error code. */
  if (bytes_read<=0) return bytes_read;
  /* Update the buflen to reflect what we read from the socket */
  xf->xbuflen=xf->xbuflen+bytes_read;
  /* Do the conversion.  First we set up _reader_ and _limit_ to scan
     the data we just read from the socket */
  reader=xf->xbuf; limit=reader+xf->xbuflen;
  /* Now we initialize a temporary output stream from our input
     buffer, arranging the output to write to the end of the valid
     input. Conversion will write to this output stream, filling up
     the input buffer. */
  if ((xf->u8_streaminfo)&(U8_STREAM_MALLOCD)) {
    U8_INIT_OUTPUT_X(&tmpout,xf->u8_bufsz-unread_bytes,xf->u8_inbuf,
		     U8_STREAM_MALLOCD);}
  else {U8_INIT_OUTPUT_X(&tmpout,xf->u8_bufsz-unread_bytes,xf->u8_inbuf,
			 U8_FIXED_STREAM);}
  /* Position the temporary output stream at the end of the valid input. */
  tmpout.u8_outptr=end;
  /* Now we do the actual conversion, where u8_convert writes into the
     string stream passed it as its first argument. If this has any trouble,
     it will stop and reader will hold the most recent conversion state. */
  convert_val=
    u8_convert(xf->encoding,(xf->u8_streaminfo&U8_STREAM_CRLFS),
               &tmpout,&reader,limit);
  /* Now we start updating the input stream to reflect all of the new
     data (and possibly a new buffer) */
  if (((start)!=(tmpout.u8_outbuf)) &&
      ((xf->u8_streaminfo)&(U8_STREAM_OWNS_BUF))) { 
    /* If tmpout grew its buffer during the write, we need to free the
       old buffer here. */
    u8_free(xf->u8_inbuf);
    xf->u8_inbuf=tmpout.u8_outbuf;}
  else xf->u8_inbuf=tmpout.u8_outbuf;
  /* Now we copy stuff back into the input stream. */
  xf->u8_inptr=tmpout.u8_outbuf;
  /* The limit of the valid input is the end of the tmpout stream. */
  xf->u8_inlim=tmpout.u8_outptr;
  xf->u8_bufsz=tmpout.u8_bufsz;
  /* Just in case tmpout grew while being filled but was originally static */
  if ((tmpout.u8_streaminfo)&(U8_STREAM_MALLOCD))
    xf->u8_streaminfo=(xf->u8_streaminfo)|(U8_STREAM_MALLOCD);
  /* Now, overwrite what you've converted with what you haven't converted
     yet and updated the buflen. */
  bytes_converted=reader-xf->xbuf;
  memmove(xf->xbuf,xf->xbuf+bytes_converted,xf->xbuflen-bytes_converted);
  xf->xbuflen=xf->xbuflen-bytes_converted;
  if (convert_val<0)
    /* If you erred, return the errval */
    return convert_val;
  /* Return the number of new unread UTF-8 bytes */
  else return (xf->u8_inlim-xf->u8_inptr)-unread_bytes;
}
U8_EXPORT int u8_init_xinput(struct U8_XINPUT *xi,int fd,u8_encoding enc)
{
  if (fd<0) return -1;
  else {
    U8_INIT_INPUT(((u8_input)xi),U8_DEFAULT_XFILE_BUFSIZE);
    xi->fd=fd; xi->xbuf=u8_malloc(U8_DEFAULT_XFILE_BUFSIZE);
    xi->xbuflen=0; xi->xbuflim=U8_DEFAULT_XFILE_BUFSIZE;
    xi->u8_fillfn=(u8_fillfn)fill_xinput;
    xi->u8_closefn=(u8_input_closefn)u8_close_xinput;
    xi->u8_streaminfo=xi->u8_streaminfo|U8_STREAM_OWNS_XBUF;
    xi->encoding=enc;
    u8_register_open_xfile((U8_XOUTPUT *)xi);
    return fd;}
}

U8_EXPORT struct U8_XINPUT *u8_open_xinput(int fd,u8_encoding enc)
{
  if (fd<0) return NULL;
  else {
    struct U8_XINPUT *xi=u8_alloc(struct U8_XINPUT);
    if (u8_init_xinput(xi,fd,enc)>=0) {
      xi->u8_streaminfo=xi->u8_streaminfo|U8_STREAM_MALLOCD;
      return xi;}
    u8_free(xi);
    return NULL;}
}

U8_EXPORT int u8_xinput_setbuf(struct U8_XINPUT *xi,int bufsiz)
{
  if (bufsiz<=0) {
    u8_seterr(InvalidBufsize,"u8_xinput_setbuf",NULL);
    return -1;}
  else if ((xi->u8_streaminfo&U8_STREAM_OWNS_XBUF)==0) {
    u8_seterr(SpecialXBUF,"u8_xinput_setbuf",NULL);
    return -1;}
  else if (xi->xbuflen>=bufsiz) return 0;
  else {
    xi->xbuf=u8_realloc(xi->xbuf,bufsiz);
    xi->xbuflim=bufsiz;
    return 1;}
}

U8_EXPORT struct U8_XINPUT *u8_open_input_file
   (u8_string filename,u8_encoding enc,int flags,int perm)
{
  int fd;
  u8_string realname=u8_realpath(filename,NULL);
  char *fname=u8_tolibc(realname);
  if (flags<=0)
    flags=flags|O_RDONLY;
  if (!((flags&O_RDONLY) || (flags&O_RDWR)))
    flags=flags|O_RDONLY;
  if (perm<=0) perm=XFILE_OPEN_PERMS;
  fd=open(fname,flags,perm);
  u8_free(fname); u8_free(realname);
  if (fd>=0) {
    struct U8_XINPUT *xi=u8_open_xinput(fd,enc);
    xi->u8_streaminfo=xi->u8_streaminfo|U8_STREAM_OWNS_SOCKET|U8_STREAM_CAN_SEEK;
    return xi;}
  else {
    u8_seterr(u8_strerror(errno),"u8_open_input_file",
              u8_strdup(filename));
    return NULL;}
}

U8_EXPORT void u8_close_xinput(struct U8_XINPUT *f)
{
  u8_deregister_open_xfile((U8_XOUTPUT *)f);
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_inbuf);
  if (f->u8_streaminfo&U8_STREAM_OWNS_XBUF) u8_free(f->xbuf);
  if (f->u8_streaminfo&U8_STREAM_OWNS_SOCKET) close(f->fd);
  f->fd=-1;
  if (f->u8_streaminfo&U8_STREAM_MALLOCD) u8_free(f);
}

/* OUTPUT */

static int flush_xoutput(struct U8_XOUTPUT *xf)
{
  unsigned char *buf=xf->xbuf;
  int buflen=xf->xbuflim;
  while (xf->u8_outptr>xf->u8_outbuf) {
    /* Pick the region to change */
    const u8_byte *scan=xf->u8_outbuf, *scan_end=xf->u8_outptr;
    /* Convert the data to the appropriate encoding */
    u8_localize(xf->encoding,&scan,scan_end,
                xf->escape,(xf->u8_streaminfo&U8_STREAM_CRLFS),
                buf,&buflen);
    /* Write it out to the file descriptor */
    u8_writeall(xf->fd,buf,buflen);
    /* Write over what you just wrote out with what you haven't written
       yet and move the point to the end of the unwritten data. */
    memmove(xf->u8_outbuf,scan,xf->u8_outptr-scan);
    xf->u8_outptr=xf->u8_outbuf+(xf->u8_outptr-scan);
    /* Reset the buflen to the limit */
    buflen=xf->xbuflim;}
  return xf->u8_outlim-xf->u8_outptr;
}

U8_EXPORT int u8_init_xoutput
   (struct U8_XOUTPUT *xo,int fd,u8_encoding enc)
{
  if (fd<0) return -1;
  else {
    U8_INIT_OUTPUT((u8_output)xo,U8_DEFAULT_XFILE_BUFSIZE);
    xo->fd=fd; xo->xbuf=u8_malloc(U8_DEFAULT_XFILE_BUFSIZE);
    xo->xbuflen=0; xo->xbuflim=U8_DEFAULT_XFILE_BUFSIZE;
    xo->encoding=enc; xo->xbuf[0]='\0';
    xo->u8_flushfn=(u8_flushfn)flush_xoutput;
    xo->u8_closefn=(u8_output_closefn)u8_close_xoutput;
    xo->u8_streaminfo=xo->u8_streaminfo|U8_STREAM_OWNS_XBUF;
    u8_register_open_xfile(xo);
    return 1;}
}

U8_EXPORT struct U8_XOUTPUT *u8_open_xoutput(int fd,u8_encoding enc)
{
  if (fd<0) return NULL;
  else {
    struct U8_XOUTPUT *xo=u8_alloc(struct U8_XOUTPUT);
    if (u8_init_xoutput(xo,fd,enc)>=0) {
      xo->u8_streaminfo=xo->u8_streaminfo|U8_STREAM_MALLOCD;
      return xo;}
    u8_free(xo);
    return NULL;}
}

U8_EXPORT int u8_xoutput_setbuf(struct U8_XOUTPUT *xo,int bufsiz)
{
  if (bufsiz<=0) {
    u8_seterr(InvalidBufsize,"u8_xoutput_setbuf",NULL);
    return -1;}
  else if ((xo->u8_streaminfo&U8_STREAM_OWNS_XBUF)==0) {
    u8_seterr(SpecialXBUF,"u8_xoutput_setbuf",NULL);
    return -1;}
  else if (xo->xbuflen>=bufsiz) return 0;
  else {
    xo->xbuf=u8_realloc(xo->xbuf,bufsiz);
    xo->xbuflim=bufsiz;
    return 1;}
}

U8_EXPORT struct U8_XOUTPUT *u8_open_output_file
   (u8_string filename,u8_encoding enc,int flags,int perm)
{
  u8_string realname=u8_realpath(filename,NULL);
  char *fname=u8_tolibc(realname);
  int fd;
  if (flags<=0)
    flags=flags|O_WRONLY|O_TRUNC|O_CREAT;
  if (!((flags&O_WRONLY) || (flags&O_RDWR)))
    flags=flags|O_WRONLY;
  if (perm<=0) perm=XFILE_OPEN_PERMS;
  fd=open(fname,flags,perm);
  u8_free(fname); u8_free(realname);
  if (fd>=0) {
    struct U8_XOUTPUT *out=u8_open_xoutput(fd,enc);
    if (out) out->u8_streaminfo=out->u8_streaminfo|U8_STREAM_OWNS_SOCKET|U8_STREAM_CAN_SEEK;
    return out;}
  else {
    u8_seterr(u8_strerror(errno),"u8_open_output_file",
              u8_strdup(filename));
    return NULL;}
}

U8_EXPORT off_t u8_getpos(struct U8_STREAM *f)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      int delta=out->u8_outptr-out->u8_outbuf;
      off_t pos=lseek(out->fd,0,SEEK_CUR);
      if (pos<0) return pos; else return pos+delta;}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      int delta=in->u8_inlim-in->u8_inptr;
      off_t pos=lseek(in->fd,0,SEEK_CUR);
      if (pos<0) return pos; else return pos-delta;}
  else {
    u8_seterr(u8_nopos,"u8_getpos",NULL);
    return -1;}
}

U8_EXPORT off_t u8_setpos(struct U8_STREAM *f,off_t off)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      u8_flush((struct U8_OUTPUT *)f);
      return lseek(out->fd,off,SEEK_SET);}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      u8_byte *buf=(u8_byte *)in->u8_inbuf;
      in->u8_inptr=in->u8_inlim=in->u8_inbuf; *buf='\0';
      return lseek(in->fd,off,SEEK_SET);}
  else {
    u8_seterr(u8_nopos,"u8_setpos",NULL);
    return -1;}
}

U8_EXPORT off_t u8_endpos(struct U8_STREAM *f)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      off_t cur=lseek(out->fd,0,SEEK_CUR);
      off_t end=lseek(out->fd,0,SEEK_END);
      if (lseek(out->fd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1;}
      else return end;}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      off_t cur=lseek(in->fd,0,SEEK_CUR);
      off_t end=lseek(in->fd,0,SEEK_END);
      if (lseek(in->fd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1;}
      else return end;}
  else {
    u8_seterr(u8_nopos,"u8_setpos",NULL);
    return -1;}
}

U8_EXPORT double u8_getprogress(struct U8_STREAM *f)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      int delta=out->u8_outptr-out->u8_outbuf;
      off_t cur=lseek(out->fd,0,SEEK_CUR);
      off_t end=lseek(out->fd,0,SEEK_END);
      if (lseek(out->fd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1.0;}
      else return 100.0*(((double)(cur+delta))/((double)end));}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      int delta=in->u8_inlim-in->u8_inptr;
      off_t cur=lseek(in->fd,0,SEEK_CUR);
      off_t end=lseek(in->fd,0,SEEK_END);
      if (lseek(in->fd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1.0;}
      else return 100.0*(((double)(cur-delta))/((double)end));}
  else {
    u8_seterr(u8_nopos,"u8_setpos",NULL);
    return -1.0;}
}

U8_EXPORT void u8_close_xoutput(struct U8_XOUTPUT *f)
{
  if (f->u8_flushfn) f->u8_flushfn((U8_OUTPUT *)f);
  u8_deregister_open_xfile(f);
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_outbuf);
  if (f->u8_streaminfo&U8_STREAM_OWNS_XBUF) u8_free(f->xbuf);
  if (f->u8_streaminfo&U8_STREAM_OWNS_SOCKET) close(f->fd);
  if (f->u8_streaminfo&U8_STREAM_MALLOCD) u8_free(f);
}

/* Tracking open XFILEs */

#if U8_THREADS_ENABLED
static u8_mutex xfile_registry_lock;
#endif

static struct U8_OPEN_XFILES *open_xfiles=NULL;

U8_EXPORT void u8_register_open_xfile(struct U8_XOUTPUT *out)
{
  struct U8_OPEN_XFILES *new=u8_alloc(struct U8_OPEN_XFILES);
  u8_lock_mutex((&xfile_registry_lock));
  new->xfile=out;
  new->next=open_xfiles;
  open_xfiles=new;
  u8_unlock_mutex((&xfile_registry_lock));
}

U8_EXPORT void u8_deregister_open_xfile(struct U8_XOUTPUT *out)
{
  struct U8_OPEN_XFILES *scan=open_xfiles, **head=&open_xfiles;
  u8_lock_mutex((&xfile_registry_lock));
  while (scan)
    if (scan->xfile == out) {
      *head=scan->next; u8_free(scan);
      u8_unlock_mutex((&xfile_registry_lock));
      return;}
    else {head=&(scan->next); scan=scan->next;}
  u8_unlock_mutex((&xfile_registry_lock));
}

U8_EXPORT void u8_close_xfiles()
{
  struct U8_OPEN_XFILES *scan=open_xfiles, *next;
  u8_lock_mutex((&xfile_registry_lock));
  while (scan) {
    if ((scan->xfile->u8_streaminfo)&U8_OUTPUT_STREAM)
      scan->xfile->u8_flushfn((U8_OUTPUT *)(scan->xfile));
    if ((scan->xfile->u8_streaminfo)&U8_STREAM_OWNS_SOCKET)
      close(scan->xfile->fd);
    if ((scan->xfile->u8_streaminfo)&U8_STREAM_MALLOCD)
      u8_free(scan->xfile);
    next=scan->next;
    u8_free(scan);
    open_xfiles=scan=next;}
  u8_unlock_mutex((&xfile_registry_lock));
}

/* Initialization procedure */

U8_EXPORT void u8_init_xfiles_c()
{
#if U8_THREADS_ENABLED
  u8_init_mutex(&xfile_registry_lock);
#endif
  atexit(u8_close_xfiles);
  u8_register_source_file(_FILEINFO);
}
