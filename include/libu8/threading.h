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

#ifndef LIBU8_THREADING_H
#define LIBU8_THREADING_H 1
#define LIBU8_THREADING_H_VERSION __FILE__

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
#define pthread_attr_default NULL
#define u8_init_mutex(x) _u8_init_mutex(x)
#define u8_destroy_mutex(x) pthread_mutex_destroy(x)
#define u8_new_threadkey(key_loc,del_fcn) pthread_key_create(key_loc,del_fcn)
#define u8_init_condvar(x) pthread_cond_init(x,NULL)
#define u8_destroy_condvar(x) pthread_cond_destroy(x)
#define u8_condvar_wait(cv,lck) pthread_cond_wait(cv,lck)
#define u8_condvar_timedwait(cv,lck,tm) pthread_cond_timedwait(cv,lck,tm)
#define u8_condvar_signal(cv) pthread_cond_signal(cv)
#define u8_condvar_broadcast(cv) pthread_cond_broadcast(cv)

#define u8_lock_mutex(x) pthread_mutex_lock((x))
#define u8_unlock_mutex(x) pthread_mutex_unlock((x))
#define u8_tld_get(key) pthread_getspecific(key)
#define u8_tld_set(key,v) pthread_setspecific(key,v)

/* Read/write locks are replaced with simple mutexes if
   not available natively. */
#if HAVE_PTHREAD_RWLOCK_INIT
typedef pthread_rwlock_t u8_rwlock;
#define u8_init_rwlock(x) pthread_rwlock_init(x,0)
#define u8_destroy_rwlock(x) pthread_rwlock_destroy(x)
#define u8_read_lock(x) pthread_rwlock_rdlock((x))
#define u8_write_lock(x) pthread_rwlock_wrlock((x))
#define u8_rw_unlock(x) pthread_rwlock_unlock((x))
#else
typedef pthread_mutex_t u8_rwlock;
#define u8_init_rwlock(x) pthread_mutex_init(x,0)
#define u8_destroy_rwlock(x) pthread_mutex_destroy(x)
#define u8_read_lock(x) pthread_mutex_lock((x))
#define u8_write_lock(x) pthread_mutex_lock((x))
#define u8_rw_unlock(x) pthread_mutex_unlock((x))
#endif

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

#endif /* LIBU8_THREADING_H */

