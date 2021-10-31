/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2021 beingmeta, LLC
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

void init_timefns_c(void);
void init_filefns_c(void);
void init_netfns_c(void);
void init_srvfns_c(void);

void init_pathfns_c(void);
void init_fileio_c(void);
void init_rusage_c(void);
void init_digestfns_c(void);
void init_cryptofns_c(void);

U8_EXPORT int u8_initialize_fns()
{
  u8_register_source_file(_FILEINFO);

  init_timefns_c();
  init_filefns_c();
  init_netfns_c();
  init_srvfns_c();

  init_pathfns_c();
  init_fileio_c();
  init_rusage_c();
  init_digestfns_c();
  init_cryptofns_c();

  return 8069;
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
