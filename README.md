**libu8** is a portability and utility library written in modern C for
Unix-based platforms. It provides:

* string utilities for portably working with UTF-8 encodings

* stream-based text I/O using UTF-8 internally but allowing
  output to multiple encodings

* a client networking library for socket io, connection pools, and
simple hostname lookup;

* extensible and customizable logging functions (using `u8_printf`);

* wrappers for time-related functions allowing fine-grained times 
  and specification of arbitrary time zones;

* an exception handling library using `setjmp`/`longjmp` with
  unwinds and dynamic error catching;

* signal handlers for turning synchronous signals into exceptions;

* a server networking library for lightweight multi-threaded server
  implementation.

* an extensible printf (`u8_printf`)  function including output to strings;

* various hash and digest functions, including MD5, Google's cityhash,
  and various SHAx functions;

* cryptographic function wrappers using local libraries;

* a variety of file or URI path manipulation functions;

* a wrapper for rusage() resource system calls;

* wrappers for accessing file and directory contents and metadata;

* support for lookup up and interpreting named character entities;

**libu8** is especially designed for software which uses UTF-8
internally but may interact with applications and services employing
different character encodings.


