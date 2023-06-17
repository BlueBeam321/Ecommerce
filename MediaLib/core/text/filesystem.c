/*****************************************************************************
 * filesystem.c: Common file system helpers
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_fs.h>
#include <vlc_charset.h>
#include <assert.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32
#include <linux/uio.h>
#endif
//#include <unistd.h>

/**
 * Opens a FILE pointer.
 * @param filename file path, using UTF-8 encoding
 * @param mode fopen file open mode
 * @return NULL on error, an open FILE pointer on success.
 */
FILE *vlc_fopen (const char *filename, const char *mode)
{
    int rwflags = 0, oflags = 0;

    for (const char *ptr = mode; *ptr; ptr++)
    {
        switch (*ptr)
        {
            case 'r':
                rwflags = O_RDONLY;
                break;

            case 'a':
                rwflags = O_WRONLY;
                oflags |= O_CREAT | O_APPEND;
                break;

            case 'w':
                rwflags = O_WRONLY;
                oflags |= O_CREAT | O_TRUNC;
                break;

            case 'x':
                oflags |= O_EXCL;
                break;

            case '+':
                rwflags = O_RDWR;
                break;

#ifdef O_BINARY
            case 'b':
                oflags = (oflags & ~O_TEXT) | O_BINARY;
                break;

            case 't':
                oflags = (oflags & ~O_BINARY) | O_TEXT;
                break;
#endif
        }
    }

    int fd = vlc_open (filename, rwflags | oflags, 0666);
    if (fd == -1)
        return NULL;

    FILE *stream = fdopen (fd, mode);
    if (stream == NULL)
        vlc_close (fd);

    return stream;
}


static int dummy_select( const char *str )
{
    (void)str;
    return 1;
}

/**
 * Does the same as vlc_scandir(), but takes an open directory pointer
 * instead of a directory path.
 */
int vlc_loaddir( DIR *dir, char ***namelist,
                  int (*select)( const char * ),
                  int (*compar)( const char **, const char ** ) )
{
    assert (dir);

    if (select == NULL)
        select = dummy_select;

    char **tab = NULL;
    unsigned num = 0;

    rewinddir (dir);

    for (unsigned size = 0;;)
    {
        errno = 0;
        const char *entry = vlc_readdir (dir);
        if (entry == NULL)
        {
            if (errno)
                goto error;
            break;
        }

        if (!select (entry))
            continue;

        if (num >= size)
        {
            size = size ? (2 * size) : 16;
            char **newtab = realloc (tab, sizeof (*tab) * (size));

            if (unlikely(newtab == NULL))
                goto error;
            tab = newtab;
        }

        tab[num] = strdup(entry);
        if (likely(tab[num] != NULL))
            num++;
    }

    if (compar != NULL && num > 0)
        qsort (tab, num, sizeof (*tab),
               (int (*)( const void *, const void *))compar);
    *namelist = tab;
    return num;

error:
    for (unsigned i = 0; i < num; i++)
        free (tab[i]);
    free (tab);
    return -1;
}

/**
 * Selects file entries from a directory, as GNU C scandir().
 *
 * @param dirname UTF-8 diretory path
 * @param pointer [OUT] pointer set, on successful completion, to the address
 * of a table of UTF-8 filenames. All filenames must be freed with free().
 * The table itself must be freed with free() as well.
 *
 * @return How many file names were selected (possibly 0),
 * or -1 in case of error.
 */
int vlc_scandir( const char *dirname, char ***namelist,
                  int (*select)( const char * ),
                  int (*compar)( const char **, const char ** ) )
{
    DIR *dir = vlc_opendir (dirname);
    int val = -1;

    if (dir != NULL)
    {
        val = vlc_loaddir (dir, namelist, select, compar);
        closedir (dir);
    }
    return val;
}

#if defined (_WIN32) || defined (__OS2__)
# include <vlc_rand.h>

int vlc_mkstemp( char *template )
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const int i_digits = sizeof(digits)/sizeof(*digits) - 1;

    /* */
    assert( template );

    /* Check template validity */
    const size_t i_length = strlen( template );
    char *psz_rand = &template[i_length-6];

    if( i_length < 6 || strcmp( psz_rand, "XXXXXX" ) )
    {
        errno = EINVAL;
        return -1;
    }

    /* */
    for( int i = 0; i < 256; i++ )
    {
        /* Create a pseudo random file name */
        uint8_t pi_rand[6];

        vlc_rand_bytes( pi_rand, sizeof(pi_rand) );
        for( int j = 0; j < 6; j++ )
            psz_rand[j] = digits[pi_rand[j] % i_digits];

        /* */
        int fd = vlc_open( template, O_CREAT | O_EXCL | O_RDWR, 0600 );
        if( fd >= 0 )
            return fd;
        if( errno != EEXIST )
            return -1;
    }

    errno = EEXIST;
    return -1;
}
#endif

#ifdef _WIN32
static wchar_t *widen_path(const char *path)
{
    wchar_t *wpath;

    errno = 0;
    wpath = ToWide(path);
    if (wpath == NULL)
    {
        if (errno == 0)
            errno = ENOENT;
        return NULL;
    }
    return wpath;
}

#define CONVERT_PATH(path, wpath, err) \
    wchar_t *wpath = wide_path(path); \
    if (wpath == NULL) return (err)


int vlc_open(const char *filename, int flags, ...)
{
    int mode = 0;
    va_list ap;

    flags |= O_NOINHERIT; /* O_CLOEXEC */
    /* Defaults to binary mode */
    if ((flags & O_TEXT) == 0)
        flags |= O_BINARY;

    va_start(ap, flags);
    if (flags & O_CREAT)
    {
        int unixmode = va_arg(ap, int);
        if (unixmode & 0444)
            mode |= _S_IREAD;
        if (unixmode & 0222)
            mode |= _S_IWRITE;
    }
    va_end(ap);

    /*
    * open() cannot open files with non-“ANSI” characters on Windows.
    * We use _wopen() instead. Same thing for mkdir() and stat().
    */
    wchar_t *wpath = widen_path(filename);
    if (wpath == NULL)
        return -1;

    int fd = _wopen(wpath, flags, mode);
    free(wpath);
    return fd;
}

int vlc_openat(int dir, const char *filename, int flags, ...)
{
    (void)dir; (void)filename; (void)flags;
    errno = ENOSYS;
    return -1;
}

int vlc_memfd(void)
{
#if 0
    int fd, err;

    FILE *stream = tmpfile();
    if (stream == NULL)
        return -1;

    fd = vlc_dup(fileno(stream));
    err = errno;
    fclose(stream);
    errno = err;
    return fd;
#else /* Not currently used */
    errno = ENOSYS;
    return -1;
#endif
}

int vlc_close(int fd)
{
    return close(fd);
}

int vlc_mkdir(const char *dirname, mode_t mode)
{
    wchar_t *wpath = widen_path(dirname);
    if (wpath == NULL)
        return -1;

    int ret = _wmkdir(wpath);
    free(wpath);
    (void)mode;
    return ret;
}

char *vlc_getcwd(void)
{
    wchar_t *wdir = _wgetcwd(NULL, 0);
    if (wdir == NULL)
        return NULL;

    char *dir = FromWide(wdir);
    free(wdir);
    return dir;
}

/* Under Windows, these wrappers return the list of drive letters
* when called with an empty argument or just '\'. */
DIR *vlc_opendir(const char *dirname)
{
    wchar_t *wpath = widen_path(dirname);
    if (wpath == NULL)
        return NULL;

    vlc_DIR *p_dir = malloc(sizeof(*p_dir));
    if (unlikely(p_dir == NULL))
    {
        free(wpath);
        return NULL;
    }

    /* Special mode to list drive letters */
    if (wpath[0] == L'\0' || (wcscmp(wpath, L"\\") == 0))
    {
        free(wpath);
        p_dir->wdir = NULL;
        p_dir->u.drives = GetLogicalDrives();
        p_dir->entry = NULL;
        return (void *)p_dir;
    }

    assert(wpath[0]); // wpath[1] is defined
    p_dir->u.insert_dot_dot = !wcscmp(wpath + 1, L":\\");

    _WDIR *wdir = _wopendir(wpath);
    free(wpath);
    if (wdir == NULL)
    {
        free(p_dir);
        return NULL;
    }
    p_dir->wdir = wdir;
    p_dir->entry = NULL;
    return (void *)p_dir;
}

const char *vlc_readdir(DIR *dir)
{
    vlc_DIR *p_dir = (vlc_DIR *)dir;

    free(p_dir->entry);

    /* Drive letters mode */
    if (p_dir->wdir == NULL)
    {
        DWORD drives = p_dir->u.drives;
        if (drives == 0)
        {
            p_dir->entry = NULL;
            return NULL; /* end */
        }

        unsigned int i;
        for (i = 0; !(drives & 1); i++)
            drives >>= 1;
        p_dir->u.drives &= ~(1UL << i);
        assert(i < 26);

        if (asprintf(&p_dir->entry, "%c:\\", 'A' + i) == -1)
            p_dir->entry = NULL;
    }
    else if (p_dir->u.insert_dot_dot)
    {
        /* Adds "..", gruik! */
        p_dir->u.insert_dot_dot = false;
        p_dir->entry = strdup("..");
    }
    else
    {
        struct _wdirent *ent = _wreaddir(p_dir->wdir);
        p_dir->entry = (ent != NULL) ? FromWide(ent->d_name) : NULL;
    }
    return p_dir->entry;
}

int vlc_stat(const char *filename, struct stat *buf)
{
    wchar_t *wpath = widen_path(filename);
    if (wpath == NULL)
        return -1;

    int ret = _wstati64(wpath, buf);
    free(wpath);
    return ret;
}

int vlc_lstat(const char *filename, struct stat *buf)
{
    return vlc_stat(filename, buf);
}

int vlc_unlink(const char *filename)
{
    wchar_t *wpath = widen_path(filename);
    if (wpath == NULL)
        return -1;

    int ret = _wunlink(wpath);
    free(wpath);
    return ret;
}

int vlc_rename(const char *oldpath, const char *newpath)
{
    int ret = -1;

    wchar_t *wold = widen_path(oldpath), *wnew = widen_path(newpath);
    if (wold == NULL || wnew == NULL)
        goto out;

    if (_wrename(wold, wnew) && (errno == EACCES || errno == EEXIST))
    {   /* Windows does not allow atomic file replacement */
        if (_wremove(wnew))
        {
            errno = EACCES; /* restore errno */
            goto out;
        }
        if (_wrename(wold, wnew))
            goto out;
    }
    ret = 0;
out:
    free(wnew);
    free(wold);
    return ret;
}

int vlc_dup(int oldfd)
{
    int fd = dup(oldfd);
    if (fd != -1)
        setmode(fd, O_BINARY);
    return fd;
}

int vlc_pipe(int fds[2])
{
    return _pipe(fds, 32768, O_NOINHERIT | O_BINARY);
}

ssize_t vlc_write(int fd, const void *buf, size_t len)
{
    return write(fd, buf, len);
}

ssize_t vlc_writev(int fd, const struct iovec *iov, int count)
{
    vlc_assert_unreachable();
}

#include <vlc_network.h>

int vlc_socket(int pf, int type, int proto, bool nonblock)
{
    int fd = socket(pf, type, proto);
    if (fd == -1)
        return -1;

    if (nonblock)
        ioctlsocket(fd, FIONBIO, &(unsigned long){ 1 });
    return fd;
}

int vlc_socketpair(int pf, int type, int proto, int fds[2], bool nonblock)
{
    (void)pf; (void)type; (void)proto; (void)fds; (void)nonblock;
    errno = ENOSYS;
    return -1;
}

int vlc_accept(int lfd, struct sockaddr *addr, socklen_t *alen, bool nonblock)
{
    int fd = accept(lfd, addr, alen);
    if (fd != -1 && nonblock)
        ioctlsocket(fd, FIONBIO, &(unsigned long){ 1 });
    return fd;
}

FILE *vlc_win32_tmpfile(void)
{
    TCHAR tmp_path[MAX_PATH - 14];
    int i_ret = GetTempPath(MAX_PATH - 14, tmp_path);
    if (i_ret == 0)
        return NULL;

    TCHAR tmp_name[MAX_PATH];
    i_ret = GetTempFileName(tmp_path, TEXT("VLC"), 0, tmp_name);
    if (i_ret == 0)
        return NULL;

    HANDLE hFile = CreateFile(tmp_name,
        GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    int fd = _open_osfhandle((intptr_t)hFile, 0);
    if (fd == -1) {
        CloseHandle(hFile);
        return NULL;
    }

    FILE *stream = _fdopen(fd, "w+b");
    if (stream == NULL) {
        _close(fd);
        return NULL;
    }
    return stream;
}
#else
#if !defined(HAVE_ACCEPT4) || !defined HAVE_MKOSTEMP
static inline void vlc_cloexec(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC | fcntl(fd, F_GETFD));
}
#endif

int vlc_open(const char *filename, int flags, ...)
{
    unsigned int mode = 0;
    va_list ap;

    va_start(ap, flags);
    if (flags & (O_CREAT))
        mode = va_arg(ap, unsigned int);
    va_end(ap);

#ifdef O_CLOEXEC
    return open(filename, flags | O_CLOEXEC, mode);
#else
    int fd = open(filename, flags, mode);
    if (fd != -1)
        vlc_cloexec(fd);
    return -1;
#endif
}

int vlc_openat(int dir, const char *filename, int flags, ...)
{
    unsigned int mode = 0;
    va_list ap;

    va_start(ap, flags);
    if (flags & (O_CREAT))
        mode = va_arg(ap, unsigned int);
    va_end(ap);

#ifdef HAVE_OPENAT
    return openat(dir, filename, flags | O_CLOEXEC, mode);
#else
    VLC_UNUSED(dir);
    VLC_UNUSED(filename);
    VLC_UNUSED(mode);
    errno = ENOSYS;
    return -1;
#endif
}

int vlc_mkstemp(char *template)
{
#if defined (HAVE_MKOSTEMP) && defined (O_CLOEXEC)
    return mkostemp(template, O_CLOEXEC);
#else
    int fd = mkstemp(template);
    if (fd != -1)
        vlc_cloexec(fd);
    return fd;
#endif
}

int vlc_memfd(void)
{
    int fd;
#if O_TMPFILE
    fd = vlc_open("/tmp", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd != -1)
        return fd;
    /* ENOENT means either /tmp is missing (!) or the kernel does not support
    * O_TMPFILE. EISDIR means /tmp exists but the kernel does not support
    * O_TMPFILE. EOPNOTSUPP means the kernel supports O_TMPFILE but the /tmp
    * filesystem does not. Do not fallback on other errors. */
    if (errno != ENOENT && errno != EISDIR && errno != EOPNOTSUPP)
        return -1;
#endif

    char bufpath[] = "/tmp/""vlc_package""XXXXXX";

    fd = vlc_mkstemp(bufpath);
    if (fd != -1)
        unlink(bufpath);
    return fd;
}

int vlc_close(int fd)
{
    int ret;
#ifdef POSIX_CLOSE_RESTART
    ret = posix_close(fd, 0);
#else
    ret = close(fd);
    /* POSIX.2008 (and earlier) does not specify if the file descriptor is
    * closed on failure. Assume it is as on Linux and most other common OSes.
    * Also emulate the correct error code as per newer POSIX versions. */
    if (unlikely(ret != 0) && unlikely(errno == EINTR))
        errno = EINPROGRESS;
#endif
    assert(ret == 0 || errno != EBADF); /* something is corrupt? */
    return ret;
}

int vlc_mkdir(const char *dirname, mode_t mode)
{
    return mkdir(dirname, mode);
}

DIR *vlc_opendir(const char *dirname)
{
    return opendir(dirname);
}

const char *vlc_readdir(DIR *dir)
{
    struct dirent *ent = readdir(dir);
    return (ent != NULL) ? ent->d_name : NULL;
}

int vlc_stat(const char *filename, struct stat *buf)
{
    return stat(filename, buf);
}

int vlc_lstat(const char *filename, struct stat *buf)
{
    return lstat(filename, buf);
}

int vlc_unlink(const char *filename)
{
    return unlink(filename);
}

int vlc_rename(const char *oldpath, const char *newpath)
{
    return rename(oldpath, newpath);
}

char *vlc_getcwd(void)
{
    long path_max = pathconf(".", _PC_PATH_MAX);
    size_t size = (path_max == -1 || path_max > 4096) ? 4096 : path_max;

    for (;; size *= 2)
    {
        char *buf = malloc(size);
        if (unlikely(buf == NULL))
            break;

        if (getcwd(buf, size) != NULL)
            return buf;
        free(buf);

        if (errno != ERANGE)
            break;
    }
    return NULL;
}

int vlc_dup(int oldfd)
{
#ifdef F_DUPFD_CLOEXEC
    return fcntl(oldfd, F_DUPFD_CLOEXEC, 0);
#else
    int newfd = dup(oldfd);
    if (newfd != -1)
        vlc_cloexec(oldfd);
    return newfd;
#endif
}

int vlc_pipe(int fds[2])
{
#ifdef HAVE_PIPE2
    return pipe2(fds, O_CLOEXEC);
#else
    int ret = pipe(fds);
    if (ret == 0)
    {
        vlc_cloexec(fds[0]);
        vlc_cloexec(fds[1]);
    }
    return ret;
#endif
}

ssize_t vlc_write(int fd, const void *buf, size_t len)
{
    struct iovec iov = { .iov_base = (void *)buf, .iov_len = len };

    return vlc_writev(fd, &iov, 1);
}

ssize_t vlc_writev(int fd, const struct iovec *iov, int count)
{
    sigset_t set, oset;

    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, &oset);

    ssize_t val = writev(fd, iov, count);
    if (val < 0 && errno == EPIPE)
    {
#if (_POSIX_REALTIME_SIGNALS > 0)
        siginfo_t info;
        struct timespec ts = { 0, 0 };

        while (sigtimedwait(&set, &info, &ts) >= 0 || errno != EAGAIN);
#else
        for (;;)
        {
            sigset_t s;
            int num;

            sigpending(&s);
            if (!sigismember(&s, SIGPIPE))
                break;

            sigwait(&set, &num);
            assert(num == SIGPIPE);
        }
#endif
    }

    if (!sigismember(&oset, SIGPIPE)) /* Restore the signal mask if changed */
        pthread_sigmask(SIG_SETMASK, &oset, NULL);
    return val;
}

#include <vlc_network.h>

#ifndef HAVE_ACCEPT4
static void vlc_socket_setup(int fd, bool nonblock)
{
    vlc_cloexec(fd);

    if (nonblock)
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

#ifdef SO_NOSIGPIPE
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof(int));
#endif
}
#endif

int vlc_socket(int pf, int type, int proto, bool nonblock)
{
#ifdef SOCK_CLOEXEC
    if (nonblock)
        type |= SOCK_NONBLOCK;

    int fd = socket(pf, type | SOCK_CLOEXEC, proto);
# ifdef SO_NOSIGPIPE
    if (fd != -1)
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof(int));
# endif
#else
    int fd = socket(pf, type, proto);
    if (fd != -1)
        vlc_socket_setup(fd, nonblock);
#endif
    return fd;
}

int vlc_socketpair(int pf, int type, int proto, int fds[2], bool nonblock)
{
#ifdef SOCK_CLOEXEC
    if (nonblock)
        type |= SOCK_NONBLOCK;

    int ret = socketpair(pf, type | SOCK_CLOEXEC, proto, fds);
# ifdef SO_NOSIGPIPE
    if (ret == 0)
    {
        const int val = 1;

        setsockopt(fds[0], SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
        setsockopt(fds[1], SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    }
# endif
#else
    int ret = socketpair(pf, type, proto, fds);
    if (ret == 0)
    {
        vlc_socket_setup(fds[0], nonblock);
        vlc_socket_setup(fds[1], nonblock);
    }
#endif
    return ret;
}

int vlc_accept(int lfd, struct sockaddr *addr, socklen_t *alen, bool nonblock)
{
#ifdef HAVE_ACCEPT4
    int flags = SOCK_CLOEXEC;
    if (nonblock)
        flags |= SOCK_NONBLOCK;

    int fd = accept4(lfd, addr, alen, flags);
# ifdef SO_NOSIGPIPE
    if (fd != -1)
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof(int));
# endif
#else
    int fd = accept(lfd, addr, alen);
    if (fd != -1)
        vlc_socket_setup(fd, nonblock);
#endif
    return fd;
}
#endif
