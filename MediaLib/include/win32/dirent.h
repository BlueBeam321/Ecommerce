/*
 * dirent.h - dirent API for Microsoft Visual Studio
 *
 * Copyright (C) 2006-2012 Toni Ronkko
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * ``Software''), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL TONI RONKKO BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Version 1.13, Dec 12 2012, Toni Ronkko
 * Use traditional 8+3 file name if the name cannot be represented in the
 * default ANSI code page.  Now compiles again with MSVC 6.0.  Thanks to
 * Konstantin Khomoutov for testing.
 *
 * Version 1.12.1, Oct 1 2012, Toni Ronkko
 * Bug fix: renamed wide-character DIR structure _wDIR to _WDIR (with
 * capital W) in order to maintain compatibility with MingW.
 *
 * Version 1.12, Sep 30 2012, Toni Ronkko
 * Define PATH_MAX and NAME_MAX.  Added wide-character variants _wDIR, 
 * _wdirent, _wopendir(), _wreaddir(), _wclosedir() and _wrewinddir().
 * Thanks to Edgar Buerkle and Jan Nijtmans for ideas and code.
 *
 * Do not include windows.h.  This allows dirent.h to be integrated more
 * easily into programs using winsock.  Thanks to Fernando Azaldegui.
 *
 * Version 1.11, Mar 15, 2011, Toni Ronkko
 * Defined FILE_ATTRIBUTE_DEVICE for MSVC 6.0.
 *
 * Version 1.10, Aug 11, 2010, Toni Ronkko
 * Added d_type and d_namlen fields to dirent structure.  The former is
 * especially useful for determining whether directory entry represents a
 * file or a directory.  For more information, see
 * http://www.delorie.com/gnu/docs/glibc/libc_270.html
 *
 * Improved conformance to the standards.  For example, errno is now set
 * properly on failure and assert() is never used.  Thanks to Peter Brockam
 * for suggestions.
 *
 * Fixed a bug in rewinddir(): when using relative directory names, change
 * of working directory no longer causes rewinddir() to fail.
 *
 * Version 1.9, Dec 15, 2009, John Cunningham
 * Added rewinddir member function
 *
 * Version 1.8, Jan 18, 2008, Toni Ronkko
 * Using FindFirstFileA and WIN32_FIND_DATAA to avoid converting string
 * between multi-byte and unicode representations.  This makes the
 * code simpler and also allows the code to be compiled under MingW.  Thanks
 * to Azriel Fasten for the suggestion.
 *
 * Mar 4, 2007, Toni Ronkko
 * Bug fix: due to the strncpy_s() function this file only compiled in
 * Visual Studio 2005.  Using the new string functions only when the
 * compiler version allows.
 *
 * Nov  2, 2006, Toni Ronkko
 * Major update: removed support for Watcom C, MS-DOS and Turbo C to
 * simplify the file, updated the code to compile cleanly on Visual
 * Studio 2005 with both unicode and multi-byte character strings,
 * removed rewinddir() as it had a bug.
 *
 * Aug 20, 2006, Toni Ronkko
 * Removed all remarks about MSVC 1.0, which is antiqued now.  Simplified
 * comments by removing SGML tags.
 *
 * May 14 2002, Toni Ronkko
 * Embedded the function definitions directly to the header so that no
 * source modules need to be included in the Visual Studio project.  Removed
 * all the dependencies to other projects so that this header file can be
 * used independently.
 *
 * May 28 1998, Toni Ronkko
 * First version.
 *****************************************************************************/
#ifndef DIRENT_H
#define DIRENT_H

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#   define _X86_
#endif


#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <winbase.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* Indicates that d_type field is available in dirent structure */
#define _DIRENT_HAVE_D_TYPE

/* Indicates that d_namlen field is available in dirent structure */
#define _DIRENT_HAVE_D_NAMLEN

/* Entries missing from MSVC 6.0 */
#if !defined(FILE_ATTRIBUTE_DEVICE)
#   define FILE_ATTRIBUTE_DEVICE 0x40
#endif

/* File type and permission flags for stat() */
#if !defined(S_IFMT)
#   define S_IFMT   _S_IFMT                     /* File type mask */
#endif
#if !defined(S_IFDIR)
#   define S_IFDIR  _S_IFDIR                    /* Directory */
#endif
#if !defined(S_IFCHR)
#   define S_IFCHR  _S_IFCHR                    /* Character device */
#endif
#if !defined(S_IFFIFO)
#   define S_IFFIFO _S_IFFIFO                   /* Pipe */
#endif
#if !defined(S_IFREG)
#   define S_IFREG  _S_IFREG                    /* Regular file */
#endif
#if !defined(S_IREAD)
#   define S_IREAD  _S_IREAD                    /* Read permission */
#endif
#if !defined(S_IWRITE)
#   define S_IWRITE _S_IWRITE                   /* Write permission */
#endif
#if !defined(S_IEXEC)
#   define S_IEXEC  _S_IEXEC                    /* Execute permission */
#endif
#if !defined(S_IFIFO)
#   define S_IFIFO _S_IFIFO                     /* Pipe */
#endif
#if !defined(S_IFBLK)
#   define S_IFBLK   0                          /* Block device */
#endif
#if !defined(S_IFLNK)
#   define S_IFLNK   0                          /* Link */
#endif
#if !defined(S_IFSOCK)
#   define S_IFSOCK  0                          /* Socket */
#endif

#if defined(_MSC_VER)
#   define S_IRUSR  S_IREAD                     /* Read user */
#   define S_IWUSR  S_IWRITE                    /* Write user */
#   define S_IXUSR  0                           /* Execute user */
#   define S_IRGRP  0                           /* Read group */
#   define S_IWGRP  0                           /* Write group */
#   define S_IXGRP  0                           /* Execute group */
#   define S_IROTH  0                           /* Read others */
#   define S_IWOTH  0                           /* Write others */
#   define S_IXOTH  0                           /* Execute others */
#endif

/* Maximum length of file name */
#if !defined(PATH_MAX)
#   define PATH_MAX MAX_PATH
#endif
#if !defined(FILENAME_MAX)
#   define FILENAME_MAX MAX_PATH
#endif
#if !defined(NAME_MAX)
#   define NAME_MAX FILENAME_MAX
#endif

/* File type flags for d_type */
#define DT_UNKNOWN  0
#define DT_REG      S_IFREG
#define DT_DIR      S_IFDIR
#define DT_FIFO     S_IFIFO
#define DT_SOCK     S_IFSOCK
#define DT_CHR      S_IFCHR
#define DT_BLK      S_IFBLK

/* Macros for converting between st_mode and d_type */
#define IFTODT(mode) ((mode) & S_IFMT)
#define DTTOIF(type) (type)

/*
 * File type macros.  Note that block devices, sockets and links cannot be
 * distinguished on Windows and the macros S_ISBLK, S_ISSOCK and S_ISLNK are
 * only defined for compatibility.  These macros should always return 0
 * on Windows.
 */
#define	S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define	S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define	S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define	S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)
#define	S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#define	S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)

/* Return the exact length of d_namlen without zero terminator */
#define _D_EXACT_NAMLEN(p) ((p)->d_namlen)

/* Return number of bytes needed to store d_namlen */
#define _D_ALLOC_NAMLEN(p) (PATH_MAX + 1)


#ifdef __cplusplus
extern "C" {
#endif


/* Wide-character version */
struct _wdirent {
    long d_ino;                                 /* Always zero */
    unsigned short d_reclen;                    /* Structure size */
    size_t d_namlen;                            /* Length of name without \0 */
    int d_type;                                 /* File type */
    wchar_t d_name[PATH_MAX + 1];               /* File name */
};
typedef struct _wdirent _wdirent;

struct _WDIR {
    struct _wdirent ent;                        /* Current directory entry */
    WIN32_FIND_DATAW data;                      /* Private file data */
    int cached;                                 /* True if data is valid */
    HANDLE handle;                              /* Win32 search handle */
    wchar_t *patt;                              /* Initial directory name */
};
typedef struct _WDIR _WDIR;

_WDIR *_wopendir (const wchar_t *dirname);
struct _wdirent *_wreaddir (_WDIR *dirp);
int _wclosedir (_WDIR *dirp);
void _wrewinddir (_WDIR* dirp);


/* For compatibility with Symbian */
#define wdirent _wdirent
#define WDIR _WDIR
#define wopendir _wopendir
#define wreaddir _wreaddir
#define wclosedir _wclosedir
#define wrewinddir _wrewinddir


/* Multi-byte character versions */
struct dirent {
    long d_ino;                                 /* Always zero */
    unsigned short d_reclen;                    /* Structure size */
    size_t d_namlen;                            /* Length of name without \0 */
    int d_type;                                 /* File type */
    char d_name[PATH_MAX + 1];                  /* File name */
};
typedef struct dirent dirent;

struct DIR {
    struct dirent ent;
    struct _WDIR *wdirp;
};
typedef struct DIR DIR;

DIR *opendir (const char *dirname);
struct dirent *readdir (DIR *dirp);
int closedir (DIR *dirp);
void rewinddir (DIR* dirp);


/* Internal utility functions */
WIN32_FIND_DATAW *dirent_first (_WDIR *dirp);
WIN32_FIND_DATAW *dirent_next (_WDIR *dirp);

int dirent_mbstowcs_s(
    size_t *pReturnValue,
    wchar_t *wcstr,
    size_t sizeInWords,
    const char *mbstr,
    size_t count);

int dirent_wcstombs_s(
    size_t *pReturnValue,
    char *mbstr,
    size_t sizeInBytes,
    const wchar_t *wcstr,
    size_t count);

void dirent_set_errno (int error);

#ifdef __cplusplus
}
#endif

typedef unsigned int mode_t;

#endif /*DIRENT_H*/
