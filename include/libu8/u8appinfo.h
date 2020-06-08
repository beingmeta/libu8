/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8APPINFO_H
#define LIBU8_U8APPINFO_H 1
#define LIBU8_U8APPINFO_H_VERSION __FILE__

/* Application/process identifying information */

/** Returns a string identifying the current process and thread
    @param buf a buffer to use (mallocd otherwise)
    @returns a character string
**/
U8_EXPORT char *u8_procinfo(char *buf);

/** Returns a UTF-8 string describing the current application for inclusion
     in messages or other output or logging.
    This string is not consed, so it should not be freed.
    @returns a utf-8 string
**/
U8_EXPORT u8_string u8_appid(void);
/** Sets the UTF-8 string describing the current application.
    @param id a utf-8 string
    @returns void
**/
U8_EXPORT void u8_identify_application(u8_string id);
/** Sets the UTF-8 string describing the current application, unless
     it has already been set.  This can be used to provide default appids
     while allowing programs to override the default.
    This returns 1 if it did anything (no appid had been previously set) or
      zero otherwise.
    @param id a utf-8 string
    @returns int
**/
U8_EXPORT int u8_default_appid(u8_string id);

/* Version/revision information */

U8_EXPORT u8_string u8_revision, u8_version;
U8_EXPORT int u8_major, u8_minor, u8_release;

U8_EXPORT
/** Returns the current libu8 revision string (derived from the GIT branch)
    @returns a utf8 string
**/
u8_string u8_getrevision(void);

U8_EXPORT
/** Returns the current libu8 version string (major.minor.release)
    @returns a utf8 string
**/
u8_string u8_getversion(void);

U8_EXPORT
/** Returns the current libu8 major version number
    @returns an int
**/
int u8_getmajorversion(void);

/** Gets a variable specified in the environment.
    @param envvar a variable name
    @returns a utf-8 string, copied
**/
U8_EXPORT u8_string u8_getenv(u8_string envvar);

/** Gets and parses (as a number), an environment variable
    @param envvar a variable name
    @param dflt a default value if the environment variable isn't defined or can't be parsed as a number
    @returns a long long
**/
U8_EXPORT long long u8_getenv_int(u8_string envvar,long long dflt);

/** Gets and parses (as a number), an environment variable
    @param envvar a variable name
    @param dflt a default value if the environment variable isn't defined or can't be parsed as a number
    @returns a long long
**/
U8_EXPORT double u8_getenv_float(u8_string envvar,double dflt);

U8_EXPORT
/** Sets a variable i the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @param setval a string to set
    @param overwrite whether to replace an existing value
    @returns 1 if anything was done, 0 if not, -1 on error
**/
int u8_setenv(u8_string envvar,u8_string setval,int overwrite);

U8_EXPORT
/** Sets a variable i the current environment.
    @param envvar the variable name, as a UTF-8 string.
    @returns 1 if anything was done, 0 if not, -1 on error
**/
int u8_unsetenv(u8_string envvar);

#endif
