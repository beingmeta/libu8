/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2012 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_STDIO_H
#define LIBU8_STDIO_H 1
#define LIBU8_STDIO_H_VERSION __FILE__

#include <stdio.h>
#include <stdarg.h>

/* With psuedo encodings */

U8_EXPORT void u8_ascii_stdio(void);
U8_EXPORT void u8_latin1_stdio(void);
U8_EXPORT void u8_utf8_stdio(void);

U8_EXPORT void u8_stdoutISstderr(int);

U8_EXPORT void u8_fprintf(FILE *f,u8_string format_string,...);
U8_EXPORT void u8_fputs(u8_string s,FILE *f);

#endif /* #ifndef LIBU8_STDIO_H */
