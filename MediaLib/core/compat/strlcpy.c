/*****************************************************************************
 * strlcpy.c: BSD strlcpy() replacement
 *****************************************************************************/

#ifdef _WIN32
#include <stddef.h>

/**
 * Copy a string to a sized buffer. The result is always nul-terminated
 * (contrary to strncpy()).
 *
 * @param dest destination buffer
 * @param src string to be copied
 * @param len maximum number of characters to be copied plus one for the
 * terminating nul.
 *
 * @return strlen(src)
 */
size_t strlcpy (char *tgt, const char *src, size_t bufsize)
{
    size_t length;

    for (length = 1; (length < bufsize) && *src; length++)
        *tgt++ = *src++;

    if (bufsize)
        *tgt = '\0';

    while (*src++)
        length++;

    return length - 1;
}
#endif
