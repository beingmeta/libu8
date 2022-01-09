/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_PRINTF_H
#define LIBU8_PRINTF_H 1
#define LIBU8_PRINTF_H_VERSION __FILE__

/** \file u8printf.h
    This is a customizable formatted printer (based on printf) that
    works with libu8 streams (and wrapped for standard output).

    The function u8_fprintf, provided by <libu8/u8stdio.h>, is a version
    of u8_printf which outputs to a FILE * pointer from the POSIX STDIO library.
**/

#include <stdarg.h>

U8_EXPORT int u8_printf_underscore_ints;

/* PRINTF */

/** Outputs formatted data to a libu8 stream.
    @param stream a pointer to a U8_OUTPUT stream.
    @param fmtstring a format control string
    @param ... other arguments, interpreted by @a fmtstring
    @returns the number of directives which were processed
    This provides all of the standard printf formatting/conversion
    operators (%d, %s, %f, %%, etc) and generally uses sprintf to
    generate the output string, so most of the optional flags work
    as well.  In addition to any new operators defined by external
    modules, libu8 provides two special operators %m and %t for
    internationalization and date/time display respectively.
    + %m calls gettext on its argument to return a translated version
    appropriate to the current locale;
    + %t generates a string describing a particular date/time.	With
    the operator '*', it uses the current date/time, with the option
    'x' it takes a pointer to a @a U8_XTIME structure, and with neither
    option it takes a @a time_t value.	The option 'G' forces the conversion
    to UTC/GMT time; otherwise either the current time zone or the time zone
    explicitly specified in the @a U8_XTIME structure is used.
    + %s takes a special modifier '0' which indicates that its string
    argument should be freed (with @a u8_free()) after use; this allows
    greater conciseness in output expressions.
**/
U8_EXPORT int u8_printf(u8_output stream,u8_string fmtstring,...);

/** Outputs formatted data to a libu8 stream.
    @param fmtstring a format control string
    @param ... other arguments, interpreted by @a fmtstring
    @returns a UTF-8 string
    This generates a freshly malloc'd UTF-8 string based on converting
    its arguments using @a fmtstring.
**/
U8_EXPORT u8_string u8_mkstring(u8_string fmtstring,...);

/** Outputs formatted data to a pre-allocated string buffer
    @param buf a string buffer
    @param buflen the length of the buffer (a size_t)
    @param fmtstring a format control string
    @param ... other arguments, interpreted by @a fmtstring
    @returns a UTF-8 string
    This calls printf with output going to a pre-allocated buffer.
    Output beyond the end of the buffer (defined by the `buflen`
    argument) is discarded.
**/
U8_EXPORT u8_string u8_sprintf
(unsigned char *buf,size_t buflen,u8_string fmtstring,...);

/** Outputs formatted data to a pre-allocated string buffer
    @param buf a string buffer
    @param fmtstring a format control string
    @param ... other arguments, interpreted by @a fmtstring
    @returns a UTF-8 string
    This calls printf with output going to a pre-allocated buffer.
    This must be able to determine the size of the buffer.
    Output beyond the end of the buffer is discarded.
**/
#define u8_bprintf(buf,fmtstring,...)			\
  u8_sprintf(buf,sizeof(buf),fmtstring, ##__VA_ARGS__)

/** Outputs formatted data to a libu8 stream, reading inputs
    from a va_list structure.
    @param stream a pointer to a U8_OUTPUT stream
    @param fmtstring a format control string
    @param args a pointer to a <stdarg.h> va_list structure
    @returns the number of directives processed
    This is a version of u8_printf designed for stdarg functions
    which wrap the u8_print functionality.
**/
U8_EXPORT int u8_do_printf(u8_output stream,u8_string fmtstring,va_list *args);

typedef u8_string (*u8_printf_handler)
(u8_output,char *,u8_byte *,int,va_list *args);
U8_EXPORT u8_printf_handler u8_printf_handlers[128];

U8_EXPORT int u8_errout(u8_output,struct U8_EXCEPTION *);

/* Textdomain functions */

#if (HAVE_GETTEXT)
#include <libintl.h>
typedef u8_string (*u8_xlatefn)(u8_string);
U8_EXPORT void u8_register_xlatefn(u8_xlatefn);
U8_EXPORT void u8_register_textdomain(char *domain);
U8_EXPORT u8_string u8_getmessage(u8_string msg);
#else
#define u8_register_textdomain(x)
#define u8_getmessage(x) (x)
#endif

#endif /* LIBU8_PRINTF_H */
