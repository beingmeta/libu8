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

#ifndef LIBU8_U8SRVFNS_H
#define LIBU8_U8SRVFNS_H 1
#define LIBU8_U8SRVFNS_H_VERSION __FILE__

/** \file u8srvfns.h
    These functions provide a scaffolding for internet servers
     of various sorts, implementing a simple thread pool and listener
     model.
 **/

typedef struct U8_SERVER *u8_server;

#define U8_CLIENT_OPEN 1
#define U8_CLIENT_CLOSING 2
#define U8_CLIENT_CLOSED 4
#define U8_CLIENT_LOG_TRANSACT 8
#define U8_CLIENT_ASYNC 16
#define U8_CLIENT_FLAG_MAX 16

#define U8_CLIENT_FIELDS                      \
  u8_socket socket;                           \
  unsigned int flags, n_trans, n_errs;        \
  u8_utime started, queued, active;           \
  u8_utime reading, writing;                  \
  u8_string idstring;                         \
  unsigned char *buf;                         \
  size_t off, len, buflen, delta;             \
  unsigned int ownsbuf:1, grows:1;            \
  /* Tracking total transaction time */       \
  long long tsum, tsum2, tmax; int tcount;    \
  /* Tracking total spent reading */          \
  long long rsum, rsum2, rmax; int rcount;    \
  /* Tracking total spent writing */          \
  long long wsum, wsum2, wmax; int wcount;    \
  /* Tracking total spent in event loop */    \
  long long asum, asum2, amax; int acount;    \
  /* Tracking total spent in handler */       \
  long long xsum, xsum2, xmax; int xcount;    \
  int (*callback)(struct U8_CLIENT *,void *); \
  void *cbdata;                               \
  struct U8_SERVER *server

/** struct U8_CLIENT
     This structure represents a particular live client of a server.
     The socket field is the socket going to the client, the flags field
     contains flags describing the configuration and state of the client,
     the idstring field is a string identify the client for logging and
     debugging, and the server field is the server (a U8_SERVER struct)
     to which the client is connected. **/
typedef struct U8_CLIENT {
  u8_socket socket;
  unsigned int flags, n_trans, n_errs;
  u8_utime started, queued, active;
  u8_utime reading, writing;
  u8_string idstring;
  unsigned char *buf;
  size_t off, len, buflen, delta;
  unsigned int ownsbuf:1, grows:1;
  /* Tracking total transaction time */
  long long tsum, tsum2, tmax; int tcount;
  /* Tracking total spent reading */
  long long rsum, rsum2, rmax; int rcount;
  /* Tracking total spent writing */
  long long wsum, wsum2, wmax; int wcount;
  /* Tracking total spent in event loop */
  long long asum, asum2, amax; int acount;
  /* Tracking total spent in handler */
  long long xsum, xsum2, xmax; int xcount;
  int (*callback)(struct U8_CLIENT *,void *);
  void *cbdata;
  struct U8_SERVER *server;} U8_CLIENT;
typedef struct U8_CLIENT *u8_client;

/* u8_client_init:
 Initializes the client structure.
 @param client a pointer to a client stucture, or NULL if one is to be consed
 @param len the length of the structure, as allocated or to be consed
 @param sock a socket (or -1) for the client
 @param srv a pointer to the server launching the client
 Returns: a pointer to a client structure, consed if not provided.
*/
U8_EXPORT u8_client u8_client_init(u8_client client,size_t len,
				   struct sockaddr *addrbuf,size_t addrlen,
				   u8_socket sock,u8_server srv);

/** Forces a client to be closed.
     @param cl a pointer to a U8_CLIENT struct
     @returns void
**/
U8_EXPORT void u8_client_close(u8_client cl);
#if U8_THREADS_ENABLED
/** Declares that client is done with its request and can receive others
     @param cl a pointer to a U8_CLIENT struct
     @returns void
**/
U8_EXPORT void u8_client_done(u8_client cl);
#else
#define u8_client_done(cl)
#endif

#define U8_SERVER_CLOSED 1
#define U8_SERVER_LOG_LISTEN 2
#define U8_SERVER_LOG_CONNECT 4
#define U8_SERVER_LOG_TRANSACT 8
#define U8_SERVER_ASYNC 16

/** struct U8_SERVER_INFO 
    describes information about a particular server socket used
     to listen for new connections. 
**/
typedef struct U8_SERVER_INFO {
  u8_socket socket; u8_string idstring;
  struct sockaddr *addr;} U8_SERVER_INFO;
typedef struct U8_SERVER_INFO *u8_server_info;

/** struct U8_SERVER
     This structure represents a server's state and connections.
**/
typedef struct U8_SERVER {
  fd_set servers, clients, reading, writing;
  /* n_servers is the number of sockets being listened on,
     server_info describes each one. */
  int n_servers, flags; struct U8_SERVER_INFO *server_info;
  /* This array maps socket numbers to clients */
  struct U8_CLIENT **socketmap;
  int n_clients; /* Total number of live clients */
  int n_busy; /* How many clients are currently busy (in the middle of transactions) */
  int n_accepted; /* How many connections have been accepted to date */
  int n_trans; /* How many transactions have been completed to date */
  int n_errs; /* How many transactions yielded errors */
  u8_socket socket_max; /* Largest open socket */
  int socket_lim; /* The size of socketmap */
  /* Tracking total transaction time */
  long long tsum, tsum2, tmax; int tcount;
  /* Tracking total spent reading */
  long long rsum, rsum2, rmax; int rcount;
  /* Tracking total spent writing */
  long long wsum, wsum2, wmax; int wcount;
  /* Tracking total spent in event loop */
  long long asum, asum2, amax; int acount;
  /* Tracking total spent in handler */
  long long xsum, xsum2, xmax; int xcount;
  /* Handling functions */
  u8_client (*acceptfn)(struct U8_SERVER *,u8_socket sock,
			struct sockaddr *,size_t);
  int (*servefn)(u8_client);
  int (*closefn)(u8_client);
  int (*readyfn)(u8_client);
  /* Miscellaneous data */
  void *serverdata;
  /* We don't handle non-threaded right now. */
#if U8_THREADS_ENABLED
  u8_mutex lock;
  u8_condvar empty, full;
  /* n_tasks is the number of waiting requests in ->queue.
     max_tasks is the space available in ->queue
     n_threads is the number of threads in the pool */
  int n_queued, max_queued, n_threads, max_backlog;
  pthread_t *thread_pool;
  u8_client *queue;
  int queue_len, queue_head, queue_tail;
#endif
} U8_SERVER;

U8_EXPORT int u8_server_init(struct U8_SERVER *,int,int,int,
			     u8_client (*acceptfn)(u8_server,u8_socket,
						   struct sockaddr *,size_t),
			     int (*servefn)(u8_client),
			     int (*closefn)(u8_client));


/** Configures a server to listen on a particular address (port/host or file)
     @param server a pointer to a U8_SERVER struct
     @param hostname (a utf-8 string) the hostname to listen for
     @param port (an int) the port to listen on
     @returns void
**/
U8_EXPORT int u8_add_server(struct U8_SERVER *server,char *hostname,int port);

/** Starts a loop listening for requests to a server
     @param server a pointer to a U8_SERVER struct
     @returns void
**/
U8_EXPORT void u8_server_loop(struct U8_SERVER *server);

/** Shuts down a server, stopping listening and closing all
     connections when completed.
     @param server a pointer to a U8_SERVER struct
     @returns 1 if the server was newly and successfully closed
**/
U8_EXPORT int u8_server_shutdown(struct U8_SERVER *server);

/* Server Status */

typedef struct U8_SERVER_STATS {
  int n_reqs, n_errs, n_complete, n_busy, n_active, n_reading, n_writing;
  /* Tracking total transaction time */
  long long tsum, tsum2, tmax; int tcount;
  /* Tracking total spent reading */
  long long rsum, rsum2, rmax; int rcount;
  /* Tracking total spent writing */
  long long wsum, wsum2, wmax; int wcount;
  /* Tracking total spent in event loop */
  long long asum, asum2, amax; int acount;
  /* Tracking total spent in handler */
  long long xsum, xsum2, xmax; int xcount;}
  U8_SERVER_STATS;
typedef struct U8_SERVER_STATS *u8_server_stats;

U8_EXPORT u8_server_stats u8_server_statistics(u8_server,struct U8_SERVER_STATS *);
/** Returns activity statistics for a server
     @param server a pointer to a U8_SERVER struct
     @param stats a pointer to a U8_SERVER_STATS structure, or NULL
     @returns a pointer to a U8_SERVER_STATS structure (allocated if neccessary)
**/


/** Returns a user readable string describing the server's status,
     including active and pending tasks, total transactions, etc.
     @param server a pointer to a U8_SERVER struct
     @param buf NULL or a pointer to a buffer for the report
     @param buflen the size of buf if non NULL
     @returns the string of status information; if BUF was NULL, this
       was mallocd; otherwise, it is BUF with the output truncated
       to buflen
**/
U8_EXPORT u8_string u8_server_status(struct U8_SERVER *server,u8_byte *buf,int buflen);

/** Returns a machine readable (tab-separated) string describing the server's status,
     including active and pending tasks, total transactions, etc.
     @param server a pointer to a U8_SERVER struct
     @param buf NULL or a pointer to a buffer for the report
     @param buflen the size of buf if non NULL
     @returns the string of status information; if BUF was NULL, this
       was mallocd; otherwise, it is BUF with the output truncated
       to buflen
**/
U8_EXPORT u8_string u8_server_status_raw(struct U8_SERVER *server,u8_byte *buf,int buflen);

U8_EXPORT int u8_select(fd_set *reading,fd_set *writing,int max);

#endif /* U8_U8SRVFNS_H */
