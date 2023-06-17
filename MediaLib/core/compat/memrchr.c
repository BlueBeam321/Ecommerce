
#ifdef _WIN32
#include <string.h>

/*
 * Reverse memchr()
 * Find the last occurrence of 'c' in the buffer 's' of size 'n'.
 */
void* memrchr(const void *s, int c, size_t n)
{
    const unsigned char *cp;

    if (n != 0) {
        cp = (unsigned char *)s + n;
        do {
            if (*(--cp) == (unsigned char)c)
                return((void *)cp);
        } while (--n != 0);
    }
    return(NULL);
}
#endif
