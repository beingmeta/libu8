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

#ifndef LIBU8_U8SUBSCRIBE_H
#define LIBU8_U8SUBSCRIBE_H 1
#define LIBU8_U8SUBSCRIBE_H_VERSION __FILE__

/* Subscribing to files */

/** The subscription structure maps filenames to ifchanged functions. **/
typedef struct U8_SUBSCRIPTION {
  char *filename; /* the locally encoded filename subscribed to. */
  time_t mtime; /* the modification of the time when last processes */
  /* function to call when the subscription needs to be renewed. */
  int (*callback)(u8_string,void *);
  /* the subscription data passed to the callback function */
  void *callback_data;
  /* the next node in the subscription table */
  struct U8_SUBSCRIPTION *next;}
  U8_SUBSCRIPTION;
/** A pointer to a subscription structure. **/
typedef struct U8_SUBSCRIPTION *u8_subscription;

/** Arranges for an action when a file changes.
    Creates a *subscription* which will call fn(filename,data) both
    initially and whenever it notices that filename has changed.
    The function u8_renew checks a particular subscription and the function
    u8_renew_all checks all subscriptions.
    @param filename a utf-8 pathname
    @param fn a function on a string and a pointer
    @param data data passed as a second argument to fn
    @returns a pointer to a U8_SUBSCRIPTION struct
**/
U8_EXPORT u8_subscription u8_subscribe
(u8_string filename,int (*fn)(u8_string,void *),void *data);

/** Calls a function on a file if it has changed.
    Renews a subscription created by u8_subscribe, calling
    the associated function if the corresponding file has
    changed.
    Returns -1 if the modification time can't be determined,
    0 if the file hasn't changed, and the return value
    of the subscription function otherwise.
    @param s a u8_subscription
    @returns int
**/
U8_EXPORT int u8_renew(u8_subscription s);

/** Renews all the subscriptions registered by u8_subscribe.
    @returns int
**/
U8_EXPORT int u8_renew_all(void);


#endif
