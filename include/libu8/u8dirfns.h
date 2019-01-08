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

#ifndef LIBU8_U8DIRFNS_H
#define LIBU8_U8DIRFNS_H 1
#define LIBU8_U8DIRFNS_H_VERSION __FILE__

/* Making directories */

U8_EXPORT mode_t u8_default_dir_mode;


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

/** Returns a selection of the files within a directory.
    This handles ingoing and outoing conversion of pathnames to the
    local character set.  Symbolic links are included whether they 
    resolve to real files or not.
    @param dirname a utf-8 pathname
    @param which a mask specifying the kinds of files to return
    @param fullpath if non-zero, return absolute pathnames
    @returns a NULL-terminated array of utf-8 strings.
 **/
U8_EXPORT u8_string *u8_readdir(u8_string dirname,int which,int fullpath);

#define U8_LIST_FILES 1
#define U8_LIST_DIRS 2
#define U8_LIST_LINKS 4
#define U8_LIST_MAGIC 8

#endif
