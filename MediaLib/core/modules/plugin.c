/*****************************************************************************
 * plugin.c : Low-level dynamic library handling
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include "common/core/modules/modules.h"

#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <dlfcn.h>
#endif

#ifdef _WIN32
#if 1 //(_WIN32_WINNT < _WIN32_WINNT_WIN7)
static BOOL WINAPI SetThreadErrorModeFallback(DWORD mode, DWORD *oldmode)
{
    /* TODO: cache the pointer */
    HANDLE h = GetModuleHandle(_T("kernel32.dll"));
    if (unlikely(h == NULL))
        return FALSE;

    BOOL(WINAPI *SetThreadErrorModeReal)(DWORD, DWORD *);

    SetThreadErrorModeReal = GetProcAddress(h, "SetThreadErrorMode");
    if (SetThreadErrorModeReal != NULL)
        return SetThreadErrorModeReal(mode, oldmode);

# if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    UINT(WINAPI *GetErrorModeReal)(void);
    DWORD curmode = 0;

    GetErrorModeReal = (void *)GetProcAddress(h, "GetErrorMode");
    if (GetErrorModeReal != NULL)
        curmode = GetErrorModeReal();
    else
        curmode = SEM_FAILCRITICALERRORS;
# else
    DWORD curmode = GetErrorMode();
# endif
    /* Extra flags should be OK. Missing flags are NOT OK. */
    if ((mode & curmode) != mode)
        return FALSE;
    if (oldmode != NULL)
        *oldmode = curmode;
    return TRUE;
}
# define SetThreadErrorMode SetThreadErrorModeFallback
#endif

static char *GetWindowsError(void)
{
    wchar_t wmsg[256];
    int i = 0, i_error = GetLastError();

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, i_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        wmsg, 256, NULL);

    /* Go to the end of the string */
    while (!wmemchr(L"\r\n\0", wmsg[i], 3))
        i++;

    snwprintf(wmsg + i, 256 - i, L" (error %i)", i_error);
    return FromWide(wmsg);
}
#endif

/**
 * Load a dynamically linked library using a system dependent method.
 *
 * \param p_this vlc object
 * \param path library file
 * \param p_handle the module handle returned
 * \return 0 on success as well as the module handle.
 */
int module_Load (vlc_object_t *p_this, const char *path,
                 module_handle_t *p_handle, bool lazy)
{
#ifdef _WIN32
    wchar_t *wfile = ToWide(path);
    if (wfile == NULL)
        return -1;

    module_handle_t handle = NULL;
    DWORD mode;
    if (SetThreadErrorMode (SEM_FAILCRITICALERRORS, &mode) != 0)
    {
        handle = LoadLibraryExW(wfile, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        SetThreadErrorMode(mode, NULL);
    }
    free (wfile);

    if (handle == NULL)
    {
        char *psz_err = GetWindowsError();
        msg_Warn(p_this, "cannot load module `%s' (%s)", path, psz_err);
        free(psz_err);
        return -1;
    }

    *p_handle = handle;
    (void)lazy;
    return 0;
#else
#if defined (RTLD_NOW)
    const int flags = lazy ? RTLD_LAZY : RTLD_NOW;
#elif defined (DL_LAZY)
    const int flags = DL_LAZY;
#else
    const int flags = 0;
#endif

    module_handle_t handle = dlopen (path, flags);
    if( handle == NULL )
    {
        msg_Warn( p_this, "cannot load module `%s' (%s)", path, dlerror() );
        return -1;
    }
    *p_handle = handle;
    return 0;
#endif
}

/**
 * CloseModule: unload a dynamic library
 *
 * This function unloads a previously opened dynamically linked library
 * using a system dependent method. No return value is taken in consideration,
 * since some libraries sometimes refuse to close properly.
 * \param handle handle of the library
 * \return nothing
 */
void module_Unload( module_handle_t handle )
{
#ifdef _WIN32
    FreeLibrary( handle );
#else
#if !defined(__SANITIZE_ADDRESS__)
#ifdef HAVE_VALGRIND_VALGRIND_H
    if( RUNNING_ON_VALGRIND > 0 )
        return; /* do not dlclose() so that we get proper stack traces */
#endif
    dlclose( handle );
#else
    (void) handle;
#endif
#endif
}

/**
 * Looks up a symbol from a dynamically loaded library
 *
 * This function queries a loaded library for a symbol specified in a
 * string, and returns a pointer to it. We don't check for dlerror() or
 * similar functions, since we want a non-NULL symbol anyway.
 *
 * @param handle handle to the module
 * @param psz_function function name
 * @return NULL on error, or the address of the symbol
 */
void *module_Lookup( module_handle_t handle, const char *psz_function )
{
#ifdef _WIN32

#else
    return dlsym( handle, psz_function );
#endif
}
