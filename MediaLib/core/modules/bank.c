/*****************************************************************************
 * bank.c : Modules list
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_modules.h>
#include <vlc_fs.h>
#include <vlc_block.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
//#include <unistd.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif

#include "common/core/config/configuration.h"
#include "common/core/modules/modules.h"

typedef struct vlc_modcap
{
    char *name;
    module_t **modv;
    size_t modc;
} vlc_modcap_t;

static int vlc_modcap_cmp(const void *a, const void *b)
{
    const vlc_modcap_t *capa = a, *capb = b;
    return strcmp(capa->name, capb->name);
}

static void vlc_modcap_free(void *data)
{
    vlc_modcap_t *cap = data;

    free(cap->modv);
    free(cap->name);
    free(cap);
}

static int vlc_module_cmp (const void *a, const void *b)
{
    const module_t *const *ma = a, *const *mb = b;
    /* Note that qsort() uses _ascending_ order,
     * so the smallest module is the one with the biggest score. */
    return (*mb)->i_score - (*ma)->i_score;
}

static void vlc_modcap_sort(const void *node, const VISIT which,
                            const int depth)
{
    vlc_modcap_t *const *cp = node, *cap = *cp;

    if (which != postorder && which != leaf)
        return;

    qsort(cap->modv, cap->modc, sizeof (*cap->modv), vlc_module_cmp);
    (void) depth;
}

static struct
{
    vlc_mutex_t lock;
    block_t *caches;
    void *caps_tree;
    unsigned usage;
} modules = { VLC_STATIC_MUTEX, NULL, NULL, 0 };

vlc_plugin_t *vlc_plugins = NULL;

/**
 * Adds a module to the bank
 */
static int vlc_module_store(module_t *mod)
{
    const char *name = module_get_capability(mod);
    vlc_modcap_t *cap = malloc(sizeof (*cap));
    if (unlikely(cap == NULL))
        return -1;

    cap->name = strdup(name);
    cap->modv = NULL;
    cap->modc = 0;

    if (unlikely(cap->name == NULL))
        goto error;

    vlc_modcap_t **cp = tsearch(cap, &modules.caps_tree, vlc_modcap_cmp);
    if (unlikely(cp == NULL))
        goto error;

    if (*cp != cap)
    {
        vlc_modcap_free(cap);
        cap = *cp;
    }

    module_t **modv = realloc(cap->modv, sizeof (*modv) * (cap->modc + 1));
    if (unlikely(modv == NULL))
        return -1;

    cap->modv = modv;
    cap->modv[cap->modc] = mod;
    cap->modc++;
    return 0;
error:
    vlc_modcap_free(cap);
    return -1;
}

/**
 * Adds a plugin (and all its modules) to the bank
 */
static void vlc_plugin_store(vlc_plugin_t *lib)
{
    /*vlc_assert_locked (&modules.lock);*/

    lib->next = vlc_plugins;
    vlc_plugins = lib;

    for (module_t *m = lib->module; m != NULL; m = m->next)
        vlc_module_store(m);
}

/**
 * Registers a statically-linked plug-in.
 */
static vlc_plugin_t *module_InitStatic(vlc_plugin_cb entry)
{
    /* Initializes the statically-linked library */
    vlc_plugin_t *lib = vlc_plugin_describe (entry);
    if (unlikely(lib == NULL))
        return NULL;

    return lib;
}

extern vlc_plugin_cb vlc_static_modules[];

static void module_InitStaticModules(void)
{
    if (!vlc_static_modules)
        return;

    for (unsigned i = 0; vlc_static_modules[i]; i++)
    {
        vlc_plugin_t *lib = module_InitStatic(vlc_static_modules[i]);
        if (likely(lib != NULL))
            vlc_plugin_store(lib);
    }
}

int module_Map(vlc_object_t *obj, vlc_plugin_t *plugin)
{
    (void) obj; (void) plugin;
    return 0;
}

static void module_Unmap(vlc_plugin_t *plugin)
{
    (void) plugin;
}

/**
 * Init bank
 *
 * Creates a module bank structure which will be filled later
 * on with all the modules found.
 */
void module_InitBank (void)
{
    vlc_mutex_lock (&modules.lock);

    if (modules.usage == 0)
    {
        /* Fills the module bank structure with the core module infos.
         * This is very useful as it will allow us to consider the core
         * library just as another module, and for instance the configuration
         * options of core will be available in the module bank structure just
         * as for every other module. */
        vlc_plugin_t *plugin = module_InitStatic(vlc_entry__core);
        if (likely(plugin != NULL))
            vlc_plugin_store(plugin);
        config_SortConfig ();
    }
    modules.usage++;
}

/**
 * Unloads all unused plugin modules and empties the module
 * bank in case of success.
 */
void module_EndBank (bool b_plugins)
{
    vlc_plugin_t *libs = NULL;
    block_t *caches = NULL;
    void *caps_tree = NULL;

    /* If plugins were _not_ loaded, then the caller still has the bank lock
     * from module_InitBank(). */
    if( b_plugins )
        vlc_mutex_lock (&modules.lock);
    /*else
        vlc_assert_locked (&modules.lock); not for static mutexes :( */

    assert (modules.usage > 0);
    if (--modules.usage == 0)
    {
        config_UnsortConfig ();
        libs = vlc_plugins;
        caches = modules.caches;
        caps_tree = modules.caps_tree;
        vlc_plugins = NULL;
        modules.caches = NULL;
        modules.caps_tree = NULL;
    }
    vlc_mutex_unlock (&modules.lock);

    tdestroy(caps_tree, vlc_modcap_free);

    while (libs != NULL)
    {
        vlc_plugin_t *lib = libs;

        libs = lib->next;
        module_Unmap(lib);
        vlc_plugin_destroy(lib);
    }

    block_ChainRelease(caches);
}

#undef module_LoadPlugins
/**
 * Loads module descriptions for all available plugins.
 * Fills the module bank structure with the plugin modules.
 *
 * \param p_this vlc object structure
 * \return total number of modules in bank after loading all plug-ins
 */
size_t module_LoadPlugins (vlc_object_t *obj)
{
    /*vlc_assert_locked (&modules.lock); not for static mutexes :( */

    if (modules.usage == 1)
    {
        module_InitStaticModules();
        config_UnsortConfig ();
        config_SortConfig ();

        twalk(modules.caps_tree, vlc_modcap_sort);
    }
    vlc_mutex_unlock (&modules.lock);

    size_t count;
    module_t **list = module_list_get (&count);
    module_list_free (list);
    msg_Dbg (obj, "plug-ins loaded: %u modules", count);
    return count;
}

/**
 * Frees the flat list of VLC modules.
 * @param list list obtained by module_list_get()
 * @param length number of items on the list
 * @return nothing.
 */
void module_list_free (module_t **list)
{
    free (list);
}

/**
 * Gets the flat list of VLC modules.
 * @param n [OUT] pointer to the number of modules
 * @return table of module pointers (release with module_list_free()),
 *         or NULL in case of error (in that case, *n is zeroed).
 */
module_t **module_list_get (size_t *n)
{
    module_t **tab = NULL;
    size_t i = 0;

    assert (n != NULL);

    for (vlc_plugin_t *lib = vlc_plugins; lib != NULL; lib = lib->next)
    {
        module_t **nt = realloc(tab, (i + lib->modules_count) * sizeof (*tab));
        if (unlikely(nt == NULL))
        {
            free (tab);
            *n = 0;
            return NULL;
        }

        tab = nt;
        for (module_t *m = lib->module; m != NULL; m = m->next)
            tab[i++] = m;
    }
    *n = i;
    return tab;
}

/**
 * Builds a sorted list of all VLC modules with a given capability.
 * The list is sorted from the highest module score to the lowest.
 * @param list pointer to the table of modules [OUT]
 * @param name name of capability of modules to look for
 * @return the number of matching found, or -1 on error (*list is then NULL).
 * @note *list must be freed with module_list_free().
 */
ssize_t module_list_cap (module_t *** list, const char *name)
{
    const vlc_modcap_t **cp = tfind(&name, &modules.caps_tree, vlc_modcap_cmp);
    if (cp == NULL)
    {
        *list = NULL;
        return 0;
    }

    const vlc_modcap_t *cap = *cp;
    size_t n = cap->modc;
    module_t **tab = vlc_alloc (n, sizeof (*tab));
    *list = tab;
    if (unlikely(tab == NULL))
        return -1;

    memcpy(tab, cap->modv, sizeof (*tab) * n);
    return n;
}
