/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2014 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.
*/

#include "libu8/u8source.h"
#include "libu8/libu8.h"

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8stringfns.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"
#include "libu8/u8fileio.h"

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if WIN32
#include <io.h>
#include <sys/locking.h>
U8_EXPORT int u8_lock_fd(int fd,int for_write)
{
#if 0
  return _locking(fd,_LK_LOCK,0);
#endif
  return 1;
}
U8_EXPORT int u8_unlock_fd(int fd)
{
#if 0
  return _locking(fd,_LK_UNLCK,0);
#endif
  return 1;
}
#elif FD_WITH_FILE_LOCKING
U8_EXPORT int u8_lock_fd(int fd,int for_write)
{
  struct flock lock_data;
  int retval;
  lock_data.l_whence=0; lock_data.l_start=0; lock_data.l_len=0;
  lock_data.l_type=((for_write) ? (F_WRLCK) : (F_RDLCK));
  lock_data.l_pid=getpid();
  retval=fcntl(fd,F_SETLK,&lock_data);
  if (retval == 0) {errno=0;}
  return retval;
}
U8_EXPORT int u8_unlock_fd(int fd)
{
  struct flock lock_data;
  int retval;
  lock_data.l_whence=0; lock_data.l_start=0; lock_data.l_len=0;
  lock_data.l_type=F_UNLCK; lock_data.l_pid=getpid();
  retval=fcntl(fd,F_SETLK,&lock_data);
  errno=0;
  return retval;
}
#else
U8_EXPORT int u8_lock_fd(int fd,int for_write)
{
  return 1;
}
U8_EXPORT int u8_unlock_fd(int fd)
{
  return 1;
}
#endif

/* Opening files */

U8_EXPORT FILE *u8_fopen_locked(u8_string path,char *mode)
{
  char *lpath=u8_localpath(path);
  FILE *f=fopen(lpath,mode);
  int for_write=((strchr(mode,'w'))||(strchr(mode,'a'))||(strchr(mode,'+')));
  if (f == NULL) {
    perror("fopen"); errno=0; u8_free(lpath);
    return NULL;}
  else {
    int fd=fileno(f); u8_lock_fd(fd,for_write); u8_free(lpath);
    return f;}
}

U8_EXPORT FILE *u8_fopen(u8_string path,char *mode)
{
  char copied_mode[16]; int lock=0, mlen=strlen(mode);
  if (mlen>15) return NULL;
  else {
    int i=0, j=0;
    while (i < mlen)
      if (mode[i] == 'l') {lock=1; i++;}
      else {
        copied_mode[j++]=mode[i++];}
    copied_mode[j]='\0';}
  if (lock) return u8_fopen_locked(path,copied_mode);
  else {
    char *lpath=u8_localpath(path);
    FILE *f=fopen(lpath,mode);
    u8_free(lpath);
    return f;}
}

U8_EXPORT int u8_fclose(FILE *f)
{
  int retval=u8_unlock_fd(fileno(f));
  fclose(f);
  return retval;
}

U8_EXPORT int u8_open_fd(u8_string path,int flags,mode_t mode)
{
  char *lpath=u8_localpath(path);
  int fd=open(lpath,flags,mode);
  u8_free(lpath);
  if (fd<0) u8_graberr(errno,"u8_open_fd",u8_strdup(path));
  return fd;
}

U8_EXPORT int u8_close_fd(int fd)
{
  return close(fd);
}

/* Subscription */

static struct U8_SUBSCRIPTION *subscriptions=NULL;
#if U8_THREADS_ENABLED
static u8_mutex subscription_lock;
#endif

static int renewing_all=0;

U8_EXPORT u8_subscription u8_subscribe
  (u8_string filename,int (*fn)(u8_string,void *),void *data)
{
  char *lpath=u8_localpath(filename);
  struct U8_SUBSCRIPTION *scan; struct stat fileinfo;
  u8_lock_mutex(&subscription_lock);
  scan=subscriptions;
  while (scan)
    if (strcmp(lpath,scan->filename)==0) {
      if (fn!=NULL) scan->callback=fn;
      if (data!=NULL) scan->callback_data=data;
      u8_free(lpath);
      u8_unlock_mutex(&subscription_lock);
      return scan;}
    else scan=scan->next;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberr(-1,"u8_subscribe",u8_strdup(filename));
    u8_free(lpath);
    u8_unlock_mutex(&subscription_lock);
    return NULL;}
  else {
    int retval=fn(lpath,data);
    if (retval<0) return NULL;
    else {
      u8_subscription newrec=u8_alloc(struct U8_SUBSCRIPTION);
      newrec->filename=lpath;
      newrec->callback=fn;
      newrec->callback_data=data;
      newrec->mtime=fileinfo.st_mtime;
      newrec->next=subscriptions;
      subscriptions=newrec;
      u8_unlock_mutex(&subscription_lock);
      return newrec;}}
}

U8_EXPORT int u8_renew(u8_subscription s)
{
  struct stat fileinfo;
  if (s->callback==NULL) return 0;
  else if (stat(s->filename,&fileinfo)<0) {
    u8_graberr(-1,"u8_renew",u8_fromlibc(s->filename));
    return -1;}
  else if (s->mtime<fileinfo.st_mtime) {
    return s->callback(s->filename,s->callback_data);}
  else return 0;
}

U8_EXPORT int u8_renew_all()
{
  if (renewing_all) return 0;
  else {
    struct stat fileinfo; int count=0;
    U8_SUBSCRIPTION *scan;
    u8_lock_mutex(&subscription_lock);
    if (renewing_all) {
      u8_unlock_mutex(&subscription_lock);
      return 0;}
    else {
      renewing_all=1; scan=subscriptions;
      u8_unlock_mutex(&subscription_lock);}
    while (scan)
      if (scan->callback==NULL) scan=scan->next;
      else if (stat(scan->filename,&fileinfo)<0) {
        u8_log(LOG_NOTICE,u8_strerror(errno),"u8_renew_all",scan->filename);
        errno=0; scan=scan->next;}
      else if (scan->mtime<fileinfo.st_mtime) {
        if (scan->callback(scan->filename,scan->callback_data)<0) {
          u8_condition cond; u8_context cxt=NULL; u8_string details=NULL;
          if (u8_poperr(&cond,&cxt,&details))
            u8_log(LOG_ERR,cond,"Renew error for %s",scan->filename);
          if (details) u8_free(details);}
        else count++;
        scan=scan->next;}
      else scan=scan->next;
  return count;}
}

/* Init function */

U8_EXPORT void u8_init_fileio_c()
{
#if U8_THREADS_ENABLED
  u8_init_mutex(&subscription_lock);
#endif
  u8_register_source_file(_FILEINFO);
}
