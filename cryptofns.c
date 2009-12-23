/* -*- Mode: C; -*- */

/* Copyright (C) 2009 beingmeta, inc.
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
#include "libu8/u8digestfns.h"
#include <string.h>
#include <openssl/evp.h>

static char versionid[] MAYBE_UNUSED=
  "$Id: digestfns.c 11 2009-05-08 20:46:29Z haase $";

U8_EXPORT u8_BadCryptoKey=_("bad crypto key value");
U8_EXPORT u8_InternalCryptoError=_("internal libcrypto error");
U8_EXPORT u8_UnknownCipher=_("Unknown cipher name");
U8_EXPORT u8_UnknownCipherNID=_("Unknown cipher name");

typedef int (*u8_read_block)(unsigned char*,int,void *);
typedef int (*u8_write_block)(unsigned char*,int,void *);

static int u8_cryptic(int do_encrypt,char *cname,
		      unsigned char *key,int keylen,
		      crypt_read_block *reader,crypt_write_block *writer,
		      void *readstate,void *writestate,
		      u8_context caller)
{
  EVP_CIPHER_CTX ctx;
  int inlen, outlen, totalout=0;
  unsigned char inbuf[1024], outbuf[1024+EVP_MAX_BLOCK_LENGTH];
  EVP_CIPHER *cipher=EVP_get_cipherbyname(cname);
  if (cipher) {
    int needkeylen=EVP_CIPHER_key_length(cipher);
    int ivlen=EVP_CIPHER_iv_length(cipher);
    unsigned char *initval;
    if (keylen!=needkeylen)
      return u8_reterr(u8_BadCryptoKey,caller||"u8_cryptic",
		       u8_mkstring("%d!=%d(%s)",keylen,needkeylen,cname));
    else initval=((ivlen)&&(u8_random_vector(ivlen)));
    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, cipher, NULL, NULL, NULL, do_encrypt);
    EVP_CIPHER_CTX_set_key_length(&ctx, 10);
    /* We finished modifying parameters so now we can set key and IV */
    EVP_CipherInit_ex(&ctx, NULL, NULL, key, iv, do_encrypt);
    while (1) {
      inlen = reader(inbuf,1024,readstate);
      if(inlen <= 0) break;
      if(!(EVP_CipherUpdate(&ctx,outbuf,&outlen,inbuf,inlen))) {
	EVP_CIPHER_CTX_cleanup(&ctx);
	return u8_reterr(u8_CryptoError,caller||"u8_cryptic",u8_strdup(cname));}
      else writer(outbuf,outlen,writestate);
      totalout=totalout+outlen;}
    if(!(EVP_CipherFinal_ex(&ctx,outbuf,&outlen))) {
      EVP_CIPHER_CTX_cleanup(&ctx);
      return u8_reterr(u8_CryptoError,caller||"u8_cryptic",u8_strdup(cname));}
    else {
      writer(outbuf,outlen,writestate);
      EVP_CIPHER_CTX_cleanup(&ctx);
      totalout=totalout+outlen;
      return totalout;}}
  else return u8_reterr("Unknown cipher",caller||u8_cryptic,u8_strdup(cname));
}
