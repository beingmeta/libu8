/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2015 beingmeta, inc.
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
    @param a uid_t or int
    @returns a u8_string
**/
U8_EXPORT u8_string u8_username(uid_t uid);

/** Changes the mode of a file
    @param filename a utf-8 pathname
    @param mode the file mode for the file
    @returns int: 1 if the mode was changed, 0 if it wasn't, and -1 on error
 **/
U8_EXPORT int u8_chmod(u8_string filename,mode_t mode);

/* Making directories */

/** Makes a directory with a particular mode
    This handles conversion to the local character set.
    @param dirname a utf-8 pathname
    @param mode the file mode for the directory, if created
    @returns int: 0 if the directory already existed, 1
      if it was created, and -1 on error
 **/
U8_EXPORT int u8_mkdir(u8_string dirname,mode_t mode);

/** Makes all the containg directories for a pathname
    This handles conversion to the local character set.
    @param pathname a utf-8 pathname
    @param mode the file mode for any created directories
    @returns int: -1 on error or the number of directories
      actually created
 **/
U8_EXPORT int u8_mkdirs(u8_string pathname,mode_t mode);

/** Removes a directory, which must be empty
    This handles conversion to the local character set and uses
    the rmdir() system call.
    @param dirname a utf-8 pathname
    @returns int: 1 if the directory actually existed, 0
      if it didn't, and -1 on error
 **/
U8_EXPORT int u8_rmdir(u8_string dirname);

/** Removes a directory tree, which is deleted recursively
    @param dirname a utf-8 pathname
    @returns int: 1 if the directory actually existed, 0
      if it didn't, and -1 on error
 **/
U8_EXPORT int u8_rmtree(u8_string dirname);

/** Returns a unique temporary directory
    Based on Unix function mkdtemp, this takes a template of
     the form pathXXXXXX (6 Xs: some platforms allow more) and
     generates a unique directory name.  This copies its
     argument and adds extra Xs, also handling absolute path
     conversion and character set conversion with libc.
    @param template a utf-8 pathname template
    @returns a count of deleted paths (files or directories)
 **/
U8_EXPORT u8_string u8_tempdir(u8_string template);

/* Searching for files */

/** Finds a file meeting certain criteria on a complex search path.
    This function generates a series of pathnames based on the name and
     the searchpath and returns the first pathname for which testp returns
     non-zero.
    The search path consists of colon separated components; if a component
     contains a '%' character, a pathname is generated by substituting
     name for the '%' character; otherwise, the component is appended,
     as a directory name, to name.
    @param name a utf-8 pathname
    @param searchpath a utf-8 string
    @param testp a test function (if NULL, uses u8_file_existsp)
    @returns a u8_string
**/
U8_EXPORT u8_string u8_find_file
  (u8_string name,u8_string searchpath,int (*testp)(u8_string s));

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

/* Getting directory contents */

/** Returns the non-directory non-special files within a directory.
    This handles ingoing and outoing conversion of pathnames to the
    local character set.  Symbolic links are included if they resolve
    to non-directory files.
    @param dirname a utf-8 pathname
    @param fullpath if non-zero, return absolute pathnames
    @returns a NULL-terminated array of utf-8 strings.
 **/
U8_EXPORT u8_string *u8_getfiles(u8_string dirname,int fullpath);

/** Returns the subdirectories within a directory.
    This handles ingoing and outoing conversion of pathnames to the
    local character set.  Symbolic links are included if they resolve
    to directories.
    @param dirname a utf-8 pathname
    @param fullpath if non-zero, return absolute pathnames
    @returns a NULL-terminated array of utf-8 strings.
 **/
U8_EXPORT u8_string *u8_getdirs(u8_string dirname,int fullpath);

#endif

