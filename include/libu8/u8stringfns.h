/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2010 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8STRINGFNS_H
#define LIBU8_U8STRINGFNS_H 1
#define LIBU8_U8STRINGFNS_H_VERSION \
        "$Id$"

/** \file u8stringfns.h
    These functions provide utilities over UTF-8 strings.
    Many of these work generically over NUL-terminated strings
     but some are particular to UTF-8.
 **/

U8_EXPORT u8_condition
  u8_UnexpectedEOD, u8_BadUTF8, u8_BadUNGETC, u8_NoZeroStreams;

U8_EXPORT int u8_utf8warn;

#include <stdio.h>
#include <stdarg.h>

/** Returns an uppercase version of a UTF-8 string.
    @param string a UTF-8 string
    @returns a UTF-8 string with all lowercase characters
      converted to uppercase.
**/
U8_EXPORT u8_string u8_upcase(u8_string string);

/** Returns a lower version of a UTF-8 string.
    @param string a UTF-8 string
    @returns a UTF-8 string with all uppercase characters
      converted to lowercase.
**/
U8_EXPORT u8_string u8_downcase(u8_string string);

/** Appends together any number of UTF-8 strings
    This takes any number of UTF-8 strings, finishing with
    a NULL pointer, and returns the result of appending them
    together.
**/
U8_EXPORT u8_string u8_string_append(u8_string first_string,...);

/** Substitutes one string for another within its input
    This takes an input string, a key string, and a replacement
    string.  It returns a copy of the input string with the replacement
    string substituted for all occurences of the key string.  Note: that
    this does not do any UTF-8 normalization.
**/
U8_EXPORT u8_string u8_string_subst
   (u8_string input,u8_string key,u8_string replace);

/** Extracts and copies a substring of a UTF-8 string.
    @param start a pointer into a UTF-8 string
    @param end a pointer into a later location in the same string
    @returns a UTF-8 string extracted from between the two pointers
**/
U8_EXPORT u8_string u8_slice(u8_byte *start,u8_byte *end);

/** Returns the number of characters in a UTF-8 string.
    This counts the number of unicode codepoints, so combining
    characters are counted as separate characters.
    This assumes that the string is NUL terminated; to count characters
     given a particular end pointer use u8_strlen_x()
    @param string a UTF-8 string
    @returns the number of characters (codepoints) in the string
**/
U8_EXPORT int u8_strlen(u8_string string);

/** Returns the number of bytes in a UTF-8 string.  This is just
     an alias for the C library function strlen();
    @param string a UTF-8 string
    @returns the number of bytes in the string
**/
#define u8_bytelen(string) (strlen(string))

/** Returns the number of characters in a UTF-8 string with an explicit length.
    This counts the number of unicode codepoints, so combining
    characters are counted as separate characters.
    @param string a UTF-8 string
    @param len the number of bytes in the string to be measured
    @returns the number of characters (codepoints) in the string
**/
U8_EXPORT int u8_strlen_x(u8_string string,int len);

/** Returns a pointer into @a string starting at the @a ith character.
    This does not copy its result, so the returned string shares memory
    with the @a string.
    @param string a UTF-8 string
    @param i how many characters in to start the string
    @returns a substring (not copied)
**/
U8_EXPORT u8_string u8_substring(u8_string string,int i);

/** Returns the first codepoint in @a strptr
    @param strptr a pointer into a UTF-8 string
    @returns the unicode code point at the pointer.
**/
U8_EXPORT int u8_string_ref(u8_string strptr);

/** Checks if the start of a string is a valid UTF-8 representation.
    This checks only for a single character representation.
    @param s a possible UTF-8 string
    @returns 1 if the pointer refers to a valid UTF-8 sequence, 0 otherwise
**/
U8_EXPORT int u8_validptr(u8_byte *s);

/** Checks if a string is a valid UTF-8 representation.
    @param s a possible UTF-8 string
    @returns 1 if the string is a valid UTF-8 string, 0 otherwise
**/
U8_EXPORT int u8_validp(u8_byte *s);

/** Checks if the @a n bytes starting at @a s are a valid UTF-8 string.
    @param s a possible UTF-8 string
    @param n the number of bytes in the string
    @returns 1 if the pointer refers to a valid UTF-8 sequence, 0 otherwise
**/
U8_EXPORT int u8_validate(u8_byte *s,int n);

/** Checks the validity of a UTF-8 string and copies it
    @param s a possibly (probably) valid UTF-8 string.
    @returns a valid UTF-8 string or NULL
**/
U8_EXPORT u8_string u8_valid_copy(u8_byte *s);

/** Checks the validity of a UTF-8 string and copies it, converting CRLFS
    @param s a possibly (probably) valid UTF-8 string.
    @returns a valid UTF-8 string or NULL
**/
U8_EXPORT u8_string u8_convert_crlfs(u8_byte *s);

/* u8_sgetc */

U8_EXPORT int _u8_sgetc(u8_byte **sptr);

#if U8_INLINE_IO
/** Returns a Unicode code point from a pointer to a pointer to UTF-8 string.
    This advances the pointer to the next encoded character.
    @param sptr a pointer to a pointer into a UTF-8 encoding
    @returns a unicode code point
**/
static int u8_sgetc(u8_byte **sptr)
{
  int i, ch, byte=**sptr, size;
  u8_byte *scan=*sptr; u8_string start=*sptr;
  /* Catch this error */
  if (U8_EXPECT_FALSE(byte == 0)) return -1;
  else if (U8_EXPECT_TRUE(byte<0x80)) {(*sptr)++; return byte;}
  else if (U8_EXPECT_FALSE(byte < 0xc0)) {
    /* Unexpected continuation byte */
    if (u8_utf8warn)
      u8_log(LOG_WARN,u8_BadUTF8,_("Unexpected continuation byte: 0x%2x"),byte);
    (*sptr)++;
    return 0xFFFD;}
  /* Otherwise, figure out the size and initial byte fragment */
  else if (byte < 0xE0) {size=2; ch=byte&0x1F;}
  else if (byte < 0xF0) {size=3; ch=byte&0x0F;}
  else if (byte < 0xF8) {size=4; ch=byte&0x07;}
  else if (byte < 0xFC) {size=5; ch=byte&0x3;}     
  else if (byte < 0xFE) {size=6; ch=byte&0x1;}
  else { /* Bad data, return the character */
    if (u8_utf8warn) 
      u8_log(LOG_WARN,u8_BadUTF8,_("Illegal UTF-8 byte: 0x%2x"),byte);
    (*sptr)++; return 0xFFFD;}
  i=size-1; scan++;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      if (u8_utf8warn) {
	char buf[256];
	int j=0; buf[0]='\0'; while (j<size) {
	  char tmp[8]; sprintf(tmp,"%2x",start[j]);
	  if (j>0) strcat(buf," ");
	  strcat(buf,tmp); j++;}
	u8_log(LOG_WARN,u8_BadUTF8,_("Truncated UTF-8 sequence: 0x%s"),buf);}
      *sptr=scan;
      return 0xFFFD;}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  /* And now we update the data structure */
  *sptr=scan;
  return ch;
}
#else
#define u8_sgetc _u8_sgetc
#endif

/* Byte/Character offset conversion */

U8_EXPORT u8_charoff _u8_charoffset(u8_string s,u8_byteoff i);
U8_EXPORT u8_byteoff _u8_byteoffset(u8_string s,u8_charoff i,u8_byteoff l);

#if U8_INLINE_IO
static int u8_charoffset(u8_string s,u8_byteoff i)
{
  u8_string pt=s+i; int j=0;
  while (s < pt) {
    j++; if (u8_sgetc(&s)<0) break;}
  return j;
}

static int u8_byteoffset(u8_string s,u8_charoff offset,u8_byteoff max)
{
  u8_string string=s, lim=s+max; int c=1;
  if (offset<0) return -1;
  else while ((string < lim) && (offset > 0)) {
      c=u8_sgetc(&string); offset--;}
  if (string > lim) return -1;
  else return string-s;
}
#else
#define u8_charoffset(s,i) _u8_charoffset(s,i)
#define u8_byteoffset(s,i,m) _u8_byteoffset(s,i,m)
#endif

#endif /* ndef LIBU8_U8STRINGFNS_H */

