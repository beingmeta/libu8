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
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"

#include <string.h>

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if ((HAVE_SYS_TYPES_H)&&(HAVE_UTIME_H))
#include <sys/types.h>
#include <utime.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#include <stdio.h>

#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#else
# define dirent direct
# if defined(HAVE_SYS_NDIR_H)
#  include <sys/ndir.h>
# endif
# if defined(HAVE_SYS_DIR_H)
#  include <sys/dir.h>
# endif
#endif

#if HAVE_PWD_H
#include <pwd.h>
static u8_string get_uname(u8_uid uid)
{
  struct passwd *entry=getpwuid(uid);
  return u8_fromlibc(entry->pw_name);
}
U8_EXPORT u8_string u8_username(u8_uid uid)
{
  return get_uname(uid);
}
U8_EXPORT u8_uid u8_getuid(u8_string name)
{
  char *s = u8_tolibc(name);
  struct passwd _info, *info=NULL;
  char buf[2048];
  int rv = getpwnam_r(s,&_info,buf,2048,&info);
  u8_free(s);
  if (rv<0) {
    errno=0;
    return -1;}
  else if (info) {
    u8_uid uid = info->pw_uid;
    errno=0;
    return uid;}
  else {
    errno=0;
    return -1;}
}
#else
static u8_string get_uname(u8_uid ignored)
{
  return u8_strdup("kilroy");
}
U8_EXPORT u8_string u8_username(u8_uid uid)
{
  return get_uname(uid);
}
U8_EXPORT u8_uid u8_getuid(u8_string name)
{
  return (u8_uid) -1;
}
#endif


#if HAVE_GRP_H
#include <grp.h>
static u8_string get_grname(u8_gid gid)
{
  struct group *entry=getgrgid(gid);
  return u8_fromlibc(entry->gr_name);
}
U8_EXPORT u8_string u8_groupname(u8_gid gid)
{
  return get_grname(gid);
}
U8_EXPORT u8_gid u8_getgid(u8_string name)
{
  char *s = u8_tolibc(name);
  struct group _info, *info=NULL;
  char buf[2048];
  int rv = getgrnam_r(s,&_info,buf,2048,&info);
  u8_free(s);
  if (rv<0) {
    errno=0;
    return -1;}
  else if (info) {
    u8_uid uid = info->gr_gid;
    errno=0;
    return uid;}
  else {
    errno=0;
    return -1;}
}
#else
static u8_string get_grname(u8_uid ignored)
{
  return u8_strdup("illuminati");
}
U8_EXPORT u8_string u8_username(u8_uid uid)
{
  return get_grname(uid);
}
U8_EXPORT u8_uid u8_getuid(u8_string name)
{
  return (u8_uid) -1;
}
#endif


/* File predicates */

#if HAVE_SYS_STAT_H
U8_EXPORT int u8_directoryp(u8_string filename)
{
  struct stat status;
  if (stat(filename,&status) < 0) {
    U8_CLEAR_ERRNO();
    return 0;}
  else {
    return ((status.st_mode&S_IFMT)==S_IFDIR);}
}
#ifdef S_IFLNK
U8_EXPORT int u8_symlinkp(u8_string filename)
{
  struct stat status;
  if (lstat(filename,&status) < 0) {
    U8_CLEAR_ERRNO();
    return 0;}
  else {
    return ((status.st_mode&S_IFMT)==S_IFLNK);}
}
#endif
#ifdef S_IFSOCK
U8_EXPORT int u8_socketp(u8_string filename)
{
  struct stat status;
  if (stat(filename,&status) < 0) {
    U8_CLEAR_ERRNO(); return 0;}
  else {
    return ((status.st_mode&S_IFMT)==S_IFSOCK);}
}
#endif
#endif

#if HAVE_ACCESS
static int try_access(u8_string filename,int mode)
{
  if (access(filename,mode)==0)
    return 0;
  else {
    u8_string path=u8_localpath(filename);
    int rv=access(path,mode);
    u8_free(path);
    U8_CLEAR_ERRNO();
    return rv;}
}

static int file_existsp(u8_string filename)
{
  int retval=try_access(filename,F_OK);
  if (retval==0)
    return 1;
  else {
    return 0;}
}
static int file_writablep(u8_string filename)
{
  int retval=try_access(filename,W_OK);
  if (retval==0) return 1;
  else {
    U8_CLEAR_ERRNO();
    return 0;}
}
static int file_readablep(u8_string filename)
{
  int retval=try_access(filename,R_OK);
  if (retval==0) return 1;
  else {
    U8_CLEAR_ERRNO();
    return 0;}
}
static int file_executablep(u8_string filename)
{
  int retval=try_access(filename,X_OK);
  if (retval==0) return 1;
  else {
    U8_CLEAR_ERRNO();
    return 0;}
}
#else
#if HAVE_SYS_STAT_H
static int file_existsp(u8_string filename)
{
  struct stat status;
  if (stat(filename,&status) < 0) {
    U8_CLEAR_ERRNO();
    return 0;}
  else return 1;
}
#else
static int file_existsp(u8_string filename)
{
  FILE *f=fopen(filename,"r");
  U8_CLEAR_ERRNO();
  if (f) {
    fclose(f);
    U8_CLEAR_ERRNO();
    return 1;}
  else return 0;
}
#endif

static int file_readablep(u8_string filename)
{
  FILE *f=fopen(filename,"r");
  U8_CLEAR_ERRNO();
  if (f) {
    fclose(f);
    U8_CLEAR_ERRNO();
    return 1;}
  else return 0;
}
static int file_writablep(u8_string filename)
{
  FILE *f=fopen(filename,"a");
  U8_CLEAR_ERRNO();
  if (f) {
    fclose(f);
    U8_CLEAR_ERRNO();
    return 1;}
  else return 0;
}
#endif

U8_EXPORT int u8_file_existsp(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_existsp(filename);
  if (retval<0) {
    if (errno) {
      u8_log(LOG_WARN,"u8_file_existsp","Error for '%s' (%s)",
             lpath,strerror(errno));
      errno=0;}
    else u8_log(LOG_WARN,"u8_file_existsp","Error for '%s'",lpath);
    retval=0;}
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_readablep(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_readablep(filename);
  if (retval<0) {
    if (errno) {
      u8_log(LOG_WARN,"u8_file_readablep","Error for '%s' (%s)",
             lpath,strerror(errno));
      errno=0;}
    else u8_log(LOG_WARN,"u8_file_readablep","Error for '%s'",lpath);
    retval=0;}
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_executablep(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_executablep(filename);
  if (retval<0) {
    if (errno) {
      u8_log(LOG_WARN,"u8_file_executablep","Error for '%s' (%s)",
             lpath,strerror(errno));
      errno=0;}
    else u8_log(LOG_WARN,"u8_file_executablep","Error for '%s'",lpath);
    retval=0;}
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_writablep(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_writablep(filename);
  if (retval<0) {
    if (errno) {
      u8_log(LOG_WARN,"u8_file_writablep","Error for '%s' (%s)",
             lpath,strerror(errno));
      errno=0;}
    else u8_log(LOG_WARN,"u8_file_writeablep","Error for '%s'",lpath);
    retval=0;}
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_regularp(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat info;
  int retval=stat(lpath,&info);
  if (retval<0) {
    if (errno) {
      u8_log(LOG_WARN,"u8_file_normalp","Error for '%s' (%s)",
             lpath,strerror(errno));
      errno=0;}
    else u8_log(LOG_WARN,"u8_file_readablep","Error for '%s'",lpath);
    return 0;}
  u8_free(lpath);
  if ( ( (info.st_mode&S_IFMT) == S_IFDIR ) ||
       ( (info.st_mode&S_IFMT) == S_IFSOCK ) )
    return 0;
  else if ( (info.st_mode&S_IFMT) == S_IFREG )
    return 1;
  else return 0;
}

/* File info */

U8_EXPORT time_t u8_file_ctime(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    int saved_errno = errno; errno=0;
    u8_graberr(saved_errno,"u8_file_ctime",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_ctime;}
}

U8_EXPORT time_t u8_file_mtime(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberrno("u8_file_mtime",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_mtime;}
}

#if ((HAVE_SYS_TYPES_H)&&(HAVE_UTIME_H))
U8_EXPORT time_t u8_set_mtime(u8_string filename,time_t mtime)
{
  char *lpath=u8_localpath(filename);
  struct utimbuf tbuf={-1,mtime};
  int rv=utime(lpath,&tbuf);
  if (rv<0) {
    u8_graberrno("u8_set_mtime",lpath);
    return (time_t)-1;}
  else {
    u8_free(lpath);
    return mtime;}
}
#else
U8_EXPORT time_t u8_set_mtime(u8_string filename,time_t mtime)
{
  return u8_reterr(u8_NotImplemented,"u8_set_mtime",u8_localpath(filename));
}
#endif

U8_EXPORT time_t u8_file_atime(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberrno("u8_file_atime",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_atime;}
}

#if ((HAVE_SYS_TYPES_H)&&(HAVE_UTIME_H))
U8_EXPORT time_t u8_set_atime(u8_string filename,time_t atime)
{
  char *lpath=u8_localpath(filename);
  struct utimbuf tbuf={atime,-1};
  int rv=utime(lpath,&tbuf);
  if (rv<0) {
    u8_graberrno("u8_set_atime",lpath);
    return (time_t)-1;}
  else {
    u8_free(lpath);
    return atime;}
}
#else
U8_EXPORT time_t u8_set_mtime(u8_string filename,time_t mtime)
{
  return u8_reterr(u8_NotImplemented,"u8_set_mtime",u8_localpath(filename));
}
#endif

U8_EXPORT int u8_file_mode(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberrno("u8_file_mode",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_mode;}
}

U8_EXPORT ssize_t u8_file_size(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberrno("u8_file_size",u8_strdup(filename));
    u8_free(lpath);
    return (ssize_t) -1;}
  else {
    u8_free(lpath);
    return (ssize_t) fileinfo.st_size;}
}

U8_EXPORT u8_string u8_file_owner(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberrno("u8_file_owner",u8_strdup(filename));
    u8_free(lpath);
    return NULL;}
  else {
    u8_free(lpath);
    return get_uname(fileinfo.st_uid);}
}

/* Readlink */

U8_EXPORT u8_string u8_readlink(u8_string filename,int absolute)
{
  char *lpath=u8_localpath(filename);
  char *linkname; ssize_t linklen;
  struct stat linkinfo;
  if (lstat(lpath,&linkinfo)<0) {
    u8_graberrno("u8_readlink",u8_strdup(filename));
    u8_free(lpath);
    return NULL;}
  else {
    linkname=u8_malloc(linkinfo.st_size+16);
    linklen=readlink(lpath,linkname,linkinfo.st_size+1);
    if (linklen<0) {
      u8_graberrno("u8_readlink",u8_strdup(filename));
      u8_free(lpath); u8_free(linkname);
      return NULL;}
    if (linklen>linkinfo.st_size+15) {
      u8_seterr(_("Link grew between lstat and readlink"),"u8_readlink",
                u8_strdup(filename));
      u8_free(lpath); u8_free(linkname);
      return NULL;}
    else if (absolute) {
      u8_string dir=u8_dirname(filename), abspath;
      if (dir==NULL) dir=u8_getcwd();
      linkname[linklen]='\0';
      abspath=u8_abspath(linkname,dir);
      u8_free(lpath); u8_free(linkname); u8_free(dir);
      return abspath;}
    else {
      u8_string result;
      linkname[linklen]='\0';
      result=u8_fromlibc(linkname);
      u8_free(lpath); u8_free(linkname);
      return result;}}
}

U8_EXPORT u8_string u8_getlink(u8_string filename,int absolute)
{
  char *lpath=u8_localpath(filename);
  char *linkname; ssize_t linklen;
  struct stat linkinfo;
  if (lstat(lpath,&linkinfo)<0) {
    u8_free(lpath);
    errno=0;
    return NULL;}
  else {
    linkname=u8_malloc(linkinfo.st_size+16);
    linklen=readlink(lpath,linkname,linkinfo.st_size+1);
    if (linklen<0) {
      errno = 0;
      u8_free(lpath); u8_free(linkname);
      return NULL;}
    if (linklen>linkinfo.st_size+15) {
      u8_seterr(_("Link grew between lstat and readlink"),"u8_readlink",
                u8_strdup(filename));
      u8_free(lpath); u8_free(linkname);
      return NULL;}
    else if (absolute) {
      u8_string dir=u8_dirname(filename), abspath;
      if (dir==NULL) dir=u8_getcwd();
      linkname[linklen]='\0';
      abspath=u8_abspath(linkname,dir);
      u8_free(lpath); u8_free(linkname); u8_free(dir);
      errno = 0;
      return abspath;}
    else {
      u8_string result;
      linkname[linklen]='\0';
      result=u8_fromlibc(linkname);
      u8_free(lpath); u8_free(linkname);
      errno = 0;
      return result;}}
}

/* Searching for files */

static void buildname(u8_byte *buf,u8_string name,int namelen,
                      u8_string start,u8_string end,
                      u8_string ins,u8_string instoo)
{
  if ((instoo)&&(ins==NULL)) {ins=instoo; instoo=NULL;}
  if (instoo) {
    strncpy(buf,start,ins-start);
    strncpy(buf+(ins-start),name,namelen);
    strncpy(buf+((ins-start)+namelen),ins+1,instoo-(ins+1));
    strncpy(buf+((ins-start)+namelen+(instoo-(ins+1))),name,namelen);
    strncpy(buf+((ins-start)+namelen+(instoo-(ins+1))+namelen),
            instoo+1,end-(instoo+1));
    buf[(ins-start)+namelen+(instoo-ins)+namelen+end-instoo]='\0';}
  else if (ins) {
    strncpy(buf,start,ins-start);
    strncpy(buf+(ins-start),name,namelen);
    strncpy(buf+((ins-start)+namelen),ins+1,end-(ins+1));
    buf[(end-start)+(namelen-1)]='\0';}
  else {
    u8_byte *namestart=buf+(end-start);
    strncpy(buf,start,end-start);
    if ((namestart==buf)||(namestart[-1]!='/'))
      *namestart++='/';
    strncpy(namestart,name,namelen);
    *(namestart+namelen)='\0';}
}

U8_EXPORT u8_string u8_find_file(u8_string name,u8_string searchpath,
                                 int (*testp)(u8_string))
{
  int namelen=strlen(name), buflen=strlen(searchpath)+namelen*2+4;
  const u8_byte *start=searchpath, *end, *ins, *instoo=NULL;
  u8_byte *buf=u8_malloc(buflen);
  u8_string probename;
  memset(buf,0,buflen);
  if (testp==NULL) testp=u8_file_existsp;
  while ((end=u8_strchrs(start,":, ;",1))) {
    ins=strchr(start,'%');
    if ((ins==NULL) || (ins>end)) ins=NULL;
    if (ins) instoo=strchr(ins+1,'%');
    if (instoo>end) instoo=NULL;
    buildname(buf,name,namelen,start,end,ins,instoo);
    probename=u8_abspath(buf,NULL);
    if (testp(probename)) {
      u8_free(probename);
      return buf;}
    else {
      u8_free(probename);
      start=end+1;}}
  end=start+strlen(start);
  ins=strchr(start,'%');
  if ((ins==NULL) || (ins>end)) ins=NULL;
  if (ins) instoo=strchr(ins+1,'%');
  if (instoo>end) instoo=NULL;
  buildname(buf,name,namelen,start,end,ins,instoo);
  probename=u8_abspath(buf,NULL);
  if (testp(probename)) {
    u8_free(probename);
    return buf;}
  else {u8_free(probename); u8_free(buf); return NULL;}
}

/* File manipulation functions */

U8_EXPORT int u8_removefile(u8_string filename)
{
  char *abspath=u8_localpath(filename);
  int retval=remove(abspath);
  if (retval<0)
    u8_graberrno("u8_removefile",u8_strdup(filename));
  u8_free(abspath);
  return retval;
}

U8_EXPORT int u8_movefile(u8_string from,u8_string to)
{
  char *absfrom=u8_localpath(from);
  char *absto=u8_localpath(to);
  int retval=rename(absfrom,absto);
  if (retval<0) u8_graberrno("u8_movefile",u8_mkstring("%s->%s",from,to));
  u8_free(absfrom); u8_free(absto);
  return retval;
}

#if HAVE_SYMLINK
U8_EXPORT int u8_linkfile(u8_string from,u8_string to)
{
  char *absfrom=u8_localpath(from);
  char *absto=u8_localpath(to);
  int retval=symlink(absfrom,absto);
  u8_free(absfrom); u8_free(absto);
  if (retval<0) u8_graberrno("u8_linkfile",u8_mkstring("%s->%s",from,to));
  return retval;
}
#else
U8_EXPORT int u8_linkfile(u8_string from,u8_string to)
{
  return u8_reterr("No WIN32 symlink","u8_linkfile",NULL);
}
#endif

/* Changing modes */

U8_EXPORT int u8_chmod(u8_string name,mode_t mode)
{
  struct stat fileinfo;
  char *localized=u8_localpath(name);
  int retval=-1;
  if (stat(localized,&fileinfo)<0) {
    u8_graberrno("u8_chmod",u8_strdup(name));}
  else if (fileinfo.st_mode==mode)
    retval=0;
  else {
    retval=chmod(localized,mode);
    if (retval>=0) retval=1;}
  u8_free(localized);
  return retval;
}

/* Setting access generally */

U8_EXPORT int u8_set_access
(u8_string filename,u8_string owner,u8_string group,mode_t mode)
{
  struct stat fileinfo;
  char *localized=u8_localpath(filename);
  if (stat(localized,&fileinfo)<0) {
    u8_graberrno("u8_chmod",u8_strdup(filename));
    u8_free(localized);
    return -1;}
  u8_uid uid = (owner) ? (u8_getuid(owner)) : (-1);
  u8_gid gid = (group) ? (u8_getgid(group)) : (-1);
  int rv = 0;
  int changes = (uid>=0) + (gid>=0) + (mode>=0);
  if ( ( (gid >= 0) && (fileinfo.st_gid != gid) ) ||
       ( (uid >= 0) && (fileinfo.st_uid != uid) ) )
    rv = chown(localized,uid,gid);
  if (rv<0) {
    u8_graberrno("u8_set_access/ownership",u8_strdup(filename));}
  else if ( (mode >= 0) && ((rv=chmod(localized,mode))<0) ) {
    u8_graberrno("u8_set_access/mode",u8_strdup(filename));}
  else NO_ELSE;
  u8_free(localized);
  if (rv<0)
    return rv;
  else return changes;
}

U8_EXPORT int u8_set_access_x
(u8_string filename,u8_uid uid,u8_gid gid,mode_t mode)
{
  struct stat fileinfo;
  char *localized=u8_localpath(filename);
  if (stat(localized,&fileinfo)<0) {
    u8_graberrno("u8_chmod",u8_strdup(filename));
    u8_free(localized);
    return -1;}
  int rv = 0;
  int changes = (uid>=0) + (gid>=0) + (mode>=0);
  if ( ( (gid >= 0) && (fileinfo.st_gid != gid) ) ||
       ( (uid >= 0) && (fileinfo.st_uid != uid) ) )
    rv = chown(localized,uid,gid);
  if (rv<0) {
    u8_graberrno("u8_set_access/ownership",u8_strdup(filename));}
  else if ( (mode >= 0) && ((rv=chmod(localized,mode))<0) ) {
    u8_graberrno("u8_set_access/mode",u8_strdup(filename));}
  else NO_ELSE;
  u8_free(localized);
  if (rv<0)
    return rv;
  else return changes;
}

/* Manipulating directories */

mode_t u8_default_dir_mode =
  (S_IFDIR |
   S_IRUSR | S_IWUSR | S_IXUSR |
   S_IRGRP | S_IWGRP | S_IXGRP |
   S_IROTH | S_IXOTH);

U8_EXPORT int u8_mkdir(u8_string name,mode_t mode)
{
  if (u8_directoryp(name)) {
    if (mode>=0) {
      char *localized = u8_localpath(name);
      int rv = chmod(localized,mode);
      if (rv<0) {
        u8_graberrno("u8_mkdir/chmod",localized);}
      else u8_free(localized);
      return rv;}
    return 0;}
  else {
    mode_t use_mode = (mode<0) ? (u8_default_dir_mode) : (mode|S_IFDIR);
    const char *localized=u8_localpath(name);
    int retval=mkdir(localized,use_mode);
    if (retval<0) {
      u8_graberrno("u8_mkdir",localized);}
    else u8_free(localized);
    if (retval<0) return -1; else return 1;}
}

static int mkdirs(u8_string dirname,mode_t mode)
{
  if ((dirname[0]=='\0')||
      ((dirname[0]=='/')&&(dirname[1]=='\0')))
    return 0;
  else if (u8_directoryp(dirname)) return 0;
  else if (strchr(dirname,'/')) {
    u8_string parent=u8_dirname(dirname);
    int made=mkdirs(parent,mode), retval;
    u8_free(parent);
    if (made<0) return made;
    else retval=u8_mkdir(dirname,mode);
    if (retval<0) return retval;
    else return made+retval;}
  else return u8_mkdir(dirname,mode);
}

U8_EXPORT int u8_mkdirs(u8_string arg,mode_t mode)
{
  u8_string rpath=u8_realpath(arg,NULL);
  size_t len=strlen(rpath);
  u8_string dirname=
    ((rpath[len-1]=='/')?((u8_string)(u8_strdup(rpath))):(u8_dirname(rpath)));
  int retval=mkdirs(dirname,mode);
  u8_free(dirname);
  u8_free(rpath);
  return retval;
}

U8_EXPORT int u8_rmdir(u8_string arg)
{
  u8_string rpath=u8_realpath(arg,NULL);
  if (u8_directoryp(rpath)) {
    char *localized=u8_localpath(rpath);
    int retval=rmdir(localized);
    if (rpath!=((u8_string)localized)) u8_free(localized);
    u8_free(rpath);
    if (retval<0) return retval;
    else return 1;}
  else {
    u8_free(rpath);
    return 0;}
}

/* Temporary directories */

#if HAVE_MKDTEMP
U8_EXPORT u8_string u8_tempdir(u8_string template)
{
  char *buf=u8_localpath(template), *dir; u8_string result;
  /* Add any missing X's, up to 6.  On some platforms, you
     can have more than six.  */
  int len=strlen(buf); int add_x=6;
  char *scan=buf+(len-1); while (*scan=='X') {
    add_x--; scan--;}
  if (add_x) {
    char *newbuf=u8_malloc(len+add_x+1), *write=newbuf+len;;
    memcpy(newbuf,buf,len);
    int i=0; while (i<add_x) {write[i++]='X';}
    write[add_x]='\0';
    if (((u8_string)buf)!=template) u8_free(buf);
    buf=newbuf;}
  else if (((u8_string)buf)==template) buf=u8_strdup(buf);
  else NO_ELSE;
  dir=mkdtemp(buf);
  if (dir) {
    result=u8_fromlibc(dir);
    if (dir!=buf) u8_free(buf);
    if (result!=((u8_string)dir)) u8_free(dir);
    return result;}
  else {
    u8_free(buf);
    return NULL;}
}
#endif

/* Scanning directories */

#if HAVE_DIRENT_H
static u8_string *getfiles_helper(u8_string dirname,int which,int ret_fullpath)
{
  DIR *dp; struct dirent *entry;
  u8_string *results=u8_alloc_n(8,u8_string);
  int n_results=0, max_results=8;
  u8_string abspath=u8_abspath(dirname,NULL);
  char *dirpath=u8_localpath(abspath);
  u8_free(abspath); dp=opendir(dirpath);
  if (dp==NULL) {
    u8_graberrno("u8_getfiles",u8_strdup(dirname));
    u8_free(dirpath);
    return NULL;}
  else while ((entry=readdir(dp))) {
      struct stat fileinfo; char *name=entry->d_name;
      if (!(((name[0]=='.')&&(name[1]=='\0'))||
            ((name[0]=='.')&&(name[1]=='.')&&(name[2]=='\0')))) {
        u8_string fullpath=u8_mkpath(dirpath,name); int forced=0;
        if (stat(fullpath,&fileinfo)<0) {
          u8_free(fullpath); continue;}
	/* If you're listing links and the file isn't a link, check
	   if the reference itself is a link and force a return. */
	if ((which&U8_LIST_LINKS) && (!(S_ISLNK(fileinfo.st_mode))) &&
	    (!((which&U8_LIST_DIRS)||(which&U8_LIST_FILES)))) {
	  struct stat linkinfo;
	  if (lstat(fullpath,&linkinfo)<0) {
	    u8_free(fullpath); continue;}
	  else if (S_ISLNK(linkinfo.st_mode)) forced=1;
	  else NO_ELSE;}
        if ((which == 0) || (forced) ||
	    ((which&U8_LIST_DIRS) && (S_ISDIR(fileinfo.st_mode))) ||
	    ((which&U8_LIST_FILES) && (S_ISREG(fileinfo.st_mode))) ||
	    ((which&U8_LIST_LINKS) && (S_ISLNK(fileinfo.st_mode))) ||
	    ((which&U8_LIST_MAGIC) && 
	     (!((S_ISDIR(fileinfo.st_mode))||
		(S_ISREG(fileinfo.st_mode))||
		(S_ISLNK(fileinfo.st_mode)))))) {
          if (n_results+1>=max_results) {
            results=u8_realloc_n(results,max_results*2,u8_string);
            max_results=max_results*2;}
          if (ret_fullpath)
	    results[n_results++]=u8_strdup(fullpath);
          else results[n_results++]=u8_fromlibc(entry->d_name);
          u8_free(fullpath);}
        else u8_free(fullpath);}}
  results[n_results++]=NULL;
  closedir(dp);
  u8_free(dirpath);
  return results;
}
static int remove_tree_helper(u8_string dirname)
{
  DIR *dp; struct dirent *entry; int count=0;
  u8_string abspath=u8_abspath(dirname,NULL);
  char *dirpath=u8_localpath(abspath);
  u8_free(abspath); dp=opendir(dirpath);
  if (dp==NULL) {
    u8_graberrno("u8_remove_tree",u8_strdup(dirname));
    u8_clear_errors(1);
    u8_free(dirpath);
    return 0;}
  else while ((entry=readdir(dp))) {
      struct stat fileinfo; char *name=entry->d_name;
      if (!(((name[0]=='.')&&(name[1]=='\0'))||
            ((name[0]=='.')&&(name[1]=='.')&&(name[2]=='\0')))) {
	u8_string elt=u8_fromlibc(entry->d_name);
	u8_string fullpath=u8_mkpath(dirpath,elt);
	if (stat(fullpath,&fileinfo)<0) {
	  u8_free(elt); u8_free(fullpath); continue;}
        if ((fileinfo.st_mode)&(S_IFDIR)) {
          int retval=remove_tree_helper(fullpath);
          if (retval>=0) {
            count=count+retval;
            retval=u8_rmdir(fullpath);
            if (retval>=0) count++;
	    u8_free(elt); u8_free(fullpath);}
          else {
            u8_graberrno("u8_remove_tree",fullpath);
            u8_clear_errors(1);}}
        else if (((fileinfo.st_mode)&(S_IFLNK))||
                 ((fileinfo.st_mode)&(S_IFREG))||
                 ((fileinfo.st_mode)&(S_IFSOCK))) {
          int retval=u8_removefile(fullpath);
          if (retval<0) {
            u8_graberrno("u8_remove_tree",fullpath);
            u8_clear_errors(1);}
          else {
            count++;
	    u8_free(elt);
	    u8_free(fullpath);}}
	else {u8_free(elt); u8_free(fullpath);}}}
  closedir(dp);
  u8_free(dirpath);
  return count;
}
#else
static u8_string *getfiles_helper(u8_string dirname,int which,int fullpath)
{
  u8_seterr(_("No directory lists"),"getfiles_helper",u8_strdup(dirname));
  return NULL;
}
static int remove_tree_helper(u8_string dirname) { return 0; }
#endif

U8_EXPORT int u8_rmtree(u8_string dirname)
{
  int count=remove_tree_helper(dirname);
  if (count>=0) {
    int retval=u8_rmdir(dirname);
    if (retval<0) return retval;
    else return count+1;}
  else return count;
}

U8_EXPORT u8_string *u8_readdir(u8_string dirname,int which,int fullpath)
{
  return getfiles_helper(dirname,which,fullpath);
}

U8_EXPORT u8_string *u8_getfiles(u8_string dirname,int fullpath)
{
  return getfiles_helper(dirname,U8_LIST_FILES,fullpath);
}

U8_EXPORT u8_string *u8_getdirs(u8_string dirname,int fullpath)
{
  return getfiles_helper(dirname,U8_LIST_DIRS,fullpath);
}

/* Init function */

void init_filefns_c()
{
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
