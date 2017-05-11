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

#ifndef LIBU8_U8STRINGFNS_H
#define LIBU8_U8STRINGFNS_H 1
#define LIBU8_U8STRINGFNS_H_VERSION __FILE__

/** \file u8stringfns.h
    These functions provide utilities over UTF-8 strings.
    Many of these work generically over NUL-terminated strings
     but some are particular to UTF-8.
 **/

U8_EXPORT u8_condition u8_UnexpectedEOD, u8_BadUNGETC, u8_NoZeroStreams, u8_BadUnicodeChar;
U8_EXPORT u8_condition u8_TruncatedUTF8, u8_BadUTF8, u8_BadUTF8byte;

U8_EXPORT int u8_utf8warn, u8_utf8err;

#include <stdarg.h>

/** Returns an uppercase version of a UTF-8 string.
    @param string a UTF-8 string
    @returns a UTF-8 string with all lowercase characters
      converted to uppercase.
**/
U8_EXPORT u8_string u8_upcase(u8_string string);

/** Returns a lowercase version of a UTF-8 string.
    @param string a UTF-8 string
    @returns a UTF-8 string with all uppercase characters
      converted to lowercase.
**/
U8_EXPORT u8_string u8_downcase(u8_string string);

/** Determines if a string has a particular prefix
    @param string a UTF-8 string
    @param prefix a UTF-8 string
    @param casefold an int (1 or 0)
    @returns 1 if string starts with prefix, 0 otherwise
    If casefold is not zero, case is ignored
**/
U8_EXPORT int u8_has_prefix(u8_string string,u8_string prefix,int casefold);


/** Determines if a string has a particular suffix
    @param string a UTF-8 string
    @param suffix a UTF-8 string
    @param casefold an int (1 or 0)
    @returns 1 if string ends with suffix, 0 otherwise
    If casefold is not zero, case is ignored
**/
U8_EXPORT int u8_has_suffix(u8_string string,u8_string suffix,int casefold);

/** Searches for a character in a string
    @param haystack a UTF-8 string
    @param needles a sequence of characters in a UTF-8 string
    @param order an integer indicating which match is returned
    @returns a substring of 'string' or NULL

    This returns a pointer to the place in 'string' where any of the
    characters 'chars' is found or NULL if none of them are found.  If
    order is 0 , return the string based on the first character (in
    'chars') to match, otherwise priority is based on where the match
    occurs in the searched 'string'.

    If order is > 0, this returns the earliest match
    if order is < 0, this return the furthest match
    if order is = 0, this return the first match in strings
**/
U8_EXPORT u8_string u8_strchrs(u8_string haystack,u8_string needles,int order);

/** Searches for a character in a string
    @param string a UTF-8 string
    @param ch a char
    @param n which match to return, use n<0 to match from the end, 
           n=0 is the same as n=1
    @returns a substring of 'string' or NULL
    
**/
U8_EXPORT u8_string u8_strchr(u8_string string,int ch,int n);

/** Searches for a set of strings in a string
    @param haystack a UTF-8 string
    @param needles a NUL-terminated array of string pointers
    @param order an int
    @returns a substring of 'string' or NULL

    This returns a place in 'string' where any of the strings in
    'strings' occurs; order determines which match is returned (when
    there are several).  If order is 0, priority is based on the order
    of the strings in 'strings', otherwise, priority is based on the
    earliest match occurs in the searched 'string'.

    If order is > 0, this returns the earliest match
    if order is < 0, this return the furthest match
    if order is = 0, this return the first match in strings

**/
U8_EXPORT u8_string u8_strstrs(u8_string haystack,u8_string needles[],int order);

/** Searches for a substring in a string
    @param haystack a UTF-8 string
    @param needle a string
    @param direction indicates which match to return, use n<0 to match
           from the end, n=0 is the same as n=1
    @returns a substring of 'string' or NULL
    
**/
U8_EXPORT u8_string u8_strstr(u8_string haystack,u8_string needle,int direction);

/** Generates a base-ten representation of a long long int
    This should be safe to use in, for example, signal handlers, where printf
    is verboten.
    @param n a long long int (may be automatically cast up, of course)
    @param buf a static 24-byte buffer (long enough to contain the largest N)
    @returns pointer to that buffer
**/
U8_EXPORT char *u8_itoa10(long long int n,char buf[32]);

/* Handy definition */

/* Write a long long to a string.  This writes the string without
   using printf or other 'heavyweight' functions. It can be used in
   resource-tight environments and can also be called in signal
   handlers as needed.
   @param val the signed integer value to write to the buffer
   @param buf a character buffer to place the representation
   @param buflen the size of the character buffer
 */
U8_EXPORT char *u8_write_long_long(long long l,char *buf,size_t buflen);

/** Generates a base-ten representation of a long long int
    This should be safe to use in, for example, signal handlers, where printf
    is verboten.
    @param n a long long int (may be automatically cast up, of course)
    @param buf a static 24-byte buffer (long enough to contain the largest N)
    @returns pointer to that buffer
**/
U8_EXPORT char *u8_uitoa10(unsigned long long int n,char buf[32]);

/** Generates a base-ten representation of a long long int
    This should be safe to use in, for example, signal handlers, where printf
    is verboten.
    @param n a long long int (may be automatically cast up, of course)
    @param buf a static 24-byte buffer (long enough to contain the largest N)
    @returns pointer to that buffer
**/
U8_EXPORT char *u8_uitoa8(unsigned long long int n,char buf[32]);

/** Generates a base-eight representation of an unsigned long long int
    This should be safe to use in, for example, signal handlers, where printf
    is verboten.
    @param n a long long int (may be automatically cast up, of course)
    @param buf a static 24-byte buffer (long enough to contain the largest N)
    @returns pointer to that buffer
**/
U8_EXPORT char *u8_uitoa16(unsigned long long int n,char buf[32]);

/** Returns a decomposed version of a UTF-8 string.
    @param string a UTF-8 string
    @returns a UTF-8 string with all composed characters
      broken down
**/
U8_EXPORT u8_string u8_decompose(u8_string string);

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
U8_EXPORT u8_string u8_slice(const u8_byte *start,const u8_byte *end);

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
U8_EXPORT int u8_validptr(const u8_byte *s);

/** Checks if a string is a valid UTF-8 representation.
    @param s a possible UTF-8 string
    @returns 1 if the string is a valid UTF-8 string, 0 otherwise
**/
U8_EXPORT int u8_validp(u8_string s);

/** Checks if the @a n bytes starting at @a s are a valid UTF-8 string.
    @param s a possible UTF-8 string
    @param n the number of bytes in the string
    @returns 1 if the pointer refers to a valid UTF-8 sequence, 0 otherwise
**/
U8_EXPORT int u8_validate(u8_string s,int n);

/** Checks the validity of a UTF-8 string and copies it
    @param s a possibly (probably) valid UTF-8 string.
    @returns a valid UTF-8 string or NULL
**/
U8_EXPORT u8_string u8_valid_copy(u8_string s);

/** Checks the validity of a UTF-8 string and copies it, converting CRLFS
    @param s a possibly (probably) valid UTF-8 string.
    @returns a valid UTF-8 string or NULL
**/
U8_EXPORT u8_string u8_convert_crlfs(u8_string s);

/** Adds indentation at the beginning of every line within a string
    @param input an original string, possibly with newlines
    @param indent the indent string
    @returns a copy of the original string with the indent string
      inserted before every '\n'
**/
U8_EXPORT u8_string u8_indent_text(u8_string input,u8_string indent);

/** Returns an ascii-converted substring of a possible UTF-8 string
    @param s a possibly (probably) valid UTF-8 string.
    @param n the size (in bytes) of the substring to return
    @param buf an optional buffer to use (otherwise, one is mallocd
    @returns an ASCII string, possibly with %-encoded bytes
**/
U8_EXPORT char *u8_grab_bytes(u8_string s,int n,char *buf);

/* u8_sgetc */

U8_EXPORT int _u8_sgetc(const u8_byte **sptr);
U8_EXPORT int _u8_sgetc_lim(const u8_byte **sptr,const u8_byte *lim);

static U8_MAYBE_UNUSED char hexchars[]="0123456789ABCDEF";

#if U8_INLINE_IO
/** Returns a Unicode code point from a pointer to a pointer to UTF-8 string.
    This advances the pointer to the next encoded character.
    @param sptr a pointer to a pointer into a UTF-8 encoding
    @returns a unicode code point
**/
static int u8_sgetc_lim(const u8_byte **sptr,const u8_byte *lim)
{
  int i, ch, byte=**sptr, size;
  const u8_byte *scan=*sptr;
  /* Catch this error */
  if (U8_EXPECT_FALSE(byte == 0)) return -1;
  else if (U8_EXPECT_TRUE((lim)&&(scan>=lim))) return -1;
  else if (U8_EXPECT_TRUE(byte<0x80)) {(*sptr)++; return byte;}
  else if (U8_EXPECT_FALSE(byte < 0xc0)) {
    /* Unexpected continuation byte */
    if (u8_utf8err) {
      char *details; int n_bytes=UTF8_BUGWINDOW;
      if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
      details=u8_grab_bytes(*sptr,n_bytes,NULL);
      u8_log(LOG_WARN,u8_BadUTF8,_("Unexpected UTF-8 continuation byte: '%s'"),
             details);
      u8_seterr(u8_BadUTF8byte,"u8_sgetc",details);
      (*sptr)++; return -2;}
    else if (u8_utf8warn) {
      char buf[UTF8_BUGWINDOW]; int n_bytes=UTF8_BUGWINDOW;
      if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
      u8_grab_bytes(*sptr,n_bytes,buf);
      u8_log(LOG_WARN,u8_BadUTF8,_("Unexpected UTF-8 continuation byte: '%s'"),
             buf);}
    (*sptr)++;
    return 0xFFFD;}
  /* Otherwise, figure out the size and initial byte fragment */
  else if (byte < 0xE0) {size=2; ch=byte&0x1F;}
  else if (byte < 0xF0) {size=3; ch=byte&0x0F;}
  else if (byte < 0xF8) {size=4; ch=byte&0x07;}
  else if (byte < 0xFC) {size=5; ch=byte&0x3;}
  else if (byte < 0xFE) {size=6; ch=byte&0x1;}
  else { /* Bad data, return the character */
    if (u8_utf8err) {
      char *details; int n_bytes=UTF8_BUGWINDOW;
      if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
      details=u8_grab_bytes(*sptr,n_bytes,NULL);
      u8_log(LOG_WARN,u8_BadUTF8,_("Illegal UTF-8 byte: '%s'"),details);
      u8_seterr(u8_BadUTF8byte,"u8_sgetc",details);
      (*sptr)++; return -2;}
    else if (u8_utf8warn) {
      char buf[UTF8_BUGWINDOW]; int n_bytes=UTF8_BUGWINDOW;
      if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
      u8_grab_bytes(*sptr,n_bytes,buf);
      u8_log(LOG_WARN,u8_BadUTF8,_("Illegal UTF-8 byte: '%s'"),buf);}
    (*sptr)++; return 0xFFFD;}
  i=size-1; scan++;
  while (i) {
    if ((*scan<0x80) || (*scan>=0xC0)) {
      if (u8_utf8err) {
        char *details; int n_bytes=UTF8_BUGWINDOW;
        if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
        details=u8_grab_bytes(*sptr,n_bytes,NULL);
        u8_log(LOG_WARN,u8_BadUTF8,_("Truncated UTF-8 sequence: '%s'"),details);
        u8_seterr(u8_TruncatedUTF8,"u8_sgetc",details);
        return -2;}
      else if (u8_utf8warn) {
        char buf[UTF8_BUGWINDOW]; int n_bytes=UTF8_BUGWINDOW;
        if ((lim)&&((lim-(*sptr))<n_bytes)) n_bytes=lim-*sptr;
        u8_grab_bytes(*sptr,n_bytes,buf);
        u8_log(LOG_WARN,u8_BadUTF8,_("Truncated UTF-8 sequence: '%s'"),buf);}
      *sptr=scan;
      return 0xFFFD;}
    else {ch=(ch<<6)|(*scan&0x3F); scan++; i--;}}
  /* And now we update the data structure */
  *sptr=scan;
  return ch;
}
static int u8_sgetc(const u8_byte **sptr)
{
  return u8_sgetc_lim(sptr,NULL);
}
#else
#define u8_sgetc _u8_sgetc
#define u8_sgetc_lim _u8_sgetc_lim
#endif

/* Copying UTF-8 string prefixes into fixed length buffers */

U8_EXPORT int _u8_char_len(u8_string s);
U8_EXPORT u8_string _u8_string2buf(u8_string string,u8_byte *buf,size_t len);

#if U8_INLINE_IO
static U8_MAYBE_UNUSED
/** Returns the length of the utf-8 character whose representation
    starts at s. Returns -1 if the byte at the start of the string is
    not the start of a UTF-8 sequence or an ASCII-128 (which is a
    sequence of one).

    @param s a UTF-8 string
    @returns an int between 1 and 7 inclusive or -1
**/
int u8_char_len(u8_string s)
{
  int s1=*s;
  if (s1 < 0x80) return 1;
  else if (s1 < 0xC0) return -1;
  else if (s1 < 0xE0) return 2;
  else if (s1 < 0xF0) return 3;
  else if (s1 < 0xF8) return 4;
  else if (s1 < 0xFC) return 5;
  else if (s1 < 0xFE) return 6;
  else return -1;
}

static U8_MAYBE_UNUSED
/** Copies at most *len* bytes of *string* into *buf*, making sure
    that the copy doesn't terminate inside of a UTF-8 multi-byte
    representation.
    @param string a UTF-8 string
    @param buf a pointer to a byte array of at least *len* bytes
    @param len the length of the byte array
    @returns an int between 1 and 7 inclusive or -1
**/

u8_string u8_string2buf(u8_string string,u8_byte *buf,size_t len)
{
  u8_string scan=string;
  u8_byte *write=buf, *buflim=buf+len-1;
  int clen=u8_char_len(scan);
  while ((*scan) && (clen>0) && ((write+clen)<buflim)) {
    if (clen==1)
      *write++=*scan++;
    else {
      memmove(write,scan,clen);
      scan+=clen; write+=clen;}
    clen=u8_char_len(scan);}
  *write='\0';
  return buf;
}
#else
#define u8_char_len   _u8_char_len
#define u8_string2buf _u8_string2buf
#endif

/* Byte/Character offset conversion */

U8_EXPORT u8_charoff _u8_charoffset(u8_string s,u8_byteoff i);
U8_EXPORT u8_byteoff _u8_byteoffset(u8_string s,u8_charoff i,u8_byteoff l);

#if U8_INLINE_IO
static U8_MAYBE_UNUSED
int u8_charoffset(u8_string s,u8_byteoff i)
{
  u8_string pt=s+i; int j=0;
  while (s < pt) {
    j++; if (u8_sgetc(&s)<0) break;}
  return j;
}

static U8_MAYBE_UNUSED
int u8_byteoffset(u8_string s,u8_charoff offset,u8_byteoff max)
{
  u8_string string=s, lim=s+max; int c=0;
  if (offset<0) return -1;
  else while ((string < lim) && (offset > 0) && (c>=0)) {
      c=u8_sgetc(&string); offset--;}
  if (string > lim) return -1;
  else return string-s;
}
#else
#define u8_charoffset(s,i) _u8_charoffset(s,i)
#define u8_byteoffset(s,i,m) _u8_byteoffset(s,i,m)
#endif

#endif /* ndef LIBU8_U8STRINGFNS_H */
