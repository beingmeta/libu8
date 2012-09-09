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

#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

static u8_condition UnknownError=_("Unknown error");
static u8_condition EmptyExceptionStack=_("Attempt to pop an empty exception stack");

#if (U8_USE_TLS)
u8_tld_key u8_current_exception_key;
#elif (U8_USE__THREAD)
__thread u8_exception u8_current_exception;
#else
u8_exception u8_current_exception;
#endif

/* New error model */

U8_EXPORT u8_exception u8_make_exception
  (u8_condition c,u8_context cxt,u8_string details,
   void *xdata,void (*freefn)(void *))
{
  struct U8_EXCEPTION *newex=u8_alloc(struct U8_EXCEPTION);
  memset(newex,0,sizeof(struct U8_EXCEPTION));
  newex->u8x_cond=c; newex->u8x_context=cxt;
  newex->u8x_details=details;
  newex->u8x_xdata=xdata;
  newex->u8x_free_xdata=freefn;
  return newex;
}

U8_EXPORT u8_exception u8_new_exception
  (u8_condition c,u8_context cxt,u8_string details,
   void *xdata,void (*freefn)(void *))
{
  struct U8_EXCEPTION *newex=u8_alloc(struct U8_EXCEPTION);
  memset(newex,0,sizeof(struct U8_EXCEPTION));
  newex->u8x_cond=c; newex->u8x_context=cxt;
  newex->u8x_details=details;
  newex->u8x_xdata=xdata;
  newex->u8x_free_xdata=freefn;
  newex->u8x_prev=NULL;
  return newex;
}

U8_EXPORT u8_exception u8_push_exception
  (u8_condition c,u8_context cxt,u8_string details,
   void *xdata,void (*freefn)(void *))
{
  struct U8_EXCEPTION *current=u8_current_exception;
  struct U8_EXCEPTION *newex;
  if ((current) && (c==NULL)) c=current->u8x_cond;
  if (current) {
    newex=u8_make_exception(c,cxt,details,xdata,freefn);
    newex->u8x_prev=current;}
  else newex=u8_new_exception(c,cxt,details,xdata,freefn);
#if (U8_USE_TLS)
  u8_tld_set(u8_current_exception_key,newex);
#else
  u8_current_exception=newex;
#endif  
  return newex;
}

U8_EXPORT u8_exception u8_extend_exception
  (u8_context cxt,u8_string details,void *xdata,void (*freefn)(void *))
{
  struct U8_EXCEPTION *current=u8_current_exception;
  struct U8_EXCEPTION *newex;
  u8_condition c=((current) ? (current->u8x_cond) : (EmptyExceptionStack));
  newex=u8_make_exception(c,cxt,details,xdata,freefn);
  newex->u8x_prev=current;
#if (U8_USE_TLS)
  u8_tld_set(u8_current_exception_key,newex);
#else
  u8_current_exception=newex;
#endif  
  return newex;
}

U8_EXPORT u8_exception u8_free_exception(u8_exception ex,int full)
{
  u8_exception prev=ex;
  while (ex) {
    if (ex->u8x_details) u8_free(ex->u8x_details);
    if ((ex->u8x_xdata) && (ex->u8x_free_xdata))
      (ex->u8x_free_xdata)(ex->u8x_xdata);
    prev=ex->u8x_prev;
    memset(ex,0,sizeof(struct U8_EXCEPTION));
    u8_free(ex);
    if (full) ex=prev; else break;}
  return prev;
}

U8_EXPORT u8_exception u8_pop_exception()
{
  struct U8_EXCEPTION *current=u8_current_exception, *prev;
  if (current==NULL) {
    u8_log(LOG_CRIT,EmptyExceptionStack,"Popping a non existent error");
    return NULL;}
  if (current->u8x_details) u8_free(current->u8x_details);
  if ((current->u8x_xdata) && (current->u8x_free_xdata))
    (current->u8x_free_xdata)(current->u8x_xdata);
  prev=current->u8x_prev;
  memset(current,0,sizeof(struct U8_EXCEPTION));
#if (U8_USE_TLS)
  u8_tld_set(u8_current_exception_key,prev);
#else
  u8_current_exception=prev;
#endif  
  u8_free(current);
  return prev;
}

U8_EXPORT u8_exception u8_exception_root(u8_exception ex)
{
  if (ex) while (ex->u8x_prev) ex=ex->u8x_prev;
  return ex;
}

U8_EXPORT int u8_exception_stacklen(u8_exception ex)
{
  int n=0;
  while (ex) {n++; ex=ex->u8x_prev;}
  return n;
}

U8_EXPORT u8_exception u8_erreify()
{
  u8_exception ex=u8_current_exception;
#if (U8_USE_TLS)
  u8_tld_set(u8_current_exception_key,NULL);
#else
  u8_current_exception=NULL;
#endif  
  return ex;
}

U8_EXPORT u8_exception u8_restore_exception(u8_exception ex)
{
  struct U8_EXCEPTION *current=u8_current_exception;
  if (current==NULL) {
#if (U8_USE_TLS)
    u8_tld_set(u8_current_exception_key,ex);
#else
    u8_current_exception=ex;
#endif
    return ex;}
  else {
    u8_exception root=current;
    while (root->u8x_prev) root=root->u8x_prev;
    root->u8x_prev=ex;
    return current;}
}

U8_EXPORT int u8_reterr(u8_condition c,u8_context cxt,u8_string details)
{
  u8_push_exception(c,cxt,details,NULL,NULL);
  return -1;
}

U8_EXPORT u8_exception u8_errpush
   (u8_exception ex,u8_context cxt,u8_string details,
    void *xdata,void (*freefn)(void *))
{
  struct U8_EXCEPTION *newex=u8_alloc(struct U8_EXCEPTION);
  memset(newex,0,sizeof(struct U8_EXCEPTION));
  newex->u8x_cond=ex->u8x_cond; newex->u8x_context=cxt;
  newex->u8x_details=details; newex->u8x_prev=ex;
  newex->u8x_xdata=xdata; newex->u8x_free_xdata=xdata;
  return newex;
}

/* Displaying errors */

U8_EXPORT int u8_errout(u8_output out,struct U8_EXCEPTION *ex)
{
  if (ex==NULL) ex=u8_current_exception;
  if (ex==NULL) return 0;
  else if ((ex->u8x_details) && (ex->u8x_context))
    return u8_printf(out,"%m (%s): %s",
		     ex->u8x_cond,ex->u8x_context,ex->u8x_details);
  else if (ex->u8x_context)
    return u8_printf(out,"%m (%s)",ex->u8x_cond,ex->u8x_context);
  else if (ex->u8x_details)
    return u8_printf(out,"%m: %s",ex->u8x_cond,ex->u8x_details);
  else return u8_printf(out,"%m",ex->u8x_cond);
}

U8_EXPORT u8_string u8_errstring(struct U8_EXCEPTION *ex)
{
  if (ex==NULL) ex=u8_current_exception;
  if (ex==NULL) return NULL;
  else {
    U8_OUTPUT out; U8_INIT_OUTPUT(&out,32);
    if (u8_errout(&out,ex)>0) return out.u8_outbuf;
    else {
      strcpy(out.u8_outbuf,"meta error");
      return out.u8_outbuf;}}
}

U8_EXPORT
void u8_clear_errors(int report)
{
  struct U8_EXCEPTION *ex=u8_current_exception;
  while (ex) {
    if (report) {
      u8_byte buf[128]; struct U8_OUTPUT out;
      U8_INIT_OUTPUT_BUF(&out,128,buf);
      u8_errout(&out,ex);
      u8_logger(LOG_ERR,NULL,out.u8_outbuf);
      if (out.u8_streaminfo&U8_STREAM_OWNS_BUF)
	u8_free(out.u8_outbuf);}
    ex=u8_pop_exception();}
}


/* Legacy functions, should be deprecated */

U8_EXPORT void u8_seterr(u8_condition c,u8_context cxt,u8_string details)
{
  u8_push_exception(c,cxt,details,NULL,NULL);
}

U8_EXPORT int u8_geterr(u8_condition *c,u8_context *cxt,u8_string *details)
{
  struct U8_EXCEPTION *current=u8_current_exception;
  if (current) {
    if (c) *c=current->u8x_cond;
    if (cxt) *cxt=current->u8x_context;
    if (details) {
      if (current->u8x_details)
	*details=u8_strdup(current->u8x_details);
      else *details=NULL;}
    return 1;}
  else return 0;
}
U8_EXPORT int u8_poperr(u8_condition *c,u8_context *cxt,u8_string *details)
{
  int gotten=u8_geterr(c,cxt,details);
  if (gotten) u8_pop_exception();
  return gotten;
}

/* Getting strerror info */

#define U8_ERRNO_MAP_SIZE 256

static struct U8_SPARSE_ERRNO_MAP {
  int num; u8_condition condition;
  struct U8_SPARSE_ERRNO_MAP *next;} *sparse_errno_map=NULL;

static u8_condition errno_map[U8_ERRNO_MAP_SIZE];

#if (U8_THREADS_ENABLED)
static u8_mutex strerror_lock;
#endif

U8_EXPORT u8_condition u8_strerror(int num)
{
#if ((U8_THREADS_ENABLED) && (!(HAVE_STRERROR_R)))
  char *strerror_result;
#endif
  char buf[256];
  u8_condition condition;
  struct U8_SPARSE_ERRNO_MAP *scan=sparse_errno_map;
  int retval;
  if ((num<U8_ERRNO_MAP_SIZE) && (errno_map[num]))
    return errno_map[num];
  else while (scan)
    if (scan->num==num) return scan->condition;
    else scan=scan->next;
  u8_lock_mutex(&strerror_lock);
  if (num>=U8_ERRNO_MAP_SIZE) {
    /* Handle the potential race condition */
    scan=sparse_errno_map; while (scan)
      if (scan->num==num) {
	u8_unlock_mutex(&strerror_lock);
	return scan->condition;}
      else scan=scan->next;}
#if ((U8_THREADS_ENABLED) && (HAVE_STRERROR_R)) /*  */
  {
# if (STRERROR_R_CHAR_P)
    char *result=strerror_r(num,buf,256);
    if (result==NULL) condition=UnknownError;
    else condition=(u8_condition) u8_fromlibc(result);
#else
    if ((retval=strerror_r(num,buf,256))) condition=UnknownError;
    else condition=(u8_condition) u8_fromlibc(buf);
#endif
  }
#elif U8_THREADS_ENABLED
  strerror_result=strerror(num);
  condition=(u8_condition)u8_fromlibc(strerror_result);
#endif  
  if (num<U8_ERRNO_MAP_SIZE) errno_map[num]=condition;
  else {
    struct U8_SPARSE_ERRNO_MAP *newmap=u8_alloc(struct U8_SPARSE_ERRNO_MAP);
    newmap->num=num; newmap->condition=condition;
    newmap->next=sparse_errno_map;
    sparse_errno_map=newmap;}
  u8_unlock_mutex(&strerror_lock);
  return condition;
}

U8_EXPORT void u8_graberr(int num,u8_context cxt,u8_string details)
{
  if (num<0) {num=errno; errno=0;}
  u8_seterr(u8_strerror(num),cxt,details);
}

U8_EXPORT void u8_graberrno(u8_context cxt,u8_string details)
{
  if (errno) {
    u8_graberr(errno,cxt,details);
    errno=0;}
}


/* Initialization */

U8_EXPORT void u8_init_exceptions_c()
{
  int i=0; while (i<U8_ERRNO_MAP_SIZE) errno_map[i++]=NULL;

#if ((U8_THREADS_ENABLED) && (U8_USE_TLS))
  u8_new_threadkey(&u8_current_exception_key,NULL);
#endif
  
#if ((U8_THREADS_ENABLED) && (!(HAVE_STRERROR_R)))
  u8_init_mutex(&strerror_lock);
#endif

  u8_register_source_file(_FILEINFO);
}

