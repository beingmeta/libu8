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
  "$Id: libu8io.c 3635 2009-04-22 02:45:30Z haase $";

#include "libu8/libu8io.h"

U8_EXPORT void u8_init_convert_c(void);
U8_EXPORT void u8_init_xfiles_c(void);

static int io_init_done=0;

U8_EXPORT int u8_initialize_io()
{
  if (io_init_done) return io_init_done;
  else io_init_done=8069;
  u8_init_convert_c();
  u8_init_xfiles_c();
  return io_init_done;
}


/* The CVS log for this file
   $Log: libu8io.c,v $
   Revision 1.5  2005/03/11 14:40:55  haase
   Fix include references

   Revision 1.4  2005/02/22 16:41:41  haase
   Added automatic flushing and closing of xfiles

   Revision 1.3  2005/02/16 02:29:36  haase
   Various fixes to get library init functions working

   Revision 1.2  2005/02/12 03:38:47  haase
   Added copyrights and in-file CVS info


*/
