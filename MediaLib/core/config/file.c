/*****************************************************************************
 * file.c: configuration file handling
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_charset.h>
#include <vlc_fs.h>
#include <vlc_actions.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>

#include <errno.h>                                                  /* errno */
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef __APPLE__
#   include <xlocale.h>
#elif defined(HAVE_USELOCALE)
#include <locale.h>
#endif
//#include <unistd.h>

#include "common/core/config/configuration.h"
#include "common/core/modules/modules.h"

/*****************************************************************************
 * config_CreateDir: Create configuration directory if it doesn't exist.
 *****************************************************************************/
int config_CreateDir(vlc_object_t *p_this, const char *psz_dirname)
{
    if (!psz_dirname || !*psz_dirname) return -1;

    if (vlc_mkdir(psz_dirname, 0700) == 0)
        return 0;

    switch (errno)
    {
        case EEXIST:
            return 0;

        case ENOENT:
        {
            /* Let's try to create the parent directory */
            char psz_parent[256/*strlen(psz_dirname) + 1*/], *psz_end;
            strcpy(psz_parent, psz_dirname);

            psz_end = strrchr(psz_parent, DIR_SEP_CHAR);
            if (psz_end && psz_end != psz_parent)
            {
                *psz_end = '\0';
                if (config_CreateDir(p_this, psz_parent) == 0)
                {
                    if (!vlc_mkdir(psz_dirname, 0700))
                        return 0;
                }
            }
        }
    }

    msg_Warn(p_this, "could not create %s: %s", psz_dirname,
              vlc_strerror_c(errno));
    return -1;
}
