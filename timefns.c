/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2013 beingmeta, inc.
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

typedef unsigned long long u8ull;
typedef unsigned int u8uint;
#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8timefns.h"

#include <math.h>
/* We just include this for sscanf */
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#if HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if U8_THREADS_ENABLED
static u8_mutex timefns_lock;
#endif

/* Utility functions */

static MAYBE_UNUSED double getprecfactor(enum u8_timestamp_precision precision)
{
  switch (precision) {
  case u8_year: return 1000000000.0*3600*24*365;
  case u8_month: return 1000000000.0*3600*24*30;
  case u8_day: return 1000000000.0*3600*24;
  case u8_hour: return 1000000000.0*3600;
  case u8_minute: return 1000000000.0*60;
  case u8_second: return 1000000000.0;
  case u8_millisecond: return 1000000.0;
  case u8_microsecond: return 1000.0;
  case u8_nanosecond: return 1.0;
  default: return 1.0;}
}

static void copy_tm2xt(struct tm *tptr,struct U8_XTIME *xt)
{
  xt->u8_sec=tptr->tm_sec;
  xt->u8_min=tptr->tm_min;
  xt->u8_hour=tptr->tm_hour;
  xt->u8_mday=tptr->tm_mday;
  xt->u8_mon=tptr->tm_mon;
  xt->u8_year=tptr->tm_year+1900;
  xt->u8_wday=tptr->tm_wday;
  xt->u8_yday=tptr->tm_yday;
  xt->u8_wnum=(tptr->tm_yday/7);
}

static void copy_xt2tm(struct U8_XTIME *xt,struct tm *tptr)
{
  tptr->tm_sec=xt->u8_sec;
  tptr->tm_min=xt->u8_min;
  tptr->tm_hour=xt->u8_hour;
  tptr->tm_mday=xt->u8_mday;
  tptr->tm_mon=xt->u8_mon;
  tptr->tm_year=xt->u8_year-1900;
  tptr->tm_wday=xt->u8_wday;
  tptr->tm_yday=xt->u8_yday;
}

/* Finer grained time */

U8_EXPORT
/** Returns the number of milliseconds since the epoch.
    This returns a value with whatever accuracy it can get.
    @returns a long long counting milliseconds
*/
long long u8_millitime()
{
#if HAVE_GETTIMEOFDAY
  struct timeval now;
  if (gettimeofday(&now,NULL) < 0)
    return -1;
  else return now.tv_sec*1000+(now.tv_usec/1000);
#elif HAVE_FTIME
    struct timeb now;
#if WIN32
    /* In WIN32, ftime doesn't return an error value.
       ?? We should really do something respectable here.*/
    ftime(&now);
#else 
    if (ftime(&now) < 0) return -1;
    else return now.time*1000+now.millitm;
#endif
#else
    return ((int)time(NULL))*1000;
#endif
}

/* New core functionality */

unsigned int u8_precision_secs[12]=
  {1,3600*24*365,3600*24*30,3600*24,3600,60,1,1,1,1,1,1};

U8_EXPORT u8_xtime u8_init_xtime
  (struct U8_XTIME *xt,time_t tick,u8_tmprec prec,int nsecs,
   int tzoff,int dstoff)
{
  time_t offtick;
  unsigned int prec_secs;
  struct tm _tptr, *tptr=&_tptr;
  if (xt==NULL) xt=u8_malloc(sizeof(struct U8_XTIME));
  memset(xt,0,sizeof(struct U8_XTIME));
  if (tick<0) {
#if HAVE_GETTIMEOFDAY
    struct timeval tv; struct timezone tz;
    if (gettimeofday(&tv,&tz) < 0) {
      u8_graberr(errno,"u8_xlocaltime/gettimeofday",NULL);
      return NULL;}
    tick=tv.tv_sec;
    xt->u8_nsecs=tv.tv_usec*1000;
    if (prec==u8_maxtmprec) xt->u8_prec=u8_microsecond;
#elif HAVE_FTIME
    struct timeb tb;
    if (ftime(&tb)<0) {
      u8_graberr(errno,"u8_xlocaltime/ftime",NULL);
      return NULL;}
    tick=tb.time;
    xt->u8_nsecs=tb.millitm*1000000;
    if (prec==u8_maxtmprec) xt->u8_prec=u8_millisecond;
#else
    if ((tick=time(NULL))<0) {
      u8_graberr(errno,"u8_xlocaltime/time",NULL);
      return NULL;}
    if (prec==u8_maxtmprec) xt->u8_prec=u8_second;
#endif
  }

  /* Offset the tick to get a "fake" gmtime */
  offtick=tick+tzoff; /* +dstoff */

  /* Adjust for the precision, rounding to the middle of the range */
  prec_secs=u8_precision_secs[prec];
  offtick=offtick-(offtick%prec_secs)+prec_secs/2;

  /* Get the broken down representation for the "fake" time. */
#if HAVE_GMTIME_R
  gmtime_r(&offtick,&_tptr);
#else
  u8_lock_mutex(&timefns_lock);
  tptr=gmtime(&offtick);
#endif
  copy_tm2xt(tptr,xt);
#if (!(HAVE_GMTIME_R))
  u8_unlock_mutex(&timefns_lock);
#endif
  /* Set the real time and the offset */
  xt->u8_tick=tick; xt->u8_tzoff=tzoff; xt->u8_dstoff=dstoff;
  /* Initialize the precision */
  if (prec==u8_maxtmprec) prec=u8_second;
  xt->u8_prec=prec;
  /* Don't have redundant nanoseconds */
  if (xt->u8_prec<=u8_second) xt->u8_nsecs=0;
  return xt;
}

U8_EXPORT u8_xtime u8_local_xtime
  (struct U8_XTIME *xt,time_t tick,u8_tmprec prec,int nsecs)
{
  struct tm _tptr, *tptr=&_tptr;
  if (xt==NULL) xt=u8_malloc(sizeof(struct U8_XTIME));
  memset(xt,0,sizeof(struct U8_XTIME));
  if (tick<0) {
#if HAVE_GETTIMEOFDAY
    struct timeval tv; struct timezone tz;
    if (gettimeofday(&tv,&tz) < 0) {
      u8_graberr(errno,"u8_xlocaltime/gettimeofday",NULL);
      return NULL;}
    tick=tv.tv_sec;
    xt->u8_nsecs=tv.tv_usec*1000;
    if ((prec==u8_maxtmprec) || (prec>u8_microsecond))
      prec=u8_microsecond;
#elif HAVE_FTIME
    struct timeb tb;
    if (ftime(&tb)<0) {
      u8_graberr(errno,"u8_xlocaltime/ftime",NULL);
      return NULL;}
    tick=tb.time;
    xt->u8_nsecs=tb.millitm*1000000;
    if ((prec==u8_maxtmprec) || (prec>u8_millisecond))
      prec=u8_millisecond;
#else
    if ((tick=time(NULL))<0) {
      u8_graberr(errno,"u8_xlocaltime/time",NULL);
      return NULL;}
    if ((prec<0) || (prec>u8_second))
      prec=u8_second;
#endif
  }

  /* Get the broken down representation for the "fake" time. */
#if HAVE_LOCALTIME_R
  localtime_r(&tick,&_tptr);
#else
  u8_lock_mutex(&timefns_lock);
  tptr=localtime(&tick);
#endif
  copy_tm2xt(tptr,xt);
#if (!(HAVE_LOCALTIME_R))
  u8_unlock_mutex(&timefns_lock);
#endif
  /* Set the real time and the offset */
  xt->u8_tick=tick;
#if HAVE_TM_GMTOFF
  if (tptr->tm_isdst) {
    xt->u8_tzoff=tptr->tm_gmtoff-3600;
    xt->u8_dstoff=3600;}
  else xt->u8_tzoff=tptr->tm_gmtoff;
#elif ((HAVE_GETTIMEOFDAY) || (HAVE_FTIME))
  if (daylight) {
    xt->u8_tzoff=-timezone-3600;
    xt->u8_dstoff=3600;}
  else xt->u8_tzoff=-timezone;
#else
  xt->u8_tzoff=0;
#endif
  /* Initialize the precision */
  if (prec==u8_maxtmprec) prec=u8_second;
  xt->u8_prec=prec;
  /* Don't have redundant nanoseconds */
  if (xt->u8_prec<=u8_second) xt->u8_nsecs=0;
  return xt;
}

static time_t mktime_x(struct U8_XTIME *xt,int islocal)
{
  time_t tick; struct tm tptr; int localoff;
  memset(&tptr,0,sizeof(struct tm));
  copy_xt2tm(xt,&tptr);
  /* This tells it to figure it out. */
  tick=mktime(&tptr);
  if (tick<0) {
    u8_graberr(-1,"u8_mktime",NULL);
    return tick;}
  xt->u8_wday=tptr.tm_wday;
  xt->u8_yday=tptr.tm_yday;
  if (islocal) {
#if (HAVE_TM_GMTOFF)
    if (tptr.tm_isdst>0) {
      xt->u8_dstoff=3600;
      xt->u8_tzoff=tptr.tm_gmtoff-3600;}
    else {
      xt->u8_dstoff=0;
      xt->u8_tzoff=tptr.tm_gmtoff;}
#else
    if (daylight) xt->u8_dstoff=0;
    xt->u8_tzoff=timezone;
#endif
  }
  else {
#if HAVE_TM_GMTOFF
    localoff=tptr.tm_gmtoff;
#else
    localoff=(timezone+((daylight>1)?(daylight):(daylight*3600)));
#endif
    xt->u8_tick=tick=tick+localoff-(xt->u8_tzoff+xt->u8_dstoff);}
  return tick;
}

U8_EXPORT
/* u8_mktime_x
   Fills out an XTIME structure based on the broken down time representation
   Arguments: a pointer to a tm struct and a time_t value
   Returns: the time_t value or -1 if it failed
*/
time_t u8_mktime(struct U8_XTIME *xt)
{
  return mktime_x(xt,0);
}

U8_EXPORT
/* u8_mktime
   Fills out an XTIME structure based on the broken down time representation
   Arguments: a pointer to a tm struct and a time_t value
   Returns: the time_t value or -1 if it failed
*/
time_t u8_mklocaltime(struct U8_XTIME *xt)
{
  return mktime_x(xt,1);
}

/* Initializing xtime structures */

U8_EXPORT
/* u8_localtime_x
     Arguments: a pointer to a tm struct, a time_t value, and a nanosecond value (or -1)
     Returns: the time_t value or -1 if it failed
*/
time_t u8_localtime_x(struct U8_XTIME *xt,time_t tick,int nsecs)
{
  struct tm _now, *now=&_now;
#if HAVE_LOCALTIME_R
  localtime_r(&tick,&_now);
#else
  u8_lock_mutex(&timefns_lock);
  now=localtime(&tick);
  if (now == NULL) {
    u8_unlock_mutex(&timefns_lock);
    return -1;}
#endif
  copy_tm2xt(now,xt);
#if HAVE_TM_GMTOFF
  if (now->tm_isdst) {
    xt->u8_dstoff=3600;
    xt->u8_tzoff=now->tm_gmtoff-3600;}
  else {
    xt->u8_tzoff=now->tm_gmtoff;
    xt->u8_dstoff=0;}
#endif
#if (!(HAVE_LOCALTIME_R))
  u8_unlock_mutex(&timefns_lock);
#endif
  xt->u8_tick=tick;
  if (nsecs<0) {
    xt->u8_nsecs=0;
    xt->u8_prec=u8_second;}
  else {xt->u8_nsecs=nsecs; xt->u8_prec=u8_nanosecond;}
  return tick;
}
U8_EXPORT
/* u8_localtime
     Arguments: a pointer to a tm struct and a time_t value
     Returns: the time_t value or -1 if it failed
*/
time_t u8_localtime(struct U8_XTIME *xt,time_t tick)
{
  return u8_localtime_x(xt,tick,-1);
}

U8_EXPORT
/* u8_set_xtime_precision
     Arguments: a pointer to an xtime struct and an integer
     Returns: void
*/
void u8_set_xtime_precision(struct U8_XTIME *xt,u8_tmprec prec)
{
  time_t secs=xt->u8_tick;
  if (prec==u8_microsecond) {
    double nsecs=(double)xt->u8_nsecs;
    double usecs=1000*round(nsecs/1000);
    xt->u8_nsecs=usecs;}
  else if (prec==u8_millisecond) {
    double nsecs=(double)xt->u8_nsecs;
    double msecs=1000000*round(nsecs/1000000);
    xt->u8_nsecs=msecs;}
  else if (prec>=u8_second) {
    xt->u8_nsecs=0;
    if (prec==u8_minute) secs=(secs/60)*60;
    else if (prec==u8_hour) secs=(secs/3600)*3600;
    else if (prec==u8_day) secs=(secs/(24*3600))*(24*3600);
    else if (prec==u8_month) secs=(secs/(30*24*3600))*(30*24*3600);
    else  if (prec==u8_year) secs=(secs/(365*24*3600))*(365*24*3600);
    else {}}
  else {}
  xt->u8_prec=prec;
}

U8_EXPORT
/* u8_set_xtime_gmtoff
     Arguments: a pointer to an xtime struct, a tzoff (int), and a dstoff (int)
     Returns: void
*/
void u8_set_xtime_gmtoff(struct U8_XTIME *xt,int tzoff,int dstoff)
{
  int gmtoff=tzoff-dstoff;
  time_t secs=xt->u8_tick+gmtoff;
  u8_init_xtime(xt,((xt->u8_tick)-tzoff),xt->u8_prec,
		xt->u8_nsecs,tzoff,dstoff);
}

U8_EXPORT
/* u8_xtime_plus
     Arguments: a pointer to an xtime struct and an integer
     Returns: void
*/
void u8_xtime_plus(struct U8_XTIME *xt,double delta)
{
  int sign=(delta>=0.0);
  double absval=((delta<0.0) ? (-delta) : (delta));
  double secs=floor(absval);
  double ndelta=(delta-absval)*1000000000.0;
  u8_tmprec precision=xt->u8_prec;
  time_t tick=xt->u8_tick; unsigned int nsecs=xt->u8_nsecs;
  int tzoff=xt->u8_tzoff, dstoff=xt->u8_dstoff;
  tick=((sign) ? (tick+secs) : (tick-secs));
  if ((sign) && (nsecs+ndelta>1000000000)) {
    tick++; nsecs=(nsecs+ndelta)-1000000000;}
  if ((!(sign)) && (xt->u8_nsecs-ndelta<0)) {
    tick--; nsecs=(nsecs-ndelta)+1000000000;}
  u8_init_xtime(xt,tick,precision,nsecs,tzoff,dstoff);
  /* Reset the precision */
  xt->u8_prec=precision;
}

U8_EXPORT
/* u8_xtime_diff
     Arguments: pointers to two xtime structs
     Returns: void
*/
double u8_xtime_diff(struct U8_XTIME *xt,struct U8_XTIME *yt)
{
  double xsecs=((double)(xt->u8_tick))+(0.000000001*xt->u8_nsecs);
  double ysecs=((double)(yt->u8_tick))+(0.000000001*yt->u8_nsecs);
  double totaldiff=xsecs-ysecs;
#if 0 /* This doesn't seem to work, but the idea is to only return a result
	 as precise as is justified. */
  enum u8_timestamp_precision result_precision=
    ((xt->u8_prec<yt->u8_prec) ? (xt->u8_prec) : (yt->u8_prec));
  double precfactor=getprecfactor(result_precision);
  return trunc(precfactor*totaldiff)/precfactor;
#endif
  return totaldiff;
}

U8_EXPORT
/* u8_xtime_to_tptr
     Arguments: pointers to an xtime struct and a tm struct
     Returns: void
*/
time_t u8_xtime_to_tptr(struct U8_XTIME *xt,struct tm *tm)
{
  
  memset(tm,0,sizeof(struct tm));
  copy_xt2tm(xt,tm);
  tm->tm_gmtoff=xt->u8_tzoff;
  if (xt->u8_dstoff) tm->tm_isdst=1;
  return mktime(tm);
}

U8_EXPORT
/* u8_now:
     Arguments: a pointer to an extended time pointer
     Returns: a time_t or -1 if it fails for some reason

  This will try and get the finest precision time it can.
*/
time_t u8_now(struct U8_XTIME *xtp)
{
  if (xtp) {
    u8_xtime result=u8_local_xtime(xtp,-1,u8_femtosecond,0);
    if (result) return result->u8_tick;
    else return -1;}
  else {
    time_t result=time(NULL);
    if (result<0) {
      u8_graberr(errno,"u8_now",NULL); errno=0;}
    return result;}
}

/* Time and strings */

struct TZENTRY {char *name; int tzoff; int dstoff;};
static struct TZENTRY tzones[]= {
  {"Z",0,0},
  {"GMT",0,0},
  {"UT",0,0},
  {"UTC",0,0},  
  {"EST",-5*3600,0},
  {"EDT",-5*3600,3600},
  {"CST",-6*3600,0},
  {"CDT",-6*3600,3600},
  {"MST",-7*3600,0},
  {"MDT",-7*3600,0},
  {"PST",-8*3600,0},
  {"PDT",-8*3600,3600},
  {"CET",1*3600,0},
  {"EET",2*3600,0},
  {NULL,0,0}};

static int lookup_tzname(char *string,int dflt)
{
  struct TZENTRY *zones=tzones;
  while ((*zones).name)
    if (strcasecmp(string,(*zones).name) == 0)
      return (*zones).tzoff;
    else zones++;
  return dflt;
}

U8_EXPORT
/* u8_parse_tzspec:
     Arguments: a string and a default offset
     Returns: an offset from UTC

This uses a built in table but should really use operating system
facilities if they were even remotely standardized.
*/
int u8_parse_tzspec(u8_string s,int dflt)
{
  int hours=0, mins=0, secs=0, sign=1;
  char *offstart=strchr(s,'+');
  if (offstart == NULL) {
    offstart=strchr(s,'-');
    if (offstart) sign=-1;
    else return lookup_tzname(s,dflt);}
  sscanf(offstart+1,"%d:%d:%d",&hours,&mins,&secs);
  return sign*(hours*3600+mins*60+secs);
}

U8_EXPORT
/* u8_use_tzspec:
     Arguments: a pointer to a U8_XTIME structure, a string, and a default offset
     Returns: an offset from UTC

This uses a built in table but should really use operating system
facilities if they were even remotely standardized.
*/
void u8_apply_tzspec(struct U8_XTIME *xt,u8_string s)
{
  int hours=0, mins=0, secs=0, sign=1;
  int dhours=0, dmins=0, dsecs=0, dsign=1;
  char *offstart=strchr(s,'+');
  if (offstart == NULL) {
    offstart=strchr(s,'-');
    if (offstart) sign=-1;}
  if (offstart) {
    char *dstart=strchr(offstart+1,'+');
    sscanf(offstart+1,"%d:%d:%d",&hours,&mins,&secs);
    xt->u8_tzoff=sign*(hours*3600+mins*60+secs);
    if (!(dstart)) {
      dstart=strchr(offstart+1,'-');
      if (dstart) dsign=-1;}
    if (dstart) {
      sscanf(dstart+1,"%d:%d:%d",&dhours,&dmins,&dsecs);
      xt->u8_dstoff=dsign*(dhours*3600+dmins*60+dsecs);}}
  else {
    struct TZENTRY *zones=tzones;
    while ((*zones).name)
      if (strcasecmp(s,(*zones).name) == 0) {
	xt->u8_tzoff=(*zones).tzoff;
	xt->u8_dstoff=(*zones).dstoff;
	return;}
      else zones++;}
}

U8_EXPORT
/* u8_iso8601_to_xtime:
     Arguments: a string and a pointer to a timestamp structure
     Returns: -1 on error, the time as a time_t otherwise

This takes an iso8601 string and fills out an extended time pointer which
includes possible timezone and precision information.
*/
time_t u8_iso8601_to_xtime(u8_string s,struct U8_XTIME *xtp)
{
  char *tzstart;
  int stdpos[]={-1,4,7,10,13,16,19,20}, *pos=stdpos;
  int basicpos[]={-1,4,6,8,11,13,15,17};
  int nsecs=0, n_elts, len=strlen(s);
  if (strchr(s,'/')) return (time_t) -1;
  memset(xtp,0,sizeof(struct U8_XTIME));
  n_elts=sscanf(s,"%04u-%02hhu-%02hhuT%02hhu:%02hhu:%02hhu.%u",
		&xtp->u8_year,&xtp->u8_mon,
		&xtp->u8_mday,&xtp->u8_hour,
		&xtp->u8_min,&xtp->u8_sec,
		&nsecs);
  if ((n_elts == 1)&&(len>4)) {
    /* Assume basic format */
    n_elts=sscanf(s,"%04u%02hhu%02hhuT%02hhu%02hhu%02hhu.%u",
		  &xtp->u8_year,&xtp->u8_mon,
		  &xtp->u8_mday,&xtp->u8_hour,
		  &xtp->u8_min,&xtp->u8_sec,
		  &nsecs);
    pos=basicpos;}
  /* Give up if you can't parse anything */
  if (n_elts == 0) return (time_t) -1;
  /* Adjust month */
  xtp->u8_mon--;
  /* Set precision */
  xtp->u8_prec=n_elts;
  if (n_elts <= 6) xtp->u8_nsecs=0;
  if (n_elts == 7) {
    char *start=s+pos[n_elts], *scan=start;
    int n_digits=0;
    while (isdigit(*scan)) scan++; n_digits=scan-start;
    if (n_digits==9) {
      xtp->u8_nsecs=nsecs;
      xtp->u8_prec=u8_nanosecond;}
    else if (n_digits<9) {
      int multiplier=1, missing=9-n_digits, i=0;
      while (i<missing) {multiplier=multiplier*10; i++;}
      if (n_digits<=3) xtp->u8_prec=u8_millisecond;
      else if (n_digits<=6) xtp->u8_prec=u8_microsecond;
      else xtp->u8_prec=u8_nanosecond;
      xtp->u8_nsecs=nsecs*multiplier;}
    else if (n_digits>9) {
      int divisor=1, extra=n_digits-9, i=0;
      while (i<extra) {divisor=divisor*10; i++;}
      xtp->u8_nsecs=nsecs/divisor;
      xtp->u8_prec=u8_nanosecond;}
    tzstart=scan;}
  else tzstart=s+pos[n_elts];
  if ((tzstart)&&(*tzstart)) {
    u8_apply_tzspec(xtp,tzstart);
    xtp->u8_tick=mktime_x(xtp,0);}
  else xtp->u8_tick=mktime_x(xtp,1);
  return xtp->u8_tick;
}

void xtime_to_iso8601(u8_output ss,struct U8_XTIME *xt,int flags)
{
  char buf[128], tzbuf[128];
  struct U8_XTIME utc, *xtptr;
  char *dash="-", *colon=":";
  u8_tmprec prec=xt->u8_prec;
  if ((flags)&(U8_ISO8601_BASIC)) {dash=""; colon="";}
  if (((flags)&(U8_ISO8601_BASIC))&&(prec>u8_second)) prec=u8_second;
  else if (((flags)&(U8_ISO8601_NOMSECS))&&(prec>u8_second)) prec=u8_second;
  else {}
  if ((flags)&(U8_ISO8601_UTC)&&
      ((xt->u8_tzoff!=0)||(xt->u8_dstoff!=0))) {
    time_t tick=xt->u8_tick;
    u8_init_xtime(&utc,xt->u8_tick,prec,
		  ((prec>u8_second)?(xt->u8_nsecs):(0)),
		  0,0);
    xtptr=&utc;}
  else xtptr=xt;
  switch (prec) {
  case u8_year:
    sprintf(buf,"%04d",xtptr->u8_year); break;
  case u8_month:
    sprintf(buf,"%04d%s%02d",
	    xtptr->u8_year,dash,xtptr->u8_mon+1); break;
  case u8_day:
    sprintf(buf,"%04d%s%02d%s%02d",
	    xtptr->u8_year,dash,xtptr->u8_mon+1,dash,xtptr->u8_mday); break;
  case u8_hour:
    sprintf(buf,"%04d%s%02d%s%02dT%02d",
	    xtptr->u8_year,dash,xtptr->u8_mon+1,dash,xtptr->u8_mday,
	    xtptr->u8_hour);
    break;
  case u8_minute:
    sprintf(buf,"%04d%s%02d%s%02dT%02d%s%02d",
	    xtptr->u8_year,dash,xtptr->u8_mon+1,dash,xtptr->u8_mday,
	    xtptr->u8_hour,colon,xtptr->u8_min);
    break;
    /* u8_maxtmprec should never get this var */
  case u8_maxtmprec:
    u8_log(LOG_WARN,"xtime_to_iso8601","Invalid precision %d for tick=%ld",
	   (int)prec,(long long)(xtptr->u8_tick));
  case u8_second:
    sprintf(buf,"%04d%s%02d%s%02dT%02d%s%02d%s%02d",
	    xtptr->u8_year,dash,xtptr->u8_mon+1,dash,xtptr->u8_mday,
	    xtptr->u8_hour,colon,xtptr->u8_min,colon,xtptr->u8_sec);
    break;
  case u8_millisecond:
    /* The cases here and below don't exist for the basic format */
    sprintf(buf,"%04d-%02d-%02dT%02d:%02d:%02d.%03d",
	    xtptr->u8_year,xtptr->u8_mon+1,xtptr->u8_mday,
	    xtptr->u8_hour,xtptr->u8_min,xtptr->u8_sec,
	    xtptr->u8_nsecs/1000000); break;
  case u8_microsecond:
    sprintf(buf,"%04d-%02d-%02dT%02d:%02d:%02d.%06d",
	    xtptr->u8_year,xtptr->u8_mon+1,xtptr->u8_mday,
	    xtptr->u8_hour,xtptr->u8_min,xtptr->u8_sec,
	    xtptr->u8_nsecs/1000); break;
  case u8_nanosecond: case u8_picosecond: case u8_femtosecond:
    sprintf(buf,"%04d-%02d-%02dT%02d:%02d:%02d.%09d",
	    xtptr->u8_year,xtptr->u8_mon+1,xtptr->u8_mday,
	    xtptr->u8_hour,xtptr->u8_min,xtptr->u8_sec,
	    xtptr->u8_nsecs); break;
  default:
    sprintf(buf,"%04d-%02d-%02dT%02d:%02d:%02d",
	    xtptr->u8_year,xtptr->u8_mon+1,xtptr->u8_mday,
	    xtptr->u8_hour,xtptr->u8_min,xtptr->u8_sec);
    break;}
  if ((flags)&(U8_ISO8601_NOZONE)) tzbuf[0]='\0';
  else if ((xtptr->u8_tzoff) ||  (xtptr->u8_dstoff)) {
    int off=xtptr->u8_tzoff+xtptr->u8_dstoff;
    char *sign=((off<0) ? "-" : "+");
    int tzoff=((off<0) ? (-((off))) : (off));
    int hours=tzoff/3600,
      minutes=(tzoff%3600)/60,
      seconds=tzoff%3600-minutes*60;
    if (seconds)
      sprintf(tzbuf,"%s%d:%02d:%02d",sign,hours,minutes,seconds);
    else sprintf(tzbuf,"%s%d:%02d",sign,hours,minutes);}
  else strcpy(tzbuf,"Z");
  if (xtptr->u8_prec > u8_day)
    u8_printf(ss,"%s%s",buf,tzbuf);
  else u8_printf(ss,"%s",buf);
}

U8_EXPORT
/* u8_xtime_to_iso8601:
     Arguments: a timestamp and a pointer to a string stream
     Returns: -1 on error, the time as a time_t otherwise

This takes xtime pointer and outputs an ISO8601 representation of it,
obeying the precision of the XTIME structure
*/
void u8_xtime_to_iso8601(u8_output ss,struct U8_XTIME *xt)
{
  xtime_to_iso8601(ss,xt,0);
}

U8_EXPORT
/* u8_xtime_to_iso8601_x:
     Arguments: a timestamp, a pointer to a string stream, and a flags arg
     Returns: -1 on error, the time as a time_t otherwise

This takes xtime pointer and outputs an ISO8601 representation of it,
obeying the precision of the XTIME structure

The flags arg provides display options, which can be ORd together:
  U8_ISO8601_BASIC:   use the more compact BASIC format)
  U8_ISO8601_NOZONE:  don't show the timezone information
  U8_ISO8601_NOMSECS: don't show milli/micro/nano seconds
  U8_ISO8601_UTC:     display time as UTC
*/
void u8_xtime_to_iso8601_x(u8_output ss,struct U8_XTIME *xt,int flags)
{
  xtime_to_iso8601(ss,xt,flags);
}

static u8_string month_names[12]=
  {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
static u8_string dow_names[7]=
  {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static int getmonthnum(u8_string s)
{
  int i=0;
  while (i<12)
    if (strncasecmp(s,month_names[i],3)==0)
      return i;
    else i++;
  return -1;
}

U8_EXPORT
/* u8_rfc822_to_xtime:
     Arguments: a string and a pointer to a timestamp structure
     Returns: -1 on error, the time as a time_t otherwise

This takes an rfc822 string and fills out an extended time pointer which
includes possible timezone and precision information.
*/
time_t u8_rfc822_to_xtime(u8_string s,struct U8_XTIME *xtp)
{
  char tzspec[128], dow[128], mon[128]; int n_elts;
  if (strchr(s,'/')) return (time_t) -1;
  tzspec[0]='\0'; dow[0]='\0'; mon[0]='\0';
  memset(xtp,0,sizeof(struct U8_XTIME));
  if (isdigit(*s)) 
    n_elts=sscanf(s,"%hhd %s %d %hhd:%hhd:%hhd %s",
		  &xtp->u8_mday,(char *)&mon,
		  &xtp->u8_year,
		  &xtp->u8_hour,
		  &xtp->u8_min,&xtp->u8_sec,
		  (char *)tzspec);
  else {
    n_elts=sscanf(s,"%s %hhd %s %d %hhd:%hhd:%hhd %s",
		  (char *)&dow,&xtp->u8_mday,(char *)&mon,
		  &xtp->u8_year,
		  &xtp->u8_hour,
		  &xtp->u8_min,&xtp->u8_sec,
		  (char *)tzspec);
    xtp->u8_mon=getmonthnum(mon);}
  /* Give up if you can't parse anything */
  if (n_elts == 0) return (time_t) -1;
  /* Set precision */
  xtp->u8_prec=u8_second;
  xtp->u8_nsecs=0;
  if (tzspec[0]) u8_apply_tzspec(xtp,tzspec);
  xtp->u8_tick=u8_mktime(xtp); xtp->u8_nsecs=0;
  return xtp->u8_tick;
}

U8_EXPORT
/* u8_xtime_to_rfc822:
     Arguments: a timestamp and a pointer to a string stream
     Returns: -1 on error, the time as a time_t otherwise

This takes an iso8601 string and fills out an extended time pointer which
includes possible timezone and precision information.
*/
void u8_xtime_to_rfc822(u8_output ss,struct U8_XTIME *xtp)
{
  struct U8_XTIME asgmt;
  char buf[128];
  u8_init_xtime(&asgmt,xtp->u8_tick,xtp->u8_prec,xtp->u8_nsecs,0,0);
  sprintf(buf,"%s, %d %s %04d %02d:%02d:%02d",
	  dow_names[asgmt.u8_wday],
	  asgmt.u8_mday,
	  month_names[asgmt.u8_mon],
	  (asgmt.u8_year),
	  asgmt.u8_hour,asgmt.u8_min,asgmt.u8_sec);
  u8_printf(ss,"%s +0000",buf);
}

U8_EXPORT
/* u8_xtime_to_rfc822_x:
     Arguments: a timestamp and a pointer to a string stream
     Returns: -1 on error, the time as a time_t otherwise

This takes an iso8601 string and fills out an extended time pointer which
includes possible timezone and precision information.
*/
void u8_xtime_to_rfc822_x(u8_output ss,struct U8_XTIME *xtp,int zone,int flags)
{
  struct U8_XTIME inzone;
  char buf[128];
  if (zone==0)
    u8_init_xtime(&inzone,xtp->u8_tick,xtp->u8_prec,xtp->u8_nsecs,0,0);
  else if (zone==-1)
    u8_init_xtime
      (&inzone,xtp->u8_tick,xtp->u8_prec,xtp->u8_nsecs,
       xtp->u8_tzoff,xtp->u8_dstoff);
  else if (zone==1)
    u8_local_xtime(&inzone,xtp->u8_tick,xtp->u8_prec,xtp->u8_nsecs);
  else u8_init_xtime(&inzone,xtp->u8_tick,xtp->u8_prec,xtp->u8_nsecs,zone,0);
  sprintf(buf,"%s, %d %s %04d %02d:%02d:%02d",
	  dow_names[inzone.u8_wday],
	  inzone.u8_mday,
	  month_names[inzone.u8_mon],
	  (inzone.u8_year),
	  inzone.u8_hour,inzone.u8_min,inzone.u8_sec);
  if (flags&U8_RFC822_NOZONE) u8_puts(ss,buf);
  else {
    int minus=(inzone.u8_tzoff+inzone.u8_dstoff)<0;
    int off=((minus)?(-(inzone.u8_tzoff+inzone.u8_dstoff)):
	     (inzone.u8_tzoff+inzone.u8_dstoff));
    int hroff=off/3600, minoff=off%3600/60;
    u8_printf(ss,"%s %s%02d%02d",buf,((minus)?"-":"+"),hroff,minoff);}
}

/* printf handling for time related values */

static u8_string time_printf_handler
  (u8_output s,char *cmd,u8_string buf,int bufsz,va_list *args)
{
  struct U8_XTIME xt;
  if (strchr(cmd,'*'))
    /* Uses the current time, doesn't consume an argument. */
    if (strchr(cmd+1,'G'))
      u8_init_xtime(&xt,-1,u8_femtosecond,0,0,0);
    else u8_local_xtime(&xt,-1,u8_femtosecond,0);
  else if (strchr(cmd,'X'))
    /* The argument is an U8_XTIME pointer. */
    if (strchr(cmd,'G')) {
      /* Convert the result to GMT/UTC */
      struct U8_XTIME *xtarg=va_arg(*args,u8_xtime);
      memcpy(&xt,xtarg,sizeof(struct U8_XTIME));}
    else {
      /* Use the time intrinsic to the XTIME structure. */
      struct U8_XTIME *xtarg=va_arg(*args,u8_xtime);
      memcpy(&xt,xtarg,sizeof(struct U8_XTIME));}
  /* Otherwise, the argument is time_t value. */
  else if (strchr(cmd,'G')) 
    /* Use it as GMT. */
    u8_init_xtime(&xt,va_arg(*args,time_t),u8_second,0,0,0);
  else /* Display it as local time */
    u8_local_xtime(&xt,va_arg(*args,time_t),u8_second,0);
  if (strchr(cmd,'i')) {
    /* With this argument, specify the precision to be used. */
    u8_tmprec precision=u8_second;
    if (strchr(cmd,'Y')) precision=u8_year;
    else if (strchr(cmd,'D')) precision=u8_day;
    else if (strstr(cmd,"HM")) precision=u8_minute;
    else if (strstr(cmd,"MS")) precision=u8_millisecond;
    else if (strchr(cmd,'H')) precision=u8_hour;
    else if (strchr(cmd,'S')) precision=u8_second;
    else if (strchr(cmd,'U')) precision=u8_microsecond;
    else if (strchr(cmd,'M')) precision=u8_month;
    else if (strchr(cmd,'N')) precision=u8_nanosecond;
    xt.u8_prec=precision;
    u8_xtime_to_iso8601(s,&xt);
    return NULL;}
  else if (strchr(cmd,'l')) {
    /* With 'l' output the date together with the time. */
    struct tm tmp; copy_xt2tm(&xt,&tmp);
    strftime(buf,bufsz,"%d%b%Y@%H:%M:%S%z",&tmp);
    return buf;}
  else {
    /* With no flags, just output the time. */
    struct tm tmp; copy_xt2tm(&xt,&tmp);
    strftime(buf,bufsz,"%H:%M:%S",&tmp);
    return buf;}
}

/* UUID generation and related functions */

long long u8_uuid_node=-1;

static unsigned long long flip64(unsigned long long _w)
{ return ((((u8ull)(_w&(0xFF))) << 56) |
	  (((u8ull)(_w&(0xFF00))) << 40) |
	  (((u8ull)(_w&(0xFF0000))) << 24) |
	  (((u8ull)(_w&(0xFF000000))) << 8) |
	  (((u8ull)(_w>>56)) & 0xFF) |
	  (((u8ull)(_w>>40)) & 0xFF00) |
	  (((u8ull)(_w>>24)) & 0xFF0000) |
	  (((u8ull)(_w>>8)) & 0xFF000000));}

#define knuth_hash(i)  ((((u8ull)(i))*2654435761LL)%(0x10000000LL))

#if HAVE_UUID_GENERATE_TIME
static unsigned long long generate_nodeid(int temp)
{
  uuid_t tmp; unsigned char buf[16];
  unsigned long long id;
  if ((!(temp))&&(u8_uuid_node>=0))
    return u8_uuid_node;
  uuid_generate_time(tmp);
  memcpy(buf,tmp,16);
  id=u8_uuid_nodeid(buf);
  if (!(temp)) u8_uuid_node=id;
  return id;
}
#else
static unsigned long long generate_nodeid(int temp)
{
  time_t now=time(NULL);
  unsigned long long id;
  if ((!(temp))&&(u8_uuid_node>=0))
    return u8_uuid_node;
  id=(((u8ull)u8_random(65536))<<32)|
    (((u8ull)knuth_hash((unsigned int)now))<<16)|
    ((u8ull)(u8_random(65536)));
  if (!(temp)) u8_uuid_node=id;
  return id;
}
#endif

/* The main constructor function */

static u8_uuid consuuid
   (struct U8_XTIME *xtime,unsigned long long nodeid,short clockid,
    u8_uuid buf)
{
  unsigned long long nanotick=
    122192928000000000LL+
    (((unsigned long long)xtime->u8_tick)*10000000)+
    xtime->u8_nsecs/100;
  unsigned char *data=((buf)?(buf):(u8_malloc(16*sizeof(unsigned char))));
  unsigned long long low, high;
  unsigned long long *lowp=(unsigned long long *)(data+8);
  unsigned long long *highp=(unsigned long long *)(data);
  low=(((u8ull)nodeid)&((u8ull)0xFFFFFFFFFFFFLL))|
    /* THis is the 14-bit clockid */
    (((u8ull)(clockid|0x1000))<<48)|
    /* This is the variant code */
    (0x8000000000000000LL);
  high=
    ((nanotick&       0xFFFFFFFF)<<32)|
    ((nanotick&   0xFFFF00000000LL)>>16)|
    ((nanotick&0xFFF000000000000LL)>>48)|
    /* This is the version (time-based) */
    (0x1000);
#if WORDS_BIGENDIAN
  *highp=((u8ull)high); *lowp=((u8ull)low);
#else
  *highp=((u8ull)(flip64(high))); *lowp=((u8ull)(flip64(low)));
#endif
    return (u8_uuid) data;
}

/* UUID functions */

U8_EXPORT u8_uuid u8_consuuid
  (struct U8_XTIME *xtime,long long nodeid,short clockid,u8_uuid buf)
{
  struct U8_XTIME timebuf;
  if (xtime==NULL) {
    u8_now(&timebuf); xtime=&timebuf;}
  if (clockid<0) clockid=u8_random(256*4*16);
  if (nodeid<0) {
    if (u8_uuid_node<0) 
      nodeid=generate_nodeid(0);
    else nodeid=u8_uuid_node;}
  if (u8_uuid_node<0) u8_uuid_node=nodeid;
  return consuuid(xtime,nodeid,clockid,buf);
}

U8_EXPORT u8_string u8_uuidstring(u8_uuid uuid,u8_byte *buf)
{
  if (buf==NULL) buf=u8_malloc(37);
  sprintf(buf,
	  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	  ((u8uint)(uuid[0])),((u8uint)(uuid[1])),((u8uint)(uuid[2])),
	  ((u8uint)(uuid[3])),((u8uint)(uuid[4])),((u8uint)(uuid[5])),
	  ((u8uint)(uuid[6])),((u8uint)(uuid[7])),((u8uint)(uuid[8])),
	  ((u8uint)(uuid[9])),((u8uint)(uuid[10])),((u8uint)(uuid[11])),
	  ((u8uint)(uuid[12])),((u8uint)(uuid[13])),((u8uint)(uuid[14])),
	  ((u8uint)(uuid[15])));
  return buf;
}

U8_EXPORT u8_uuid u8_parseuuid(u8_string buf,u8_uuid uuid)
{
  unsigned char *ubuf=((uuid)?(uuid):(u8_malloc(16)));
  u8ull high, low, *highp=((u8ull *)ubuf), *lowp=((u8ull *)(ubuf+8));
  u8ull head, nodelow; unsigned int nodehigh, b1, b2, b3; 
  int items=sscanf(buf,"%8llx-%4x-%4x-%4x-%4x%8llx",
		   &head,&b1,&b2,&b3,&nodehigh,&nodelow);
  if (items!=6) {
    if (!(uuid)) u8_free(ubuf);
    return NULL;}
  high=((((u8ull)head)<<32))|(b1<<16)|(b2);
  low=((((u8ull)b3)<<48))|((((u8ull)nodehigh)<<32))|(nodelow);
#if WORDS_BIGENDIAN
  *highp=high; *lowp=high;
#else
  *highp=flip64(high); *lowp=flip64(low);
#endif
  return (u8_uuid) buf;
}

#define TIMEUUIDP(uuid) ((uuid[6]&0xF0)==0x10)

static long long get_nanotick(unsigned long long high)
{
  return
    ((high&0x0FFFFFFF00000000LL)>>32)|
    ((high&         0xFFFF0000LL)<<16)|
    ((high&              0xFFFLL)<<48);
}

U8_EXPORT long long u8_uuid_nodeid(u8_uuid uuid)
{
  unsigned char *ubuf=uuid;
#if WORDS_BIGENDIAN
  u8ull *lowp=(u8ull *)(ubuf+8), low=*lowp;
#else
  u8ull *lowp=(u8ull *)(ubuf+8), low=flip64(*lowp);
#endif  
  if (TIMEUUIDP(ubuf))
    return low&(0xFFFFFFFFFFFFLL);
  else return -1;
}

U8_EXPORT long long u8_uuid_timestamp(u8_uuid uuid)
{
  unsigned char *ubuf=uuid;
#if WORDS_BIGENDIAN
  u8ull *highp=(u8ull *)(ubuf), high=*highp;
#else
  u8ull *highp=(u8ull *)(ubuf), high=flip64(*highp);
#endif  
  if (TIMEUUIDP(ubuf))
    return get_nanotick(high);
  else return -1;
}

U8_EXPORT time_t u8_uuid_tick(u8_uuid uuid)
{
  unsigned char *ubuf=uuid;
#if WORDS_BIGENDIAN
  u8ull *highp=(u8ull *)(ubuf), high=*lowp;
#else
  u8ull *highp=(u8ull *)(ubuf), high=flip64(*highp);
#endif  
  u8ull nanotick=get_nanotick(high);
  if (TIMEUUIDP(ubuf)) {
    time_t tick=((nanotick-122192928000000000LL)/100000000);
    if (tick<0) return -1;
    else return tick;}
  else return -1;
}

U8_EXPORT struct U8_XTIME *u8_uuid_xtime(u8_uuid uuid,struct U8_XTIME *xt)
{
  unsigned char *ubuf=uuid;
#if WORDS_BIGENDIAN
  u8ull *highp=(u8ull *)(ubuf), high=*highp;
#else
  u8ull *highp=(u8ull *)(ubuf), high=flip64(*highp);
#endif  
  u8ull nanotick=get_nanotick(high);
  if (!(TIMEUUIDP(ubuf))) return NULL;
  if (!(xt)) xt=u8_alloc(struct U8_XTIME);
  u8_init_xtime
    (xt,(time_t)((nanotick-122192928000000000LL)/10000000),
     u8_nanosecond,(nanotick%10)*100,0,0);
  return xt;
}

/* Generating fresh UUIDs */
static short clockid=-1;
static long long last_nanotick=-1;

U8_EXPORT u8_uuid freshuuid(u8_uuid uuid)
{
  struct U8_XTIME buf; long long nanotick;
  if (clockid<0) clockid=u8_random(256*4*16);
  if (u8_uuid_node<0) u8_uuid_node=generate_nodeid(0);
  u8_now(&buf);
  nanotick=122192928000000000LL+
    (((unsigned long long)buf.u8_tick)*10000000)+
    buf.u8_nsecs/100;
  if (last_nanotick>nanotick) clockid=(clockid+1)%(256*4*16);
  last_nanotick=nanotick;
  return consuuid(&buf,u8_uuid_node,clockid,uuid);
}

U8_EXPORT u8_uuid u8_getuuid(u8_uuid buf)
{
#if HAVE_UUID_GENERATE_TIME
  uuid_t tmp; 
  uuid_generate_time(tmp);
  if (buf==NULL) buf=u8_malloc(16);
  memcpy(buf,tmp,16);
  return (u8_uuid)buf;
#else
  return freshuuid(buf);
#endif
}

/* Initialization functions */

U8_EXPORT void u8_init_timefns_c()
{
  u8_printf_handlers['t']=time_printf_handler;
#if U8_THREADS_ENABLED
  u8_init_mutex(&timefns_lock);
#endif
  u8_register_source_file(_FILEINFO);
}
