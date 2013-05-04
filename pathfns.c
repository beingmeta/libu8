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

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8pathfns.h"
#include "libu8/u8filefns.h"

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#ifndef HOMEDIR_ROOT
#define HOMEDIR_ROOT "/home/"
#endif

#if HAVE_PWD_H
#include <pwd.h>
u8_string get_homedir(char *uname)
{
  if (uname == NULL) {
    struct passwd *entry=getpwuid(getuid());
    if (errno) errno=0;
    if (entry)
      return u8_fromlibc(entry->pw_dir);
    else return NULL;}
  else {
    struct passwd *entry=getpwnam(uname);
    if (errno) errno=0;
    if (entry)
      return u8_fromlibc(entry->pw_dir);
    else return NULL;}
}
#else
static u8_string get_homedir(u8_string user)
{
  if (user == NULL)
    return u8_strdup(HOMEDIR_ROOT);
  else return u8_string_append(HOMEDIR_ROOT,user,NULL);
}
#endif

static int ends_in_slashp(u8_string s)
{
  u8_byte *end=strrchr(s,'/');
  if ((end) && (end[1]=='\0')) return 1;
  else return 0;
}

U8_EXPORT u8_string u8_getcwd()
{
  char *wd=getcwd(NULL,0); u8_string uwd;
  if (wd==NULL) {
    u8_graberr(-1,"u8_getcwd",NULL);
    return NULL;}
  else uwd=u8_fromlibc(wd);
  u8_free(wd);
  return uwd;
}

U8_EXPORT int u8_setcwd(u8_string dirname)
{
  char *lpath=u8_localpath(dirname);
  if (lpath==NULL) return -1;
  else if (chdir(lpath)<0) {
    u8_graberr(-1,"u8_setcwd",u8_fromlibc(lpath));
    u8_free(lpath);
    return -1;}
  else return 1;
}

U8_EXPORT char *u8_localpath(u8_string path)
{
  return u8_2libc(u8_abspath(path,NULL));
}

U8_EXPORT u8_string u8_mkpath(u8_string dir,u8_string base)
{
  u8_string result;
  int dirlen=strlen(dir), baselen=strlen(base);
  unsigned int newlen=dirlen+baselen+2;
  if (dirlen==0) return u8_strdup(base);
  newlen=(((newlen%4)==0)?(newlen):(((newlen/4)+1)*4));
  result=u8_malloc(newlen);
  strcpy(result,dir);
  if (result[dirlen-1]=='/') {result[dirlen-1]='\0'; dirlen--;}
  while (1) {
    if (((base[0])=='\0')||(result[0]=='\0')) break;
    else if ((base[0]=='.')&&(base[1]=='/')) base=base+2;
    else if ((base[0]=='.')&&(base[1]=='.')&&(base[2]=='/')) {
      u8_byte *lastslash=strrchr(result,'/');
      if (lastslash) {
	*lastslash='\0'; dirlen=(lastslash-result);
	base=base+3;}
      else {strcpy(result,base); return result;}}
    else break;}
  result[dirlen]='/'; strcpy(result+dirlen+1,base);
  return result;
}

U8_EXPORT u8_string u8_abspath(u8_string path,u8_string wd)
{
  if (path[0] == '/') return u8_valid_copy(path);
  else if (path[0] == '~') {
    u8_byte uname[64], *name_end=strchr(path,'/'), *homedir;
    int namelen;
    if (name_end==NULL) {
      namelen=strlen(path); name_end=path+namelen;}
    else if (name_end-path > 63)
      return u8_string_append(HOMEDIR_ROOT,path+1,NULL);
    else namelen=(name_end-(path+1));
    if (namelen) {
      strncpy(uname,path+1,namelen); uname[namelen]='\0';
      homedir=get_homedir(uname);}
    else homedir=get_homedir(NULL);
    if (homedir) {
      u8_string result=u8_abspath(name_end+1,homedir);
      u8_free(homedir);
      return result;}
    else return u8_abspath(name_end+1,NULL);}
  else {
    u8_string result, absroot=wd; int need_free=0;
    if (wd == NULL) {absroot=u8_getcwd(); need_free=1;}
    if (ends_in_slashp(absroot)) {
      result=u8_mkpath(absroot,path);
      if (need_free) u8_free(absroot);
      return result;}
    else if (u8_directoryp(absroot)) {
      result=u8_mkpath(absroot,path);
      if (need_free) u8_free(absroot);
      return result;}
    else {
      u8_string dir=u8_dirname(absroot);
      result=u8_mkpath(dir,path);
      if (need_free) u8_free(absroot);
      u8_free(dir);
      return result;}}
}

U8_EXPORT u8_string u8_dirname(u8_string path)
{
  u8_string copy=u8_valid_copy(path);
  if (copy) {
    u8_byte *dirend=strrchr(copy,'/');
    if ((dirend)&&(dirend[1]=='\0')) {
      /* If the slash is at the end of the string,
	 use the next one. */
      *dirend='\0'; dirend=strrchr(copy,'/');}
    if (dirend) {dirend[1]='\0'; return copy;}
    else return copy;}
  else return NULL;
}

U8_EXPORT u8_string u8_basename(u8_string path,u8_string suffix)
{
  u8_byte *dirend=strrchr(path,'/'); u8_string copy, suff;
  if (dirend) copy=u8_strdup(dirend+1); else copy=u8_strdup(path);
  if (suffix == NULL) return copy;
  if (strcmp(suffix,"*")==0) suff=strchr(copy,'.');
  else {
    /* Get the last occurence of 'suffix' */
    u8_byte *scan=strstr(copy,suffix); int sufflen=strlen(suffix);
    suff=scan; while (scan) {
      suff=scan; scan=strstr(suff+sufflen,suffix);}}
  /* This is good for a default case, where people leave the '.' off
     of the suffix arg. */
  if ((suff>copy) && (suff[-1]=='.')) suff--;
  if (suff) *suff='\0';
  return copy;
}

/* Getting realpaths */

U8_EXPORT
#if HAVE_REALPATH
u8_string u8_realpath(u8_string path,u8_string wd)
{
  u8_string abspath=u8_abspath(path,wd); 
  if (abspath) {
    char *result=realpath(abspath,NULL);
    if (result) {
      u8_string u8ified=u8_fromlibc(result);
      u8_free(abspath);
      if (((u8_string)result)==u8ified) return result;
      u8_free(result);
      return u8ified;}
    else return abspath;}
  else return NULL;
}
#else
u8_string u8_realpath(u8_string path,u8_string wd)
{
  return u8_abspath(path,wd);
}
#endif

/* Init function */

U8_EXPORT void u8_init_pathfns_c()
{
  u8_register_source_file(_FILEINFO);
}


