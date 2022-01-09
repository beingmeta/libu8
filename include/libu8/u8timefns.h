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

#ifndef LIBU8_TIMEFNS_H
#define LIBU8_TIMEFNS_H 1
#define LIBU8_TIMEFNS_H_VERSION __FILE__

/** \file u8timefns.h
    Provides various functions for manipulating information about dates and
    times.  It's chief data structure is struct U8_XTIME, which combines a
    broken down time representation with a nanosecond timer and information
    about timezone, dst offset, and precision.	Significantly, these
    functions are intended to represent most possible timezones, rather
    than just the local time and GMT.  U8_XTIME structures
    are populated by u8_now, u8_localtime, and u8_iso8601_to_xtime.
    u8_mktime returns a time_t value from a U8_XTIME pointer and
    u8_xtime_to_iso8601 outputs a printed representation of U8_XTIME.
    (Also see the explanation of the %t conversion in the u8printf
    docmentation.)  Finally, u8_xtime_diff and u8_xtime_plus
    do precision-sensitive arithmetic on U8_XTIME structures.
**/

#include "u8xtime.h"

#include "u8mktime.h"

#include "u8xtimefns.h"

#include "u8timestrings.h"

/* Miscellanous Functions */

#include "u8millitime.h"

#include "u8sleep.h"

#include "u8uuid.h"

#endif
