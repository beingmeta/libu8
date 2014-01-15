/* -*- Mode: C; Character-encoding: utf-8; -*- */

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

#include "libu8/source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

U8_EXPORT void u8_init_timefns_c(void);
U8_EXPORT void u8_init_filefns_c(void);
U8_EXPORT void u8_init_netfns_c(void);
U8_EXPORT void u8_init_srvfns_c(void);

U8_EXPORT void u8_init_pathfns_c(void);
U8_EXPORT void u8_init_fileio_c(void);
U8_EXPORT void u8_init_rusage_c(void);
U8_EXPORT void u8_init_digestfns_c(void);
U8_EXPORT void u8_init_cryptofns_c(void);

U8_EXPORT int u8_initialize_fns()
{
  u8_register_source_file(_FILEINFO);

  u8_init_timefns_c();
  u8_init_filefns_c();
  u8_init_netfns_c();
  u8_init_srvfns_c();

  u8_init_pathfns_c();
  u8_init_fileio_c();
  u8_init_rusage_c();
  u8_init_digestfns_c();
  u8_init_cryptofns_c();

  return 8069;
}
