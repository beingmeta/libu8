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

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include <stdlib.h>
#include <string.h>

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

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
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

/* String/character typedefs */

typedef unsigned char u8_byte;
typedef const u8_byte *u8_string;
typedef int u8_unicode_char;
typedef int u8_unichar;

typedef char *u8_charstring;
typedef char *u8_chars;

/* This is for offsets into UTF-8 encoded strings, to distinguish
   byte and character offsets. */
typedef int u8_byteoff;
typedef int u8_charoff;

/* Type coercion, conditional, and constant string macros */

#define U8STR(x) ((u8_string)(x))
#define U8S0() ((u8_string)(""))
#define U8ALT(s,d) ((s)?((u8_string)(s)):((u8_string)(d)))
#define U8IF(s,d) ((s)?((u8_string)(d)):(U8_S0()))

#define U8OPTSTR(b,s,a) (U8IF(s,b)),(U8ALT(s,(U8_S0()))),(U8IF(s,a))

/* Errors */

typedef const unsigned char *u8_condition;
typedef const unsigned char *u8_context;

#include "u8exceptions.h"

/* Handy definition */

/* Write a long long to a string.  This writes the string without
   using printf or other 'heavyweight' functions. It can be used in
   resource-tight environments and can also be called in signal
   handlers as needed.
   @param val the signed integer value to write to the buffer
   @param buf a character buffer to place the representation
   @param buflen the size of the character buffer

 */
U8_EXPORT char *u8_write_long_long(long long l,char *buf,size_t buflen);

/* Threading compatability */

#include "threading.h"

/* Thread proxy functions */

/** Locks a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_lock(u8_mutex *m);

/** Unlocks a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_unlock(u8_mutex *m);

/** Initialize a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_init(u8_mutex *m);

/** Destroys a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_destroy(u8_mutex *m);

/* Thread init functions */

typedef int (*u8_threadinitfn)(void);
typedef void (*u8_threadexitfn)(void);
U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn);
U8_EXPORT int u8_register_threadexit(u8_threadexitfn fn);
U8_EXPORT int u8_run_threadinits(void);
U8_EXPORT int u8_threadexit(void);

U8_EXPORT int u8_n_threadinits;
U8_EXPORT int u8_n_threadexitfns;

#if (U8_USE__THREAD)
U8_EXPORT __thread int u8_initlevel;
#define u8_getinitlevel() (u8_initlevel)
#define u8_setinitlevel(n) u8_initlevel=(n);
#elif (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_initlevel_key;
#define u8_getinitlevel() ((int) ((u8_wideint)(u8_tld_get(u8_initlevel_key))))
#define u8_setinitlevel(n) u8_tld_set(u8_initlevel_key,((void *)n))
#else
U8_EXPORT int u8_initlevel;
#define u8_getinitlevel() (u8_initlevel)
#define u8_setinitlevel(n) u8_initlevel=(n);
#endif

#define u8_threadcheck() \
  if (u8_getinitlevel()<u8_n_threadinits) u8_run_threadinits()

/* UTF-8 String maniuplation */

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

/* Application/process identifying information */

/** Returns a long identifying the current thread
    @returns long a numeric thread identifier (OS dependent)
**/
U8_EXPORT long long u8_threadid(void);

/** Returns a string identifying the current process and thread
    @param buf a buffer to use (mallocd otherwise)
    @returns a character string
**/
U8_EXPORT char *u8_procinfo(char *buf);

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

/* UTF-8 handling config functions */

/** Gets/sets whether warnings are produced for invalid UTF-8 sequences, which
     can be annoyingly common with some content.
    @param flag an int
    @returns int
  If the flag is negative, the current value is returned
**/
U8_EXPORT int u8_config_utf8warn(int flag);

/** Gets/sets whether errors are produced for invalid UTF-8 sequences.
    @param flag an int
    @returns int
  If the flag is negative, the current value is returned
**/
U8_EXPORT int u8_config_utf8err(int flag);

/* Initialization functions for submodules */

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

/* Version/revision information */

U8_EXPORT u8_string u8_revision, u8_version;
U8_EXPORT int u8_major, u8_minor, u8_release;

U8_EXPORT
/** Returns the current libu8 revision string (derived from the GIT branch)
    @returns a utf8 string
**/
u8_string u8_getrevision(void);

U8_EXPORT
/** Returns the current libu8 version string (major.minor.release)
    @returns a utf8 string
**/
u8_string u8_getversion(void);

U8_EXPORT
/** Returns the current libu8 major version number
    @returns an int
**/
int u8_getmajorversion(void);

/* GETTEXT */

#if ((defined(HAVE_GETTEXT))&&(HAVE_GETTEXT))
#define u8_gettext(d,x) ((d==NULL) ? (gettext(x)) : (dgettext(d,x)))
#else
#define u8_gettext(d,x) (x)
#endif

#if (!((defined(HAVE_TEXTDOMAIN))&&(HAVE_TEXTDOMAIN)))
#ifndef textdomain
#define textdomain(domain)
#endif
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN))&&(HAVE_BINDTEXTDOMAIN)))
#ifndef bindtextdomain
#define bindtextdomain(domain,dir)
#endif
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN_CODESET))&&(HAVE_BINDTEXTDOMAIN_CODESET)))
#ifndef bindtextdomain_codeset
#define bindtextdomain_codeset(domain,dir)
#endif
#endif

#define _(x) (x)
#define N_(x) x

U8_EXPORT int _u8_dbg(u8_string s);

/* Malloc */

U8_EXPORT void *u8_dmalloc(size_t);

#ifndef U8_MALLOC
#define U8_MALLOC malloc
#endif

#define u8_zero_array(r) memset(r,0,sizeof(r))
#define u8_zero_struct(r) memset(&r,0,sizeof(r))

#define u8_malloc(sz) U8_MALLOC(sz)
#define u8_realloc(ptr,tosz) \
  ((ptr==NULL) ? (U8_MALLOC(tosz)) : (realloc(ptr,tosz)))
#define u8_free(ptr) free((char *)ptr)
#define u8_xfree(ptr) if (ptr) free((char *)ptr); else ptr=ptr;

#define u8_alloc(t) ((t *)(u8_malloc(sizeof(t))))
#define u8_alloc_n(n,t) ((t *)(u8_malloc(sizeof(t)*(n))))
#define u8_realloc_n(ptr,n,t) ((t *)(u8_realloc(ptr,sizeof(t)*(n))))

#define u8_malloc_struct(sname) \
  ((struct sname *)(U8_MALLOC(sizeof(struct sname))))
#define u8_malloc_array(n,t) ((t *)(U8_MALLOC(n*sizeof(t))))

/* Zalloc (allocate and fill with zeros) */

/* Declare here. Also declared below with docs. */
U8_EXPORT char *u8_write_long_long(long long,char *,size_t);
U8_EXPORT void u8_raise(u8_condition,u8_context,u8_string);

/** Allocates and zero-clears a block of memory.
    This raises the condition u8_MallocFailed (using u8_raise) if
     it fails. The caller argument 
   @param n_bytes the number of bytes to allocate
   @param caller a u8_context (static) string identifying 
      the caller for use if the allocation fails
   @returns void *
**/
static void *u8_zalloc_bytes(size_t n_bytes,u8_context caller)
{
  void *block = U8_MALLOC(n_bytes);
  if (block) {
    memset(block,0,n_bytes);
    return block;}
  else {
    char *buf=u8_malloc(32); 
    if (buf) {
      char *details = u8_write_long_long(n_bytes,buf,32);
      if (details)
	u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),buf);
      else u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),buf);}
    else u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),NULL);
    // Never reached
    return NULL;}
}
#define u8_zalloc(typename) u8_zalloc_bytes(sizeof(typename),NULL)

#define u8_zalloc(typename) u8_zalloc_bytes(sizeof(typename),NULL)
#define u8_zalloc_n(n,typename) u8_zalloc_bytes((n)*(sizeof(typename)),NULL)

#define u8_zalloc_for(caller,typename) \
  u8_zalloc_bytes(sizeof(typename),caller)
#define u8_zalloc_n_for(caller,n,typename) \
  u8_zalloc_bytes(((n)*(sizeof(typename))),caller)

#define u8_zalloc_struct(sname) \
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),NULL)))
#define u8_zalloc_struct_for(sname,caller) \
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),(caller))))

#define u8_zalloc_array(n,t)			\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),NULL)))
#define u8_zalloc_array_for(n,t,caller)		\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),caller)))

/** Allocates and zero-clears a block of memory
   @param sz the number of bytes to allocate
   @returns void *
**/
U8_EXPORT void *u8_mallocz(size_t sz);

/** Reallocates a block of memory, zero clearing any new parts
   @param ptr a previously allocated (with malloc) block of memory
   @param sz the number of bytes to allocate
   @param osz the previous number of bytes, needed to know how much
     to zero out
   @returns void *
**/
U8_EXPORT void *u8_reallocz(void *ptr,size_t sz,size_t osz);

/** Copies a block of memory into a larger block, zero clearing any new parts
   @param ptr an existing block of memory
   @param sz the number of bytes to allocate
   @param osz the number of bytes currently allocated at the location
   @returns void *
**/
U8_EXPORT void *u8_extalloc(void *ptr,size_t sz,size_t osz);

#define u8_allocz(t) ((t *)(u8_mallocz(sizeof(t))))
#define u8_allocz_n(n,t) ((t *)(u8_mallocz(sizeof(t)*(n))))

/* strdup */

#if HAVE_STRDUP
#define u8_strdup(x) (strdup(x))
#define u8_xstrdup(x) ((x==NULL)?(x):(strdup(x)))
#else
#define u8_strdup(x) _u8_strdup(x)
#define u8_xstrdup(x) ((x==NULL)?(x):(_u8_strdup(x)))
#endif

#define u8s(x) ((U8_EXPECT_FALSE(x==NULL))?(NULL):(u8_strdup(x)))

U8_EXPORT u8_string _u8_strdup(u8_string);
U8_EXPORT u8_string u8_strndup(u8_string,int);

#ifndef UTF8_BUGWINDOW
#define UTF8_BUGWINDOW 64
#endif

/* Piles */

typedef struct U8_PILE {
  void **u8_elts; size_t u8_len, u8_max; int u8_mallocd;} U8_PILE;
typedef struct U8_PILE *u8_pile;

U8_EXPORT int _u8_grow_pile(u8_pile p,int delta);

#define U8_INIT_PILE(p,len) \
  p->u8_len=0; p->u8_max=len; p->u8_mallocd=1; \
  p->u8_elts=u8_malloc(sizeof(void *)*len)
#define U8_INIT_STATIC_PILE(p,vec,len)          \
  p->u8_len=0; p->u8_max=len; p->u8_elts=(void **)vec

#define u8_pile_add(p,e) \
  if ((p)->u8_len>=(p)->u8_max) \
    {_u8_grow_pile((p),1); (p)->u8_elts[((p)->u8_len)++]=((void *)e);}  \
  else (p)->u8_elts[((p)->u8_len)++]=((void *)e)

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

/** Returns the system page size
    @returns long
**/
U8_EXPORT long u8_getpagesize(void);

/** Gets a variable specified in the environment.
    @param envvar a variable name
    @returns a utf-8 string, copied
**/
U8_EXPORT u8_string u8_getenv(u8_string envvar);

/** Returns elapsed time in seconds since some moment after application
     startup.
    @returns a double indicating seconds
**/
U8_EXPORT double u8_elapsed_time(void);

U8_EXPORT
/** Returns the number of microseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting microseconds
*/
long long u8_microtime(void);

/** Initializes logging
    @returns void
**/
U8_EXPORT void u8_initialize_logging(void);

/* Contours */

#include "u8contour.h"

/* Handling endian-ness */

U8_INLINE_FCN unsigned int u8_flip_word(unsigned int _w)
{ return ((((_w) << 24) & 0xff000000) | (((_w) << 8) & 0x00ff0000) |
          (((_w) >> 8) & 0x0000ff00) | ((_w) >>24) );}

U8_INLINE_FCN unsigned long long u8_flip_word8(unsigned long long _w)
{ return (((_w&(0xFF)) << 56) |
          ((_w&(0xFF00)) << 48) |
          ((_w&(0xFF0000)) << 24) |
          ((_w&(0xFF000000)) << 8) |
          ((_w>>56) & 0xFF) |
          ((_w>>40) & 0xFF00) |
          ((_w>>24) & 0xFF0000) |
          ((_w>>8) & 0xFF000000));}

U8_INLINE_FCN unsigned int u8_flip_ushort(unsigned short _w)
{ return ((((_w) >> 8) & 0x0000ff) | (((_w) << 8) & 0x0000ff00) );}

#if WORDS_BIGENDIAN
#define u8_net_order(x) (x)
#define u8_host_order(x) (x)
#define u8_ushort_net_order(x) (x)
#define u8_ushort_host_order(x) (x)
#else
#define u8_net_order(x) fd_flip_word(x)
#define u8_host_order(x) fd_flip_word(x)
#define u8_ushort_host_order(x) fd_flip_ushort(x)
#define u8_ushort_net_order(x) fd_flip_ushort(x)
#endif

/* Declare cityhash prototypes */

U8_EXPORT u8_int16 u8_cityhash128(const unsigned char *s,size_t len);
U8_EXPORT u8_int8 u8_cityhash64(const unsigned char *s,size_t len);

/* File and module recording */

typedef struct U8_SOURCE_FILE_RECORD {
  u8_string filename;
  struct U8_SOURCE_FILE_RECORD *next;} U8_SOURCE_FILE_RECORD;
typedef struct U8_SOURCE_FILE_RECORD *u8_source_file_record;

U8_EXPORT void u8_register_source_file(u8_string s);
U8_EXPORT void u8_for_source_files(void (*f)(u8_string s,void *),void *data);

#endif /* ndef LIBU8_LIBU8_H */
