/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2013 beingmeta, inc.
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

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8io.h"

U8_EXPORT void u8_init_convert_c(void);
U8_EXPORT void u8_init_xfiles_c(void);
U8_EXPORT void u8_init_filestring_c(void);

static int u8io_init_done=0;

U8_EXPORT int u8_initialize_io()
{
  if (u8io_init_done) return u8io_init_done;
  else u8io_init_done=8069;
  u8_register_source_file(_FILEINFO);
  u8_init_convert_c();
  u8_init_xfiles_c();
  u8_init_filestring_c();
  return u8io_init_done;
}

