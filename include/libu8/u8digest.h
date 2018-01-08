/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2018 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8digest.h
    Provides libu8 digest functions
    This wraps or implements various functions for cryptography
    and related functions (like digests)
 **/

#ifndef LIBU8_DIGESTFNS_H
#define LIBU8_DIGESTFNS_H 1
#define LIBU8_DIGESTFNS_H_VERSION __FILE__

/** Returns the MD5 hash (16 bytes) of a data buffer
    @param data a pointer to a data buffer
    @param len the number of bytes in the data buffer (or -1)
    @param result a result buffer (at least 20 bytes) or NULL
    @returns the MD5 hash of the provided data
  If @a len is negative, strlen() is called on the input data.
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_md5
  (const unsigned char *data,int len,unsigned char *result);

/** Returns the SHA-1 hash (20 bytes) of a data buffer
    @param data a pointer to a data buffer
    @param len the number of bytes in the data buffer (or -1)
    @param result a result buffer (at least 20 bytes) or NULL
    @returns the SHA1 hash of the provided data
  If @a len is negative, strlen() is called on the input data.
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_sha1
  (const unsigned char *data,int len,unsigned char *result);

/** Returns the SHA-256 hash (32 bytes) of a data buffer
    @param data a pointer to a data buffer
    @param len the number of bytes in the data buffer (or -1)
    @param result a result buffer (at least 20 bytes) or NULL
    @returns the SHA1 hash of the provided data
  If @a len is negative, strlen() is called on the input data.
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_sha256
  (const unsigned char *data,int len,unsigned char *result);

/** Returns the SHA-384 hash (48 bytes) of a data buffer
    @param data a pointer to a data buffer
    @param len the number of bytes in the data buffer (or -1)
    @param result a result buffer (at least 20 bytes) or NULL
    @returns the SHA1 hash of the provided data
  If @a len is negative, strlen() is called on the input data.
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_sha384
  (const unsigned char *data,int len,unsigned char *result);

/** Returns the SHA-512 hash (64 bytes) of a data buffer
    @param data a pointer to a data buffer
    @param len the number of bytes in the data buffer (or -1)
    @param result a result buffer (at least 20 bytes) or NULL
    @returns the SHA1 hash of the provided data
  If @a len is negative, strlen() is called on the input data.
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_sha512
  (const unsigned char *data,int len,unsigned char *result);

/** Returns a signed HMAC-SHA1 signature (20 bytes) of a data buffer
    @param key         a pointer to a key buffer
    @param key_len     the length of the key buffer in bytes (or -1)
    @param data        a pointer to a data buffer
    @param data_len    the number of bytes in the data buffer (or -1)
    @param result      a result buffer (at least 20 bytes) or NULL
    @param result_len  the byte length of the result buffer
    @returns the SHA1 hash of the provided data
  If @a len or @a key_len is negative, strlen() is called on the
    corresponding input buffers;
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_hmac_sha1
  (const unsigned char *key,int key_len,
   const unsigned char *data,int data_len,
   unsigned char *result,int *result_len);

/** Returns a signed HMAC-SHA256 signature (32 bytes) of a data buffer
    @param key         a pointer to a key buffer
    @param key_len     the length of the key buffer in bytes (or -1)
    @param data        a pointer to a data buffer
    @param data_len    the number of bytes in the data buffer (or -1)
    @param result      a result buffer (at least 32 bytes) or NULL
    @param result_len  the byte length of the result buffer
    @returns the SHA1 hash of the provided data
  If @a len or @a key_len is negative, strlen() is called on the
    corresponding input buffers;
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_hmac_sha256
  (const unsigned char *key,int key_len,
   const unsigned char *data,int data_len,
   unsigned char *result,int *result_len);


/** Returns a signed HMAC-SHA384 signature (48 bytes) of a data buffer
    @param key         a pointer to a key buffer
    @param key_len     the length of the key buffer in bytes (or -1)
    @param data        a pointer to a data buffer
    @param data_len    the number of bytes in the data buffer (or -1)
    @param result      a result buffer (at least 32 bytes) or NULL
    @param result_len  the byte length of the result buffer
    @returns the SHA1 hash of the provided data
  If @a len or @a key_len is negative, strlen() is called on the
    corresponding input buffers;
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_hmac_sha384
  (const unsigned char *key,int key_len,
   const unsigned char *data,int data_len,
   unsigned char *result,int *result_len);

/** Returns a signed HMAC-SHA512 signature (64 bytes) of a data buffer
    @param key         a pointer to a key buffer
    @param key_len     the length of the key buffer in bytes (or -1)
    @param data        a pointer to a data buffer
    @param data_len    the number of bytes in the data buffer (or -1)
    @param result      a result buffer (at least 32 bytes) or NULL
    @param result_len  the byte length of the result buffer
    @returns the SHA1 hash of the provided data
  If @a len or @a key_len is negative, strlen() is called on the
    corresponding input buffers;
  If @a result is NULL, a buffer of appropriate size is created with malloc()
**/
U8_EXPORT unsigned char *u8_hmac_sha512
  (const unsigned char *key,int key_len,
   const unsigned char *data,int data_len,
   unsigned char *result,int *result_len);


/* Computes the CRC32 code for a buffer.
   @param crc initial crc value, should be zero for the first call
   @param a pointer to a buffer to compute the CRC code for
   @param the number of bytes in the buffer to process
   @returns a new CRC code
  Note that this function is cryptographically useless.
*/
U8_EXPORT u8_int4 u8_crc32(u8_int4 crc, const void *buf, size_t size);

/* Declare cityhash prototypes */

U8_EXPORT u8_int16 u8_cityhash128(const unsigned char *s,size_t len);
U8_EXPORT u8_int8 u8_cityhash64(const unsigned char *s,size_t len);

#endif
