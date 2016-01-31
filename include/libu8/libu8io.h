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

#ifndef LIBU8_LIBU8IO_H
#define LIBU8_LIBU8IO_H 1
#define LIBU8_LIBU8IO_H_VERSION __FILE__

#include "libu8/libu8.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8convert.h"
#include "libu8/u8fileio.h"
#include "libu8/xfiles.h"

U8_EXPORT u8_condition u8_CantOpenFile;

U8_EXPORT u8_string u8_filestring(u8_string path,u8_string encname);
U8_EXPORT unsigned char *u8_filedata(u8_string path,int *size);

#endif
