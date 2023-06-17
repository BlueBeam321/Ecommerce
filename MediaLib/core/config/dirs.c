/*****************************************************************************
 * dirs.c: XDG directories configuration
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_charset.h>
#include "common/core/config/configuration.h"

#include <assert.h>

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>

char *config_GetLibDir (void)
{
    /* Get our full path */
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery (config_GetLibDir, &mbi, sizeof(mbi)))
        goto error;

    wchar_t wpath[MAX_PATH];
    if (!GetModuleFileName ((HMODULE) mbi.AllocationBase, wpath, MAX_PATH))
        goto error;

    wchar_t *file = wcsrchr (wpath, L'\\');
    if (file == NULL)
        goto error;
    *file = L'\0';

    return FromWide (wpath);
error:
    abort ();
}

char *config_GetDataDir (void)
{
    const char *path = getenv ("VLC_DATA_PATH");
    return (path != NULL) ? strdup (path) : config_GetLibDir ();
}

static char *config_GetShellDir (int csidl)
{
    wchar_t wdir[MAX_PATH];

    if (SHGetFolderPathW (NULL, csidl | CSIDL_FLAG_CREATE,
        NULL, SHGFP_TYPE_CURRENT, wdir ) == S_OK)
        return FromWide (wdir);
    return NULL;
}

static char *config_GetAppDir (void)
{
    /* if portable directory exists, use it */
    TCHAR path[MAX_PATH];
    if (GetModuleFileName (NULL, path, MAX_PATH))
    {
        TCHAR *lastDir = _tcsrchr (path, '\\');
        if (lastDir)
        {
            _tcscpy (lastDir + 1, TEXT("portable"));
            DWORD attrib = GetFileAttributes (path);
            if (attrib != INVALID_FILE_ATTRIBUTES &&
                (attrib & FILE_ATTRIBUTE_DIRECTORY))
                return FromT (path);
        }
    }

    char *psz_dir;
    char *psz_parent = config_GetShellDir (CSIDL_APPDATA);

    if (psz_parent == NULL ||  asprintf (&psz_dir, "%s\\MVP", psz_parent) == -1)
        psz_dir = NULL;
    free (psz_parent);
    return psz_dir;
}

char *config_GetUserDir (vlc_userdir_t type)
{
    switch (type)
    {
    case VLC_HOME_DIR:
        return config_GetShellDir (CSIDL_PERSONAL);
    case VLC_CONFIG_DIR:
    case VLC_DATA_DIR:
        return config_GetAppDir();
    case VLC_CACHE_DIR:
        return config_GetAppDir();
    case VLC_DESKTOP_DIR:
    case VLC_DOWNLOAD_DIR:
    case VLC_TEMPLATES_DIR:
    case VLC_PUBLICSHARE_DIR:
    case VLC_DOCUMENTS_DIR:
        return config_GetUserDir(VLC_HOME_DIR);
    case VLC_MUSIC_DIR:
        return config_GetShellDir(CSIDL_MYMUSIC);
    case VLC_PICTURES_DIR:
        return config_GetShellDir(CSIDL_MYPICTURES);
    case VLC_VIDEOS_DIR:
        return config_GetShellDir(CSIDL_MYVIDEO);
    }
    vlc_assert_unreachable();
}
#else
#include <unistd.h>
#include <pwd.h>
#include <assert.h>
#include <limits.h>

#if !defined (__linux__)
/**
 * Determines the shared data directory
 *
 * @return a nul-terminated string or NULL. Use free() to release it.
 */
char *config_GetDataDir (void)
{
    const char *path = getenv ("VLC_DATA_PATH");
    return strdup ((path != NULL) ? path : PKGDATADIR);
}

/**
 * Determines the architecture-dependent data directory
 *
 * @return a string (always succeeds).
 */
char *config_GetLibDir (void)
{
    return strdup (PKGLIBDIR);
}
#endif

static char *config_GetHomeDir (void)
{
    /* 1/ Try $HOME  */
    const char *home = getenv ("HOME");
    if (home != NULL)
        return strdup (home);
#if defined(HAVE_GETPWUID_R)
    /* 2/ Try /etc/passwd */
    long max = sysconf (_SC_GETPW_R_SIZE_MAX);
    if (max != -1)
    {
        char buf[max];
        struct passwd pwbuf, *pw;

        if (getpwuid_r (getuid (), &pwbuf, buf, sizeof (buf), &pw) == 0
          && pw != NULL)
            return strdup (pw->pw_dir);
    }
#endif
    return NULL;
}

static char *config_GetAppDir (const char *xdg_name, const char *xdg_default)
{
    char *psz_dir;
    char var[sizeof ("XDG__HOME") + strlen (xdg_name)];

    /* XDG Base Directory Specification - Version 0.6 */
    snprintf (var, sizeof (var), "XDG_%s_HOME", xdg_name);

    const char *home = getenv (var);
    if (home != NULL)
    {
        if (asprintf (&psz_dir, "%s/vlc", home) == -1)
            psz_dir = NULL;
        return psz_dir;
    }

    char *psz_home = config_GetHomeDir ();
    if( psz_home == NULL
     || asprintf( &psz_dir, "%s/%s/vlc", psz_home, xdg_default ) == -1 )
        psz_dir = NULL;
    free (psz_home);
    return psz_dir;
}

static char *config_GetTypeDir (const char *xdg_name)
{
    const size_t namelen = strlen (xdg_name);
    const char *home = getenv ("HOME");
    const char *dir = getenv ("XDG_CONFIG_HOME");
    const char *file = "user-dirs.dirs";

    if (home == NULL)
        return NULL;
    if (dir == NULL)
    {
        dir = home;
        file = ".config/user-dirs.dirs";
    }

    char *path;
    if (asprintf (&path, "%s/%s", dir, file) == -1)
        return NULL;

    FILE *stream = fopen (path, "rte");
    free (path);
    path = NULL;
    if (stream != NULL)
    {
        char *linebuf = NULL;
        size_t linelen = 0;

        while (getline (&linebuf, &linelen, stream) != -1)
        {
            char *ptr = linebuf;
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (strncmp (ptr, "XDG_", 4))
                continue;
            ptr += 4; /* Skip XDG_ */
            if (strncmp (ptr, xdg_name, namelen))
                continue;
            ptr += namelen; /* Skip XDG type name */
            if (strncmp (ptr, "_DIR", 4))
                continue;
            ptr += 4; /* Skip _DIR */
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (*ptr != '=')
                continue;
            ptr++; /* Skip equality sign */
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (*ptr != '"')
                continue;
            ptr++; /* Skip quote */
            linelen -= ptr - linebuf;

            char *out;
            if (strncmp (ptr, "$HOME", 5))
            {
                path = malloc (linelen);
                if (path == NULL)
                    continue;
                out = path;
            }
            else
            {   /* Prefix with $HOME */
                const size_t homelen = strlen (home);
                ptr += 5;
                path = malloc (homelen + linelen - 5);
                if (path == NULL)
                    continue;
                memcpy (path, home, homelen);
                out = path + homelen;
            }

            while (*ptr != '"')
            {
                if (*ptr == '\\')
                    ptr++;
                if (*ptr == '\0')
                {
                    free (path);
                    path = NULL;
                    break;
                }
                *(out++) = *(ptr++);
            }
            if (path != NULL)
                *out = '\0';
            break;
        }
        free (linebuf);
        fclose (stream);
    }

    /* Default! */
    if (path == NULL)
    {
        if (strcmp (xdg_name, "DESKTOP") == 0)
        {
            if (asprintf (&path, "%s/Desktop", home) == -1)
                return NULL;
        }
        else
            path = strdup (home);
    }

    return path;
}


char *config_GetUserDir (vlc_userdir_t type)
{
    switch (type)
    {
        case VLC_HOME_DIR:
            break;
        case VLC_CONFIG_DIR:
            return config_GetAppDir ("CONFIG", ".config");
        case VLC_DATA_DIR:
            return config_GetAppDir ("DATA", ".local/share");
        case VLC_CACHE_DIR:
            return config_GetAppDir ("CACHE", ".cache");

        case VLC_DESKTOP_DIR:
            return config_GetTypeDir ("DESKTOP");
        case VLC_DOWNLOAD_DIR:
            return config_GetTypeDir ("DOWNLOAD");
        case VLC_TEMPLATES_DIR:
            return config_GetTypeDir ("TEMPLATES");
        case VLC_PUBLICSHARE_DIR:
            return config_GetTypeDir ("PUBLICSHARE");
        case VLC_DOCUMENTS_DIR:
            return config_GetTypeDir ("DOCUMENTS");
        case VLC_MUSIC_DIR:
            return config_GetTypeDir ("MUSIC");
        case VLC_PICTURES_DIR:
            return config_GetTypeDir ("PICTURES");
        case VLC_VIDEOS_DIR:
            return config_GetTypeDir ("VIDEOS");
    }
    return config_GetHomeDir ();
}
#endif