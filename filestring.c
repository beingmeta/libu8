/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2010 beingmeta, inc.
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

static char versionid[]  MAYBE_UNUSED=
  "$Id$";

#include "libu8/libu8io.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

u8_condition u8_CantOpenFile=_("Can't open file for reading");

U8_EXPORT unsigned char *u8_filedata(u8_string filename,int *n_bytes)
{
  u8_string abspath=u8_localpath(filename);
  unsigned int size, to_read, bytes_read=0; unsigned char *data;
  FILE *f=u8_fopen(abspath,"rb");
  if (f==NULL) {
    u8_seterr(u8_CantOpenFile,"u8_filedata",abspath);
    *n_bytes=-1;
    return NULL;}
  fseek(f,0,SEEK_END);
  to_read=size=ftell(f);
  /* We malloc an extra byte in case we're going to use this as a string. */
  data=u8_malloc(size+1);
  fseek(f,0,SEEK_SET);
  while (to_read) {
    int delta=fread(data+bytes_read,1,to_read,f);
    if (delta>0) {
      to_read=to_read-delta; bytes_read=bytes_read+delta;}
    else if (errno==EAGAIN) {errno=0; continue;}
    else {
      u8_free(data);
      u8_graberr(-1,"u8_filedata",abspath);
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
  u8_byte *buf; int n_bytes;
  buf=u8_filedata(filename,&n_bytes);
  if ((buf==NULL) || (n_bytes<0)) return NULL;
  buf[n_bytes]='\0';
  /* If there are no bytes, there can be no conversion */
  if (n_bytes==0) return buf;
  if (encname == NULL) enc=NULL;
  else if (strcmp(encname,"auto")==0)
    enc=u8_guess_encoding(buf);
  else enc=u8_get_encoding(encname);
  if (enc) {
    struct U8_OUTPUT out;
    unsigned char *scan=buf;
    U8_INIT_OUTPUT(&out,n_bytes+n_bytes/2);
    u8_convert(enc,1,&out,&scan,buf+n_bytes);
    u8_free(buf);
    return out.u8_outbuf;}
  else return (u8_string) buf;
}
