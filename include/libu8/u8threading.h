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

#ifndef LIBU8_THREADING_H
#define LIBU8_THREADING_H 1
#define LIBU8_THREADING_H_VERSION __FILE__

#ifndef U8_THREAD_DEBUG
#define U8_THREAD_DEBUG 0
#endif

/* Debug functions */

U8_EXPORT int u8_mutex_lock_dbg(pthread_mutex_t *mutex);
U8_EXPORT int u8_mutex_unlock_dbg(pthread_mutex_t *mutex);
U8_EXPORT int u8_rwlock_wrlock_dbg(pthread_rwlock_t *rwlock);
U8_EXPORT int u8_rwlock_rdlock_dbg(pthread_rwlock_t *rwlock);
U8_EXPORT int u8_rwlock_unlock_dbg(pthread_rwlock_t *rwlock);

typedef enum {
  u8_dbg_lock_req, u8_dbg_lock, u8_dbg_unlock,
  u8_dbg_read_req, u8_dbg_write_req,
  u8_dbg_rdlock, u8_dbg_wrlock,
  u8_dbg_rwunlock }
  u8_lockop;
typedef int (*u8_mutex_fn)(pthread_mutex_t *,u8_lockop,double);
typedef int (*u8_rwlock_fn)(pthread_rwlock_t *,u8_lockop,double);

U8_EXPORT int u8_thread_log_enabled;
U8_EXPORT int u8_thread_debug_loglevel;
U8_EXPORT u8_mutex_fn u8_mutex_tracefn;
U8_EXPORT u8_rwlock_fn u8_rwlock_tracefn;

/* Portable threading */

#if (HAVE_PTHREAD_H)
#define U8_THREADS_ENABLED 1
#include <pthread.h>
#define U8_MUTEX_DECL(var) u8_mutex var
#define U8_RWLOCK_DECL(var) u8_rwlock var
typedef pthread_mutex_t u8_mutex;
typedef pthread_cond_t u8_condvar;
typedef pthread_key_t u8_tld_key;
U8_EXPORT int _u8_init_mutex(u8_mutex *);
U8_EXPORT int _u8_init_recursive_mutex(u8_mutex *);
#define pthread_attr_default NULL
#define u8_init_mutex(x) _u8_init_mutex(x)
#define u8_init_recursive_mutex(x) _u8_init_recursive_mutex(x)
#define u8_destroy_mutex(x) pthread_mutex_destroy(x)
#define u8_new_threadkey(key_loc,del_fcn) pthread_key_create(key_loc,del_fcn)
#define u8_init_condvar(x) pthread_cond_init(x,NULL)
#define u8_destroy_condvar(x) pthread_cond_destroy(x)
#define u8_condvar_wait(cv,lck) pthread_cond_wait(cv,lck)
#define u8_condvar_timedwait(cv,lck,tm) pthread_cond_timedwait(cv,lck,tm)
#define u8_condvar_signal(cv) pthread_cond_signal(cv)
#define u8_condvar_broadcast(cv) pthread_cond_broadcast(cv)

#define _u8_lock_mutex(x) pthread_mutex_lock((x))
#define _u8_unlock_mutex(x) pthread_mutex_unlock((x))

#if U8_THREAD_DEBUG
#define u8_lock_mutex(x) u8_mutex_lock_dbg((x))
#define u8_unlock_mutex(x) u8_mutex_unlock_dbg((x))
#else
#define u8_lock_mutex(x) pthread_mutex_lock((x))
#define u8_unlock_mutex(x) pthread_mutex_unlock((x))
#endif

#define u8_tld_get(key) pthread_getspecific(key)
#define u8_tld_set(key,v) pthread_setspecific(key,v)

/* Read/write locks are replaced with simple mutexes if
   not available natively. */
#if HAVE_PTHREAD_RWLOCK_INIT
typedef pthread_rwlock_t u8_rwlock;
#define u8_init_rwlock(x) pthread_rwlock_init(x,0)
#define u8_destroy_rwlock(x) pthread_rwlock_destroy(x)

#define _u8_read_lock(x) pthread_rwlock_rdlock((x))
#define _u8_write_lock(x) pthread_rwlock_wrlock((x))
#define _u8_rw_unlock(x) pthread_rwlock_unlock((x))

#if U8_THREAD_DEBUG
#define u8_read_lock(x) u8_rwlock_rdlock_dbg((x))
#define u8_write_lock(x) u8_rwlock_wrlock_dbg((x))
#define u8_rw_unlock(x) u8_rwlock_unlock_dbg((x))
#else
#define u8_read_lock(x) pthread_rwlock_rdlock((x))
#define u8_write_lock(x) pthread_rwlock_wrlock((x))
#define u8_rw_unlock(x) pthread_rwlock_unlock((x))
#endif

#else /* HAVE_PTHREAD_RWLOCK_INIT */

typedef pthread_mutex_t u8_rwlock;
#define u8_init_rwlock(x) pthread_mutex_init(x,0)
#define u8_destroy_rwlock(x) pthread_mutex_destroy(x)

#define _u8_read_lock(x) pthread_mutex_lock((x))
#define _u8_write_lock(x) pthread_mutex_lock((x))
#define _u8_rw_unlock(x) pthread_mutex_unlock((x))

#if U8_THREAD_DEBUG
#define u8_read_lock(x) u8_rwlock_rdlock_dbg((x))
#define u8_write_lock(x) u8_rwlock_wrlock_dbg((x))
#define u8_rw_unlock(x) u8_rwlock_unlock_dbg((x))
#else
#define u8_read_lock(x) pthread_mutex_lock((x))
#define u8_write_lock(x) pthread_mutex_lock((x))
#define u8_rw_unlock(x) pthread_mutex_unlock((x))
#endif

#endif /* else HAVE_PTHREAD_RWLOCK_INIT */

#elif WIN32
#define U8_THREADS_ENABLED 1
#define U8_MUTEX_DECL(var) HANDLE var
#define U8_RWLOCK_DECL(var) HANDLE var
typedef HANDLE pthread_mutex_t;
typedef HANDLE u8_mutex;
typedef HANDLE pthread_cond_t;
typedef HANDLE u8_condvar;
typedef HANDLE u8_rwlock;

#define pthread_attr_default NULL
#define pthread_mutex_lock(x)  (WaitForSingleObject((*(x)),INFINITE))
#define pthread_mutex_unlock(x) ReleaseMutex(*(x))
#define pthread_mutex_init(_mloc,attr) (*(_mloc))=CreateMutex(NULL,FALSE,NULL)
#define pthread_mutex_destroy(_mloc) CloseHandle((*(_mloc)))

#define pthread_cond_init(_cloc,attr) (*(_cloc))=CreateEvent(NULL,FALSE,FALSE,NULL)
#define pthread_cond_signal(_x)  (SetEvent((*(_x))))
#define pthread_cond_wait(_c,_m)  (ReleaseMutex(*(_m)),(WaitForSingleObject((*(_c)),INFINITE)))

#define pthread_join(_thread,_ignored) WaitForSingleObject(_thread,INFINITE)

typedef HANDLE pthread_t;

static DWORD _thread_tmp;

#define pthread_create(ptid,ignored,fcn,arg) \
   (*(ptid))=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)fcn,arg,0,&_thread_tmp)

typedef DWORD u8_tld_key;
#define u8_new_threadkey(lvalue,ignored) (*(lvalue))=TlsAlloc();
#define u8_tld_get(key) TlsGetValue(key)
#define u8_tld_set(key,v) TlsSetValue(key,v)

#define u8_init_mutex(_mloc) \
   (*(_mloc))=CreateMutex(NULL,FALSE,NULL)
#define u8_init_recursive_mutex(_mloc) \
   (*(_mloc))=CreateMutex(NULL,FALSE,NULL)
#define u8_init_rwlock(_mloc) \
   (*(_mloc))=CreateMutex(NULL,FALSE,NULL)
#define u8_destroy_mutex(x) pthread_mutex_destroy(x)
#define u8_destroy_rwlock(x) pthread_mutex_destroy(x)
#define u8_lock_mutex(x) pthread_mutex_lock((x))
#define u8_unlock_mutex(x) pthread_mutex_unlock((x))
#define u8_read_lock(x) pthread_mutex_lock((x))
#define u8_write_lock(x) pthread_mutex_lock((x))
#define u8_rw_unlock(x) pthread_mutex_unlock((x))
#else
#define U8_THREADS_ENABLED 0
#define U8_MUTEX_DECL(var)
#define u8_init_mutex(x)
#define u8_init_recursive_mutex(x)
#define u8_init_rwlock(x)
#define u8_destroy_mutex(x)
#define u8_destroy_rwlock(x)
#define u8_new_threadkey(key_loc,del_fcn)

#define u8_lock_mutex(x)
#define u8_unlock_mutex(x)
#define u8_read_lock(x)
#define u8_write_lock(x)
#define u8_rw_unlock(x)
#endif

#if U8_THREADS_ENABLED
#if HAVE_THREAD_STORAGE_CLASS
U8_EXPORT __thread void *u8_stack_base;
#define u8_stack_base() (u8_stack_base)
#define U8_SET_STACK_BASE()	  \
  volatile int _stack_base=17; \
  u8_stack_base=(void *)&_stack_base
static U8_MAYBE_UNUSED ssize_t u8_stack_depth()
{
  int _stackval=42;
  if (u8_stack_base==NULL) return -1;
  else {
    ssize_t diff=((void *)&(_stackval))-u8_stack_base;
    if (diff<0) return -diff; else return diff;}
}
#else
U8_EXPORT u8_tld_key u8_stack_base_key;
#define u8_stack_base() (u8_tld_get(u8_stack_base_key))
#define U8_SET_STACK_BASE()	  \
  volatile int _stack_base=17*42; \
  u8_tld_set(u8_stack_base_key,(void *)&_stack_base)
static U8_MAYBE_UNUSED ssize_t u8_stack_depth()
{
  int _stackval=42; 
  void *stack_base=u8_tld_get(u8_stack_base_key);
  if (stack_base==NULL) return -1;
  else {
    ssize_t diff=((void *)&(_stackval))-stack_base;
    if (diff<0) return -diff; else return diff;}
}
#endif
#else /* not U8_THREADS_ENABLED */
#define u8_stack_base() ((void *)(NULL))
#define U8_SET_STACK_BASE()
static U8_MAYBE_UNUSED ssize_t u8_stack_depth()
{
  return -1;
}
#endif

/* Thread proxy functions */

/** Locks a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_lock(u8_mutex *m);

/** Unlocks a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_unlock(u8_mutex *m);

/** Initialize a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_init(u8_mutex *m);

/** Destroys a POSIX thread mutex
    @param m a pointer to a mutex
    @returns void
**/
U8_EXPORT void u8_mutex_destroy(u8_mutex *m);

/* Thread init functions */

typedef int (*u8_threadinitfn)(void);
typedef void (*u8_threadexitfn)(void);
U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn);
U8_EXPORT int u8_register_threadexit(u8_threadexitfn fn);

U8_EXPORT void u8_stackinit(void);

U8_EXPORT int u8_threadinit(void);
U8_EXPORT int u8_threadexit(void);
U8_EXPORT int u8_run_threadinits(void);

U8_EXPORT int u8_n_threadinits;
U8_EXPORT int u8_n_threadexitfns;

#define u8_threadcheck() \
  if (u8_getinitlevel()<u8_n_threadinits) u8_run_threadinits()

#if (U8_USE__THREAD)
U8_EXPORT __thread int u8_initlevel;
#define u8_getinitlevel() (u8_initlevel)
#define u8_setinitlevel(n) u8_initlevel=(n);
#elif (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_initlevel_key;
#define u8_getinitlevel() ((int) ((u8_wideint)(u8_tld_get(u8_initlevel_key))))
#define u8_setinitlevel(n) u8_tld_set(u8_initlevel_key,((void *)n))
#else
U8_EXPORT int u8_initlevel;
#define u8_getinitlevel() (u8_initlevel)
#define u8_setinitlevel(n) u8_initlevel=(n);
#endif

/** Returns a long identifying the current thread
    @returns long a numeric thread identifier (OS dependent)
**/
U8_EXPORT long long u8_threadid(void);

/* Stack info */

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_stack_base_key;
U8_EXPORT u8_tld_key u8_stack_size_key;
#else
U8_EXPORT __thread void *u8_stack_base;
U8_EXPORT __thread ssize_t u8_stack_size;
#endif

#if (U8_USE_TLS)
#define u8_stackbase() \
  ((u8_tld_get(u8_stack_base_key)) || \
   (u8_stackinit(),u8_tld_get(u8_stack_base_key)))
#define u8_stacksize() \
  ((u8_tld_get(u8_stack_size_key)) || \
   (u8_stackinit(),u8_tld_get(u8_stack_size_key)))
#else
#define u8_stackbase() \
  ((u8_stack_base) || (u8_stackinit(),u8_stack_base))
#define u8_stacksize() \
  ((u8_stack_size) || (u8_stackinit(),u8_stack_size))
#endif

/* Trace functions */

U8_EXPORT int u8_mutex_wait(u8_mutex *mutex);
U8_EXPORT int u8_mutex_release(u8_mutex *mutex);
U8_EXPORT int u8_rwlock_wait(u8_rwlock *mutex,int write);
U8_EXPORT int u8_rwlock_release(u8_rwlock *mutex);

#endif /* LIBU8_THREADING_H */

