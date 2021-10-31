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

#define U8_INLINE_CONTOURS 1

#include "libu8/u8source.h"
#include "libu8/libu8.h"

u8_condition u8_BadDynamicContour=_("Bad Dynamic Contour");

#if (U8_USE_TLS)
u8_tld_key u8_dynamic_contour_key;
#elif (U8_USE__THREAD)
__thread u8_contour u8_dynamic_contour;
#else
u8_contour u8_dynamic_contour;
#endif

U8_EXPORT void _u8_push_contour(u8_contour contour)
{
  u8_push_contour(contour);
}

U8_EXPORT void _u8_pop_contour(u8_contour contour)
{
  u8_pop_contour(contour);
}

U8_EXPORT void _u8_throw_contour(u8_contour contour)
{
  u8_throw_contour(contour);
}

/* Initialization function (just records source file info) */

void init_contour_c()
{
#if (U8_USE_TLS)
  u8_new_threadkey(&u8_dynamic_contour_key,NULL);
#endif
  u8_register_source_file(_FILEINFO);
}

/* Emacs local variables
   ;;;  Local variables: ***
   ;;;  compile-command: "make debugging;" ***
   ;;;  indent-tabs-mode: nil ***
   ;;;  End: ***
*/
