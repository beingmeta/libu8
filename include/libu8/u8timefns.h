/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2015 beingmeta, inc.
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
     about timezone, dst offset, and precision.  Significantly, these
     functions are intended to represent most possible timezones, rather
     than just the local time and GMT.  U8_XTIME structures
     are populated by u8_now, u8_localtime, and u8_iso8601_to_xtime.
     u8_mktime returns a time_t value from a U8_XTIME pointer and
     u8_xtime_to_iso8601 outputs a printed representation of U8_XTIME.
     (Also see the explanation of the %t conversion in the u8printf
     docmentation.)  Finally, u8_xtime_diff and u8_xtime_plus
     do precision-sensitive arithmetic on U8_XTIME structures.
 **/

#if defined(TIME_WITH_SYS_TIME)
# include <sys/time.h>
# include <time.h>
#else
# if defined(HAVE_SYS_TIME_H)
# include <sys/time.h>
# else
# include <time.h>
# endif
#endif

#if HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

/** Identifies the precision for a timestamp, ranging from u8_year to
    u8_femtosecond.  Precisions finer than u8_nanosecond are not currently
    effective and on many platforms, precisions finer than microseconds
    are not generally significant. **/
typedef enum u8_timestamp_precision {
  u8_maxtmprec=0,
  u8_year=1, u8_month=2, u8_day=3, u8_hour=4, u8_minute=5, u8_second=6,
  u8_millisecond=7, u8_microsecond=8, u8_nanosecond=9,
  u8_picosecond=10, u8_femtosecond=11} u8_tmprec;

U8_EXPORT unsigned int u8_precision_secs[12];

#define U8_ISO8601_STD 0
#define U8_ISO8601_BASIC 1
#define U8_ISO8601_NOZONE 2
#define U8_ISO8601_NOMSECS 4
#define U8_ISO8601_UTC 8

#define U8_RFC822_STD 16
#define U8_RFC822_NOZONE 2

/** The U8_XTIME struct defines a variable precision timezone-offset
    time representation with extractable components.  **/
typedef struct U8_XTIME {
  /* Broken down time values */
  unsigned int u8_year;
  unsigned char u8_mon, u8_mday, u8_hour, u8_min, u8_sec;
  unsigned char u8_wday, u8_yday, u8_wnum;
  /* Absolute time values */
  time_t u8_tick;    /**< integral number of seconds past the POSIX epoch **/
  int u8_nsecs;      /**< number of nanoseconds beyond the last seconds tick **/
  u8_tmprec u8_prec; /**< the precision of this timestamp **/
  /** The offset of this time from GMT **/
  short u8_tzoff, u8_dstoff;} U8_XTIME;
typedef struct U8_XTIME *u8_xtime;

#define u8_dbltime(tm) \
     (((tm).tv_sec*1000000.0)+(((tm).tv_usec)*1.0))
#define u8_dbldifftime(later,earlier)                           \
  (((((later).tv_sec)-((earlier).tv_sec))*1000000.0)+((((later).tv_usec)-((earlier).tv_usec))*1.0))

/** Populates a U8_XTIME structure based on a POSIX time_t value.
    This populates the broken down components of an XTIME structure based
      on a given timezone offset (including DST compensation).
    If the xt pointer is NULL, one is consed with u8_alloc/malloc
    If the time value is negative, the current date and time is used.
      @param xt a pointer to a U8_XTIME structure
      @param tick a POSIX time_t value (seconds past the epoch)
      @param prec a u8_tmprec value indicating the precision of the timestamp
      @param nsecs the number of nanoseconds  after the indicated second
      @param tzoff the timezone offset to use in populating the structure
      @param dstoff the daylight savings time offset to use
    When the tick value is negative (referring to the current time),
      the precision argument is bumped down to the available precision.
    The nsecs argument may be ignored based on the precision argument.
**/
U8_EXPORT u8_xtime u8_init_xtime
  (struct U8_XTIME *xt,time_t tick,u8_tmprec prec,int nsecs,
   int tzoff,int dstoff);

/** Populates a U8_XTIME structure for the local timezone based
       on a POSIX time_t value.
    This populates the broken down components of an XTIME structure based
      on the local timezone
    If the xt pointer is NULL, one is consed with u8_alloc/malloc
    If the time value is negative, the current date and time is used.
    @param xt a pointer to a U8_XTIME structure
    @param tick a POSIX time_t value (seconds past the epoch)
    @param prec a u8_tmprec value indicating the precision of the timestamp
    @param nsecs the number of nanoseconds  after the indicated second
    When the tick value is negative (referring to the current time),
      the precision argument is bumped down to the available precision.
    The nsecs argument may be ignored based on the precision argument.
**/
U8_EXPORT u8_xtime u8_local_xtime
  (struct U8_XTIME *xt,time_t tick,u8_tmprec prec,int nsecs);


/** Populates a U8_XTIME structure with local time based on a POSIX
    time_t value.
    This populates the broken down components based on the current
    timezone and initializes tzoff.
    @param xt a pointer to a U8_XTIME structure
    @param tick a POSIX time_t value (seconds past the epoch)
**/
U8_EXPORT time_t u8_localtime(struct U8_XTIME *xt,time_t tick);

/** Populates a U8_XTIME structure with a local time based on a
    POSIX time_t and a nanosecond count.
    This populates the broken down components based on the current
    timezone and initializes tzoff.
    @param xt a pointer to a U8_XTIME structure
    @param tick a POSIX time_t value (seconds past the epoch)
    @param nsecs an integer number of nanoseconds past the tick
**/
U8_EXPORT time_t u8_localtime_x(struct U8_XTIME *xt,time_t tick,int nsecs);

/** Populates a struct tm structure from an U8_XTIME representation
    @param xt a pointer to a U8_XTIME structure
    @param tm a pointer to a tm structure
    @returns the time_t value for the moment represented by both
      data structures
**/
U8_EXPORT time_t u8_xtime_to_tptr(struct U8_XTIME *xt,struct tm *tm);

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

/* Miscellanous Functions */

U8_EXPORT
/** Returns the number of milliseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting milliseconds
**/
long long u8_millitime(void);

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
