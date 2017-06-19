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

#ifndef LIBU8_U8NETFNS_H
#define LIBU8_U8NETFNS_H 1
#define LIBU8_U8NETFNS_H_VERSION __FILE__

/** \file u8netfns.h
    These functions provide for network access, especially
     network database functions and connection creation.
 **/

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#if HAVE_SYS_UN_H
# include <sys/un.h>
#endif
#if HAVE_NETDB_H
# include <netdb.h>
#endif
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if WIN32
#include <winsock.h>
#include <ws2tcpip.h>
#endif

#define U8_CONNPOOL_REGISTERED 1

/** struct U8_CONNPOOL
   maintains a pool of sockets connected to a particular remote port/host.
**/
typedef struct U8_CONNPOOL {
  u8_string u8cp_id; /* The string passed to u8_connect to create connections for this block */
  unsigned int u8cp_bits;
  u8_mutex u8cp_lock; /* The lock used to coordinate access to the connection block */
  u8_condvar u8cp_drained; /* This is signalled whenever the "drained" status of the
                         connection block changes. */
  /* How long to wait for an initial connection. Not currently used */
  struct timespec u8cp_timeout;
  int u8cp_n_open;  /* how many open sockets in the block */
  int u8cp_n_inuse; /* how many sockets currently being used */
  int u8cp_n_waiting; /* requestors waiting for connections */
  int u8cp_reserve; /* how many open sockets to keep as a reserve */
  int u8cp_cap;     /* the maximum number of open sockets allowed */
  int u8cp_reconnect_wait1; /* how long to wait before starting to try to reconnect */
  int u8cp_reconnect_wait; /* how long to wait between reconnect attempts */
  int u8cp_reconnect_tries; /* how many reconnect attempts to make */
  u8_socket *u8cp_inuse; /* array of all sockets in use */
  u8_socket *u8cp_free;  /* array of all sockets available to use */
  struct U8_CONNPOOL *u8cp_next;
  } *u8_connpool;

U8_EXPORT u8_condition u8_NetworkError;
U8_EXPORT u8_condition u8_BadPortSpec;

U8_EXPORT int u8_cpdebug;
U8_EXPORT int u8_warn_waitlevel;
U8_EXPORT int u8_reconnect_wait1;
U8_EXPORT int u8_reconnect_wait;
U8_EXPORT int u8_reconnect_tries;

/** Parses a variety of host/port syntaxes
    @param spec the address specification (a utf-8 string)
    @param portp a pointer to the port number (an int for the family, e.g. )
    @param result a buffer to hold the host address
    @param buflen the length of the buffer
    @returns the hostname for TCP/IP addresses or the filename
       for filesystem domain addresses; puts a port number in *portp*,
       storing zero for filesystem domain addresses and -1 for an error
       (also returns NULL)
**/
U8_EXPORT u8_byte *u8_parse_addr
   (u8_string spec,int *portp,u8_byte *result,ssize_t buflen);

/** Returns the configured name of the current host.
    @returns a utf-8 string describing the current host
**/
U8_EXPORT u8_string u8_gethostname(void);

/** Returns a mallocd pointer to a host entry for a named host for
     a particular address family
    @param hostname (a utf-8 string)
    @param family (an int for the family, e.g. )
    @returns a pointer to a hostent structure
**/
U8_EXPORT struct hostent *u8_gethostbyname(u8_string hostname,int family);

/** Returns a mallocd pointer to a host entry for the host with
     a particular address
    @param addr (a pointer to a byte array describing an address)
    @param len (the number of bytes in the address)
    @param family (an int for the address family, e.g. AF_INET, AF_INET6)
    @returns a pointer to a hostent structure
**/
U8_EXPORT struct hostent *u8_gethostbyaddr(char *addr,int len,int family);

/** Returns the primary hostname for a particular hostname.
     This looks  up the hostname and then returns the first name in the entry.
    @param hostname (a network hostname)
    @returns a mallocd UTF-8 string describing the primary host name
**/
U8_EXPORT u8_string u8_host_primary(u8_string hostname);

/** Returns a vector of addresses assigned to a particular hostname
     The number of address is deposited in n_addrsp and the address family
      of the returned addresses is deposited in addr_familyp.
    @param hostname (a network hostname)
    @param n_addrsp (a pointer to an int)
    @param addr_familyp (a pointer to an int (address family))
    @returns a vector of pointers to byte arrays describing addresses
**/
U8_EXPORT char **u8_lookup_host
  (u8_string hostname,int *n_addrsp,unsigned int *addr_familyp);

/** Converts a connection spec (e.g. port\@host) into canonical form,
     converting the port into a number and the host into a primary
     hostname.
    @param spec (a utf-8 string of the form port\@host
    @returns a utf-8 canonical string
**/
U8_EXPORT u8_string u8_canonical_addr(u8_string spec);

/** Opens a socket to a specified port and host, storing the
     host's address in addrp.
    @param spec (a utf-8 string of the form port\@host)
    @param addrp a pointer to a pointer to a utf-8 string, in
      which is deposited identifying information for the host contacted
    @returns a u8_socket (int) which is a socket_id
**/
U8_EXPORT u8_socket u8_connect_x(u8_string spec,u8_string *addrp);

/** Opens a socket to a specified port and host
    @param spec (a utf-8 string of the form port\@host)
    @returns a u8_socket (int) which is a socket_id
**/
U8_EXPORT u8_socket u8_connect(u8_string spec);

/** Returns a unique identifer for the current session.
     This is based on a pid, a hostname, and the time at which
       the id was generated.
    @returns a string
**/
U8_EXPORT u8_string u8_sessionid(void);

/** Returns an integer port id from a port string.
     This looks port ids up and (if that fails) converts
      the string into an integer based on "touch-tone" encoding
    @param portspec (a utf-8 string)
    @returns an integer port number
**/
U8_EXPORT int u8_get_portno(u8_string portspec);

/** Sets whether a connection delays output to fill packets
     (the Nagle algorithm)
    @param conn (a u8_socket, an int socket id)
    @param flag (whether the connection should delay output)
    @returns 1 if successful
**/
U8_EXPORT int u8_set_nodelay(u8_socket conn,int flag);

/** Returns a human readable string representation of a sockaddr structure.
    @param sockaddr a pointer to a sockaddr struct.
    @returns a mallocd UTF-8 string
**/
U8_EXPORT u8_string u8_sockaddr_string(struct sockaddr *sockaddr);

/** Returns a connection block structure initialized with a
     particular connection id and a reserve count.  This mallocs
     the connection block structure if neccessary.
     @param cb a pointer to a connection block structure or NULL
     @param id a utf-8 string identifying the connection, passed to u8_connect
     @param reserve an int indicating the maximum number of open connections
       to keep in reserve
     @param cap how many connections to keep alive at any time
     @param init how many initial connections to create
     @returns a pointer to a connpool, either as passed in or mallocd.
 **/
U8_EXPORT u8_connpool
  u8_init_connpool(u8_connpool cb,u8_string id,int reserve,int cap,int init);

/** Returns a connection (an int socket id) from a given connection block.
    This opens a new connection if needed but will use existing connections
     which have been passed in with u8_return_connection.
     @param cb a pointer to a connection block structure
     @returns a u8_socket (integer socket id)
 **/
U8_EXPORT u8_socket u8_get_connection(u8_connpool cb);

/** Returns a connection to a connection block.
    This adds the connection argument back to the queue of connections
     so it can be reused by u8_get_connection.  If the number
     of unused connections is over the connection block's reserve,
     the connection is just close()d.  Consequently, if the reserve
     is zero, this always closes the connection.
    Zero is returned if the connection was closed (and discarded), -1 on an error,
       and otherwise the connection itself is returned.
     @param cb a pointer to a connection block structure
     @param c a u8_socket (integer socket id)
     @returns a u8_socket (integer socket id)
 **/
U8_EXPORT u8_socket u8_return_connection(u8_connpool cb,u8_socket c);

/** Discards a connection received from a connection block.
    This removes the connection argument from the queue of connections
     for a connection block.  This removes it from inuse queue but
     does not add it back to the free queue.  It is intended to be used
     for connections which have been closed or are in error states.
    If there are requests pending on the connection block, this will attempt
     to open a new connection and add it to the block.
    Zero is returned if the connection was closed and no new connection made,
     -1 is returned on error, and otherwise the new connection is returned.
     @param cb a pointer to a connection block structure
     @param c a u8_socket (integer socket id)
     @returns a u8_socket (integer socket id)
 **/
U8_EXPORT u8_socket u8_discard_connection(u8_connpool cb,u8_socket c);

/** Reopens a connection from a connection block, replacing it in the block.
    The new connection (socket) is returned or -1 if there is an error.
     @param cb a pointer to a connection block structure
     @param c a u8_socket (integer socket id)
 **/
U8_EXPORT u8_socket u8_reconnect(u8_connpool cb,u8_socket c);

/** Returns a connection block structure initialized with a
     particular connection id and a reserve count.  This registers
     the resulting structure so that subsequent calls with identical
     ids yield the same structure, which is created if neccessary.
     @param id a utf-8 string identifying the connection, passed to u8_connect
     @param reserve an int indicating the maximum number of open connections
       to keep in reserve
     @param cap an int indicating the maximum number of open connections
       for this pool
     @param init an int indicating the number of initial connections
       to open
     @returns a pointer to a connpool
     If the cap and reserve values are negative, the current or default
      values are used; otherwise, the cap and reserve for the pool
      may be increased but will not be decreased (so the actual cap
      and reserve is the max of the values specified in different
      calls to u8_open_connpool.
 **/
U8_EXPORT u8_connpool u8_open_connpool(u8_string id,int reserve,int cap,int init);


/** Closes a connection block.
    This closes the open sockets and frees the associated memory for
     a connection block.
     @param cb a pointer to a connection block structure
     @param dowarn an int controlling the output of warning messages
       on closing live sockets
     @returns a pointer to a connection block structure.
 **/
U8_EXPORT u8_connpool u8_close_connpool(u8_connpool cb,int dowarn);

/** Sets the session identifier
    This sets the global process session identifier
     @param newid a utf-8 string
     @returns its argument, which should not be freed
 **/
U8_EXPORT u8_string u8_identify_session(u8_string newid);

/* Data transmission functions */

U8_EXPORT int u8_getbytes(int msecs,int socket_id,char *data,int len,int flags);
U8_EXPORT int u8_sendbytes(int msecs,int socket,const char *buf,int size,int flags);
U8_EXPORT int u8_transact(int timeout,int socket,char *msg,char *expect);

/* SMTP */

U8_EXPORT char *u8_default_mailhost, *u8_default_maildomain;
U8_EXPORT char *u8_default_from, *u8_default_replyto;
U8_EXPORT int u8_smtp_timeout;

/** struct U8_MAILHEADER
    Represents a mail header for use with u8_smtp
**/
typedef struct U8_MAILHEADER {
  u8_string label;
  u8_string value;} U8_MAILHEADER;
typedef struct U8_MAILHEADER *u8_mailheader;

U8_EXPORT int u8_smtp(const char *mailhost,const char *maildomain,
                      const char *from,const char *dest,const char *ctype,
                      int n_headers, u8_mailheader *headers,
                      const unsigned char *message,int message_len);

#endif /* U8_U8NETFNS_H */
