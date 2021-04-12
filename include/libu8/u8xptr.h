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

/** \file u8xptr.h
    This definitions support storage of pointers with
    type and allocation (free) information
**/

#ifndef LIBU8_U8XPTR_H
#define LIBU8_U8XPTR_H 1
#define LIBU8_U8XPTR_H_VERSION __FILE__

#include "u8strings.h"

/** The U8_XPTR wraps a pointer object with a free function and
    a typeid string **/
typedef struct U8_XPTR {
  u8_context u8x_typeid;
  void (*u8x_freefn)(void *);
  void *u8x_ptrval;} *u8_xptr;

void (*u8_xptr_freefn)(struct U8_XPTR *);


#endif /* LIBU8_U8XPTR_H */
