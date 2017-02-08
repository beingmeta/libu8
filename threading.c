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

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8elapsed.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#if ((HAVE_SYS_SYSCALL_H)&&(HAVE_SYSCALL))
#include <sys/syscall.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#if HAVE_PTHREAD_H
#include <pthread.h>
static pthread_mutexattr_t u8_default_mutex_attr;
static pthread_mutexattr_t u8_recursive_mutex_attr;
#endif

#ifndef MAX_THREADINITFNS
#define MAX_THREADINITFNS 128
#endif
#ifndef MAX_THREAEXITFNS
#define MAX_THREADEXITFNS 128
#endif

int u8_thread_log_enabled=1;
int u8_thread_debug_loglevel=LOGDEBUG;
int u8_thread_debug_showid=1;
u8_mutex_fn u8_mutex_tracefn=NULL;
u8_rwlock_fn u8_rwlock_tracefn=NULL;

#if (U8_USE_TLS)
u8_tld_key u8_stack_base_key;
u8_tld_key u8_stack_size_key;
#elif (U8_USE__THREAD)
__thread void *u8_stack_base=NULL;
__thread ssize_t u8_stack_size=0;
#else
void *u8_stack_base=NULL;
ssize_t u8_stack_size=0;
#endif

#define ULL(i) ((unsigned long long)(i))

static u8_string ll2str(long long n,u8_byte *buf,size_t len)
{
  strcpy(buf,"Tx");
  u8_write_long_long(n,buf+2,len);
  return buf;
}
#define thid() \
  ((u8_thread_debug_showid) ? (u8_threadid()) : (-1))
#define thidstr(n,buf) (ll2str(n,buf,sizeof(buf)))

/* Getting a thread ID */

#if ((HAVE_PTHREAD_THREADID_NP)&&(HAVE_PTHREAD_SELF))
U8_EXPORT long long u8_threadid()
{
  long long tid;
  pthread_t self=pthread_self();
  pthread_threadid_np(self,&tid);
  return tid;
}
#elif ((HAVE_SYS_SYSCALL_H)&&(HAVE_SYSCALL))
U8_EXPORT long long u8_threadid()
{
  pid_t tid=syscall(SYS_gettid);
  return (long long) tid;
}
#elif (HAVE_PTHREAD_SELF)
U8_EXPORT long long u8_threadid()
{
  pthread_t self=pthread_self();
  return (unsigned long long int)self;
}
#elif (HAVE_GETPID) /* No threads, not really */
U8_EXPORT long long u8_threadid()
{
  return (long long) getpid();
}
#else
U8_EXPORT long long u8_threadid()
{
  return -1;
}
#endif

#if (HAVE_GETPID)
U8_EXPORT char *u8_procinfo(char *buf)
{
  pid_t pid=getpid(); long long tid=u8_threadid();
  if (!(buf)) buf=u8_mallocz(128);
  sprintf(buf,"%ld:%lld",(unsigned long int)pid,(unsigned long long)tid);
  return buf;
}
#else
U8_EXPORT char *u8_procinfo(char *buf)
{
  if (buf) {
    strcpy(buf,"no procinfo");
    return buf;}
  else return u8_strdup("no procinfo");
}
#endif

/* POSIX Mutexes */

#if HAVE_PTHREAD_H

U8_EXPORT int _u8_init_mutex(u8_mutex *mutex)
{
  return pthread_mutex_init(mutex,&u8_default_mutex_attr);
}

U8_EXPORT int _u8_init_recursive_mutex(u8_mutex *mutex)
{
  return pthread_mutex_init(mutex,&u8_recursive_mutex_attr);
}

U8_EXPORT int u8_mutex_wait(pthread_mutex_t *mutex)
{
  u8_mutex_fn tracefn=u8_mutex_tracefn;
  double started=u8_elapsed_time();
  if (tracefn) tracefn(mutex,u8_dbg_lock_req,started);
  int rv=pthread_mutex_lock(mutex);
  double finished=u8_elapsed_time();
  long long tid=thid(); u8_byte buf[128];
  if (tracefn) tracefn(mutex,u8_dbg_lock,finished-started);
  if (u8_thread_log_enabled) {
    if (rv)
      u8_log(u8_thread_debug_loglevel,"MutexWait",
	     "Waited %fs for error %d on mutex 0x%llx\t(t=%f) %s",
	     finished-started,rv,ULL(mutex),started,thidstr(tid,buf));
    else u8_log(u8_thread_debug_loglevel,"MutexWait",
		"Waited %fs for mutex 0x%llx\t(t=%f) %s",
		finished-started,ULL(mutex),started,thidstr(tid,buf));}
  return rv;
}

U8_EXPORT int u8_mutex_release(u8_mutex *mutex)
{
  u8_mutex_fn tracefn=u8_mutex_tracefn;
  double now=u8_elapsed_time();
  if (tracefn) tracefn(mutex,u8_dbg_unlock,now);
  int rv=pthread_mutex_unlock(mutex);
  long long tid=thid(); u8_byte buf[128];
  if (u8_thread_log_enabled) {
    if (rv)
      u8_log(u8_thread_debug_loglevel,"MutexRelease",
	     "Got error %d releasing mutex 0x%llx\t(t=%f) %s",
	     rv,ULL(mutex),now,thidstr(tid,buf));
    else u8_log(u8_thread_debug_loglevel,"MutexRelease",
		"Releasing mutex 0x%llx\t(t=%f) %s",
		ULL(mutex),now,thidstr(tid,buf));}
  return rv;
}

U8_EXPORT int u8_rwlock_wait(u8_rwlock *rwlock,int write)
{
  u8_rwlock_fn tracefn=u8_rwlock_tracefn;
  double started=u8_elapsed_time();
  if (tracefn)
    tracefn(rwlock,((write)?(u8_dbg_write_req):(u8_dbg_read_req)),started);
#if HAVE_PTHREAD_RWLOCK_INIT
  int rv= (write) ? (pthread_rwlock_wrlock(rwlock)) :
    (pthread_rwlock_rdlock(rwlock));
#else
  int rv=pthread_mutex_lock(mutex);
#endif
  double finished=u8_elapsed_time();
  long long tid=thid(); u8_byte buf[128];
  if (tracefn)
    tracefn(rwlock,((write)?(u8_dbg_wrlock):(u8_dbg_rdlock)),
	    (finished-started));
  if (u8_thread_log_enabled) {
    if (rv)
      u8_log(u8_thread_debug_loglevel,"RWLockWait",
	     "Waited %fs for error %d on %s lock 0x%llx\t(t=%f) %s",
	     finished-started,rv,((write)?("write"):("read")),
	     ULL(rwlock),started,thidstr(tid,buf));
    else u8_log(u8_thread_debug_loglevel,"RWLockWait",
		"Waited %fs for %s 0x%llx",finished-started,
		((write)?("write"):("read")),ULL(rwlock),
		started,thidstr(tid,buf));}
  return rv;
}

U8_EXPORT int u8_rwlock_release(u8_rwlock *rwlock)
{
  u8_rwlock_fn tracefn=u8_rwlock_tracefn;
  double now=u8_elapsed_time();
  if (tracefn) tracefn(rwlock,u8_dbg_rwunlock,now);
  int rv=pthread_rwlock_unlock(rwlock);
  long long tid=thid(); u8_byte buf[128];
  if (u8_thread_log_enabled) {
    if (rv)
      u8_log(u8_thread_debug_loglevel,"RWUnlock/ERR",
	     "Got error %d unlocking read/write lock 0x%llx\t(t=%f) %s",
	     rv,ULL(rwlock),now,thidstr(tid,buf));
    else u8_log(u8_thread_debug_loglevel,"RWUnlock",
		"Unlocked read/write lock  0x%llx\t(t=%f) %s",
		ULL(rwlock),thidstr(tid,buf));}
  return rv;
}

U8_EXPORT int u8_mutex_lock_dbg(u8_mutex *mutex)
{
  int rv=pthread_mutex_trylock(mutex);
  if (rv) return u8_mutex_wait(mutex);
  else return rv;
}

U8_EXPORT int u8_mutex_unlock_dbg(u8_mutex *mutex)
{
  return pthread_mutex_unlock(mutex);
}

#if HAVE_PTHREAD_RWLOCK_INIT

U8_EXPORT int u8_rwlock_wrlock_dbg(u8_rwlock *rwlock)
{
  int rv=pthread_rwlock_trywrlock(rwlock);
  if (rv) return u8_rwlock_wait(rwlock,1);
  else return rv;
}
U8_EXPORT int u8_rwlock_rdlock_dbg(u8_rwlock *rwlock)
{
  int rv=pthread_rwlock_tryrdlock(rwlock);
  if (rv) return u8_rwlock_wait(rwlock,0);
  else return rv;
}

U8_EXPORT int u8_rwlock_unlock_dbg(u8_rwlock *rwlock)
{
  return pthread_rwlock_unlock(rwlock);
}

#else /* HAVE_PTHREAD_RWLOCK_INIT */

U8_EXPORT int u8_rwlock_wrlock_dbg(u8_rwlock *rwlock)
{
  int rv=pthread_mutex_trylock(rwlock);
  if (rv) return u8_pthread_rwlock_wait(rwlock,1);
  else return rv;
}

U8_EXPORT int u8_rwlock_rdlock_dbg(u8_rwlock *rwlock)
{
  int rv=pthread_mutex_trylock(rwlock);
  if (rv) return u8_pthread_rwlock_wait(rwlock,0);
  else return rv;
}

U8_EXPORT int u8_rwlock_unlock_dbg(u8_rwlock *rwlock)
{
  return pthread_mutex_unlock(rwlock);
}

#endif /* not HAVE_PTHREAD_RWLOCK_INIT */

#endif /* HAVE_PTHREAD_H */

/* Thread initialization */

int u8_n_threadinits=0;
static int n_threadexitfns=0;
static u8_threadexitfn threadexitfns[MAX_THREADEXITFNS];
static u8_threadinitfn threadinitfns[MAX_THREADINITFNS];
static u8_mutex threadinitfns_lock;

#if (U8_USE_TLS)
u8_tld_key u8_initlevel_key;
#elif (U8_USE__THREAD)
__thread int u8_initlevel=0;
#else
int u8_initlevel=0;
#endif

U8_EXPORT int u8_threadexit(void);

/* Stack info */

U8_EXPORT void u8_stackinit()
{
#if (U8_USE_TLS)
  if (u8_tld_get(_u8_stack_base_key)) return;
#else
  if (u8_stack_base) return;
#endif
  pthread_t self=pthread_self();
#if ( (HAVE_PTHREAD_GET_STACKADDR_NP) && (HAVE_PTHREAD_GET_STACKSIZE_NP) )
  void *stackbase=pthread_get_stackaddr_np(self);
  ssize_t stacksize=pthread_get_stacksize_np(self);
#elif ( (HAVE_PTHREAD_GET_ATTR_NP) && (HAVE_PTHREAD_ATTR_GET_STACK_NP) )
  void *stackbase; ssize_t stacksize;
  pthread_attr_t attr;
  pthread_get_attr_np(self,&attr);
  pthread_attr_getstack(&attr,&stackbase,&stacksize);
#else
  void *stackbase=&self; ssize_t stacksize=0;
#endif

#if (U8_USE_TLS)
  u8_tld_set(_u8_stack_base_key,stackbase);
  u8_tld_set(_u8_stack_size_key,stacksize);
#else
  u8_stack_base=stackbase;
  u8_stack_size=stacksize;
#endif
}

U8_EXPORT int u8_threadinit()
{
  u8_stackinit();
  return u8_run_threadinits();
}

/* Thread inits */

U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn)
{
  int i=0;
  u8_lock_mutex(&threadinitfns_lock);
  while (i<u8_n_threadinits)
    if (threadinitfns[i]==fn) {
      u8_unlock_mutex(&threadinitfns_lock);
      return 0;}
    else i++;
  if (i<MAX_THREADINITFNS) {
    threadinitfns[i]=fn; u8_n_threadinits++;
    u8_unlock_mutex(&threadinitfns_lock);
    return 1;}
  else {
    u8_seterr(_("Too many thread init fns"),"u8_register_threadinit",NULL);
    u8_unlock_mutex(&threadinitfns_lock);
    return -1;}
}

U8_EXPORT int u8_run_threadinits()
{
  u8_wideint start=u8_getinitlevel(), n=u8_n_threadinits, errs=0, i=start;
  while (i<n) {
    int retval=threadinitfns[i]();
    if (retval<0) {
      u8_log(LOG_CRIT,"THREADINIT","Thread init function (%d) failed",i);
      errs++;}
    i++;}
  u8_setinitlevel(n);
  if (errs) return -errs;
  else return n-start;
}
U8_EXPORT int u8_threadexit()
{
  int i=0, n=n_threadexitfns;
  while (i<n) {
    threadexitfns[i]();
    i++;}
  return i;
}

U8_EXPORT int u8_register_threadexit(u8_threadexitfn fn)
{
  int i=0;
  u8_lock_mutex(&threadinitfns_lock);
  while (i<n_threadexitfns)
    if ((threadexitfns[i])==fn) {
      u8_unlock_mutex(&threadinitfns_lock);
      return 0;}
    else i++;
  if (i<MAX_THREADEXITFNS) {
    threadexitfns[i]=fn; n_threadexitfns++;
    u8_unlock_mutex(&threadinitfns_lock);
    return 1;}
  else {
    u8_seterr(_("Too many thread exit fns"),"u8_register_threadexit",NULL);
    u8_unlock_mutex(&threadinitfns_lock);
    return -1;}
}

#if HAVE_PTHREAD_H

static void threadexit_atexit()
{
  u8_threadexit();
}

static time_t threading_c_init=0;

U8_EXPORT void u8_init_threading_c(void)
{
  if (threading_c_init) return;
  threading_c_init=time(NULL);

  memset(&u8_default_mutex_attr,0,sizeof(u8_default_mutex_attr));
  memset(&u8_recursive_mutex_attr,0,sizeof(u8_recursive_mutex_attr));

#if HAVE_PTHREAD_MUTEXATTR_INIT
  pthread_mutexattr_init(&u8_default_mutex_attr);
  pthread_mutexattr_init(&u8_recursive_mutex_attr);
#endif

#if HAVE_PTHREAD_MUTEXATTR_SETTYPE
  pthread_mutexattr_settype(&u8_default_mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutexattr_settype(&u8_recursive_mutex_attr,PTHREAD_MUTEX_RECURSIVE);
#endif

  u8_init_mutex(&threadinitfns_lock);

#if (U8_USE_TLS)
  u8_new_threadkey(&u8_initlevel_key,NULL);
  u8_new_threadkey(&u8_stack_base_key,NULL);
  u8_new_threadkey(&u8_stack_size_key,NULL);
#endif

  atexit(threadexit_atexit);
}

#else /* not HAVE_PTHREAD_H */

U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn)
{
  return fn();
}
U8_EXPORT int u8_run_threadinits()
{
  return 0;
}

/* Functions for mutex operations for cases where the pthread
   functions are overriden */
U8_EXPORT void u8_mutex_lock(u8_mutex *m)
{
  u8_lock_mutex(m);
}
U8_EXPORT void u8_mutex_unlock(u8_mutex *m)
{
  u8_unlock_mutex(m);
}
U8_EXPORT void u8_mutex_init(u8_mutex *m)
{
  u8_unlock_mutex(m);
}
U8_EXPORT void u8_mutex_destroy(u8_mutex *m)
{
  u8_unlock_mutex(m);
}

U8_EXPORT void u8_init_threading_c(void)
{
}

#endif /* else HAVE_PTHREAD */
