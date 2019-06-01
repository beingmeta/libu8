/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8XTIME_H
#define LIBU8_U8XTIME_H 1
#define LIBU8_U8XTIME_H_VERSION __FILE__

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
    @param tm a pointer to a tm structure, or NULL
    @returns a filled out struct tm pointer, either *tm* or a newly allocated
      pointer if *tm* is NULL
**/
U8_EXPORT struct tm *u8_xtime_to_tptr(struct U8_XTIME *xt,struct tm *tm);

/** Tries to return a timezone string for a combination of a timezone
    offset and a DST offset.
    @param tzoff time-zone offset in seconds
    @param dstoff summer/daylight savings time offset in seconds
    @returns a time zone string NULL
**/
U8_EXPORT u8_string u8_get_tm_zone(int tzoff,int dstoff);

U8_EXPORT struct U8_XTIME u8_start_time;

#endif
