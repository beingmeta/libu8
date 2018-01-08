/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2018 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8UUID_H
#define LIBU8_U8UUID_H 1
#define LIBU8_U8UUID_H_VERSION __FILE__

/** \file u8uuid.h
    
    Provides functions and type definitions for constructing and
    deconstructing UUIDs.

 **/

#include "u8xtime.h"

/* UUIDs */

typedef unsigned char U8_UUID[16];
typedef unsigned char *u8_uuid;

U8_EXPORT
/**
   Gets a 16-byte UUID based on the time and MAC address
   Arguments: none
   @param xtime an xtime structure
   @param nodeid a 48-bit node id, -1 means use the default
   @param clockid a clock id, -1 means make one up
   @param buf a (possibly NULL) 16-byte buffer to use/return
   @returns an array of 16 bytes
 */
u8_uuid u8_consuuid(struct U8_XTIME *xtime,long long nodeid,short clockid,
                    u8_uuid buf);

U8_EXPORT
/**
   Gets a 16-byte UUID based on the time and MAC address
   @param buf a (possibly NULL) 16-byte buffer to use/return
   @returns an array of 16 bytes
 */
u8_uuid u8_getuuid(u8_uuid buf);


U8_EXPORT
/**
   Gets the text representation of a 16-byte UUID
   based on the time and MAC address
   @param uuid a UUID
   @param strbuf a (possibly NULL) string buffer
   @returns a UTF-8 (actually, ASCII) string
 */
u8_string u8_uuidstring(u8_uuid uuid,u8_byte *strbuf);

U8_EXPORT
/**
   Converts a text representation of a UUID into a UUID
   Arguments: a string
   @param string a UTF-8 string
   @param uuid a (possibly NULL) 16-byte buffer to use/return
   @returns an array of 16 bytes
 */
u8_uuid u8_parseuuid(u8_string string,u8_uuid uuid);


U8_EXPORT
/**
   Gets the node id for a time-based UUID, returning -1 if the UUID is not
    time-based.
   @param uuid a pointer to uuid (u8_uuid)
   @returns a long long (actually 48 bits)
 */
long long u8_uuid_nodeid(u8_uuid uuid);


U8_EXPORT
/**
   Gets the timestamp for the UUID, returning -1 if the UUID is not
    time-based.  The timestamp is the number of 100-nanosecond ticks
    since 15 October 1582, when the Gregorian calendar was adopted.
   @param uuid a pointer to uuid (u8_uuid)
   @returns a long long uuid timestamp
 */
long long u8_uuid_timestamp(u8_uuid uuid);

U8_EXPORT
/**
   Gets the time_t tick for the UUID, returning -1 if the UUID is not
    time-based or is outside of the time_t range
   @param uuid a pointer to uuid (u8_uuid)
   @returns a time_t value
 */
time_t u8_uuid_tick(u8_uuid uuid);

U8_EXPORT
/**
   Gets the time_t tick for the UUID, returning -1 if the UUID is not
    time-based or is outside of the time_t range
   @param uuid a pointer to uuid (u8_uuid)
   @param xtime a (possibly NULL) pointer to a U8_XTIME struct to use
   @returns a pointer to a U8_XTIME struct
 */
struct U8_XTIME *u8_uuid_xtime(u8_uuid uuid,struct U8_XTIME *xtime);

#endif
