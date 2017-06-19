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

/** \file u8rusage.h
    Provides u8_getrusage.
    This is a version of the Unix getrusage() call which
     provides more portable functionality.  In particular, it
     will return non-zero memory numbers for Linux by consulting the
     /proc file system.
 **/

#ifndef LIBU8_RUSAGE_H
#define LIBU8_RUSAGE_H 1
#define LIBU8_RUSAGE_H_VERSION __FILE__

#if (HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#elif (HAVE_RESOURCE_H)
#include <resource.h>
#endif

/** Returns the system page size
    @returns long
**/
U8_EXPORT long u8_getpagesize(void);

#if HAVE_GETRUSAGE

/** Returns resource usage information.
    This patches implementation-specific in getting usage
     information, particularly the reticence of Linux to put
     any numbers in the rusage memory information fields.
    @param who either RUSAGE_SELF or RUSAGE_CHILDREN
    @param r a pointer to an rusage struct.
    @returns 0 on success, -1 on error
**/
U8_EXPORT int u8_getrusage(int who,struct rusage *r);

/** Returns a string summarizing resource usage
    @param r a pointer to an rusage struct.
    @returns a malloc'd string
**/
U8_EXPORT u8_string u8_rusage_string(struct rusage *r);

#endif /* HAVE_GETRUSAGE */

/** Returns the resident size of the current process
    @returns resident size in bytes, as an ssize_t
**/
U8_EXPORT ssize_t u8_memusage(void);

/** Returns the virtual memory size of the current process
    @returns virtual memory size in bytes, as an ssize_t
**/
U8_EXPORT ssize_t u8_vmemusage(void);

/** Returns the total physical memory of the system
    @returns the memory size in bytes, as an ssize_t
**/
U8_EXPORT ssize_t u8_physmem(void);

/** Returns the total available physical memory
    @returns the memory size in bytes, as an ssize_t
**/
U8_EXPORT ssize_t u8_avphysmem(void);

/** Returns the fraction of physical memory used by the resident set for
    the current process.
    @returns the ratio (a double)
**/
U8_EXPORT double u8_memload(void);

/** Returns the fraction of physical memory used by the resident set for
    the current process.
    @returns the ratio (a double)
**/
U8_EXPORT double u8_vmemload(void);

#endif
