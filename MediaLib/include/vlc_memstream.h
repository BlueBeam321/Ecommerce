/*****************************************************************************
 * vlc_memstream.h:
 *****************************************************************************/

#ifndef VLC_MEMSTREAM_H
# define VLC_MEMSTREAM_H 1

# include <stdarg.h>
# include <stdio.h>

struct vlc_memstream
{
    union
    {
        FILE *stream;
        int error;
    };
    char *ptr;
    size_t length;
};

VLC_API
int vlc_memstream_open(struct vlc_memstream *ms);

VLC_API
int vlc_memstream_flush(struct vlc_memstream *ms) VLC_USED;

VLC_API
int vlc_memstream_close(struct vlc_memstream *ms) VLC_USED;

VLC_API
size_t vlc_memstream_write(struct vlc_memstream *ms,
                           const void *ptr, size_t len);

VLC_API
int vlc_memstream_putc(struct vlc_memstream *ms, int c);

VLC_API
int vlc_memstream_puts(struct vlc_memstream *ms, const char *str);

VLC_API
int vlc_memstream_vprintf(struct vlc_memstream *ms, const char *fmt,
                          va_list args);

VLC_API
int vlc_memstream_printf(struct vlc_memstream *s, const char *fmt,
                         ...) VLC_FORMAT(2,3);

# ifdef __GNUC__
static inline int vlc_memstream_puts_len(struct vlc_memstream *ms,
                                         const char *str, size_t len)
{
    return (vlc_memstream_write(ms, str, len) == len) ? (int)len : EOF;
}
#  define vlc_memstream_puts(ms,s) \
    (__builtin_constant_p(__builtin_strlen(s)) ? \
        vlc_memstream_puts_len(ms,s,__builtin_strlen(s)) : \
        vlc_memstream_puts(ms,s))
# endif
#endif /* VLC_MEMSTREAM_H */
