/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2009-2015 beingmeta, inc.
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
#include "libu8/u8crypto.h"
#include "libu8/u8stringfns.h"
#include "libu8/u8streamio.h"
#include "libu8/u8printf.h"
#include "libu8/u8bytebuf.h"

#include <string.h>

#if ((HAVE_EVP_CIPHER_CTX_INIT)&&(HAVE_OPENSSL_EVP_H)&& \
     (HAVE_OPENSSL_ERR_H)&&(HAVE_OPENSSL_EVP_H)&& \
     (!((HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)&&(HAVE_CCCRYPTORCREATE))))
#include <openssl/err.h>
#include <openssl/evp.h>
#elif ((HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)&&(HAVE_CCCRYPTORCREATE))
#include <CommonCrypto/CommonCryptor.h>
#else
#endif

#define CRYPTO_LOGLEVEL LOG_DEBUG /* LOG_WARN */

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

static int cryptofns_init=0;

u8_condition u8_BadCryptoKey=_("bad crypto key value");
u8_condition u8_BadCryptoIV=_("bad crypto initial value (iv)");
u8_condition u8_CipherInit_Failed=_("cipher init failed");
u8_condition u8_InternalCryptoError=_("internal libcrypto error");
u8_condition u8_UnknownCipher=_("Unknown cipher name");
u8_condition u8_UnknownCipherNID=_("Unknown cipher name");
u8_condition u8_NoCrypto=_("No cryptographic functions available");

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

#if ((HAVE_EVP_CIPHER_CTX_INIT)&&(HAVE_OPENSSL_EVP_H)&& \
     (HAVE_OPENSSL_ERR_H)&&(HAVE_OPENSSL_EVP_H)&& \
     (!((HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)&&(HAVE_CCCRYPTORCREATE))))

static u8_context OPENSSL_CRYPTIC="u8_cryptic/OpenSSL";

U8_EXPORT ssize_t u8_cryptic
(int do_encrypt,const char *cname,
 const unsigned char *key,int keylen,
 const unsigned char *iv,int ivlen,
 u8_block_reader reader,u8_block_writer writer,
 void *readstate,void *writestate,
 u8_context caller)
{
  EVP_CIPHER_CTX ctx;
  int inlen, outlen, retval=0;
  ssize_t totalout=0, totalin=0;
  unsigned char inbuf[1024], outbuf[1024+EVP_MAX_BLOCK_LENGTH];
  const EVP_CIPHER *cipher=((cname)?(EVP_get_cipherbyname(cname)):
			    (EVP_aes_128_cbc()));

  if (cipher) {
    int needkeylen=EVP_CIPHER_key_length(cipher);
    int needivlen=EVP_CIPHER_iv_length(cipher);
    int blocksize=EVP_CIPHER_block_size(cipher);
    if (blocksize>1024) blocksize=1024;
    u8_log(CRYPTO_LOGLEVEL,OPENSSL_CRYPTIC,
	   " %s cipher=%s, keylen=%d/%d, ivlen=%d/%d, blocksize=%d\n",
	   ((do_encrypt)?("encrypt"):("decrypt")),
	   cname,keylen,needkeylen,ivlen,needivlen,blocksize);

    if ((needivlen)&&(ivlen)&&(ivlen!=needivlen))
      return u8_reterr(u8_BadCryptoIV,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       u8_mkstring("%d!=%d(%s)",ivlen,needivlen,cname));

    memset(&cxt,0,sizeof(ctx));

    EVP_CIPHER_CTX_init(&ctx);

    retval=EVP_CipherInit(&ctx, cipher, NULL, NULL, do_encrypt);
    if (retval==0)
      return u8_reterr(u8_CipherInit_Failed,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       u8_strdup(cname));

    retval=EVP_CIPHER_CTX_set_key_length(&ctx,keylen);
    if (retval==0)
      return u8_reterr(u8_BadCryptoKey,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       u8_mkstring("%d!=%d(%s)",keylen,needkeylen,cname));

    if ((needivlen)&&(ivlen!=needivlen))
      return u8_reterr(u8_BadCryptoIV,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       u8_mkstring("%d!=%d(%s)",ivlen,needivlen,cname));

    retval=EVP_CipherInit(&ctx, cipher, key, iv, do_encrypt);
    if (retval==0)
      return u8_reterr(u8_CipherInit_Failed,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       u8_strdup(cname));

    while (1) {
      inlen = reader(inbuf,blocksize,readstate);
      if (inlen <= 0) {
	u8_log(CRYPTO_LOGLEVEL,OPENSSL_CRYPTIC,
	       "Finished %s(%s) with %ld in, %ld out",
	       ((do_encrypt)?("encrypt"):("decrypt")),
	       cname,totalin,totalout);
	break;}
      else totalin=totalin+inlen;
      if (!(EVP_CipherUpdate(&ctx,outbuf,&outlen,inbuf,inlen))) {
	char *details=u8_malloc(256);
	unsigned long err=ERR_get_error();
	ERR_error_string_n(err,details,256);
	EVP_CIPHER_CTX_cleanup(&ctx);
	return u8_reterr(u8_InternalCryptoError,
			 ((caller)?(caller):((u8_context)"u8_cryptic")),
			 details);}
      else {
	u8_log(CRYPTO_LOGLEVEL,OPENSSL_CRYPTIC,
	       "%s(%s) consumed %d/%ld bytes, emitted %d/%ld bytes"
	       " in=<%v>\n out=<%v>",
	       ((do_encrypt)?("encrypt"):("decrypt")),cname,
	       inlen,totalin,outlen,totalout+outlen,
	       inbuf,inlen,outbuf,outlen);
	writer(outbuf,outlen,writestate);
	totalout=totalout+outlen;}}
    if (!(EVP_CipherFinal(&ctx,outbuf,&outlen))) {
      char *details=u8_malloc(256);
      unsigned long err=ERR_get_error();
      ERR_error_string_n(err,details,256);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return u8_reterr(u8_InternalCryptoError,
		       ((caller)?(caller):(OPENSSL_CRYPTIC)),
		       details);}
    else {
      writer(outbuf,outlen,writestate);
      u8_log(CRYPTO_LOGLEVEL,OPENSSL_CRYPTIC,
	     "%s(%s) done after consuming %ld/%ld bytes, emitting %ld/%ld bytes"
	     "\n final out=<%v>",
	     ((do_encrypt)?("encrypt"):("decrypt")),cname,
	     inlen,totalin,outlen,totalout+outlen,
	     outbuf,outlen);
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
  if (cryptofns_init) return;

  cryptofns_init=1;

  OpenSSL_add_all_algorithms();

  u8_register_source_file(_FILEINFO);
}

#elif ((HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)&&(HAVE_CCCRYPTORCREATE))

static u8_context COMMONCRYPTO_CRYPTIC="u8_cryptic/CommonCrypto";

typedef struct U8_CCCIPHER {
  u8_string cc_name; CCAlgorithm cc_algorithm; CCOptions cc_opts;
  int cc_ivlen, cc_keymin, cc_keymax, cc_blocksize;}
  *u8_cipher;
static struct U8_CCCIPHER *get_cipher(u8_string s);

U8_EXPORT ssize_t u8_cryptic
(int do_encrypt,const char *cname,
 const unsigned char *key,int keylen,
 const unsigned char *iv,int ivlen,
 u8_block_reader reader,u8_block_writer writer,
 void *readstate,void *writestate,
 u8_context caller)
{
  CCCryptorRef ctx;
  CCOptions options=0;
  ssize_t inlen, outlen, totalin=0, totalout=0, retval=0;
  unsigned char inbuf[1024], outbuf[1024];
  struct U8_CCCIPHER *cipher=get_cipher(cname);
  if (cipher) {
    size_t blocksize=cipher->cc_blocksize;
    ssize_t needivlen=cipher->cc_ivlen;
    if (!((keylen<=cipher->cc_keymax)&&(keylen>=cipher->cc_keymin)))
      return u8_reterr(u8_BadCryptoKey,
		       ((caller)?(caller):((u8_context)"u8_cryptic")),
		       u8_mkstring("%d!=[%d,%d](%s)",keylen,
				   cipher->cc_keymin,cipher->cc_keymax,
				   cname));
    if ((needivlen)&&(ivlen!=needivlen))
      return u8_reterr(u8_BadCryptoIV,
		       ((caller)?(caller):(COMMONCRYPTO_CRYPTIC)),
		       u8_mkstring("%d!=%d(%s)",ivlen,needivlen,cname));

    if (needivlen==0) iv=NULL;

    CCCryptorStatus status=CCCryptorCreate
      (((do_encrypt)? (kCCEncrypt) : (kCCDecrypt)),
       cipher->cc_algorithm,cipher->cc_opts,key,keylen,iv,&ctx);

    u8_log(CRYPTO_LOGLEVEL,COMMONCRYPTO_CRYPTIC,
	   " %s cipher=%s, keylen=%d/[%d,%d], ivlen=%d, blocksize=%d\n",
	   ((do_encrypt)?("encrypt"):("decrypt")),
	   cname,keylen,cipher->cc_keymin,cipher->cc_keymax,
	   ivlen,blocksize);

    while (1) {
      inlen = reader(inbuf,blocksize,readstate);
      if (inlen <= 0) {
	u8_log(CRYPTO_LOGLEVEL,COMMONCRYPTO_CRYPTIC,
	       "Finished %s(%s) with %ld in, %ld out",
	       ((do_encrypt)?("encrypt"):("decrypt")),cname,
	       totalin,totalout);
	break;}
      if ((status=CCCryptorUpdate(ctx,inbuf,inlen,outbuf,1024,&outlen))
	  !=kCCSuccess) {
	CCCryptorRelease(ctx);
	return u8_reterr(u8_InternalCryptoError,
			 ((caller)?(caller):((u8_context)"u8_cryptic")),
			 NULL);}
      else {
	u8_log(CRYPTO_LOGLEVEL,COMMONCRYPTO_CRYPTIC,
	       "%s(%s) consumed %d/%ld bytes, emitted %d/%ld bytes"
	       " in=<%v>\n out=<%v>",
	       ((do_encrypt)?("encrypt"):("decrypt")),cname,
	       inlen,totalin,outlen,totalout+outlen,
	       inbuf,inlen,outbuf,outlen);
	writer(outbuf,outlen,writestate);
	totalout=totalout+outlen;}}

    if ((status=CCCryptorFinal(ctx,outbuf,1024,&outlen))!=kCCSuccess) {
      CCCryptorRelease(ctx);
      return u8_reterr(u8_InternalCryptoError,
		       ((caller)?(caller):((u8_context)"u8_cryptic")),
		       NULL);}
    else {
      writer(outbuf,outlen,writestate);
      u8_log(CRYPTO_LOGLEVEL,COMMONCRYPTO_CRYPTIC,
	     "%s(%s) done after consuming %ld/%ld bytes, emitting %ld/%ld bytes"
	     "\n final out=<%v>",
	     ((do_encrypt)?("encrypt"):("decrypt")),cname,
	     inlen,totalin,outlen,totalout+outlen,
	     outbuf,outlen);
      CCCryptorRelease(ctx);
      totalout=totalout+outlen;
      return totalout;}}
  else return u8_reterr("Unknown cipher",
			((caller)?(caller):((u8_context)"u8_cryptic")),
			u8_strdup(cname));
}

#define MAX_CIPHERS 64

static int n_ciphers=0;
static u8_cipher ciphers[64]=
  {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static u8_string default_cipher_name="BLOWFISH";
static struct U8_CCCIPHER default_cipher=
  {"AES128",kCCAlgorithmAES,kCCOptionPKCS7Padding,
   kCCKeySizeAES256,kCCKeySizeAES256,
   kCCBlockSizeAES128,kCCBlockSizeAES128};

static struct U8_CCCIPHER *get_cipher(u8_string name)
{
  if (name==NULL) return &default_cipher;
  else {
    int i=0, lim=n_ciphers;
    if (name) while (i<lim) {
	if (strcasecmp(name,ciphers[i]->cc_name)==0)
	  return ciphers[i];
	else i++;}
    return NULL;}
}

/* This isn't entirely threadsafe */
static void add_cc_cipher(u8_string name,CCAlgorithm alg,CCOptions opts,
			  ssize_t keymin,ssize_t keymax,
			  ssize_t ivlen,ssize_t blocksize)
{
  if (n_ciphers<MAX_CIPHERS) {
    u8_cipher fresh=u8_malloc(sizeof(struct U8_CCCIPHER));
    fresh->cc_name=name; fresh->cc_algorithm=alg; fresh->cc_opts=opts;
    fresh->cc_keymin=keymin; fresh->cc_keymax=keymax;
    fresh->cc_ivlen=ivlen; fresh->cc_blocksize=blocksize;
    ciphers[n_ciphers++]=fresh;}
  else u8_log(LOG_CRIT,_("TooManyCiphers"),"Can't declare cipher#%d %s",
	      n_ciphers,name);
}

U8_EXPORT void u8_init_cryptofns_c()
{
  if (cryptofns_init) return;

  cryptofns_init=1;

  add_cc_cipher("AES",kCCAlgorithmAES,kCCOptionPKCS7Padding,
		kCCKeySizeAES128,kCCKeySizeAES256,
		kCCBlockSizeAES128,kCCBlockSizeAES128);
  add_cc_cipher("AES128",kCCAlgorithmAES,kCCOptionPKCS7Padding,
		kCCKeySizeAES128,kCCKeySizeAES128,
		kCCBlockSizeAES128,kCCBlockSizeAES128);
  add_cc_cipher("AES256",kCCAlgorithmAES,kCCOptionPKCS7Padding,
		kCCKeySizeAES256,kCCKeySizeAES256,
		kCCBlockSizeAES128,kCCBlockSizeAES128);
  add_cc_cipher("3DES",kCCAlgorithm3DES,kCCOptionPKCS7Padding,
		kCCKeySize3DES,kCCKeySize3DES,
		kCCBlockSize3DES,kCCBlockSize3DES);
  add_cc_cipher("DES3",kCCAlgorithm3DES,kCCOptionPKCS7Padding,
		kCCKeySize3DES,kCCKeySize3DES,
		kCCBlockSize3DES,kCCBlockSize3DES);
  add_cc_cipher("DES",kCCAlgorithmDES,kCCOptionPKCS7Padding,
		kCCKeySizeDES,kCCKeySizeDES,
		kCCBlockSizeDES,kCCBlockSizeDES);
  add_cc_cipher("RC4",kCCAlgorithmRC4,0,kCCKeySizeMinRC4,kCCKeySizeMaxRC4,0,1);
  add_cc_cipher("ARC4",kCCAlgorithmRC4,0,kCCKeySizeMinRC4,kCCKeySizeMaxRC4,0,1);
  add_cc_cipher("CAST",kCCAlgorithmCAST,kCCOptionPKCS7Padding,
		kCCKeySizeMinCAST,kCCKeySizeMaxCAST,
		kCCBlockSizeCAST,kCCBlockSizeCAST);
  add_cc_cipher("BLOWFISH",kCCAlgorithmBlowfish,kCCOptionPKCS7Padding,
		kCCKeySizeMinBlowfish,kCCKeySizeMaxBlowfish,
		kCCBlockSizeBlowfish,kCCBlockSizeBlowfish);
  add_cc_cipher("BF",kCCAlgorithmBlowfish,kCCOptionPKCS7Padding,
		kCCKeySizeMinBlowfish,kCCKeySizeMaxBlowfish,
		kCCBlockSizeBlowfish,kCCBlockSizeBlowfish);

  add_cc_cipher("AES-ECB",kCCAlgorithmAES,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySizeAES128,kCCKeySizeAES256,
		kCCBlockSizeAES128,kCCBlockSizeAES128);
  add_cc_cipher("3DES-ECB",kCCAlgorithm3DES,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySize3DES,kCCKeySize3DES,kCCBlockSize3DES,kCCBlockSize3DES);
  add_cc_cipher("DES3-ECB",kCCAlgorithm3DES,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySize3DES,kCCKeySize3DES,kCCBlockSize3DES,kCCBlockSize3DES);
  add_cc_cipher("DES-ECB",kCCAlgorithmDES,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySizeDES,kCCKeySizeDES,kCCBlockSizeDES,kCCBlockSizeDES);
  add_cc_cipher("RC4-ECB",kCCAlgorithmRC4,0|kCCOptionECBMode,kCCKeySizeMinRC4,kCCKeySizeMaxRC4,1,1);
  add_cc_cipher("ARC4-ECB",kCCAlgorithmRC4,0|kCCOptionECBMode,kCCKeySizeMinRC4,kCCKeySizeMaxRC4,1,1);
  add_cc_cipher("CAST-ECB",kCCAlgorithmCAST,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySizeMinCAST,kCCKeySizeMaxCAST,
		kCCBlockSizeCAST,kCCBlockSizeCAST);
  add_cc_cipher("BLOWFISH-ECB",kCCAlgorithmBlowfish,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySizeMinBlowfish,kCCKeySizeMaxBlowfish,
		kCCBlockSizeBlowfish,kCCBlockSizeBlowfish);
  add_cc_cipher("BF-ECB",kCCAlgorithmBlowfish,kCCOptionPKCS7Padding|kCCOptionECBMode,
		kCCKeySizeMinBlowfish,kCCKeySizeMaxBlowfish,
		kCCBlockSizeBlowfish,kCCBlockSizeBlowfish);
  
  u8_register_source_file(_FILEINFO);
}

#else /* No crypto */

U8_EXPORT ssize_t u8_cryptic
(int do_encrypt,const char *cname,
 const unsigned char *key,int keylen,
 const unsigned char *iv,int ivlen,
 u8_block_reader reader,u8_block_writer writer,
 void *readstate,void *writestate,
 u8_context caller)
{
  u8_seterr(u8_NoCrypto,"u8_cryptic",NULL);
  return -1;
}

U8_EXPORT void u8_init_cryptofns_c()
{
  if (cryptofns_init) return;
  cryptofns_init=1;
  u8_register_source_file(_FILEINFO);
}

#endif

U8_EXPORT unsigned char *u8_encrypt
(const unsigned char *input,size_t len,
 const char *cipher,
 const unsigned char *key,size_t keylen,
 const unsigned char *iv,size_t ivlen,
 size_t *result_len)
{
  ssize_t bytecount;
  struct U8_BYTEBUF in, out;
  unsigned char *outbuf=u8_malloc(2*len);
  in.u8_direction=u8_input_buffer;
  in.u8_buf=in.u8_ptr=(u8_byte *)input; in.u8_lim=(u8_byte *)(input+len);
  in.u8_direction=u8_output_buffer; out.u8_growbuf=len;
  out.u8_ptr=out.u8_buf=outbuf; out.u8_lim=outbuf+2*len;
  bytecount=u8_cryptic
    (1,cipher,key,keylen,iv,ivlen,
     (u8_block_reader)u8_bbreader,(u8_block_writer)u8_bbwriter,
     &in,&out,"u8_encrypt");
  if (bytecount<0) return NULL;
  else *result_len=bytecount;
  return out.u8_buf;
}

U8_EXPORT unsigned char *u8_decrypt
(const unsigned char *input,size_t len,
 const char *cipher,
 const unsigned char *key,size_t keylen,
 const unsigned char *iv,size_t ivlen,
 size_t *result_len)
{
  ssize_t bytecount;
  struct U8_BYTEBUF in, out;
  unsigned char *outbuf=u8_malloc(2*len);
  in.u8_direction=u8_input_buffer;
  in.u8_buf=in.u8_ptr=(u8_byte *)input; in.u8_lim=(u8_byte *)(input+len);
  in.u8_direction=u8_output_buffer; out.u8_growbuf=len;
  out.u8_ptr=out.u8_buf=outbuf; out.u8_lim=outbuf+2*len;
  bytecount=u8_cryptic
    (0,cipher,key,keylen,iv,ivlen,
     (u8_block_reader)u8_bbreader,(u8_block_writer)u8_bbwriter,
     &in,&out,"u8_decrypt");
  if (bytecount<0) return NULL;
  else *result_len=bytecount;
  return out.u8_buf;
}
U8_EXPORT void u8_init_cryptofns()
{
  u8_init_cryptofns_c();
}


/* Testing notes:
   openssl enc -e -a -aes128 -in sample -nosalt -out sample.aes128 -md md5 -p -k testing -iv 0
     key=AE2B1FCA515949E5D54FB22B8ED95575
     iv =00000000000000000000000000000000
   openssl enc -e -a -rc4 -in sample -nosalt -out sample.rc4 -md md5 -p -k testing
     key=AE2B1FCA515949E5D54FB22B8ED95575
*/
