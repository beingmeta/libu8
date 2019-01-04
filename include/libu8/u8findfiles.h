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

#ifndef LIBU8_U8FINDFILES_H
#define LIBU8_U8FINDFILES_H 1
#define LIBU8_U8FINDFILES_H_VERSION __FILE__

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

#endif
