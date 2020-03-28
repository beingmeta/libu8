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

#ifndef LIBU8_U8STRINGS_H
#define LIBU8_U8STRINGS_H 1
#define LIBU8_U8STRINGS_H_VERSION __FILE__

/* String/character typedefs */

typedef unsigned char u8_byte;
typedef const u8_byte *u8_string;
typedef u8_byte *u8_buf;
typedef int u8_char;
typedef int u8_unicode_char;
typedef int u8_unichar;

typedef char *u8_charstring;
typedef char *u8_chars;

/* This is for offsets into UTF-8 encoded strings, to distinguish
   byte and character offsets. */
typedef ssize_t u8_byteoff;
typedef ssize_t u8_charoff;

/* Type coercion, conditional, and constant string macros */

#define U8STR(x)   ((u8_string)(x))
#define U8ALT(s,d) ((s)?((u8_string)(s)):((u8_string)(d)))
#define U8IF(s,d)  ((s)?((u8_string)(d)):(U8SNUL))
#define U8S(s)     ((s)?((u8_string)(s)):((u8_string)""))
#define U8S0()     ((u8_string)(""))

#define U80STR  ((u8_string)(""))
#define U8SNUL  ((u8_string)(""))
#define U8SPACE ((u8_string)(" "))

#define U8OPT(string,after) \
  ((string)?(string):(U8SNUL)),((string)?(after):(U8SNUL))

#define U8OPTSTR(b,s,a) (U8IF(s,b)),(U8ALT(s,(U8SNUL))),(U8IF(s,a))

/* strdup */

#if HAVE_STRDUP
#define u8_strdup(x) (((x)==NULL)?((u8_buf)NULL):((u8_buf)strdup(x)))
#else
#define u8_strdup(x) (((x)==NULL)?((u8_buf)NULL):(_u8_strdup(x)))
#endif

#define u8dup(x) ((U8_EXPECT_FALSE(x==NULL))?(NULL):(u8_strdup(x)))
#define u8s(x) ((U8_EXPECT_FALSE(x==NULL))?(NULL):(u8_strdup(x)))

U8_EXPORT u8_buf _u8_strdup(u8_string);
U8_EXPORT u8_buf u8_strndup(u8_string,int);

/* Comparison */

#define U8_STRCMP_CI 1
U8_EXPORT int u8_strcmp(u8_string s1,u8_string s2,int cmpflags);

#define HAVE_U8_STRCMP 1

/* UTF-8 handling config functions */

/** Gets/sets whether warnings are produced for invalid UTF-8 sequences, which
     can be annoyingly common with some content.
    @param flag an int
    @returns int
  If the flag is negative, the current value is returned
**/
U8_EXPORT int u8_config_utf8warn(int flag);

/** Gets/sets whether errors are produced for invalid UTF-8 sequences.
    @param flag an int
    @returns int
  If the flag is negative, the current value is returned
**/
U8_EXPORT int u8_config_utf8err(int flag);

#endif

