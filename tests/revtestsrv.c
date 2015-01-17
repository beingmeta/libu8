/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2015 beingmeta, inc.
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
#include "libu8/libu8io.h"
#include "libu8/xfiles.h"
#include "libu8/u8ctype.h"
#include "libu8/u8netfns.h"
#include "libu8/u8srvfns.h"

#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#define NTASKS 500
#define NSOCKS 500
#define NTHREADS 8
#define MAXBACKLOG 20

struct ECHO_SERVER_DATA {
  char *prefix;};

struct ECHO_CONN {
  U8_CLIENT_FIELDS;
  ssize_t n_bytes;};


static u8_client echosrv_accept
      (u8_server srv,u8_socket sock,
       struct sockaddr *addr,size_t addr_len)
{
  /* We'll accept anything without checking addr (which is who is
     connecting to us). */
  struct ECHO_CONN *consed=
    (struct ECHO_CONN *)
    u8_client_init(NULL,sizeof(struct ECHO_CONN),
		   (struct sockaddr *)addr,addr_len,
		   sock,srv);
  u8_set_nodelay(sock,1); ec->n_bytes=-1;
  u8_message("Accepted connection from %s",consed->idstring);
  return (u8_client) consed;
}

static int echosrv_handle(u8_client ucl)
{
  char buf[16];
  struct ECHO_CONN *ec=(struct ECHO_CONN *)ucl;
  struct U8_SERVER *es=ec->server;
  struct ECHO_SERVER_DATA *esd=(struct ECHO_SERVER_DATA *)es->serverdata;
  if (ec->n_bytes<0) {
    int bytes_read=read(ucl->socket,buf,4); size_t n_bytes=0;
    if (bytes_read<0) return -1;
    else if (bytes_read!=4) {
      u8_client_close(ucl);
      return -1;}
    else {
      n_bytes=n_bytes+(((unsigned long)buf[0])<<24);
      n_bytes=n_bytes+(((unsigned long)buf[1])<<16);
      n_bytes=n_bytes+(((unsigned long)buf[2])<<8);
      n_bytes=n_bytes+((unsigned long)buf[3]);}
    u8_log(LOG_INFO,"Reading %ld bytes from @x%lx#%d/%d[%d](%s)",
	   n_bytes,((unsigned long)ucl),ucl->clientid,ucl->socket,
	   ucl->stats.n_trans,ucl->idstring);
    ucl->buf=u8_malloc(n_bytes); ucl->reading=u8_microtime();
    ucl->off=0; ucl->len=n_bytes;
    return 1;}
  else if (ucl->writing) {
    ucl->writing=0; u8_free(ucl->buf);
    ucl->n_bytes=-1;
    return 0;}
  else {
    int i=0, n=ec->n_bytes, max=n-1;
    unsigned char *buf=ucl->buf;
    unsigned char *reversed=u8_malloc(n+4);
    while (i<n) {reversed[i+4]=buf[(n-i)+3]; i++;}
    reversed[0]=((n>>24)&0xFF);
    reversed[1]=((n>>16)&0xFF);
    reversed[2]=((n>>8)&0xFF);
    reversed[3]=((n)&0xFF);
    u8_free(ucl->buf);
    ucl->buf=reversed; ucl->off=0; ucl->n_bytes=n+4;
    ucl->reading=0; ucl->writing=u8_microtime();
    return 1;}
}

static int echosrv_close(u8_client ucl)
{
  struct ECHO_CONN *ec=(struct ECHO_CONN *)ucl;
  /* Close the socket */
  close(ec->socket);
  /* Debug message */
  u8_message("Closed connection to %s",ec->idstring);
  return 1;
}

static int normal_exit=0;

int main(int argc,char *argv[])
{
  int i=0;
  struct U8_SERVER eserv;
  struct ECHO_SERVER_DATA *esd=u8_malloc(sizeof(struct ECHO_SERVER_DATA));
  u8_server_init(&eserv,MAXBACKLOG,NTASKS,NTHREADS,NSOCKS,
		 echosrv_accept,echosrv_handle,echosrv_close);
  esd->prefix=u8_fromlibc(argv[1]);
  eserv.serverdata=esd;
  i=2; while (i<argc) {
    u8_add_server(&eserv,argv[i],0); i++;}

  u8_server_loop(&eserv); normal_exit=1;

  return 0;

}
