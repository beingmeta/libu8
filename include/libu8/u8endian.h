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

#ifndef LIBU8_U8ENDIAN_H
#define LIBU8_U8ENDIAN_H 1
#define LIBU8_U8ENDIAN_H_VERSION __FILE__

/* Handling endian-ness */

U8_INLINE unsigned int u8_flip_word(unsigned int _w)
{ return ((((_w) << 24) & 0xff000000) | (((_w) << 8) & 0x00ff0000) |
	  (((_w) >> 8) & 0x0000ff00) | ((_w) >>24) );}

U8_INLINE unsigned long long u8_flip_word8(unsigned long long _w)
{ return (((_w&(0xFF)) << 56) |
	  ((_w&(0xFF00)) << 48) |
	  ((_w&(0xFF0000)) << 24) |
	  ((_w&(0xFF000000)) << 8) |
	  ((_w>>56) & 0xFF) |
	  ((_w>>40) & 0xFF00) |
	  ((_w>>24) & 0xFF0000) |
	  ((_w>>8) & 0xFF000000));}

U8_INLINE_FCN unsigned int u8_flip_ushort(unsigned short _w)
{ return ((((_w) >> 8) & 0x0000ff) | (((_w) << 8) & 0x0000ff00) );}

#if WORDS_BIGENDIAN
#define u8_net_order(x) (x)
#define u8_host_order(x) (x)
#define u8_ushort_net_order(x) (x)
#define u8_ushort_host_order(x) (x)
#else
#define u8_net_order(x) fd_flip_word(x)
#define u8_host_order(x) fd_flip_word(x)
#define u8_ushort_host_order(x) fd_flip_ushort(x)
#define u8_ushort_net_order(x) fd_flip_ushort(x)
#endif

#endif
