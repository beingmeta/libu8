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

/** \file u8fileio.h
    These functions provide wrappers for the STDIO and open() calls.
    In addition to doing conversion of pathnames to the local file system,
     they attempt to provide accessible portable locking facilities.
 **/

#ifndef LIBU8_FILEIO_H
#define LIBU8_FILEIO_H 1
#define LIBU8_FILEIO_H_VERSION __FILE__

#include <stdio.h>
#include <time.h>

/** Opens a file using STDIO while ensuring unique access.
    This uses fcntl() locking if available.  This also converts
     its utf-8 argument to the local character set.
    @param path a utf-8 string
    @param mode a file mode, as for fopen (but simpler)
    @returns FILE * pointer to a stdio file struct
 **/
U8_EXPORT FILE *u8_fopen_locked(u8_string path,char *mode);

/** Opens a file using STDIO.
    This also converts
     its utf-8 argument to the local character set.
    @param path a utf-8 string
    @param mode a file mode, as for fopen (but simpler)
    @returns FILE * pointer to a stdio file struct
 **/
U8_EXPORT FILE *u8_fopen(u8_string path,char *mode);

/** Closes a STDIO file.
    This unlocks the underlying file descriptor if neccessary.
    @param f a FILE * pointer to a STDIO file struct
 **/
U8_EXPORT int u8_fclose(FILE *f);

/** Locks a particular file descriptor.
    This generally uses fcntl locking.
    @param fd an open file descriptor
    @param for_writing whether to lock the file for writing
    @return the file descriptor or -1 on error.
**/
U8_EXPORT int u8_lock_fd(int fd,int for_writing);

/** Unlocks a particular file descriptor.
    This generally uses fcntl locking.
    @param fd an open file descriptor
    @return 1 if successful or -1 on error.
**/
U8_EXPORT int u8_unlock_fd(int fd);

/** Opens a file descriptor.
    This converts the utf-8 pathname to the local encoding.
    @param path a utf-8 pathname string
    @param flags passed to open()
    @param mode passed to open()
    @returns a new file descriptor (int) or -1 on error
 **/
U8_EXPORT int u8_open_fd(u8_string path,int flags,mode_t mode);

/** Closes a file descriptor.
    @param fd a valid file descriptor
    @returns -1 on error
 **/
U8_EXPORT int u8_close_fd(int fd);

/* Subscribing to files */

/** The subscription structure maps filenames to ifchanged functions. **/
typedef struct U8_SUBSCRIPTION {
  char *filename; /* the locally encoded filename subscribed to. */
  time_t mtime; /* the modification of the time when last processes */
  /* function to call when the subscription needs to be renewed. */
  int (*callback)(u8_string,void *);
 /* the subscription data passed to the callback function */
  void *callback_data;
  /* the next node in the subscription table */
  struct U8_SUBSCRIPTION *next;}
  U8_SUBSCRIPTION;
/** A pointer to a subscription structure. **/
typedef struct U8_SUBSCRIPTION *u8_subscription;

/** Arranges for an action when a file changes.
    Creates a *subscription* which will call fn(filename,data) both
    initially and whenever it notices that filename has changed.
    The function u8_renew checks a particular subscription and the function
    u8_renew_all checks all subscriptions.
    @param filename a utf-8 pathname
    @param fn a function on a string and a pointer
    @param data data passed as a second argument to fn
    @returns a pointer to a U8_SUBSCRIPTION struct
**/
U8_EXPORT u8_subscription u8_subscribe
  (u8_string filename,int (*fn)(u8_string,void *),void *data);

/** Calls a function on a file if it has changed.
    Renews a subscription created by u8_subscribe, calling
    the associated function if the corresponding file has
    changed.
    Returns -1 if the modification time can't be determined,
     0 if the file hasn't changed, and the return value
     of the subscription function otherwise.
    @param s a u8_subscription
    @returns int
**/
U8_EXPORT int u8_renew(u8_subscription s);

/** Renews all the subscriptions registered by u8_subscribe.
    @returns int
**/
U8_EXPORT int u8_renew_all(void);


#endif

