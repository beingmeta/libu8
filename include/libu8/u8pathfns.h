/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2014 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8pathfns.h
    These functions provide ways to manipulate file pathnames.
    They provide both utility functions and translation from
     unix-style UTF-8 pathnames to the local file system encoding
     and conventions.  They do not provide access (in general) to
     file metadata or content.
 **/

#ifndef LIBU8_U8PATHFNS_H
#define LIBU8_U8PATHFNS_H 1
#define LIBU8_U8PATHFNS_H_VERSION __FILE__

/** Gets the current working directory.
    Converts its result from the local encoding to utf-8
    @returns utf8_string
**/
U8_EXPORT u8_string u8_getcwd(void);

/** Changes the current working directory.
    Converts its argument into the local encoding and calls
    chdir() to make it the default directory.
    @param dirname the directory name in utf-8
    @returns -1 on error, 0 otherwise
**/
U8_EXPORT int u8_setcwd(u8_string dirname);

/** Returns an absolute local pathname from a utf-8 pathname.
    This handles interpretation of relative pathnames
    and ~ prefixes as well as encoding conversion.
    @param path a utf8 string
    @returns a locally-encoded string
**/
U8_EXPORT char *u8_localpath(u8_string path);

/** Makes a pathname by combining a directory and a name component.
    @param dir a utf-8 directory name
    @param name a utf-8 filename
    @returns a utf-8 string
**/
U8_EXPORT u8_string u8_mkpath(u8_string dir,u8_string name);

/** Returns an absolute pathname given a current directory.
    @param path a pathname
    @param wd a directory name
    @returns a utf-8 string
**/
U8_EXPORT u8_string u8_abspath(u8_string path,u8_string wd);

/** Returns the directory component of a pathname.
    @param path a utf-8 pathname
    @returns a utf-8 directory pathname
**/
U8_EXPORT u8_string u8_dirname(u8_string path);

/** Returns the non-directory non-suffix component of a pathname.
    Returns the non-directory non-suffix component of a pathname.
    If the suffix is provided, it is stripped from the end of the pathname.
    @param path a utf-8 pathname
    @param suffix a utf-8string
    @returns a utf-8 pathname
**/
U8_EXPORT u8_string u8_basename(u8_string path,u8_string suffix);


/** Returns an absolute pathname, resolving symbolic links.
    @param path a pathname
    @param wd a directory name
    @returns a utf-8 string
**/
u8_string u8_realpath(u8_string path,u8_string wd);

#endif

