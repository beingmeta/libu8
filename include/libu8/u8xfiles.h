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

#ifndef LIBU8_XFILES_H
#define LIBU8_XFILES_H 1
#define LIBU8_XFILES_H_VERSION __FILE__

U8_EXPORT u8_condition u8_nopos, u8_nowrite, u8_no_read;

#define U8_DEFAULT_XFILE_BUFSIZE 10000

/** struct U8_XFILE
    is an abstract generalization of the U8_XINPUT and U8_XOUTPUT structs which
    provide stream-based conversion to and from other character sets. **/
typedef struct U8_XFILE {
  U8_STREAM_FIELDS;
  int u8_xfd; struct U8_TEXT_ENCODING *u8_xencoding; int u8_xescape;
  unsigned char *u8_xbuf; int u8_xbuflive, u8_xbuflim;
#if U8_THREADS_ENABLED
  u8_mutex u8_lock;
#endif
} U8_XFILE;
typedef struct U8_XFILE *u8_xfile;

/** struct U8_XINPUT
    is the extension of U8_INPUT which deals with stream-based conversion
    from other character sets.	It's logic reads chunks of input from a POSIX
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
    data read from the file descriptor.	 xbuf is a pointer to the buffer,
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
    into other character sets.	It's logic converts written UTF-8 data into
    a local character set and writes that data to an external POSIX file
    descriptor.	 In addition to the U8_INPUT fields, the fd field contains
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

/* Creates an input XFILE given a file descriptor and an encoding
   @param fd an integer file description
   @param enc a pointer to an U8_ENCODING structure or NULL
   @returns a pointer to a U8_XINPUT structure
*/
U8_EXPORT struct U8_XINPUT *u8_open_xinput(int fd,u8_encoding enc);

/* Creates an output XFILE given a file descriptor and an encoding
   @param fd an integer file description
   @param enc a pointer to an U8_ENCODING structure or NULL
   @returns a pointer to a U8_XOUTPUT structure
*/
U8_EXPORT struct U8_XOUTPUT *u8_open_xoutput(int fd,u8_encoding enc);

/* Initializes an XFILE structure for input given a file descriptor
   and an encoding

   @param xo a pointer to an U8_XOUTPUT structure
   @param fd an integer file description
   @param enc a pointer to an U8_ENCODING structure or NULL
   @returns 1 on success -1 on failure
*/
U8_EXPORT int u8_init_xinput(struct U8_XINPUT *xi,int fd,u8_encoding enc);

/* Initializes an XFILE structure for output given a file descriptor
   and an encoding

   @param xo a pointer to an U8_XOUTPUT structure
   @param fd an integer file description
   @param enc a pointer to an U8_ENCODING structure or NULL
   @returns 1 on success -1 on failure
*/
U8_EXPORT int u8_init_xoutput(struct U8_XOUTPUT *xo,int fd,u8_encoding enc);

U8_EXPORT int u8_xoutput_setbuf(struct U8_XOUTPUT *xo,int bufsiz);
U8_EXPORT int u8_xinput_setbuf(struct U8_XINPUT *xi,int bufsiz);

/* Opens an input XFILE given a filename
   @param filename a filename (utf8-encoded)
   @param enc a pointer to an U8_ENCODING structure or NULL
   @param flags to be passed to open()
   @param perm a permissions for created files
   @returns a pointer to a U8_XINPUT structure
*/
U8_EXPORT struct U8_XINPUT *u8_open_input_file
(u8_string filename,u8_encoding enc,int flags,int perm);

/* Opens an output XFILE given a filename
   @param filename a filename (utf8-encoded)
   @param enc a pointer to an U8_ENCODING structure or NULL
   @param flags to be passed to open()
   @param perm a permissions for created files
   @returns a pointer to a U8_XOUTPUT structure
*/
U8_EXPORT struct U8_XOUTPUT *u8_open_output_file
(u8_string filename,u8_encoding enc,int flags,int perm);

U8_EXPORT off_t u8_getpos(struct U8_STREAM *);
U8_EXPORT off_t u8_setpos(struct U8_STREAM *,off_t off);
U8_EXPORT off_t u8_endpos(struct U8_STREAM *);
U8_EXPORT double u8_getprogress(struct U8_STREAM *);

/* Closes an output XFILE
   @param xo a pointer to a U8_XOUTPUT struct
*/
U8_EXPORT void u8_close_xoutput(struct U8_XOUTPUT *xo);

/* Closes an output XFILE
   @param xi a pointer to a U8_XINPUT struct
*/
U8_EXPORT void u8_close_xinput(struct U8_XINPUT *xo);

U8_EXPORT void u8_flush_xoutput(struct U8_XOUTPUT *f);

/* Fills the buffer for an XFILE, reading input from the
   XFILE's file descriptor and converting it according to
   the XFILE's encoding.
   @param xf an XFILE struct
   @returns the number of bytes converted
*/
U8_EXPORT int u8_fill_xinput(struct U8_XINPUT *xf);

/** struct U8_OPEN_XFILES
    is a linked list of open xfiles used to ensure that
    buffers are flushed when the process ends normally.
**/
typedef struct U8_OPEN_XFILES {
  struct U8_XFILE *xfile;
  struct U8_OPEN_XFILES *next;} U8_OPEN_XFILES;
typedef struct U8_OPEN_XFILES *u8_open_xfiles;

U8_EXPORT void u8_register_open_xfile(struct U8_XFILE *out);
U8_EXPORT void u8_deregister_open_xfile(struct U8_XFILE *out);
U8_EXPORT void u8_close_xfiles();

#endif
