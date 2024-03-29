/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2022 Kenneth Haase (ken.haase@alum.mit.edu)
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

/** \file u8streamio.h
    These functions and macros support I/O with UTF-8 streams.
    The files here provide a generic buffered I/O layer and immediate
    operations with in-memory streams writing to UTF-8 byte buffers.
    Within libu8, this provides support for "xfiles" which provide
    automatic conversion to/from external character encodings.
**/

#ifndef LIBU8_U8STREAMIO_H
#define LIBU8_U8STREAMIO_H 1
#define LIBU8_U8STREAMIO_H_VERSION __FILE__

/* String input */

#include <assert.h>
#include <string.h>

U8_EXPORT ssize_t u8_validate(u8_string s,size_t n);

U8_EXPORT u8_condition u8_UnexpectedEOD, u8_BadUNGETC, u8_NoZeroStreams;
U8_EXPORT u8_condition u8_TruncatedUTF8, u8_BadUTF8;

U8_EXPORT int u8_utf8warn, u8_utf8err;

#ifndef U8_BUF_THROTTLE_POINT
#define U8_BUF_THROTTLE_POINT (256*1024)
#endif

#ifndef U8_BUF_MIN_GROW
#define U8_BUF_MIN_GROW 16
#endif

#ifndef U8_BUF_INIT_SIZE
#define U8_BUF_INIT_SIZE 64
#endif

#ifndef U8_UTF8BUG_WINDOW
#define U8_UTF8BUG_WINDOW 64
#endif

/* Generic streams */

/* TODO: These bits should be organized and updated */

/** This bit describes whether the stream is mallocd or static.
    Mallocd streams are freed when closed. **/
#define U8_STREAM_MALLOCD     0x01
/** This bit describes whether the stream is an output or input stream.	 **/
#define U8_OUTPUT_STREAM      0x02
/** This bit describes whether the stream is responsible for freeing its
    buffer when closed.	 **/
#define U8_STREAM_OWNS_BUF    0x04
/** This bit describes whether the stream can grow to accomodate more input
    or output. **/
#define U8_FIXED_STREAM	      0x08
/** This bit describes whether the stream has overflowed its fixed length  **/
#define U8_STREAM_OVERFLOW    0x10
/** This bit specifies if the streams buffer is 'constant' (won't change) **/
#define U8_CONST_STREAM	      0x20
/** This bit indicates that the stream is 'interactive' **/
#define U8_STREAM_TTY	      0x40
#define U8_INPUT_TTY	      U8_STREAM_TTY
#define U8_OUTPUT_TTY	      U8_STREAM_TTY

/* These bits are for streams which are XFILES */

/** This bit describes whether an XFILE stream is responsible for freeing its
    translation buffer when closed.  **/
#define U8_STREAM_OWNS_XBUF   0x100
/** This bit describes whether an XFILE stream is responsible for closing
    its socket/file descriptor when closed.  **/
#define U8_STREAM_OWNS_SOCKET 0x200
/** This bit describes whether seeks are possible on an XFILE's underlying
    socket/file descriptor.  **/
#define U8_STREAM_CAN_SEEK    0x400
/** This bit describes whether the XFILE should do CRLF translation.
    This is mostly neccessary for dealing with DOS/Windows, and causes
    newlines (0x) to turn into the sequence (0x0x). **/
#define U8_STREAM_CRLFS	      0x800

/* These are bits which customize I/O handling/intent */

/** Whether to emit warnings for utf-8 errors **/
#define U8_INPUT_UTF8WARN     0x10000
#define U8_STREAM_UTF8WARN    U8_INPUT_UTF8WARN
/** Whether to generate hard errors for utf-8 errors **/
#define U8_INPUT_UTF8ERR     0x20000
#define U8_STREAM_UTF8ERR     U8_INPUT_UTF8ERR

/** Whether or not the stream should not be verbose. **/
#define U8_OUTPUT_TACITURN    0x10000
#define U8_STREAM_TACITURN    U8_OUTPUT_TACITURN
/** This bit indicates that the stream should be verbose. **/
#define U8_OUTPUT_VERBOSE     0x20000
#define U8_STREAM_VERBOSE     U8_OUTPUT_VERBOSE
/** This bit indicates that the stream is intended for humans to read **/
#define U8_HUMAN_OUTPUT       0x40000
#define U8_HUMANE_OUTPUT      U8_HUMAN_OUTPUT
#define U8_OUTPUT_HUMANE      U8_HUMAN_OUTPUT
#define U8_STREAM_HUMANE      U8_HUMAN_OUTPUT

#define U8_SUB_STREAM_MASK					\
  ( U8_STREAM_TACITURN | U8_STREAM_VERBOSE | U8_STREAM_HUMANE | U8_STREAM_TTY )

#define U8_STREAM_NEXT_FLAG 0x10000

#define U8_IO_STREAM_FLAGS 0xFF000000
#define U8_IO_STREAM_FLAGS 0xFF000000

#define U8_STREAM_FIELDS		      \
  int u8_bufsz; unsigned int u8_streaminfo;   \
  u8_byte *u8_strbuf, *u8_strptr, *u8_strlim; \
  void *u8_cfn; void *u8_xfn;		      \
  u8_string u8_typetag; void *u8_typedata

/** struct U8_STREAM is an abstract structural type which is extended
    by U8_INPUT and U8_OUTPUT.	The general layout of a stream structure is
    an integer buffer size, and an integer to store streaminfo bitwise.
    This is followed by three string pointers into a UTF-8 stream, either
    for input or output, and a pointer to a close function and a transfer (xfn).
**/
typedef struct U8_STREAM {U8_STREAM_FIELDS;} U8_STREAM;
typedef struct U8_STREAM *u8_stream;

U8_EXPORT
/* Increases the size of an output stream's buffer
   @param outstream a pointer to an input stream
   @param delta the minimum number of bytes to grow the stream
   @returns the size of the stream's new buffer or -1 if the operation
   failed
*/
ssize_t u8_grow_stream(struct U8_STREAM *stream,ssize_t delta);

#define U8_OUTPUT_FIELDS				       \
  /* Size of the buffer, and bits. */			       \
  int u8_bufsz, u8_streaminfo;				       \
  /* The buffer, where we are in it, and where it runs out. */ \
  u8_byte *u8_outbuf, *u8_write, *u8_outlim;		       \
  /* How we get more space */				       \
  int (*u8_closefn)(struct U8_OUTPUT *);		       \
  int (*u8_flushfn)(struct U8_OUTPUT *);		       \
  /* Type identifier */					       \
  u8_string u8_typetag;					       \
  /* Type data */					       \
  void *u8_typedata

/** struct U8_OUTPUT is a structural type which provides for UTF-8 output.
    This structure is subclassed by other structures which share its initial
    fields, allowing casting into the more general class which output functions
    operate over.
    At any point, the stream has at least one internal buffer of UTF-8
    characters, pointed to by u8_inbuf and with a current cursor of
    u8_read and a limit (the end of writable data) of u8_inlim.	 The size
    of the buffer is in u8_bufsz (note that this is redundant with u8_outlim)
    and various other bits are stored in u8_streaminfo.
    If an output operation overflows the buffer, the u8_flushfn (if non-NULL)
    is called on the stream.  If space is still not available, the
    output buffer is automatically grown.  Also provided is a u8_closefn
    which indicates that an application is done with a stream.
**/
typedef struct U8_OUTPUT {U8_OUTPUT_FIELDS;} U8_OUTPUT;
typedef struct U8_OUTPUT *u8_output;
typedef int (*u8_flushfn)(struct U8_OUTPUT *f);
typedef int (*u8_output_closefn)(struct U8_OUTPUT *f);

#define u8_outbuf_max(s) ((s)->u8_maxlen)
#define u8_outbuf_len(s) (((s)->u8_write)-((s)->u8_outbuf))
#define u8_outbuf_bytes(s) ((s)->u8_outbuf)
#define u8_outbuf_written(s) (((s)->u8_write)-((s)->u8_outbuf))
#define u8_outbuf_space(s) (((s)->u8_outlim)-((s)->u8_write))

/* Closes an input or output stream. */
U8_EXPORT int u8_close(U8_STREAM *stream);

U8_EXPORT int _u8_close_soutput(u8_output o);

/* u8_close_output:
     Arguments: out an output stream
     Returns: closes the stream, attempting to flush any available buffered data
*/
U8_EXPORT int u8_close_output(u8_output o);

/* u8_reset_output:
     Arguments: out an output stream
     Returns: ssize_t, the number of bytes available in the reset stream
 Discards the buffered output on the stream
*/
U8_EXPORT ssize_t u8_reset_output(u8_output out);

/** Allocates and opens an output string with an initial size.
    @param initial_size the initial space allocated for the stream
    @returns a u8_output stream
**/
U8_EXPORT U8_OUTPUT *u8_open_output_string(int initial_size);

#if U8_INLINE_IO
U8_INLINE_FCN U8_MAYBE_UNUSED
void U8_SETUP_OUTPUT(u8_output s,size_t sz,
		     unsigned char *buf,
		     unsigned char *write,
		     int flags);
/** Initializes a string output stream, with options.
    This allocates a buffer for the stream if @a buf is NULL.
    The can be passed a statically allocated buffer and
    will track whether it is still being used as the buffer grows.
    If *write* is NULL, it defaults to *buf*;
    A zero byte is deposited into *write to NUL-terminate the output;
    If you are defining a flushfn on a stream, you must do this after
    U8_SETUP_OUTPUT, since it initializes the flushfn to NULL.
    @param s a pointer to a U8_OUTPUT structure
    @param sz the number of bytes in the buffer
    @param buf a buffer pointer or NULL
    @param write NULL or a pointer within *buf*
    @param flags flags for the stream being initialized
    @returns void
**/
static void U8_SETUP_OUTPUT(u8_output s,size_t sz,
			    unsigned char *buf,
			    unsigned char *write,
			    int flags)
{
  assert(sz>0);
  if (flags<0)
    flags = ( ((u8_utf8warn)?(U8_STREAM_UTF8WARN):(0)) |
	      ((u8_utf8err)?(U8_STREAM_UTF8ERR):(0)) );
  flags |= U8_OUTPUT_STREAM;
  if (buf)
    (s)->u8_outbuf=buf;
  else {
    (s)->u8_outbuf=buf=u8_malloc(sz);
    flags |= U8_STREAM_OWNS_BUF;}
  if (write==NULL)
    write=buf;
  else if ( (write<buf) || (write>buf+sz) )
    write=buf;
  else NO_ELSE;
  (s)->u8_write=write;
  *write='\0';
  (s)->u8_outlim=(s)->u8_outbuf+sz;
  (s)->u8_bufsz=sz;
  (s)->u8_flushfn=NULL;
  (s)->u8_closefn=_u8_close_soutput;
  (s)->u8_streaminfo=flags;
}
#else
U8_EXPORT void _U8_SETUP_OUTPUT(u8_output s,size_t sz,
				unsigned char *buf,
				unsigned char *write,
				int flags);
#define U8_SETUP_OUTPUT(s,sz,buf,write,flags)	\
  _U8_SETUP_OUTPUT(s,sz,buf,write,flags)
#endif

static U8_MAYBE_UNUSED void U8_INIT_OUTPUT_X(u8_output s,size_t sz,
					     unsigned char *buf,
					     int flags)
{
  U8_SETUP_OUTPUT(s,sz,buf,buf,flags);
}

/** Initializes a string output stream with a particular initial size
    This always allocates a buffer but arranges for the buffer to grow
    @param s a pointer to a U8_OUTPUT structure
    @param sz the number of bytes in the buffer
    @returns void
**/
#define U8_INIT_OUTPUT(s,sz)			\
  U8_INIT_OUTPUT_X((s),sz,NULL,0);

/** Initializes a string output stream with a initial buffer.
    This will allocates a buffer if the output grows beyond the initial size.
    @param s a pointer to a U8_OUTPUT structure
    @param sz the number of bytes in the buffer
    @param buf a pointer to a byte/character array with at least @a sz elements
    @returns void
**/
#define U8_INIT_OUTPUT_BUF(s,sz,buf)		\
  U8_INIT_OUTPUT_X((s),sz,buf,0);

/** Initializes a string output stream with a particular initial size
    This always allocates a buffer but arranges for the buffer to grow
    @param s a pointer to a U8_OUTPUT structure
    @param sz the number of bytes in the buffer
    @returns void
**/
#define U8_INIT_STATIC_OUTPUT(s,sz)	\
  {memset((&s),0,sizeof(s));		\
    U8_INIT_OUTPUT_X((&s),sz,NULL,-1);}
#define U8_INIT_STATIC_OUTPUT_BUF(s,sz,buf)	\
  {memset((&s),0,sizeof(s));			\
    memset(buf,0,sz);				\
    U8_INIT_OUTPUT_X((&s),sz,buf,-1);}

/** U8_INIT_FIXED_OUTPUT
    Initializes a string output stream with a fixed size buffer
    This stream will discard content after the buffer is exhausted
    @param s a pointer to a U8_OUTPUT structure
    @param sz the number of bytes in the buffer
    @param buf a pointer to a byte buffer which must exist
    @returns void
**/
#define U8_INIT_FIXED_OUTPUT(s,sz,buf)		\
  U8_INIT_OUTPUT_X(s,sz,buf,U8_FIXED_STREAM)

#define U8_FIXED_OUTPUT_FLAGS (U8_OUTPUT_STREAM|U8_FIXED_STREAM)
#define U8_FIXED_OUTPUT(name,sz)			     \
  U8_MAYBE_UNUSED struct U8_OUTPUT name, *name ## out=&name;		\
  u8_byte _buf_ ## name[sz];						\
  U8_INIT_OUTPUT_X(&name,sz,_buf_ ## name,U8_FIXED_OUTPUT_FLAGS)

#define U8_STATIC_OUTPUT_FLAGS U8_OUTPUT_STREAM
#define U8_STATIC_OUTPUT(name,sz)			     \
  U8_MAYBE_UNUSED struct U8_OUTPUT name, *name ## out=&name;		\
  u8_byte _buf_ ## name[sz];						\
  U8_INIT_OUTPUT_X(&name,sz,_buf_ ## name,U8_STATIC_OUTPUT_FLAGS)

#define U8_SUB_STREAM(s,size,parent)					\
  U8_STATIC_OUTPUT(s,size);						\
  {if (parent)								\
      s.u8_streaminfo |=						\
	( ( (parent)->u8_streaminfo ) & (U8_SUB_STREAM_MASK) ); }

/** Returns the string content of the output stream. **/
#define u8_outstring(s) ((s)->u8_outbuf)
/** Returns the length in bytes of the string content of the output
    stream. **/
#define u8_outlen(s)	(((s)->u8_write)-((s)->u8_outbuf))

U8_EXPORT int _u8_putc(struct U8_OUTPUT *f,int c);
U8_EXPORT int _u8_putn(struct U8_OUTPUT *f,u8_string string,int len);
U8_EXPORT int _u8_output_needs(u8_output out,size_t n_bytes);

#if U8_INLINE_IO
/** Writes the unicode code point @a c to the stream @a f.
    The stream may buffer characters in an internal UTF-8
    representation.
    @param f an output stream
    @param c a unicode code point
    @returns the number of bytes written or -1 on an error
**/
U8_INLINE_FCN int u8_putc(struct U8_OUTPUT *f,int c)
{
  if (U8_EXPECT_FALSE(f->u8_write+1>=f->u8_outlim))
    return _u8_putc(f,c);
  else if (U8_EXPECT_TRUE((c<0x80)&&(c>0))) {
    *(f->u8_write++)=c;
    *(f->u8_write)='\0';
    return 1;}
  else return _u8_putc(f,c);
}

/** Writes @a n unicode code points from @a data to the stream @a f.
    The stream may buffer characters in an internal UTF-8
    representation.
    @param f an output stream
    @param data an UTF-8 encoded string
    @param len the number of bytes to be written from @a data
    @returns the number of bytes written or -1 on an error
**/
U8_INLINE_FCN int u8_putn(struct U8_OUTPUT *f,u8_string data,int len)
{
  if (U8_EXPECT_FALSE(len==0)) return 0;
  if (U8_EXPECT_FALSE(f->u8_write+len+1>=f->u8_outlim))
    return _u8_putn(f,data,len);
  else {
    memcpy(f->u8_write,data,len);
    f->u8_write=f->u8_write+len;
    *(f->u8_write)='\0';
    return len;}
}

U8_INLINE_FCN int u8_output_needs(u8_output out,size_t n_bytes)
{
  if (u8_outbuf_space(out)>=n_bytes)
    return 1;
  else if ( ( (out->u8_streaminfo) & (U8_FIXED_STREAM) ) &&
	    (out->u8_flushfn == NULL) )
    return 0;
  else return _u8_output_needs(out,n_bytes);
}
#else
#define u8_putc _u8_putc
#define u8_putn _u8_putn
#define u8_output_needs _u8_output_needs
#endif

#define u8_puts(f,s) _u8_putn(f,s,strlen(s))

U8_EXPORT
/* Doubles the size of an output stream's buffer, bounded by a maximum
   limit.
   @param outstream a pointer to an output stream
   @param max_size the maximum size for the buffer
   @returns the size of the stream's new buffer or -1 if the operation
   failed
*/
ssize_t u8_grow_output_stream(struct U8_OUTPUT *outstream,ssize_t to_size);

/* Input streams */

#define U8_INPUT_FIELDS						\
  /* How big the buffer is, and other info. */			\
  int u8_bufsz, u8_streaminfo;					\
  /* The buffer, the read point, and the end of valid data */	\
  u8_byte *u8_inbuf, *u8_read, *u8_inlim;			\
  /* The function we call to close the stream. */		\
  int (*u8_closefn)(struct U8_INPUT *);				\
  /* The function to get more data */				\
  int (*u8_fillfn)(struct U8_INPUT *);				\
  /* Type identifier */						\
  u8_string u8_typetag;						\
  /* Type data */						\
  void *u8_typedata


/** struct U8_INPUT
    is a structure used for stream-based UTF-8 input. This structure is
    subclassed by other structures which share its initial fields,
    allowing casting into the more general class which input functions
    operate over.
    At any point, the stream has at least one internal buffer of UTF-8
    characters, pointed to by u8_inbuf and with a current cursor of
    u8_read and a limit (the end of valid data) of u8_inlim.  The size
    of the buffer is in u8_bufsz and various other bits are stored in
    u8_streaminfo.  If an input operation needs more than the buffered
    data, the u8_fillfn is called on the stream, if non-NULL.  Also
    provided is a u8_closefn which is used whenever the application
    indicates that it is done with a stream.  **/
typedef struct U8_INPUT {U8_INPUT_FIELDS;} U8_INPUT;
typedef struct U8_INPUT *u8_input;

#define u8_inbuf_read(s) (((s)->u8_read)-((s)->u8_inbuf))
#define u8_inbuf_ready(s) (((s)->u8_inlim)-((s)->u8_read))

U8_EXPORT int _u8_close_sinput(u8_input i);
U8_EXPORT int u8_close_input(u8_input i);

/** Opens an input stream reading characters from the UTF-8 string @a input.
    This is the simplest kind of input stream and is malloc'd.
    @param input a null-terminated UTF-8 string
    @returns a pointer to a U8_INPUT stream
**/
U8_EXPORT U8_INPUT *u8_open_input_string(u8_string input);

#if U8_INLINE_IO
/** Initializes an input stream to take input from a string.
    The string contains a UTF-8 representation which can be
    read from the U8_INPUT stream codepoint by codepoint.
    @param s a pointer to a U8_INPUT stream
    @param n the size of the buffer to be read from
    @param buf a UTF-8 string
    @returns void
**/
U8_INLINE_FCN void U8_INIT_STRING_INPUT(u8_input s,int n,const u8_byte *buf)
{
  if (n<0) n=strlen(buf);
  (s)->u8_bufsz=n;
  (s)->u8_inbuf=(s)->u8_read=(u8_byte *)buf;
  (s)->u8_inlim=(u8_byte *)(buf+n);
  (s)->u8_closefn=_u8_close_sinput; (s)->u8_fillfn=NULL;
  (s)->u8_streaminfo=0;
}

/** Initializes an input stream with a particular (empty) buffer
    @param s a pointer to a U8_INPUT stream
    @param n the size of the buffer to be read from
    @param buf a buffer of bytes to use for saving input
    @param bits the logical OR of fields controlling the stream
    @returns void
**/
U8_INLINE_FCN void U8_INIT_INPUT_X(u8_input s,size_t n,u8_byte *buf,int bits)
{
  if (bits<0)
    bits= ( (-bits) |
	    ((buf) ? (0) : (U8_STREAM_OWNS_BUF))     |
	    ((u8_utf8warn)?(U8_STREAM_UTF8WARN):(0)) |
	    ((u8_utf8err)?(U8_STREAM_UTF8ERR):(0)) );
  else bits = ( bits |
		((buf) ? (0) : (U8_STREAM_OWNS_BUF)) );
  if (buf==NULL) {
    buf=u8_malloc(n);
    buf[0]='\0';}
  (s)->u8_bufsz=n;
  (s)->u8_inbuf=(s)->u8_read=buf;
  (s)->u8_inlim=buf;
  (s)->u8_closefn=_u8_close_sinput;
  (s)->u8_fillfn=NULL;
  (s)->u8_streaminfo=bits;
}
#else
U8_EXPORT void _U8_INIT_INPUT_X(u8_input s,size_t n,u8_byte *buf,int bits);
U8_EXPORT void _U8_INIT_STRING_INPUT(u8_input s,size_t n,const u8_byte *buf);
#define U8_INIT_STRING_INPUT _U8_INIT_STRING_INPUT
#define U8_INIT_INPUT_X _U8_INIT_INPUT_X
#endif

/** Initializes an input stream with a buffer of @a n bytes
    This allocates the buffer and sets its U8_OWNS_BUF bit.
    @param s a pointer to a U8_INPUT stream
    @param n the size of the buffer for the stream to use
    @returns void
**/
#define U8_INIT_INPUT(s,n) U8_INIT_INPUT_X(s,n,NULL,0)

typedef int (*u8_input_closefn)(struct U8_INPUT *f);
typedef int (*u8_fillfn)(struct U8_INPUT *f);

/* Helpful for bug reports */

U8_EXPORT
u8_string u8_get_input_context
(struct U8_INPUT *in,size_t n_before,size_t n_after,u8_string sep);
U8_EXPORT
u8_string u8_get_output_context(struct U8_OUTPUT *out,size_t n_before);

/* Core functions */

U8_EXPORT int _u8_getc(struct U8_INPUT *f);
U8_EXPORT int _u8_getn(u8_byte *ptr,int n,struct U8_INPUT *f);
/** Reads a string from @a f into @a buf up to the string @a eos.
    This stores the number of bytes read into @a sizep and returns
    a pointer to buf.  If there is not enough space in @a buf
    (which has @a len bytes), u8_gets_x returns NULL but
    deposits the number of bytes needed into @a sizep.
    If @a buf is NULL, this function allocates a new buffer/string
    with enough space to hold the requested data.
    The terminating sequence itself is not included in the result.
    @param buf an buffer/string of @a n bytes
    @param len the number of bytes available in @a buf
    @param f a pointer to a U8_INPUT stream
    @param eos a UTF-8 string indicating the "end of record"
    @param sizep a pointer to an int used to record how many bytes
    were read (or are needed)
    @returns a pointer to the buffer or results or NULL if the
    provided buffer was to small to contain the requested data.
**/
U8_EXPORT u8_string u8_gets_x
(u8_byte *buf,int len,struct U8_INPUT *f,u8_string eos,ssize_t *sizep);

/** Puts the character @a c back into the input stream @a f.
    This can be used by parsing algorithms which get a character, look
    at it and then put it back before calling another procedure.
    @param f a pointer to a U8_INPUT stream
    @param c the unicode code point last read from @a stream
    @returns -1 on error
**/
U8_EXPORT int u8_ungetc(struct U8_INPUT *f,int c);

/** Returns the next character to be read from @a f.
    This does not advance the buffer point but will
    try to fetch data if needed.  If there is a UTF-8
    parsing error, this either returns -2 or issues
    a warning and returns \\uFFFD, depending on whether
    the stream has its U8_STREAM_UTFERR bit set.
    @param f a pointer to a U8_INPUT stream
    @returns -1 on error
**/
U8_EXPORT int u8_probec(struct U8_INPUT *f);

/** Returns the next character to be read from @a f.
    This does not advance the buffer point and does not
    attempt to fill the buffer.
    @param f a pointer to a U8_INPUT stream
    @returns -1 on error
**/
U8_EXPORT int u8_peekc(struct U8_INPUT *f);

/** Returns a UTF-8 string from @a f terminated by @a eos or
    the end of the stream.  The terminating sequence itself
    is not included in the result.
    @param f a pointer to a U8_INPUT stream
    @param eos a string indicating the end of a record
    @returns a UTF-8 string pointer
**/
#define u8_getrec(f,eos) (u8_gets_x(NULL,0,f,eos,NULL))

/** Returns a UTF-8 string from @a f terminated by a newline
    the end of the stream.
    @param f a pointer to a U8_INPUT stream
    @returns a UTF-8 string pointer
**/
#define u8_gets(f) (u8_gets_x(NULL,0,f,"\n",NULL))

#if U8_INLINE_IO
/** Returns a single unicode code point from the stream @a f.
    @param f a pointer to a U8_INPUT stream
    @returns a unicode code point (an int)
**/
U8_INLINE_FCN int u8_getc(struct U8_INPUT *f)
{
  if (U8_EXPECT_FALSE(f->u8_read>=f->u8_inlim))
    return _u8_getc(f);
  else if (U8_EXPECT_TRUE(*(f->u8_read)<0x80))
    return *(f->u8_read++);
  else return _u8_getc(f);
}
/** Reads up to @a n bytes from @a f into @a ptr.
    @param ptr a to a byte array
    @param n the maximum number of bytes to read (at least the size of @a ptr)
    @param f a pointer to a U8_INPUT stream.
    @returns the number of bytes actually read
**/
U8_INLINE_FCN int u8_getn(u8_byte *ptr,int n,struct U8_INPUT *f)
{
  if (U8_EXPECT_FALSE(f->u8_read+n+1>=f->u8_inlim))
    return _u8_getn(ptr,n,f);
  else {
    int valid_len=u8_validate(f->u8_read,n);
    strncpy(ptr,f->u8_read,valid_len); ptr[valid_len]='\0';
    f->u8_read=f->u8_read+valid_len;
    return n;}
}
#else
#define u8_getc _u8_getc
#define u8_getn _u8_getn
#endif

U8_EXPORT
/* Doubles the size of an input stream's buffer bounded by a maximum
   limit
   @param instream a pointer to an input stream
   @param max_size the maximum size for the buffer
   @returns the size of the stream's new buffer or -1 if the operation
   failed
*/
ssize_t u8_grow_input_stream(struct U8_INPUT *in,ssize_t to_size);

/* Default output ports */

/** Sets the global output stream to @a out.  This is
    used as the default when a thread doesn't specify
    a default output stream.
    @param out a pointer to a U8_OUTPUT stream
**/
U8_EXPORT void u8_set_global_output(u8_output out);

/** Sets the default output stream (for the current thread) to @a out.
    @param out a pointer to a U8_OUTPUT stream
**/
U8_EXPORT void u8_set_default_output(u8_output out);

/** Resets the default output stream (for the current thread) to @a out.
    If @a out is the same as u8_global_output, this clears the thread-local
    output stream value (so that changes to u8_global_output will change
    this threads default output stream).
    @param out a pointer to a U8_OUTPUT stream
**/
U8_EXPORT void u8_reset_default_output(u8_output out);

/** Gets the default output stream for the current thread.
    This defaults to u8_global_output.
    @returns a pointer to a U8_OUTPUT structure
**/
U8_EXPORT U8_OUTPUT *u8_get_default_output(void);

/** This variable is the global output stream
    (a pointer to a U8_OUTPUT structure or equivalent)
**/
U8_EXPORT u8_output u8_global_output;

/** This macro (which looks like a variable) refers to the
    default output for the current thread or the global output
    (when no thread default has been defined).
**/
#define u8_current_output u8_global_output
#undef u8_current_output

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_default_output_key;
#define u8_current_output (u8_get_default_output())
#elif (U8_USE__THREAD)
U8_EXPORT __thread u8_output u8_default_output;
#define u8_current_output					\
  ((u8_default_output)?(u8_default_output):(u8_global_output))
#else
U8_EXPORT u8_output u8_default_output;
#define u8_current_output					\
  ((u8_default_output)?(u8_default_output):(u8_global_output))
#endif

/* Default input ports */

/** Sets the global input stream to @a out.  This is
    used as the default when a thread doesn't specify
    a default input stream.
    @param out a pointer to a U8_INPUT stream
**/
U8_EXPORT void u8_set_global_input(u8_input out);

/** Sets the default input stream (for the current thread) to @a out.
    @param out a pointer to a U8_INPUT stream
**/
U8_EXPORT void u8_set_default_input(u8_input out);

/** Resets the default input stream (for the current thread) to @a out.
    If @a out is the same as u8_global_input, this clears the thread-local
    input stream value (so that changes to u8_global_input will change
    this threads default input stream).
    @param out a pointer to a U8_INPUT stream
**/
U8_EXPORT void u8_reset_default_input(u8_input out);

/** Gets the default input stream for the current thread.
    This defaults to u8_global_input.
    @returns a pointer to a U8_INPUT structure
**/
U8_EXPORT U8_INPUT *u8_get_default_input(void);

/** This variable is the global input stream
    (a pointer to a U8_INPUT structure or equivalent)
**/
U8_EXPORT u8_input u8_global_input;

/** This macro (which looks like a variable) refers to the
    default input for the current thread or the global input
    (when no thread default has been defined).
**/
#define u8_current_input u8_global_input
#undef u8_current_input

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_default_input_key;
#define u8_current_input (u8_get_default_input())
#elif (U8_USE__THREAD)
U8_EXPORT __thread u8_input u8_default_input;
#define u8_current_input					\
  ((u8_default_input)?(u8_default_input):(u8_global_input))
#else
U8_EXPORT u8_input u8_default_input;
#define u8_current_input					\
  ((u8_default_input)?(u8_default_input):(u8_global_input))
#endif

/* Other functions */

/** Reads and interprets an XML character entity from @a in.
    @param in a pointer to a U8_INPUT stream positioned just after
    the ampersand (&) of an XML character entity
    @returns a unicode code point
**/
U8_EXPORT int u8_get_entity(U8_INPUT *in);

U8_EXPORT void u8_flush(struct U8_OUTPUT *);
U8_EXPORT void u8_flush_input(struct U8_INPUT *);

#endif /* LIBU8_U8STREAMIO_H */
