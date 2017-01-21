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

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8elapsed.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>

#define ULL(x) ((unsigned long long)(x))

int u8_debug_thread_loglevel=LOGWARN;
int u8_debug_thread_showid=1;
u8_mutex_fn u8_mutex_tracefn=NULL;
u8_rwlock_fn u8_rwlock_tracefn=NULL;

static u8_string ll2str(long long n,u8_byte *buf,size_t len)
{
  strcpy(buf,"Tx");
  u8_write_long_long(n,buf+2,len);
  return buf;
}
#define thid() \
  ((u8_debug_thread_showid) ? (u8_threadid()) : (-1))
#define thidstr(n,buf) (ll2str(n,buf,sizeof(buf)))

U8_EXPORT int u8_mutex_wait(pthread_mutex_t *mutex)
{
  u8_mutex_fn tracefn=u8_mutex_tracefn;
  double started=u8_elapsed_time();
  if (tracefn) tracefn(mutex,u8_dbg_lock_req,started);
  int rv=pthread_mutex_lock(mutex);
  double finished=u8_elapsed_time();
  long long tid=thid(); u8_byte buf[128];
  if (tracefn) tracefn(mutex,u8_dbg_lock,finished-started);
  if (rv)
    u8_log(u8_debug_thread_loglevel,"MutexWait",
	   "Waited %fs for error %d on mutex 0x%llx\t(t=%f) %s",
	   finished-started,rv,ULL(mutex),started,thidstr(tid,buf));
  else u8_log(u8_debug_thread_loglevel,"MutexWait",
	      "Waited %fs for mutex 0x%llx\t(t=%f) %s",
	      finished-started,ULL(mutex),started,thidstr(tid,buf));
  return rv;
}

U8_EXPORT int u8_mutex_release(u8_mutex *mutex)
{
  u8_mutex_fn tracefn=u8_mutex_tracefn;
  double now=u8_elapsed_time();
  if (tracefn) tracefn(mutex,u8_dbg_unlock,now);
  int rv=pthread_mutex_unlock(mutex);
  long long tid=thid(); u8_byte buf[128];
  if (rv)
    u8_log(u8_debug_thread_loglevel,"MutexRelease",
	   "Got error %d releasing mutex 0x%llx\t(t=%f) %s",
	   rv,ULL(mutex),now,thidstr(tid,buf));
  else u8_log(u8_debug_thread_loglevel,"MutexRelease",
	      "Releasing mutex 0x%llx\t(t=%f) %s",
	      ULL(mutex),now,thidstr(tid,buf));
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
  if (rv)
    u8_log(u8_debug_thread_loglevel,"RWLockWait",
	   "Waited %fs for error %d on %s lock 0x%llx\t(t=%f) %s",
	   finished-started,rv,((write)?("write"):("read")),
	   ULL(rwlock),started,thidstr(tid,buf));
  else u8_log(u8_debug_thread_loglevel,"RWLockWait",
	      "Waited %fs for %s 0x%llx",finished-started,
	      ((write)?("write"):("read")),ULL(rwlock),
	      started,thidstr(tid,buf));
  return rv;
}

U8_EXPORT int u8_rwlock_release(u8_rwlock *rwlock)
{
  u8_rwlock_fn tracefn=u8_rwlock_tracefn;
  double now=u8_elapsed_time();
  if (tracefn) tracefn(rwlock,u8_dbg_rwunlock,now);
  int rv=pthread_rwlock_unlock(rwlock);
  long long tid=thid(); u8_byte buf[128];
  if (rv)
    u8_log(u8_debug_thread_loglevel,"RWUnlock/ERR",
	   "Got error %d unlocking read/write lock 0x%llx\t(t=%f) %s",
	   rv,ULL(rwlock),now,thidstr(tid,buf));
  else u8_log(u8_debug_thread_loglevel,"RWUnlock",
	      "Unlocked read/write lock  0x%llx\t(t=%f) %s",
	      ULL(rwlock),thidstr(tid,buf));
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

#endif /* HAVE_PTHREAD_H */

#endif /* else HAVE_PTHREAD_RWLOCK_INIT */
