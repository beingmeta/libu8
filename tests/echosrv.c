#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "libu8/libu8.h"
#include "libu8/libu8io.h"
#include "libu8/xfiles.h"
#include "libu8/u8ctype.h"
#include "libu8/u8netfns.h"
#include "libu8/u8srvfns.h"

#define NTASKS 500
#define NTHREADS 8
#define MAXBACKLOG 20

struct ECHO_SERVER_DATA {
  char *prefix;};

struct ECHO_CONN {
  U8_CLIENT_FIELDS;
  char lastin[256];};


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
  u8_set_nodelay(sock,1);
  u8_message("Accepted connection from %s",consed->idstring);
  return (u8_client) consed;
}

static int echosrv_handle(u8_client ucl)
{
  char inbuf[256];
  struct ECHO_CONN *ec=(struct ECHO_CONN *)ucl;
  struct U8_SERVER *es=ec->server;
  struct ECHO_SERVER_DATA *esd=(struct ECHO_SERVER_DATA *)es->serverdata;
  int n_bytes=recv(ucl->socket,inbuf,256,0);
  if (n_bytes<0) return -1;
  else if (n_bytes==0) {
    u8_client_close(ucl);
    return -1;}
  else if ((ec->lastin) && ((strcmp(inbuf,ec->lastin))==0)) {
    int retval=send(ucl->socket,"ditto\n",6,0);
    if (retval<0) return -1;}
  else {
    int retval=send(ucl->socket,esd->prefix,strlen(esd->prefix),0);
    if (retval<0) return -1;
    retval=send(ucl->socket,inbuf,n_bytes,0);
    if (retval<0) return -1;
    strcpy(ec->lastin,inbuf);}
  return 0;
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
  u8_server_init(&eserv,MAXBACKLOG,NTASKS,NTHREADS,
		 echosrv_accept,echosrv_handle,echosrv_close);
  esd->prefix=u8_fromlibc(argv[1]);
  eserv.serverdata=esd;
  i=2; while (i<argc) {
    u8_add_server(&eserv,argv[i],0); i++;}

  u8_server_loop(&eserv); normal_exit=1;

  return 0;

}
