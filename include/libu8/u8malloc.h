/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2016 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8MALLOC_H
#define LIBU8_U8MALLOC_H 1
#define LIBU8_U8MALLOC_H_VERSION __FILE__

/* Malloc */

U8_EXPORT void *u8_dmalloc(size_t);

static U8_MALLOCFN U8_MAYBE_UNUSED
void *u8_tidy_malloc(size_t n_bytes)
{
  void *ptr=malloc(n_bytes);
  if ( (ptr) && (errno) ) errno=0;
  return ptr;
}

static U8_MAYBE_UNUSED
void *u8_tidy_realloc(void *ptr,size_t newsz)
{
  if (ptr == NULL)
    return u8_tidy_malloc(newsz);
  else {
    void *newptr=realloc(ptr,newsz);
    if ( (newptr) && (errno) ) errno=0;
    return newptr;}
}

#ifndef U8_MALLOC
#if CHECK_DANGLING_ERRNOS
#define U8_MALLOC u8_tidy_malloc
#else
#define U8_MALLOC malloc
#endif
#endif

#if CHECK_DANGLING_ERRNOS
#define u8_realloc(ptr,newsz) (u8_tidy_realloc((void *)ptr,newsz))
#define u8_free(ptr) (free((char *)ptr),errno=0)
#else
#define u8_free(ptr) free((char *)ptr)
#define u8_realloc(ptr,newsz) \
  ((ptr==NULL) ? (U8_MALLOC(newsz)) : (realloc(ptr,newsz)))
#endif

#define u8_zero_array(r) memset(r,0,sizeof(r))
#define u8_zero_struct(r) memset(&r,0,sizeof(r))

#define u8_malloc(sz) U8_MALLOC(sz)

#define u8_xfree(ptr) if (ptr) free((char *)ptr); else ptr=ptr;

#define u8_alloc(t) ((t *)(u8_malloc(sizeof(t))))
#define u8_alloc_n(n,t) ((t *)(u8_malloc(sizeof(t)*(n))))
#define u8_realloc_n(ptr,n,t) ((t *)(u8_realloc(ptr,sizeof(t)*(n))))

#define u8_malloc_struct(sname) \
  ((struct sname *)(U8_MALLOC(sizeof(struct sname))))
#define u8_malloc_array(n,t) ((t *)(U8_MALLOC(n*sizeof(t))))

/* Zalloc (allocate and fill with zeros) */

/* Declare here. Also declared below with docs. */
U8_EXPORT char *u8_write_long_long(long long,char *,size_t);
U8_EXPORT void u8_raise(u8_condition,u8_context,u8_string);

/** Allocates and zero-clears a block of memory.
    This raises the condition u8_MallocFailed (using u8_raise) if
     it fails. The caller argument 
   @param n_bytes the number of bytes to allocate
   @param caller a u8_context (static) string identifying 
      the caller for use if the allocation fails
   @returns void *
**/
static U8_MALLOCFN U8_MAYBE_UNUSED
void *u8_zalloc_bytes(size_t n_bytes,u8_context caller)
{
  void *block = U8_MALLOC(n_bytes);
  if (block) {
    memset(block,0,n_bytes);
    return block;}
  else {
    char *buf=u8_malloc(32); 
    if (buf) {
      char *details = u8_write_long_long(n_bytes,buf,32);
      if (details)
	u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),buf);
      else u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),buf);}
    else u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),NULL);
    // Never reached
    return NULL;}
}
#define u8_zalloc(typename) u8_zalloc_bytes(sizeof(typename),NULL)

#define u8_zalloc(typename) u8_zalloc_bytes(sizeof(typename),NULL)
#define u8_zalloc_n(n,typename) u8_zalloc_bytes((n)*(sizeof(typename)),NULL)

#define u8_zalloc_for(caller,typename) \
  u8_zalloc_bytes(sizeof(typename),caller)
#define u8_zalloc_n_for(caller,n,typename) \
  u8_zalloc_bytes(((n)*(sizeof(typename))),caller)

#define u8_zalloc_struct(sname) \
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),NULL)))
#define u8_zalloc_struct_for(sname,caller) \
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),(caller))))

#define u8_zalloc_array(n,t)			\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),NULL)))
#define u8_zalloc_array_for(n,t,caller)		\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),caller)))

/** Allocates and zero-clears a block of memory
   @param sz the number of bytes to allocate
   @returns void *
**/
U8_EXPORT void *u8_mallocz(size_t sz);

/** Reallocates a block of memory, zero clearing any new parts
   @param ptr a previously allocated (with malloc) block of memory
   @param sz the number of bytes to allocate
   @param osz the previous number of bytes, needed to know how much
     to zero out
   @returns void *
**/
U8_EXPORT void *u8_reallocz(void *ptr,size_t sz,size_t osz);

/** Copies a block of memory into a larger block, zero clearing any new parts
   @param ptr an existing block of memory
   @param sz the number of bytes to allocate
   @param osz the number of bytes currently allocated at the location
   @returns void *
**/
U8_EXPORT void *u8_extalloc(void *ptr,size_t sz,size_t osz);

#define u8_allocz(t) ((t *)(u8_mallocz(sizeof(t))))
#define u8_allocz_n(n,t) ((t *)(u8_mallocz(sizeof(t)*(n))))

#ifndef UTF8_BUGWINDOW
#define UTF8_BUGWINDOW 64
#endif

#endif
