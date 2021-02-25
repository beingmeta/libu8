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

#ifndef LIBU8_U8MALLOC_H
#define LIBU8_U8MALLOC_H 1
#define LIBU8_U8MALLOC_H_VERSION __FILE__

/* Malloc */

U8_EXPORT ssize_t u8_max_malloc;
U8_EXPORT void *u8_watchptr;
U8_EXPORT void *u8_dmalloc(size_t);
U8_EXPORT void *u8_dmalloc_n(size_t,size_t);
U8_EXPORT void *u8_drealloc(void *,size_t);
U8_EXPORT void u8_dfree(void *);

#ifndef U8_MMAP_THRESHOLD
#define U8_MMAP_THRESHOLD 0x8000000
#endif

U8_EXPORT size_t u8_mmap_threshold;
U8_EXPORT void *u8_big_alloc(ssize_t n);
U8_EXPORT void *u8_big_realloc(void *ptr,ssize_t n);
U8_EXPORT ssize_t u8_big_free(void *);
U8_EXPORT void *u8_big_copy(const void *src,ssize_t newlen,ssize_t oldlen);
U8_EXPORT void *u8_big_calloc(ssize_t n,ssize_t eltsz);

#define u8_big_alloc_n(n,type) (u8_big_calloc((n),sizeof(type)))
#define u8_big_realloc_n(ptr,n,t) ((t *)(u8_big_realloc(ptr,sizeof(t)*(n))))

#ifdef u8_malloc
/* Assume everything is defined */
#elif U8_DEBUG_MALLOC
#define u8_malloc u8_dmalloc
#define u8_malloc_n(n,sz) (u8_dmalloc_n((n),(sz)))
#define u8_realloc(n,sz) (u8_drealloc((n),(sz)))
#define u8_zmalloc(sz) (u8_dmalloc(sz))
#define u8_zmalloc_n(n,sz) (u8_dmalloc(n*sz))
#else /* U8_DEBUG_MALLOC */
U8_INLINE_FCN void *u8_malloc(size_t sz)
{
  void *result = malloc(sz);
  if (result) errno=0;
  return result;
}
U8_INLINE_FCN void *u8_malloc_n(size_t n,size_t sz)
{
  void *result = calloc(n,sz);
  if (result) errno=0;
  return result;
}
U8_INLINE_FCN void *u8_realloc(void *ptr,size_t new_size)
{
  void *result = (ptr == NULL) ? (malloc(new_size)) : (realloc(ptr,new_size));
  if (result) errno=0;
  return result;
}
U8_INLINE_FCN void *u8_zmalloc(size_t sz)
{
  void *result = calloc(sz,1);
  if (result) errno=0;
  return result;
}
U8_INLINE_FCN void *u8_zmalloc_n(size_t n,size_t sz)
{
  void *result = calloc(n,sz);
  if (result) errno=0;
  return result;
}
#endif /* not U8_DEBUG_MALLOC */

U8_INLINE_FCN void _u8_free(void *ptr)
{
  free((void *)ptr);
  errno=0;
}
#define u8_free(x) _u8_free((void *)(x))

#define u8_zero_array(r) memset(r,0,sizeof(r))
#define u8_zero_struct(r) memset(&r,0,sizeof(r))

#define u8_xfree(ptr) if (ptr) free((char *)ptr); else ptr=ptr;

#define u8_alloc(t) ((t *)(u8_zmalloc(sizeof(t))))
#define u8_alloc_n(n,t) ((t *)(u8_malloc_n(n,sizeof(t))))
#define u8_realloc_n(ptr,n,t) ((t *)(u8_realloc(ptr,sizeof(t)*(n))))

#define u8_malloc_struct(sname)				\
  ((struct sname *)(u8_zmalloc(sizeof(struct sname))))
#define u8_malloc_array(n,t) ((t *)(u8_malloc(n*sizeof(t))))

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
void *u8_alloc_throw(size_t n_bytes,u8_context caller)
{
  void *block = u8_malloc(n_bytes);
  if (block) return block;
  else {
    u8_raise(u8_MallocFailed,U8ALT(caller,"u8_zalloc_bytes"),NULL);
    return NULL;}
}

#define u8_zalloc_bytes(n,caller) u8_zmalloc(n)
#define u8_zalloc(typename) u8_zmalloc(sizeof(typename))
#define u8_zalloc_n(n,typename) u8_zmalloc_n(n,sizeof(typename))

#define u8_zalloc_for(caller,typename)		\
  u8_zalloc_bytes(sizeof(typename),caller)
#define u8_zalloc_n_for(caller,n,typename)		\
  u8_zalloc_bytes(((n)*(sizeof(typename))),caller)

#define u8_zalloc_struct(sname)						\
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),NULL)))
#define u8_zalloc_struct_for(sname,caller)				\
  ((struct sname *)(u8_zalloc_bytes(sizeof(struct sname),(caller))))

#define u8_zalloc_array(n,t)			\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),NULL)))
#define u8_zalloc_array_for(n,t,caller)			\
  ((t *)(u8_zalloc_bytes((n*sizeof(t)),caller)))

/** Reallocates a block of memory, zero clearing any new parts
    @param ptr a previously allocated (with malloc) block of memory
    @param sz the number of bytes to allocate
    @param osz the previous number of bytes, needed to know how much
    to zero out
    @returns void *
**/
U8_EXPORT void *u8_zrealloc(void *ptr,size_t sz,size_t osz);

/** Copies a block of memory into a larger block, zero clearing any new parts
    @param ptr an existing block of memory
    @param sz the number of bytes to allocate
    @param osz the number of bytes currently allocated at the location
    @returns void *
**/
U8_EXPORT void *u8_extalloc(void *ptr,size_t sz,size_t osz);

/* Because it allocates memory */

U8_MAYBE_UNUSED
/** Copies a block of bytes into a freshly allocated block
    @param size the length of the block
    @param source a pointer to the block
    @returns a pointer to the newly allocated block
**/
static void *u8_memdup(size_t len,const void *source)
{
  unsigned char *copy=u8_malloc(len);
  if ((copy)&&(source)) {
    memcpy(copy,source,len);
    return copy;}
  else if (source==NULL) {
    u8_seterr(u8_NullArg,"u8_memdup",NULL);
    return NULL;}
  else {
    u8_seterr(u8_MallocFailed,"u8_memdup",NULL);
    return NULL;}
}

/* Legacy definitions */
#define u8_allocz u8_alloc
#define u8_allocz_n u8_alloc_n
#define u8_mallocz u8_zmalloc
#define u8_reallocz u8_zrealloc

#ifndef UTF8_BUGWINDOW
#define UTF8_BUGWINDOW 64
#endif

#endif
