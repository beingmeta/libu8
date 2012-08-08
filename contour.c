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

#define U8_INLINE_CONTOURS 1

#include "libu8/libu8.h"

u8_condition u8_BadDynamicContour=_("Bad Dynamic Contour");

#if (U8_USE_TLS)
u8_tld_key u8_dynamic_contour_key;
#elif (U8_USE__THREAD)
__thread u8_contour u8_dynamic_contour;
#else
u8_contour u8_dynamic_contour;
#endif

U8_EXPORT void _u8_push_contour(u8_contour contour)
{
  u8_push_contour(contour);
}

U8_EXPORT void _u8_pop_contour(u8_contour contour)
{
  u8_pop_contour(contour);
}

U8_EXPORT void _u8_throw_contour(u8_contour contour)
{
  u8_throw_contour(contour);
}

U8_EXPORT void u8_grow_contour(u8_contour c)
{
  if (c->u8c_max_blocks==0) {
    c->u8c_blocks=u8_malloc(U8_CONTOUR_INIT_N_BLOCKS*sizeof(void *));
    c->u8c_max_blocks=U8_CONTOUR_INIT_N_BLOCKS;}
  else if ((c->u8c_flags)&(U8_CONTOUR_BLOCKS_MALLOCD)) {
    int new_max=c->u8c_max_blocks*2;
    void **new_blocks=u8_realloc(c->u8c_blocks,new_max*sizeof(void *));
    c->u8c_blocks=new_blocks;
    c->u8c_max_blocks=new_max;}
  else {
    int new_max=c->u8c_max_blocks*2;
    void **new_blocks=u8_malloc(new_max*sizeof(void *));
    memcpy(new_blocks,c->u8c_blocks,sizeof(void *)*(c->u8c_n_blocks));
    c->u8c_blocks=new_blocks;
    c->u8c_max_blocks=new_max;}
}

struct U8_CONTOUR_TYPE u8_contour_types[16]=
  {{"u8contour",0,sizeof(struct U8_CONTOUR),NULL},
   {NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},
   {NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},
   {NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL},
   {NULL,0,0,NULL},{NULL,0,0,NULL},{NULL,0,0,NULL}};

int u8_n_contour_types=1;

/* Contour malloc support */

U8_EXPORT void *_u8_contour_malloc(u8_contour c,size_t bytes)
{
  return u8_contour_malloc(c,bytes);
}

U8_EXPORT void _u8_contour_release(u8_contour c,void *ptr)
{
  return _u8_contour_release(c,ptr);
}

U8_EXPORT void u8_contour_free(u8_contour c,void *ptr)
{
  u8_contour scan=c;
  while (scan) {
    if (scan->u8c_n_blocks) {
      void **blocks=scan->u8c_blocks;
      int i=scan->u8c_n_blocks-1;
      /* Intentionally scanning backwards because that seems like a more
	 likely case. */
      while (i>=0)
	if (blocks[i]==ptr) {
	  blocks[i]=NULL; u8_free(ptr);
	  return;}
	else i--;}
    scan=scan->u8c_outer_contour;}
  /* Should this err or warn? */
  u8_free(ptr);
}

U8_EXPORT void *u8_contour_realloc(u8_contour c,void *ptr,size_t new_size)
{
  u8_contour scan=c;
  while (scan) {
    if (scan->u8c_n_blocks) {
      void **blocks=scan->u8c_blocks;
      /* Intentionally scanning backwards */
      int i=scan->u8c_n_blocks-1;
      while (i>=0)
	if (blocks[i]==ptr) {
	  void *newptr=u8_realloc(ptr,new_size);
	  blocks[i]=newptr;
	  return newptr;}
	else i++;}
    scan=scan->u8c_outer_contour;}
  /* Should this err or warn? */
  return u8_realloc(ptr,new_size);
}

/* Initialization function (just records source file info) */

U8_EXPORT void u8_init_contour_c()
{
  u8_register_source_file(_FILEINFO);
}
