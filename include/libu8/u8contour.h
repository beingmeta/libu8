/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8contour.h
    These declarations and macros support dynamic execution contexts
     for use in exception handling, memory de/allocation, etc.
 **/

#ifndef LIBU8_U8CONTOUR_H
#define LIBU8_U8CONTOUR_H 1
#define LIBU8_U8CONTOUR_H_VERSION __FILE__

#ifndef U8_INLINE_CONTOURS
#define U8_INLINE_CONTOURS 0
#endif

#ifndef U8_CONTOUR_SHORT_STACK_SIZE
#define U8_CONTOUR_SHORT_STACK_SIZE 8
#endif

#include <setjmp.h>

U8_EXPORT u8_condition u8_BadDynamicContour;

#ifndef U8_CONTOUR_INIT_N_BLOCKS
#define U8_CONTOUR_INIT_N_BLOCKS 64
#endif

/* Flags */

#define U8_CONTOUR_BASE_FLAGS 0x1001
#define U8_CONTOUR_VALIDP(x) ((((x)->u8c_flags)&(0x0F))==0x1001)
#define U8_CONTOUR_TYPE_BASE 8
#define U8_CONTOUR_TYPE(x) ((((x)->u8c_flags)&(0xF0))>>4)
#define U8_CONTOUR_FLAGS_BASE 8
#define U8_CONTOUR_EXCEPTIONAL U8_CONTOUR_FLAGS_BASE
#define U8_CONTOUR_LABEL_MALLOCD ((U8_CONTOUR_EXCEPTIONAL)<<1)
#define U8_CONTOUR_BLOCKS_MALLOCD ((U8_CONTOUR_LABEL_MALLOCD)<<1)

/* A contour is a dynamic context used for a variety of purposes
     including exception handling, non-local exits, memory de/allocation,
     debugging, etc.

   All contours have the same initial set of fields defined by libu8.
   These fields are defined in the macro U8_CONTOUR_FIELDS for inclusion
     in other contour structs.

   These include a label (u8_string, possibly NULL), flags (an unsigned int),
     a jumpbuffer (for setjmp/longjmp), and a pointer to the containing
     context, and information for cleanup.  The low 8 bits of the flags
     specify the contour type (see above) which provides a name and a
     pop function.  In addition, the default contour contains two queues
     for locks and blocks (mutexes and malloc'd memory), each of which
     has a length and a max.
*/

/* Maybe this should also have a queue of cleanup functions */
#define U8_CONTOUR_FIELDS \
  const unsigned char *u8c_label;			     \
  unsigned int u8c_flags;				     \
  int u8c_depth;					     \
  sigjmp_buf u8c_jmpbuf;				     \
  u8_condition u8c_condition;				     \
  union {						     \
    u8_exception exception;				     \
    u8_context context;} u8c_exinfo;			     \
  u8_byte u8c_exdetails[256];                                \
  struct U8_CONTOUR *u8c_outer_contour;                      \
  u8_contour_popfn u8c_popfn;				     \
  void *u8c_popdata

typedef void (*u8_contour_popfn)(void *);

/** struct U8_CONTOUR
    This representats a dynamic execution contour for use with
    setjmp/longmp exception handling
**/
typedef struct U8_CONTOUR {U8_CONTOUR_FIELDS;} U8_CONTOUR;
typedef struct U8_CONTOUR *u8_contour;

#define U8_INIT_CONTOUR(contour,label,flags,popfn,popdata)	\
  contour->u8c_label=label;					\
  contour->u8c_flags=flags|U8_CONTOUR_BASE_FLAGS;		\
  contour->u8c_popfn=popfn;					\
  contour->u8c_popdata=popdata

/* The current dynamic contour.
   This uses __thread vars if possible, TLS if not, and just a
   global variable if there aren't any pthreads.
   It is pushed onto a stack when created.*/

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_dynamic_contour_key;
#define u8_dynamic_contour ((u8_contour)(u8_tld_get(u8_dynamic_contour_key)))
#elif (U8_USE__THREAD)
U8_EXPORT __thread u8_contour u8_dynamic_contour;
#else
U8_EXPORT u8_contour u8_dynamic_contour;
#endif

/* The current lexical/static contour.
   This is bound by U8_WITHIN_CONTOUR as well as
   statically bound to NULL. */

static U8_MAYBE_UNUSED const struct U8_CONTOUR *u8_static_contour=NULL;

#define U8_WITHIN_CONTOUR \
  u8_contour u8_static_contour=u8_dynamic_contour;

/* Macros for creating dynamic contours */

#define U8_WITH_CONTOUR(label,flags)                            \
  struct U8_CONTOUR _u8_contour_struct = { 0 };			\
  struct U8_CONTOUR *_u8_contour=&_u8_contour_struct;		\
  U8_INIT_CONTOUR(_u8_contour,label,flags,NULL,NULL);		\
  if (setjmp(_u8_contour_struct.u8c_jmpbuf) == 0) {		\
  u8_push_contour(&(_u8_contour_struct));
#define U8_WITH_XCONTOUR(label,flags,popfn,popdata)		\
  struct U8_CONTOUR _u8_contour_struct = { 0 };			\
  struct U8_CONTOUR *_u8_contour=&_u8_contour_struct;		\
  U8_INIT_CONTOUR(_u8_contour,label,flags,popfn,popdata);	\
  if (setjmp(_u8_contour_struct.u8c_jmpbuf) == 0) {		\
  u8_push_contour(&(_u8_contour_struct));
#define U8_UNWIND_PROTECT U8_WITH_CONTOUR

#define U8_ON_EXCEPTION } else {             \
  if (_u8_contour->u8c_condition != NULL) {				\
    if ( (_u8_contour->u8c_flags) & (U8_CONTOUR_EXCEPTIONAL) )		\
      u8_expush(_u8_contour->u8c_exinfo.exception);			\
    else u8_seterr(_u8_contour->u8c_condition,				\
		   _u8_contour->u8c_exinfo.context,			\
		   ((_u8_contour->u8c_exdetails[0]) ?			\
		    (u8_strdup(_u8_contour->u8c_exdetails)) :		\
		    (NULL)));						\
    _u8_contour->u8c_condition=NULL;}					\
  else {}
#define U8_END_EXCEPTION                                         \
  } u8_pop_contour(&_u8_contour_struct);

#define U8_ON_UNWIND }							\
    if (_u8_contour->u8c_condition != NULL) {				\
    if ( (_u8_contour->u8c_flags) & (U8_CONTOUR_EXCEPTIONAL) )		\
      u8_expush(_u8_contour->u8c_exinfo.exception);			\
    else u8_seterr(_u8_contour->u8c_condition,				\
		   _u8_contour->u8c_exinfo.context,			\
		   ((_u8_contour->u8c_exdetails[0]) ?			\
		    (u8_strdup(_u8_contour->u8c_exdetails)) :		\
		    (NULL)));						\
    _u8_contour->u8c_condition=NULL;}
#define U8_END_UNWIND				\
  u8_pop_contour(&_u8_contour_struct);

#define U8_CLEAR_CONTOUR()                             \
  _u8_contour->u8c_flags&=(~(U8_CONTOUR_EXCEPTIONAL));

#define U8_LEAVE_CONTOUR() \
  u8_pop_contour(&_u8_contour_struct)
/* Use this when returning from a block which creates a countour. */
#define u8_return(x) ((u8_pop_contour(&_u8_contour_struct)),(x))
/* Use this when returning a malloc'd block from within. */
#define u8_return_block(x) \
  ((u8_contour_release(x)),(u8_pop_contour(&_u8_contour_struct)),(x))

/* Pushing and popping contours */

U8_EXPORT void _u8_push_contour(u8_contour);
U8_EXPORT void _u8_throw_contour(u8_contour);
U8_EXPORT void _u8_pop_contour(u8_contour);

#if U8_INLINE_CONTOURS
static void u8_push_contour(u8_contour contour)
{
  u8_contour prev=u8_dynamic_contour;

  contour->u8c_depth=((prev) ? (1+(prev->u8c_depth)) : (1));
  contour->u8c_outer_contour=prev;

#if (U8_USE_TLS)
  u8_tld_set(u8_dynamic_contour_key,contour);
#else
  u8_dynamic_contour=contour;
#endif
}
static void u8_pop_contour(u8_contour contour)
{
  u8_contour next, cur=u8_dynamic_contour;

  int i, n; void **blocks;
  if (contour==NULL) contour=cur;
  else if (contour!=cur) {
    /* This could be rewritten to be a little safer.
       If we kept a thread local vector of the last few contours,
       we could look for a valid contour and just lose some stuff. */
    u8_log(LOG_CRITICAL,u8_BadDynamicContour,"The popped contour isn't current");
    exit(1);}
  if (cur->u8c_popfn) {
    if (cur->u8c_popdata)
      (cur->u8c_popfn)(cur->u8c_popdata);
    else (cur->u8c_popfn)(cur);}
  next=contour->u8c_outer_contour;
#if (U8_USE_TLS)
  u8_tld_set(u8_dynamic_contour_key,next);
#else
  u8_dynamic_contour=next;
#endif
}
static void u8_throw_contour(u8_contour contour)
{
  if (contour==NULL) {
    u8_contour cur=u8_dynamic_contour;
    if (cur==NULL) {
      u8_log(LOG_CRIT,u8_BadDynamicContour,"No active contour");
      exit(1);}
    else contour=cur;}
  siglongjmp(contour->u8c_jmpbuf,1);
}
#else
#define u8_push_contour(ctr) _u8_push_contour(ctr)
#define u8_pop_contour(ctr) _u8_pop_contour(ctr)
#define u8_throw_contour(ctr) _u8_throw_contour(ctr)
#endif

#define u8_return_from(result,contour) \
  return ((u8_pop_contour(contour)),result)

#endif /* LIBU8_U8CONTOUR_H */
