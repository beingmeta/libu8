/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2012 beingmeta, inc.
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

#ifndef MAX_BACKLOG
#define MAX_BACKLOG 16
#endif

#include "libu8/libu8io.h"
#include "libu8/u8filefns.h"
#include "libu8/u8netfns.h"
#include "libu8/u8srvfns.h"
#include "libu8/u8timefns.h"
#include <unistd.h>
#include <limits.h>
/* #include <poll.h> */

static fd_set _NULL_FDS, *NULL_FDS=&_NULL_FDS;

static u8_condition ClosedClient=_("Closed connection");
static u8_condition ServerShutdown=_("Shutting down server");
static u8_condition NewServer=_("New server port");
static u8_condition NewClient=_("New connection");
static u8_condition RejectedConnection=_("Rejected connection");
static u8_condition ClientRequest=_("Client request");
static u8_condition BadSocket=_("Bad socket value");
static u8_condition Inconsistency=_("Internal inconsistency");

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
/* u8_client_init:
    Arguments: a pointer to a client
    Returns: void
 Initializes the client structure.
 @arg client a pointer to a client stucture, or NULL if one is to be consed
 @arg len the length of the structure, as allocated or to be consed
 @arg sock a socket (or -1) for the client
 @arg server the server of which the client will be a part
 Returns: a pointer to a client structure, consed if not provided.
*/
u8_client u8_client_init(u8_client client,size_t len,
			 struct sockaddr *addrbuf,size_t addrlen,
			 u8_socket sock,u8_server srv)
{
  if (!(client)) client=u8_malloc(len);
  memset(client,0,len);
  client->socket=sock;
  client->server=srv;
  client->started=client->queued=client->active=-1;
  client->reading=client->writing=-1;
  client->off=client->len=client->buflen=client->delta=0;
  if ((srv->flags)&U8_SERVER_ASYNC)
    client->flags=client->flags|U8_CLIENT_ASYNC;
  return client;
}

U8_EXPORT
/* u8_client_done:
    Arguments: a pointer to a client
    Returns: void
 Marks the client's current task as done, updating server data structures
 appropriately. */
int u8_client_done(u8_client cl)
{
  if (cl->started>0) {
    U8_SERVER *server=cl->server;
    if (server->donefn) {
      int retval=server->donefn(cl);
      if (retval<0) {
	u8_log(LOG_ERR,"u8_client_done",
	       "Error when closing client %lx (#%d) (%s)",
	       (unsigned long long)cl,cl->socket,cl->idstring);
	u8_clear_errors(1);}}
    u8_lock_mutex(&(server->lock));
    server->n_busy--;
    if (cl->started>0) {
      u8_utime cur=u8_microtime();
      long long ttime=cur-cl->started;
      cl->tsum+=ttime;
      cl->tsum2+=(ttime*ttime);
      if (ttime>cl->tmax) cl->tmax=ttime;
      cl->tcount++;
      cl->started=-1;}
    cl->active=cl->writing=-1;
    if (cl->queued>0) {
      u8_log(LOG_WARNING,"u8_client_done",
	     "Finishing transaction on a queued client %lx (#%d) (%s)",
	     (unsigned long long)cl,cl->socket,cl->idstring);
      cl->queued=-1;}
    if (cl->socket>0) {
      FD_SET(cl->socket,&server->reading);
      FD_CLR(cl->socket,&server->writing);}
    u8_unlock_mutex(&(server->lock));
    return 1;}
  else return 0;
}

/* Closing clients */

static void update_server_stats(u8_client cl);

U8_EXPORT
/* u8_client_close:
    Arguments: a pointer to a client
    Returns: void
 Marks the client's current task as done, updating server data structures
 appropriately, and ending by closing the client close function.
 If a busy client is closed, it has its U8_CLIENT_CLOSING flag set.
 This tells the event loop to call the close function when the client
 server function actually returns.
*/
int u8_client_close(u8_client cl)
{
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    U8_SERVER *server=cl->server; u8_socket sock=cl->socket;
    u8_lock_mutex(&(server->lock));
    /* Catch race conditions */
    if (cl->flags&U8_CLIENT_CLOSED) {
      u8_unlock_mutex(&(server->lock));
      return 0;}
    if (cl->started>0) {
      /* The client is in the middle of something, so we let it
	 finish. */
      cl->flags=cl->flags|(U8_CLIENT_CLOSING);
      u8_unlock_mutex(&(server->lock));
      return 0;}
    else {
      u8_string idstring=cl->idstring; int rv=0;
      if (cl->started>0) {
	u8_utime cur=u8_microtime();
	long long ttime=cur-cl->started;
	cl->tsum+=ttime;
	cl->tsum2+=(ttime*ttime);
	if (ttime>cl->tmax) cl->tmax=ttime;
	cl->tcount++;
	cl->started=-1;}
      cl->reading=cl->writing=cl->active=cl->writing=0;
      if (cl->queued>0) {
	u8_log(LOG_WARNING,"u8_client_done",
	       "Closing a queued client %lx (#%d) (%s)",
	       (unsigned long long)cl,cl->socket,cl->idstring);
	cl->queued=-1;}
      if (sock>0) {
	server->socketmap[sock]=NULL;
	FD_CLR(cl->socket,&server->clients);
	FD_CLR(cl->socket,&server->reading);
	FD_CLR(cl->socket,&server->writing);}
      server->n_clients--; server->n_busy--;
      cl->flags=cl->flags|U8_CLIENT_CLOSED;
      update_server_stats(cl);
      u8_unlock_mutex(&(server->lock));
      if (server->closefn)
	rv=server->closefn(cl);
      else if (sock>0)
	rv=close(sock);
      else rv=0;
      if (rv<0) {
	u8_log(LOG_ERR,ClosedClient,
	       "Error closing @x%lx#%d (%s)",
	       ((unsigned long)cl),sock,idstring);
	u8_graberrno("u8_close_client",u8s(idstring));
	u8_clear_errors(1);}
      else if (cl->server->flags&U8_SERVER_LOG_CONNECT)
	u8_log(LOG_INFO,ClosedClient,"Closed #%d for %s",sock,idstring);
      else {}
      if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
      u8_free(idstring);
      u8_free(cl);
      return 1;}}
  else {
    u8_log(LOG_WARN,"u8_client_close",
	   "Closing already closed socket @x%lx#%d (%s)",
	   ((unsigned long)cl),cl->socket,cl->idstring);
    return 0;}
}

/* This is the internal version used when shutting down a server.
   We dont wait for anything. */
static int client_close(u8_client cl)
{
  int retval=0;
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    U8_SERVER *server=cl->server; u8_socket sock=cl->socket;
    cl->active=cl->reading=cl->started=cl->queued=cl->writing=-1;
    if (sock>0) {
      server->socketmap[sock]=NULL;
      FD_CLR(sock,&server->clients);
      FD_CLR(sock,&server->reading);
      FD_CLR(sock,&server->writing);}
    server->n_clients--;
    if (cl->started>0) {
      /* It's in the middle of something, so we just flag it as closing. */
      cl->flags=cl->flags|(U8_CLIENT_CLOSING);
      return 0;}
    else {
      u8_string idstring=cl->idstring;
      cl->flags=cl->flags|U8_CLIENT_CLOSED;
      if (server->closefn) retval=server->closefn(cl);
      else if (sock>0) {
	retval=close(sock);
	if (retval<0) cl->socket=-(sock);
	else cl->socket=0;}
      else retval=0;
      if (retval<0) {
	u8_log(LOG_ERR,ClosedClient,
	       "Error while closing %s client %d (%s)",
	       server->serverid,socket,cl->idstring);
	u8_graberrno("client_close",u8s(cl->idstring));
	u8_clear_errors(1);}
      if (cl->socket>0) retval=close(cl->socket);
      else retval=0;
      if (retval<0) {
	u8_log(LOG_ERR,ClosedClient,
	       "Error while closing socket %d for %s client %d (%s)",
	       sock,server->serverid,sock,cl->idstring);
	u8_graberrno("client_close",u8s(cl->idstring));
	u8_clear_errors(1);}
      else if (cl->server->flags&U8_SERVER_LOG_CONNECT)
	u8_log(LOG_INFO,ClosedClient,"Closed #%d for %s",sock,idstring);
      if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
      server->socketmap[sock]=NULL;
      u8_free(idstring);
      u8_free(cl);
      return 1;}}
  else return 0;
}

/* This is used when a transaction finishes and the socket has been
   closed during shutdown. */
static void finish_client_close(u8_client cl)
{
  if ((cl->flags&U8_CLIENT_CLOSED)==0) {
    U8_SERVER *server=cl->server; u8_socket sock=cl->socket;
    /* We grab idstring, because our code allocated it but we will use it
       only after the closefn has freed the client object. */
    u8_string idstring=cl->idstring;
    u8_lock_mutex(&(server->lock));
    /* Catch race conditions */
    if (cl->flags&U8_CLIENT_CLOSED) {
      u8_unlock_mutex(&(server->lock));
      return;}
    cl->reading=cl->writing=cl->active=cl->writing=0;
    if (cl->queued>0) {
      u8_log(LOG_WARN,"u8_client_done",
	     "Finishing close on a queued client %lx (#%d) (%s)",
	     (unsigned long long)cl,cl->socket,cl->idstring);
      cl->queued=-1;}
    if (sock>0) {
      server->socketmap[sock]=NULL;
      FD_CLR(cl->socket,&server->clients);
      FD_CLR(cl->socket,&server->reading);
      FD_CLR(cl->socket,&server->writing);}
    server->n_clients--;
    cl->flags=cl->flags&U8_CLIENT_CLOSED;
    server->n_busy--;
    if (cl->started>0) {
      u8_utime cur=u8_microtime();
      long long ttime=cur-cl->started;
      cl->tsum+=ttime;
      cl->tsum2+=(ttime*ttime);
      if (ttime>cl->tmax) cl->tmax=ttime;
      cl->tcount++;
      cl->started=-1;}
    
    /* Transfer them to the server */
    update_server_stats(cl);

    u8_unlock_mutex(&(server->lock));
    server->closefn(cl);
    if (cl->server->flags&U8_SERVER_LOG_CONNECT)
      u8_log(LOG_INFO,ClosedClient,"Closed #%d for %s",sock,idstring);
    if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
    u8_free(idstring);
    u8_free(cl);}
}

static void update_server_stats(u8_client cl)
{
  u8_server server=cl->server;
  /* Transfer all the client statistics to the server */
  server->tsum+=cl->tsum; server->tsum2+=cl->tsum2; server->tcount+=cl->tcount;
  if (cl->tmax>server->tmax) server->tmax=cl->tmax;
  server->asum+=cl->asum; server->asum2+=cl->asum2; server->acount+=cl->acount;
  if (cl->amax>server->amax) server->amax=cl->amax;
  server->rsum+=cl->rsum; server->rsum2+=cl->rsum2; server->rcount+=cl->rcount;
  if (cl->rmax>server->rmax) server->rmax=cl->rmax;
  server->wsum+=cl->wsum; server->wsum2+=cl->wsum2; server->wcount+=cl->wcount;
  if (cl->wmax>server->wmax) server->wmax=cl->wmax;
  server->xsum+=cl->xsum; server->xsum2+=cl->xsum2; server->xcount+=cl->xcount;
  if (cl->xmax>server->xmax) server->xmax=cl->xmax;
  server->n_errs+=cl->n_errs;
}

#if U8_THREADS_ENABLED
static u8_client pop_task(struct U8_SERVER *server)
{
  u8_client task=NULL;
  u8_lock_mutex(&(server->lock));
  while ((server->n_queued == 0) && ((server->flags&U8_SERVER_CLOSED)==0)) 
    u8_condvar_wait(&(server->empty),&(server->lock));
  if (server->flags&U8_SERVER_CLOSED) {}
  else if (server->n_queued) {
    task=server->queue[server->queue_head++];
    if (server->queue_head>=server->queue_len) server->queue_head=0;
    server->n_queued--;}
  else {}
  if ((task)&&((task->active>0))) {
    /* This should probably never happen */
    u8_log(LOG_CRIT,"pop_task(u8)","popping active task %lx (#%d) (%s)",
	   task,task->socket,task->idstring);
    task->queued=-1; task=NULL;}
  else if (task) {
    u8_utime curtime=u8_microtime();
    task->queued=-1; task->active=curtime;
    if (task->started<0) {
      task->started=curtime;
      task->n_trans++; server->n_trans++;
      server->n_busy++;}}
  else {}
  u8_unlock_mutex(&(server->lock));
  return task;
}

static int push_task(struct U8_SERVER *server,u8_client cl)
{
  if (server->n_queued >= server->max_queued) return 0;
  if (cl->queued>0) return 0;
  server->queue[server->queue_tail++]=cl;
  if (server->queue_tail>=server->queue_len) server->queue_tail=0;
  server->n_queued++;
  if (cl->active>0) {
    u8_utime cur=u8_microtime();
    long long atime=cur-cl->active;
    cl->asum+=atime;
    cl->asum2+=(atime*atime);
    if (atime>cl->amax) cl->amax=atime;
    cl->acount++;
    cl->queued=cur;
    cl->active=-1;}
  else cl->queued=u8_microtime(); 
  u8_condvar_signal(&(server->empty));
  return 1;
}

static void *event_loop(void *thread_arg)
{
  struct U8_SERVER *server=(struct U8_SERVER *)thread_arg;
  /* Check for additional thread init functions */
  while (1) {
    u8_client cl; int dobreak=0; int result=0, closed=0;
    u8_utime cur;
    /* Check that this thread's init functions are up to date */
    u8_threadcheck();
    cl=pop_task(server);
    if (!(cl)) continue;
    if ((cl->socket<0)||(cl->socket>server->socket_max)||
	(!(FD_ISSET(cl->socket,(&(server->clients)))))) {
      u8_log(LOG_ERR,BadSocket,"Bad socket value %d for %lx",
	     cl->socket,(unsigned long)cl);
      cl->active=-1;
      continue;}
    if (((server->flags)&(U8_SERVER_LOG_TRANSACT))||
	((cl->flags)&(U8_CLIENT_LOG_TRANSACT)))
      u8_log(LOG_DEBUG,ClientRequest,
	     "Handling activity from %lx (#%d) %s[%d]",
	     ((unsigned long)cl),cl->socket,cl->idstring,cl->n_trans);
    if ((!(cl))&&(server->flags&U8_SERVER_CLOSED)) {
      cl->active=-1;
      break;}
    else if (!(cl)) {cl->active=-1;}
    else if ((cl->reading>0)||(cl->writing>0)) {
      size_t delta;
      if (cl->off<cl->len) {
	/* We're in the middle of reading or writing a chunk of data */
	if (cl->writing>0)
	  delta=write(cl->socket,cl->buf+cl->off,cl->len-cl->off);
	else delta=read(cl->socket,cl->buf+cl->off,cl->len-cl->off);
	u8_log(LOG_DEBUG,((cl->writing>0)?("Writing"):("Reading")),
	       "Processed %d bytes for %d",delta,cl->socket);
	if (delta>0) cl->off=cl->off+delta;}
      /* If we've still got data to read/write, we continue,
	 otherwise, we fall through */
      if (cl->off<cl->len) {
	/* Keep listening */
	u8_lock_mutex(&server->lock);
	if (cl->socket>0) {
	  if (cl->writing>0) {
	    FD_SET(cl->socket,&server->writing);
	    FD_CLR(cl->socket,&server->reading);}
	  else {
	    FD_SET(cl->socket,&server->reading);
	    FD_CLR(cl->socket,&server->writing);}}
	u8_unlock_mutex(&server->lock);
	result=1;}
      else {
	u8_utime cur=u8_microtime(); long long xtime;
	if (cl->writing>0) {
	  long long wtime=cur-cl->writing;
	  cl->wsum+=wtime;
	  cl->wsum2+=(wtime*wtime);
	  if (wtime>cl->wmax) cl->wmax=wtime;
	  cl->wcount++;}
	else {
	  long long rtime=cur-cl->reading;
	  cl->rsum+=rtime;
	  cl->rsum2+=(rtime*rtime);
	  if (rtime>cl->rmax) cl->rmax=rtime;
	  cl->rcount++;}
	if (cl->callback) {
	  void *state=cl->cbstate;
	  u8_client_callback callback=cl->callback;
	  cl->callback=NULL; cl->cbstate=NULL;
	  result=callback(cl,state);}
	else result=server->servefn(cl);
	xtime=u8_microtime()-cur;
	cl->xsum+=xtime;
	cl->xsum2+=(xtime*xtime);
	if (xtime>cl->xmax) cl->xmax=xtime;
	cl->xcount++;}}
    else {
      u8_utime cur=u8_microtime(); long long xtime;
      result=server->servefn(cl);
      xtime=u8_microtime()-cur;
      cl->xsum+=xtime;
      cl->xsum2+=(xtime*xtime);
      if (xtime>cl->xmax) cl->xmax=xtime;
      cl->xcount++;}
    /* When the servefn is called, cl->async=0 and there are three
       possible states:
       1. no asynchrony is going on (buf is NULL)
       2. buf is full (and the read/write is complete) and either
       ...a. we were reading (cl->writing==0) or
       ...b. we were writing (cl->writing==1)
    */
    if (result<0) {
      u8_exception ex=u8_current_exception;
      u8_log(LOG_ERR,"event_loop",
	     "Error result from client %lx (#%d) (%s)[%d]",
	     ((unsigned long)cl),cl->socket,cl->idstring,cl->n_trans);
      if (cl->flags&U8_CLIENT_CLOSED) {}
      else if (cl->flags&U8_CLIENT_CLOSING) {
	finish_client_close(cl); closed=1;
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else if (cl->active>0) u8_client_done(cl);
      while (ex) {
	u8_log(LOG_WARN,ClientRequest,
	       "Error during activity on %lx (#%d) %s[%d] (%s)",
	       ((unsigned long)cl),cl->socket,cl->idstring,
	       cl->n_trans,u8_errstring(ex));
	ex=u8_pop_exception();}
      cl->n_errs++;}
    else if (result==0) {
      u8_utime cur=u8_microtime();
      long long ttime=cur-cl->started;
      /* Request is completed */
      if (((server->flags)&U8_SERVER_LOG_TRANSACT)||
	  ((cl->flags)&U8_CLIENT_LOG_TRANSACT))
	u8_log(LOG_INFO,ClientRequest,
	       "Completed transaction with %lx (#%d) %s[%d]",
	       (unsigned long)cl,cl->socket,cl->idstring,cl->n_trans);
      if (server->flags&U8_SERVER_CLOSED) dobreak=1;
      if (cl->flags&U8_CLIENT_CLOSED) {
	u8_log(LOG_CRIT,Inconsistency,
	       "Result returned from closed client %lx %s",
	       ((unsigned long)cl),cl->idstring);
	closed=1; if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else if (cl->flags&U8_CLIENT_CLOSING) {
	finish_client_close(cl); closed=1;
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}
      else {
	if (cl->active>0) u8_client_done(cl);
	if ((cl->buf)&&(cl->ownsbuf)) u8_free(cl->buf);
	cl->buf=NULL; cl->off=cl->len=cl->buflen=0; cl->ownsbuf=0;}}
    else if (((cl->reading>0)||(cl->writing>0))&&(cl->buf!=NULL)) {
      /* The execution queued some input or output */
      ssize_t delta;
      if (cl->writing>0)
	delta=write(cl->socket,cl->buf+cl->off,((size_t)(cl->len-cl->off)));
      else delta=read(cl->socket,cl->buf+cl->off,((size_t)(cl->len-cl->off)));
      u8_log(LOG_DEBUG,((cl->writing>0)?("Writing"):("Reading")),
	     "Processed %d bytes for %d",delta,cl->socket);
      if (delta>0) cl->off=cl->off+delta;
      /* If we've still got data to read/write, we continue,
	 otherwise, we fall through */
      if (cl->off<cl->len) {
	/* Keep listening */
	u8_lock_mutex(&server->lock);
	if (cl->socket>0) {
	  if (cl->writing>0) {
	    FD_SET(cl->socket,&server->writing);
	    FD_CLR(cl->socket,&server->reading);}
	  else {
	    FD_SET(cl->socket,&server->reading);
	    FD_CLR(cl->socket,&server->writing);}}
	u8_unlock_mutex(&server->lock);}
      else push_task(server,cl);}
    else {
      /* Request is not yet completed */
      if (((server->flags)&U8_SERVER_LOG_TRANSACT)||
	  ((cl->flags)&U8_CLIENT_LOG_TRANSACT))
	u8_log(LOG_DEBUG,ClientRequest,
	       "Yield during request for %lx (#%d) %s[%d]",
	       ((unsigned long)cl),cl->socket,cl->idstring,cl->n_trans);
     /* We need to handle the case where the request is not completed
	 but the server is closing down.  Right now, we don't let the
	 server loop exit until all the pending tasks have closed.  */}
    if (cl->active>0) {
      u8_utime cur=u8_microtime();
      long long atime=cur-cl->active;
      cl->asum+=atime;
      cl->asum2+=(atime*atime);
      if (atime>cl->amax) cl->amax=atime;
      cl->acount++;
      cl->active=-1;}
    if (dobreak) break;
    if (!(closed)) {
      u8_lock_mutex(&server->lock);
      /* Start listening again (it was stopped by pop_task() */
      if (cl->socket>0) {
	  if (cl->writing>0) {
	    FD_SET(cl->socket,&server->writing);
	    FD_CLR(cl->socket,&server->reading);}
	  else {
	    FD_SET(cl->socket,&server->reading);
	    FD_CLR(cl->socket,&server->writing);}}
      u8_unlock_mutex(&server->lock);}}
  u8_threadexit();
  return NULL;
}
#endif

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
  int  max_clients=DEFAULT_MAX_CLIENTS;
  va_list args; int prop; int retval;
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
    case U8_SERVER_LOGLEVEL: {
      int level=(va_arg(args,int));
      if (level>2) flags=flags|U8_SERVER_LOG_TRANSACT;
      if (level>1) flags=flags|U8_SERVER_LOG_CONNECT;
      if (level>0) flags=flags|U8_SERVER_LOG_LISTEN;
      continue;}
    default:
      u8_log(LOG_CRIT,"u8_init_server",
	     "Unknown property code %d for server",prop);
      continue;}}
  va_end(args);
  
  server->n_clients=0; server->n_servers=0; server->flags=0;
  server->socket_max=0; server->socket_lim=256;
  server->max_backlog=((maxback<=0) ? (MAX_BACKLOG) : (maxback));
  server->socketmap=u8_alloc_n(256,u8_client);
  server->server_info=NULL;
  FD_ZERO(&server->servers);
  FD_ZERO(&server->clients);
  FD_ZERO(&server->reading);
  FD_ZERO(&server->writing);
  memset(server->socketmap,0,sizeof(u8_client)*256);
  server->acceptfn=acceptfn;
  server->servefn=servefn;
  server->closefn=closefn;
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
		   int maxback,int max_queued,int n_threads,
		   u8_client (*acceptfn)(u8_server,u8_socket,
					 struct sockaddr *,size_t),
		   int (*servefn)(u8_client),
		   int (*closefn)(u8_client))
{
 if (u8_init_server
      (server,acceptfn,servefn,NULL,closefn,
       U8_SERVER_NTHREADS,n_threads,
       U8_SERVER_INIT_CLIENTS,max_queued,
       U8_SERVER_MAX_QUEUE,max_queued,
       U8_SERVER_BACKLOG,maxback,
       -1))
    return 1;
  else return 0;
}

static int do_shutdown(struct U8_SERVER *server,int grace)
{
  int i=0, max_socket, n_servers=server->n_servers;
  int n_errs=0, idle_clients=0, active_clients=0;
  u8_utime deadline=u8_microtime()+grace;
  u8_lock_mutex(&server->lock);
  max_socket=server->socket_max;
  if (server->flags&U8_SERVER_CLOSED) return 0;
  server->flags=server->flags|U8_SERVER_CLOSED;
  /* Close all the server sockets */
  u8_log(LOG_WARN,ServerShutdown,"Closing %d listening socket(s)",n_servers);
  while (i<n_servers) {
    struct U8_SERVER_INFO *info=&(server->server_info[i++]);
    u8_socket sock=info->socket, retval;
    FD_CLR(sock,&(server->servers));
    FD_CLR(sock,&(server->reading));
    retval=close(sock);
    if (retval<0) {
      u8_log(LOG_WARN,ServerShutdown,
	     "Error (%s) closing socket %d listening at %s",
	     strerror(errno),sock,info->idstring);
      n_errs++;}
    else if (server->flags&U8_SERVER_LOG_LISTEN)
      u8_log(LOG_NOTICE,ServerShutdown,"Closed socket %d listening at %s",
	     sock,info->idstring);
    if (info->idstring) u8_free(info->idstring);
    if (info->addr) u8_free(info->addr);
    info->idstring=NULL; info->addr=NULL; info->socket=-1;}
  if (n_errs)
    u8_log(LOG_WARN,ServerShutdown,
	   "Closed %d listening socket(s) with %d errors",
	   n_servers,n_errs);
  else u8_log(LOG_NOTICE,ServerShutdown,"Closed %d listening socket(s)",
	      n_servers);
  if (server->server_info) {
    u8_free(server->server_info);
    server->server_info=NULL;}
  /* Close all the idle client sockets and mark all the busy clients closed */
  i=0; while (i<max_socket) {
    if (FD_ISSET(i,&(server->clients))) {
      if (server->socketmap[i]->started<0) {
	client_close(server->socketmap[i]);
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
    /* Wait for the busy connections to finish.
       Should this wait around somehow? */
    u8_lock_mutex(&server->lock); 
    while ((server->n_busy)&&(u8_microtime()<deadline)) {
      u8_unlock_mutex(&server->lock);
      sleep(1);
      u8_lock_mutex(&server->lock);}}
  if (server->n_busy) {
    u8_log(LOG_CRIT,ServerShutdown,
	   "Forcing %d active socket(s) closed after %dus",
	   server->n_busy,grace);
    n_errs=0; i=0; while (i<max_socket) {
      if (FD_ISSET(i,&(server->clients)))
	if (server->socketmap[i]->started>0) {
	  int rv=client_close(server->socketmap[i]);
	  if (rv<0)
	    u8_log(LOG_ERR,ServerShutdown,
		   "Error while closing client %d",i);}
      i++;}}
#endif
  u8_free(server->socketmap); server->socketmap=NULL;
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

U8_EXPORT int u8_server_shutdown(struct U8_SERVER *server,int grace)
{
  if (grace)
    server->shutdown=grace;
  else server->shutdown=-1;
  return 0;
}

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
  u8_log(LOG_WARN,_("sockopt"),"Can't set server socket to non-blocking");
#endif
}

static int open_server_socket(struct sockaddr *sockaddr,int maxbacklog)
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
    server->n_servers++;}
  info->socket=sock; info->addr=addr;
  FD_SET(sock,&(server->servers));
  FD_SET(sock,&(server->reading));
  if (sock>server->socket_max) server->socket_max=sock;
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
  else if (((strchr(spec,'@'))==NULL)&&((strchr(spec,':'))==NULL)) {
    /* Spec is just a port */
    int portno=u8_get_portno(spec);
    return u8_add_server(server,NULL,u8_get_portno(spec));}
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
  if (sock<0) return sock;
  if (sock>=server->socket_lim) {
    int new_lim=((sock/64)+1)*64;
    u8_client *newmap=
      u8_realloc_n(server->socketmap,new_lim,u8_client);
    if (newmap) {
      int i=server->socket_lim;
      while (i<new_lim) newmap[i++]=NULL;
      server->socketmap=newmap;
      server->socket_lim=new_lim;}
    else {
      u8_unlock_mutex(&(server->lock));
      u8_graberr(-1,"add_client",NULL);
      return -1;}}
  server->socketmap[sock]=client;
  FD_SET(sock,&(server->clients));
  FD_SET(sock,&(server->reading));
  if (sock>server->socket_max) server->socket_max=sock;
  client->server=server; server->n_clients++;
  return 1;
}

static int server_wait(fd_set *reading,fd_set *writing,fd_set *other,
		       int max_sockets,struct timeval *to)
{
  return select(max_sockets+1,reading,writing,other,to);
}

U8_EXPORT int u8_select(fd_set *reading,fd_set *writing,fd_set *other,int max)
{
  struct timeval tv; tv.tv_sec=0; tv.tv_usec=0;
  return select(max+1,
		((reading==NULL)?(NULL_FDS):(reading)),
		((writing==NULL)?(NULL_FDS):(writing)),
		((other==NULL)?(NULL_FDS):(other)),
		&tv);
}

#if 0
U8_EXPORT int u8_test_socket(u8_socket sock)
{
  struct pollfd _fds[5], *fds=_fds; int rv=-1;
  memset(&_fds,0,sizeof(_fds));
  _fds[0].fd=sock;
  _fds[0].events=POLLIN|POLLPRI|POLLOUT;
  rv=poll(fds,1,5);
  return ((int)(fds[0].revents));
}
#endif

static int server_accept(u8_server server,u8_socket i)
{
  /* If the activity is on the server socket, open a new socket */
  char addrbuf[1024];
  int addrlen=1024; u8_socket sock;
  memset(addrbuf,0,1024);
  sock=accept(i,(struct sockaddr *)addrbuf,&addrlen);
  if (sock < 0) {
    u8_log(LOG_ERR,u8_NetworkError,_("Failed accept on socket %d"),i);
    errno=0;
    return -1;}
  else {
    u8_client new_client=server->acceptfn
      (server,sock,(struct sockaddr *)addrbuf,addrlen);
    if (new_client) {
      u8_string idstring=
	((new_client->idstring)?(new_client->idstring):
	 (u8_sockaddr_string((struct sockaddr *)addrbuf)));
      server->n_accepted++;
      if (server->flags&U8_SERVER_LOG_CONNECT) 
	u8_log(LOG_INFO,NewClient,"Opened #%d for %s",
	       sock,idstring);
      if (!(new_client->idstring)) new_client->idstring=idstring;
      add_client(server,new_client);
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
  int retval=recv(sock,buf,1,MSG_PEEK), oretval=0;
  return (retval>0);
}

/* This listens for connections and pushes tasks (unless we're not
   threaded, in which case it dispatches to the servefn right
   away).  */
static int server_listen(struct U8_SERVER *server)
{
  fd_set reading, writing, other;
  int i, max_socket, n_actions, retval;
  struct timeval _timeout, *timeout; 
  memset(&_timeout,0,sizeof(_timeout));
  _timeout.tv_sec=0; _timeout.tv_usec=500;
  u8_lock_mutex(&(server->lock));
  FD_ZERO(&reading); FD_ZERO(&writing); FD_ZERO(&other);
  reading=server->reading; writing=server->writing; other=server->clients;
  max_socket=server->socket_max;
  timeout=((server->n_clients) ? (&_timeout) : (NULL));
  u8_unlock_mutex(&(server->lock));
  /* Wait for activity on one of your open sockets */
  while ((retval=server_wait(&reading,&writing,&other,max_socket,timeout)) == 0) {
    if (retval<0) return retval;
    if (server->shutdown) {
      do_shutdown(server,server->shutdown);
      return 0;}
    u8_lock_mutex(&(server->lock));
    reading=server->reading;
    writing=server->writing;
    other=server->clients;
    timeout=((server->n_clients) ? (&_timeout) : (NULL));
    memset(&_timeout,0,sizeof(_timeout));
    _timeout.tv_usec=5000; _timeout.tv_sec=0;
    u8_unlock_mutex(&(server->lock));
    if (server->flags&U8_SERVER_CLOSED) return 0;}
  /* Iterate over the range of sockets */
  u8_lock_mutex(&(server->lock));
  i=0; while (i <= max_socket) {
    u8_client client=server->socketmap[i];
    if (server->shutdown) {
      do_shutdown(server,server->shutdown);
      return 0;}
    if (!(((FD_ISSET(i,&reading))&&(FD_ISSET(i,&server->reading)))||
	  ((FD_ISSET(i,&writing))&&(FD_ISSET(i,&server->writing)))||
	  ((FD_ISSET(i,&other))&&(FD_ISSET(i,&server->clients)))))
      /* Nothing happened here */
      i++;
    else if (FD_ISSET(i,&server->servers)) {
      /* We got a connection request */
      int retval=server_accept(server,i++);
      if (retval<=0) continue;}
    else if (client==NULL) {
      u8_log(LOG_ERR,u8_NetworkError,"Clientless socket %d!!!",i);
      i++; continue;}
    else if (client->active>0) {
      i++; continue;}
    else if ((FD_ISSET(i,&writing))&&(FD_ISSET(i,&server->writing))&&
	     (client->writing>0)) {
#if U8_THREADS_ENABLED
      if (push_task(server,client)) {
	n_actions++;
	FD_CLR(client->socket,&(server->reading));
	FD_CLR(client->socket,&(server->writing));}
      else {}
#endif
      i++;}
    else if (((FD_ISSET(i,&reading))&&
	      (FD_ISSET(i,&server->reading)))) {
#if U8_THREADS_ENABLED
      if (!(socket_peek(client->socket))) {
	/* No real data, so we close it (probably the other side closed)
	   the connection. */
	u8_client_close(client);
	i++; continue;}
      if (push_task(server,client)) {
	n_actions++;
	FD_CLR(client->socket,&(server->reading));
	FD_CLR(client->socket,&(server->writing));}
      else {}
#else
      n_actions++;
      server->servefn(client);
#endif
      i++;}
    else {
      /* What else can go here (it doesn't seem to work for closed sockets) */
      char buf[5]; int rv; ssize_t retval;
      u8_log(LOG_WARN,"other condition",
	     "Checking if socket %d (%s) was closed",
	     i,client->idstring);
      retval=recv(client->socket,buf,1,MSG_PEEK);
      if (retval<=0) u8_client_close(client);}}
  u8_unlock_mutex(&(server->lock));
  return 0;
}

U8_EXPORT
int u8_push_task(struct U8_SERVER *server,u8_client cl)
{
  int retval;
  u8_lock_mutex(&(server->lock));
  retval=push_task(server,cl);
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
  stats->n_complete=server->tcount;
  stats->tsum=server->tsum; stats->tsum2=server->tsum2; stats->tcount=server->tcount;
  stats->tmax=server->tmax;
  stats->asum=server->asum; stats->asum2=server->asum2; stats->acount=server->acount;
  stats->amax=server->amax;
  stats->rsum=server->rsum; stats->rsum2=server->rsum2; stats->rcount=server->rcount;
  stats->rmax=server->rmax;
  stats->wsum=server->wsum; stats->wsum2=server->wsum2; stats->wcount=server->wcount;
  stats->wmax=server->wmax;
  stats->xsum=server->xsum; stats->xsum2=server->xsum2; stats->xcount=server->xcount;
  stats->xmax=server->xmax;
  stats->n_errs=server->n_errs;
  clients=server->socketmap; lim=server->socket_max;
  while (i<lim) {
    u8_client cl=clients[i++];
    if (cl) {
      if (cl->started>0) stats->n_busy++;
      if (cl->active>0) stats->n_active++;
      if (cl->reading>0) stats->n_reading++;
      if (cl->writing>0) stats->n_writing++;      
      stats->tsum+=cl->tsum; stats->tsum2+=cl->tsum2; stats->tcount+=cl->tcount;
      if (cl->tmax>stats->tmax) stats->tmax=cl->tmax;
      stats->asum+=cl->asum; stats->asum2+=cl->asum2; stats->acount+=cl->acount;
      if (cl->amax>stats->amax) stats->amax=cl->amax;
      stats->rsum+=cl->rsum; stats->rsum2+=cl->rsum2; stats->rcount+=cl->rcount;
      if (cl->rmax>stats->rmax) stats->rmax=cl->rmax;
      stats->wsum+=cl->wsum; stats->wsum2+=cl->wsum2; stats->wcount+=cl->wcount;
      if (cl->wmax>stats->wmax) stats->wmax=cl->wmax;
      stats->xsum+=cl->xsum; stats->xsum2+=cl->xsum2; stats->xcount+=cl->xcount;
      if (cl->xmax>stats->xmax) stats->xmax=cl->xmax;
      stats->n_errs+=cl->n_errs;}}
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
  FD_ZERO(NULL_FDS);
  u8_register_source_file(_FILEINFO);
}
