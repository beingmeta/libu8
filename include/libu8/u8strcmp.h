/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2021 beingmeta, LLC
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

static int _u8_strcmp(u8_string s1,u8_string s2,int cmpflags)
{
  u8_string scan1 = s1, scan2 = s2;
  while ( (*scan1) && (*scan2) ) {
    if ( (*scan1 < 0x80 ) && (*scan2 < 0x80 ) ) {
      int c1, c2;
      if (cmpflags & U8_STRCMP_CI) {
	c1 = u8_tolower(*scan1);
	c2 = u8_tolower(*scan2);}
      else {
	c1 = *scan1;
	c2 = *scan2;}
      if (c1 < c2)
	return -1;
      else if (c1 > c2)
	return 1;
      else NO_ELSE;
      scan1++;
      scan2++;}
    else {
      int c1 = u8_sgetc(&scan1);
      int c2 = u8_sgetc(&scan2);
      if (cmpflags & U8_STRCMP_CI) {
	c1 = u8_tolower(c1);
	c2 = u8_tolower(c2);}
      if (c1<c2)
	return -1;
      else if (c1>c2)
	return 1;
      else NO_ELSE;}}
  if (*scan1)
    return 1;
  else if (*scan2)
    return -1;
  else return 0;
}

#define u8_strcmp _u8_strcmp

#endif

