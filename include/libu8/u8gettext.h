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

#ifndef LIBU8_U8GETTEXT_H
#define LIBU8_U8GETTEXT_H 1
#define LIBU8_U8GETTEXT_H_VERSION __FILE__

/* GETTEXT */

#if ((defined(HAVE_GETTEXT))&&(HAVE_GETTEXT))
#define u8_gettext(d,x) ((d==NULL) ? (gettext(x)) : (dgettext(d,x)))
#else
#define u8_gettext(d,x) (x)
#endif

#if (!((defined(HAVE_TEXTDOMAIN))&&(HAVE_TEXTDOMAIN)))
#ifndef textdomain
#define textdomain(domain)
#endif
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN))&&(HAVE_BINDTEXTDOMAIN)))
#ifndef bindtextdomain
#define bindtextdomain(domain,dir)
#endif
#endif
#if (!((defined(HAVE_BINDTEXTDOMAIN_CODESET))&&(HAVE_BINDTEXTDOMAIN_CODESET)))
#ifndef bindtextdomain_codeset
#define bindtextdomain_codeset(domain,dir)
#endif
#endif

#define _(x) (x)
#define N_(x) x

#endif
