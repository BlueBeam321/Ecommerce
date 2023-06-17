/*****************************************************************************
 * Declarations for getopt_long()
 *****************************************************************************/

#ifndef VLC_GETOPT_H
#define VLC_GETOPT_H 1

typedef struct vlc_getopt_s
{
/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.  */

    char *arg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

    int ind;

/* Set to an option character which was unrecognized.  */

    int opt;

/* The next char to be scanned in the option-element
   in which the last option character we returned was found.
   This allows us to pick up the scan where we left off.

   If this is zero, or a null string, it means resume the scan
   by advancing to the next ARGV-element.  */

    char *nextchar;

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

    int first_nonopt;
    int last_nonopt;

} vlc_getopt_t;

/* Describe the long-named options requested by the application.
   The LONG_OPTIONS argument to getopt_long or getopt_long_only is a vector
   of `struct option' terminated by an element containing a name which is
   zero.

   The field `has_arg' is:
   false if the option does not take an argument,
   true if the option requires an argument.

   If the field `flag' is not NULL, it points to a variable that is set
   to the value given in the field `val' when the option is found, but
   left unchanged if the option is not found.

   To have a long-named option do something other than set an `int' to
   a compiled-in constant, such as set a value from `optarg', set the
   option's `flag' field to zero and its `val' field to a nonzero
   value (the equivalent single-letter option character, if there is
   one).  For long options that have a zero `flag' field, `getopt'
   returns the contents of the `val' field.  */

struct vlc_option
{
    const char *name;
    bool has_arg;
    int *flag;
    int val;
};

extern int vlc_getopt_long(int argc, char *const *argv, const char *shortopts,
                           const struct vlc_option *longopts, int *longind,
                           vlc_getopt_t * state);

#endif                /* VLC_GETOPT_H */
