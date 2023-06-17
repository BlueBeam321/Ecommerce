/*****************************************************************************
 * modules.h : Module management functions.
 *****************************************************************************/

#ifndef LIBVLC_MODULES_H
#define LIBVLC_MODULES_H 1

# include <vlc_atomic.h>

extern struct vlc_plugin_t *vlc_plugins;

/** The plugin handle type */
typedef void *module_handle_t;

#define MODULE_SHORTCUT_MAX 20

/** Plugin entry point prototype */
typedef int (*vlc_plugin_cb) (int (*)(void *, void *, int, ...), void *);

/** Core module */
int vlc_entry__core (int (*)(void *, void *, int, ...), void *);

/**
 * Internal module descriptor
 */
struct module_t
{
    vlc_plugin_t *plugin; /**< Plug-in/library containing the module */
    module_t   *next;

    /** Shortcuts to the module */
    unsigned    i_shortcuts;
    const char **pp_shortcuts;

    /*
     * Variables set by the module to identify itself
     */
    const char *psz_shortname;                              /**< Module name */
    const char *psz_longname;                   /**< Module descriptive name */
    const char *psz_help;        /**< Long help string for "special" modules */

    const char *psz_capability;                              /**< Capability */
    int      i_score;                          /**< Score for the capability */

    /* Callbacks */
    const char *activate_name;
    const char *deactivate_name;
    void *pf_activate;
    void *pf_deactivate;
};

vlc_plugin_t *vlc_plugin_create(void);
void vlc_plugin_destroy(vlc_plugin_t *);
module_t *vlc_module_create(vlc_plugin_t *);
void vlc_module_destroy (module_t *);

vlc_plugin_t *vlc_plugin_describe(vlc_plugin_cb);
int vlc_plugin_resolve(vlc_plugin_t *, vlc_plugin_cb);

void module_InitBank (void);
size_t module_LoadPlugins( vlc_object_t * );
#define module_LoadPlugins(a) module_LoadPlugins(VLC_OBJECT(a))
void module_EndBank (bool);
int module_Map(vlc_object_t *, vlc_plugin_t *);

ssize_t module_list_cap (module_t ***, const char *);

int vlc_bindtextdomain (const char *);

/* Low-level OS-dependent handler */
int module_Load (vlc_object_t *, const char *, module_handle_t *, bool);
void *module_Lookup (module_handle_t, const char *);
void module_Unload (module_handle_t);

/* Plugins cache */
vlc_plugin_t *vlc_cache_load(vlc_object_t *, const char *, block_t **);
vlc_plugin_t *vlc_cache_lookup(vlc_plugin_t **, const char *relpath);

void CacheSave(vlc_object_t *, const char *, vlc_plugin_t *const *, size_t);

#endif /* !LIBVLC_MODULES_H */
