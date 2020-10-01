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

#ifndef LIBU8_U8EXCEPTIONS_H
#define LIBU8_U8EXCEPTIONS_H 1
#define LIBU8_U8EXCEPTIONS_H_VERSION __FILE__

/** \file u8exceptions.h
    These functions provide ways of managing exception state for
     libu8 and programs built on it.
 **/

typedef const unsigned char *u8_condition;
typedef const unsigned char *u8_context;

typedef void (*u8_exception_xdata_freefn)(void *);

U8_EXPORT u8_condition u8_strerror(int num);

/** struct U8_EXCEPTION
     represents an error condition.  The u8x_cond field
      contains the error condition, the u8x_context field contains
      the a string describing context in which the error
      occurred, the u8x_details field (if not null) provides
      more detailed information about the error.
     The u8x_xdata field, if not NULL, is a pointer to
      application-specific data which will be freed by
      the u8x_free_xdata function.
     Finally, the u8x_next field points to the error condition
      (or NULL) which was active when this error was indicated.
**/
typedef struct U8_EXCEPTION {
  u8_condition u8x_cond;
  u8_context u8x_context;
  double u8x_moment;
  long long u8x_thread;
  u8_string u8x_details;
  void *u8x_xdata;
  u8_exception_xdata_freefn u8x_free_xdata;
  struct U8_EXCEPTION *u8x_prev;} U8_EXCEPTION;
typedef struct U8_EXCEPTION *u8_exception;

#if (U8_USE_TLS)
U8_EXPORT u8_tld_key u8_current_exception_key;
#define u8_current_exception ((u8_exception)(u8_tld_get(u8_current_exception_key)))
#define u8_set_current_exception(ex) \
  ((u8_exception)(u8_tld_set(u8_current_exception_key,((void *)ex)),ex))
#elif (U8_USE__THREAD)
U8_EXPORT __thread u8_exception u8_current_exception;
#define u8_set_current_exception(ex) u8_current_exception=ex
#else
U8_EXPORT u8_exception u8_current_exception;
#define u8_set_current_exception(ex) u8_current_exception=ex
#endif

/** Creates a new exception
    Returns  a new exception exception, but *doesn't* push it
     onto the exception stack.
    The condition and context are constant strings, the details
     should be a mallocd string.  xdata and freefn can be NULL.
     if freefn is provided, it is called on xdata when the
     exception is popped.
   @param c a utf-8 condition string (u8_condition)
   @param cxt a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @param xdata a void pointer to additional data describing
                  the error or its context
   @param freefn a function to call on the xdata when freeing it
   @returns an exception structure
**/
U8_EXPORT U8_NOINLINE u8_exception u8_new_exception
  (u8_condition c,u8_context cxt,u8_string details,
   void *xdata,void (*freefn)(void *));

/** Pushes a new exception
    Sets the current exception, pushing it onto the dynamic
     exception stack.
    The condition and context are constant strings, the details
     should be a mallocd string.  xdata and freefn can be NULL.
     if freefn is provided, it is called on xdata when the
     exception is popped.
   @param condition a utf-8 condition string (u8_condition)
   @param context a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @param xdata a void pointer to additional data describing
                  the error or its context
   @param freefn a function to call on the xdata when freeing it
   @returns an exception structure
**/
U8_EXPORT U8_NOINLINE u8_exception u8_push_exception
  (u8_condition condition,u8_context context,u8_string details,
   void *xdata,void (*freefn)(void *));

/** Pushes an exception object
    Sets the current exception to *newex*
   @param newex a pointer to an exception object
   @returns an exception structure
**/
U8_EXPORT U8_NOINLINE u8_exception u8_expush(u8_exception newex);

/** Pops the exception stack
    Clears the current exception, popping it from the dynamic
     exception stack, which subsequently points to
     the exception which was current when the current exception
     was pushed.
   @returns an exception structure, the new current exception (possibly NULL)
**/
U8_EXPORT U8_NOINLINE u8_exception u8_pop_exception(void);

/** Returns the root of the current error state
     This climbs the u8x_prev hierachy and returns the
      exception at the top which (presumably) started it all.
     When passed NULL, this returns NULL
   @param ex an exception object
   @returns an exception object
**/
U8_EXPORT u8_exception u8_exception_root(u8_exception ex);

/** Returns the number of exceptions in the stack starting at ex
     This climbs the u8x_prev hierachy and counts the number of
      exceptions
     When passed NULL, this returns 0
   @param ex an exception object
   @returns a non-negative integer
**/
U8_EXPORT int u8_exception_stacklen(u8_exception ex);

/** Returns and clears the current error state.
     This retrieves and clears the current error state
      and the chain of errors beyond it.
   @returns an exception object
**/
U8_EXPORT u8_exception u8_erreify(void);

/** Restores the exception stack
    Takes an exception structure, typically returned by u8_erreify(),
     and makes it current.  If there is no current exception, the structure
     simply becomes the current exception; if there is a current exception,
     the argument is added as the previous exception to the root of the
     current exception.
   @param ex a pointer to an exception structure not currently on the stack
   @returns an exception structure, the new current exception (possibly NULL)
**/
U8_EXPORT u8_exception u8_restore_exception(u8_exception ex);

/** Creates an exception object
     Creates a new exception object but does not make it current.
    The condition and context are constant strings, the details
     should be a mallocd string.  xdata and freefn can be NULL.
     if freefn is provided, it is called on xdata when the
     exception is popped.
   @param condition a utf-8 condition string (u8_condition)
   @param context a utf-8 context string (a const string)
   @param moment a double indicating the elapsed time point
                   (in seconds) when the exception occurred
   @param thread a long long indicating the thread id current
                  when the exception was invoked
   @param details a utf-8 string detailing the error, or NULL
   @param xdata a void pointer to additional data describing
                  the error or its context
   @param freefn a function to call on the xdata when freeing it
   @returns an exception structure
**/
U8_EXPORT U8_NOINLINE u8_exception u8_make_exception
  (u8_condition condition,u8_context context,u8_string details,
   double moment,long long thread,
   void *xdata,void (*freefn)(void *));

/** Frees an exception object or stack
    This is implicitly called by u8_pop_exception, so it is not
     normally needed without an intervening call to u8_erreify().
    Frees its argument, an exception; with a second argument of 1,
     this recursively frees the whole exception stack.
    This returns the previous exception or NULL if the whole
     stack was freed.
   @param ex a pointer to an exception structure
   @param full 1/0 indicating whether to free the whole stack
   @returns an exception structure, or NULL
**/
U8_EXPORT u8_exception u8_free_exception(u8_exception ex,int full);

/** Sets the current error state and returns an error value (-1)
    The condition and context are constant strings, but the details
     string should be mallocd.
   @param c a utf-8 condition string (u8_condition)
   @param cxt a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @returns -1
**/
U8_EXPORT U8_NOINLINE int u8_reterr
   (u8_condition c,u8_context cxt,u8_string details);

/** Creates a new exception based on an existing one
     This creates a new exception whose previous pointer
     is an existing exception but provides new field.
   @returns an exception object
**/
U8_EXPORT U8_NOINLINE u8_exception u8_errpush
   (u8_exception ex,u8_context cxt,u8_string details,
    void *xdata,void (*freefn)(void *));

/** Clears all current errors.
    If report is non-zero, the errors are reported as messages
   @param report an integer
   @returns void
**/
U8_EXPORT void u8_clear_errors(int report);

/** Returns a UTF-8 string describing an error state
   @param errdata a pointer to a U8_ERRDATA structure
   @returns void
**/
U8_EXPORT u8_string u8_errstring(struct U8_EXCEPTION *errdata);

#define U8_CLEAR_ERR() errno=0

/** Returns a u8_condition object corresponding to a given errno,
     using the POSIX strerror function.
    @param num an errno value
    @returns a utf-8 condition object (a const string)
**/
U8_EXPORT u8_condition u8_strerror(int num);

/** Takes an errno error and creates a libu8 error, given a particular
    context and details string.
   @param num an errno number
   @param cxt a context string (a const utf-8 string)
   @param details a details string (a malloc'd utf-8 string)
**/
U8_EXPORT void u8_graberr(int num,u8_context cxt,u8_string details);

/** Takes the current errno value (if non zero) and creates a libu8 error,
      given a particular context and details string.
   @param cxt a context string (a const utf-8 string)
   @param details a details string (a malloc'd utf-8 string)
**/
#define u8_graberrno(cxt,details) \
  {int _saved_errno = errno; errno=0; u8_graberr(_saved_errno,cxt,details);}

/* Legacy functions */

/** Sets the current error state.
    The condition and context are constant strings, but the details
     string should be mallocd.
   @param condition a utf-8 condition string (u8_condition)
   @param context a utf-8 context string (a const string)
   @param details a utf-8 string detailing the error, or NULL
   @returns void
**/
U8_EXPORT U8_NOINLINE void u8_seterr(u8_condition condition,u8_context context,
				     u8_string details);

/** Gets the current error state.
   This retrieves the current error state and stores it in designated locations.
    A null pointer indicates that no value is to be stored.
   @param conditionp a pointer to a location to store the error condition
   @param contextp a pointer to a location to store the error context
   @param detailsp a pointer to a location to store a (copy of) the error details
   @returns 1 if there is a current error, 0 otherwise
**/
U8_EXPORT int u8_geterr
  (u8_condition *conditionp,u8_context *contextp,u8_string *detailsp);

/** Gets and clears the current error state.
   This retrieves the current error state and stores it in designated locations.
    A null pointer indicates that no value is to be stored.
   After retrieving it, the current error state is cleared.  Note that
    any error state existing before the error was signalled is then restored.
   @param conditionp a pointer to a location to store the error condition
   @param contextp a pointer to a location to store the error context
   @param detailsp a pointer to a location to store a (copy of) the error details
   @returns 1 if there is a current error, 0 otherwise
**/
U8_EXPORT int u8_poperr
  (u8_condition *conditionp,u8_context *contextp,u8_string *detailsp);

U8_EXPORT u8_condition u8_MallocFailed;
U8_EXPORT u8_condition u8_NullArg;
U8_EXPORT u8_condition u8_UnexpectedErrno;
U8_EXPORT u8_condition u8_NotImplemented;

#define u8err(rv,condition,context,details) \
  ( (u8_seterr(condition,context,details)) , (rv) )
#define u8err_x(rv,condition,context,details,data,freefn)	\
  ( (u8_push_exception(condition,context,details,data,freefn)) , (rv) )

#endif /* ndef LIBU8_U8EXCEPTIONS_H */
