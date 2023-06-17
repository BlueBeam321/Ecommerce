/*****************************************************************************
 * vlc_fixups.h: portability fix up included from config.h
 *****************************************************************************/

/**
 * \file
 * This file is a collection of portability fixes
 */

#ifndef LIBVLC_FIXUPS_H
#define LIBVLC_FIXUPS_H

#if defined( _WIN32 )
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

//#ifdef _DEBUG
//#ifndef DBG_NEW
//#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
//#define new DBG_NEW   
//#endif
//#endif

#include <winsock2.h>
#include <Ws2tcpip.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <windows.h>

#ifndef SHUT_RD
#define SHUT_RD    SD_RECEIVE
#endif
#ifndef SHUT_WR
#define SHUT_WR    SD_SEND
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR  SD_BOTH
#endif

#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifndef __cplusplus
#if defined(__ANDROID__)
#define thread_local    _Thread_local
#elif defined(_WIN32)
#define thread_local    __declspec(thread)
#else
#define thread_local
#endif
# endif

# include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
# include <stddef.h>
# include <stdio.h>
# include <stdarg.h>
# include <sys/types.h>
# include <dirent.h>

#ifdef __cplusplus
# define VLC_NOTHROW throw ()
extern "C" {
#else
# define VLC_NOTHROW
#endif

/* stddef.h */
#if !defined (__cplusplus) && !defined (HAVE_MAX_ALIGN_T)
typedef struct {
  long long ll;
  long double ld;
} max_align_t;
#endif

ssize_t getdelim (char **, size_t *, int, FILE *);
ssize_t getline (char **, size_t *, FILE *);
int asprintf (char **, const char *, ...);
int vasprintf (char **, const char *, va_list);

int ffsll(long long);
void *memrchr(const void *, int, size_t);

int strcasecmp (const char *, const char *);
char *strcasestr (const char *, const char *);
int strverscmp (const char *, const char *);
char * strnstr (const char *, const char *, size_t);
char *strndup (const char *, size_t);
size_t strlcpy (char *, const char *, size_t);
char *strsep (char **, const char *);
char *strtok_r(char *, const char *, char **);

struct tm *gmtime_r (const time_t *, struct tm *);
struct tm *localtime_r (const time_t *, struct tm *);
time_t timegm(struct tm *);

#define TIME_UTC 1

#ifdef _WIN32
struct timespec {
    time_t  tv_sec;   /* Seconds */
    long    tv_nsec;  /* Nanoseconds */
};
#endif

int timespec_get(struct timespec *, int);

#ifdef _WIN32
struct timezone;
int gettimeofday(struct timeval *, struct timezone *);
#endif

#ifndef _WIN32
int fsync (int fd);
long pathconf (const char *path, int name);
#endif

/* dirent.h */
int dirfd(DIR *);
DIR *fdopendir (int);

#ifdef __cplusplus
} /* extern "C" */
#endif

void *aligned_alloc(size_t, size_t);

#if defined (_WIN32) && defined(_MSC_VER)
#define aligned_free(ptr)  _aligned_free(ptr)
#else
#define aligned_free(ptr)  free(ptr)
#endif

/* locale.h */
# ifndef HAVE_NEWLOCALE
#  define LC_ALL_MASK      0
#  define LC_NUMERIC_MASK  0
#  define LC_MESSAGES_MASK 0
#  define LC_GLOBAL_LOCALE ((locale_t)(uintptr_t)1)
typedef void *locale_t;

static inline void freelocale(locale_t loc)
{
    (void)loc;
}
static inline locale_t newlocale(int mask, const char * locale, locale_t base)
{
    (void)mask; (void)locale; (void)base;
    return NULL;
}
# else
#  include <locale.h>
# endif

static inline locale_t uselocale(locale_t loc)
{
    (void)loc;
    return NULL;
}

#define _(str)            vlc_gettext (str)
#define N_(str)           gettext_noop (str)
#define gettext_noop(str) (str)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
void swab (const void *, void *, ssize_t);
#endif

#ifdef _WIN32
struct pollfd;
int poll(struct pollfd *, unsigned, int);
#endif

#include <errno.h>
struct if_nameindex
{
    unsigned if_index;
    char    *if_name;
};
//#define if_nametoindex(name)   atoi(name)
//#define if_nameindex()         (errno = ENOBUFS, NULL)
//#define if_freenameindex(list) (void)0

#ifdef _WIN32
struct iovec
{
    void  *iov_base;
    size_t iov_len;
};
#define IOV_MAX 255
struct msghdr
{
    void         *msg_name;
    size_t        msg_namelen;
    struct iovec *msg_iov;
    size_t        msg_iovlen;
    void         *msg_control;
    size_t        msg_controllen;
    int           msg_flags;
};
#endif

typedef struct entry {
    char *key;
    void *data;
} ENTRY;

typedef enum {
    FIND, ENTER
} ACTION;

typedef enum {
    preorder,
    postorder,
    endorder,
    leaf
} VISIT;

void *tsearch( const void *key, void **rootp, int(*cmp)(const void *, const void *) );
void *tfind( const void *key, const void **rootp, int(*cmp)(const void *, const void *) );
void *tdelete( const void *key, void **rootp, int(*cmp)(const void *, const void *) );
void twalk( const void *root, void(*action)(const void *nodep, VISIT which, int depth) );
void tdestroy( void *root, void (*free_node)(void *nodep) );

double erand48 (unsigned short subi[3]);
long jrand48 (unsigned short subi[3]);
long nrand48 (unsigned short subi[3]);

#ifdef __OS2__
# undef HAVE_FORK   /* Implementation of fork() is imperfect on OS/2 */

# define SHUT_RD    0
# define SHUT_WR    1
# define SHUT_RDWR  2

/* GAI error codes */
# ifndef EAI_BADFLAGS
#  define EAI_BADFLAGS -1
# endif
# ifndef EAI_NONAME
#  define EAI_NONAME -2
# endif
# ifndef EAI_AGAIN
#  define EAI_AGAIN -3
# endif
# ifndef EAI_FAIL
#  define EAI_FAIL -4
# endif
# ifndef EAI_NODATA
#  define EAI_NODATA -5
# endif
# ifndef EAI_FAMILY
#  define EAI_FAMILY -6
# endif
# ifndef EAI_SOCKTYPE
#  define EAI_SOCKTYPE -7
# endif
# ifndef EAI_SERVICE
#  define EAI_SERVICE -8
# endif
# ifndef EAI_ADDRFAMILY
#  define EAI_ADDRFAMILY -9
# endif
# ifndef EAI_MEMORY
#  define EAI_MEMORY -10
# endif
# ifndef EAI_OVERFLOW
#  define EAI_OVERFLOW -11
# endif
# ifndef EAI_SYSTEM
#  define EAI_SYSTEM -12
# endif

# ifndef NI_NUMERICHOST
#  define NI_NUMERICHOST 0x01
#  define NI_NUMERICSERV 0x02
#  define NI_NOFQDN      0x04
#  define NI_NAMEREQD    0x08
#  define NI_DGRAM       0x10
# endif

# ifndef NI_MAXHOST
#  define NI_MAXHOST 1025
#  define NI_MAXSERV 32
# endif

# define AI_PASSIVE     1
# define AI_CANONNAME   2
# define AI_NUMERICHOST 4

struct addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

const char *gai_strerror (int);

int  getaddrinfo  (const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo (struct addrinfo *res);
int  getnameinfo  (const struct sockaddr *sa, socklen_t salen,
                   char *host, int hostlen, char *serv, int servlen,
                   int flags);

/* OS/2 does not support IPv6, yet. But declare these only for compilation */
# include <stdint.h>

struct in6_addr
{
    uint8_t s6_addr[16];
};

struct sockaddr_in6
{
    uint8_t         sin6_len;
    uint8_t         sin6_family;
    uint16_t        sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

# define IN6_IS_ADDR_MULTICAST(a)   (((__const uint8_t *) (a))[0] == 0xff)

static const struct in6_addr in6addr_any =
    { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

# include <errno.h>
# ifndef EPROTO
#  define EPROTO (ELAST + 1)
# endif

# ifndef HAVE_IF_NAMETOINDEX
#  define if_nametoindex(name)  atoi(name)
# endif
#endif

void sincos(double, double *, double *);
void sincosf(float, float *, float *);

char *realpath(const char*  pathname, char*  resolved_path);

#ifdef _WIN32
FILE *vlc_win32_tmpfile(void);
#endif

#if defined(_WIN32) && defined(__MINGW64_VERSION_MAJOR)
# define IN6_IS_ADDR_MULTICAST IN6_IS_ADDR_MULTICAST
#endif

#if defined(__APPLE__) || defined(_WIN32)
# define fdatasync fsync
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !LIBVLC_FIXUPS_H */
