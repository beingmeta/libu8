/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.

*/

#include "libu8/config.h"
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

ssize_t u8_assumed_stacksize=U8_ASSUMED_STACKSIZE;
int u8_stack_direction=0;

int u8_thread_log_enabled=1;
int u8_thread_debug_loglevel=LOGDEBUG;
int u8_thread_debug_showid=1;
u8_mutex_fn u8_mutex_tracefn=NULL;
u8_rwlock_fn u8_rwlock_tracefn=NULL;

#define ULL(i) ((unsigned long long)((u8_ptrbits)i))

static u8_string ll2str(long long n,u8_byte *buf,size_t len)
{
  strcpy(buf,"Tx");
  u8_write_long_long(n,buf+2,len);
  return buf;
}
#define thid() \
  ((u8_thread_debug_showid) ? (u8_threadid()) : (-1))
#define thidstr(n,buf) (ll2str(n,buf,sizeof(buf)))

/* Aliases */

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
  u8_init_mutex(m);
}
U8_EXPORT void u8_mutex_destroy(u8_mutex *m)
{
  u8_destroy_mutex(m);
}

/* Getting a thread ID */

#if ( (HAVE_PTHREAD_THREADID_NP) && (HAVE_PTHREAD_SELF) )
U8_EXPORT long long u8_threadid()
{
  long long tid;
  pthread_t self=pthread_self();
  pthread_threadid_np(self,&tid);
  return tid;
}
#elif ( (HAVE_SYS_SYSCALL_H) && (HAVE_SYSCALL) )
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
#elif (HAVE_GETPID)
/* No threads, not really, but we'll call the process a thread */
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
  pid_t pid=getpid();
  long long tid=u8_threadid();
  char pidbuf[64], tidbuf[64];
  u8_write_long_long((long long) pid,pidbuf,64);
  u8_write_long_long((long long) tid,tidbuf,64);
  if (!(buf)) buf=u8_zmalloc(128);
  strcpy(buf,pidbuf); strcat(buf,":"); strcat(buf,tidbuf);
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
/* Use regular mutexes as read/write locks */

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

#else /* not HAVE_PTHREAD_H */
/* These functions are current all defined by do-nothing macros if you
   don't have pthreads. */
#endif /* HAVE_PTHREAD_H */

/* Thread initialization */

int u8_n_threadinits=0;
static int n_threadexitfns=0;
static u8_threadexitfn threadexitfns[MAX_THREADEXITFNS];
static u8_threadinitfn threadinitfns[MAX_THREADINITFNS];
static u8_mutex threadinitfns_lock;
static u8_mutex threadexitfns_lock;

#if (U8_USE_TLS)
u8_tld_key u8_initlevel_key;
#elif (U8_USE__THREAD)
__thread int u8_initlevel=0;
#else
int u8_initlevel=0;
#endif

U8_EXPORT int u8_threadexit(void);

U8_EXPORT int u8_threadinit()
{
  u8_init_stack();
  return u8_run_threadinits();
}

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
  u8_init_stack();
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
  u8_lock_mutex(&threadexitfns_lock);
  while (i<n_threadexitfns)
    if ((threadexitfns[i])==fn) {
      u8_unlock_mutex(&threadexitfns_lock);
      return 0;}
    else i++;
  if (i<MAX_THREADEXITFNS) {
    threadexitfns[i]=fn;
    n_threadexitfns++;
    u8_unlock_mutex(&threadexitfns_lock);
    return 1;}
  else {
    u8_seterr(_("Too many thread exit fns"),"u8_register_threadexit",NULL);
    u8_unlock_mutex(&threadexitfns_lock);
    return -1;}
}

/* Stack info */

#if (U8_USE_TLS)
u8_tld_key u8_stack_base_key;
u8_tld_key u8_stack_size_key;
#elif (U8_USE__THREAD)
__thread void *u8_stack_base=NULL;
__thread ssize_t u8_stack_size=0;
#else /* Just shared variables, no threads */
void *u8_stack_base=NULL;
ssize_t u8_stack_size=0;
#endif

#if ( HAVE_PTHREAD_GETATTR_NP && HAVE_PTHREAD_ATTR_GETSTACK )
U8_EXPORT void u8_init_stack()
{
  if (u8_stack_size) return;
  pthread_t self=pthread_self();
  ssize_t stacksize=-1;
  pthread_attr_t attr;
  U8_SET_STACK_BASE();
  pthread_getattr_np(self,&attr);
  pthread_attr_getstacksize(&attr,&stacksize);
  pthread_attr_destroy(&attr);
  if (stacksize<=0) {
    u8_log(LOGWARN,"CantGetStackSize",
	   "Can't get stacksize for libu8 initialization, assuming %lld",
	   u8_assumed_stacksize);
    stacksize=u8_assumed_stacksize;}
  u8_set_stack_size(stacksize);
}
#elif HAVE_PTHREAD_GET_STACKADDR_NP
U8_EXPORT void u8_init_stack()
{
  if (u8_stack_size) return;
  pthread_t self=pthread_self();
  ssize_t stacksize=pthread_get_stacksize_np(self);
  U8_SET_STACK_BASE();
  if (stacksize<=0) {
    u8_log(LOGWARN,"CantGetStackSize",
	   "Can't get stacksize for libu8 initialization, assuming %lld",
	   u8_assumed_stacksize);
    stacksize=u8_assumed_stacksize;}
  u8_set_stack_size(stacksize);
}
#else
U8_EXPORT void u8_init_stack()
{
  if (u8_stack_size) return;
  else {
    U8_SET_STACK_BASE();
    u8_set_stack_size(u8_assumed_stacksize);}
}
#endif

U8_EXPORT ssize_t u8_stacksize()
{
  return u8_stack_size;
}
U8_EXPORT void *u8_stackbase()
{
  return u8_stack_base;
}

static int get_stack_direction_inner(int *outerp)
{
  int inner, *innerp=&inner;
  if (innerp>outerp) return 1;
  else if (outerp>innerp) return -1;
  else return 0;
}

static int get_stack_direction()
{
  int outer;
  return get_stack_direction_inner(&outer);
}

/* Initialization code */

#if HAVE_PTHREAD_H

static time_t threading_initialized=0;
static void threadexit_atexit(void);

U8_EXPORT void u8_initialize_threading(void)
{
  if (threading_initialized) return;
  threading_initialized=time(NULL);

  u8_stack_direction=get_stack_direction();
  if (u8_stack_direction==0) 
    u8_log(LOGCRIT,"NoStackDirection","Couldnt' determine stack direction");

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
  u8_init_mutex(&threadexitfns_lock);

#if (U8_USE_TLS)
  u8_new_threadkey(&u8_initlevel_key,NULL);
  u8_new_threadkey(&u8_stack_base_key,NULL);
  u8_new_threadkey(&u8_stack_size_key,NULL);
#endif

  atexit(threadexit_atexit);
}

static void threadexit_atexit()
{
  u8_threadexit();
}

#else /* not HAVE_PTHREAD_H */

U8_EXPORT void u8_initialize_threading(void)
{
  if (threading_initialized) return;
  threading_initialized=time(NULL);

  u8_stack_direction=get_stack_direction();
  if (u8_stack_direction==0) 
    u8_log(LOGCRIT,"NoStackDirection","Couldnt' determine stack direction");

  atexit(threadexit_atexit);
}

U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn)
{
  return fn();
}
U8_EXPORT int u8_run_threadinits()
{
  return 0;
}

#endif

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
