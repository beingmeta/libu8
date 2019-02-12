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
#include "libu8/u8xfiles.h"

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

static ssize_t writeall(int sock,const unsigned char *data,size_t len)
{
  size_t bytes_to_write = len;
  while (bytes_to_write > 0) {
    ssize_t delta=write(sock,data,bytes_to_write);
    if (delta<0)
      if (errno==EAGAIN) continue;
      else return delta;
    else if (delta==0)
      if (errno==EAGAIN) continue;
      else if (bytes_to_write>0)
        return -1;
      else return len;
    else {
      data=data+delta;
      bytes_to_write -= delta;}}
  return len;
}

/* Input */

/* This returns the number of bytes added */
U8_EXPORT int u8_fill_xinput(struct U8_XINPUT *xf)
{
  /* There are three basic steps:
   * Overwriting what we've already read with the data we have.
   * Reading data from the file/socket
   * Writing that data as UTF-8 into the buffer
   * Updating all the various pointers in the structure.
   */
  struct U8_OUTPUT tmpout; int convert_val;
  int unread_bytes=xf->u8_inlim-xf->u8_read;
  int bytes_read=0, bytes_converted=0;
  const unsigned char *reader, *limit;
  u8_byte *start=(u8_byte *)xf->u8_inbuf, *cur=(u8_byte *)xf->u8_read;
  u8_byte *end=(u8_byte *)xf->u8_inlim;
  int blocking=u8_get_blocking(xf->u8_xfd);
  if (cur>start) {
    /* First, if we've read anything at all, remove it, compressing the
       input buffer to make more space. */
    memmove(start,cur,unread_bytes);
    /* Now we're reading form the front of the buffer. */
    xf->u8_read=xf->u8_inbuf;
    /* And the limit of valid data is just unread_bytes further on */
    end= xf->u8_inlim= xf->u8_inbuf+unread_bytes;
    /* Null terminate it, if there's space (there has to be) */
    start[unread_bytes]='\0';
    cur=start+unread_bytes;}
  /* Now, fill the read buffer from the input socket */
  bytes_read=read(xf->u8_xfd,
                  /* These are the bytes still to be converted in xbuf */
                  xf->u8_xbuf+xf->u8_xbuflive,
                  xf->u8_xbuflim-xf->u8_xbuflive);
  /* If you had trouble or didn't get any data, return zero or the error code. */
  if (bytes_read<=0) {
    return bytes_read;}
  /* Update the buflen to reflect what we read from the socket */
  xf->u8_xbuflive=xf->u8_xbuflive+bytes_read;
  /* Do the conversion.  First we set up _reader_ and _limit_ to scan
     the data we just read from the socket */
  reader=xf->u8_xbuf; limit=reader+xf->u8_xbuflive;
  /* Now we initialize a temporary output stream from our input
     buffer, arranging the output to write to the end of the valid
     input. Conversion will write to this output stream, filling up
     the input buffer. */
  {U8_SETUP_OUTPUT( &tmpout,
                    (xf->u8_bufsz),(xf->u8_inbuf),end,
                    (((xf->u8_streaminfo)&(U8_STREAM_MALLOCD))?
                     (U8_STREAM_MALLOCD):
                     (U8_FIXED_STREAM)) );}
  /* Now we do the actual conversion, where u8_convert writes into the
     string stream passed it as its first argument. If this has any trouble,
     it will stop and reader will hold the most recent conversion state. */
  convert_val=
    u8_convert(xf->u8_xencoding,(xf->u8_streaminfo&U8_STREAM_CRLFS),
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
  xf->u8_read=tmpout.u8_outbuf;
  /* The limit of the valid input is the end of the tmpout stream. */
  xf->u8_inlim=tmpout.u8_write;
  xf->u8_bufsz=tmpout.u8_bufsz;
  /* Just in case tmpout grew while being filled but was originally static */
  if ((tmpout.u8_streaminfo)&(U8_STREAM_MALLOCD))
    xf->u8_streaminfo=(xf->u8_streaminfo)|(U8_STREAM_MALLOCD);
  /* Now, overwrite what you've converted with what you haven't converted
     yet and updated the buflen. */
  bytes_converted=reader-xf->u8_xbuf;
  memmove(xf->u8_xbuf,xf->u8_xbuf+bytes_converted,xf->u8_xbuflive-bytes_converted);
  xf->u8_xbuflive=xf->u8_xbuflive-bytes_converted;
  if (convert_val<0) {
    /* If you erred, return the errval */
    return convert_val;}
  /* Return the number of new unread UTF-8 bytes */
  else {
    return (xf->u8_inlim-xf->u8_read)-unread_bytes;}
}
U8_EXPORT int u8_init_xinput(struct U8_XINPUT *xi,int fd,u8_encoding enc)
{
  if (fd<0) return -1;
  else {
    U8_INIT_INPUT(((u8_input)xi),U8_DEFAULT_XFILE_BUFSIZE);
    xi->u8_xfd=fd;
    xi->u8_xbuf=u8_malloc(U8_DEFAULT_XFILE_BUFSIZE);
    xi->u8_xbuflim=U8_DEFAULT_XFILE_BUFSIZE;
    xi->u8_fillfn=(u8_fillfn)u8_fill_xinput;
    xi->u8_closefn=(u8_input_closefn)u8_close_xinput;
    xi->u8_streaminfo |= U8_STREAM_OWNS_XBUF;
    if (isatty(fd)) xi->u8_streaminfo |= U8_STREAM_TTY;
    xi->u8_xencoding=enc;
    xi->u8_xbuflive=0;
    return fd;}
}

U8_EXPORT struct U8_XINPUT *u8_open_xinput(int fd,u8_encoding enc)
{
  if (fd<0) return NULL;
  else {
    struct U8_XINPUT *xi=u8_alloc(struct U8_XINPUT);
    if (u8_init_xinput(xi,fd,enc)>=0) {
      xi->u8_streaminfo=xi->u8_streaminfo|U8_STREAM_MALLOCD;
      u8_register_open_xfile((u8_xfile)xi);
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
  else if (xi->u8_xbuflive>=bufsiz) return 0;
  else {
    off_t read_off=xi->u8_read-xi->u8_inbuf;
    off_t read_lim=xi->u8_inlim-xi->u8_inbuf;
    u8_byte *newbuf=(bufsiz>xi->u8_bufsz) ?
      (u8_realloc(xi->u8_inbuf,bufsiz)) : (NULL);
    u8_byte *newxbuf= ((xi->u8_xbuflim)<bufsiz) ?
      (u8_realloc(xi->u8_xbuf,bufsiz)) : (NULL);
    if ((newbuf) && (newbuf != xi->u8_inbuf)) {
      xi->u8_inbuf=newbuf;
      xi->u8_read=newbuf+read_off;
      xi->u8_inlim=newbuf+read_lim;
      xi->u8_bufsz=bufsiz;}
    if ((newxbuf) && (newxbuf != xi->u8_xbuf)) {
      xi->u8_xbuf=newxbuf;
      xi->u8_xbuflim=bufsiz;}
    return 1;}
}

U8_EXPORT struct U8_XINPUT *u8_open_input_file
(u8_string filename,u8_encoding enc,int flags,int perm)
{
  int fd, open_errno = 0;
  u8_string realname=u8_realpath(filename,NULL);
  char *fname=u8_tolibc(realname);
  if (flags<=0)
    flags=flags|O_RDONLY;
  if (!((flags&O_RDONLY) || (flags&O_RDWR)))
    flags=flags|O_RDONLY;
  if (perm<=0) perm=XFILE_OPEN_PERMS;
  fd=open(fname,flags,perm);
  if (fd<0) {open_errno=errno; errno=0;}
  u8_free(fname); u8_free(realname);
  if (fd>=0) {
    struct U8_XINPUT *xi=u8_open_xinput(fd,enc);
    xi->u8_streaminfo |= (U8_STREAM_OWNS_SOCKET|U8_STREAM_CAN_SEEK);
    return xi;}
  else {
    u8_seterr(u8_strerror(open_errno),"u8_open_input_file",u8_strdup(filename));
    return NULL;}
}

U8_EXPORT void u8_close_xinput(struct U8_XINPUT *f)
{
  u8_deregister_open_xfile((u8_xfile)f);
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_inbuf);
  f->u8_inbuf=f->u8_read=f->u8_inlim=NULL;
  f->u8_bufsz=0;
  if (f->u8_streaminfo&U8_STREAM_OWNS_XBUF) {
    unsigned char *buf=f->u8_xbuf;
    f->u8_xbuf=NULL;
    u8_free(buf);}
  if (f->u8_streaminfo&U8_STREAM_OWNS_SOCKET)
    close(f->u8_xfd);
  f->u8_xfd=-1;
  if (f->u8_streaminfo&U8_STREAM_MALLOCD)
    u8_free(f);
}

/* OUTPUT */

static int flush_xoutput(struct U8_XOUTPUT *xf)
{
  unsigned char *buf=xf->u8_xbuf;
  int buflen=xf->u8_xbuflim;
  while (xf->u8_write>xf->u8_outbuf) {
    /* Pick the region to change */
    const u8_byte *scan=xf->u8_outbuf, *scan_end=xf->u8_write;
    /* Convert the data to the appropriate encoding */
    u8_localize(xf->u8_xencoding,&scan,scan_end,
                xf->u8_xescape,(xf->u8_streaminfo&U8_STREAM_CRLFS),
                buf,&buflen);
    /* Write it out to the file descriptor */
    if (writeall(xf->u8_xfd,buf,buflen)<0) {
      if (errno) u8_graberr(errno,"flush_xoutput",NULL);
      return -1;}
    /* Write over what you just wrote out with what you haven't written
       yet and move the point to the end of the unwritten data. */
    memmove(xf->u8_outbuf,scan,xf->u8_write-scan);
    xf->u8_write=xf->u8_outbuf+(xf->u8_write-scan);
    /* Reset the buflen to the limit */
    buflen=xf->u8_xbuflim;}

  return xf->u8_outlim-xf->u8_write;
}

U8_EXPORT int u8_init_xoutput
(struct U8_XOUTPUT *xo,int fd,u8_encoding enc)
{
  if (fd<0) return -1;
  else {
    U8_INIT_OUTPUT((u8_output)xo,U8_DEFAULT_XFILE_BUFSIZE);
    xo->u8_xfd=fd; xo->u8_xbuf=u8_malloc(U8_DEFAULT_XFILE_BUFSIZE);
    xo->u8_xbuflive=0; xo->u8_xbuflim=U8_DEFAULT_XFILE_BUFSIZE;
    xo->u8_xencoding=enc; xo->u8_xbuf[0]='\0';
    xo->u8_flushfn=(u8_flushfn)flush_xoutput;
    xo->u8_closefn=(u8_output_closefn)u8_close_xoutput;
    xo->u8_streaminfo=xo->u8_streaminfo|U8_STREAM_OWNS_XBUF;
    if (isatty(fd)) xo->u8_streaminfo |= U8_STREAM_TTY;
    return 1;}
}

U8_EXPORT struct U8_XOUTPUT *u8_open_xoutput(int fd,u8_encoding enc)
{
  if (fd<0) return NULL;
  else {
    struct U8_XOUTPUT *xo=u8_alloc(struct U8_XOUTPUT);
    if (u8_init_xoutput(xo,fd,enc)>=0) {
      xo->u8_streaminfo=xo->u8_streaminfo|U8_STREAM_MALLOCD;
      if (isatty(fd)) xo->u8_streaminfo |= U8_STREAM_TTY;
      u8_register_open_xfile((u8_xfile)xo);
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
  else if (xo->u8_xbuflive>=bufsiz) return 0;
  else {
    off_t write_off=xo->u8_write-xo->u8_outbuf;
    off_t write_lim=xo->u8_outlim-xo->u8_outbuf;
    u8_byte *newbuf=(bufsiz>xo->u8_bufsz) ?
      (u8_realloc(xo->u8_outbuf,bufsiz)) : (NULL);
    u8_byte *newxbuf= ((xo->u8_xbuflim)<bufsiz) ?
      (u8_realloc(xo->u8_xbuf,bufsiz)) : (NULL);
    if ((newbuf) && (newbuf != xo->u8_outbuf)) {
      xo->u8_outbuf=newbuf;
      xo->u8_write=newbuf+write_off;
      xo->u8_outlim=newbuf+write_lim;
      xo->u8_bufsz=bufsiz;}
    if ((newxbuf) && (newxbuf != xo->u8_xbuf)) {
      xo->u8_xbuf=newxbuf;
      xo->u8_xbuflim=bufsiz;}
    return 1;}
}

U8_EXPORT struct U8_XOUTPUT *u8_open_output_file
(u8_string filename,u8_encoding enc,int flags,int perm)
{
  u8_string realname=u8_realpath(filename,NULL);
  char *fname=u8_tolibc(realname);
  int fd, open_errno = 0;
  if (flags<=0)
    flags=flags|O_WRONLY|O_TRUNC|O_CREAT;
  if (!((flags&O_WRONLY) || (flags&O_RDWR)))
    flags=flags|O_WRONLY;
  if (perm<=0) perm=XFILE_OPEN_PERMS;
  fd=open(fname,flags,perm);
  if (fd<0) {open_errno=errno; errno=0;}
  u8_free(fname); u8_free(realname);
  if (fd>=0) {
    struct U8_XOUTPUT *out=u8_open_xoutput(fd,enc);
    if (out) out->u8_streaminfo |= (U8_STREAM_OWNS_SOCKET|U8_STREAM_CAN_SEEK);
    return out;}
  else {
    u8_seterr(u8_strerror(open_errno),"u8_open_output_file",
              u8_strdup(filename));
    return NULL;}
}

U8_EXPORT off_t u8_getpos(struct U8_STREAM *f)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      int delta=out->u8_write-out->u8_outbuf;
      off_t pos=lseek(out->u8_xfd,0,SEEK_CUR);
      if (pos<0) return pos; else return pos+delta;}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      int delta=in->u8_inlim-in->u8_read;
      off_t pos=lseek(in->u8_xfd,0,SEEK_CUR);
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
      return lseek(out->u8_xfd,off,SEEK_SET);}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      u8_byte *buf=(u8_byte *)in->u8_inbuf;
      in->u8_read=in->u8_inlim=in->u8_inbuf; *buf='\0';
      return lseek(in->u8_xfd,off,SEEK_SET);}
  else {
    u8_seterr(u8_nopos,"u8_setpos",NULL);
    return -1;}
}

U8_EXPORT off_t u8_endpos(struct U8_STREAM *f)
{
  if ((f->u8_streaminfo)&(U8_STREAM_CAN_SEEK))
    if ((f->u8_streaminfo)&(U8_OUTPUT_STREAM)) {
      struct U8_XOUTPUT *out=(struct U8_XOUTPUT *)f;
      off_t cur=lseek(out->u8_xfd,0,SEEK_CUR);
      off_t end=lseek(out->u8_xfd,0,SEEK_END);
      if (lseek(out->u8_xfd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1;}
      else return end;}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      off_t cur=lseek(in->u8_xfd,0,SEEK_CUR);
      off_t end=lseek(in->u8_xfd,0,SEEK_END);
      if (lseek(in->u8_xfd,cur,SEEK_SET)<0) {
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
      int delta=out->u8_write-out->u8_outbuf;
      off_t cur=lseek(out->u8_xfd,0,SEEK_CUR);
      off_t end=lseek(out->u8_xfd,0,SEEK_END);
      if (lseek(out->u8_xfd,cur,SEEK_SET)<0) {
        u8_seterr("lost current pos","u8_endpos",NULL);
        return -1.0;}
      else return 100.0*(((double)(cur+delta))/((double)end));}
    else {
      struct U8_XINPUT *in=(struct U8_XINPUT *)f;
      int delta=in->u8_inlim-in->u8_read;
      off_t cur=lseek(in->u8_xfd,0,SEEK_CUR);
      off_t end=lseek(in->u8_xfd,0,SEEK_END);
      if (lseek(in->u8_xfd,cur,SEEK_SET)<0) {
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
  u8_deregister_open_xfile((u8_xfile)f);
  if (f->u8_streaminfo&U8_STREAM_OWNS_BUF) u8_free(f->u8_outbuf);
  f->u8_outbuf=f->u8_write=f->u8_outlim=NULL; f->u8_bufsz=0;
  if (f->u8_streaminfo&U8_STREAM_OWNS_XBUF) u8_free(f->u8_xbuf);
  f->u8_xbuf=NULL;
  if (f->u8_streaminfo&U8_STREAM_OWNS_SOCKET) close(f->u8_xfd);
  f->u8_xfd=-1; f->u8_xbuflive=0; f->u8_xbuflim=0;
  if (f->u8_streaminfo&U8_STREAM_MALLOCD) u8_free(f);
}

U8_EXPORT void u8_flush_xoutput(struct U8_XOUTPUT *f)
{
  u8_flush((u8_output)f);
  fsync(f->u8_xfd);
  U8_CLEAR_ERRNO();
}

/* Tracking open XFILEs */

#if U8_THREADS_ENABLED
static u8_mutex xfile_registry_lock;
#endif

static struct U8_OPEN_XFILES *open_xfiles=NULL;

U8_EXPORT void u8_register_open_xfile(struct U8_XFILE *out)
{
  struct U8_OPEN_XFILES *new=u8_alloc(struct U8_OPEN_XFILES);
  u8_lock_mutex((&xfile_registry_lock));
  new->xfile=out;
  new->next=open_xfiles;
  open_xfiles=new;
  u8_unlock_mutex((&xfile_registry_lock));
}

U8_EXPORT void u8_deregister_open_xfile(struct U8_XFILE *out)
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
      ((u8_xoutput)scan->xfile)->u8_flushfn((U8_OUTPUT *)(scan->xfile));
    if ((scan->xfile->u8_streaminfo)&U8_STREAM_OWNS_SOCKET)
      close(scan->xfile->u8_xfd);
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

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
