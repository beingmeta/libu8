/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2012 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_LIBU8_H
#define LIBU8_LIBU8_H 1
#define LIBU8_LIBU8_H_VERSION \
        "$Id$"

/** \file libu8.h
    These functions provide miscellaneous functionality.
 **/

#include "libu8/revision.h"

#include "libu8/config.h"

#if U8_LARGEFILES_ENABLED
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#endif

#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef U8_WITH_STDIO
#define U8_WITH_STDIO 1
#endif

#if MINGW
#define WIN32 1
#endif

#if WIN32
#include <windows.h>
#define random rand
#define srandom srand
#define sleep(x) Sleep(x*1000)
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
#define U8_INLINE __attribute__ ((inline))
#define U8_NOINLINE __attribute__ ((noinline))
#define MAYBE_UNUSED __attribute__ ((unused))
#define U8_INLINE_FCN static __attribute__ ((unused))
#else
#define U8_INLINE
#define U8_NOINLINE
#define MAYBE_UNUSED
#define U8_INLINE_FCN static
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
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
typedef long u8_connection;

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

U8_EXPORT u8_int16 u8_cityhash128(const unsigned char *s,size_t len);
U8_EXPORT u8_int8 u8_cityhash64(const unsigned char *s,size_t len);

/* Load threading compatability libraries */
#include "threading.h"

/* Thread init functions */

typedef int (*u8_threadinitfn)(void);
typedef void (*u8_threadexitfn)(void);
U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn);
U8_EXPORT int u8_register_threadexit(u8_threadexitfn fn);
U8_EXPORT int u8_run_threadinits(void);
U8_EXPORT int u8_threadexit(void);

U8_EXPORT int u8_n_threadinits;
U8_EXPORT int u8_n_threadexitfns;

#if ((U8_THREADS_ENABLED) && (HAVE_THREAD_STORAGE_CLASS) && (!(U8_FORCE_TLS)))
  U8_EXPORT __thread int u8_initlevel;
  #define u8_getinitlevel() (u8_initlevel)
  #define u8_setinitlevel(n) u8_initlevel=(n);
#else
  U8_EXPORT u8_tld_key u8_initlevel_key;
  #define u8_getinitlevel() ((int) ((u8_wideint)(u8_tld_get(u8_initlevel_key))))
  #define u8_setinitlevel(n) u8_tld_set(u8_initlevel_key,((void *)n))
#endif

#define u8_threadcheck() \
  if (u8_getinitlevel()<u8_n_threadinits) u8_run_threadinits()

/* UTF-8 String maniuplation */

typedef unsigned char u8_byte;
typedef u8_byte *u8_string;
typedef int u8_unicode_char;
typedef int u8_unichar;

typedef char *u8_charstring;
typedef char *u8_chars;

/* This is for offsets into UTF-8 encoded strings, to distinguish
   byte and character offsets. */
typedef int u8_byteoff;
typedef int u8_charoff;

/** Converts a UTF-8 string to the encoding expected by system (libc) functions.
    If the system encoding is UTF-8, this returns its argument, rather than
    copying it, as u8_tolibc does.
    @param string a utf-8 string
    @returns a character string in the local encoding
**/
U8_EXPORT char *u8_2libc(u8_string string);
/** Converts a UTF-8 string to the encoding expected by system (libc) functions.
    If the system encoding is UTF-8, this validates and copies the argument.
    @param string a utf-8 string
    @returns a character string in the local encoding
**/
U8_EXPORT char *u8_tolibc(u8_string string);
/** Converts a natively encoded string into a UTF-8 string.
    If the system encoding is UTF-8, this validates and copies the argument.
    @param local_string a locally encoded text string
    @returns a utf-8 encoded string
**/
U8_EXPORT u8_string u8_fromlibc(char *local_string);
/** This sets the functions used to map to and from libc.
   @param fromfn a function from character strings to utf-8 strings
   @param tofn a function from utf-8 strings to character strings.
   @returns void */
U8_EXPORT void u8_set_libcfns
   (u8_string (*fromfn)(char *),char *(*tofn)(u8_string));

/* General initialization, etc. */

/** Returns a UTF-8 string describing the current application for inclusion
     in messages or other output or logging.
    This string is not consed, so it should not be freed.
    @returns a utf-8 string
**/
U8_EXPORT u8_string u8_appid(void);
/** Sets the UTF-8 string describing the current application.
    @param id a utf-8 string
    @returns void
**/
U8_EXPORT void u8_identify_application(u8_string id);
/** Sets the UTF-8 string describing the current application, unless
     it has already been set.  This can be used to provide default appids
     while allowing programs to override the default.
    This returns 1 if it did anything (no appid had been previously set) or
      zero otherwise.
    @param id a utf-8 string
    @returns int
**/
U8_EXPORT int u8_default_appid(u8_string id);

/** Sets whether warnings are produced for invalid UTF-8 sequences, which
     can be annoyingly common with some content.
    @param flag an int
    @returns int
**/
U8_EXPORT int u8_config_utf8warn(int flag);

/** Initializes the core UTF-8 functions (from libu8.h)
    @returns void
**/
U8_EXPORT int u8_initialize(void) U8_LIBINIT_FN;
/** Initializes UTF-8 io functions, including character set
     conversion and stream I/O.
    @returns void
**/
U8_EXPORT int u8_initialize_io(void) U8_LIBINIT_FN;
/** Initializes UTF-8 miscellaneous functions, including time keeping,
     file access, network access, and server functions.
    @returns void
**/
U8_EXPORT int u8_initialize_fns(void) U8_LIBINIT_FN;

/** Initializes the full character data table for the libu8 library.
    @returns void
**/
U8_EXPORT void u8_init_chardata_c(void) U8_LIBINIT_FN;

/** Initializes the messaging functions which use POSIX stdio. */
U8_EXPORT void u8_initialize_u8stdio(void) U8_LIBINIT_FN;
/** Initializes the messaging functions which use POSIX syslog. */
U8_EXPORT void u8_initialize_u8syslog(void) U8_LIBINIT_FN;

/* The current subversion revision */
U8_EXPORT u8_string u8_svnrev;

/* GETTEXT */

#if ((defined(HAVE_GETTEXT))&&(HAVE_GETTEXT))
#define u8_gettext(d,x) ((d==NULL) ? (gettext(x)) : (dgettext(d,x)))
#else
#define u8_gettext(d,x) (x)
#endif

#if (!((defined(HAVE_TEXTDOMAIN))&&(HAVE_TEXTDOMAIN)))
#define textdomain(domain)
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN))&&(HAVE_BINDTEXTDOMAIN)))
#define bindtextdomain(domain,dir)
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN_CODESET))&&(HAVE_BINDTEXTDOMAIN_CODESET)))
#define bindtextdomain_codeset(domain,dir)
#endif

#define _(x) (x)
#define N_(x) x

/* Malloc */

#define u8_malloc(sz) malloc(sz)
#define u8_realloc(ptr,tosz) \
  ((ptr==NULL) ? (malloc(tosz)) : (realloc(ptr,tosz)))
#define u8_free(ptr) free(ptr)

#define u8_alloc(t) ((t *)(u8_malloc(sizeof(t))))
#define u8_alloc_n(n,t) ((t *)(u8_malloc(sizeof(t)*(n))))
#define u8_realloc_n(ptr,n,t) ((t *)(u8_realloc(ptr,sizeof(t)*(n))))

#define u8_malloc_struct(sname) ((struct sname *)(malloc(sizeof(struct sname))))
#define u8_malloc_array(n,t) ((t *)(malloc(n*sizeof(t))))

/** Allocates and zero-clears a block of memory
   @param sz the number of bytes to allocate
   @returns void *
**/
U8_EXPORT void *u8_mallocz(size_t sz);

/** Reallocates a block of memory, zero clearing any new parts
   @param sz the number of bytes to allocate
   @returns void *
**/
U8_EXPORT void *u8_reallocz(void *ptr,size_t sz,size_t osz);

/** Copies a block of memory into a larger block, zero clearing any new parts
   @param sz the number of bytes to allocate
   @returns void *
**/
U8_EXPORT void *u8_extalloc(void *ptr,size_t n,size_t osz);

#define u8_allocz(t) ((t *)(u8_mallocz(sizeof(t))))
#define u8_allocz_n(n,t) ((t *)(u8_mallocz(sizeof(t)*(n))))

/* strdup */

#if HAVE_STRDUP
#define u8_strdup(x) ((u8_string)(strdup(x)))
#else
#define u8_strdup(x) _u8_strdup(x)
#endif

U8_EXPORT u8_string _u8_strdup(u8_string);
U8_EXPORT u8_string u8_strndup(u8_string,int);

/* Errors */

typedef const unsigned char *u8_condition;
typedef const unsigned char *u8_context;

#include "u8exceptions.h"

/* Messages */

#include "u8logging.h"

/* Raising errors */

/** Signals an error with particular details.
    This currently exits, though it may be expanded to an exception
     throwing architecture in the near future.
   @param ex a utf-8 condition string (u8_condition)
   @param cxt a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @returns void (shouldn't)
**/
U8_EXPORT void u8_raise(u8_condition ex,u8_context cxt,u8_string details);

/** Sets the function used when an error is raised.
   @param h is a function on a condition, a context, and a utf-8 string
   @returns void (shouldn't)
**/
U8_EXPORT void u8_set_error_handler
  (void (*h)(u8_condition,u8_context,u8_string));

/* Miscellaneous */

/** Returns a random integer between 0 and n-1 (inclusive)
    @param n the upper limit for the random number
    @returns an int between 0 and n-1
**/ 
U8_EXPORT unsigned int u8_random(unsigned int n);
/** Sets the random seed value used for the random number generation.
     Setting the random seed value to the same value will cause the sequence
     of numbers generated by u8_random to be the same.
    @param seed the seed value
    @returns void
**/ 
U8_EXPORT void u8_randomize(unsigned int seed);

/** Dynamically loads a named file into the running image.
    @param filename a utf-8 pathname
    @returns void
**/ 
U8_EXPORT void *u8_dynamic_load(u8_string filename);

/** Gets a variable specified in the environment.
    @param varname a variable name
    @returns a utf-8 string, copied
**/ 
U8_EXPORT u8_string u8_getenv(u8_string varname);

/** Returns elapsed time in seconds since some moment after application
     startup.
    @returns a double indicating seconds
**/
U8_EXPORT double u8_elapsed_time(void);

/** Initializes syslog
    @returns void
**/ 
U8_EXPORT void u8_init_syslog(void);

/* Contours */

#include "u8contour.h"

#endif /* ndef LIBU8_LIBU8_H */


