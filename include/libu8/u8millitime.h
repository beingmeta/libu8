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

#ifndef LIBU8_MILLITIME_H
#define LIBU8_MILLTIME_H 1
#define LIBU8_MILITIME_H_VERSION __FILE__

/** \file u8elapsed.h

    Provides various functions for getting timer related information

**/

U8_EXPORT
/** Returns elapsed time in seconds since some moment after application
    startup.
    @returns a double indicating seconds
**/
double u8_elapsed_time(void);

U8_EXPORT
/** Returns the number of microseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting microseconds
*/
long long u8_microtime(void);

U8_EXPORT
/** Returns the number of milliseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting milliseconds
**/
long long u8_millitime(void);

#endif
