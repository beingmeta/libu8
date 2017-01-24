/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2017 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8SLEEP_H
#define LIBU8_U8SLEEP_H 1
#define LIBU8_U8SLEEP_H_VERSION __FILE__

U8_EXPORT
/** Makes the current thread sleeps for seconds and fractions of
    seconds. u8_sleep(3.7) will return after (roughly) 3.7 seconds.

    @param seconds how long to sleep
    @returns a time_t value (the current time in seconds)
**/
time_t u8_sleep(double seconds);

#endif
