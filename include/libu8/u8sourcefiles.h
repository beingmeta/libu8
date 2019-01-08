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

#ifndef LIBU8_U8SOURCEFILES_H
#define LIBU8_U8SOURCEFILES_H 1
#define LIBU8_U8SOURCEFILES_H_VERSION __FILE__

/* File and module recording */

/** struct U8_SOURCE_FILE_RECORDS

    records source file information for an application.  Source file
    initializations can call `u8_register_source_file` to register a
    particular source file for use in diagnosis and debugging.  The
    argument can be provided by pre-processor macros file _FILE_ or
    other constructions. The best practice is to use a string which
    contains additional identifying information such as an SHA1 hash
    and a modification time. This is used in the libu8 sources and
    implemented etc/fileinfo.c and the libu8 makefile. */
typedef struct U8_SOURCE_FILE_RECORD {
  u8_string filename;
  struct U8_SOURCE_FILE_RECORD *next;} U8_SOURCE_FILE_RECORD;
typedef struct U8_SOURCE_FILE_RECORD *u8_source_file_record;

/** Declares source file information
    @param source at UTF-8 string
    @returns void

    Records information about a source file in the runtime environment.
    Note that the argument is not copied, so it should be either static
    or malloc'd.
**/
U8_EXPORT void u8_register_source_file(u8_string source);

/** Iterates over source file information
    @param iterfn a function taking a constant UTF-8 string and a void * argument
    @param data a void * data pointer to be passed to *iterfn*
    @returns void

    Iterates over all declared source files
**/
U8_EXPORT void u8_for_source_files(void (*iterfn)(u8_string s,void *),void *data);

#endif
