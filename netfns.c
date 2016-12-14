/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2016 beingmeta, inc.
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

#include "libu8/u8netfns.h"
#include "libu8/libu8io.h"
#include "libu8/u8filefns.h"
#include "libu8/u8srvfns.h"
#include "libu8/u8timefns.h"
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#ifndef MAX_HOSTNAME
#define MAX_HOSTNAME 1024
#endif

#if WIN32
#include <winsock.h>
#endif

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if U8_THREADS_ENABLED
static u8_mutex netfns_lock;
#endif

static u8_condition InternalError=_("Internal error");
static u8_condition SocketTimeout=_("Socket timeout");
static u8_condition NoConnection=_("Cannot connect to server");
static u8_condition NoFileSockets MAYBE_UNUSED
                      =_("No UNIX domain (file) sockets");
static u8_condition UnknownHost MAYBE_UNUSED=_("Unknown host");

int u8_reconnect_wait1=1;
int u8_reconnect_wait=5;
int u8_reconnect_tries=5;

int u8_warn_waitlevel=4;

int u8_cpdebug=0;
static u8_string ConnPools="ConnPools";

#define CPDBG0(cp,msg) {            \
  if (u8_cpdebug)                  \
   u8_log(u8_cpdebug,ConnPools,msg,\
          cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open);}
#define CPDBG1(cp,msg,arg1) {            \
  if (u8_cpdebug)                  \
   u8_log(u8_cpdebug,ConnPools,msg,\
          cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,arg1);}
#define CPDBG2(cp,msg,arg1,arg2) {            \
  if (u8_cpdebug)                  \
   u8_log(u8_cpdebug,ConnPools,msg,\
          cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,\
          arg1,arg2);}

#if HAVE_GETPID
static int getu8pid()
{
  return (int) getpid();
}
#else
static int getu8pid()
{
  return -1;
}
#endif

u8_condition u8_NetworkError=_("network error");
u8_condition u8_BadPortSpec=_("Bad port specification");

#define saddr(x) ((struct sockaddr *)(&x))

#include <errno.h>

/** sockaddrs **/

static u8_string addr_string(int port,unsigned char *addr,int len)
{
  if (len==4)
    return u8_mkstring
      ("%d@%d.%d.%d.%d",
       port,
       (int)addr[0],(int)addr[1],(int)addr[2],(int)addr[3]);
  else if (len == 8)
    return u8_mkstring
      ("%d@%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
       port,
       (int)addr[0],(int)addr[1],(int)addr[2],(int)addr[3],
       (int)addr[4],(int)addr[5],(int)addr[6],(int)addr[7]);
  else return u8_mkstring("%d@<<Weird address length>>",port);
}

U8_EXPORT u8_string u8_sockaddr_string(struct sockaddr *s)
{
  if (s->sa_family==AF_INET) {
    struct sockaddr_in *inaddr=(struct sockaddr_in *)s;
    return addr_string(ntohs(inaddr->sin_port),
                       (unsigned char *)&(inaddr->sin_addr),4);}
#if HAVE_SYS_UN_H
  else if (s->sa_family==AF_UNIX) {
    struct sockaddr_un *unaddr=(struct sockaddr_un *)s;
    return u8_mkstring("%s@unix",unaddr->sun_path);}
#endif
#ifdef AF_INET6
  else if (s->sa_family==AF_INET6) {
    struct sockaddr_in6 *inaddr=(struct sockaddr_in6 *)s;
    return addr_string(ntohs(inaddr->sin6_port),
                       (unsigned char *)&(inaddr->sin6_addr),8);}
#endif
  else return u8_strdup("strange sockaddr");
}

/** TT (Touch-tone) encoding **/

static char tt_codes[128]=
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
 -1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,
 7,7,7,7,8,8,8,9,9,9,9,-1,-1,-1,-1,-1,
 -1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,
 7,7,7,7,8,8,8,9,9,9,9,-1,-1,-1,-1,-1};


U8_EXPORT int u8_get_portno(u8_string string)
{
  struct servent *service;
  service=getservbyname(string,"tcp");
  if (service) return ntohs(service->s_port);
  /* I've been told that /etc/services sometimes fails to contain http */
  else if (strcmp(string,"http") == 0) return 80;
  else {
    int sum=0;
    while (*string) {
      int code=tt_codes[(int)*string];
      if (code == -1) return -1;
      sum=sum*10+code; string++;}
    return sum;}
}

/* Utility hostent functions */

static struct hostent *copy_hostent(struct hostent *he)
{
  struct hostent *copy; char *buf, *alloc;
  int n_bytes=sizeof(struct hostent);
  int addr_len=he->h_length, n_aliases=0, n_addrs=0;
  char **scanner, **writer;
  n_bytes=n_bytes+strlen(he->h_name)+1;
  /* Count the space for the aliases, counting each string. */
  scanner=he->h_aliases; while (*scanner) {
    n_bytes=n_bytes+strlen(*scanner)+1;
    scanner++; n_aliases++;}
  n_bytes=n_bytes+sizeof(char *)*(n_aliases+1);
  scanner=he->h_addr_list; while (*scanner) {
    n_bytes=n_bytes+addr_len;
    scanner++; n_addrs++;}
  n_bytes=n_bytes+sizeof(char *)*(n_addrs+1);
  alloc=buf=u8_malloc(n_bytes);
  copy=(struct hostent *)buf;
  copy->h_length=he->h_length; copy->h_addrtype=he->h_addrtype;
  alloc=buf+sizeof(struct hostent);
  /* Copy aliases */
  writer=copy->h_aliases=(char **) alloc;
  alloc=alloc+sizeof(char *)*(n_aliases+1);
  scanner=he->h_aliases; while (*scanner) {
    int len=strlen(*scanner);
    strcpy(alloc,*scanner);
    *writer++=alloc;
    alloc=alloc+len+1;
    scanner++;}
  *writer++=NULL;
  /* Copy addrs */
  writer=copy->h_addr_list=(char **) alloc;
  alloc=alloc+sizeof(char *)*(n_addrs+1);
  scanner=he->h_addr_list; while (*scanner) {
    memcpy(alloc,*scanner,addr_len);
    *writer++=alloc;
    alloc=alloc+addr_len;
    scanner++;}
  *writer++=NULL;
  /* Copy main name */
  {
    int namelen=strlen(he->h_name);
    strcpy(alloc,he->h_name);
    copy->h_name=alloc;
    alloc=alloc+namelen+1;}
  if (alloc != buf+n_bytes) {
    u8_raise(InternalError,"copy_hostent",NULL);
    return NULL;}
  else return copy;
}

U8_EXPORT struct hostent *u8_gethostbyname(u8_string hname,int type)
{
  char *name=u8_tolibc(hname);
  struct hostent *fetched, *copied;
  char MAYBE_UNUSED _buf[1024], *buf=_buf;
  int MAYBE_UNUSED bufsiz=0, herrno=0, retval;
#if HAVE_GETHOSTBYNAME2_R
  struct hostent _fetched, *result; fetched=&_fetched;
  if (type>0)
    retval=
      gethostbyname2_r(name,type,fetched,buf,1024,&result,&herrno);
  else retval=gethostbyname_r(name,fetched,buf,1024,&result,&herrno);
  while (retval==ERANGE) {
    if (bufsiz) {
      u8_free(buf);
      bufsiz=bufsiz*2;}
    else bufsiz=2048;
    buf=u8_malloc(bufsiz);
    if (type>0)
      retval=
        gethostbyname2_r(name,type,fetched,buf,1024,&result,&herrno);
    else retval=gethostbyname_r(name,fetched,buf,1024,&result,&herrno);}
  if (result==NULL) {
    if (bufsiz) u8_free(buf);
    u8_graberr(herrno,"u8_gethostbyname",u8_strdup(hname));
    return NULL;}
  copied=((fetched==NULL) ? (NULL) :(copy_hostent(fetched)));
  if (bufsiz) u8_free(buf);
#else
  u8_lock_mutex(&netfns_lock);
  fetched=gethostbyname(name);
  if (fetched==NULL) {
    u8_seterr(UnknownHost,"u8_gethostbyname",name);
    u8_unlock_mutex(&netfns_lock);
    return NULL;}
  copied=copy_hostent(fetched);
  u8_unlock_mutex(&netfns_lock);
#endif
  u8_free(name);
  return copied;
}

U8_EXPORT struct hostent *u8_gethostbyaddr(char *addr,int len,int type)
{
  struct hostent _fetched, *fetched=&_fetched, *copied;
  u8_lock_mutex(&netfns_lock);
  fetched=gethostbyaddr(addr,len,type);
  copied=((fetched==NULL) ? (NULL) :(copy_hostent(fetched)));
  u8_unlock_mutex(&netfns_lock);
  return copied;
}

static char **copy_addrs(struct hostent *he,int *len,int *n)
{
  int addr_len=he->h_length, n_addrs=0, buflen;
  char **scan=he->h_addr_list, **write, *buf, *alloc;
  while (*scan) {n_addrs++; scan++;}
  buflen=sizeof(char *)*(n_addrs+1)+addr_len*n_addrs;
  buf=u8_malloc(buflen);
  write=(char **)buf; alloc=buf+sizeof(char *)*(n_addrs+1);
  scan=he->h_addr_list; while (*scan) {
    memcpy(alloc,*scan,addr_len);
    *write++=alloc;
    alloc=alloc+addr_len;
    scan++;}
  *write=NULL;
  *len=addr_len; if (n) *n=n_addrs;
  return (char **)buf;
}

U8_EXPORT u8_string u8_gethostname()
{
  char local_hostname[MAX_HOSTNAME+1];
  if (gethostname(local_hostname,MAX_HOSTNAME) < 0) {
    u8_graberr(-1,"u8_gethostname",NULL);
    return NULL;}
  else {
    int i=0; char *scan=local_hostname;
    /* Handle the possibility (rare) of a hostname overflow */
    while (i<MAX_HOSTNAME)
      if (*scan=='\0') return u8_strdup(local_hostname);
      else scan++;
    return u8_strndup(local_hostname,MAX_HOSTNAME);}
}

/* Host lookup */

U8_EXPORT u8_string u8_host_primary(u8_string hostname)
{
  struct hostent *h=u8_gethostbyname(hostname,-1);
  if (h==NULL) return NULL;
  else {
    u8_string pname=u8_fromlibc(h->h_name);
    u8_free(h);
    return pname;}
}

U8_EXPORT char **u8_lookup_host
   (u8_string hname,int *addr_len,unsigned int *typep)
{
  char *name=u8_tolibc(hname);
  struct hostent *fetched;
  char MAYBE_UNUSED _buf[1024], *buf=_buf, **addrs;
  int MAYBE_UNUSED bufsiz=0, herrno=0, retval, type=((typep)?(*typep):(-1));
  int retries=0;
#if HAVE_GETHOSTBYNAME2_R
  struct hostent _fetched, *result; fetched=&_fetched;
  if (type>=0)
    retval=
      gethostbyname2_r(name,type,fetched,buf,1024,&result,&herrno);
  else retval=gethostbyname_r(name,fetched,buf,1024,&result,&herrno);
  while ((retries<7)&&((retval==ERANGE)||(herrno==ERANGE)||(herrno==EINTR))) {
    if ((retval==ERANGE)||(herrno==ERANGE)) {
      if (bufsiz) {
	u8_free(buf);
	bufsiz=bufsiz*2;}
      else bufsiz=2048;
      buf=u8_malloc(bufsiz);}
    if (type>=0)
      retval=
	gethostbyname2_r(name,type,fetched,buf,bufsiz,&result,&herrno);
    else retval=gethostbyname_r(name,fetched,buf,bufsiz,&result,&herrno);
    retries++;}
  if (result==NULL) {
    if (bufsiz) u8_free(buf);
    u8_graberr(herrno,"u8_lookup_host",u8_strdup(hname));
    u8_free(name);
    return NULL;}
  addrs=copy_addrs(fetched,addr_len,NULL);
  if (typep) *typep=fetched->h_addrtype;
  if (bufsiz) u8_free(buf);
#else
  u8_lock_mutex(&netfns_lock);
  fetched=gethostbyname(name);
  if (fetched==NULL) {
    u8_seterr(UnknownHost,"u8_lookup_host",name);
    u8_unlock_mutex(&netfns_lock);
    return NULL;}
  addrs=copy_addrs(fetched,addr_len,NULL);
  u8_unlock_mutex(&netfns_lock);
#endif
  u8_free(name);
  return addrs;
}

/* Make connections */

U8_EXPORT u8_byte *u8_parse_addr
    (u8_string spec,int *portp,u8_byte *result,ssize_t buflen)
{
  u8_byte *split=strchr(spec,'@'); int len=strlen(spec);
  if ((result==NULL)||(buflen<0)) {
    buflen=len+1; result=u8_malloc(buflen); }
  if (split==spec) {
    *portp=0; strcpy(result,spec+1);
    return result;}
  else if (split) {
    int portlen=split-spec, hostlen=len-(portlen+1);
    u8_byte _portspec[32], *portspec;
    if (portlen>31) portspec=u8_malloc(1+(split-spec));
    else portspec=_portspec;
    strncpy(portspec,spec,portlen); portspec[portlen]='\0';
    *portp=u8_get_portno(portspec);
    if (portspec!=_portspec) u8_free(portspec);
    if (hostlen>=buflen) result=u8_malloc(hostlen+1);
    strncpy(result,split+1,hostlen); result[hostlen]='\0';
    return result;}
  else if ((split=strrchr(spec,':'))) {
    /* We search backwards for the colon because we want to handle
       IPv6 addresses with double colons separating octets (?) */
    if ((split[1]=='\0')||((split>spec)&&(split[-1]==':'))) {
      *portp=0; strcpy(result,spec+1);
      return result;}
    *portp=u8_get_portno(split+1);
    if (buflen<(split-spec)) {
      buflen=(split-spec)+1; result=u8_malloc(buflen);}
    strncpy(result,spec,split-spec);
    if (result[split-spec-1]==':')
      /* Accept a double colon before the port number, in case the address
         portion is an IPV6 numeric address. */
      result[split-spec-1]='\0';
    else result[split-spec]='\0';
    return result;}
  else return NULL;
}

U8_EXPORT u8_string u8_canonical_addr(u8_string spec)
{
  u8_byte _hostname[128], *hostname=_hostname; int portno;
  u8_string result;
  hostname=u8_parse_addr(spec,&portno,hostname,128);
  if (strchr(hostname,':'))
    result=u8_mkstring("%s::%d",hostname,portno);
  else result=u8_mkstring("%s:%d",hostname,portno);
  if (hostname!=_hostname) u8_free(hostname);
  return result;
}

U8_EXPORT u8_socket u8_connect_x(u8_string spec,u8_string *addrp)
{
  u8_byte _hostname[128], *hostname=_hostname; int portno=-1;
  long socket_id;
  hostname=u8_parse_addr(spec,&portno,hostname,128);
  if (portno<0) return ((u8_socket)(-1));
  else if (!(hostname)) return ((u8_socket)(-1));
  else if (portno) {
    struct sockaddr_in sockaddr;
    int addr_len, family=AF_INET;
    /* Lookup the host */
    char **addrs=u8_lookup_host(hostname,&addr_len,&family), **scan=addrs;
    if (addrs==NULL) {
      if (hostname!=_hostname) u8_free(hostname);
      return -1;}
    /* Get a socket */
    if ((socket_id=socket(family,SOCK_STREAM,0))<0) {
      u8_graberr(-1,"u8_connect:socket",u8_strdup(spec));
      u8_free(addrs);
      if (hostname!=_hostname) u8_free(hostname);
      return ((u8_socket)(-1));}
#if 0
    if (timeout>0) {
      struct timeval tv;
      tv.tv_sec=timeout/1000; tv.tv_usec=(timeout%1000)*1000;
      setsockopt(socket_id,level,SO_SNDTIMEO,(void *)&tv,sizeof(struct timeval));
      tv.tv_sec=timeout/1000; tv.tv_usec=(timeout%1000)*1000;
      setsockopt(socket_id,SO_RCVTIME0,(void *)&tv,sizeof(struct timeval));}
    setsockopt(socket_id,SO_NOSIGPIPE,(void *)1,sizeof(int));
#endif
    while (*scan) {
      char *addr=*scan++;
      sockaddr.sin_port=htons((short)portno);
      memcpy(&(sockaddr.sin_addr),addr,addr_len);
      sockaddr.sin_family=family;
      if (connect(socket_id,saddr(sockaddr),sizeof(struct sockaddr_in))<0)
        if (*scan==NULL) {
          close(socket_id);
          u8_free(addrs);
          if (hostname!=_hostname) u8_free(hostname);
          u8_graberr(-1,"u8_connect:connect",u8_strdup(spec));
          return ((u8_socket)(-1));}
        else errno=0; /* Try the next address */
      else {
        if (addrp) *addrp=u8_sockaddr_string((struct sockaddr *)&sockaddr);
        if (hostname!=_hostname) u8_free(hostname);
        u8_free(addrs);
        return (u8_socket)socket_id;}}
    u8_free(addrs);
    if (hostname!=_hostname) u8_free(hostname);
    return ((u8_socket)(-1));}
  else {
#if HAVE_SYS_UN_H
    struct sockaddr_un sockaddr;
    if ((socket_id=socket(PF_LOCAL,SOCK_STREAM,0))<0) {
      u8_graberr(-1,"u8_connect:socket",u8_strdup(spec));
      if (hostname!=_hostname) u8_free(hostname);
      return -1;}
    sockaddr.sun_family=AF_UNIX;
    strcpy(sockaddr.sun_path,hostname);
    if (connect(socket_id,saddr(sockaddr),sizeof(struct sockaddr_un))<0) {
      close(socket_id);
      u8_graberr(-1,"u8_connect:connect",u8_strdup(spec));
      if (hostname!=_hostname) u8_free(hostname);
      return ((u8_socket)(-1));}
    else return (u8_socket)socket_id;
#else
    u8_seterr(NoFileSockets,"u8_connect",NULL);
    return ((u8_socket)(-1));
#endif
  }
}

U8_EXPORT u8_socket u8_connect(u8_string spec)
{
  return u8_connect_x(spec,NULL);
}

U8_EXPORT int u8_set_nodelay(u8_socket c,int flag)
{
  int retval=setsockopt
    (c,IPPROTO_TCP,TCP_NODELAY,(void *) &flag,sizeof(int));
  if (retval<0)
#ifdef ENOTSUP
    /* If we don't support nodelay, assume its okay (e.g. the socket doesn't do
       the Nagle algorithm, for instance, it's a file socket). */
    if (errno==ENOTSUP) {
      errno=0; return 0;}
    else
#endif
#ifdef EOPNOTSUPP
    /* If we don't support nodelay, assume its okay (e.g. the socket doesn't do
       the Nagle algorithm, for instance, it's a file socket). */
    if (errno==EOPNOTSUPP) {
      errno=0; return 0;}
    else
#endif
      {
        u8_graberr(errno,"u8_set_nodelay",NULL);
        return retval;}
  else return retval;
}


/* Connection pools
   These maintain a pool of connections to a particular address,
   adding new ones if neccessary.  A smarter version of this might
   wait in some cases. */

U8_EXPORT u8_connpool
  u8_init_connpool(u8_connpool cp_arg,u8_string id,int reserve,int cap,int init)
{
  u8_connpool cp;
  if (cp_arg==NULL) cp=u8_alloc(struct U8_CONNPOOL);
  else cp=cp_arg;
  if ((cap<0) && (reserve>0)) cap=reserve+3;
  cp->u8cp_id=u8_strdup(id);
  cp->u8cp_bits=0;
  cp->u8cp_reserve=reserve;
  cp->u8cp_cap=cap;
  cp->u8cp_n_open=0;
  cp->u8cp_n_inuse=0;
  cp->u8cp_n_waiting=0;
  cp->u8cp_inuse=u8_alloc_n(cap,u8_socket);
  cp->u8cp_free=u8_alloc_n(cap,u8_socket);
  cp->u8cp_reconnect_wait1=-1;
  cp->u8cp_reconnect_wait=-1;
  cp->u8cp_reconnect_tries=-1;
  cp->u8cp_next=NULL;
  u8_init_mutex(&(cp->u8cp_lock));
  u8_init_condvar(&(cp->u8cp_drained));
  if (init>0) {
    int i=0;
    if (init>cap) init=cap;
    while (i<init) {
      int conn=u8_connect(id);
      if (conn<0) {
        u8_socket *opened=cp->u8cp_free;
        int j=0, lim=cp->u8cp_n_open;
        u8_graberr(errno,"u8_init_connblock",cp->u8cp_id);
        while (j<lim) {
          close(opened[j]); j++;}
        u8_free(cp->u8cp_free);
        u8_free(cp->u8cp_inuse);
        if (cp_arg==NULL) u8_free(cp);
        return NULL;}
      else {
        cp->u8cp_free[i++]=conn;
        cp->u8cp_n_open++;}}}
  return cp;
}

/* used to reconfigure a connection pool, increasing the cap or
   init.  */
static int grow_connpool(u8_connpool cp,int newcap,int newinit)
{
  /* If thew newcap is greater than the current cap, you need to
     grow the connection vectors. */
  u8_lock_mutex(&(cp->u8cp_lock));
  CPDBG2(cp,"(%s/%d/%d) growing connection pool to %d/%d",newinit,newcap);
  if (newcap>cp->u8cp_cap) {
    u8_socket *newfree, *newinuse;
    newfree=u8_realloc_n(cp->u8cp_free,newcap,u8_socket);
    if (newfree) cp->u8cp_free=newfree;
    newinuse=u8_realloc_n(cp->u8cp_inuse,newcap,u8_socket);
    if (newinuse) cp->u8cp_inuse=newinuse;
    if ((newfree==NULL) || (newinuse==NULL)) {
      u8_graberr(errno,"grow_connpool",cp->u8cp_id);
      u8_unlock_mutex(&(cp->u8cp_lock));
      return -1;}
    cp->u8cp_cap=newcap;}
  /* Truncate newinit to the cap */
  if (newinit>cp->u8cp_cap) newinit=cp->u8cp_cap;
  /* If the newinit is greater than the number of open connections,
     you need to open some new connections. */
  if (newinit>cp->u8cp_n_open) {
    int have=cp->u8cp_n_open, need=newinit-have;
    while (have<need) {
      u8_socket c=u8_connect(cp->u8cp_id);
      if (c<0) {
        u8_graberr(errno,"grow_connpool",u8_strdup(cp->u8cp_id));
        u8_unlock_mutex(&(cp->u8cp_lock));
        return -1;}
      cp->u8cp_free[have++]=c; cp->u8cp_n_open++;}}
  u8_unlock_mutex(&(cp->u8cp_lock));
  return cp->u8cp_cap;
}

U8_EXPORT u8_socket u8_get_connection(u8_connpool cp)
{
  u8_socket retval=-1;
  u8_lock_mutex(&(cp->u8cp_lock));
  CPDBG0(cp,"(%s/%d/%d) Getting connection");
  if (cp->u8cp_reserve<1) {
    /* If we're not really maintaining a pool, just connect. */
    u8_socket c;
    if ((c=u8_connect(cp->u8cp_id))<0) {
      u8_unlock_mutex(&(cp->u8cp_lock));
      return c;}
    else {
      /* But just track n_open */
      cp->u8cp_n_open++;
      u8_unlock_mutex(&(cp->u8cp_lock));
      return c;}}
  else {
    int n_open=cp->u8cp_n_open, n_inuse=cp->u8cp_n_inuse;
    int n_free=n_open-n_inuse;    if (n_free>0) {
      /* Use an existing connection */
      retval=cp->u8cp_free[n_free-1];
      cp->u8cp_inuse[cp->u8cp_n_inuse]=retval;
      cp->u8cp_n_inuse++;
      CPDBG2(cp,"(%s/%d/%d) Using existing connection %d at %d",
             retval,n_free-1);}
    else if (((cp->u8cp_cap)>0) && ((cp->u8cp_n_open)>=(cp->u8cp_cap))) {
      /* If there's a cap and we're at it, start waiting. */
      cp->u8cp_n_waiting++;
      if (cp->u8cp_n_waiting>u8_warn_waitlevel)
        u8_log(LOG_WARNING,ConnPools,
               "(%s/%d/%d) %d requests currently waiting",
               cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,
               cp->u8cp_n_waiting);
      CPDBG0(cp,"(%s/%d/%d) Waiting for free connection");
      while (1) {
        u8_condvar_wait(&(cp->u8cp_drained),&(cp->u8cp_lock));
        if ((cp->u8cp_n_inuse)<(cp->u8cp_n_open)) {
          CPDBG0(cp,"(%s/%d/%d) Stopped waiting for free connection");
          n_free=cp->u8cp_n_open-cp->u8cp_n_inuse;
          retval=cp->u8cp_free[n_free-1];
          cp->u8cp_inuse[cp->u8cp_n_inuse]=retval;
          cp->u8cp_n_inuse++;
          CPDBG2(cp,"(%s/%d/%d) Using existing connection %d at %d",
                 retval,n_free-1);
          cp->u8cp_n_waiting--;
          break;}}}
    else {
      /* Otherwise, make a new connection. */
      CPDBG0(cp,"(%s/%d/%d) Opening new connection");
      retval=u8_connect(cp->u8cp_id);
      if (retval<0) {
        CPDBG0(cp,"(%s/%d/%d) Couldn't open new connection");
        u8_unlock_mutex(&(cp->u8cp_lock));
        return retval;}
      cp->u8cp_inuse[cp->u8cp_n_inuse]=retval;
      cp->u8cp_n_inuse++; cp->u8cp_n_open++;
      u8_log(LOG_NOTICE,ConnPools,"(%s/%d/%d) Added new connection %d",
             cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,retval);
      CPDBG2(cp,"(%s/%d/%d) Stored new connection %d at %d",
             retval,cp->u8cp_n_inuse-1);}
    u8_unlock_mutex(&(cp->u8cp_lock));
    return retval;}
}

/* Returns a connection to the pool, possibly discarding it. */
static u8_socket return_connection(u8_connpool cp,u8_socket c,int discard)
{
  u8_lock_mutex(&(cp->u8cp_lock));
  CPDBG1(cp,"(%s/%d/%d) Freeing (returning) connection %d",c);
  if (cp->u8cp_reserve<1) {
    /* non-pooling mode */
    close(c); cp->u8cp_n_open--;
    u8_unlock_mutex(&(cp->u8cp_lock));
    return 0;}
  else {
    int retval=-1;
    int n_open=cp->u8cp_n_open, n_inuse=cp->u8cp_n_inuse;
    int n_free=n_open-n_inuse;
    u8_socket *scan=cp->u8cp_inuse, *limit=scan+n_inuse;
    /* Find the connection */
    while (scan<limit) if (*scan==c) break; else scan++;
    if (scan>=limit) {
      /* Didn't find connection */
      u8_unlock_mutex(&(cp->u8cp_lock));
      if (discard)
        u8_seterr(_("Connection not in block"),"u8_discard_connection",
                  u8_mkstring("%d",c));
      else u8_seterr(_("Connection not in block"),"u8_return_connection",
                     u8_mkstring("%d",c));
      return -1;}
    CPDBG2(cp,"(%s/%d/%d) Found connection %d at %d",c,scan-cp->u8cp_inuse);
    /* Move the inuse records down */
    memmove(scan,scan+1,sizeof(u8_socket)*(limit-(scan+1)));
    /* Bump the inuse pointer */
    cp->u8cp_n_inuse--;
    CPDBG1(cp,"(%s/%d/%d) Removed %d from inuse",c);
    if (discard)
      u8_log(LOG_NOTICE,ConnPools,"(%s/%d/%d) Discarding %d",
             cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,c);
    /* Now either discard the connection or add it to the free vector. */
    if (discard)
      /* If we're discarding this connection but there are still waiting
         requests, we generate a new connection to put back into the pool. */
      if ((cp->u8cp_n_waiting)>0) {
        CPDBG0(cp,"(%s/%d/%d) Opening replacement connection");
        c=u8_connect(cp->u8cp_id);
        if (c<0) {
          cp->u8cp_n_open--; close(c);
          /* Couldn't open the connection, bump n_open down */
          CPDBG0(cp,"(%s/%d/%d) Couldn't open replacement connection");
          u8_log(LOG_WARNING,ConnPools,
                 "(%s/%d/%d) Couldn't open replacement connection",
                 cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open);
          u8_unlock_mutex(&(cp->u8cp_lock));
          return c;}
        /* We don't bump n_open because we're doing a replacement.
           But do add the new connection to the free vector.  */
        CPDBG2(cp,"(%s/%d/%d) Stored replacement connection %d at %d",
               c,n_free);
        retval=cp->u8cp_free[n_free]=c;}
      else {
        close(c);
        cp->u8cp_n_open--;
        CPDBG1(cp,"(%s/%d/%d) Closed discarded connection %d",c);
        u8_unlock_mutex(&(cp->u8cp_lock));
        return 0;}
#if 0
    /* We've commented this out because it is too eager.
       It turns out that the 'load' when you return a connection
       isn't a good guide for whether or not to keep it.
       This might eventually be address by some kind of
       external process. */
    else if ((cp->u8cp_n_waiting==0) &&
             (((cp->u8cp_n_open)-(cp->u8cp_n_inuse))>(cp->u8cp_reserve))) {
      /* Close the connection if no one is waiting and if the
         number of free connections is greater than the reserve. */
      close(c); cp->u8cp_n_open--; retval=0;
      CPDBG1(cp,"(%s/%d/%d) Closed discarded connection %d",c);}
#endif
    /* if not discarding, add the returned connection to the free vector. */
    else {
      CPDBG2(cp,"(%s/%d/%d) Stored free connection %d at %d",
             c,n_free);
      retval=cp->u8cp_free[n_free]=c;}
    u8_condvar_signal(&(cp->u8cp_drained));
    u8_unlock_mutex(&(cp->u8cp_lock));
    return retval;}
}

#if 0
/* Was used for reconnection on failed connections */
/* Now implemented in terms of return/get connection */
static u8_socket replace_connection(u8_connpool cp,u8_socket oldc,u8_socket newc)
{
  int retval=-1;
  u8_lock_mutex(&(cp->u8cp_lock));
  {
    int n_open=cp->u8cp_n_open, n_inuse=cp->u8cp_n_inuse, n_free=n_open-n_inuse;
    u8_socket *scan=cp->u8cp_inuse, *limit=scan+n_inuse, newc=-1;
    /* Find the connection */
    while (scan<limit) if (*scan==oldc) break; else scan++;
    if (scan>=limit) {
      u8_seterr(_("Connection not in block"),"u8_close_connection",
                u8_mkstring("%d",oldc));
      retval=-1;}
    *scan=retval=newc;
    close(oldc);
  }
  u8_unlock_mutex(&(cp->u8cp_lock));
  return retval;
}
#endif

U8_EXPORT u8_socket u8_reconnect(u8_connpool cp,u8_socket c)
{
  int retry_count=0;
  int retry_wait=(((cp->u8cp_reconnect_wait)>=0) ? (cp->u8cp_reconnect_wait) : (u8_reconnect_wait));
  int retry_wait1=(((cp->u8cp_reconnect_wait1)>=0) ? (cp->u8cp_reconnect_wait1) : (u8_reconnect_wait1));
  int max_retries=(((cp->u8cp_reconnect_tries)>=0) ? (cp->u8cp_reconnect_tries) : (u8_reconnect_tries));
  u8_log(LOG_NOTICE,ConnPools,"(%s/%d/%d) Reconnecting replaces %d",
         cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,c);
  /* Put the connection back */
  return_connection(cp,c,1);
  /* Wait before reconnecting */
  c=-1; sleep(retry_wait1);
  /* Keep trying */
  c=-1; while ((c<0) && (retry_count<max_retries)) {
    c=u8_get_connection(cp); retry_count++;
    if (c<0) sleep(retry_wait);}
  if (c<0) {
    u8_log(LOG_NOTICE,ConnPools,_("(%s/%d/%d) Failed to reconnect after %d attempts"),
           cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,retry_count);
    u8_seterr(NoConnection,"u8_reconnect",u8_strdup(cp->u8cp_id));}
  else u8_log(LOG_NOTICE,ConnPools,_("(%s/%d/%d) Reconnected at %d after %d attempts"),
              cp->u8cp_id,cp->u8cp_n_inuse,cp->u8cp_n_open,c,retry_count);
  return c;
}

U8_EXPORT u8_socket u8_return_connection(u8_connpool cp,u8_socket c)
{
  return return_connection(cp,c,0);
}

U8_EXPORT u8_socket u8_discard_connection(u8_connpool cp,u8_socket c)
{
  return return_connection(cp,c,1);
}

/* Conn pools */

static u8_mutex connpools_lock;
struct U8_CONNPOOL *connpools=NULL;

U8_EXPORT u8_connpool u8_open_connpool
  (u8_string id,int reserve,int cap,int init)
{
  struct U8_CONNPOOL *conn=connpools;
  u8_lock_mutex(&connpools_lock);
  conn=connpools;
  while (conn)
    if (strcmp(id,conn->u8cp_id)==0) break;
    else conn=conn->u8cp_next;
  if (conn==NULL) {
    conn=u8_init_connpool(NULL,id,reserve,cap,init);
    if (conn==NULL) return conn;
    conn->u8cp_next=connpools;
    conn->u8cp_bits=conn->u8cp_bits|U8_CONNPOOL_REGISTERED;
    connpools=conn;}
  if ((reserve>0) && (reserve>(conn->u8cp_reserve)))
    conn->u8cp_reserve=reserve;
  if ((cap>0) && (cap>(conn->u8cp_cap)))
    if (grow_connpool(conn,cap,init)<0) {
      u8_unlock_mutex(&connpools_lock);
      return NULL;}
  u8_unlock_mutex(&connpools_lock);
  return conn;
}

U8_EXPORT u8_connpool u8_close_connpool(u8_connpool cp,int dowarn)
{
  u8_lock_mutex(&(cp->u8cp_lock));
  if ((dowarn) && (cp->u8cp_n_inuse))
    u8_log(LOG_WARN,"BADCLOSE",
           "Closing the connection block for %s while some connections are still active",
           cp->u8cp_id);
  {
    int n_open=cp->u8cp_n_open, n_inuse=cp->u8cp_n_inuse;
    int i, n_free=n_open-n_inuse;
    i=0; while (i<n_free) {close(cp->u8cp_free[i]); i++;}
    i=0; while (i<n_inuse) {close(cp->u8cp_inuse[i]); i++;}
    u8_free(cp->u8cp_inuse); u8_free(cp->u8cp_free);
    cp->u8cp_n_inuse=cp->u8cp_n_open=0;
    u8_free(cp->u8cp_id);
  }
  u8_lock_mutex(&(cp->u8cp_lock));
  u8_destroy_mutex(&(cp->u8cp_lock));
  if ((cp->u8cp_bits)&(U8_CONNPOOL_REGISTERED)) {
    u8_lock_mutex(&connpools_lock);
    if (connpools==cp) {
      connpools=cp->u8cp_next;
      u8_free(cp);
      return NULL;}
    else {
      struct U8_CONNPOOL *last=connpools, *scan=connpools->u8cp_next;
      while (scan)
        if (scan==cp) break;
        else {last=scan; scan=last->u8cp_next;}
      if (scan) {
        last->u8cp_next=scan->u8cp_next;
        u8_free(scan);
        return NULL;}
      else if (dowarn) {
        u8_log(LOG_WARN,"BADCLOSE",
               "Internal inconsitency: can't find registered connpool",
               cp->u8cp_id);
        return cp;}
      else return NULL;}}
  else return cp;
}

/* Getting the session id */

static u8_string sessionid=NULL;

U8_EXPORT
u8_string u8_identify_session(u8_string newid)
{
  if (sessionid)
    u8_log(LOG_NOTICE,NULL,"Changing session id to \"%s\" from \"%s\"",
           newid,sessionid);
  else u8_log(LOG_NOTICE,NULL,"Session id is \"%s\"",newid);
  if (sessionid) u8_free(sessionid);
  sessionid=newid;
  return newid;
}

U8_EXPORT u8_string u8_sessionid()
{
  if (sessionid) return sessionid;
  else {
    u8_string hostname=u8_gethostname();
    u8_string appid=u8_appid();
    U8_XTIME now; U8_OUTPUT out;
    u8_lock_mutex(&netfns_lock);
    if (sessionid) {
      /* For race condition */
      u8_unlock_mutex(&netfns_lock);
      return sessionid;}
    U8_INIT_STATIC_OUTPUT(out,64);
    u8_local_xtime(&now,-1,u8_second,0);
    u8_printf(&out,"%s:p%dt%XGit@%s",
              ((appid)?(appid):((u8_string)"??")),
              getu8pid(),&now,hostname);
    u8_free(hostname);
    sessionid=out.u8_outbuf;
    u8_unlock_mutex(&netfns_lock);
    return sessionid;}
}

/** Dealing with timeouts **/

#if WIN32
static int set_noblock(int socket)
{
  unsigned long flags=1;
  ioctlsocket(socket,FIONBIO,&flags);
  return flags;
}
/* !!!! Might not do the right thing. */
static int set_doblock(int socket)
{
  unsigned long flags=0;
  ioctlsocket(socket,FIONBIO,&flags);
  return flags;
}
static MAYBE_UNUSED int reset_flags(int fd,int flags)
{
  unsigned long flag_value=flags;
  return ioctlsocket(fd,FIONBIO,&flag_value);
}
#else
static int set_noblock(int fd)
{
  int oflags=fcntl(fd,F_GETFL), flags=oflags|O_NONBLOCK;
  fcntl(fd,F_SETFL,flags);
  return oflags;
}
static int set_doblock(int fd)
{
  int oflags=fcntl(fd,F_GETFL), flags=oflags|~O_NONBLOCK;
  fcntl(fd,F_SETFL,flags);
  return oflags;
}
static MAYBE_UNUSED int reset_flags(int fd,int flags)
{
  return fcntl(fd,F_SETFL,flags);
}
#endif

#if WIN32
#define set_timeout_error()
#define test_timeout_errorp() (0)
#else
#define set_timeout_error() errno=ETIMEDOUT
#define test_timeout_errorp() (errno == ETIMEDOUT)
#endif

#if WIN32
static int inprogressp()
{
  int err=WSAGetLastError();
  /* fprintf(stderr,"inprogressp err=%d\n",err-WSABASEERR); */
  if ((err == WSAEINPROGRESS) || (err == WSAEWOULDBLOCK)) return 1;
  else return 0;
}
#else
#define inprogressp() (errno == EINPROGRESS)
#endif

static int wait_on_socket(int fd,int msecs,int rd,int wr,int exc)
{
  fd_set rs, ws, xs; struct timeval timeout;
  FD_ZERO(&rs); FD_ZERO(&ws); FD_ZERO(&xs);
  if (rd) FD_SET(fd,&rs); if (wr) FD_SET(fd,&ws); if (exc) FD_SET(fd,&xs);
  timeout.tv_sec=msecs/1000; timeout.tv_usec=(msecs%1000)*1000;
  return select(fd+1,&rs,&ws,&xs,&timeout);
}

U8_EXPORT
/* u8_getbytes:
      Arguments: an interval in milliseconds (an int), an open socket,
                 a pointer to a block of data, a number of bytes,
                 and some flags (an int) for recv()
      Returns: the number of bytes read or -1 on error
  Tries to read bytes from a connection, returning -1 if the
   connection times out.
*/
int u8_getbytes(int msecs,int socket_id,char *data,int len,int flags)
{
  int wait_result=-1;
  wait_result=wait_on_socket(socket_id,msecs,1,0,0);
  if (wait_result)
    return recv(socket_id,data,len,flags);
  else {
    return -1;}
}

U8_EXPORT
/* u8_sendbytes:
      Arguments: a socket, a pointer to a block of data,
                 the length of the block of data, and
                 flags to pass to send()
      Returns: either zero or -1 (indictating an error)
  This sends all of the bytes in a block of data, repeatedly
calling send().  This will return -1, indicating a failure, if
the attempt to write times out.
*/
int u8_sendbytes(int msecs,int socket,const char *buf,int size,int flags)
{
  /* This makes sure that all the data is sent. */
  /* result=send(socket,d.start,d.ptr-d.start,0); */
  fd_set writefds;
  const unsigned char *todo;
  int result, residue;
  todo=buf; residue=size;
  while (residue > 0) {
    int retval;
    FD_ZERO(&writefds);
    FD_SET(socket,&writefds);
    retval=wait_on_socket(socket,msecs,0,1,0);
    switch(retval) {
    case 1:
      result=send(socket,todo,residue,flags);
      if (result >= 0) {
        todo=todo+result;
        residue=residue-result;}
      else if (errno != EAGAIN)
        return result;
      else errno=0;
      break;
    case 0: {
      u8_seterr(SocketTimeout,"u8_sendbytes",NULL);
      return -1;
    default:
      if ((errno != EINTR) && (errno != EAGAIN)) {
        u8_log(LOG_WARN,NULL,"Error (%s) on socket %d, retval=%d",
               strerror(errno),socket,retval);
        u8_graberr(-1,"u8_sendbytes",NULL);
        return -1;}
      continue;}}}
  return 0;
}

U8_EXPORT int u8_transact(int timeout,int socket,char *msg,char *expect)
{
  char buf[1024]; int recv_length, total_bytes=0, retval=0;
  retval=send(socket,msg,strlen(msg),0); buf[0]='\0';
  if (retval<0) {
    u8_graberr(errno,"u8_transact",msg);
    return retval;}
  while (((recv_length=
           u8_getbytes(timeout,socket,buf+total_bytes,1024-total_bytes,0)) > 0) &&
         (total_bytes<1024))
    total_bytes=total_bytes+recv_length;
  buf[total_bytes]='\0';
  if ((total_bytes<=0) && (recv_length<0)) {
    u8_seterr(SocketTimeout,"u8_transact",NULL);
    return recv_length;}
  else if (expect)
    if (strncmp(buf,expect,strlen(expect)) != 0)
      return 0;
    else return 1;
  else return 1;
}

/* Sending SMTP mail */

char *u8_default_mailhost, *u8_default_maildomain, *u8_default_from;
int u8_smtp_timeout=250;

static int isasciip(u8_string data,int len)
{
  const u8_byte *scan=data, *limit=data+len;
  while (scan<limit)
    if ((*scan)>=0x80) return 0;
    else scan++;
  return 1;
}

static void output_mime(u8_output out,u8_string data,int len,int pos)
{
  const u8_byte *scan=data, *limit=scan+len; int escaped=0;
  if (!(isasciip(data,len))) {
    u8_puts(out,"=?utf-8?Q?");
    escaped=1; pos=pos+10;}
  while (scan<limit) {
    int c=*scan++;
    if (pos>=75) {
      if (escaped) u8_puts(out,"?=");
      u8_putc(out,'\r'); u8_putc(out,'\n');
      u8_puts(out," =?utf-8?Q?"); pos=11;}
    if ((c>=0x80) || (iscntrl(c)) || (isspace(c)))
      u8_printf(out,"=%02x",c);
    else u8_putc(out,c);}
  if (escaped) u8_puts(out,"?=");
}

U8_EXPORT
/* u8_smtp
    Arguments: a destination (a string), a contents (a string), and
               a set of fields (a lisp object)
    Returns: nothing

 Uses a local SMTP connection to send mail to a particular individual with
 a particular set of fields and a particular contents. */
int u8_smtp(const char *mailhost,const char *maildomain,
            const char *from,const char *dest,const char *ctype,
            int n_headers,u8_mailheader *headers,
            const unsigned char *message,int message_len)
{
  struct U8_OUTPUT out;
  int i=0, socket; char _buf[1024]; char buf[1024];
  if (mailhost==NULL) mailhost=u8_default_mailhost;
  if (maildomain==NULL) maildomain=u8_default_maildomain;
  if (mailhost) socket=u8_connect(mailhost);
  else if (getenv("U8MAILHOST"))
    socket=u8_connect(getenv("U8MAILHOST"));
  else {
    u8_seterr("No SMTP MAILHOST","u8_smtp",NULL);
    return -1;}
  if (from==NULL) from="kilroy";
  if (socket<0) {
    u8_graberr(errno,"u8_smtp",NULL);
    return socket;}
  else u8_set_nodelay(socket,1);
  if (u8_getbytes(u8_smtp_timeout,socket,buf,1024,0)<0) return -1;
  if (maildomain) {
    sprintf(buf,"HELO %s\r\n",maildomain);
    if (u8_transact(u8_smtp_timeout,socket,buf,"250")<=0) return -1;}
  sprintf(buf,"MAIL FROM: <%s>\r\n",from);
  if (u8_transact(u8_smtp_timeout,socket,buf,"250")<=0) return -1;
  sprintf(buf,"RCPT TO:<%s>\r\n",dest);
  if (u8_transact(u8_smtp_timeout,socket,buf,"250")<=0) return -1;
  if (u8_transact(u8_smtp_timeout,socket,"DATA\r\n","354")<=0) return -1;
  U8_INIT_FIXED_OUTPUT(&out,1024,_buf);
  u8_printf(&out,"To: %s\r\nFrom: %s\r\n",dest,from);
  i=0; while (i<n_headers) {
    struct U8_MAILHEADER *hdr=headers[i++];
    u8_printf(&out,"%s: ",hdr->label);
    output_mime(&out,hdr->value,strlen(hdr->value),strlen(hdr->label)+2);}
  if (message_len<0) message_len=strlen(message);
  if (ctype==NULL)
    sprintf(buf,"Content-type: %s; charset=utf-8;\r\n\r\n",ctype);
  else if (strncmp(ctype,"text",4)==0)
    sprintf(buf,"Content-type: %s; charset=utf-8;\r\n\r\n",ctype);
  else sprintf(buf,"Content-type: %s\r\n\r\n",ctype);
  if (ctype)
    if (u8_sendbytes(u8_smtp_timeout,socket,buf,strlen(buf),0)<0)
      return -1;
  /* Send the content */
  if (u8_sendbytes(u8_smtp_timeout,socket,message,message_len,0)<0)
    return -1;
  u8_transact(u8_smtp_timeout,socket,"\r\n.\r\n","250");
  u8_transact(u8_smtp_timeout,socket,"QUIT\r\n","221");
  close(socket);
  return 1;
}

/* Initialization code */

U8_EXPORT void u8_init_netfns_c()
{
#if U8_THREADS_ENABLED
  u8_init_mutex(&connpools_lock);
  u8_init_mutex(&netfns_lock);
#endif
  u8_register_source_file(_FILEINFO);
}
