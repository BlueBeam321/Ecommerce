/*****************************************************************************
 * vdummy.c: Dummy video output display method for testing purposes
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>

#define MODULE_NAME     vout_vdummy
#define MODULE_STRING   "vout_vdummy"

#define CHROMA_TEXT N_("Dummy image chroma format")
#define CHROMA_LONGTEXT N_( \
    "Force the dummy video output to create images using a specific chroma " \
    "format instead of trying to improve performances by using the most " \
    "efficient one.")

static int OpenDummy( vlc_object_t * );
static int OpenStats( vlc_object_t * );
static void Close( vlc_object_t * );

vlc_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy video output") )
    set_capability( "vout display", 0 )
    set_callbacks( OpenDummy, Close )
    add_shortcut( "dummy" )

    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_VOUT )
    add_string( "dummy-chroma", NULL, CHROMA_TEXT, CHROMA_LONGTEXT, true )

    add_submodule ()
    set_description( N_("Statistics video output") )
    set_capability( "vout display", 0 )
    add_shortcut( "stats" )
    set_callbacks( OpenStats, Close )
vlc_module_end ()


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
struct vout_display_sys_t {
    picture_pool_t *pool;
};
static picture_pool_t *Pool(vout_display_t *, unsigned count);
static void            Display(vout_display_t *, picture_t *, subpicture_t *);
static void            DisplayStat(vout_display_t *, picture_t *, subpicture_t *);
static int             Control(vout_display_t *, int, va_list);

/*****************************************************************************
 * OpenVideo: activates dummy vout display method
 *****************************************************************************/
static int Open(vlc_object_t *object,
                void (*display)(vout_display_t *, picture_t *, subpicture_t *))
{
    vout_display_t *vd = (vout_display_t *)object;
    vout_display_sys_t *sys;

    vd->sys = sys = calloc(1, sizeof(*sys));
    if (!sys)
        return VLC_EGENERIC;
    sys->pool = NULL;

    /* p_vd->info is not modified */

    char *chroma = var_InheritString(vd, "dummy-chroma");
    if (chroma) {
        vlc_fourcc_t fcc = vlc_fourcc_GetCodecFromString(VIDEO_ES, chroma);
        if (fcc != 0) {
            msg_Dbg(vd, "forcing chroma 0x%.8x (%4.4s)", fcc, (char*)&fcc);
            vd->fmt.i_chroma = fcc;
        }
        free(chroma);
    }
    vd->pool    = Pool;
    vd->prepare = NULL;
    vd->display = display;
    vd->control = Control;

    vout_display_DeleteWindow(vd, NULL);

    return VLC_SUCCESS;
}

static int OpenDummy(vlc_object_t *object)
{
    return Open(object, Display);
}

static int OpenStats(vlc_object_t *object)
{
    return Open(object, DisplayStat);
}

static void Close(vlc_object_t *object)
{
    vout_display_t *vd = (vout_display_t *)object;
    vout_display_sys_t *sys = vd->sys;

    if (sys->pool)
        picture_pool_Release(sys->pool);
    free(sys);
}

static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{
    vout_display_sys_t *sys = vd->sys;
    if (!sys->pool)
        sys->pool = picture_pool_NewFromFormat(&vd->fmt, count);
    return sys->pool;
}

static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
    VLC_UNUSED(vd);
    VLC_UNUSED(subpicture);
    picture_Release(picture);
}

static void DisplayStat(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
    VLC_UNUSED(vd);
    VLC_UNUSED(subpicture);
    if ( vd->fmt.i_width*vd->fmt.i_height >= sizeof(mtime_t) &&
         (picture->p->i_pitch * picture->p->i_lines) >= sizeof(mtime_t) ) {
        mtime_t date;
        memcpy(&date, picture->p->p_pixels, sizeof(date));
        msg_Dbg(vd, "VOUT got %"PRIu64" ms offset",
                (mdate() - date) / 1000 );
    }
    picture_Release(picture);
}

static int Control(vout_display_t *vd, int query, va_list args)
{
    VLC_UNUSED(vd);
    VLC_UNUSED(query);
    VLC_UNUSED(args);
    return VLC_SUCCESS;
}
