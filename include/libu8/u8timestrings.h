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

#ifndef LIBU8_U8TIMESTRINGS_H
#define LIBU8_U8TIMESTRINGS_H 1
#define LIBU8_U8TIMESTRINGS_H_VERSION __FILE__

#include "u8xtime.h"

/** Populates a U8_XTIME pointer based on an ISO-8601 formated timestamp.
    @param s a UTF-8 (ASCII) string containing an ISO-8601 timestamp.
    @param xt a pointer to a U8_XTIME structure.
    @returns the time_t value for the structure
**/
U8_EXPORT time_t u8_iso8601_to_xtime(u8_string s,struct U8_XTIME *xt);

/** Outputs an ISO-8601 representation of a U8_XTIME structure.
    @param ss a pointer to a U8_OUTPUT stream
    @param xt a pointer to a U8_XTIME structure.
    @returns void

This takes xtime pointer and outputs an ISO8601 representation of it,
obeying the precision of the XTIME structure
**/
U8_EXPORT void u8_xtime_to_iso8601(struct U8_OUTPUT *ss,struct U8_XTIME *xt);

/** Outputs an ISO-8601 of an U8_XTIME structure, with variations.
    @param ss a pointer to a U8_OUTPUT stream
    @param xt a pointer to a U8_XTIME structure.
    @param flags an int ORing together various display options
    @returns void

The flags arg provides display options, which can be ORd together:
  U8_ISO8601_BASIC:   use the more compact BASIC format)
  U8_ISO8601_NOZONE:  don't show the timezone information
  U8_ISO8601_NOMSECS: don't show milli/micro/nano seconds
  U8_ISO8601_UTC:     display time as UTC
**/
U8_EXPORT void u8_xtime_to_iso8601_x(struct U8_OUTPUT *ss,struct U8_XTIME *xt,int flags);

/** Populates a U8_XTIME pointer based on an RFC822 formated timestamp.
    @param s a UTF-8 (ASCII) string containing an ISO-8601 timestamp.
    @param xt a pointer to a U8_XTIME structure.
    @returns the time_t value for the structure
**/
U8_EXPORT time_t u8_rfc822_to_xtime(u8_string s,struct U8_XTIME *xt);

/** Outputs an RFC822 representation of a U8_XTIME structure.
    @param ss a pointer to a U8_OUTPUT stream
    @param xt a pointer to a U8_XTIME structure.
    @returns void
**/
U8_EXPORT void u8_xtime_to_rfc822(struct U8_OUTPUT *ss,struct U8_XTIME *xt);

/** Outputs an RFC822 representation of a U8_XTIME structure.
    @param ss a pointer to a U8_OUTPUT stream
    @param xt a pointer to a U8_XTIME structure.
    @param zone a timezone offset
    @param opts display option flags (currently unused)
    @returns void
**/
U8_EXPORT void u8_xtime_to_rfc822_x(struct U8_OUTPUT *ss,struct U8_XTIME *xt,int zone,int opts);

/** Returns a GMT offset in seconds for a given string.
    Negative numbers indicate offsets to clock times earlier than (east of) GMT.    Strings such as -5:00 and +2:00 are handled, as are some commoner,
    unambigous timezone codes.
    @param s a UTF-8 character string
    @param dflt an integer value used as a default if a valid offset
             cannot be determined
    @returns an int representing seconds offset from GMT
**/
U8_EXPORT int u8_parse_tzspec(u8_string s,int dflt);

/** Parses and applies timezone information to a U8_XTIME structure
    Negative numbers indicate offsets to clock times earlier than (east of) GMT.
    Strings such as -5:00 and +2:00 are handled, as are some commoner,
    unambigous timezone codes.
    @param xt a pointer to a U8_XTIME structure
    @param s a UTF-8 character string
    @returns void
**/
U8_EXPORT void u8_apply_tzspec(struct U8_XTIME *xt,u8_string s);

#endif

