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

#include "libu8/u8source.h"
#include "libu8/libu8.h"
#include "libu8/u8memlist.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8stringfns.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8fileio.h"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if ( (HAVE_MMAP) && (HAVE_SYS_MMAN_H) )
#include <sys/mman.h>
#endif

#if ((HAVE_SYS_SYSCALL_H)&&(HAVE_SYSCALL))
#include <sys/syscall.h>
#endif

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef MAX_U8RUN_JOBID_LEN
#define MAX_U8RUN_JOBID_LEN 512
#endif
#ifndef MAX_JOB_PREFIX_LEN
#define MAX_U8RUN_PREFIX_LEN 512
#endif

#if U8_DARWIN_TARGET
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif

/* Just in case */

#define LOG_EMERG	0	/* system is unusable */
#define LOG_ALERT	1	/* action must be taken immediately */
#define LOG_CRIT	2	/* critical conditions */
#define LOG_ERR		3	/* error conditions */
#define LOG_WARNING	4	/* warning conditions */
#define LOG_NOTICE	5	/* normal but significant condition */
#define LOG_INFO	6	/* informational */
#define LOG_DEBUG	7	/* debug-level messages */

void perror(const char *s);

/* Component init functions */

void init_exceptions_c(void);
void init_printf_c(void);
void init_streamio_c(void);
void init_contour_c(void);
void init_ctype_c(void);
void init_stringfns_c(void);
void init_bytebuf_c(void);
void init_cityhash_c(void);
void init_atomic_c(void);
void init_logging_c(void);

/* U8 init vars */

static int u8_initialized=0;

int u8_debug_errno=0;

int u8_utf8warn=1, u8_utf8err=0;

u8_string u8_version=LIBU8_VERSION;
u8_string u8_full_version=LIBU8_GIT_VERSION;
int u8_major_version=U8_MAJOR_VERSION;
int u8_minor_version=U8_MINOR_VERSION;
int u8_release_version=1;

time_t u8_start_tick = -1;

/* Exceptions */

u8_condition u8_UnexpectedErrno=_("Unexpected errno");
u8_condition u8_NullArg=_("NULL argument (unexpected)");
u8_condition u8_MallocFailed=_("Malloc failed");
u8_condition u8_NotImplemented=_("Function not available");

u8_condition u8_UnexpectedEOD=_("Unexpected EOD"),
  u8_BadUTF8=_("Invalid UTF-8 encoded text"),
  u8_BadUTF8Start=_("Invalid UTF-8 start byte"),
  u8_TruncatedUTF8=_("Truncated UTF-8 sequence"),
  u8_BadUnicodeChar=_("Invalid Unicode Character"),
  u8_BadUNGETC=_("UNGETC error"),
  u8_NoZeroStreams=_("No zero-length string streams");

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
    if (converted != ((char *)string))
      u8_free((u8_byte *)string);
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
  if (u8_tolibcfn)
    return u8_tolibcfn(string);
  else return (char *) u8_valid_copy(string);
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
  if (u8_fromlibcfn)
    return u8_fromlibcfn(string);
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
  if (appid == NULL) appid = u8_getenv("U8_JOBID");
  return appid;
}

U8_EXPORT int u8_default_appid(u8_string id)
{
  if (appid) return 0;
  else appid=u8_strdup(id);
  return 1;
}

/* Configuring UTF-8 messaging */

U8_EXPORT int u8_config_utf8warn(int flag)
{
  if (flag>=0) {
    int prev=u8_utf8warn; u8_utf8warn=flag;
    return prev;}
  else return u8_utf8warn;
}

U8_EXPORT int u8_config_utf8err(int flag)
{
  if (flag>=0) {
    int prev=u8_utf8err; u8_utf8err=flag;
    return prev;}
  else return u8_utf8err;
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
  u8_log(LOG_NOTICE,RandomizeCondition,"Random seed set to 0x%x",seed);
  srandom(seed);
}

/* Elapsed time */

#if HAVE_CLOCK_GETTIME
#if defined(CLOCK_BOOTTIME)
#define USE_POSIX_CLOCK CLOCK_BOOTTIME
#elif defined(CLOCK_MONOTONIC_RAW)
#define USE_POSIX_CLOCK CLOCK_MONOTONIC_RAW
#elif defined(CLOCK_MONOTONIC)
#define USE_POSIX_CLOCK CLOCK_MONOTONIC
#else
#define USE_POSIX_CLOCK 0
#endif
#else /* not HAVE_CLOCK_GETTIME */
#define USE_POSIX_CLOCK 0
#endif

static time_t elapsed_time_initialized=0;
#if USE_POSIX_CLOCK
static struct timespec estart={0};
#elif HAVE_GETTIMEOFDAY
static struct timeval estart={0};
#elif HAVE_FTIME
static struct timeb estart={0};
#else
static time_t estart=-1;
#endif

static void init_timebase(void);

U8_EXPORT double u8_elapsed_time()
{
  if (! elapsed_time_initialized) {
    init_timebase();
    if (! elapsed_time_initialized)
      return -1.0;}
#if USE_POSIX_CLOCK
    struct timespec now;
    if (clock_gettime(USE_POSIX_CLOCK,&now)<0)
      return -1.0;
    else return (now.tv_sec-estart.tv_sec)+
	   (now.tv_nsec-estart.tv_nsec)*0.000000001;
#elif HAVE_GETTIMEOFDAY
    struct timeval now;
    if (gettimeofday(&now,NULL) < 0)
      return -1.0;
    else return (now.tv_sec-estart.tv_sec)+
	   (now.tv_usec-estart.tv_usec)*0.000001;
#elif HAVE_FTIME && WIN32
    struct timeb now;
    ftime(&now);
    return (now.time-estart.time)+
      (now.millitm-estart.millitm)*0.001;
#elif HAVE_FTIME
    struct timeb now;
    if (ftime(&now) < 0) return -1.0;
    return (now.time-estart.time)+
      (now.millitm-estart.millitm)*0.001;
#else
    return (1.0*(time(NULL)-estart));
#endif
}

/* Initialize time base */

static void init_timebase()
{
  if (elapsed_time_initialized) return;
  u8_start_tick = time(NULL);
#if USE_POSIX_CLOCK
  if (clock_gettime(USE_POSIX_CLOCK,&estart)>=0)
    elapsed_time_initialized=1;
#elif HAVE_GETTIMEOFDAY
  if (gettimeofday(&estart,NULL)>=0)
    elapsed_time_initialized=1;
  else perror("u8_elapsed_time()");
#elif HAVE_FTIME && WIN32
    ftime(&estart);
#elif  HAVE_FTIME && WIN32
    if (ftime(&estart)>=0)
      elapsed_time_initialized=1;
    else perror("u8_elapsed_time()");
#else
    estart=time(NULL);
    elapsed_time_initialized=1;
#endif
}

U8_EXPORT time_t u8_elapsed_base()
{
  if (elapsed_time_initialized == 0)
    return -1;
  else return u8_start_tick;
}

/* Hand coded num/string functions */

static char decimal_weights[10]={'0','1','2','3','4','5','6','7','8','9'};

/* This writes a long long out to a string and doesn't require 
   printf or other functions. It can also be called in signal 
   handlers */
U8_EXPORT char *u8_write_long_long(long long l,char *buf, size_t buflen)
{
  char _buf[64]; 
  char *write=_buf, *read; size_t outlen=0;
  long long scan=l;
  while (scan>0) {
    int weight=scan%10;
    if (outlen>=buflen) {
      return NULL;}
    *write++=decimal_weights[weight];
    scan=scan/10;
    outlen++;}
  read=write-1; write=buf;
  while (read>=_buf) {*write++=*read--;}
  *write++='\0';
  return buf;
}

/* Microsecond time */

U8_EXPORT
/** Returns the number of microseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting microseconds
*/
u8_utime u8_microtime()
{
#if HAVE_GETTIMEOFDAY
  struct timeval now;
  if (gettimeofday(&now,NULL) < 0)
    return -1;
  else return (((long long)now.tv_sec)*1000000LL)+((long long)(now.tv_usec));
#elif HAVE_FTIME
  /* We're going to settle for millisecond precision here. */
  struct timeb now;
#if WIN32
  /* In WIN32, ftime doesn't return an error value.
     ?? We should really do something respectable here.*/
  ftime(&now);
#else
  if (ftime(&now) < 0) return -1;
  else return now.time*1000000+now.millitm*1000;
#endif
#else
  return ((int)time(NULL))*1000000;
#endif
}

/* Page size */

U8_EXPORT
/** Returns the current libu8 revision identifier
    @returns a const utf8 string (converted from the value) or NULL
**/
long u8_getpagesize()
{
#if ((HAVE_SYSCONF)&&(defined(_SC_PAGESIZE)))
  return sysconf(_SC_PAGESIZE);
#elif (HAVE_GETPAGESIZE)
  return getpagesize();
#else
  return 512;
#endif
}

/* Version information */

U8_EXPORT
/** Returns the current libu8 revision identifier
    @returns a const utf8 string (converted from the value) or NULL
**/
u8_string u8_getrevision()
{
  return LIBU8_GIT_VERSION;
}

U8_EXPORT
/** Returns the current libu8 version string
    @returns a const utf8 string (converted from the value) or NULL
**/
u8_string u8_getversion()
{
  return U8_VERSION;
}

U8_EXPORT
/** Returns the current libu8 version string
    @returns a const utf8 string (converted from the value) or NULL
**/
int u8_getmajorversion()
{
  return U8_MAJOR_VERSION;
}

/* Environment access */

U8_EXPORT
/** Returns a named value from the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @returns a utf8 string (converted from the value) or NULL
**/
u8_string u8_getenv(u8_string envvar)
{
  if (envvar == NULL) return NULL;
  char *var=u8_tolibc(envvar);
  char *envval=getenv(var);
  if (envval) {
    u8_free(var);
    return u8_fromlibc(envval);}
  else {
    u8_free(var);
    return NULL;}
}

U8_EXPORT
/** Returns a integer value from the current environment
    @param envvar the variable name, as a UTF-8 string.
    @param dflt a default value if the environment variable is not defined or couldn't be parsed
    @returns an integer (long long)
**/
long long u8_getenv_int(u8_string var,long long dflt)
{
  if (var == NULL)  {
    u8_log(LOGERR,_("NullEnvVar"),
           _("Name passed to u8_getenv_int is NULL, using default %lld"),
           dflt);
    return dflt;}
  u8_string value = u8_getenv(var), end=NULL;
  if ( (value == NULL) || (*value == '\0') ) {
    if (value) u8_free(value);
    return dflt;}
  long long intval = strtoll(value,(char **)&end,10);
  if (end == value) intval = dflt;
  u8_free(value);
  return intval;
}

U8_EXPORT
/** Returns a integer value from the current environment
    @param envvar the variable name, as a UTF-8 string.
    @param dflt a default value if the environment variable is not defined or couldn't be parsed
    @returns an integer (long long)
**/
double u8_getenv_float(u8_string var,double dflt)
{
  if (var == NULL)  {
    u8_log(LOGERR,_("NullEnvVar"),
           _("Name passed to u8_getenv_float is NULL, using default %f"),
           dflt);
    return dflt;}
  u8_string value = u8_getenv(var), end=NULL;
  if ( (value == NULL) || (*value == '\0') )
    return dflt;
  double floval = strtod(value,(char **)&end);
  if ( end == value )
    floval = dflt;
  u8_free(value);
  return floval;
}

U8_EXPORT
/** Sets a variable i the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @param setval a string to set
    @param overwrite whether to replace an existing value
    @returns 1 if anything was done, 0 if not, -1 on error
**/
int u8_setenv(u8_string envvar,u8_string setval,int overwrite)
{
  if (envvar == NULL) return u8_reterr("NullEnvVar","u8_setenv",NULL);
#if HAVE_SETENV
  const char *var=u8_tolibc(envvar);
  const char *val=(setval) ? (u8_tolibc(setval)) : (NULL);
  const char *cur=getenv(var);
  int rv = setenv(var,val,overwrite);
  if (rv<0) {
    u8_graberrno("u8_setenv",u8_strdup(envvar));}
  else if ( (cur) && (!overwrite) )
    rv = 0;
  else rv = 1;
  u8_free(var); if (val) u8_free(val);
  return rv;
#else
  return u8_reterr(u8_NotImplemented,"u8_setenv",u8_strdup(envvar));
#endif
}

U8_EXPORT
/** Sets a variable i the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @returns 1 if anything was done, 0 if not, -1 on error
**/
int u8_unsetenv(u8_string envvar)
{
  if (envvar == NULL) return u8_reterr("NullEnvVar","u8_setenv",NULL);
#if HAVE_SETENV
  const char *var=u8_tolibc(envvar);
  int cur = (getenv(var) != NULL);
  int rv = unsetenv(var);
  if (rv<0) {
    u8_graberrno("u8_setenv",u8_strdup(envvar));}
  else if (cur)
    rv = 1;
  else rv = 0;
  u8_free(var);
  return rv;
#else
  return u8_reterr(u8_NotImplemented,"u8_unsetenv",u8_strdup(envvar));
#endif
}

/* Freeing u8_xptrs */

U8_EXPORT void u8_free_xptr(struct U8_XPTR *xptr)
{
  if ( (xptr->u8x_ptrval) && (xptr->u8x_freefn) ) {
    void *ptrval = xptr->u8x_ptrval; xptr->u8x_ptrval=NULL;
    xptr->u8x_freefn(ptrval);}
}

/* Dynamic loading */

static U8_MAYBE_UNUSED u8_condition NoDynamicLoading=
  _("No dynamic loading of libraries");
static u8_condition FailedDLopen=_("Call to DLOPEN failed");

#if HAVE_DLFCN_H
U8_EXPORT
void *u8_dynamic_load(u8_string name_arg)
{
  u8_string name = (strchr(name_arg,'.')) ? (u8_strdup(name_arg)) :
    (u8_string_append(name_arg,".",U8_SHARED_SUFFIX,NULL));
  char *modname=u8_tolibc(name);
  void *module=dlopen(modname,RTLD_NOW|RTLD_GLOBAL);
#if U8_DARWIN_TARGET
  if ( (module == NULL) && (*modname != '/') && (*modname != '@') ) {
    ssize_t need_len = strlen(name)+32;
    u8_byte buf[need_len];
    strcpy(buf,"@executable_path/");
    strcat(buf,name);
    module = dlopen(buf,RTLD_NOW|RTLD_GLOBAL);
    if (module == NULL) {
      strcpy(buf,"@loader_path/");
      strcat(buf,name);
      module = dlopen(buf,RTLD_NOW|RTLD_GLOBAL);}
    if (module == NULL) {
      strcpy(buf,"@rpath/");
      strcat(buf,name);
      module = dlopen(buf,RTLD_NOW|RTLD_GLOBAL);}}
#endif
  if (module==NULL) 
    u8_seterr(FailedDLopen,"u8_dynamic_load",u8_fromlibc((char *)dlerror()));
  u8_free(modname);
  u8_free(name);
  U8_CLEAR_ERRNO();
  return module;
}

U8_EXPORT
void *u8_dynamic_symbol(u8_string symname,void *module)
{
  void *val;
  char *name=u8_tolibc(symname);
  if (module == NULL) {
    val=dlsym(RTLD_DEFAULT,name);
    u8_free(name);
    return val;}
  else {
    val=dlsym(module,name);
    u8_free(name);
    return val;}
}
#else
U8_EXPORT
void *u8_dynamic_load(u8_string name)
{
  u8_seterr(NoDynamicLoading,"u8_dynamic_load",u8_strdup(name));
  return NULL;
}
void *u8_dynamic_symbol(u8_string name,void *module)
{
  u8_seterr(NoDynamicLoading,"u8_dynamic_load",u8_strdup(name));
  return NULL;
}
#endif

/* Raising errors */

int u8_raise_debug = 0;

U8_EXPORT void u8_raise(u8_condition cond,u8_context cxt,u8_string details)
{
  u8_contour c=u8_dynamic_contour;
  if (cond == NULL) cond = "Unknown (NULL) condition";
  if (!(c)) {
    /* This is signal safe */
    {int rv=0;

      /* This writes to both stdout and stderr */
#define errout(s) \
      if (rv>=0) rv=write(STDOUT_FILENO,s,strlen(s)); \
      if (rv>=0) rv=write(STDERR_FILENO,s,strlen(s));

      errout("Unhandled exception (no context) ");
      errout(cond);
      if (cxt) {errout(" ("); errout(cxt); errout(")");}
      if (details) {errout(": "); errout(details); errout("\n");}

#undef errout

      fsync(STDERR_FILENO);
      fsync(STDOUT_FILENO);

    }

    if (u8_raise_debug) {
      struct U8_EXCEPTION ex = { 0 };
      ex.u8x_cond = cond;
      ex.u8x_context = cxt;
      ex.u8x_details = details;
      u8_debug_wait(&ex,1);}

    /* Calling u8_log isn't signal safe, but we're calling exit()
       anyway, so it might not be so bad :) */
    if ((cxt)&&(details))
      u8_log(LOG_CRIT,cond,"In context %s: %s",cxt,details);
    else if (cxt)
      u8_log(LOG_CRIT,cond,"In context %s",cxt);
    else if (details)
      u8_log(LOG_CRIT,cond,"Somewhere: %s",details);
    else u8_log(LOG_CRIT,cond,"somewhere");
    exit(1);}
  else {
    if (!(cxt)) cxt=c->u8c_label;
    c->u8c_condition=cond; c->u8c_exinfo.context=cxt;
    if (details) {
      strncpy(c->u8c_exdetails,details,sizeof(c->u8c_exdetails)-1);}
    if (u8_raise_debug) {
      struct U8_EXCEPTION ex = { 0 };
      ex.u8x_cond = cond;
      ex.u8x_context = cxt;
      ex.u8x_details = details;
      u8_debug_wait(&ex,1);}
    u8_throw_contour(c);}
}

U8_EXPORT void u8_raise_exception(u8_exception ex)
{
  u8_contour c=u8_dynamic_contour;
  if (!(c)) {
    /* This is signal safe */
    int rv=0;

    if (ex) {
      /* This writes to both stdout and stderr */
#define errout(s)                                       \
      if (rv>=0) rv=write(STDOUT_FILENO,s,strlen(s));   \
      if (rv>=0) rv=write(STDERR_FILENO,s,strlen(s));

      errout("Unhandled exception (no context) ");
      errout(ex->u8x_cond);
      if (ex->u8x_context) {
        errout(" <");
        errout(ex->u8x_context);
        errout(">");}
      if (ex->u8x_details) {
        errout(" (");
        errout(ex->u8x_details);
        errout(")\n");}

#undef errout
      fsync(STDERR_FILENO);
      fsync(STDOUT_FILENO);}

    if (u8_raise_debug) u8_debug_wait(ex,1);

    /* Calling u8_log isn't signal safe, but we're calling exit()
       anyway, so it might not be so bad :) */
    if ((ex->u8x_context)&&(ex->u8x_details))
      u8_log(LOG_CRIT,ex->u8x_cond,"In context %s: %s",
             ex->u8x_context,ex->u8x_details);
    else if (ex->u8x_context)
      u8_log(LOG_CRIT,ex->u8x_cond,"In context %s",ex->u8x_context);
    else if (ex->u8x_details)
      u8_log(LOG_CRIT,ex->u8x_cond,"Somewhere: %s",ex->u8x_details);
    else u8_log(LOG_CRIT,ex->u8x_cond,"somewhere");
    exit(1);}
  else {
    c->u8c_condition = ex->u8x_cond;
    c->u8c_exinfo.exception = ex;
    c->u8c_flags|=U8_CONTOUR_EXCEPTIONAL;
    if (u8_raise_debug) u8_debug_wait(ex,1);
    u8_throw_contour(c);}
}

U8_EXPORT void u8_set_error_handler
  (void (*h)(u8_condition,u8_context,u8_string))
{
  /* error_handler=h; */
}

/* Allocation + memset */

U8_EXPORT void *u8_zrealloc(void *ptr,size_t n,size_t oldsz)
{
  void *nptr=((ptr)?(realloc(ptr,n)):(malloc(n)));
  if (!(nptr)) {
    u8_seterr(u8_MallocFailed,"u8_zrealloc",NULL);
    return ptr;}
  if (ptr==NULL) memset(((unsigned char *)nptr),0,n);
  else if (n>oldsz) memset(((unsigned char *)nptr)+oldsz,0,(n-oldsz));
  else NO_ELSE;
  return nptr;
}

U8_EXPORT void *u8_extalloc(void *ptr,size_t n,size_t osz)
{
  void *nptr=malloc(n);
  unsigned char *cptr=(unsigned char *)nptr;
  if (!(nptr)) {
    u8_seterr(u8_MallocFailed,"u8_extalloc",NULL);
    return NULL;}
  if (ptr==NULL) memset(cptr,0,n);
  else {
    memcpy(cptr,ptr,osz);
    memset(cptr+osz,0,n-osz);}
  return nptr;
}

/* Undefine these here, so we can define proxy definitions for
   existing binaries */
#undef u8_reallocz
#undef u8_mallocz

U8_EXPORT void *u8_reallocz(void *ptr,size_t n,size_t oldsz)
{
  return u8_zrealloc(ptr,n,oldsz);
}

U8_EXPORT void *u8_mallocz(size_t n)
{
  return u8_zmalloc(n);
}

/* Big malloc */

size_t u8_mmap_threshold = U8_MMAP_THRESHOLD;

U8_EXPORT void *u8_big_alloc(ssize_t n)
{
  if (n < 0)
    return NULL;
  else if (n == 0)
    return NULL;
#if HAVE_MMAP
  if (n > u8_mmap_threshold) {
    size_t mmap_size = n+16;
    void *mmapped = mmap(NULL,mmap_size,
			 PROT_READ|PROT_WRITE,
			 MAP_PRIVATE|MAP_ANONYMOUS,
			 -1,0);
    if (mmapped) {
      ssize_t *head = (ssize_t *) mmapped;
      *head = n;
      return mmapped+16;}
    else {
      /* If mmap() fails, just fall through to malloc */
      errno=0;}}
#endif
  void *base = calloc(n+16,1);
  if (base == NULL) {
    unsigned char buf[32];
    u8_string size_string = u8_uitoa10(n,buf);
    u8_graberrno("u8_big_alloc",u8_strdup(size_string));
    return NULL;}
  ssize_t *head = (ssize_t *) base;
  *head = -n;
  return base+16;
}

U8_EXPORT void *u8_big_calloc(ssize_t n,ssize_t eltsz)
{
  return u8_big_alloc(n*eltsz);
}

U8_EXPORT ssize_t u8_big_free(void *ptr)
{
  if (ptr == NULL) return 0;
  void    *base   = ptr - 16;
  ssize_t *header = (ssize_t *) base;
  ssize_t head    = *header;
#if HAVE_MMAP
  if (head >= 0) {
    int rv = munmap(base,head);
    if (rv<0) {
      u8_graberr(errno,"u8_big_free",NULL);
      u8_exception ex = u8_current_exception;
      if (ex) {
	ex->u8x_xdata = base;
	ex->u8x_free_xdata = NULL;}
      return rv;}
    else return head;}
#endif
  u8_free(base);
  return -head;
}

U8_EXPORT void *u8_big_realloc(void *ptr,ssize_t n)
{
  if (n < 0)
    return NULL;
  else if (n == 0) {
    u8_big_free(ptr);
    return NULL;}
  int ismapped = 0;
  void    *base   = ptr - 16;
  ssize_t *header = (ssize_t *) base;
  ssize_t  head   = *header;
  size_t old_size = (head>0) ? (head) : (-head);
#if HAVE_MMAP
  if ( (n < u8_mmap_threshold) && (head < 0) ) {
    /* Both the old and the new are malloc'd, so we just realloc */
    void *newptr = u8_realloc(base,n+16);
    if (newptr == NULL) {
      u8_log(LOGCRIT,"BigReallocFailed",
	     "u8_realloc from %lld to %lld failed, errno=%d (%s)",
	     old_size,n,errno,u8_strerror(errno));
      errno=0;
      return ptr;}
    header = (ssize_t *) newptr;
    *header = -n;
    return newptr+16;}
  else {
    /* One of the new or old pointers is mmapped, so we need to copy
       the data and free the old pointer. */
    void *new_chunk = NULL;
    size_t new_extent = n+16;
    if (n >= u8_mmap_threshold) {
      new_chunk = mmap(NULL,new_extent,
		       PROT_READ|PROT_WRITE,
		       MAP_PRIVATE|MAP_ANONYMOUS,
		       -1,0);
      /* If mmap() failed, fall through to try malloc */
      if (new_chunk == NULL) errno=0;
      else ismapped=1;}
    if (new_chunk == NULL)
      new_chunk = u8_malloc(new_extent);
    if (new_chunk == NULL) {
      u8_log(LOGCRIT,"BigReallocFailed",
	     "u8_big_realloc from %lld to %lld failed, errno=%d (%s)",
	     old_size,n,errno,u8_strerror(errno));
      errno=0;
      return ptr;}

    /* The header for the new memory */
    header = (ssize_t *) new_chunk;

    /* Copy the data */
    char *src = ((char *)ptr);
    char *dst = ((char *)(new_chunk))+16;
    size_t copy_size = ( n > old_size) ? (old_size) : (n);
    memcpy(dst,src,copy_size);

    /* Store the size */
    if (ismapped)
      *header = n;
    else *header = -n;

    /* Free the old pointer */
    if (head<0) u8_free(base);
    else {
      int rv = munmap(base,old_size+16);
      if (rv<0) {
	u8_log(LOGCRIT,"MUnmapFailed","With errno=%d (%s)",
	       errno,u8_strerror(errno));
	errno=0;}}
    return (void *) dst;}
#endif
  void *new_chunk = u8_realloc(base,n+8);
  if (new_chunk == NULL) {
    u8_log(LOGCRIT,"BigReallocFailed",
	   "u8_realloc from %lld to %lld failed, errno=%d (%s)",
	   old_size,n,errno,u8_strerror(errno));
    errno=0;
    return ptr;}
  header = (ssize_t *) new_chunk;
  *header = -n;
  return new_chunk+16;
}

U8_EXPORT void *u8_big_copy(const void *src,ssize_t newlen,ssize_t oldlen)
{
  if (newlen < 0)
    return NULL;
  else if (newlen == 0)
    return NULL;
  unsigned char *buf = u8_big_alloc(newlen);
  memcpy(buf,src,oldlen);
  return buf;
}

/* Piles */

U8_EXPORT int _u8_grow_pile(u8_pile p,int delta)
{
  size_t need=p->u8_len+delta;
  size_t old_max=p->u8_max, new_max=((old_max>0)?(old_max):(16));
  while (need<new_max) new_max=new_max*2;
  if (need>=p->u8_max) {
    size_t new_max=((p->u8_max)?(p->u8_max*2):(16));
    void *new_elts;
    if (p->u8_elts) {
      if (p->u8_mallocd) {
	new_elts=u8_realloc(p->u8_elts,sizeof(void *)*new_max);
	memset(new_elts+p->u8_len,0,sizeof(void *)*(new_max-p->u8_max));}
      else {
	new_elts=u8_malloc(sizeof(void *)*new_max);
	memcpy(new_elts,p->u8_elts,sizeof(void *)*(p->u8_len));
	memset(new_elts+p->u8_len,0,sizeof(void *)*(new_max-p->u8_max));}}
    else {
      new_elts=u8_malloc(sizeof(void *)*new_max);
      memset(new_elts,0,sizeof(void *)*new_max);}
    p->u8_elts=new_elts; p->u8_max=new_max; p->u8_mallocd=1;}
  return new_max;
}

/* This is a good place to set a breakpoint */

U8_EXPORT int _u8_dbg(u8_string s)
{
  int retval=0;
  u8_log(LOG_CRIT,"In debugger: %s",s);
  return retval;
}

/* Debugging malloc */

void *u8_watchptr=NULL;
ssize_t u8_max_malloc=-1;

U8_EXPORT void *u8_dmalloc(size_t n_bytes)
{
  if ((u8_max_malloc>0)&&(n_bytes>u8_max_malloc))
    _u8_dbg("dmalloc/big");
  if (u8_watchptr) {
    void *result=calloc(n_bytes,1);
    if (result==NULL)
      _u8_dbg("dmalloc/failed");
    else if (result==u8_watchptr)
      _u8_dbg("dmalloc/watched");
    return result;}
  else return malloc(n_bytes);
}

U8_EXPORT void *u8_dmalloc_n(size_t n_elts,size_t elt_size)
{
  if ((u8_max_malloc>0)&&((n_elts*elt_size)>u8_max_malloc))
    _u8_dbg("dmalloc/big");
  if (u8_watchptr) {
    void *result=calloc(n_elts,elt_size);
    if (result==NULL)
      _u8_dbg("dmalloc/failed");
    else if (result==u8_watchptr)
      _u8_dbg("dmalloc/watched");
    return result;}
  else return calloc(n_elts,elt_size);
}

U8_EXPORT void *u8_drealloc(void *ptr,size_t n_bytes)
{
  void *newptr=NULL;
  if ((u8_max_malloc>0)&&(n_bytes>u8_max_malloc))
    _u8_dbg("drealloc/big");
  if (ptr == u8_watchptr)
    _u8_dbg("drealloc/watched");
  newptr=realloc(ptr,n_bytes);
  if (newptr==NULL) {
    _u8_dbg("drealloc/failed");
    return ptr;}
  else {
    if (newptr==u8_watchptr)
      _u8_dbg("drealloc/watched");
    return newptr;}
}

U8_EXPORT void u8_dfree(void *ptr)
{
  if (ptr==NULL) {
    _u8_dbg("u8_dfree/NULL");}
  else if (ptr==u8_watchptr) {
    _u8_dbg("u8_dfree/NULL");
    free(ptr);}
  else free(ptr);
}

/* Recording source file information */

static struct U8_SOURCE_FILE_RECORD *source_files=NULL;
static int n_source_files=0;
static u8_mutex source_registry_lock;

U8_EXPORT void u8_register_source_file(u8_string s)
{
  struct U8_SOURCE_FILE_RECORD *rec=
    u8_alloc(struct U8_SOURCE_FILE_RECORD);
  u8_lock_mutex(&source_registry_lock);
  rec->filename=s; rec->next=source_files;
  source_files=rec; n_source_files++;
  u8_unlock_mutex(&source_registry_lock);
}
U8_EXPORT void u8_for_source_files(void (*f)(u8_string s,void *),void *data)
{
  u8_lock_mutex(&source_registry_lock); {
    int i=0, n=n_source_files;
    u8_string *files=u8_alloc_n(n,u8_string), *write=files;
    struct U8_SOURCE_FILE_RECORD *scan=source_files;
    while (scan) {*write++=scan->filename; scan=scan->next;}
    u8_unlock_mutex(&source_registry_lock);
    while (i<n) {f(files[i++],data);}
    u8_free(files);}
}

/* u8list functions */

U8_EXPORT u8_memlist u8_cons_list(void *ptr,u8_memlist cdr,int big)
{
  struct U8_MEMLIST *cons = u8_alloc(struct U8_MEMLIST);
  cons->u8ml_ptr = ptr;
  cons->u8ml_next = cdr;
  cons->u8ml_big = big;
  return cons;
}

U8_EXPORT void u8_free_list(u8_memlist lst)
{
  struct U8_MEMLIST *scan = lst;
  while (scan) {
    u8_memlist next = scan->u8ml_next;
    if (scan->u8ml_big)
      u8_big_free(scan->u8ml_ptr);
    else u8_free(scan->u8ml_ptr);
    u8_free(scan);
    scan=next;}
}

/* Shims */

U8_EXPORT int u8_execvpe(char *prog,char *const argv[],char *envp[])
{
  environ = envp;
  return execvp(prog,(char **)argv);
}

/* Initialization */

U8_EXPORT void u8_initialize_logging_c(void);
U8_EXPORT void u8_initialize_threading(void);

void init_status_c(void);

U8_EXPORT int u8_initialize()
{
  if (u8_initialized) return u8_initialized;

  init_timebase();

  if (getenv("U8RAISEDEBUG")) u8_raise_debug=1;

  u8_initialize_threading();

  u8_init_mutex(&source_registry_lock);

  u8_register_source_file(_FILEINFO);

  init_printf_c(); /* Does something */
  init_exceptions_c();  /* Does something */
  init_atomic_c();

  init_streamio_c();
  init_contour_c();
  init_logging_c();
  init_ctype_c();
  init_stringfns_c();
  init_status_c();
  init_bytebuf_c();
  init_cityhash_c();

  bindtextdomain("libu8msg",NULL);
  bindtextdomain_codeset("libu8msg","utf-8");

  u8_initialized=8069;
  return 8069;
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
