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

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/u8stringfns.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8fileio.h"
#include "libu8/u8xtime.h"
#include "libu8/u8timefns.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

#ifndef MAX_U8RUN_JOBID_LEN
#define MAX_U8RUN_JOBID_LEN 512
#endif
#ifndef MAX_JOB_PREFIX_LEN
#define MAX_U8RUN_PREFIX_LEN 512
#endif

/* U8RUN state variables */

static u8_mutex status_lock;

u8_string u8run_jobid=NULL;
u8_string u8run_prefix=NULL;
u8_string u8run_statfile=NULL;
u8_string u8run_status=NULL;

int status_fileno=-1;

/* u8run state */

U8_EXPORT void u8run_setup(u8_string jobid,u8_string prefix,u8_string statfile)
{
  u8run_jobid=jobid;
  u8run_prefix=prefix;
  u8run_statfile=statfile;
}

static int use_status_file(int reopen)
{
  int err = errno;
  if (errno) {
    u8_log(LOGWARN,"DangingErrno","%s","Before u8se_status_file");
    errno=0;}
  u8_lock_mutex(&(status_lock));
  if (status_fileno>0) {
    if (reopen) {
      int rv = close(status_fileno);
      if (rv<0) {
        u8_log(LOGERR,"use_status_file","Couldn't close fileno=%d %s errno=%d:%s",
               status_fileno,u8run_statfile,err,u8_strerror(err));
        u8_unlock_mutex(&(status_lock));
        return -1;}
      else status_fileno=-1;}
    else return status_fileno;}
  if (u8run_statfile==NULL) {
    u8_unlock_mutex(&(status_lock));
    return -1;}
  else {
    char *filename = u8_tolibc(u8run_statfile);
    int fileno = open(filename,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP);
    if (fileno<0) {
      if (((u8_string)filename) != u8run_statfile) u8_free(filename);
      u8_log(LOGERR,"u8run_statfile","Couldn't open %s errno=%d:%s",
             filename,err,u8_strerror(err));
      u8_unlock_mutex(&(status_lock));
      return -1;}
    status_fileno=fileno;
    if (((u8_string)filename) != u8run_statfile) u8_free(filename);
    return fileno;}
}

static void release_status_file()
{
  u8_unlock_mutex(&status_lock);
}

U8_EXPORT ssize_t u8run_set_status(u8_string status)
{
  struct U8_XTIME now;
  U8_FIXED_OUTPUT(buf,1024);
  u8_string appid = u8_appid();
  time_t tick = u8_now(&now); 
  u8_printf(bufout,"%lld",(long long)getpid());
  if (appid) u8_printf(bufout," %s",appid);
  u8_xtime_to_iso8601(bufout,&now);
  if (status) u8_printf(bufout," %s",status);
  int fileno = use_status_file(0), err = 0;
  if (fileno>=0) {
    if (lseek(fileno,0,SEEK_SET)<0) goto err_exit;
    ssize_t outlen = u8_outbuf_len(bufout);
    ssize_t written = u8_writeall(fileno,u8_outbuf_bytes(bufout),outlen);
    if (ftruncate(fileno,outlen)<0) goto err_exit;
    if (fdatasync(fileno)<0) goto err_exit;
    if (u8run_status) u8_free(u8run_status);
    u8run_status=u8_strdup(status);
    release_status_file();
    return outlen;}
 err_exit:
  err = errno; errno=0;
  u8_log(LOGPANIC,"StatusWriteFailed",
         "Writing to %d:%s with errno %d (%s)",
         fileno,u8run_statfile,err,u8_strerror(err));
  release_status_file();
  return -1;
}

void init_status_c()
{
  u8_register_source_file(_FILEINFO);

  u8_init_mutex(&status_lock);
  
  u8_string jobid=u8_getenv("U8RUNJOBID");
  u8_string prefix=u8_getenv("U8RUNJOBPREFIX");
  u8_string statfile=getenv("U8RUNSTATUSFILE");
  if ( (jobid) || (prefix) || (statfile) )
    u8run_setup(jobid,prefix,statfile);
}
