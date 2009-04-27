/* -*- Mode: C; -*- */

/* Copyright (C) 2004-2009 beingmeta, inc.
   This file is part of the libu8 UTF-8 unicode library.

   This program comes with absolutely NO WARRANTY, including implied
   warranties of merchantability or fitness for any particular
   purpose.

    Use, modification, and redistribution of this program is permitted
    under any of the licenses found in the the 'licenses' directory 
    accompanying this distribution, including the GNU General Public License
    (GPL) Version 2 or the GNU Lesser General Public License.
*/

#include "libu8/libu8.h"

static char versionid[] MAYBE_UNUSED=
  "$Id: libu8fns.c 3635 2009-04-22 02:45:30Z haase $";

U8_EXPORT void u8_init_timefns_c(void);
U8_EXPORT void u8_init_filefns_c(void);
U8_EXPORT void u8_init_netfns_c(void);
U8_EXPORT void u8_init_srvfns_c(void);

U8_EXPORT int u8_initialize_fns()
{
  u8_init_timefns_c();
  u8_init_filefns_c();
  u8_init_netfns_c();
  u8_init_srvfns_c();
  return 8069;
}


/* The CVS log for this file
   $Log: libu8fns.c,v $
   Revision 1.6  2005/04/07 01:57:36  haase
   Added file info primitives and subscription mechanism

   Revision 1.5  2005/03/11 14:40:55  haase
   Fix include references

   Revision 1.4  2005/02/16 02:29:36  haase
   Various fixes to get library init functions working

   Revision 1.3  2005/02/15 14:34:10  haase
   Fixed threadsafe access to strerror

   Revision 1.2  2005/02/12 03:38:47  haase
   Added copyrights and in-file CVS info


*/
