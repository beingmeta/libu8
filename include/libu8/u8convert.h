/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 beingmeta, LLC
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8convert.h
    Functions and structures for converting between UTF-8 and
    external encodings.
    These are used both internally by libu8 and externally.  External
    developers should use the struct U8_TEXT_ENCODING structure
    opaquely through functions rather than accessing structure elements
    directly.
**/

#ifndef LIBU8_CONVERT_H
#define LIBU8_CONVERT_H 1
#define LIBU8_CONVERT_H_VERSION __FILE__

#define U8_ENCODING_INCLUDES_ASCII 1
#define U8_ENCODING_IS_LINEAR (U8_ENCODING_INCLUDES_ASCII<<1)

typedef int (*mb2uc_fn)(int *,const unsigned char *,size_t);
typedef int (*uc2mb_fn)(unsigned char *,int);

/** struct U8_CHARINFO_TABLE
    is used to store multi-byte character mappings.
    It's from field is an int packed with a big-endian
    representation of a byte sequence. It's to field is
    the corresponding unicode codepoint.
**/
struct U8_MB_MAP {unsigned int from, to;};

/** The U8_TEXT_ENCODING struct encodes information about a character
    encoding used for converting text between the encoding and UTF-8.
    Character encodings may have multiple names (aliases) are
    normalized by stripping punctuation and lowercasing actual
    names. **/
struct U8_TEXT_ENCODING {
  char **names; int flags;
  int charset_size;
  struct U8_MB_MAP *charset;
  struct U8_MB_MAP *charset_inv;
  uc2mb_fn uc2mb; mb2uc_fn mb2uc;
  struct U8_TEXT_ENCODING *next;};
typedef struct U8_TEXT_ENCODING *u8_encoding;

U8_EXPORT u8_encoding utf8_encoding;
U8_EXPORT u8_encoding ascii_encoding;
U8_EXPORT u8_encoding latin0_encoding;
U8_EXPORT u8_encoding latin1_encoding;

/** Defines a new text encoding.
    Encodings are specified by either a mapping vector or conversion
    functions.	If an encoding already exists with the particular
    definition, the name is added to the aliases for the encoding.
    This function sets up the tables for mapping into and out of the
    encoding.
    @param name the name of the encoding to be defined
    @param charset a character map vector associating 1-4 byte
    sequences (zero left padded as an int) with unicode
    code points
    @param size the length of the character map vector
    @param uc2mb a function for generating a multibyte sequence from
    a Unicode code point
    @param mb2uc a function for generating a Unicode code point from
    a multibyte sequence
    @param flags esp. (U8_ENCODING_INCLUDES_ASCII and U8_ENCODING_IS_LINEAR)
    @returns a pointer to a U8_TEXT_ENCODING structure
**/
U8_EXPORT u8_encoding u8_define_encoding
(u8_string name,struct U8_MB_MAP *charset,int size,
 uc2mb_fn uc2mb,mb2uc_fn mb2uc,int flags);

/** Loads information about an encoding from an external file.
    Reads several external encoding syntaxes and registers the
    corresponding encoding.
    @param name the name of the encoding to be defined, an ASCII string
    @param filename the file containing the encoding definition
    @returns a pointer to an U8_TEXT_ENCODING structure
**/
U8_EXPORT u8_encoding u8_load_encoding(u8_string name,u8_string filename);

/** Gets a particular encoding based on its name.
    Encoding names are normalized by lowercasing and stripping
    punctuation.
    @param name the name of the encoding desired
    @returns a pointer to an U8_TEXT_ENCODING structure or NULL if none exists
**/
U8_EXPORT struct U8_TEXT_ENCODING *u8_get_encoding(u8_string name);

/** Returns the default encoding.
    @returns a pointer to an U8_TEXT_ENCODING structure or NULL if none exists
**/
U8_EXPORT struct U8_TEXT_ENCODING *u8_get_default_encoding();

/** Sets the default encoding.
    @returns 1 if anything has changed
**/
U8_EXPORT int u8_set_default_encoding(char *name);

/** Guesses the encoding in a block of data
    @param data (a buffer of data)
    @returns an encoding if a known named encoding occurs inn the data
**/
U8_EXPORT struct U8_TEXT_ENCODING *u8_guess_encoding(u8_string data);

/** Converts @a n bytes of text encoded with @a enc to the stream @a out.
    Scans @a n bytes from @a scan up to @a end or a NUL, converting
    external representations based on @a enc into Unicode code points
    emitting those codepoints, as UTF-8, to @a out.
    If @a end is NULL, the scan proceeds up to the first NUL ('\\0')
    @returns a pointer to a U8_TEXT_ENCODING struct.
**/
U8_EXPORT int u8_convert
(struct U8_TEXT_ENCODING *enc,int n,
 struct U8_OUTPUT *out,
 const unsigned char **scan,const unsigned char *end);

/** Converts a range of bytes to a UTF-8 string based on @a enc.
    Operates on the range of bytes between @a start and @a end into
    a UTF-8 string based on the text encoding @a enc.  If @a end
    is NULL, this converts the whole NUL-terminated string at @a start.
    @param enc a pointer to a U8_TEXT_ENCODING struct
    @param start a pointer into an array of bytes
    @param end a pointer into the array of bytes after @a start or NULL
    @returns a UTF-8 string
**/
U8_EXPORT u8_string u8_make_string
(struct U8_TEXT_ENCODING *enc,
 const unsigned char *start,const unsigned char *end);

/** Converts a range of UTF-8 text into an external representation based on @a enc.
    Advances the scanner @a scan (a pointer to a pointer into a string) up until
    @a end, converting the code points into the external representation.  If @a end
    is NULL all code points up to the NUL byte are scanned.
    If @a buf is not NULL, it is used to accumulate the results and its size
    should be stored in the int pointed to by @a sizep.	 If there is not enough
    space in @a buf, @a scan is not advanced, NULL is returned, and the space
    needed is stored in the int referred to by @a sizep.
    If @a buf is NULL, a big enough buffer is allocated and returned.
    In either case, when a buffer is returned, the number of deposited bytes
    is stored in the value pointed to be @a sizep.
    When a unicode code point does not have an encoded representation, this function
    either:
    - signals an error if @a escape_char is -1
    - emits a Java-style unicode escape sequence
    (e.g. \\u1423 or \\U14451432) if @a escape_char is '\'
    - emits a XHTML character entitit (e.g. &copy; or @&#1423)
    @param enc a pointer to a U8_TEXT_ENCODING struct
    @param scan a pointer to a pointer into a UTF-8 representation
    @param end a pointer to somewhere after @a *scan in the UTF-8 representation, or NULL
    @param escape_char how to escape code points without a representation; either -1, '&', or '\'
    @param crlf whether to convert newlines into CRLF sequences
    @param buf a pointer to a byte buffer or NULL
    @param sizep a pointer to an int
    @returns a locally encoded character string
**/
U8_EXPORT unsigned char *u8_localize
(struct U8_TEXT_ENCODING *enc,
 const u8_byte **scan,const u8_byte *end,
 int escape_char,int crlf,u8_byte *buf,ssize_t *sizep);

/** Converts a range of UTF-8 text into an external representation based on @a enc.
    Returns a NUL-terminated string generated from the UTF-8 representation
    between @a start and @a end based on @a enc.
    If @a end is NULL, the whole string is used.
    Note that this assumes that the target encoding doesn't encode NUL,
    allowing it to be used as a string terminator in the result.
    @param enc a pointer to a U8_TEXT_ENCODING struct
    @param start the start of some UTF-8 data
    @param end the end of some UTF-8 data or NULL.
    @returns a UTF-8 string
**/
U8_EXPORT unsigned char *u8_localize_string
(struct U8_TEXT_ENCODING *enc,const u8_byte *start,const u8_byte *end);

/* Convert */

/** Converts a mime header-encoded string into UTF-8.
    Uses the MIME header-encoding convention to convert the
    range from @a start to @a end into UTF-8.
    @param start the start of some MIME header encoded text
    @param end the end of the MIME header encoded text
    @returns a UTF-8 string
**/
U8_EXPORT u8_string u8_mime_convert(const char *start,const char *end);

/** Converts a BASE64 ASCII representation into an array of bytes.
    Returns an array of bytes based on the base64 encoding between
    @a start and @a end, depositing the length into @a sizep.
    @param start a pointer into a string to convert
    @param end where to stop converting
    @param sizep a pointer to an int.
    @returns a pointer to an array of bytes
**/
U8_EXPORT unsigned char *u8_read_base64(const char *start,const char *end,
					ssize_t *sizep);

/** Converts a byte vector into a BASE64 ASCII representation.
    Returns an array of characters encoding the sequence of @a len bytes
    starting at @a data, storing the length of this representation in @a sizep.
    @param data	 a pointer into a byte array
    @param len	 the number of bytes in the array
    @param sizep a pointer to an int where the length of the result is stored
    @returns a pointer to an ASCII character string
**/
U8_EXPORT char *u8_write_base64(const unsigned char *data,int len,ssize_t *sizep);

/** Converts a quoted printable ASCII representation into an array of bytes.
    Returns an array of bytes based on the base64 encoding between
    @a start and @a end, depositing the length into @a sizep.
    @param start a pointer into a string to convert
    @param end where to stop converting
    @param sizep a pointer to an int.
    @returns a pointer to an array of bytes
**/
U8_EXPORT char *u8_read_quoted_printable(const char *start,const char *end,
					 ssize_t *sizep);

/** Converts a hexademical ASCII representation into an array of bytes.
    Returns an array of bytes based on the hexadecmial encoding of
    @a len bytes starting at @a data, depositing the length into @a sizep.
    If the length is negative, it is computed by calling strlen() on the data.
    @param start a pointer into a string to convert
    @param len where to stop converting
    @param sizep a pointer to an int.
    @returns a pointer to an array of bytes
**/
U8_EXPORT unsigned char *u8_read_base16(const char *start,int len,ssize_t *sizep);

/** Converts a byte vector into a BASE64 (hexadecimal) ASCII representation.
    Returns a NUL-terminated array of characters encoding the sequence
    of @a len bytes starting at @a data.
    @param data	 a pointer into a byte array
    @param len	 the number of bytes in the array
    @returns a pointer to an ASCII character string
**/
U8_EXPORT char *u8_write_base16(const unsigned char *data,int len);

#endif
