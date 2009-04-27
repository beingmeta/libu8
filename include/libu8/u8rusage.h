/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2009 beingmeta, inc.
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
#define LIBU8_RUSAGE_H_VERSION \
        "$Id: u8rusage.h 3635 2009-04-22 02:45:30Z haase $"

#if (!(HAVE_GETRUSAGE))
struct rusage { int noval;};
#elif (HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#elif (HAVE_RESOURCE_H)
#include <resource.h>
#endif

/** Returns resource usage information.
    This patches implementation-specific in getting usage
     information, particularly the reticence of Linux to put
     any numbers in the rusage memory information fields.
    @param who either RUSAGE_SELF or RUSAGE_CHILDREN
    @param r a pointer to an rusage struct.
    @returns 0 on success, -1 on error
**/
U8_EXPORT int u8_getrusage(int who,struct rusage *r);

/** Returns the virtual memory size of the current process
    @returns virtual memory size in bytes, as an unsigned long
**/
U8_EXPORT unsigned long u8_memusage(void);

#endif
