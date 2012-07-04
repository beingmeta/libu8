/* -*- Mode: C; -*- */

/* Copyright (C) 2009-2012 beingmeta, inc.
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
#include "libu8/u8crypto.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8bytebuf.h"

#include <string.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

u8_condition u8_BadCryptoKey=_("bad crypto key value");
u8_condition u8_BadCryptoInit=_("bad crypto init value");
u8_condition u8_InternalCryptoError=_("internal libcrypto error");
u8_condition u8_UnknownCipher=_("Unknown cipher name");
u8_condition u8_UnknownCipherNID=_("Unknown cipher name");

typedef int (*u8_block_reader)(unsigned char*,int,void *);
typedef int (*u8_block_writer)(unsigned char*,int,void *);

U8_EXPORT unsigned char *u8_random_vector(int len)
{
  unsigned char *vec=u8_malloc(len);
  if (vec) {
    int i=0; while (i<len) {
      int randomval=u8_random(256);
      vec[i++]=randomval;}
    return vec;}
  else return vec;
}

#if HAVE_EVP_CIPHER_CTX_INIT

U8_EXPORT ssize_t u8_cryptic
  (int do_encrypt,char *cname,
   unsigned char *key,int keylen,
   unsigned char *iv,int ivlen,
   u8_block_reader reader,u8_block_writer writer,
   void *readstate,void *writestate,
   u8_context caller)
{
  EVP_CIPHER_CTX ctx;
  int inlen, outlen, totalout=0;
  unsigned char inbuf[1024], outbuf[1024+EVP_MAX_BLOCK_LENGTH];
  const EVP_CIPHER *cipher=((cname)?(EVP_get_cipherbyname(cname)):
			    (EVP_bf_cbc()));
  if (cipher) {
    int needkeylen=EVP_CIPHER_key_length(cipher);
    int needivlen=EVP_CIPHER_iv_length(cipher);
    int blocksize=EVP_CIPHER_block_size(cipher);
    if (blocksize>1024) blocksize=1024;
    if ((needkeylen)&&(keylen!=needkeylen))
      return u8_reterr(u8_BadCryptoKey,
		       ((caller)?(caller):((u8_context)"u8_cryptic")),
		       u8_mkstring("%d!=%d(%s)",keylen,needkeylen,cname));
    if ((needivlen)&&(ivlen)&&(ivlen!=needivlen))
      return u8_reterr(u8_BadCryptoInit,
		       ((caller)?(caller):((u8_context)"u8_cryptic")),
		       u8_mkstring("%d!=%d(%s)",ivlen,needivlen,cname));

    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit(&ctx, cipher, NULL, NULL, do_encrypt);
    EVP_CIPHER_CTX_set_key_length(&ctx,keylen);
    EVP_CipherInit(&ctx, cipher, key, iv, do_encrypt);
    while (1) {
      inlen = reader(inbuf,blocksize,readstate);
      if(inlen <= 0) break;
      if(!(EVP_CipherUpdate(&ctx,outbuf,&outlen,inbuf,inlen))) {
	char *details=u8_malloc(256);
	unsigned long err=ERR_get_error();
	ERR_error_string_n(err,details,256);
	EVP_CIPHER_CTX_cleanup(&ctx);
	return u8_reterr(u8_InternalCryptoError,
			 ((caller)?(caller):((u8_context)"u8_cryptic")),
			 details);}
      else writer(outbuf,outlen,writestate);
      totalout=totalout+outlen;}
    if(!(EVP_CipherFinal(&ctx,outbuf,&outlen))) {
      char *details=u8_malloc(256);
      unsigned long err=ERR_get_error();
      ERR_error_string_n(err,details,256);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return u8_reterr(u8_InternalCryptoError,
		       ((caller)?(caller):((u8_context)"u8_cryptic")),
		       details);}
    else {
      writer(outbuf,outlen,writestate);
      EVP_CIPHER_CTX_cleanup(&ctx);
      totalout=totalout+outlen;
      return totalout;}}
  else {
    char *details=u8_malloc(256);
    unsigned long err=ERR_get_error();
    ERR_error_string_n(err,details,256);
    return u8_reterr("Unknown cipher",
		     ((caller)?(caller):((u8_context)"u8_cryptic")),
		     details);}
}

U8_EXPORT void u8_init_cryptofns_c()
{
  OpenSSL_add_all_algorithms();

  u8_register_source_file(_FILEINFO);
}

#endif


#if U8_HAVE_CRYPTO

U8_EXPORT unsigned char *u8_encrypt
  (unsigned char *input,size_t len,
   char *cipher,unsigned char *key,size_t keylen,
   size_t *result_len)
{
  ssize_t bytecount;
  struct U8_BYTEBUF in, out;
  unsigned char *outbuf=u8_malloc(2*len);
  in.u8_direction=u8_input_buffer;
  in.u8_buf=in.u8_ptr=input; in.u8_lim=input+len;
  in.u8_direction=u8_output_buffer; out.u8_growbuf=len;
  out.u8_ptr=out.u8_buf=outbuf; out.u8_lim=outbuf+2*len;
  bytecount=u8_cryptic
    (1,cipher,key,keylen,NULL,0,
     (u8_block_reader)u8_bbreader,(u8_block_writer)u8_bbwriter,
     &in,&out,"u8_encrypt");
  if (bytecount<0) return NULL;
  else *result_len=bytecount;
  return out.u8_buf;
}

U8_EXPORT unsigned char *u8_decrypt
  (unsigned char *input,size_t len,
   char *cipher,unsigned char *key,size_t keylen,
   size_t *result_len)
{
  ssize_t bytecount;
  struct U8_BYTEBUF in, out;
  unsigned char *outbuf=u8_malloc(2*len);
  in.u8_direction=u8_input_buffer;
  in.u8_buf=in.u8_ptr=input; in.u8_lim=input+len;
  in.u8_direction=u8_output_buffer; out.u8_growbuf=len;
  out.u8_ptr=out.u8_buf=outbuf; out.u8_lim=outbuf+2*len;
  bytecount=u8_cryptic
    (0,cipher,key,keylen,NULL,0,
     (u8_block_reader)u8_bbreader,(u8_block_writer)u8_bbwriter,
     &in,&out,"u8_encrypt");
  if (bytecount<0) return NULL;
  else *result_len=bytecount;
  return out.u8_buf;
}


#endif
