/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2011 beingmeta, inc.
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

static char versionid[] MAYBE_UNUSED=
  "$Id$";

#include "libu8/libu8.h"
#include "libu8/u8rusage.h"

/* We use stdio to open the /proc file system */
#include <stdio.h>

#if ((HAVE_MALLOC_H) && (HAVE_MALLINFO))
#include <malloc.h>
#elif ((HAVE_SYS_MALLOC_H) && (HAVE_MALLINFO))
#include <sys/malloc.h>
#endif

/* Getting rusage */

U8_EXPORT
/* u8_getrusage:
     Arguments: a pointer to an rusage structure
     Returns: -1 on error
 Gets rusage information, covering for holes in various
 implementations (currently just Linux) */
int u8_getrusage(int who,struct rusage *r)
{
#if HAVE_GETRUSAGE
  if (getrusage(who,r)<0) return -1;
#if U8_RUSAGE_PROC_PATCH
  {
    FILE *f=fopen("/proc/self/statm","r");
    int retval;
    long long total, res, shared, textsize, data, library, dirty_size;
    retval=fscanf(f,"%Ld %Ld %Ld %Ld %Ld %Ld %Ld",
		  &total,&res,&shared,
		  &textsize,&library,&data,&dirty_size);
    if (retval<0) return -1;
    /* These numbers are not entirely correct, but they're more
       correct than 0! */
    r->ru_idrss=total-(shared);
    r->ru_maxrss=res;
    r->ru_ixrss=shared;
    fclose(f);
    return 1;
  }
#endif
#else
  u8_seterr(_("no OS support"),"u8_getrusage",NULL);
  return -1;
#endif
  return 1;
}

static size_t procfs_memusage()
{
#if U8_RUSAGE_PROC_PATCH
  FILE *f=fopen("/proc/self/stat","r");
  int pid, ppid, pgrp, session, tty, tpgid; char commbuf[256], *comm=commbuf, state, retval;
  unsigned long long MAYBE_UNUSED
    flags, minflt, cminflt, majflt, cmajflt, utime, stime, ctime, cutime, cstime, priority;
  unsigned long long MAYBE_UNUSED
    nice, zero, itrealvalue, starttime, vsize, rss, rlim, startcode, endcode;
  unsigned long long MAYBE_UNUSED
    startstack, kstkesp, kstkeip, signal, blocked, sigignore, sigcatch, huh;
  unsigned long long MAYBE_UNUSED
    wchan, nswap, cnswap, exit_signal, processor, rt_priority, policy;
  long long MAYBE_UNUSED total, res, shared, textsize;
  if (f==NULL) return 0;
  else
    retval=fscanf(f,"%d %s %c %d %d %d %d %d %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Ld %Ld %Ld %Ld %Ld %Ld %Lu %Lu %Lu %Ld %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
		  &pid,comm,&state,&ppid,&pgrp,&session,&tty,&tpgid,&flags,&minflt,&cminflt,&majflt,
		  &cmajflt,&utime,&stime,&cutime,&cstime,&priority,&nice,&zero,&itrealvalue,&starttime,
		  &vsize,&rss,&rlim,&startcode,&endcode,&startstack,&kstkesp,&kstkeip,&signal,&blocked,
		  &sigignore,&sigcatch,&wchan,&nswap,&cnswap,&exit_signal,&processor,&rt_priority,
		  &policy,&huh);
  fclose(f);
  if (retval<0)
    return retval;
  else return vsize;
#else
  return 0;
#endif
}

U8_EXPORT unsigned long u8_memusage()
{
#if (HAVE_MALLINFO)
  struct mallinfo minfo=mallinfo();
  return minfo.uordblks;
#else
  return procfs_memusage();
#endif
}
