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

/*! \mainpage Welcome to libu8
 *
 * **libu8** is a portability and utility library written in modern C
 * for Unix-based platforms. *libu8* is licensed under the
 * [LGPL](https://www.gnu.org/licenses/lgpl.html) and the GPL [version
 * 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) or
 * [later](https://www.gnu.org/licenses/gpl.html).  You can find the
 * source to libu8 on [github](https://github.com/beingmeta/libu8).
 *
 * **libu8** is especially intended for software which uses UTF-8
 *  internally but may interact with applications and services employing
 * different character encodings. Over time, it has grown to provide a
 * variety of other portable functions and wrappers.
 *
 * \section intro_sec Features:
 *
 * string utilities for portably working with UTF-8 encodings
 *
 * stream-based text I/O using UTF-8 internally but allowing
 * output to multiple encodings
 *
 * a client networking library for socket io, connection pools, and
 * simple hostname lookup;
 *
 * extensible and customizable logging functions (using `u8_printf`);
 *
 * wrappers for time-related functions allowing fine-grained times 
 *  and specification of arbitrary time zones;
 *
 * an exception handling library using `setjmp`/`longjmp` with
 *  unwinds and dynamic error catching;
 *
 * signal handlers for turning synchronous signals into exceptions;
 *
 * a server networking library for lightweight multi-threaded server
 *  implementation.
 *
 * an extensible printf (`u8_printf`)  function including output to strings;
 *
 * various hash and digest functions, including MD5, Google's cityhash,
 *  and various SHAx functions;
 *
 * cryptographic function wrappers using local libraries;
 *
 * a variety of file or URI path manipulation functions;
 *
 * a wrapper for rusage() resource system calls;
 *
 * wrappers for accessing file and directory contents and metadata;
 *
 * support for lookup up and interpreting named character entities;
 *
 */

#ifndef LIBU8_LIBU8_H
#define LIBU8_LIBU8_H 1
#define LIBU8_LIBU8_H_VERSION __FILE__

/** \file libu8.h
    These functions provide miscellaneous functionality.
 **/

#include "libu8/revision.h"
#include "libu8/config.h"

#include "u8defines.h"

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "u8strings.h"

#include "u8libc.h"

#include "u8gettext.h"

#include "u8threading.h"

#include "u8exceptions.h"

#include "u8appinfo.h"

#include "u8malloc.h"

#include "u8random.h"

#include "u8digest.h"

#include "u8piles.h"

#include "u8logging.h"

#include "u8error.h"

#include "u8contour.h"

#include "u8sourcefiles.h"

U8_EXPORT
/** Initializes the core UTF-8 functions (from libu8.h)
    @returns void
**/
int u8_initialize(void) U8_LIBINIT_FN;

U8_EXPORT
/** Initializes UTF-8 io functions, including character set
     conversion and stream I/O.
    @returns void
**/
int u8_initialize_io(void) U8_LIBINIT_FN;

U8_EXPORT
/** Initializes UTF-8 miscellaneous functions, including time keeping,
     file access, network access, and server functions.
    @returns void
**/
int u8_initialize_fns(void) U8_LIBINIT_FN;

U8_EXPORT
/** Initializes the full character data table for the libu8 library.
    @returns void
**/
void u8_init_chardata_c(void) U8_LIBINIT_FN;

U8_EXPORT
/** Initializes the messaging functions which use POSIX stdio. */
void u8_initialize_u8stdio(void) U8_LIBINIT_FN;

U8_EXPORT
/** Initializes the messaging functions which use POSIX syslog. */
void u8_initialize_u8syslog(void) U8_LIBINIT_FN;

/* Miscellaneous */

/** Dynamically loads a named file into the running image.
    @param filename a utf-8 pathname
    @returns void
**/
U8_EXPORT void *u8_dynamic_load(u8_string filename);

/** Looks up a symbol (string) in a dynamic module loaded with 
    u8_dynamic_load (or search for the symbol)
    @param symname a symbol name as a utf-8 string 
    @param module a void * pointer returned by u8_dynamic_load (or NULL)
    @returns void
**/
U8_EXPORT void *u8_dynamic_symbol(u8_string symname,void *module);

#if (SIZEOF_VOID_P == 8)
#define FD_PTRHASH_CONSTANT 11400714819323198549ul
#else
#define FD_PTRHASH_CONSTANT 2654435761
#endif

U8_INLINE U8_MAYBE_UNUSED
/* Computes a hash value from a pointer
   @param ptr    a memory pointer
   @param n_bits a bit width for the result (<32)
 */
u8_int4 u8_hashptrval(void *ptr,int n_bits) 
{
  u8_wideint intrep = (u8_wideint) ptr;
  return (intrep * FD_PTRHASH_CONSTANT) >> (64 - n_bits);
}

/** Initializes logging
    @returns void
**/
U8_EXPORT void u8_init_logger(void);

U8_EXPORT int u8_logger_initialized, u8_using_syslog;
#define U8_INIT_LOGGER() if (u8_logger_initialized>0) u8_init_logger();

U8_EXPORT time_t u8_start_tick;

#endif /* ndef LIBU8_LIBU8_H */
