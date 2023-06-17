/*****************************************************************************
 * vlc_internal.h: Internal libvlc generic/misc declaration
 *****************************************************************************/

#ifndef LIBVLC_LIBVLC_H
# define LIBVLC_LIBVLC_H 1

#include <vlc_input_item.h>

/*
 * OS-specific initialization
 */
void system_Init(void);
void system_Configure(libvlc_int_t *);
#if defined(_WIN32) || defined(__OS2__)
void system_End(void);
#endif
void vlc_CPU_init(void);
void vlc_CPU_dump(vlc_object_t *);

/*
 * Logging
 */
typedef struct vlc_logger_t vlc_logger_t;

int vlc_LogPreinit(libvlc_int_t *);
int vlc_LogInit(libvlc_int_t *);
void vlc_LogDeinit(libvlc_int_t *);

/*
 * LibVLC exit event handling
 */
typedef struct vlc_exit
{
    vlc_mutex_t lock;
    void (*handler) (void *);
    void *opaque;
} vlc_exit_t;

void vlc_ExitInit( vlc_exit_t * );
void vlc_ExitDestroy( vlc_exit_t * );

/*
 * LibVLC objects stuff
 */

#define ZOOM_SECTION N_("Zoom")
#define ZOOM_QUARTER_KEY_TEXT N_("1:4 Quarter")
#define ZOOM_HALF_KEY_TEXT N_("1:2 Half")
#define ZOOM_ORIGINAL_KEY_TEXT N_("1:1 Original")
#define ZOOM_DOUBLE_KEY_TEXT N_("2:1 Double")

/**
 * Private LibVLC instance data.
 */
typedef struct vlc_dialog_provider vlc_dialog_provider;
typedef struct vlc_keystore vlc_keystore;
typedef struct vlc_actions_t vlc_actions_t;

typedef struct libvlc_priv_t
{
    libvlc_int_t       public_data;

    /* Logging */
    bool               b_stats;     ///< Whether to collect stats

    /* Singleton objects */
    vlc_logger_t      *logger;
    struct playlist_preparser_t *parser;

    /* Exit callback */
    vlc_exit_t       exit;
} libvlc_priv_t;

static inline libvlc_priv_t *libvlc_priv (libvlc_int_t *libvlc)
{
    return container_of(libvlc, libvlc_priv_t, public_data);
}

int intf_InsertItem(libvlc_int_t *, const char *mrl, unsigned optc,
                    const char * const *optv, unsigned flags);
void intf_DestroyAll( libvlc_int_t * );

#define libvlc_stats( o ) (libvlc_priv((VLC_OBJECT(o))->obj.libvlc)->b_stats)

int vlc_MetadataRequest(libvlc_int_t *libvlc, input_item_t *item,
                        input_item_meta_request_option_t i_options,
                        int timeout, void *id);

/*
 * Variables stuff
 */
void var_OptionParse (vlc_object_t *, const char *, bool trusted);

#endif
