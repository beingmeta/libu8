/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2017 beingmeta, inc.
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

U8_EXPORT int u8_server_loglevel;

#ifndef DEFAULT_TIMEOUT
#define DEFAULT_TIMEOUT 200
#endif

#define U8_CLIENT_OPEN 1
#define U8_CLIENT_ASYNC 2
#define U8_CLIENT_CLOSING 4
#define U8_CLIENT_CLOSED 8
#define U8_CLIENT_LOG_TRANSACT 16
#define U8_CLIENT_LOG_TRANSFER 32
#define U8_CLIENT_LOG_QUEUE 64
#define U8_CLIENT_FLAG_MAX 64

typedef struct U8_CLIENT *u8_client;
typedef int (*u8_client_callback)(u8_client,void *);

/** struct U8_CLIENT_STATS
    records client activity information for the u8srv library.
**/
typedef struct U8_CLIENT_STATS {
  /* Tracking total transaction time (clock time) */
  long long tsum, tsum2, tmax; int tcount;
  /* Tracking total spent queued (clock time) */
  long long qsum, qsum2, qmax; int qcount;
  /* Tracking total spent reading (clock time) */
  long long rsum, rsum2, rmax; int rcount;
  /* Tracking total spent writing (clock time) */
  long long wsum, wsum2, wmax; int wcount;
  /* Tracking total spent in handler (clock time) */
  long long xsum, xsum2, xmax; int xcount;} U8_CLIENT_STATS;
typedef struct U8_CLIENT_STATS *u8_client_stats;

#define U8_CLIENT_FIELDS                                       \
    u8_socket socket;                                          \
    int clientid, threadnum;                                   \
    unsigned int flags, n_trans, n_errs;                       \
    u8_utime started, queued, active;                          \
    u8_utime reading, writing, running;                        \
    u8_string idstring, status;                                \
    unsigned char *buf;                                        \
    size_t off, len, buflen, delta;                            \
    unsigned int ownsbuf, grows;                               \
    struct U8_CLIENT_STATS stats;                              \
    u8_client_callback callback;                               \
    void *cbstate;                                             \
    struct U8_SERVER *server

/** struct U8_CLIENT This structure represents a particular live
     client of a server.  The socket field is the socket going to the
     client, the flags field contains flags describing the
     configuration and state of the client, the idstring field and
     statestring fields are strings identifying the client for logging
     and debugging, and the server field is the server (a U8_SERVER
     struct) to which the client is connected. **/
typedef struct U8_CLIENT {
    u8_socket socket;
    int clientid, threadnum;
    unsigned int flags, n_trans, n_errs;
    u8_utime started, queued, active;
    u8_utime reading, writing, running;
    u8_string idstring, status;
    unsigned char *buf;
    size_t off, len, buflen, delta;
    unsigned int ownsbuf, grows;
    struct U8_CLIENT_STATS stats;
    u8_client_callback callback;
    void *cbstate;
    struct U8_SERVER *server;} U8_CLIENT;

/* u8_init_client:
 Initializes the client structure.
 @param client a pointer to a client stucture, or NULL if one is to be consed
 @param len the length of the structure, as allocated or to be consed
 @param sock a socket (or -1) for the client
 @param srv a pointer to the server launching the client
 Returns: a pointer to a client structure, consed if not provided.
*/
U8_EXPORT u8_client u8_init_client(u8_client client,size_t len,
                                   struct sockaddr *addrbuf,size_t addrlen,
                                   u8_socket sock,u8_server srv);
#define u8_client_init u8_init_client

/* u8_close_client:
    Arguments: a pointer to a client
    Returns: int
 Marks the client's current task as done, updating server data structures
 appropriately, and ending by closing the client close function.
 If a busy client is closed, it has its U8_CLIENT_CLOSING flag set.
 This tells the event loop to finish closing the client when it actually
 completes the current transaction.
*/
U8_EXPORT int u8_close_client(u8_client cl);
#define u8_client_close u8_close_client

/* u8_client_closed:
    Arguments: a pointer to a client
    Returns: int
  Indicates that the other end has closed the connection.  This can generate
    an indication of its own and then closes the client on this end
*/
U8_EXPORT int u8_client_closed(u8_client cl);

/* u8_client_shutdown:
    Arguments: a pointer to a client
    Returns: int
 Marks the client's current task as done, updating server data structures
 appropriately, and ending by closing the client close function.
 This will shutdown an active client.
*/
U8_EXPORT int u8_shutdown_client(u8_client cl);
#define u8_client_shutdown u8_shutdown_client

#if U8_THREADS_ENABLED
/** Declares that client is done with its request and can receive others
     @param cl a pointer to a U8_CLIENT struct
     @returns void
**/
U8_EXPORT int u8_client_done(u8_client cl);
#else
#define u8_client_done(cl)
#endif

/* Server threads */

/** struct U8_SERVER_THREAD represents a server thread in the u8srv
   library.
**/
typedef struct U8_SERVER_THREAD {
  pthread_t u8st_thread;
  struct U8_SERVER *u8st_server;
  unsigned long us8st_flags;
  long u8st_threadid;
  int u8st_slotno;
  int u8st_client;
  void *u8st_data;}
  U8_SERVER_THREAD;
typedef struct U8_SERVER_THREAD *u8_server_thread;

/* Bits of the server flags field */

#define U8_SERVER_CLOSED 1
#define U8_SERVER_ASYNC 2
#define U8_SERVER_LOG_LISTEN 4
#define U8_SERVER_LOG_CONNECT 8
#define U8_SERVER_LOG_TRANSACT 16
#define U8_SERVER_LOG_TRANSFER 32
#define U8_SERVER_LOG_QUEUE 64
#define U8_SERVER_FLAG_MAX 64

/* Argument names to u8_init_server */

#define U8_SERVER_END_ARGS (-1)
#define U8_SERVER_END_INIT (-1)
#define U8_SERVER_FLAGS (1)
#define U8_SERVER_NTHREADS (2)
#define U8_SERVER_INIT_CLIENTS (3)
#define U8_SERVER_MAX_QUEUE (4)
#define U8_SERVER_MAX_CLIENTS (5)
#define U8_SERVER_BACKLOG (6)
#define U8_SERVER_LOGLEVEL (7)
#define U8_SERVER_TIMEOUT (8)

/* Default values */

#ifndef MAX_BACKLOG
#define MAX_BACKLOG 16
#endif

#ifndef DEFAULT_NTHREADS
#define DEFAULT_NTHREADS 4
#endif

#ifndef DEFAULT_INIT_CLIENTS
#define DEFAULT_INIT_CLIENTS 32
#endif

#ifndef DEFAULT_MAX_CLIENTS
#define DEFAULT_MAX_CLIENTS 0
#endif

#ifndef DEFAULT_MAX_QUEUE
#define DEFAULT_MAX_QUEUE 128
#endif

/* Argument names to u8_init_server */

#define U8_SERVER_END_ARGS (-1)
#define U8_SERVER_END_INIT (-1)
#define U8_SERVER_FLAGS (1)
#define U8_SERVER_NTHREADS (2)
#define U8_SERVER_INIT_CLIENTS (3)
#define U8_SERVER_MAX_QUEUE (4)
#define U8_SERVER_MAX_CLIENTS (5)
#define U8_SERVER_BACKLOG (6)
#define U8_SERVER_LOGLEVEL (7)

/* Default values */

#ifndef MAX_BACKLOG
#define MAX_BACKLOG 16
#endif

#ifndef DEFAULT_NTHREADS
#define DEFAULT_NTHREADS 4
#endif

#ifndef DEFAULT_INIT_CLIENTS
#define DEFAULT_INIT_CLIENTS 32
#endif

#ifndef DEFAULT_MAX_CLIENTS
#define DEFAULT_MAX_CLIENTS 0
#endif

#ifndef DEFAULT_MAX_QUEUE
#define DEFAULT_MAX_QUEUE 128
#endif

/** struct U8_SERVER_INFO
    describes information about a particular server socket used
     to listen for new connections.
**/
typedef struct U8_SERVER_INFO {
  u8_socket socket; u8_string idstring; int poll_index;
  struct sockaddr *addr;} U8_SERVER_INFO;
typedef struct U8_SERVER_INFO *u8_server_info;

/* The server struct itself */

/** struct U8_SERVER
     This structure represents a server's state and connections.
**/
typedef struct U8_SERVER {
  u8_string serverid;

  /* Server-wide flags, whether we're shutting down, and
     how many connections to start with/grow by. */
  int flags, shutdown, init_clients, max_clients;

  /* The server addreses we're listening to for new connections */
  struct U8_SERVER_INFO *server_info; int n_servers;

  /* The connections (clients) we are currently serving */
  struct U8_CLIENT **clients; int n_clients, clients_len;
  /* free_slot is the first slot where a new client can be stored */
  int free_slot, max_slot;

  /* All sockets: n_sockets=n_servers+n_clients
     To simplify things, sockets_len is always the same as clients_len,
     and we just have NULL entries for server sockets. */
  struct pollfd *sockets;
  long poll_timeout; /* Timeout value to use when selecting */

  int n_busy; /* How many clients are currently active (mid transaction) */
  long n_accepted; /* How many connections have been accepted to date */
  long n_trans; /* How many transactions have been completed to date */
  long n_errs; /* How many transactions yielded errors */

  struct U8_CLIENT_STATS aggrestats;

  /* Handling functions */
  u8_client (*acceptfn)(struct U8_SERVER *,u8_socket sock,
                        struct sockaddr *,size_t);
  int (*servefn)(u8_client);
  int (*donefn)(u8_client);
  int (*closefn)(u8_client);

  /* These are functions called with each iteration of the
     server and client loops */
  int (*xserverfn)(struct U8_SERVER *);
  int (*xclientfn)(struct U8_CLIENT *);
  /* Miscellaneous data */
  void *serverdata;

  /* We don't handle non-threaded right now */
#if U8_THREADS_ENABLED
  u8_mutex lock;
  u8_condvar empty, full;
  /* n_tasks is the number of waiting requests in ->queue.
     max_tasks is the space available in ->queue
     n_threads is the number of threads in the pool */
  int n_queued, max_queued, n_threads, max_backlog;
  struct U8_SERVER_THREAD *thread_pool;
  u8_client *queue;
  int queue_len, queue_head, queue_tail;
#endif
} U8_SERVER;

U8_EXPORT
/** Initializes a server
     @param server a pointer to a U8_SERVER struct
     @param acceptfn the function for accepting connections
     @param servefn the function called to process data for a client
     @param donefn the function called when data processing is done
     @param closefn the function called when a client is closed
     @returns a pointer to the server object
 This initializes a server object, allocating one if server is NULL.  To
  actually start listening for requests, u8_add_server must be called
  at least once to add a listening address.
**/
struct U8_SERVER *u8_init_server
   (struct U8_SERVER *server,
    u8_client (*acceptfn)(u8_server,u8_socket,struct sockaddr *,size_t),
    int (*servefn)(u8_client),
    int (*donefn)(u8_client),
    int (*closefn)(u8_client),
    ...);

U8_EXPORT
/** Initializes a server
     @param server a pointer to a U8_SERVER struct
     @param maxback the maximum backlog on listening sockets
     @param max_queue the maximum number of tasks in the queue
     @param n_threads the  number of threads servicing the queue
     @param acceptfn the function for accepting connections
     @param servefn the function called to process data for a client
     @param closefn the function called when a client is closed
     @returns 1 on success, zero otherwise
 This initializes a server object, allocating one if server is NULL.  To
  actually start listening for requests, u8_add_server must be called
  at least once to add a listening address.
**/
int u8_server_init(struct U8_SERVER *server,
                   /* max_clients is currently ignored */
                   int maxback,int max_queue,int n_threads,
                   u8_client (*acceptfn)(u8_server,u8_socket,
                                         struct sockaddr *,size_t),
                   int (*servefn)(u8_client),
                   int (*closefn)(u8_client));

U8_EXPORT
/** Initializes a server
     @param server a pointer to a U8_SERVER struct
     @param acceptfn the function for accepting connections
     @param servefn the function called to process data for a client
     @param donefn the function called when data processing is done
     @param closefn the function called when a client is closed
     @param ... alternating property/value pairs
     @returns a pointer to the server object
 This initializes a server object, allocating one if server is NULL.  To
  actually start listening for requests, u8_add_server must be called
  at least once to add a listening address.
**/
struct U8_SERVER *u8_init_server
   (struct U8_SERVER *server,
    u8_client (*acceptfn)(u8_server,u8_socket,struct sockaddr *,size_t),
    int (*servefn)(u8_client),
    int (*donefn)(u8_client),
    int (*closefn)(u8_client),
    ...);

U8_EXPORT
/** Configures a server to listen on a particular address (port/host or file)
     @param server a pointer to a U8_SERVER struct
     @param hostname (a utf-8 string) the hostname to listen for
     @param port (an int) the port to listen on
     @returns void
**/
int u8_add_server(struct U8_SERVER *server,u8_string hostname,int port);

U8_EXPORT
/** Starts a loop listening for requests to a server
     @param server a pointer to a U8_SERVER struct
     @returns void
**/
void u8_server_loop(struct U8_SERVER *server);

U8_EXPORT
/** Pushes a client/task onto the queue for a server
    @param server a pointer to a U8_SERVER struct
    @param client a pointer to a structure head-compatible with a U8_CLIENT struct
    @param cxt the caller context, used for debugging
    @returns 1 if it did anything, zero otherwise
*/
int u8_push_task(struct U8_SERVER *server,u8_client client,u8_context cxt);

U8_EXPORT
/** Shuts down a server, stopping listening and closing all
     connections when completed.
     @param server a pointer to a U8_SERVER struct
     @param grace how long (in us) to wait for active clients to finish
     @returns 1 if the server was newly and successfully closed
**/
int u8_shutdown_server(struct U8_SERVER *server,int grace);
#define u8_server_shutdown u8_shutdown_server

/* Server Status */

/** struct U8_SERVER_STATS
    this structure aggregate processing information for a server,
    especially for generating performance statistics. **/
typedef struct U8_SERVER_STATS {
  int n_reqs, n_errs, n_complete, n_busy, n_active, n_reading, n_writing;
  /* Tracking total transaction time */
  long long tsum, tsum2, tmax; int tcount;
  /* Tracking total spent queued up */
  long long qsum, qsum2, qmax; int qcount;
  /* Tracking total spent reading */
  long long rsum, rsum2, rmax; int rcount;
  /* Tracking total spent writing */
  long long wsum, wsum2, wmax; int wcount;
  /* Tracking total spent in handlers or callbacks */
  long long xsum, xsum2, xmax; int xcount;}
  U8_SERVER_STATS;
typedef struct U8_SERVER_STATS *u8_server_stats;

U8_EXPORT
/** Returns activity statistics for a server
     @param server a pointer to a U8_SERVER struct
     @param stats a pointer to a U8_SERVER_STATS structure, or NULL
     @returns a pointer to a U8_SERVER_STATS structure (allocated if neccessary)
  This returns statistics over the life of the server
**/
u8_server_stats u8_server_statistics(u8_server server,struct U8_SERVER_STATS *stats);

U8_EXPORT
/** Returns activity statistics for a server
     @param server a pointer to a U8_SERVER struct
     @param stats a pointer to a U8_SERVER_STATS structure, or NULL
     @returns a pointer to a U8_SERVER_STATS structure (allocated if neccessary)
  This returns statistics over all of the currently open clients.
**/
u8_server_stats u8_server_livestats(u8_server server,struct U8_SERVER_STATS *stats);

U8_EXPORT
/** Returns activity statistics for a server
     @param server a pointer to a U8_SERVER struct
     @param stats a pointer to a U8_SERVER_STATS structure, or NULL
     @returns a pointer to a U8_SERVER_STATS structure (allocated if neccessary)
  This returns statistics over the current state of the current clients
**/
u8_server_stats u8_server_curstats(u8_server server,struct U8_SERVER_STATS *stats);

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

U8_EXPORT
/** Returns a machine readable (tab-separated) string describing each of the
     current clients
     @param out a pointer to a U8_OUTPUT struct
     @param server a pointer to a U8_SERVER struct
     @returns the buffer string of the output stream
**/
u8_string u8_list_clients(struct U8_OUTPUT *out,struct U8_SERVER *server);

#define U8_CLIENT_WRITE_OWNBUF 1

U8_EXPORT unsigned char *u8_client_write_x
  (u8_client cl,unsigned char *buf,size_t n,size_t off,int flags);
U8_EXPORT unsigned char *u8_client_write
  (u8_client cl,unsigned char *buf,size_t n,size_t off);

U8_EXPORT unsigned char *u8_client_read
  (u8_client cl,unsigned char *buf,size_t n,size_t off);

U8_EXPORT int u8_client_finished(u8_client cl);

#endif /* U8_U8SRVFNS_H */
