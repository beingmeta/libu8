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

#if HAVE_FCNTL && defined(F_SETLK)
#define U8_USE_F_SETLK
#elif HAVE_FLOCK
#define U8_USE_FLOCK
#elif HAVE_LOCKF
#define U8_USE_LOCKF
#endif

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

/** Opens a file descriptor
    This converts the utf-8 *path* to the local encoding.
    If *lock* is &gt; 0, *path* is locked using u8_lock_fd
    (which generally uses `F_SETLK` with `fcntl`).
    @param path a utf-8 pathname string
    @param flags passed to open()
    @param mode passed to open()
    @param lock flags describing whether to lock the underlying file
    @returns a new file descriptor (int) or -1 on error
 **/
U8_EXPORT int u8_open_file(u8_string path,int flags,mode_t mode,int lock);

/* Deprecated */
U8_EXPORT int u8_open_fd(u8_string path,int flags,mode_t mode);

/** Closes a file descriptor.
    @param fd a valid file descriptor
    @returns -1 on error
 **/
U8_EXPORT int u8_close_file(int fd);
/* Deprecated */
U8_EXPORT int u8_close_fd(int fd);

/** Checks if a file descriptor is blocking or not
    @param fd a valid file descriptor
    @returns -1 on error
 **/
U8_EXPORT int u8_get_blocking(int fd);

/** Sets a file description to blocking or non blocking
    @param fd a valid file descriptor
    @param blocking whether to make the file descriptor blocking
    @returns -1 on error
 **/
U8_EXPORT int u8_set_blocking(int fd,int blocking);

/** Writes a buffer to a file descriptor
    @param fd a valid file descriptor
    @param buf a byte vector
    @param len the size of the data to write
    @returns -1 on error, bytes written otherwise
 **/
U8_EXPORT ssize_t u8_writeall(int fd,const unsigned char *buf,size_t len);

#endif

