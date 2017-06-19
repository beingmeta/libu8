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

#ifndef LIBU8_U8PILES_H
#define LIBU8_U8PILES_H 1
#define LIBU8_U8PILES_H_VERSION __FILE__

/* Piles */

/** struct U8_PILE
   is a growable vector of void* pointers
**/
typedef struct U8_PILE {
  void **u8_elts;
  size_t u8_len, u8_max;
  int u8_mallocd;} U8_PILE;
typedef struct U8_PILE *u8_pile;

U8_EXPORT int _u8_grow_pile(u8_pile p,int delta);

#define U8_INIT_PILE(p,len) \
  p->u8_len=0; p->u8_max=len; p->u8_mallocd=1; \
  p->u8_elts=u8_malloc(sizeof(void *)*len)
#define U8_INIT_STATIC_PILE(p,vec,len)          \
  p->u8_len=0; p->u8_max=len; p->u8_elts=(void **)vec

#define u8_pile_add(p,e)				\
  if ((p)->u8_len>=(p)->u8_max)				\
    {_u8_grow_pile((p),1);				\
      (p)->u8_elts[((p)->u8_len)++]=((void *)e);}	\
  else (p)->u8_elts[((p)->u8_len)++]=((void *)e)

#endif
