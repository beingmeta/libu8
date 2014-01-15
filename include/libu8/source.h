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

#ifndef LIBU8_U8BUILD_H
#define LIBU8_U8BUILD_H 1
#define LIBU8_U8BUILD_H_VERSION __FILE__

#define _GNU_SOURCE

#include "libu8/config.h"
#include "libu8/revision.h"

#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if MINGW
#define WIN32 1
#endif

#if WIN32
#include <windows.h>
#define random rand
#define srandom srand
#define sleep(x) Sleep(x*1000)
#endif

#define LIBU8_SOURCE 1

#endif

