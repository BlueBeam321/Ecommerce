/*****************************************************************************
 * textdomain.c : Modules text domain management
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include "common/core/modules/modules.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# include <vlc_charset.h>
#endif

int vlc_bindtextdomain (const char *domain)
{
    (void)domain;
    return 0;
}

/**
 * In-tree plugins share their gettext domain with LibVLC.
 */
char *vlc_gettext (const char *msgid)
{
    return (char *)msgid;
}

char *vlc_ngettext (const char *msgid, const char *plural, unsigned long n)
{
    return (char *)((n == 1) ? msgid : plural);
}
