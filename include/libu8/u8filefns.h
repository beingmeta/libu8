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

#ifndef LIBU8_U8FILEFNS_H
#define LIBU8_U8FILEFNS_H 1
#define LIBU8_U8FILEFNS_H_VERSION __FILE__

#if UID_T_UNSIGNED
typedef int u8_uid;
#else
typedef uid_t u8_uid;
#endif

#if GID_T_UNSIGNED
typedef int u8_gid;
#else
typedef gid_t u8_gid;
#endif

/** \file u8filefns.h
    These functions access the metadata for local files.
    They include checking existing, basic type, readability,
     writability, timestamp, and owner information.
    The also include the functions for finding files along
     search paths.
 **/

/** Determines if a pathname refers to a directory.
    This returns 0 (rather than -1) if the designated path does
     not exist.
    @param filename a utf-8 pathname
    @returns 1 if the pathname identifies a directory, 0 otherwise
**/
U8_EXPORT int u8_directoryp(u8_string filename);

/** Checks if a named file exists and is a symbolic link
    @param filename a utf-8 pathname
    @returns 1 if the file exists and is a symlink, 0 otherwise
**/
U8_EXPORT int u8_symlinkp(u8_string filename);

/** Checks if a named file exists and is a socket
    @param filename a utf-8 pathname
    @returns 1 if the file exists and is a socket, 0 otherwise
**/
U8_EXPORT int u8_socketp(u8_string filename);

/** Determines if a named file exists.
    @param filename a utf-8 pathname
    @returns 1 if the file exists, 0 otherwise
**/
U8_EXPORT int u8_file_existsp(u8_string filename);

/** Determines if a named file is readable.
    @param filename a utf-8 pathname
    @returns 1 if the file is readable, 0 otherwise
**/
U8_EXPORT int u8_file_readablep(u8_string filename);

/** Determines if a named file exists and is writable.
    @param filename a utf-8 pathname
    @returns 1 if the file is writable, 0 otherwise
**/
U8_EXPORT int u8_file_writablep(u8_string filename);

/** Determines if a named file exists and is a regular file
    @param filename a utf-8 pathname
    @returns 1 if the file is regular, 0 otherwise
**/
U8_EXPORT int u8_file_regularp(u8_string filename);

/** Returns the creation time of a file
    @param filename a utf-8 pathname
    @returns time_t (the file creation time)
**/
U8_EXPORT time_t u8_file_ctime(u8_string filename);


/** Returns the latest modification time for a file
    @param filename a utf-8 pathname
    @returns time_t (the file modification time)
**/
U8_EXPORT time_t u8_file_mtime(u8_string filename);

/** Sets the latest modification time for a file
    @param filename a utf-8 pathname
    @returns time_t (the file modification time)
**/
U8_EXPORT time_t u8_set_mtime(u8_string filename,time_t);

/** Returns the latest access time of a file
    @param filename a utf-8 pathname
    @returns time_t (the file modification time)
**/
U8_EXPORT time_t u8_file_atime(u8_string filename);

/** Sets the latest access time for a file
    @param filename a utf-8 pathname
    @returns time_t (the file access time)
**/
U8_EXPORT time_t u8_set_atime(u8_string filename,time_t);

/** Returns the mode (permissions) time of a file
    @param filename a utf-8 pathname
    @returns mode_t
**/
U8_EXPORT int u8_file_mode(u8_string filename);

/** Returns the size (in bytes) of a file
    @param filename a utf-8 pathname
    @returns an off_t value
**/
U8_EXPORT ssize_t u8_file_size(u8_string filename);

/** Returns the owner of a file (as a string)
    @param filename a utf-8 pathname
    @returns a u8_string
**/
U8_EXPORT u8_string u8_file_owner(u8_string filename);

/** Returns the name associated with a UID, when available
    @param uid (a numeric uid)
    @returns a u8_string
**/
U8_EXPORT u8_string u8_username(u8_uid uid);

/** Returns the name associated with a UID, when available
    @param name (a u8_string)
    @returns u8_uid (probably some kind of int)
**/
U8_EXPORT u8_uid u8_getuid(u8_string name);

/** Returns the name associated with a GID, when available
    @param gid (a numeric uid)
    @returns a u8_string
**/
U8_EXPORT u8_string u8_groupname(u8_gid gid);

/** Returns the name associated with a UID, when available
    @param name (a u8_string)
    @returns u8_gid (probably some kind of int)
**/
U8_EXPORT u8_gid u8_getgid(u8_string name);

/** Changes the mode of a file
    @param filename a utf-8 pathname
    @param mode the file mode for the file
    @returns int: 1 if the mode was changed, 0 if it wasn't, and -1 on error
 **/
U8_EXPORT int u8_chmod(u8_string filename,mode_t mode);

/** Changes access information for a file
    @param filename a utf-8 pathname
    @param owner a utf-8 string (or NULL)
    @param group a utf-8 string (or NULL)
    @param mode the file mode for the file
    @returns int: -1 on error, number of changes (uid,gid,mode) otherwise
 **/
U8_EXPORT int u8_set_access
(u8_string filename,u8_string owner,u8_string group,mode_t mode);

/** Changes access information for a file
    @param filename a utf-8 pathname
    @param owner a utf-8 string (or NULL)
    @param group a utf-8 string (or NULL)
    @param mode the file mode for the file
    @returns int: -1 on error, number of changes (uid,gid,mode) otherwise
 **/
U8_EXPORT int u8_set_access_x
(u8_string filename,u8_uid owner,u8_gid group,mode_t mode);

/* File manipulation functions */

/** Removes a file from the local file system.
    This handles conversion to the local character set.
    @param filename a utf-8 pathname
    @returns int (-1 on error)
 **/
U8_EXPORT int u8_removefile(u8_string filename);

/** Moves a file within the local file system.
    This handles conversion of pathnames to the local character set.
    @param from a utf-8 pathname
    @param to a utf-8 pathname
    @returns int (-1 on error)
 **/
U8_EXPORT int u8_movefile(u8_string from,u8_string to);

/** Makes a symbolic link within the local file system.
    This handles conversion of pathnames to the local character set.
    @param from a utf-8 pathname
    @param to a utf-8 pathname
    @returns int (-1 on error)
 **/
U8_EXPORT int u8_linkfile(u8_string from,u8_string to);

/** Gets the target of a symbolic link, possibly converting to absolute form
    @param filename a utf-8 pathname (a symbolic link)
    @param absolute an int, non-zero resolves . and .. references
    @returns a pathname, NULL on error
 **/
U8_EXPORT u8_string u8_readlink(u8_string filename,int absolute);

/** Gets the target of a symbolic link, possibly converting to absolute form
    @param filename a utf-8 pathname (a symbolic link)
    @param absolute an int, non-zero resolves . and .. references
    @returns a pathname, NULL on error
 **/
U8_EXPORT u8_string u8_getlink(u8_string filename,int absolute);

#include "u8dirfns.h"
#include "u8findfiles.h"

#endif

