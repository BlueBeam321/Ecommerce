/*****************************************************************************
 * inhibit.c: screen saver inhibition
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_modules.h>
#include "inhibit.h"
#include <assert.h>

typedef struct
{
    vlc_inhibit_t ih;
    module_t *module;
} inhibit_t;

vlc_inhibit_t *vlc_inhibit_Create (vlc_object_t *parent)
{
    inhibit_t *priv = vlc_custom_create (parent, sizeof (*priv), "inhibit" );
    if (priv == NULL)
        return NULL;

    vlc_inhibit_t *ih = &priv->ih;
    ih->p_sys = NULL;
    ih->inhibit = NULL;

    priv->module = module_need (ih, "inhibit", NULL, false);
    if (priv->module == NULL)
    {
        vlc_object_release (ih);
        ih = NULL;
    }
    return ih;
}

void vlc_inhibit_Destroy (vlc_inhibit_t *ih)
{
    assert (ih != NULL);

    module_unneed (ih, ((inhibit_t *)ih)->module);
    vlc_object_release (ih);
}
