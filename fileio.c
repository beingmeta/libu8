/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
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
#include "libu8/u8subscribe.h"

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if USE_FLOCK
#include <sys/file.h>
U8_EXPORT int u8_lock_fd(int fd,int for_write)
{
  int rv = flock(fd, LOCK_EX);
  if (rv<0) u8_graberr(errno,"u8_lock_fd",NULL);
  return rv;
}
U8_EXPORT int u8_unlock_fd(int fd)
{
  int rv = flock(fd, LOCK_UN);
  if (rv<0) u8_graberr(errno,"u8_unlock_fd",NULL);
  return rv;
}
#elif USE_F_SETLK
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
#elif WIN32
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

U8_EXPORT off_t u8_rawpos(int fd)
{
  off_t pos=lseek(fd,0,SEEK_CUR);
  if (pos<0) {
    u8_log(LOGWARN,u8_strerror(errno),"Got errno %d",errno);
    U8_CLEAR_ERRNO();}
  return pos;
}

U8_EXPORT int u8_get_blocking(int fd)
{
  int flags=fcntl(fd,F_GETFL);
  if (flags<0) {
    u8_graberr(errno,"u8_set_blocking",NULL);
    return -1;}
  else return (!(flags&O_NONBLOCK));
}

U8_EXPORT int u8_set_blocking(int fd,int block)
{
  int oflags=fcntl(fd,F_GETFL), rv;
  if (oflags<0) {
    u8_graberr(errno,"u8_set_blocking",NULL);
    return -1;}
  if (block)
    rv=fcntl(fd,F_SETFL,(oflags&(~O_NONBLOCK)));
  else rv=fcntl(fd,F_SETFL,(oflags|O_NONBLOCK));
  if (rv<0) {
    u8_graberr(errno,"u8_set_blocking",NULL);
    return -1;}
  else return (!(oflags&O_NONBLOCK));
}

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
    int fd=fileno(f);
    int rv = u8_lock_fd(fd,for_write);
    u8_free(lpath);
    if (rv<0) {
      fclose(f);
      return NULL;}
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

U8_EXPORT int u8_open_file(u8_string path,int flags,mode_t mode,int lock_flags)
{
  char *lpath=u8_localpath(path);
  int fd=open(lpath,flags,mode);
  int for_write = ((flags)&(O_WRONLY|O_RDWR));
  u8_free(lpath);
  if (fd<0) u8_graberr(errno,"u8_open_locked_file/open",u8_strdup(path));
  if (lock_flags>0) {
    int rv = u8_lock_fd(fd,for_write);
    if (rv<0) {
      u8_graberr(errno,"u8_open_locked_file/lock",u8_strdup(path));
      close(fd);
      return -1;}}
  return fd;
}
U8_EXPORT int u8_open_fd(u8_string path,int flags,mode_t mode)
{
  return u8_open_file(path,flags,mode,0);
}

U8_EXPORT int u8_close_file(int fd)
{
  return close(fd);
}
U8_EXPORT int u8_close_fd(int fd) { return u8_close_file(fd); }

/* Utility functions */

U8_EXPORT ssize_t u8_writeall(int sock,const unsigned char *data,size_t len)
{
  size_t bytes_to_write = len;
  while (bytes_to_write > 0) {
    ssize_t delta=write(sock,data,bytes_to_write);
    if (delta<0)
      if (errno==EAGAIN) continue;
      else return delta;
    else if (delta==0)
      if (errno==EAGAIN) continue;
      else if (bytes_to_write>0)
	return -1;
      else return len;
    else {
      data=data+delta;
      bytes_to_write -= delta;}}
  return len;
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
    u8_graberrno("u8_subscribe",u8_strdup(filename));
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
    u8_graberrno("u8_renew",u8_fromlibc(s->filename));
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

void init_fileio_c()
{
#if U8_THREADS_ENABLED
  u8_init_mutex(&subscription_lock);
#endif
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
