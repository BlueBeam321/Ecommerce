#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_ERRNO_H
#   include <errno.h>
#else
static int errno1;
/* FIXME: anything clever to put here? */
#   define EFAULT 12
#   define ENOTDIR 12
#   define ENOENT 12
#   define ENOMEM 12
#   define EINVAL 12
#endif
#include <string.h>
#ifndef UNDER_CE
#ifdef _MX_WIN32_
#   include <io.h>
#   include <direct.h>
#endif
#else
#   define FILENAME_MAX (260)
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* for GetFileAttributes */

#include <tchar.h>
#define SUFFIX  "*"
#define SLASH   "\\"

struct dirent
{
	long		d_ino;		/* Always zero. */
	unsigned short	d_reclen;	/* Always zero. */
	unsigned short	d_namlen;	/* Length of name in d_name. */
	char            d_name[FILENAME_MAX]; /* File name. */
};

typedef struct __DIR
{
	/* disk transfer area for this dir */
	WIN32_FIND_DATA		dd_dta;

	/* dirent struct to return from dir (NOTE: this makes this thread
	 * safe as long as only one thread uses a particular DIR struct at
	 * a time) */
	struct dirent		dd_dir;

	/* findnext handle */
	HANDLE			dd_handle;

	/*
         * Status of search:
	 *   0 = not started yet (next entry to read is first entry)
	 *  -1 = off the end
	 *   positive = 0 based index of next entry
	 */
	int			dd_stat;

	/* given path for dir with search pattern (struct is extended) */
	char			dd_name[1];
} DIR;

DIR * opendir (const CHAR *szPath);
int	closedir (DIR * dirp);
struct dirent *	readdir (DIR * dirp);
void rewinddir (DIR * dirp);
void seekdir (DIR * dirp, long lPos);
long telldir (DIR * dirp);
