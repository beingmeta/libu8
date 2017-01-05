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

#ifndef LIBU8_U8XTIMEFNS_H
#define LIBU8_U8XTIMEFNS_H 1
#define LIBU8_U8XTIMEFNS_H_VERSION __FILE__

#include "u8xtime.h"

/** Sets the time precision of a U8_XTIME structure.
    @param xt a pointer to a U8_XTIME structure.
    @param tp a precision value (u8_tmprec), including
       u8_year, u8_month, u8_day, u8_hour, u8_minute, u8_second,
       u8_millisecond, u8_microsecond, or u8_nanosecond.
    @returns void
**/
U8_EXPORT void u8_set_xtime_precision(struct U8_XTIME *xt,u8_tmprec tp);

/** Sets the GMT offset of an XTIME structure
    @param xt a pointer to a U8_XTIME structure
    @param tzoff (int) a timezone offset in seconds
    @param dstoff (int) a local (daylight savings/summer offset) in seconds
    @returns void
**/
U8_EXPORT void u8_set_xtime_gmtoff(struct U8_XTIME *xt,int tzoff,int dstoff);

/** Returns a non-integral difference, in seconds, between U8_XTIME pointers.
    Returns the number of seconds (possibly non-integral) that the moment
     described by @a yt occurs after @a xt.  This is negative if @a yt
     occurreed before @a xt.
    This incorporates timezone offsets and computes a value based on
      the least precise of the two timestamp pointers.
    @param xt a pointer to a U8_XTIME structure.
    @param yt a pointer to a U8_XTIME structure.
    @returns a double representing a number of seconds
**/
U8_EXPORT double u8_xtime_diff(struct U8_XTIME *xt,struct U8_XTIME *yt);

/** Adds a specific interval to a U8_XTIME representation.
    The actual change will reflect the precision of the XTIME representation.
    Adding a negative value moves the point back in time.
    @param xt a pointer to a U8_XTIME structure
    @param delta an offset in seconds from the current time represented by @a xt.
    @returns void
**/
U8_EXPORT void u8_xtime_plus(struct U8_XTIME *xt,double delta);

#endif
