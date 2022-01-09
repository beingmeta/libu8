/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 beingmeta, LLC
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8MKTIME_H
#define LIBU8_U8MKTIME_H 1
#define LIBU8_U8MKTIME_H_VERSION __FILE__

/** Populates a U8_XTIME structure based on the current local time.
    This attempts to get the most precise representation possible.  **/
U8_EXPORT time_t u8_now(struct U8_XTIME *xt);

/** Returns a POSIX time_t representation for a broken down
    U8_XTIME structure, including timezone and dst offsets
    Also fills out day of week, day of year, etc.
    @param xt a pointer to a populated U8_XTIME structure.
    @returns a POSIX time_t value indicating seconds past the epoch
    for the moment represented by @a xt
**/
U8_EXPORT time_t u8_mktime(struct U8_XTIME *xt);

/** Returns a POSIX time_t representation for a broken down
    U8_XTIME structure, interpreting the time locally and filling
    out timezone and daylight offsets.
    Also fills out day of week, day of year, etc.
    @param xt a pointer to a populated U8_XTIME structure.
    @returns a POSIX time_t value indicating seconds past the epoch
    for the moment represented by @a xt
**/
U8_EXPORT time_t u8_mklocaltime(struct U8_XTIME *xt);

#endif
