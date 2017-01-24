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

#ifndef LIBU8_U8SOURCEFILES_H
#define LIBU8_U8SOURCEFILES_H 1
#define LIBU8_U8SOURCEFILES_H_VERSION __FILE__

/* File and module recording */

typedef struct U8_SOURCE_FILE_RECORD {
  u8_string filename;
  struct U8_SOURCE_FILE_RECORD *next;} U8_SOURCE_FILE_RECORD;
typedef struct U8_SOURCE_FILE_RECORD *u8_source_file_record;

U8_EXPORT void u8_register_source_file(u8_string s);
U8_EXPORT void u8_for_source_files(void (*f)(u8_string s,void *),void *data);

#endif