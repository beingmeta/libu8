/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2013 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8ctype.h
    These functions and macros interrogate and transform unicode points.
    They include standard character predicates (u8_isspace, u8_ispunct, etc)
     as well as function/macros for changing case and converting to and from
     XML character entities.
 **/

#ifndef LIBU8_CTYPE_H
#define LIBU8_CTYPE_H 1
#define LIBU8_CTYPE_H_VERSION __FILE__

#define U8_LOWER_LETTER 1
#define U8_UPPER_LETTER 2
#define U8_TITLE_LETTER 3
#define U8_MODIFIER_LETTER 4
#define U8_OTHER_LETTER 5
#define U8_MARK 6
#define U8_NUMBER 7
#define U8_FUNNY_NUMBER 8
#define U8_GLUE_PUNCTUATION 9
#define U8_BREAK_PUNCTUATION 10
#define U8_SYMBOL 11
#define U8_SEPARATOR 12
#define U8_OTHER 13

U8_EXPORT int u8_charinfo_size;
U8_EXPORT const unsigned char *u8_charinfo;
U8_EXPORT const short *u8_chardata;

/** struct U8_DECOMPOSITION
     indicates a mapping between a single Unicode codepoint
     and an equivalent Unicode sequence.
**/
typedef struct U8_DECOMPOSITION {
  int code; u8_string decomp;} U8_DECOMPOSITION;
typedef struct U8_DECOMPOSITION *u8_decomposition;

/** struct U8_CHARINFO_TABLE
     is used to store additional character info not provided 
     by the statically defined tables.
**/
U8_EXPORT const struct U8_CHARINFO_TABLE {
  unsigned int code_start, code_end;
  unsigned char typeinfo;
  unsigned int chardata;}
  *u8_extra_charinfo;

U8_EXPORT int u8_lookup_charinfo(int c);
U8_EXPORT int u8_lookup_chardata(int c);
U8_EXPORT u8_string u8_decompose_char(unsigned int ch);
U8_EXPORT int u8_base_char(unsigned int ch);

#define u8_getcharinfo(c) \
  ((c<u8_charinfo_size) ? \
   ((c%2) ? (u8_charinfo[c/2]&0xF) : ((u8_charinfo[c/2])>>4)) : \
    (u8_lookup_charinfo(c)))
#define u8_getchardata(c) \
  ((c<u8_charinfo_size) ? (u8_chardata[c]) : (u8_lookup_chardata(c)))

/** Returns 1 if its argument is an alphabetic unicode point. **/
#define u8_isalpha(c) ((c>=0) && ((u8_getcharinfo(c)) < 6)) 
/** Returns 1 if its argument is a lower-case alphabetic unicode point. **/
#define u8_islower(c) ((c>=0) && ((u8_getcharinfo(c)) == U8_LOWER_LETTER)) 
/** Returns 1 if its argument is an upper-case alphabetic unicode point. **/
#define u8_isupper(c) \
  ((c>=0) && \
   (((u8_getcharinfo(c)) == U8_UPPER_LETTER) || \
    ((u8_getcharinfo(c)) == U8_TITLE_LETTER)))
/** Returns 1 if its argument is modifier unicode point. **/
#define u8_ismodifier(c) ((c>=0) && ((u8_getcharinfo(c)) == U8_MODIFIER_LETTER)) 
/** Returns 1 if its argument is numeric digit unicode point. **/
#define u8_isdigit(c) ((c>=0) && ((u8_getcharinfo(c)) == U8_NUMBER)) 
/** Returns 1 if its argument is a punctuation character. **/
#define u8_ispunct(c)                                \
  ((c>=0) &&                                         \
   (((u8_getcharinfo(c)) == U8_GLUE_PUNCTUATION)  || \
    ((u8_getcharinfo(c)) == U8_BREAK_PUNCTUATION) || \
    ((u8_getcharinfo(c)) == U8_SYMBOL)            || \
    ((u8_getcharinfo(c)) == U8_MARK)))
/** Returns 1 if its argument is a printing character (letter,digit,punct) **/
#define u8_isprint(c)                                \
  ((c>=0) &&					     \
   ((u8_isalpha(c))                               || \
    (u8_isdigit(c))                               || \
    ((u8_getcharinfo(c)) == U8_GLUE_PUNCTUATION)  || \
    ((u8_getcharinfo(c)) == U8_BREAK_PUNCTUATION) || \
    ((u8_getcharinfo(c)) == U8_SYMBOL) ||	     \
    ((u8_getcharinfo(c)) == U8_MARK)))
/** Returns 1 if its argument is whitespace unicode point. **/
#define u8_isspace(c) ((c>=0) && ((u8_getcharinfo(c)) == U8_SEPARATOR))
/** Returns 1 if its argument is horizontal whitespace unicode point. **/
#define u8_ishspace(c)                            \
  ((c>=0) &&                                      \
   ((c==' ')||(c=='\t')||(c==0x1680)||(c==0x180e)|| \
    ((c>=0x2000)&&(c<0x200b))))
/** Returns 1 if its argument is horizontal whitespace unicode point. **/
#define u8_isvspace(c)                            \
  ((c>=0) &&                                      \
   ((c=='\n')||(c=='\r')||(c==0x0c)||(c==0x0b)||  \
    (c==0x1C)||(c==0x1D)||(c==0x1E)||(c==0x1F)||  \
    (c==0x85)||(c==0x2029)))
/** Returns 1 if its argument is a standard control character. **/
#define u8_isctrl(c) ((c>=0) && ((c<0x20) || ((c>0x7e) && (c<0x9f))))
/** Returns 1 if its argument is an alphanumeric unicode point. **/
#define u8_isalnum(c) ((c>=0) && (((u8_getcharinfo(c)) < 6) || (u8_isdigit(c)))) 
/** Returns 1 if its argument is an ASCII hex digit. **/
#define u8_isxdigit(c) ((c>=0) && ((c<128) && (isxdigit(c))))
/** Returns 1 if its argument is an ASCII octal digit. **/
#define u8_isodigit(c) ((c>=0) && ((c<128) && (isdigit(c)) && (c<'8')))

/** Returns a non-lowercase version of a unicode code point.  **/
#define u8_toupper(c) \
  ((u8_islower(c)) ? \
   ((c<0x10000) ? (c+(u8_getchardata(c))) : (u8_lookup_chardata(c))) : \
   (c))
/** Returns a non-uppercase version of a unicode code point.  **/
#define u8_tolower(c) \
  ((u8_isupper(c)) ? \
   ((c<0x10000) ? (c+(u8_getchardata(c))) : (u8_lookup_chardata(c))) : \
   (c))
/** Returns the numeric weight of a numeric unicode code point. **/
#define u8_digit_weight(c) \
  ((u8_isdigit(c)) ? ((c<0x10000) ? (u8_getchardata(c)) : (u8_lookup_chardata(c))) : (0))

/* Entity conversion */

/** Converts an XML entity name into the corresponding code point.
    This returns -1 for unrecognized or invalid entity names.
    @param name a utf-8 (ASCII) name. 
    @returns a unicode code point or -1 on error.
**/
U8_EXPORT int u8_entity2code(u8_string name);

/** Converts a code point into an XML entity name.
    This falls back to hex codes if neccessary.
    @param code a unicode code point
    @returns a mallocd utf-8 (ASCII) name.
**/
U8_EXPORT u8_string u8_code2entity(int code);

/** Parses a unicode entity name from a string, recording the endpoint.
    This is handed a pointer to a UTF-8 string (@a entity) just
      after the entity escape character ampersand ('&').
    It parses an entity name, returning the
     corresponding code and storing the end of the entity (after the
     trailing semicolon (';')) in @a endp.  If @a endp is NULL,
     the end result is not stored.  If the string does not point to
     a valid entity reference, -1 is returned.
    @param entity a pointer into a UTF-8 string
    @param endp a pointer to a location to store the end of the entity
    @returns a unicode code point
**/
U8_EXPORT int u8_parse_entity(u8_byte *entity,u8_byte **endp);

/** Parses a unicode entity name from a string, recording the endpoint.
    This version sets an error when an entity cannot be processed.
    This is handed a pointer to a UTF-8 string (@a entity) just
      after the entity escape character ampersand ('&').
    It parses an entity name, returning the
     corresponding code and storing the end of the entity (after the
     trailing semicolon (';')) in @a endp.  If @a endp is NULL,
     the end result is not stored.
    @param entity a pointer into a UTF-8 string
    @param endp a pointer to a location to store the end of the entity
    @returns a unicode code point
**/
U8_EXPORT int u8_parse_entity_err(u8_byte *entity,u8_byte **endp);

/** Sets the character information for a particular code point.
    @param n a Unicode code point
    @param info a string describing information about the character
    @param data a pointer to a short vector of data about the character
    @returns void
**/
U8_EXPORT void u8_set_charinfo(int n,unsigned char *info,short *data);

#endif

