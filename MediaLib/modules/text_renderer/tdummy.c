/*****************************************************************************
 * tdummy.c : dummy text rendering functions
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>

#define MODULE_NAME     trender_dummy
#define MODULE_STRING   "trender_dummy"

static int OpenRenderer( vlc_object_t * );

vlc_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy font renderer") )
    set_capability( "text renderer", 1 )
    set_callbacks( OpenRenderer, NULL )
vlc_module_end ()


static int RenderText( filter_t *p_filter, subpicture_region_t *p_region_out,
                       subpicture_region_t *p_region_in,
                       const vlc_fourcc_t *p_chroma_list )
{
    VLC_UNUSED(p_filter); VLC_UNUSED(p_region_out); VLC_UNUSED(p_region_in);
    VLC_UNUSED(p_chroma_list);
    return VLC_EGENERIC;
}

static int OpenRenderer( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    p_filter->pf_render = RenderText;
    return VLC_SUCCESS;
}
