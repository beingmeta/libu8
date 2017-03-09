/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2009-2017 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8sorting.h"

#include <string.h>

U8_INLINE void ptr_swap(void **a,void **b)
{
  void *t;
  t = *a;
  *a = *b;
  *b = t;
}

static void sortptrs(void *v[],int n,u8_comparefn cmp)
{
  unsigned i, j, ln, rn;
  while (n > 1) {
    ptr_swap(&v[0], &v[n/2]);
    for (i = 0, j = n; ; ) {
      do --j; while (cmp(v[j],v[0])>0);
      do ++i; while (i < j && (cmp(v[i],v[0])<0));
      if (i >= j) break; else {}
      ptr_swap(&v[i], &v[j]);}
    ptr_swap(&v[j], &v[0]);
    ln = j;
    rn = n - ++j;
    if (ln < rn) {
      sortptrs(v, ln,cmp); v += j; n = rn;}
    else {sortptrs(v + j, rn,cmp); n = ln;}}
}

U8_EXPORT void u8_sort_ptrs(void *v[],int n,u8_comparefn cmp)
{
  sortptrs(v,n,cmp);
}

static void xsortptrs(void *v[],int n,u8_xcomparefn cmp,void *cmpdata)
{
  unsigned int i, j, ln, rn;
  while (n > 1) {
    ptr_swap(&v[0], &v[n/2]);
    for (i = 0, j = n; ; ) {
      do --j; while (cmp(v[j],v[0],cmpdata)>0);
      do ++i; while (i < j && (cmp(v[i],v[0],cmpdata)<0));
      if (i >= j) break; else {}
      ptr_swap(&v[i], &v[j]);}
    ptr_swap(&v[j], &v[0]);
    ln = j;
    rn = n - ++j;
    if (ln < rn) {
      xsortptrs(v, ln, cmp, cmpdata); v += j; n = rn;}
    else {xsortptrs(v + j, rn, cmp, cmpdata); n = ln;}}
}

U8_EXPORT void u8_xsort_ptrs(void *v[],int n,u8_xcomparefn cmp,void *cmpdata)
{
  xsortptrs(v,n,cmp,cmpdata);
}


U8_EXPORT void u8_init_sorting_c()
{
  
}
