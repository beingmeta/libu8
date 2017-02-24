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

#ifndef LIBU8_XFILES_H
#define LIBU8_XFILES_H 1
#define LIBU8_XFILES_H_VERSION __FILE__

U8_EXPORT u8_condition u8_nopos, u8_nowrite, u8_no_read;

#define U8_DEFAULT_XFILE_BUFSIZE 1024
#define U8_XFILE_SEEKS 2
#define U8_XFILE_LOCKED 4

/** struct U8_XINPUT
    is the extension of U8_INPUT which deals with stream-based conversion
    from other character sets.  It's logic reads chunks of input from a POSIX
    file descriptor and then converts their content into a UTF-8
    representation in the head of the structure, where generic functions
    (on U8_INPUT) can operate on it.  In addition to the U8_INPUT fields,
    the fd field contains the POSIX file descriptor used for input,
    the encoding field points to a U8_TEXT_ENCODING structure, and the
    field escape indicates whether to automatically convert embedded escapes.
    If this field is '\\,' conversion automatically converts \\u and \\U escapes
    into Unicode code points; if this field is '&,' conversion automatically
    interprets entity escape sequences.
    The xbuf, xbuflen, and xbuflim fields describe the buffer used to store
    data read from the file descriptor.  xbuf is a pointer to the buffer,
    xbuflen is the number of valid data bytes, and xbuflim is the number
    of writable bytes in the buffer (it's allocated size).  **/
typedef struct U8_XINPUT {
  U8_INPUT_FIELDS;
  int u8_xfd; struct U8_TEXT_ENCODING *u8_xencoding; int u8_xescape;
  unsigned char *u8_xbuf; int u8_xbuflive, u8_xbuflim;
#if U8_THREADS_ENABLED
  u8_mutex u8_lock;
#endif
} U8_XINPUT;
typedef struct U8_XINPUT *u8_xinput;

/** struct U8_XOUTPUT
    is the extension of U8_OUTPUT which deals with stream-based conversion
    into other character sets.  It's logic converts written UTF-8 data into
    a local character set and writes that data to an external POSIX file
    descriptor.  In addition to the U8_INPUT fields, the fd field contains
    the POSIX file descriptor used for input, the encoding field points to a
    U8_TEXT_ENCODING structure, and the field escape indicates whether to
    automatically convert unrepresentable code points into an external
    representation.  If this field is '\\,' conversion automatically converts
    unrepresentable characters into \\u and \\U escape sequences;
    if this field is '&,' conversion automatically interprets entity escape
    sequences.
    The xbuf, xbuflen, and xbuflim fields describe the buffer used to store
    data to write to the file descriptor.  xbuf is a pointer to the buffer,
    xbuflen is the number of valid data bytes, and xbuflim is the number
    of writable bytes in the buffer (it's allocated size).  **/
typedef struct U8_XOUTPUT {
  U8_OUTPUT_FIELDS;
  int u8_xfd; struct U8_TEXT_ENCODING *u8_xencoding; int u8_xescape;
  unsigned char *u8_xbuf; int u8_xbuflive, u8_xbuflim;
#if U8_THREADS_ENABLED
  u8_mutex u8_lock;
#endif
  } U8_XOUTPUT;
typedef struct U8_XOUTPUT *u8_xoutput;

U8_EXPORT struct U8_XOUTPUT *u8_open_xoutput(int fd,u8_encoding enc);
U8_EXPORT struct U8_XINPUT *u8_open_xinput(int fd,u8_encoding enc);

U8_EXPORT int u8_init_xoutput(struct U8_XOUTPUT *,int fd,u8_encoding enc);
U8_EXPORT int u8_init_xinput(struct U8_XINPUT *,int fd,u8_encoding enc);

U8_EXPORT int u8_xoutput_setbuf(struct U8_XOUTPUT *xo,int bufsiz);
U8_EXPORT int u8_xinput_setbuf(struct U8_XINPUT *xi,int bufsiz);

U8_EXPORT struct U8_XOUTPUT *u8_open_output_file
  (u8_string filename,u8_encoding enc,int flags,int perm);
U8_EXPORT struct U8_XINPUT *u8_open_input_file
  (u8_string filename,u8_encoding enc,int flags,int perm);

U8_EXPORT off_t u8_getpos(struct U8_STREAM *);
U8_EXPORT off_t u8_setpos(struct U8_STREAM *,off_t off);
U8_EXPORT off_t u8_endpos(struct U8_STREAM *);
U8_EXPORT double u8_getprogress(struct U8_STREAM *);

U8_EXPORT void u8_close_xoutput(struct U8_XOUTPUT *);
U8_EXPORT void u8_close_xinput(struct U8_XINPUT *);

U8_EXPORT void u8_flush_xoutput(struct U8_XOUTPUT *f);

/** struct U8_OPEN_XFILES
     is a linked list of open xfiles used to ensure that
     buffers are flushed when the process ends normally.
**/
typedef struct U8_OPEN_XFILES {
  U8_XOUTPUT *xfile;
  struct U8_OPEN_XFILES *next;} U8_OPEN_XFILES;
typedef struct U8_OPEN_XFILES *u8_open_xfiles;

U8_EXPORT void u8_register_open_xfile(struct U8_XOUTPUT *out);
U8_EXPORT void u8_deregister_open_xfile(struct U8_XOUTPUT *out);
U8_EXPORT void u8_close_xfiles();

U8_EXPORT int u8_writeall(int sock,const unsigned char *data,int len);

#endif
