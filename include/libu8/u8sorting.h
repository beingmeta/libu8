/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2017 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8crypto.h
    Provides u8_cryptofns.
    This wraps or implements various functions for cryptography
    and related functions (like digests)
 **/

#ifndef LIBU8_CRYPTOFNS_H
#define LIBU8_CRYPTOFNS_H 1
#define LIBU8_CRYPTOFNS_H_VERSION __FILE__

typedef int (*u8_comparefn)(void *x,void *y);
typedef int (*u8_xcomparefn)(void *x,void *y,void *data);

/** Sorts an array of pointer sized values
    @param vec         a pointer to an array of pointers (or pointer sized values)
    @param vec_len     the number of elements in the array
    @param cmp         a comparison function which returns an int given two pointer values
    @returns void
**/
U8_EXPORT void u8_sort_ptrs(void *vec[],int vec_len,u8_comparefn cmp);

/** Sorts an array of pointer sized values, providing data to the comparison function
    @param vec         a pointer to an array of pointers (or pointer sized values)
    @param vec_len     the number of elements in the array
    @param cmp         a comparison function which returns an int given three pointer values
    @param cmpdata     a void pointer which is passed as the third argument to the comparison function.
    @returns void
**/
U8_EXPORT void u8_xsort_ptrs(void *vec[],int vec_len,u8_xcomparefn cmp,void *cmpinfo);

#endif
