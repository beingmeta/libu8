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

#include "libu8/u8source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8io.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/stat.h>

u8_condition u8_IrregularFile=_("does not resolve to a regular file");
u8_condition u8_CantOpenFile=_("Can't open file for reading");

U8_EXPORT unsigned char *u8_filedata(u8_string filename,ssize_t *n_bytes)
{
  u8_string abspath=u8_localpath(filename);
  size_t size, to_read, bytes_read=0;
  unsigned char *data;
  struct stat info;
  int rv = stat(abspath,&info);
  if (rv != 0) {
    u8_graberrno("u8_filedata",abspath);
    return NULL;}
  else if ((info.st_mode&S_IFMT)!=S_IFREG) {
    *n_bytes=-1;
    return u8err(NULL,u8_IrregularFile,"u8_filedata",abspath);}
  else to_read = size = info.st_size;
  FILE *f=u8_fopen(abspath,"rb");
  if (f==NULL) {
    if (errno) u8_graberrno("u8_filedata",abspath);
    *n_bytes=-1;
    return u8err(NULL,u8_CantOpenFile,"u8_filedata",abspath);}
  /* We malloc an extra byte in case we're going to use this as a string. */
  data=u8_malloc(size+1);
  memset(data,0,size+1);
  while (to_read) {
    int delta=fread(data+bytes_read,1,to_read,f);
    if (delta>0) {
      to_read=to_read-delta; bytes_read=bytes_read+delta;}
    else if (errno==EAGAIN) {errno=0; continue;}
    else {
      u8_graberrno("u8_filedata",abspath);
      u8_free(data);
      *n_bytes=-1;
      u8_fclose(f);
      return NULL;}}
  *n_bytes=size;
  u8_fclose(f);
  u8_free(abspath);
  return data;
}

U8_EXPORT u8_string u8_filestring(u8_string filename,u8_string encname)
{
  struct U8_TEXT_ENCODING *enc=NULL;
  u8_byte *buf; ssize_t n_bytes;
  buf=u8_filedata(filename,&n_bytes);
  if ((buf==NULL) || (n_bytes<0)) return NULL;
  buf[n_bytes]='\0';
  /* If there are no bytes, there can be no conversion */
  if (n_bytes==0) return buf;
  if (encname == NULL) enc=NULL;
  else if (strcmp(encname,"auto")==0)
    enc=u8_guess_encoding((u8_string)buf);
  else enc=u8_get_encoding(encname);
  if (enc) {
    struct U8_OUTPUT out; int retval=0;
    const unsigned char *scan=buf;
    U8_INIT_STATIC_OUTPUT(out,n_bytes+n_bytes/2);
    retval=u8_convert(enc,1,&out,&scan,buf+n_bytes);
    u8_free(buf);
    if (retval<0) {
      u8_free(out.u8_outbuf);
      return NULL;}
    return out.u8_outbuf;}
  else return (u8_string) buf;
}

U8_EXPORT void u8_init_filestring_c()
{
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
