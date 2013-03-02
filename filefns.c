/* -*- Mode: C; -*- */

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
static u8_string get_uname(uid_t uid)
{
  struct passwd *entry=getpwuid(uid);
  return u8_fromlibc(entry->pw_name);
}
#else
static u8_string get_uname(int ignored)
{
  return u8_strdup("kilroy");
}
#endif

/* File predicates */

#if HAVE_SYS_STAT_H
U8_EXPORT int u8_directoryp(u8_string filename)
{
  struct stat status;
  if (stat(filename,&status) < 0) {
    errno=0; return 0;}
  else {
    return (status.st_mode&S_IFDIR);}
}
#endif

#if HAVE_ACCESS
static int file_existsp(u8_string filename)
{
  int retval=access(filename,F_OK);
  if (retval==0) return 1;
  else {errno=0; return 0;}
}
static int file_readablep(u8_string filename)
{
  int retval=access(filename,R_OK);
  if (retval==0) return 1;
  else {errno=0; return 0;}
}
static int file_writablep(u8_string filename)
{
  int retval=access(filename,W_OK);
  if (retval==0) return 1;
  else {errno=0; return 0;}
}
#else
#if HAVE_SYS_STAT_H
static int file_existsp(u8_string filename)
{
  struct stat status;
  if (stat(filename,&status) < 0) {
    errno=0;
    return 0;}
  else return 1;
}
#else
static int file_existsp(u8_string filename)
{
  FILE *f=fopen(filename,"r");
  if (f) {
    fclose(f);
    return 1;}
  else return 0;
}
#endif

static int file_readablep(u8_string filename)
{
  FILE *f=fopen(filename,"r");
  if (f) {
    fclose(f);
    return 1;}
  else return 0;
}
static int file_writablep(u8_string filename)
{
  FILE *f=fopen(filename,"a");
  if (f) {
    fclose(f);
    return 1;}
  else return 0;
}
#endif

U8_EXPORT int u8_file_existsp(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_existsp(filename);
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_readablep(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_readablep(filename);
  u8_free(lpath);
  return retval;
}
U8_EXPORT int u8_file_writablep(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  int retval=file_writablep(filename);
  u8_free(lpath);
  return retval;
}

/* File info */

U8_EXPORT time_t u8_file_ctime(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberr(-1,"u8_file_ctime",u8_strdup(filename));
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
    u8_graberr(-1,"u8_file_mtime",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_mtime;}
}

U8_EXPORT time_t u8_file_atime(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberr(-1,"u8_file_atime",u8_strdup(filename));
    u8_free(lpath);
    return (time_t) -1;}
  else {
    u8_free(lpath);
    return fileinfo.st_atime;}
}

U8_EXPORT int u8_file_mode(u8_string filename)
{
  char *lpath=u8_localpath(filename);
  struct stat fileinfo;
  if (stat(lpath,&fileinfo)<0) {
    u8_graberr(-1,"u8_file_mode",u8_strdup(filename));
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
    u8_graberr(-1,"u8_file_size",u8_strdup(filename));
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
    u8_graberr(-1,"u8_file_owner",u8_strdup(filename));
    u8_free(lpath);
    return NULL;}
  else {
    u8_free(lpath);
    return get_uname(fileinfo.st_uid);}
}

/* Searching for files */

static void buildname(u8_byte *buf,u8_string name,int namelen,
		      u8_byte *start,u8_byte *end,u8_byte *ins)
{
  if (ins) {
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
  int namelen=strlen(name), buflen=strlen(searchpath)+namelen+2;
  u8_byte *start=searchpath, *end, *ins, *buf=u8_malloc(buflen);
  u8_string probename;
  if (testp==NULL) testp=u8_file_existsp;
  while ((end=strchr(start,':'))) {
    ins=strchr(start,'%');
    if ((ins==NULL) || (ins>end)) ins=NULL;
    buildname(buf,name,namelen,start,end,ins);
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
  buildname(buf,name,namelen,start,end,ins);
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
  u8_free(abspath);
  if (retval<0) u8_graberr(-1,"u8_removefile",u8_strdup(filename));
  return retval;
}

U8_EXPORT int u8_movefile(u8_string from,u8_string to)
{
  char *absfrom=u8_localpath(from);
  char *absto=u8_localpath(to);
  int retval=rename(absfrom,absto);
  if (retval<0) u8_graberr(-1,"u8_movefile",u8_mkstring("%s->%s",from,to));
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
  if (retval<0) u8_graberr(-1,"u8_linkfile",u8_mkstring("%s->%s",from,to));
  return retval;
}
#else
U8_EXPORT int u8_linkfile(u8_string from,u8_string to)
{
  return u8_reterr("No WIN32 symlink","u8_linkfile",NULL);
}
#endif

/* Making directories */

U8_EXPORT int u8_mkdir(u8_string name,mode_t mode)
{
  if (u8_directoryp(name)) return 0;
  else {
    const char *localized=u8_localpath(name);
    int retval=mkdir(localized,mode);
    u8_free((void *)localized);
    if (retval<0) return retval; else return 1;}
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
    if (made<0) return made;
    else retval=u8_mkdir(dirname,mode);
    if (retval<0) return retval;
    else return made+retval;}
  else return u8_mkdir(dirname,mode);
}

U8_EXPORT int u8_mkdirs(u8_string arg,mode_t mode)
{
  int len=strlen(arg);
  u8_string dirname=
    ((arg[len-1]=='/')?(u8_strdup(arg)):
     (u8_dirname(arg)));
  int retval=mkdirs(dirname,mode);
  u8_free(dirname);
  return retval;
}

U8_EXPORT int u8_rmdir(u8_string arg)
{
  if (u8_directoryp(arg)) {
    char *localized=u8_localpath(arg);
    int retval=rmdir(localized);
    if (arg!=((u8_string)localized)) u8_free(localized);
    if (retval<0) return retval;
    else return 1;}
  else return 0;
}

#if HAVE_MKDTEMP
U8_EXPORT int u8_tempdir(u8_string template)
{
  char *buf=u8_localpath(template), *dir; u8_string result;
  /* Add any missing X's, up to 6.  On some platforms, you
     can have more than six.  */
  int len=strlen(buf); int add_x=6;
  char *scan=buf+(len-1); while (*scan==='X') {
    add_x--; scan--;}
  if (add_x) {
    char *newbuf=u8_alloc(len+add_x+1), *write=newbuf+len;;
    strcpy(newbuf,buf,len);
    int i=0; while (i<add_x) {write[i++]='X';}
    write[add_x]='\0';
    if (buf!==template) u8_free(buf);
    buf=newbuf;}
  else if (buf===template) buf=u8_strdup(buf);
  else {}
  dir=mkdtemp(buf);
  result=u8_fromlibc(dir);
  if (result!=dir) u8_free(dir);
  return result;
}
#endif

/* Scanning directories */

#define JUST_FILES 1
#define JUST_DIRS 2

#if HAVE_DIRENT_H
static u8_string *getfiles_helper(u8_string dirname,int which,int ret_fullpath)
{
  DIR *dp; struct dirent *entry;
  u8_string *results=u8_alloc_n(8,u8_string);
  int n_results=0, max_results=8;
  char *abspath=u8_abspath(dirname,NULL);
  char *dirpath=u8_localpath(abspath);
  u8_free(abspath); dp=opendir(dirpath);
  if (dp==NULL) {
    u8_graberr(-1,"u8_getfiles",u8_strdup(dirname));
    u8_free(dirpath);
    return NULL;}
  else while ((entry=readdir(dp))) {
    struct stat fileinfo;
    char *fullpath=u8_mkpath(dirpath,entry->d_name);
    if (stat(fullpath,&fileinfo)<0) {
      u8_free(fullpath); continue;}
    if (((which==JUST_DIRS) && (S_ISDIR(fileinfo.st_mode))) ||
	((which==JUST_FILES) && (S_ISREG(fileinfo.st_mode)))) {
      if (n_results+1>=max_results) {
	results=u8_realloc_n(results,max_results*2,u8_string);
	max_results=max_results*2;}
      if (ret_fullpath)
	results[n_results++]=u8_fromlibc(fullpath);
      else results[n_results++]=u8_fromlibc(entry->d_name);
      u8_free(fullpath);}
    else u8_free(fullpath);}
  results[n_results++]=NULL;
  closedir(dp);
  u8_free(dirpath);
  return results;
}
#else
static u8_string *getfiles_helper(u8_string dirname,int which,int fullpath)
{
  u8_seterr(_("No directory lists"),"getfiles_helper",u8_strdup(dirname));
  return NULL;
}
#endif

U8_EXPORT u8_string *u8_getfiles(u8_string dirname,int fullpath)
{
  return getfiles_helper(dirname,JUST_FILES,fullpath);
}


U8_EXPORT u8_string *u8_getdirs(u8_string dirname,int fullpath)
{
  return getfiles_helper(dirname,JUST_DIRS,fullpath);
}

/* Init function */

U8_EXPORT void u8_init_filefns_c()
{
  u8_register_source_file(_FILEINFO);
}
