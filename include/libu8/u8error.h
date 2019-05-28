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

#ifndef LIBU8_U8ERROR_H
#define LIBU8_U8ERROR_H 1
#define LIBU8_U8ERROR_H_VERSION __FILE__

#define U8_DEBUG_ERRNO 1

U8_EXPORT int u8_debug_errno;
U8_EXPORT u8_condition u8_MissingErrno;

#define U8_CLEAR_ERRNO() if (errno) errno=0; else {}
#define U8_DISCARD_ERRNO(num) if (errno==num) errno=0; else {}

#if U8_DEBUG_ERRNO

#define U8_CHECK_ERRNO()					\
  if (u8_debug_errno) {						\
    u8_log(LOG_WARN,u8_UnexpectedErrno,				\
	   "At %s:%d (%s) errno=%d (%s)",			\
	   __FILE__,__LINE__,__FUNCTION__,			\
           errno,u8_strerror(errno));				\
    if (u8_debug_errno > 1) _u8_dbg(u8_strerror(errno));	\
    errno=0;}							\
  else errno=0;
#else
#define U8_CHECK_ERRNO() if (errno) errno=0
#endif

U8_EXPORT
/* Provides a useful function to breakpoint
   @param s an identifying message
   @returns 0 (unless overidden by the debugger)
*/
int _u8_dbg(u8_string s);

/* Raising errors */

/** Signals an error with particular details.
    This currently exits, though it may be expanded to an exception
     throwing architecture in the near future.
   @param cond a utf-8 condition string (u8_condition)
   @param cxt a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @returns void (shouldn't)
**/
U8_EXPORT void u8_raise(u8_condition cond,u8_context cxt,u8_string details);

/** Signals an error described by an existing exception object.
   @param ex a pointer to a u8_exception object
   @returns void (shouldn't)
**/
U8_EXPORT void u8_raise_exception(u8_exception ex);

/** Sets the function used when an error is raised.
   @param h is a function on a condition, a context, and a utf-8 string
   @returns void (shouldn't)
**/
U8_EXPORT void u8_set_error_handler
  (void (*h)(u8_condition,u8_context,u8_string));

#endif
