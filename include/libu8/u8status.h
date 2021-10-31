/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* Copyright (C) 2004-2019 beingmeta, inc.
   Copyright (C) 2020-2021 beingmeta, LLC
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

   Use, modification, and redistribution of this program is permitted
   under any of the licenses found in the the 'licenses' directory
   accompanying this distribution, including the GNU General Public License
   (GPL) Version 2 or the GNU Lesser General Public License.
*/

#ifndef LIBU8_U8STATUS_H
#define LIBU8_U8STATUS_H 1
#define LIBU8_U8STATUS_H_VERSION __FILE__

/* u8run state */

U8_EXPORT u8_string u8run_prefix;
U8_EXPORT u8_string u8run_jobid;
U8_EXPORT u8_string u8run_statfile;
U8_EXPORT u8_string u8run_status;

/** Sets the jobid and control file prefix for jobs started by u8run
    @param jobid a utf-8 string
    @param prefix a utf-8 string
    @returns void
**/
U8_EXPORT void u8run_setup(u8_string jobid,u8_string prefix,u8_string statfile);

/** Updates the u8run status for the current root process
    @param status a utf-8 string
    @returns int
**/
U8_EXPORT int u8run_set_status(u8_string status);

#endif
