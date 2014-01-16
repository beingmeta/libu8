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

#define U8_INLINE_IO 1

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8bytebuf.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

/* Byte buffers */

u8_condition u8_FixedBufOverflow=_("overflow of fixed byte buffer");

U8_EXPORT int _u8_bufwrite(struct U8_BYTEBUF *bb,unsigned char *buf,int len)
{
  if (len==0) return 0;
  else if (bb->u8_buf==NULL) {
    int bufsize=(((bb->u8_growbuf)>1)?(bb->u8_growbuf):(U8_BYTEBUF_DEFAULT));
    unsigned char *buf=u8_malloc(bufsize);
    memset(buf,0,bufsize);
    if (buf) {
      bb->u8_buf=bb->u8_ptr=buf;
      bb->u8_lim=buf+bufsize;}
    else return u8_reterr(u8_MallocFailed,"u8_bufwrite",NULL);}
  else if (((bb->u8_ptr)+len)>=(bb->u8_lim)) {
    if (bb->u8_growbuf==0) 
      return u8_reterr(u8_FixedBufOverflow,"u8_bufwrite",NULL);
    else {
      unsigned int ptroff=(bb->u8_ptr)-(bb->u8_buf);
      unsigned int bufsize=(bb->u8_lim)-(bb->u8_buf);
      unsigned int newsize=
	((bb->u8_growbuf==1)?(bufsize*2):(bufsize+bb->u8_growbuf));
      unsigned char *newbuf=u8_realloc(bb->u8_buf,newsize);
      if (newbuf) {
	memset(newbuf+ptroff,0,newsize-ptroff);
	bb->u8_buf=newbuf;
	bb->u8_ptr=newbuf+ptroff;
	bb->u8_lim=newbuf+newsize;}
      else return u8_reterr(u8_MallocFailed,"u8_bufwrite",NULL);}}
  memcpy(bb->u8_ptr,buf,len);
  bb->u8_ptr=bb->u8_ptr+len;
  return len;
}

U8_EXPORT int u8_bbreader(unsigned char *buf,int len,struct U8_BYTEBUF *bb)
{
  return u8_bufread(bb,buf,len);
}
U8_EXPORT int u8_bbwriter(unsigned char *buf,int len,struct U8_BYTEBUF *bb)
{
  return u8_bufwrite(bb,buf,len);
}

/* Initialization function (just records source file info) */

U8_EXPORT void u8_init_bytebuf_c()
{
  u8_register_source_file(_FILEINFO);
}
