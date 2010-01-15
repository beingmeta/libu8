/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2010 beingmeta, inc.
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

#include "libu8/u8stringfns.h"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if TIME_WITH_SYS_TIME
#include <time.h>
#include <sys/time.h>
#elif HAVE_TIME_H
#include <time.h>
#elif HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef MAX_THREADINITFNS
#define MAX_THREADINITFNS 128
#endif
#ifndef MAX_THREAEXITFNS
#define MAX_THREADEXITFNS 128
#endif

#include <errno.h>

void perror(const char *s);

U8_EXPORT void u8_init_errors_c(void);
U8_EXPORT void u8_init_printf_c(void);

u8_condition u8_MallocFailed=_("Malloc failed");

static int u8_initialized=0;

#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

/* libc conversions */

u8_string (*u8_fromlibcfn)(char *)=NULL;
char *(*u8_tolibcfn)(u8_string)=NULL;

U8_EXPORT
/* u8_2libc:
    Arguments: a u8_string
    Returns: a regular string
 This converts a utf8 string to a "regular" string intended to
  be passed to libc or other external libraries.  The conversion
  functions in both directions can be set by the function
  u8_set_libcfns().  This assumes that its argument is a malloc'd
  string and either returns that string or frees it. */
char *u8_2libc(u8_string string)
{
  if (u8_tolibcfn) {
    char *converted=u8_tolibcfn(string);
    if (converted != ((char *)string)) u8_free(string);
    return converted;}
  else return (char *) string;
}

U8_EXPORT
/* u8_tolibc:
    Arguments: a u8_string
    Returns: a regular string
 This converts a utf8 string to a "regular" string intended to
  be passed to libc or other external libraries.  This should always
  return a copy of its argument.  The conversion functions in both
  directions can be set by the function u8_set_libcfns(); */
char *u8_tolibc(u8_string string)
{
  if (u8_tolibcfn) return u8_tolibcfn(string);
  else return u8_valid_copy(string);
}

U8_EXPORT
/* u8_fromlibc:
    Arguments: a regular string
    Returns: a U8 (utf8 encoded) string
 This converts a a regular string to a utf8 string.  It is intended
  to be called on the results of library functions. This should always
  return a copy of its argument.  The conversion functions in both
  directions can be set by the function u8_set_libcfns(); */
u8_string u8_fromlibc(char *string)
{
  if (u8_fromlibcfn) return u8_fromlibcfn(string);
  else return u8_valid_copy(string);
}

U8_EXPORT
void u8_set_libcfns(u8_string (*from)(char *),char *(*to)(u8_string))
{
  u8_fromlibcfn=from;
  u8_tolibcfn=to;
}

/* Application identification */

static u8_string appid=NULL;

U8_EXPORT void u8_identify_application(u8_string newid)
{
  u8_string old_appid=appid;
  appid=u8_strdup(newid);
  if (old_appid) u8_free(old_appid);
}

U8_EXPORT u8_string u8_appid()
{
  return appid;
}

U8_EXPORT int u8_default_appid(u8_string id)
{
  if (appid) return 0;
  else appid=u8_strdup(id);
  return 1;
}

U8_EXPORT int u8_config_utf8warn(int flag)
{
  if (flag>=0) {
    int prev=u8_utf8warn; u8_utf8warn=flag;
    return prev;}
  else return u8_utf8warn;
}

/* Random numbers */

U8_EXPORT
unsigned int u8_random(unsigned int max)
{
  return random()%max;
}

static u8_condition RandomizeCondition=_("RANDOMIZE");

U8_EXPORT
void u8_randomize(unsigned int seed)
{
  u8_log(LOG_NOTICE,RandomizeCondition,"Random seed set to 0x%X",seed);
  srandom(seed);
}

/* Elapsed time */

static int elapsed_time_initialized=0;
#if HAVE_GETTIMEOFDAY
static struct timeval estart;
#elif HAVE_FTIME
static struct timeb estart;
#else
static time_t estart;
#endif

U8_EXPORT double u8_elapsed_time()
{
  if (elapsed_time_initialized) {
#if HAVE_GETTIMEOFDAY
    struct timeval now;
    if (gettimeofday(&now,NULL) < 0)
      return -1.0;
    else return (now.tv_sec-estart.tv_sec)+
	   (now.tv_usec-estart.tv_usec)*0.000001;
#elif HAVE_FTIME
    struct timeb now;
#if WIN32
    ftime(&now);
#else /* In WIN32, ftime doesn't return an error value */
    if (ftime(&now) < 0) return -1.0;
    else
#endif
      return (now.time-estart.time)+
	(now.millitm-estart.millitm)*0.001;
#else
    return (1.0*(time(NULL)-estart));
#endif
  }
  else {
#if HAVE_GETTIMEOFDAY
    if (gettimeofday(&estart,NULL)>=0)
      elapsed_time_initialized=1;
    else perror("u8_elapsed_time()");
#elif HAVE_FTIME
#if WIN32
    ftime(&estart);
#else
    if (ftime(&estart)>=0)
      elapsed_time_initialized=1;
    else perror("u8_elapsed_time()");
#endif
#else
    estart=time(NULL);
    elapsed_time_initialized=1;
#endif
    return 0.0;
  }
}

/* Environment access */

U8_EXPORT
/** Returns a named value from the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @returns a utf8 string (converted from the value) or NULL
**/
u8_string u8_getenv(u8_string envvar)
{
  char *var=u8_tolibc(envvar);
  char *envval=getenv(var);
  if (envval) {
    u8_free(var);
    return u8_fromlibc(envval);}
  else {
    u8_free(var);
    return NULL;}
}

/* Dynamic loading */

static MAYBE_UNUSED u8_condition NoDynamicLoading=
  _("No dynamic loading of libraries");
static u8_condition FailedDLopen=_("Call to DLOPEN failed");

#if HAVE_DLFCN_H
U8_EXPORT
void *u8_dynamic_load(u8_string name)
{
  char *modname=u8_tolibc(name);
  void *module=dlopen(modname,RTLD_NOW|RTLD_GLOBAL);
  if (module==NULL) {
    u8_seterr(FailedDLopen,"u8_dynamic_load",u8_fromlibc((char *)dlerror()));
    errno=0;}
  u8_free(modname);
  return module;
}
#else
U8_EXPORT
void *u8_dynamic_load(u8_string name)
{
  u8_seterr(NoDynamicLoading,"u8_dynamic_load",u8_strdup(name));
  return NULL;
}
#endif


/* Raising errors */

U8_EXPORT void u8_raise(u8_condition ex,u8_context cxt,u8_string details)
{
  exit(1);
}

U8_EXPORT void u8_set_error_handler
  (void (*h)(u8_condition,u8_context,u8_string))
{
  /* error_handler=h; */
}

/* Thread initialization */

#if (U8_THREADS_ENABLED)
int u8_n_threadinits=0;
static int n_threadexitfns=0;
static u8_threadexitfn threadexitfns[MAX_THREADEXITFNS];
static u8_threadinitfn threadinitfns[MAX_THREADINITFNS];
static u8_mutex threadinitfns_lock;
#if ((HAVE_THREAD_STORAGE_CLASS) && (!(U8_FORCE_TLS)))
__thread int u8_initlevel=0;
#else
u8_tld_key u8_initlevel_key;
#endif
#endif

#if U8_THREADS_ENABLED
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
    u8_threadcheck();
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
#else
U8_EXPORT int u8_register_threadinit(u8_threadinitfn fn)
{
  return fn();
}
U8_EXPORT int u8_run_threadinits()
{
  return 0;
}
#endif

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

static void threadexit_atexit()
{
  u8_threadexit();
}

/* Initialization */

U8_EXPORT int u8_initialize()
{
  if (u8_initialized) return u8_initialized;
  u8_init_errors_c();
  u8_init_printf_c();
  bindtextdomain("libu8msg",NULL);
  bind_textdomain_codeset("libu8msg","utf-8");

#if U8_THREADS_ENABLED
  u8_init_mutex(&threadinitfns_lock);
#if (!((HAVE_THREAD_STORAGE_CLASS) && (!(U8_FORCE_TLS))))
  u8_new_threadkey(&u8_initlevel_key,NULL);
#endif
#endif

  atexit(threadexit_atexit);

  u8_initialized=8069;
  return 8069;
}

