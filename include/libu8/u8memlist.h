/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 beingmeta, LLC
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** struct U8_MEMLIST
    Linked list of memory pointers
    * The u8ml_ptr contains a generic (void *) pointer;
    * The u8ml_big_alloc is 1 if the pointer is big_alloc'd, zero otherwise
    * The u8ml_next contains a pointer to another U8_MEMLIST struct or NULL
    **/
typedef struct U8_MEMLIST {
  void *u8ml_ptr;
  int u8ml_big;
  struct U8_MEMLIST *u8ml_next;} *u8_memlist;

/** Adds to a linked list of memory pointers, returning the new list
    @param ptr a generic pointer to a memory location
    @param cdr the existing U8_MEMLIST structure we're extending (or NULL)
    @param big non-zero if the generic pointer was allocated with u8_big_alloc
    @returns a pointer to a U8_MEMLIST structure

    The *cdr* argument should not used after this function is called.
**/
U8_EXPORT u8_memlist u8_cons_list(void *ptr,u8_memlist cdr,int big);

/** Frees a linked list of memory pointers
    @param lst a pointer to an allocated U8_MEMLIST structure or NULL
    @returns void
**/
U8_EXPORT void u8_free_list(u8_memlist lst);
