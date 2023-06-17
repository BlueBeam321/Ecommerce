/*****************************************************************************
 * getdelim.c: POSIX getdelim() and getline() replacements
 *****************************************************************************/

#if defined(_WIN32) || defined(__ANDROID__)
#include <stdio.h>
#include <stdlib.h>

ssize_t getdelim (char** lineptr, size_t* n, int delimiter, FILE* stream)
{
    char *ptr = *lineptr;
    size_t size = (ptr != NULL) ? *n : 0;
    size_t len = 0;

    for (;;)
    {
        if ((size - len) <= 2)
        {
            size = size ? (size * 2) : 256;
            ptr = realloc (*lineptr, size);
            if (ptr == NULL)
                return -1;
            *lineptr = ptr;
            *n = size;
        }

        int c = fgetc (stream);
        if (c == -1)
        {
            if (len == 0 || ferror (stream))
                return -1;
            break; /* EOF */
        }
        ptr[len++] = c;
        if (c == delimiter)
            break;
    }

    ptr[len] = '\0';
    return len;
}

ssize_t getline (char** lineptr, size_t* n, FILE* stream)
{
    return getdelim (lineptr, n, '\n', stream);
}
#endif