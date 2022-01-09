/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8LIBC_H
#define LIBU8_U8LIBC_H 1
#define LIBU8_U8LIBC_H_VERSION __FILE__

/** Converts a UTF-8 string to the encoding expected by system (libc) functions.
    If the system encoding is UTF-8, this returns its argument, rather than
    copying it, as u8_tolibc does.
    @param string a utf-8 string
    @returns a character string in the local encoding
**/
U8_EXPORT char *u8_2libc(u8_string string);

/** Converts a UTF-8 string to the encoding expected by system (libc) functions.
    If the system encoding is UTF-8, this validates and copies the argument.
    @param string a utf-8 string
    @returns a character string in the local encoding
**/
U8_EXPORT char *u8_tolibc(u8_string string);

/** Converts a natively encoded string into a UTF-8 string.
    If the system encoding is UTF-8, this validates and copies the argument.
    @param local_string a locally encoded text string
    @returns a utf-8 encoded string
**/
U8_EXPORT u8_string u8_fromlibc(char *local_string);

/** This sets the functions used to map to and from libc.
    @param fromfn a function from character strings to utf-8 strings
    @param tofn a function from utf-8 strings to character strings.
    @returns void */
U8_EXPORT void u8_set_libcfns
(u8_string (*fromfn)(char *),char *(*tofn)(u8_string));

/* Shims */

/** This is a shim for execvpe in environments (notably MacOS) which lack it.
    @param a program name which will be looked up in PATH
    @param a NULL-terminated array of string arguments
    @param a NULL-terminated array of environment binding strings of the form `var=val`
    @returns -1 on error, otherwise doesn't return */
U8_EXPORT int u8_execvpe
(char *prog,char *const argv[],char *envp[]);

#if ! HAVE_EXECVPE
#define execvpe u8_execvpe
#endif

#endif
