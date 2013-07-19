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

/* #define _GNU_SOURCE */

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

#include "libu8/libu8io.h"
#include "libu8/u8filefns.h"
#include "libu8/u8netfns.h"
#include "libu8/u8srvfns.h"
#include "libu8/u8timefns.h"
#include <unistd.h>
#include <limits.h>
#if HAVE_POLL_H
#include <poll.h>
#elif HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#ifdef POLLRDHUP
#define HUPFLAGS POLLHUP|POLLRDHUP
#else
#define HUPFLAGS POLLHUP
#endif

static u8_condition ClosedClient=_("Closed connection");
static u8_condition ServerShutdown=_("Shutting down server");
static u8_condition NewServer=_("New server port");
static u8_condition NewClient=_("New connection");
static u8_condition RejectedConnection=_("Rejected connection");
static u8_condition ClientRequest=_("Client request");
static u8_condition Inconsistency=_("Internal inconsistency");

/* Prototypes for some static definitions */

static int close_client_core(u8_client cl,int server_locked,u8_context caller);
static int finish_closing_client(u8_client cl);

static void update_client_stats(u8_client cl,long long cur,int done);
static void update_server_stats(u8_client cl);

static int push_task(struct U8_SERVER *server,u8_client cl,u8_context cxt);

static int add_client(struct U8_SERVER *server,u8_client client);
static int free_client(struct U8_SERVER *server,u8_client cl,u8_context caller);

/* Helpful functions */

static char *get_client_state(u8_client cl,char *buf)
{
  char *write=buf;
  *write++='t';
  if (cl->started>0) *write++='s';
  if (cl->active>0) *write++='a';
  if (cl->queued>0) *write++='q';
  if (cl->reading>0) *write++='r';
  if (cl->writing>0) *write++='w';
  if (cl->running>0) *write++='x';
  *write++='\0';
  return buf;
}

/* Sockaddr functions */

static struct sockaddr *get_sockaddr_in
  (int family,int port,char *addr,int addr_len)
{
  if (family==AF_INET) {
    struct sockaddr_in *sockaddr=u8_alloc(struct sockaddr_in);
    memset(sockaddr,0,sizeof(struct sockaddr_in)); 
    sockaddr->sin_port=htons((short)port);
    sockaddr->sin_family=family;
    memcpy((char *)&(sockaddr->sin_addr),addr,addr_len);
    return (struct sockaddr *)sockaddr;}
#ifdef AF_INET6
  else if (family==AF_INET6) {
    struct sockaddr_in6 *sockaddr=u8_alloc(struct sockaddr_in6);
    memset(sockaddr,0,sizeof(struct sockaddr_in6)); 
    sockaddr->sin6_port=htons((short)port);
    sockaddr->sin6_family=family;
    memcpy((char *)&(sockaddr->sin6_addr),addr,addr_len);
    return (struct sockaddr *)sockaddr;}
#endif
  else return NULL;
}

static struct sockaddr *get_sockaddr_file(u8_string string)
{
#if HAVE_SYS_UN_H
  struct sockaddr_un *sockaddr=u8_alloc(struct sockaddr_un);
  sockaddr->sun_family=AF_LOCAL;
  strcpy(sockaddr->sun_path,string);
  return (struct sockaddr *)sockaddr;
#else
  return NULL;
#endif
}

static int sockaddr_samep(struct sockaddr *s1,struct sockaddr *s2)
{
  if (s1->sa_family != s2->sa_family) return 0;
  else if (s1->sa_family==AF_INET) {
    struct sockaddr_in *a1=(struct sockaddr_in *)s1;
    struct sockaddr_in *a2=(struct sockaddr_in *)s2;
    if (a1->sin_port!=a2->sin_port) return 0;
    else return ((memcmp(&(a1->sin_addr),&(a2->sin_addr),4))==0);}
#if HAVE_SYS_UN_H
  else if (s1->sa_family==AF_UNIX)
    return ((strcmp(((struct sockaddr_un *)s1)->sun_path,
		    ((struct sockaddr_un *)s2)->sun_path))==0);
#endif
#ifdef AF_INET6
  else if (s1->sa_family==AF_INET6) {
    struct sockaddr_in6 *a1=(struct sockaddr_in6 *)s1;
    struct sockaddr_in6 *a2=(struct sockaddr_in6 *)s2;
    if (a1->sin6_port!=a2->sin6_port) return 0;
    else return ((memcmp(&(a1->sin6_addr),&(a2->sin6_addr),8))==0);}
#endif
  else return 0;
}

/* Servers */

/* This provides a generic server loop which is customized by
   functions for accepting, servicing, and closing remote clients.
   The application conses client objects whose first few fields are
   reserved for used by the server. */

static void init_server_socket(u8_socket socket_id);

U8_EXPORT
/* u8_init_client:
    Arguments: a pointer to a client
    Returns: void
 Initializes the client structure.
 @arg client a pointer to a client stucture, or NULL if one is to be consed
 @arg len the length of the structure, as allocated or to be consed
 @arg sock a socket (or -1) for the client
 @arg server the server of which the client will be a part
 Returns: a pointer to a client structure, consed if not provided.
*/
u8_client u8_init_client(u8_client client,size_t len,
			 struct sockaddr *addrbuf,size_t addrlen,
			 u8_socket sock,u8_server srv)
{
  if (!(client)) client=u8_malloc(len);
  memset(client,0,len);
  client->socket=sock;
  client->server=srv;
  client->started=client->queued=client->active=0;
  client->reading=client->writing=-1;
  client->off=client->len=client->buflen=client->delta=0;
  if ((srv->flags)&U8_SERVER_ASYNC)
    client->flags=client->flags|U8_CLIENT_ASYNC;
  return client;
}

/* Reading and writing from clients */

U8_EXPORT int u8_client_finished(u8_client cl)
{
  if ((cl->reading>0)||(cl->writing>0))
    if (cl->off<cl->len) return 0;
    else if (cl->reading>0) {
      long long rtime=u8_microtime()-cl->reading;
      cl->stats.rsum+=rtime; cl->stats.rsum2+=(rtime*rtime);
      if (rtime>cl->stats.rmax) cl->stats.rmax=rtime;
      cl->stats.rcount++;
      cl->reading=0;
      return 1;}
    else if (cl->writing>0) {
      long long wtime=u8_microtime()-cl->writing;
      cl->stats.wsum+=wtime; cl->stats.wsum2+=(wtime*wtime);
      if (wtime>cl->stats.wmax) cl->stats.wmax=wtime;
      cl->stats.wcount++;
      cl->writing=0;
      return 1;}
    else return 1;
  else return 1;
}

U8_EXPORT unsigned char *u8_client_read
  (u8_client cl,unsigned char *buf,size_t n,size_t off)
{
  char statebuf[16];
  if ((cl->reading>0)&&
      (cl->buf==buf)&&(cl->len=n)&&(cl->off>=cl->len)) {
    cl->reading=0; return cl->buf;}
  else if (cl->reading>0) {
    u8_log(LOG_WARNING,"u8_client_read",
	   "Client @x%lx#%d.%d[%s/%d](%s) is already reading %ld bytes",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring,
	   ((long)cl->len));
    return NULL;}
  else if (cl->writing>0) {
    u8_log(LOG_WARNING,"u8_client_read",
	   "Client @x%lx#%d.%d[%s/%d](%s) is still writing %ld bytes",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring,
	   ((long)cl->len));
    return NULL;}
  else if (off<n) {
    cl->reading=u8_microtime();
    cl->buflen=cl->len=n; cl->buf=buf; cl->off=off;
    return NULL;}
  else return buf;
}

U8_EXPORT unsigned char *u8_client_write
  (u8_client cl,unsigned char *buf,size_t n,size_t off)
{
  char statebuf[16];
  if ((cl->writing>0)&&(cl->buf==buf)&&(cl->len=n)) {
    if (cl->off<cl->len) return NULL;
    else {cl->writing=0; return cl->buf;}}
  else if (cl->writing>0) {
    u8_log(LOG_WARNING,"u8_client_write",
	   "Client @x%lx#%d.%d[%s/%d](%s) is already writing %ld bytes",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring,
	   ((long)cl->len));
    return NULL;}
  else if (cl->reading>0) {
    u8_log(LOG_WARNING,"u8_client_write",
	   "Write to @x%lx#%d.%d[%s/%d](%s) which is still reading %d bytes",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring,
	   ((long)cl->len));
    cl->reading=0;
    return NULL;}
  else if (off<n) {
    cl->writing=u8_microtime();
    cl->buflen=cl->len=n; cl->buf=buf; cl->off=off;
    return NULL;}
  else return buf;
}

/* Declaring a client done with a transaction (and available for another) */

U8_EXPORT
/* u8_client_done:
    Arguments: a pointer to a client
    Returns: void
 Marks the client's current task as done, updating server data structures
 appropriately. */
int u8_client_done(u8_client cl)
{
  U8_SERVER *server=cl->server; char statebuf[16];
  int clientid=cl->clientid;
  if (cl->started>0) {
    if (server->donefn) {
      int retval=server->donefn(cl);
      if (retval<0) {
	u8_log(LOG_ERR,"u8_client_done",
	       "Error when finishing transaction on @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,cl->n_trans,
	       cl->idstring);
	u8_clear_errors(1);}}
    update_client_stats(cl,u8_microtime(),1);
    cl->active=cl->writing=cl->reading=0;
    if (cl->queued>0) {
      u8_log(LOG_WARNING,"u8_client_done",
	     "Finishing transaction on a queued client @x%lx#%d.%d[%s/%d](%s)",
	     ((unsigned long)cl),cl->clientid,cl->socket,cl->n_trans,
	     cl->idstring);
      cl->queued=0;}
    if (cl->socket>0) {
      struct pollfd *pfd=&(server->sockets[clientid]);
      cl->reading=u8_microtime();
      pfd->events=((short)(POLLIN|HUPFLAGS));}
    u8_lock_mutex(&(server->lock));
    if (cl->started>0) {
      server->n_busy--;
      cl->started=0;}
     server->n_trans++;
    u8_unlock_mutex(&(server->lock));
    return 1;}
  else {
    u8_log(LOG_WARNING,"u8_client_done",
	   "Declaring done on idle client @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
    return 0;}
}

/* Closing clients */

U8_EXPORT
/* u8_close_client:
    Arguments: a pointer to a client
    Returns: int
 Marks the client's current task as done, updating server data structures
 appropriately, and ending by closing the client close function.
 If a busy client is closed, it has its U8_CLIENT_CLOSING flag set.
 This tells the event loop to finish closing the client when it actually
 completes the current transaction.
*/
int u8_close_client(u8_client cl)
{
  U8_SERVER *server=cl->server; char statebuf[16];
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    int retval;
    u8_lock_mutex(&(server->lock));
    /* Catch race conditions */
    if (cl->flags&U8_CLIENT_CLOSED) {
      u8_unlock_mutex(&(server->lock));
      return 0;}
    cl->flags|=U8_CLIENT_CLOSING;
    u8_unlock_mutex(&(server->lock));
    /* If the task is in the middle of a transaction,
       don't close it right away. */
    if (cl->started>0) {
      if (server->flags&U8_SERVER_LOG_CONNECT)
	u8_log(LOG_NOTICE,"u8_close_client",
	       "Deferring closing of  @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
      return 0;}
    retval=close_client_core(cl,0,"u8_close_client");
    return retval;}
  else {
    u8_log(LOG_WARNING,"u8_close_client",
	   "Closing already closed socket @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
    u8_unlock_mutex(&(server->lock));
    return 0;}
}

U8_EXPORT
/* u8_client_closed:
    Arguments: a pointer to a client
    Returns: int
  Indicates that the other end has closed the connection.  This can generate
    an indication of its own and then closes the client on this end
*/
int u8_client_closed(u8_client cl)
{
  char statebuf[16];
  if ((cl->server->flags)&(U8_SERVER_LOG_CONNECT))
    u8_log(LOG_NOTICE,"u8_client_closed",
	   "Other end closed @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
  return u8_client_close(cl);
}

U8_EXPORT
/* u8_shutdown_client:
    Arguments: a pointer to a client
    Returns: int
 Marks the client's current task as done, updating server data structures
 appropriately, and ending by closing the client close function.
 This will shutdown an active client.
*/
int u8_shutdown_client(u8_client cl)
{
  U8_SERVER *server=cl->server; char statebuf[16];
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    int retval;
    u8_lock_mutex(&(server->lock));
    /* Catch race conditions */
    if (cl->flags&U8_CLIENT_CLOSED) {
      u8_unlock_mutex(&(server->lock));
      return 0;}
    cl->flags|=U8_CLIENT_CLOSING;
    u8_unlock_mutex(&(server->lock));
    retval=close_client_core(cl,0,"u8_shutdown_client");
    return retval;}
  else {
    u8_log(LOG_WARNING,"u8_shutdown_client",
	   "Closing already closed socket @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
    u8_unlock_mutex(&(server->lock));
    return 0;}
}

/* This is the internal version used when shutting down a server.
   We dont wait for anything. */
static int client_close_for_shutdown(u8_client cl)
{
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    if (cl->started>0) {
      /* It's in the middle of something, so we just flag it as closing. */
      cl->flags|=U8_CLIENT_CLOSING;
      return 0;}
    else return close_client_core(cl,1,"client_close_for_shutdown");}
  else return 0;
}

/* This is used when a transaction finishes and the socket has been
   closed during shutdown. */
static int close_client_core(u8_client cl,int server_locked,u8_context caller)
{
  if (!(caller)) caller="close_client_core";
  if (cl->flags&U8_CLIENT_CLOSED) return 0;
  else {
    U8_SERVER *server=cl->server; int retval=0;
    /* We grab idstring, because our code allocated it but we will use it
       only after the closefn has freed the client object. */
    long long cur=u8_microtime();
    char statebuf[16];
    
    if (!(server_locked)) u8_lock_mutex(&(server->lock));

    if (server->flags&U8_SERVER_LOG_CONNECT)
      u8_log(LOG_INFO,"u8_close_client",
	     "Closing (%s)  @x%lx#%d.%d[%s/%d](%s)",
	     caller,((unsigned long)cl),cl->clientid,cl->socket,
	     get_client_state(cl,statebuf),
	     cl->n_trans,cl->idstring);
    
    /* Update run stats for one last time */
    if (cl->started>0) {
      u8_log(LOG_WARNING,"close_client_core",
	     "Closing (%s) running client @x%lx#%d.%d[%s/%d](%s)",
	     caller,((unsigned long)cl),cl->clientid,cl->socket,
	     get_client_state(cl,statebuf),
	     cl->n_trans,cl->idstring);
      server->n_busy--;}
    update_client_stats(cl,cur,1);

    cl->active=cl->running=cl->reading=cl->writing=cl->started=0;

    if (server->closefn)
      retval=server->closefn(cl);
    else if (cl->socket>0) 
      retval=close(cl->socket);
    else retval=0;
    
    cl->flags|=U8_CLIENT_CLOSED;
    cl->socket=-1;

    if (cl->server->flags&U8_SERVER_LOG_CONNECT)
      u8_log(LOG_NOTICE,ClosedClient,"Closed (%s) @x%lx#%d.%d[%s/%d](%s)",
	     caller,((unsigned long)cl),cl->clientid,cl->socket,
	     get_client_state(cl,statebuf),
	     cl->n_trans,cl->idstring);

    if (cl->queued>0) {
      long long interval=cur-cl->queued;
      if ((cl->reading>0)||(cl->writing>0))
	u8_log(LOG_WARNING,"close_client_core",
	       "Closing (%s) a queued client @x%lx#%d.%d[%s/%d](%s)",
	       caller,((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
      cl->stats.qsum+=interval;
      cl->stats.qsum2+=(interval*interval);
      if (interval>cl->stats.qmax) cl->stats.qmax=interval;
      cl->stats.qcount++;}
    else push_task(server,cl,caller);
    
    if (!(server_locked)) u8_unlock_mutex(&(server->lock));

    return retval;}
}

static int finish_closing_client(u8_client cl)
{
  U8_SERVER *server=cl->server;
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    int retval;
    u8_lock_mutex(&(server->lock));
    /* Catch race conditions */
    if (cl->flags&U8_CLIENT_CLOSED) {
      u8_unlock_mutex(&(server->lock));
      return 0;}
    u8_unlock_mutex(&(server->lock));
    retval=close_client_core(cl,0,"finish_closing_client");
    return retval;}
  else return 0;
}

static void update_client_stats(u8_client cl,long long cur,int done)
{
  long long interval=cur-cl->active;
  /* read/write/execute */
  if (cl->running) {
    interval=cur-cl->running;
    cl->stats.xsum+=interval;
    cl->stats.xsum2+=(interval*interval);
    if (interval>cl->stats.xmax) cl->stats.xmax=interval;
    cl->stats.xcount++;}
  if (done) {
    if (cl->started>0){
      interval=cur-cl->started;
      cl->stats.tsum+=interval;
      cl->stats.tsum2+=(interval*interval);
      if (interval>cl->stats.tmax) cl->stats.tmax=interval;
      cl->stats.tcount++;}}
}

static void update_server_stats(u8_client cl)
{
  u8_server server=cl->server;
  /* Transfer all the client statistics to the server */

  server->aggrestats.tsum+=cl->stats.tsum;
  server->aggrestats.tsum2+=cl->stats.tsum2;
  server->aggrestats.tcount+=cl->stats.tcount;
  if (cl->stats.tmax>server->aggrestats.tmax)
    server->aggrestats.tmax=cl->stats.tmax;

  server->aggrestats.qsum+=cl->stats.qsum;
  server->aggrestats.qsum2+=cl->stats.qsum2;
  server->aggrestats.qcount+=cl->stats.qcount;
  if (cl->stats.qmax>server->aggrestats.qmax)
    server->aggrestats.qmax=cl->stats.qmax;

  server->aggrestats.rsum+=cl->stats.rsum;
  server->aggrestats.rsum2+=cl->stats.rsum2;
  server->aggrestats.rcount+=cl->stats.rcount;
  if (cl->stats.rmax>server->aggrestats.rmax)
    server->aggrestats.rmax=cl->stats.rmax;

  server->aggrestats.wsum+=cl->stats.wsum;
  server->aggrestats.wsum2+=cl->stats.wsum2;
  server->aggrestats.wcount+=cl->stats.wcount;
  if (cl->stats.wmax>server->aggrestats.wmax)
    server->aggrestats.wmax=cl->stats.wmax;

  server->aggrestats.xsum+=cl->stats.xsum;
  server->aggrestats.xsum2+=cl->stats.xsum2;
  server->aggrestats.xcount+=cl->stats.xcount;
  if (cl->stats.xmax>server->aggrestats.xmax)
    server->aggrestats.xmax=cl->stats.xmax;

  server->n_errs+=cl->n_errs;
}

/* Maintaining the task/event/client queue */

#if U8_THREADS_ENABLED
static u8_client pop_task(struct U8_SERVER *server)
{
  u8_client task=NULL; char statebuf[16];
  u8_lock_mutex(&(server->lock));
  while ((server->n_queued == 0) && ((server->flags&U8_SERVER_CLOSED)==0)) 
    u8_condvar_wait(&(server->empty),&(server->lock));
  if (server->flags&U8_SERVER_CLOSED) {}
  else if (server->n_queued) {
    task=server->queue[server->queue_head];
    server->queue[server->queue_head++]=NULL;
    if (server->queue_head>=server->queue_len) server->queue_head=0;
    server->n_queued--;}
  else {}
  if (!(task)) {}
  else if (task->active>0) {
    /* This should probably never happen */
    u8_log(LOG_CRIT,"pop_task(u8)","popping active task @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)task),task->clientid,task->socket,
	   get_client_state(task,statebuf),
	   task->n_trans,task->idstring);
    task->queued=-1; task=NULL;}
  else if ((task->queued<=0)||(task->socket<0)) {
    if (((server->flags)&(U8_SERVER_LOG_QUEUE))||
	((task->flags)&(U8_CLIENT_LOG_QUEUE)))
      u8_log(LOG_NOTICE,"pop_task(u8)","Final pop of closed task @x%lx#%d.%d[%s/%d](%s)",
	     ((unsigned long)task),task->clientid,task->socket,
	     get_client_state(task,statebuf),
	     task->n_trans,task->idstring);
    free_client(task->server,task,"pop_task/closed");
    task->queued=-1;
    task=NULL;}
  else {
    u8_utime curtime=u8_microtime();
    long long qtime=curtime-task->queued;
    task->stats.qsum+=qtime; task->stats.qsum2+=(qtime*qtime);
    task->stats.qcount++;
    if (qtime>task->stats.qmax) task->stats.qmax=qtime;
    task->queued=0; task->active=curtime;
    if (((server->flags)&(U8_SERVER_LOG_QUEUE))||
	((task->flags)&(U8_CLIENT_LOG_QUEUE)))
      u8_log(LOG_NOTICE,"pop_task(u8)","Popped task @x%lx#%d.%d[%s/%d](%s)",
	     ((unsigned long)task),task->clientid,task->socket,
	     get_client_state(task,statebuf),
	     task->n_trans,task->idstring);
    if (task->started<=0) {
      task->started=curtime;
      server->n_busy++;}}
  u8_unlock_mutex(&(server->lock));
  return task;
}

static int push_task(struct U8_SERVER *server,u8_client cl,u8_context cxt)
{
  u8_utime cur=u8_microtime(); char statebuf[16];
  if (server->n_queued >= server->max_queued) return 0;
  if (cl->queued>0) return 0;
  if (cl->clientid<0) return 0;
  if ((cl->flags)&(U8_CLIENT_CLOSED)) return 0;
  if (((server->flags)&(U8_SERVER_LOG_QUEUE))||
      ((cl->flags)&(U8_CLIENT_LOG_QUEUE)))
    u8_log(LOG_NOTICE,cxt,"Queueing client @x%lx#%d.%d[%s/%d](%s)",
	   ((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
  cl->queued=cur;
  server->queue[server->queue_tail++]=cl;
  if (server->queue_tail>=server->queue_len) server->queue_tail=0;
  server->n_queued++;
  if (cl->active>0) cl->active=0;
  server->sockets[cl->clientid].events=((short)0);
  u8_condvar_signal(&(server->empty));
  return 1;
}

/* The main event loop */

static void *event_loop(void *thread_arg)
{
  char statebuf[16];
  struct U8_SERVER *server=(struct U8_SERVER *)thread_arg;
  /* Check for additional thread init functions */
  while (1) {
    u8_client cl; int dobreak=0; int result=0, closed=0;
    u8_utime cur;
    /* Check that this thread's init functions are up to date */
    u8_threadcheck();
    cl=pop_task(server);
    if (!(cl)) continue;
    else if ((cl->socket<=0)||((cl->flags)&U8_CLIENT_CLOSED)) {
      cl->active=0; continue;}
    else cur=u8_microtime();
    if (((server->flags)&(U8_SERVER_LOG_TRANSACT))||
	((cl->flags)&(U8_CLIENT_LOG_TRANSACT)))
      u8_log(LOG_NOTICE,ClientRequest,
	     "Handling activity on @x%lx#%d.%d[%s/%d](%s)",
	     ((unsigned long)cl),cl->clientid,cl->socket,
	     get_client_state(cl,statebuf),
	     cl->n_trans,cl->idstring);
    if ((cl->reading>0)||(cl->writing>0)) {
      /* We're in the middle of reading or writing a chunk of data,
	 so we try to read/write another chunk. */
      ssize_t delta;
      if (cl->off<cl->len) { /* We're not done */
	if (cl->writing>0)
	  delta=write(cl->socket,cl->buf+cl->off,cl->len-cl->off);
	else delta=read(cl->socket,cl->buf+cl->off,cl->len-cl->off);
	if (((server->flags)&(U8_SERVER_LOG_TRANSFER))||
	    ((cl->flags)&(U8_CLIENT_LOG_TRANSFER)))
	  u8_log(LOG_NOTICE,((cl->writing>0)?("Writing"):("Reading")),
		 "Processed %d bytes for @x%lx#%d.%d[%s/%d](%s)",delta,
		 ((unsigned long)cl),cl->clientid,cl->socket,
		 get_client_state(cl,statebuf),
		 cl->n_trans,cl->idstring);
	if (delta>0) cl->off=cl->off+delta;}
      /* If we've still got data to read/write, we update the poll
	 structure to keep listening and continue in the event loop.  */
      if (cl->off<cl->len) {
	/* u8_lock_mutex(&server->lock); */
	if (cl->writing>0) 
	  server->sockets[cl->clientid].events=((short)(POLLOUT|HUPFLAGS));
	else server->sockets[cl->clientid].events=((short)(POLLIN|HUPFLAGS));
	/* u8_unlock_mutex(&server->lock);*/
	cl->active=0; continue;}
      else {
	/* Otherwise, we're done with whatever reading or writing we
	   were doing, so we record stats. */
	u8_utime now=u8_microtime();
	if (cl->writing>0) {
	  long long wtime=now-cl->writing;
	  cl->stats.wsum+=wtime;
	  cl->stats.wsum2+=(wtime*wtime);
	  if (wtime>cl->stats.wmax) cl->stats.wmax=wtime;
	  cl->stats.wcount++;}
	else {
	  long long rtime=now-cl->reading;
	  cl->stats.rsum+=rtime;
	  cl->stats.rsum2+=(rtime*rtime);
	  if (rtime>cl->stats.rmax) cl->stats.rmax=rtime;
	  cl->stats.rcount++;}
	if ((((server->flags)&(U8_SERVER_LOG_TRANSACT))||
	     ((cl->flags)&(U8_CLIENT_LOG_TRANSACT)))&&
	    (cl->len>0))
	  u8_log(LOG_NOTICE,((cl->writing>0)?
			     ("event_loop/write"):
			     (cl->reading>0)?
			     ("event_loop/read"):
			     ("event_loop/weird")),
		 "Transferred all %d bytes for @x%lx#%d.%d[%s/%d](%s)",
		 cl->len,((unsigned long)cl),cl->clientid,cl->socket,
		 get_client_state(cl,statebuf),
		 cl->n_trans,cl->idstring);}}
    /* Unless there's an I/O error, call the handler */
    if (result>=0) {
      long long xtime;
      cl->running=cur=u8_microtime();
      if (cl->callback) {
	void *state=cl->cbstate;
	u8_client_callback callback=cl->callback;
	cl->callback=NULL; cl->cbstate=NULL;
	result=callback(cl,state);}
      else result=server->servefn(cl);
      cl->running=0;
      /* Record execution stats */
      xtime=u8_microtime()-cur;
      cl->stats.xsum+=xtime;
      cl->stats.xsum2+=(xtime*xtime);
      if (xtime>cl->stats.xmax) cl->stats.xmax=xtime;
      cl->stats.xcount++;}
    if (result<0) {
      u8_exception ex=u8_current_exception;
      u8_log(LOG_ERR,"event_loop",
	     "Error result from client @x%lx#%d.%d[%s/%d](%s)",
	     ((unsigned long)cl),cl->clientid,cl->socket,
	     get_client_state(cl,statebuf),
	     cl->n_trans,cl->idstring);
      if (cl->flags&U8_CLIENT_CLOSED) closed=1;
      else if (cl->flags&U8_CLIENT_CLOSING) {
	update_client_stats(cl,u8_microtime(),1);
	cl->active=0; finish_closing_client(cl); closed=1;
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else if (cl->active>0) u8_client_done(cl);
      if (ex) {
	u8_log(LOG_WARN,ClientRequest,
	       "Error during activity on @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring,u8_errstring(ex));
	u8_clear_errors(1);
	close_client_core(cl,1,"event_loop/error");
	closed=1;}
      cl->n_errs++;}
    else if (result==0) {
      u8_utime cur=u8_microtime();
      /* Request is completed */
      if (((server->flags)&U8_SERVER_LOG_TRANSACT)||
	  ((cl->flags)&U8_CLIENT_LOG_TRANSACT))
	u8_log(LOG_NOTICE,ClientRequest,
	       "Completed transaction with @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
      cl->n_trans++;
      if (server->flags&U8_SERVER_CLOSED) dobreak=1;
      if (cl->flags&U8_CLIENT_CLOSED) {
	u8_log(LOG_CRIT,Inconsistency,
	       "Result returned from closed client @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
	if (cl->started>0) {
	  update_client_stats(cl,cur,1);
	  server->n_busy--; cl->started=0;}
	closed=1; if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else if (cl->flags&U8_CLIENT_CLOSING) {
	if (cl->started>0) {
	  update_client_stats(cl,cur,1);
	  server->n_busy--; cl->started=0;}
	cl->active=0; finish_closing_client(cl); closed=1;
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else {
	if (cl->active>0) u8_client_done(cl);
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}}
    else if (((cl->reading>0)||(cl->writing>0))&&(cl->buf!=NULL)) {
      /* The execution function queued some data to read or write.
	 We give it an initial try and push the task (in some cases,
	 the operating system will just take it all and we can finish
	 up. */
      ssize_t delta;
      if (cl->writing>0)
	delta=write(cl->socket,cl->buf+cl->off,((size_t)(cl->len-cl->off)));
      else delta=read(cl->socket,cl->buf+cl->off,((size_t)(cl->len-cl->off)));
      if (((server->flags)&(U8_SERVER_LOG_TRANSFER))||
	  ((cl->flags)&(U8_CLIENT_LOG_TRANSFER)))
	u8_log(LOG_NOTICE,((cl->writing>0)?("Writing"):("Reading")),
	       "Processed %d bytes for @x%lx#%d.%d[%s/%d](%s)",delta,
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
      if (delta>0) cl->off=cl->off+delta;
      /* If we've still got data to read/write, we continue,
	 otherwise, we fall through */
      if (cl->off<cl->len) {
	/* Keep listening */
	if (cl->writing>0) 
	  server->sockets[cl->clientid].events=((short)(POLLOUT|HUPFLAGS));
	else server->sockets[cl->clientid].events=((short)(POLLIN|HUPFLAGS));}
      else if ((cl->len>0)&&((cl->writing>0)||(cl->reading>0)))
	push_task(server,cl,((cl->writing)?
			     ("event_loop/w"):("event_loop/r")));
      else {}}
    else {
      /* Request is not yet completed, but we're not waiting
	 on input or output. */
      if (((server->flags)&U8_SERVER_LOG_TRANSACT)||
	  ((cl->flags)&U8_CLIENT_LOG_TRANSACT))
	u8_log(LOG_NOTICE,ClientRequest,
	       "Yield during request for @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);}
    if (cl->active>0) {
      /* Record the stats on how much thread time has been spent
	 (how long the task has been active) */
      cl->active=0;
      if (dobreak) break;
      if (!(closed)) {
	/* Start listening again (it was stopped by pop_task() */
	if (cl->writing>0) 
	  server->sockets[cl->clientid].events=((short)(POLLOUT|HUPFLAGS));
	else server->sockets[cl->clientid].events=((short)(POLLIN|HUPFLAGS));}}}
  u8_threadexit();
  return NULL;
}

#endif

/* Creating/initializing servers */

U8_EXPORT
struct U8_SERVER *u8_init_server
   (struct U8_SERVER *server,
    u8_client (*acceptfn)(u8_server,u8_socket,struct sockaddr *,size_t),
    int (*servefn)(u8_client),
    int (*donefn)(u8_client),
    int (*closefn)(u8_client),
    ...)
{
  int i=0;
  int flags=0, init_clients=DEFAULT_INIT_CLIENTS, n_threads=DEFAULT_NTHREADS;
  int maxback=MAX_BACKLOG, max_queue=DEFAULT_MAX_QUEUE;
  int  max_clients=DEFAULT_MAX_CLIENTS, timeout=DEFAULT_TIMEOUT;
  va_list args; int prop;
  if (server==NULL) server=u8_alloc(struct U8_SERVER);
  memset(server,0,sizeof(struct U8_SERVER));
  va_start(args,closefn);
  while ((prop=va_arg(args,int))>0) {
    switch (prop) {
    case U8_SERVER_FLAGS:
      flags=flags|(va_arg(args,int)); continue;
    case U8_SERVER_NTHREADS:
      n_threads=(va_arg(args,int)); continue;
    case U8_SERVER_INIT_CLIENTS:
      init_clients=(va_arg(args,int)); continue;
    case U8_SERVER_MAX_QUEUE:
      max_queue=(va_arg(args,int)); continue;
    case U8_SERVER_MAX_CLIENTS:
      max_clients=(va_arg(args,int)); continue;
    case U8_SERVER_BACKLOG:
      max_clients=(va_arg(args,int)); continue;
    case U8_SERVER_TIMEOUT:
      timeout=(va_arg(args,int)); continue;
    case U8_SERVER_LOGLEVEL: {
      int level=(va_arg(args,int));
      if (level>3) flags|=U8_SERVER_LOG_TRANSFER;
      if (level>2) flags|=U8_SERVER_LOG_TRANSACT;
      if (level>1) flags|=U8_SERVER_LOG_CONNECT;
      if (level>0) flags|=U8_SERVER_LOG_LISTEN;
      continue;}
    default:
      u8_log(LOG_CRIT,"u8_init_server",
	     "Unknown property code %d for server",prop);
      continue;}}
  va_end(args);
  
  if (init_clients<=0) init_clients=1;
  server->serverid=NULL; server->flags=flags; server->shutdown=0;
  server->init_clients=init_clients; server->max_clients=max_clients;
  server->server_info=NULL; server->n_servers=0;
  server->clients=u8_alloc_n(init_clients,u8_client);
  memset(server->clients,0,sizeof(u8_client)*init_clients);
  server->n_clients=0; server->clients_len=init_clients;
  server->sockets=u8_alloc_n(init_clients,struct pollfd); 
  memset(server->sockets,0,sizeof(struct pollfd)*init_clients);
  server->free_slot=server->max_slot=0;
  server->poll_timeout=timeout;
  server->max_backlog=((maxback<=0) ? (MAX_BACKLOG) : (maxback));
  server->acceptfn=acceptfn;
  server->servefn=servefn;
  server->closefn=closefn;
  server->donefn=donefn;
#if U8_THREADS_ENABLED
  u8_init_mutex(&(server->lock));
  u8_init_condvar(&(server->empty)); u8_init_condvar(&(server->full));  
  server->n_threads=n_threads;
  server->queue=u8_alloc_n(max_queue,u8_client);
  server->n_queued=0; server->max_queued=max_queue;
  server->queue_head=0; server->queue_tail=0; server->queue_len=max_queue;
  server->thread_pool=u8_alloc_n(n_threads,pthread_t);
  server->n_trans=0; /* Transaction count */
  server->n_accepted=0; /* Accept count (new clients) */
  i=0; while (i < n_threads) {
	 pthread_create(&(server->thread_pool[i]),
			pthread_attr_default,
			event_loop,(void *)server);
	 i++;}
#endif

  return server;
}

U8_EXPORT
int u8_server_init(struct U8_SERVER *server,
		   /* max_clients is currently ignored */
		   int maxback,int max_queue,int n_threads,
		   u8_client (*acceptfn)(u8_server,u8_socket,
					 struct sockaddr *,size_t),
		   int (*servefn)(u8_client),
		   int (*closefn)(u8_client))
{
  if (u8_init_server
      (server,acceptfn,servefn,NULL,closefn,
       U8_SERVER_NTHREADS,n_threads,
       U8_SERVER_INIT_CLIENTS,max_queue,
       U8_SERVER_MAX_QUEUE,max_queue,
       U8_SERVER_BACKLOG,maxback,
       -1))
    return 1;
  else return 0;
}

static int do_shutdown(struct U8_SERVER *server,int grace)
{
  int i=0, n_servers=server->n_servers;
  int n_errs=0, idle_clients=0, active_clients=0, clients_len;
  u8_utime deadline=u8_microtime()+grace;
  struct pollfd *sockets; u8_client *clients;
  if (server->flags&U8_SERVER_CLOSED) return 0;
  u8_lock_mutex(&server->lock);
  if (server->flags&U8_SERVER_CLOSED) {
    u8_unlock_mutex(&server->lock);
    return 0;}
  else {
    sockets=server->sockets;
    clients=server->clients;
    clients_len=server->clients_len;}
  server->flags=server->flags|U8_SERVER_CLOSED;
  /* Close all the server sockets */
  u8_log(LOG_WARNING,ServerShutdown,"Closing %d listening socket(s)",n_servers);
  while (i<n_servers) {
    struct U8_SERVER_INFO *info=&(server->server_info[i++]);
    u8_socket socket=info->socket, poll_index=info->poll_index, retval=0;
    sockets[poll_index].fd=-1; sockets[poll_index].events=((short)0);
    if (socket>=0) retval=close(socket);
    if (retval<0) {
      u8_log(LOG_WARNING,ServerShutdown,
	     "Error (%s) closing socket %d listening at %s",
	     strerror(errno),socket,info->idstring);
      n_errs++;}
    else if (server->flags&U8_SERVER_LOG_LISTEN)
      u8_log(LOG_NOTICE,ServerShutdown,"Closed socket %d listening at %s",
	     socket,info->idstring);
    if (info->idstring) u8_free(info->idstring);
    if (info->addr) u8_free(info->addr);
    info->idstring=NULL; info->addr=NULL;
    info->socket=-1;}
  if (n_errs)
    u8_log(LOG_WARNING,ServerShutdown,
	   "Closed %d listening socket(s) with %d errors",
	   n_servers,n_errs);
  else u8_log(LOG_NOTICE,ServerShutdown,"Closed %d listening socket(s)",
	      n_servers);
  if (server->server_info) {
    u8_free(server->server_info);
    server->server_info=NULL;}
  /* Close all the idle client sockets and mark all the busy clients closed */
  i=0; while (i<clients_len) {
    u8_client client=clients[i];    
    if (client) {
      if (client->started<=0) {
	client_close_for_shutdown(client);
	clients[i]=NULL;
	sockets[i].fd=-1;
	sockets[i].events=((short)0);
	idle_clients++;}
      else active_clients++;}
    i++;}
  u8_log(LOG_NOTICE,ServerShutdown,
	 "Closed %d idle client socket(s), %d active clients left (n_busy=%d)",
	 idle_clients,active_clients,server->n_busy);
#if U8_THREADS_ENABLED
  /* The busy clients will decrement server->n_busy when they're finished.
     We wait for this to happen, sleeping for one second intervals. */
  u8_condvar_broadcast(&server->empty);
  u8_unlock_mutex(&server->lock); sleep(1);
  if (server->n_busy) {
    /* Wait for the busy connections to finish or a timeout */
    u8_lock_mutex(&server->lock); 
    while ((server->n_busy)&&(u8_microtime()<deadline)) {
      u8_unlock_mutex(&server->lock);
      sleep(1);
      u8_lock_mutex(&server->lock);}}
  u8_unlock_mutex(&server->lock);
  if (server->n_busy) {
    u8_log(LOG_CRIT,ServerShutdown,
	   "Forcing %d active socket(s) closed after %dus",
	   server->n_busy,grace);
    i=0; while (i<clients_len) {
      u8_client client=clients[i];    
      if (client) {
	client_close_for_shutdown(client);
	clients[i]=NULL;
	sockets[i].fd=-1;
	sockets[i].events=((short)0);
	idle_clients++;}
      i++;}}
#endif
  u8_free(server->clients); server->clients=NULL;
  u8_free(server->sockets); server->sockets=NULL;
  server->clients_len=0;
#if U8_THREADS_ENABLED  
  u8_free(server->thread_pool); server->thread_pool=NULL;
  u8_free(server->queue); server->queue=NULL;
#endif
  u8_unlock_mutex(&server->lock);
  u8_destroy_mutex(&server->lock);
  u8_destroy_condvar(&(server->empty));
  u8_destroy_condvar(&(server->full));  
  return 1;
}

U8_EXPORT int u8_shutdown_server(struct U8_SERVER *server,int grace)
{
  if (grace)
    server->shutdown=grace;
  else server->shutdown=-1;
  return 0;
}
#define u8_server_shutdown u8_shutdown_server

/* Opening various kinds of servers */

static void init_server_socket(u8_socket socket_id)
{
#if WIN32
  unsigned long nonblocking=1;
#endif
  /* We set the server socket to be non-blocking */
#if (defined(F_SETFL) && defined(O_NDELAY))
  fcntl(socket_id,F_SETFL,O_NDELAY);
#endif
#if (defined(F_SETFL) && defined(O_NONBLOCK))
  fcntl(socket_id,F_SETFL,O_NONBLOCK);
#endif
#if WIN32
  ioctlsocket(socket_id,FIONBIO,&nonblocking);
#else
  u8_log(LOG_WARNING,_("sockopt"),"Can't set server socket to non-blocking");
#endif
}

static u8_socket open_server_socket(struct sockaddr *sockaddr,int maxbacklog)
{
  u8_socket socket_id=-1, on=1, addrsize, family;
  if (sockaddr->sa_family==AF_INET) {
    family=AF_INET; addrsize=sizeof(struct sockaddr_in);}
#ifdef AF_INET6
  else if (sockaddr->sa_family==AF_INET6) {
    family=AF_INET6; addrsize=sizeof(struct sockaddr_in6);}
#endif
#if HAVE_SYS_UN_H
  else if (sockaddr->sa_family==AF_UNIX) {
    family=AF_UNIX; addrsize=sizeof(struct sockaddr_un);}
#endif
  else {
    u8_seterr(_("invalid sockaddr"),"open_server_socket",NULL);
    return -1;}
  socket_id=socket(family,SOCK_STREAM,0);
  if (socket_id < 0) {
    u8_graberr(-1,"open_server_socket:socket",u8_sockaddr_string(sockaddr));
    return -1;}
  else if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR,
		      (void *) &on, sizeof(on)) < 0) {
    u8_graberr(-1,"open_server_socket:setsockopt",u8_sockaddr_string(sockaddr));
    return -1;}
  if ((bind(socket_id,(struct sockaddr *) sockaddr,addrsize)) < 0) {
    u8_graberr(-1,"open_server_socket:bind",u8_sockaddr_string(sockaddr));
    return -1;}
  if ((listen(socket_id,maxbacklog)) < 0) {
    u8_graberr(-1,"open_server_socket:listen",u8_sockaddr_string(sockaddr));
    return -1;}
  else {
    init_server_socket(socket_id);
    return socket_id;}
}

static int add_socket(struct U8_SERVER *server,u8_socket sock,short events)
{
  u8_client *clients=server->clients;
  struct pollfd *sockets=server->sockets;
  int clients_len=server->clients_len;
  if (sock<0) return sock;
  else if (server->free_slot==server->clients_len) {
    /* Grow the arrays if neccessary */
    int cur_len=server->clients_len;
    int grow_by=server->init_clients;
    int new_len=cur_len+grow_by;
    clients=u8_realloc(clients,sizeof(u8_client)*new_len);
    sockets=u8_realloc(sockets,sizeof(struct pollfd)*new_len);
    if ((clients)&&(sockets)) {
      memset(clients+clients_len,0,sizeof(u8_client)*grow_by);
      memset(sockets+clients_len,0,sizeof(struct pollfd)*grow_by);
      server->clients=clients; server->sockets=sockets;
      server->clients_len=new_len;}
    else {
      u8_log(LOG_CRIT,"add_socket","Couldn't allocate more clients/sockets");
      return -1;}}
  if (server->free_slot==server->max_slot) {
    int slot=server->free_slot;
    struct pollfd *pfd=&(server->sockets[slot]);
    memset(pfd,0,sizeof(struct pollfd));
    pfd->fd=sock; pfd->events=((short)events);
    server->free_slot++; server->max_slot++;
    return slot;}
  else {
    int slot=server->free_slot, max_slot=server->max_slot;
    struct pollfd *pfd=&(server->sockets[slot]);
    int i=slot+1; while (i<max_slot) {
      if ((!(clients[i]))&&(sockets[i].fd<0)) break;
      else i++;}
    server->free_slot=i;
    memset(pfd,0,sizeof(struct pollfd));
    pfd->fd=sock; pfd->events=((short)events);
    return slot;}
}

static struct U8_SERVER_INFO *add_server
  (struct U8_SERVER *server,u8_socket sock,struct sockaddr *addr)
{
  u8_server_info info;
  if (server->server_info==NULL) {
    info=server->server_info=u8_alloc(struct U8_SERVER_INFO);
    server->n_servers++;}
  else {
    server->server_info=
      u8_realloc_n(server->server_info,server->n_servers+1,
		   struct U8_SERVER_INFO);
    info=&(server->server_info[server->n_servers]);
    memset(info,0,sizeof(struct U8_SERVER_INFO));
    server->n_servers++;}
  info->socket=sock; info->addr=addr;
  info->poll_index=add_socket(server,sock,POLLIN);
  return info;
}

static struct U8_SERVER_INFO *find_server
  (struct U8_SERVER *server,struct sockaddr *addr)
{
  if (server->server_info==NULL) return NULL;
  else {
    struct U8_SERVER_INFO *servers=server->server_info;
    int i=0;
    while (i<server->n_servers)
      if (sockaddr_samep(servers[i].addr,addr))
	return &(servers[i]);
      else i++;
    return NULL;}
}

static int add_server_from_spec(struct U8_SERVER *server,u8_string spec)
{
  if ((strchr(spec,'/')) ||
      ((strchr(spec,'@')==NULL) &&
       (strchr(spec,':')==NULL) &&
       (strchr(spec,'.'))))
    return u8_add_server(server,spec,-1);
  else if (((strchr(spec,'@'))==NULL)&&((strchr(spec,':'))==NULL)) 
    /* Spec is just a port */
    return u8_add_server(server,NULL,u8_get_portno(spec));
  else {
    int portno=-1;
    u8_byte _hostname[128], *hostname=u8_parse_addr(spec,&portno,_hostname,128);
    if ((!(hostname))||(portno<0))
      return u8_reterr(u8_BadPortSpec,"add_server_from_spec",u8s(spec));
    return u8_add_server(server,hostname,portno);}
}

U8_EXPORT
int u8_add_server(struct U8_SERVER *server,char *hostname,int port)
{
  if (hostname==NULL)
    if (port<=0) return -1;
    else {
      int n_servers=0;
      u8_string thishost=u8_gethostname();
      if (thishost) 
	n_servers=u8_add_server(server,thishost,port);
      else n_servers=-1;
      u8_free(thishost);
      return n_servers;}
  else if (port==0)
    return add_server_from_spec(server,hostname);
  else if (port<0) {
    /* Open a file socket */
    struct sockaddr *addr=get_sockaddr_file(hostname); 
    struct U8_SERVER_INFO *info;
    if (find_server(server,addr)) {
      u8_log(LOG_NOTICE,NewServer,"Already listening at %s",hostname);
      u8_free(addr); return 0;}
    else {
      u8_socket sock=open_server_socket(addr,server->max_backlog);
      if (sock<0) {
	u8_free(addr);
	return -1;}
      else info=add_server(server,sock,addr);
      info->idstring=u8_strdup(hostname);
      if (server->flags&U8_SERVER_LOG_LISTEN)
	u8_log(LOG_INFO,NewServer,"Listening at %s",hostname);
      return 1;}}
  else {
    int n_servers=0;
    struct hostent *hostinfo=u8_gethostbyname(hostname,AF_INET);
    if (hostinfo==NULL) return -1;
    else {
      unsigned char **addrs=(unsigned char **)
	((hostinfo) ? (hostinfo->h_addr_list) : (NULL));
      if (addrs==NULL) return 0;
      while (*addrs) {
	struct sockaddr *sockaddr=
	  get_sockaddr_in(AF_INET,port,*addrs,hostinfo->h_length);
	if (find_server(server,sockaddr)) {
	  u8_free(sockaddr); addrs++;}
	else {
	  struct U8_SERVER_INFO *info;
	  u8_socket sock=open_server_socket(sockaddr,server->max_backlog);
	  if (sock>=0) {
	    info=add_server(server,sock,sockaddr);
	    if (info) info->idstring=u8_sockaddr_string(sockaddr);
	    if (server->flags&U8_SERVER_LOG_LISTEN)
	      u8_log(LOG_NOTICE,NewServer,"Listening to %s",info->idstring);}
	  else {
	    u8_log(LOG_ERROR,u8_NetworkError,"Can't open one socket");
	    if (sockaddr) u8_free(sockaddr);
	    if (hostinfo) u8_free(hostinfo);
	    return -1;}
	  addrs++; n_servers++;}}
      if (hostinfo) u8_free(hostinfo);
      return n_servers;}}
}

/* The core server loop */

static int add_client(struct U8_SERVER *server,u8_client client)
{
  u8_socket sock=client->socket;
  int slot=add_socket(server,sock,POLLIN);
  if (slot<0) return slot;
  server->clients[slot]=client; client->clientid=slot;
  client->server=server; server->n_clients++;
  return slot;
}

static int free_client(struct U8_SERVER *server,u8_client cl,u8_context caller)
{
  char statebuf[16];
  int clientid=cl->clientid;
  struct pollfd *pfd=&(server->sockets[clientid]);
  if (((server->flags)&(U8_SERVER_LOG_QUEUE))||
      ((cl->flags)&(U8_CLIENT_LOG_QUEUE)))
    u8_log(LOG_NOTICE,"free_client(u8)","Freeing (%s) client/task @x%lx#%d.%d[%s/%d](%s)",
	   caller,((unsigned long)cl),cl->clientid,cl->socket,
	   get_client_state(cl,statebuf),
	   cl->n_trans,cl->idstring);
  memset(pfd,0,sizeof(struct pollfd)); pfd->fd=-1;
  server->n_clients--;
  update_server_stats(cl);
  server->clients[clientid]=NULL;
  /* If the newly empty slot is before the current free_slot,
     make it the free_slot. */
  if (clientid<server->free_slot) server->free_slot=clientid;
  u8_free(cl->idstring);
  u8_free(cl);
  return 1;
}

static int server_accept(u8_server server,u8_socket i)
{
  /* If the activity is on the server socket, open a new socket */
  char addrbuf[1024]; char statebuf[16];
  int addrlen=1024; u8_socket sock;
  memset(addrbuf,0,1024);
  sock=accept(i,(struct sockaddr *)addrbuf,&addrlen);
  if (sock < 0) {
    u8_log(LOG_ERR,u8_NetworkError,_("Failed accept on socket %d"),i);
    errno=0;
    return -1;}
  else {
    u8_client cl=server->acceptfn
      (server,sock,(struct sockaddr *)addrbuf,addrlen);
    if (cl) {
      u8_string idstring=
	((cl->idstring)?(cl->idstring):
	 (u8_sockaddr_string((struct sockaddr *)addrbuf)));
      server->n_accepted++;
      if (!(cl->idstring)) cl->idstring=idstring;
      add_client(server,cl);
      if (server->flags&U8_SERVER_LOG_CONNECT) 
	u8_log(LOG_NOTICE,NewClient,"Opened @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)cl),cl->clientid,cl->socket,
	       get_client_state(cl,statebuf),
	       cl->n_trans,cl->idstring);
      return 1;}
    else if (server->flags&U8_SERVER_LOG_CONNECT) {
      u8_string connid=
	u8_sockaddr_string((struct sockaddr *)addrbuf);
      u8_log(LOG_NOTICE,RejectedConnection,"Rejected connection from %s");
      u8_free(connid);
      return 0;}
    else return 0;}
}

static int socket_peek(u8_socket sock)
{
  unsigned char buf[5];
  int retval=recv(sock,buf,1,MSG_PEEK);
  return (retval>0);
}

#if DESTRUCTIVE_POLL
static struct pollfd *update_socketbuf
   (struct U8_SERVER *server,struct pollfd **bufp,int *lenp)
{
  int len=*lenp; int need_len=server->max_slot; struct pollfd *buf=*bufp;
  if (len!=need_len) {
    if (buf) *bufp=buf=u8_realloc(buf,sizeof(struct pollfd)*need_len);
    else *bufp=buf=u8_malloc(sizeof(struct pollfd)*need_len);}
  memcpy(buf,server->sockets,sizeof(struct pollfd)*need_len);
  *lenp=need_len;
  return buf;
}
#else
static struct pollfd *update_socketbuf
   (struct U8_SERVER *server,struct pollfd **bufp,int *lenp)
{
  *bufp=server->sockets; *lenp=server->max_slot;
  return *bufp;
}
#endif

static int server_handle_poll(struct U8_SERVER *server,
			      struct pollfd *sockets,
			      int n_socks);

/* This listens for connections and pushes tasks (unless we're not
   threaded, in which case it dispatches to the servefn right
   away).  */
static int server_listen(struct U8_SERVER *server)
{
  struct pollfd *sockets=NULL;
  int n_socks=0, retval, timeout=server->poll_timeout;
  update_socketbuf(server,&sockets,&n_socks);
  /* Wait for activity on one of your open sockets */
  while ((retval=poll(sockets,n_socks,timeout)) == 0) {
    if (retval<0) return retval;
    if (server->shutdown) {
      do_shutdown(server,server->shutdown);
      if (sockets!=server->sockets) u8_free(sockets);
      return 0;}
    if (server->flags&U8_SERVER_CLOSED) return 0;
    update_socketbuf(server,&sockets,&n_socks);}
  if (server->shutdown) {
    do_shutdown(server,server->shutdown);
    if (sockets!=server->sockets) u8_free(sockets);
    return 0;}
  return server_handle_poll(server,sockets,n_socks);
}

static int server_handle_poll(struct U8_SERVER *server,
			      struct pollfd *sockets,
			      int n_socks)
{
  struct U8_CLIENT **clients;
  int i=0, n_actions=0;
  char statebuf[16];
  /* Iterate over the range of sockets */
  u8_lock_mutex(&(server->lock));
  clients=server->clients;
  i=0; while (i <= n_socks) {
    u8_client client; short events;
    if (sockets[i].fd<0) {i++; continue;}
    else if (sockets[i].revents==0) {
      i++; continue;}
    else {
      client=clients[i];
      events=sockets[i].revents;}
    if ((client==NULL)&&(events&POLLIN)) {
      /* Server connection */
      int retval=server_accept(server,sockets[i].fd);
      if (retval<0) u8_clear_errors(1);}
    else if (client==NULL) {
      /* Error on server socket? */}
#if U8_THREADS_ENABLED
    else if (client->active>0) {
      /* A thread is working on this client.  Don't touch it. */}
    else if (events&(HUPFLAGS)) {
      if ((client->server->flags)&(U8_SERVER_LOG_CONNECT)&&
	  (client->socket>=0))
	u8_log(LOG_NOTICE,"server_listen",
	       "Other end closed (HUP) @x%lx#%d.%d[%s/%d](%s)",
	       ((unsigned long)client),client->clientid,client->socket,
	       get_client_state(client,statebuf),
	       client->n_trans,client->idstring);
      if (client->started>0) {client->started=0; server->n_busy--;}
      close_client_core(client,1,"server_handle_poll/HUP");
      i++; continue;}
    else if (((events&POLLOUT)&&((client->writing)>0))||
	     ((events&POLLIN)&&((client->reading)>0))) {
      if (push_task(server,client,
		    (((client->writing)>0)?
		     ("server_listen/write"):
		     ("server_listen/read")))) {
	sockets[i].events=
	  ((short)(((short)(sockets[i].events))&
		   ((short)(~(POLLIN|POLLOUT)))));
	n_actions++;}}
    else if ((events&POLLNVAL)&&(client->socket<0)) {
      sockets[i].fd=-1;
      if (!(client->queued>0))
	push_task(server,client,"server_handle_pool/inval");
      i++; continue;}
    else if ((events&POLLNVAL)||
	     ((events&POLLIN)&&(!(socket_peek(client->socket))))) {
      /* No real data, so we close it (probably the other side closed)
	 the connection. */
      if (((client->server->flags)&(U8_SERVER_LOG_CONNECT))&&
	  (client->socket>=0))
	u8_log(LOG_NOTICE,"server_listen",
	       "Other end closed (%s) @x%lx#%d.%d[%s/%d](%s)",
	       ((events&POLLNVAL)?("closed/invalid"):("no data")),
	       ((unsigned long)client),client->clientid,client->socket,
	       get_client_state(client,statebuf),
	       client->n_trans,client->idstring);
      if (client->started>0) {client->started=0; server->n_busy--;}
      close_client_core(client,1,"server_handle_poll/POLLINVAL");
      i++; continue;}
    else if (events&POLLIN) {
      if (push_task(server,client,"server_listen/?")) n_actions++;}
#else
    else if (events&POLLIN) {
      server->servefn(client);
      n_actions++;}
#endif
    else {}
    i++;}
  u8_unlock_mutex(&(server->lock));
  if (sockets!=server->sockets) u8_free(sockets);
  return n_actions;
}


U8_EXPORT
int u8_push_task(struct U8_SERVER *server,u8_client cl,u8_context cxt)
{
  int retval;
  u8_lock_mutex(&(server->lock));
  retval=push_task(server,cl,cxt);
  u8_unlock_mutex(&(server->lock));
  return retval;
}

U8_EXPORT
void u8_server_loop(struct U8_SERVER *server)
{
  while ((server->flags&U8_SERVER_CLOSED)==0) server_listen(server);
}

/* Getting server status */

U8_EXPORT u8_server_stats u8_server_statistics
  (u8_server server,struct U8_SERVER_STATS *stats)
{
  int i=0, lim;
  struct U8_CLIENT **clients;
  if (stats==NULL) stats=u8_alloc(struct U8_SERVER_STATS);
  memset(stats,0,sizeof(struct U8_SERVER_STATS));
  u8_lock_mutex(&(server->lock));
  stats->n_reqs=server->n_trans;
  stats->n_errs=server->n_errs;

  stats->n_complete=server->aggrestats.tcount;

  stats->tsum=server->aggrestats.tsum; stats->tsum2=server->aggrestats.tsum2;
  stats->tcount=server->aggrestats.tcount; stats->tmax=server->aggrestats.tmax;
  stats->qsum=server->aggrestats.qsum; stats->qsum2=server->aggrestats.qsum2;
  stats->qcount=server->aggrestats.qcount; stats->qmax=server->aggrestats.qmax;
  stats->rsum=server->aggrestats.rsum; stats->rsum2=server->aggrestats.rsum2;
  stats->rcount=server->aggrestats.rcount; stats->rmax=server->aggrestats.rmax;
  stats->wsum=server->aggrestats.wsum; stats->wsum2=server->aggrestats.wsum2;
  stats->wcount=server->aggrestats.wcount; stats->wmax=server->aggrestats.wmax;
  stats->xsum=server->aggrestats.xsum; stats->xsum2=server->aggrestats.xsum2;
  stats->xcount=server->aggrestats.xcount; stats->xmax=server->aggrestats.xmax;

  stats->n_errs=server->n_errs;

  u8_unlock_mutex(&(server->lock));

  /* Now add up everything from the current clients */
  clients=server->clients; lim=server->max_slot;
  while (i<lim) {
    u8_client cl=clients[i++];
    if (cl) {
      if (cl->started>0) stats->n_busy++;
      if (cl->active>0) stats->n_active++;
      if (cl->reading>0) stats->n_reading++;
      if (cl->writing>0) stats->n_writing++;      

      stats->tsum+=cl->stats.tsum; stats->tsum2+=cl->stats.tsum2;
      stats->tcount+=cl->stats.tcount;
      if (cl->stats.tmax>stats->tmax) stats->tmax=cl->stats.tmax;

      stats->qsum+=cl->stats.qsum; stats->qsum2+=cl->stats.qsum2;
      stats->qcount+=cl->stats.qcount;
      if (cl->stats.qmax>stats->qmax) stats->qmax=cl->stats.qmax;

      stats->rsum+=cl->stats.rsum; stats->rsum2+=cl->stats.rsum2;
      stats->rcount+=cl->stats.rcount;
      if (cl->stats.rmax>stats->rmax) stats->rmax=cl->stats.rmax;

      stats->wsum+=cl->stats.wsum; stats->wsum2+=cl->stats.wsum2;
      stats->wcount+=cl->stats.wcount;
      if (cl->stats.wmax>stats->wmax) stats->wmax=cl->stats.wmax;

      stats->xsum+=cl->stats.xsum; stats->xsum2+=cl->stats.xsum2;
      stats->xcount+=cl->stats.xcount;
      if (cl->stats.xmax>stats->xmax) stats->xmax=cl->stats.xmax;

      stats->n_errs+=cl->n_errs;}}
  u8_unlock_mutex(&(server->lock));
  return stats;
}

U8_EXPORT u8_server_stats u8_server_livestats
  (u8_server server,struct U8_SERVER_STATS *stats)
{
  int i=0, lim;
  struct U8_CLIENT **clients;
  if (stats==NULL) stats=u8_alloc(struct U8_SERVER_STATS);
  memset(stats,0,sizeof(struct U8_SERVER_STATS));
  u8_lock_mutex(&(server->lock));
  stats->n_reqs=server->n_trans;
  stats->n_errs=server->n_errs;

  stats->n_complete=server->aggrestats.tcount;

  stats->tsum=server->aggrestats.tsum; stats->tsum2=server->aggrestats.tsum2;
  stats->tcount=server->aggrestats.tcount; stats->tmax=server->aggrestats.tmax;
  stats->qsum=server->aggrestats.qsum; stats->qsum2=server->aggrestats.qsum2;
  stats->qcount=server->aggrestats.qcount; stats->qmax=server->aggrestats.qmax;
  stats->rsum=server->aggrestats.rsum; stats->rsum2=server->aggrestats.rsum2;
  stats->rcount=server->aggrestats.rcount; stats->rmax=server->aggrestats.rmax;
  stats->wsum=server->aggrestats.wsum; stats->wsum2=server->aggrestats.wsum2;
  stats->wcount=server->aggrestats.wcount; stats->wmax=server->aggrestats.wmax;
  stats->xsum=server->aggrestats.xsum; stats->xsum2=server->aggrestats.xsum2;
  stats->xcount=server->aggrestats.xcount; stats->xmax=server->aggrestats.xmax;

  stats->n_errs=server->n_errs;

  /* Now add up everything from the current clients */
  clients=server->clients; lim=server->max_slot;
  while (i<lim) {
    u8_client cl=clients[i++];
    if (cl) {
      if (cl->started>0) stats->n_busy++;
      if (cl->active>0) stats->n_active++;
      if (cl->reading>0) stats->n_reading++;
      if (cl->writing>0) stats->n_writing++;      

      stats->tsum+=cl->stats.tsum; stats->tsum2+=cl->stats.tsum2;
      stats->tcount+=cl->stats.tcount;
      if (cl->stats.tmax>stats->tmax) stats->tmax=cl->stats.tmax;

      stats->qsum+=cl->stats.qsum; stats->qsum2+=cl->stats.qsum2;
      stats->qcount+=cl->stats.qcount;
      if (cl->stats.qmax>stats->qmax) stats->qmax=cl->stats.qmax;

      stats->rsum+=cl->stats.rsum; stats->rsum2+=cl->stats.rsum2;
      stats->rcount+=cl->stats.rcount;
      if (cl->stats.rmax>stats->rmax) stats->rmax=cl->stats.rmax;

      stats->wsum+=cl->stats.wsum; stats->wsum2+=cl->stats.wsum2;
      stats->wcount+=cl->stats.wcount;
      if (cl->stats.wmax>stats->wmax) stats->wmax=cl->stats.wmax;

      stats->xsum+=cl->stats.xsum; stats->xsum2+=cl->stats.xsum2;
      stats->xcount+=cl->stats.xcount;
      if (cl->stats.xmax>stats->xmax) stats->xmax=cl->stats.xmax;

      stats->n_errs+=cl->n_errs;}}
  u8_unlock_mutex(&(server->lock));
  return stats;
}

U8_EXPORT u8_server_stats u8_server_curstats
  (u8_server server,struct U8_SERVER_STATS *stats)
{
  int i=0, lim;
  struct U8_CLIENT **clients; long long cur, start=0;
  if (stats==NULL) stats=u8_alloc(struct U8_SERVER_STATS);
  memset(stats,0,sizeof(struct U8_SERVER_STATS));
  u8_lock_mutex(&(server->lock));
  stats->n_reqs=server->n_trans;
  stats->n_errs=server->n_errs;
  stats->n_complete=server->aggrestats.tcount;
  clients=server->clients; lim=server->max_slot;
  cur=u8_microtime();
  while (i<lim) {
    u8_client cl=clients[i++];
    if (cl) {
      if (cl->started>0) stats->n_busy++;
      if (cl->active>0) stats->n_active++;
      if (cl->reading>0) stats->n_reading++;
      if (cl->writing>0) stats->n_writing++;      
      if ((start=cl->started)) {
	long long interval=cur-start;
	stats->tsum+=interval; stats->tsum2+=(interval*interval);
	if (interval>stats->tmax) stats->tmax=interval;
      	stats->tcount++;}
      if ((start=cl->queued)) {
	long long interval=cur-start;
	stats->qsum+=interval; stats->qsum2+=(interval*interval);
	if (interval>stats->qmax) stats->qmax=interval;
      	stats->qcount++;}
      else {}
      if ((start=cl->reading)) {      
	long long interval=cur-start;
	stats->rsum+=interval; stats->rsum2+=(interval*interval);
	if (interval>stats->rmax) stats->rmax=interval;
	stats->rcount++;}
      else if ((start=cl->writing)) {
	long long interval=cur-start;
	stats->wsum+=interval; stats->wsum2+=(interval*interval);
	if (interval>stats->wmax) stats->wmax=interval;
	stats->wcount++;}
      else if ((start=cl->running)) {
	long long interval=cur-start;
	stats->xsum+=interval; stats->xsum2+=interval*interval;
	if (interval>stats->xmax) stats->xmax=interval;
	stats->xcount++;}}}
  u8_unlock_mutex(&(server->lock));
  return stats;
}

U8_EXPORT
u8_string u8_server_status(struct U8_SERVER *server,u8_byte *buf,int buflen)
{
  struct U8_OUTPUT out;
  if (buf) {U8_INIT_FIXED_OUTPUT(&out,buflen,buf);}
  else {U8_INIT_OUTPUT(&out,256);}
  u8_lock_mutex(&(server->lock));
  u8_printf
    (&out,
     "%s Config: %d/%d/%d threads/maxqueue/backlog; Clients: %d/%d/%d busy/active/ever; Requests: %d/%d/%d live/queued/total;\n",
     server->server_info->idstring,
     server->n_threads,server->max_queued,server->max_backlog,
     server->n_busy,server->n_clients,server->n_accepted,
     server->n_busy,server->n_queued,server->n_trans);
  u8_unlock_mutex(&(server->lock));
  return out.u8_outbuf;
}

U8_EXPORT
u8_string u8_server_status_raw(struct U8_SERVER *server,u8_byte *buf,int buflen)
{
  struct U8_OUTPUT out;
  if (buf) {U8_INIT_FIXED_OUTPUT(&out,buflen,buf);}
  else {U8_INIT_OUTPUT(&out,256);}
  u8_lock_mutex(&(server->lock));
  u8_printf
    (&out,"%s\t%d\%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
     server->server_info->idstring,
     server->n_threads,server->max_queued,server->max_backlog,
     server->n_busy,server->n_clients,server->n_accepted,
     server->n_busy,server->n_queued,server->n_trans);
  u8_unlock_mutex(&(server->lock));
  return out.u8_outbuf;
}

/* Initialize */

U8_EXPORT void u8_init_srvfns_c()
{
  u8_register_source_file(_FILEINFO);
}
