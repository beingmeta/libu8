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

#ifndef LIBU8_U8DEFINES_H
#define LIBU8_U8DEFINES_H 1
#define LIBU8_U8DEFINES_H_VERSION __FILE__

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#if U8_LARGEFILES_ENABLED
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#endif

#ifndef U8_WITH_STDIO
#define U8_WITH_STDIO 1
#endif

#if LIBU8_SOURCE
#pragma GCC diagnostic ignored "-Wpointer-sign"
#endif

#ifndef U8_DLL
#define U8_DLL 0
#endif

#if WIN32
#if U8_DLL
#define U8_EXPORT __declspec(dllexport)
#else
#define U8_EXPORT __declspec(dllimport)
#endif
#else
#define U8_EXPORT extern
#endif

#if __GNUC__
#define U8_INLINE static inline
#define U8_INLINE_FCN static inline
#define U8_NOINLINE __attribute__ ((noinline))
#define U8_MAYBE_UNUSED __attribute__ ((unused))
#define U8_DEPRECATED __attribute__ ((deprecated))
#define U8_MAYBE_UNUSED __attribute__ ((unused))
#define U8_MALLOCFN __attribute__ ((malloc))
#else
#define U8_INLINE static
#define U8_INLINE_FCN static
#define U8_NOINLINE
#define MAYBE_UNUSED
#define U8_MAYBE_UNUSED
#define U8_DEPRECATE
#define U8_MALLOCFN
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif

#if (!(U8_THREADS_ENABLED))
#define U8_USE_TLS 0
#define U8_USE__THREAD 0
#elif ((U8_FORCE_TLS) || (!(HAVE_THREAD_STORAGE_CLASS)))
#define U8_USE_TLS 1
#define U8_USE__THREAD 0
#elif (HAVE_THREAD_STORAGE_CLASS)
#define U8_USE_TLS 0
#define U8_USE__THREAD 1
#else
#define U8_USE_TLS 1
#define U8_USE_TLS 0
#endif

/* Checking constructor attributes */

#if HAVE_CONSTRUCTOR_ATTRIBUTES
#define U8_LIBINIT_FN __attribute__ ((constructor))
#define U8_DO_LIBINIT(fn) fn
#else
#define U8_LIBINIT_FN
#define U8_DO_LIBINIT(fn) fn()
#endif

#ifndef HAVE_LSEEKO
#define lseeko lseek
#endif

#ifndef DESTRUCTIVE_POLL
#define DESTRUCTIVE_POLL 0
#endif

#ifndef U8_THREADS_ENABLED
#define U8_THREADS_ENABLED 0
#endif

#ifndef HAVE_CONSTRUCTOR_ATTRIBUTES
#define HAVE_CONSTRUCTOR_ATTRIBUTES 1
#endif

#ifndef HAVE_BUILTIN_EXPECT
#define HAVE_BUILTIN_EXPECT 0
#endif

#ifndef HAVE_THREAD_STORAGE_CLASS
#define HAVE_THREAD_STORAGE_CLASS 0
#endif

#if HAVE_BUILTIN_EXPECT
#define U8_EXPECT_TRUE(x) (__builtin_expect(x,1))
#define U8_EXPECT_FALSE(x) (__builtin_expect(x,0))
#else
#define U8_EXPECT_TRUE(x) (x)
#define U8_EXPECT_FALSE(x) (x)
#endif

#if HAVE_EVP_CIPHER_CTX_INIT
#define U8_HAVE_CRYPTO 1
#else
#define U8_HAVE_CRYPTO 0
#endif

/* This is for sockets */
typedef long u8_socket;

/* This is for microseconds since the epoch. */
typedef long long u8_utime;

#if (SIZEOF_INT == 4)
typedef unsigned int u8_int4;
#elif (SIZEOF_LONG == 4)
typedef unsigned long u8_int4;
#elif (SIZEOF_SHORT == 4)
typedef unsigned short u8_int4;
#elif (SIZEOF_LONG_LONG == 4)
typedef unsigned long long u8_int4;
#else
typedef unsigned int u8_int4;
#endif

#if (SIZEOF_INT == 2)
typedef unsigned int u8_int2;
#elif (SIZEOF_LONG == 2)
typedef unsigned long u8_int2;
#elif (SIZEOF_SHORT == 2)
typedef unsigned short u8_int2;
#else
typedef unsigned short u8_int2;
#endif

#if (SIZEOF_INT == 8)
typedef unsigned int u8_int8;
#elif (SIZEOF_LONG == 8)
typedef unsigned long u8_int8;
#elif (SIZEOF_LONG_LONG == 8)
typedef unsigned long long u8_int8;
#elif (SIZEOF_SHORT == 8)
typedef unsigned short u8_int8;
#else
typedef unsigned long long u8_int8;
#endif

#if (SIZEOF_LONG_LONG == 16)
typedef unsigned long long u8_int16;
#else
typedef struct U8_INT16 {
  u8_int8 first; u8_int8 second;} u8_int16;
#endif

/* Wide ints are the smallest ints as big as a pointer */
#if (SIZEOF_LONG_LONG == SIZEOF_VOID_P)
typedef unsigned long long u8_wideint;
#elif (SIZEOF_LONG == SIZEOF_VOID_P)
typedef unsigned long u8_wideint;
#else
typedef unsigned int u8_wideint;
#endif

#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

#define U8_SETBITS(loc,bits)   (loc) |= bits
#define U8_CLEARBITS(loc,bits) (loc) &= (~(bits))
#define U8_CHECKBITS(loc,bits) ( (loc) & (bits) )
#define U8_BITP(loc,bits)      ( (loc) & (bits) )

#endif
