/*****************************************************************************
 * vlc_plugin.h : Macros used from within a module.
 *****************************************************************************/

#ifndef LIBVLC_MODULES_MACROS_H
# define LIBVLC_MODULES_MACROS_H 1

/**
 * \file
 * This file implements plugin (module) macros used to define a vlc module.
 */

enum vlc_module_properties
{
    VLC_MODULE_CREATE,
    VLC_CONFIG_CREATE,

    /* DO NOT EVER REMOVE, INSERT OR REPLACE ANY ITEM! It would break the ABI!
     * Append new items at the end ONLY. */
    VLC_MODULE_CPU_REQUIREMENT=0x100,
    VLC_MODULE_SHORTCUT,
    VLC_MODULE_CAPABILITY,
    VLC_MODULE_SCORE,
    VLC_MODULE_CB_OPEN,
    VLC_MODULE_CB_CLOSE,
    VLC_MODULE_NO_UNLOAD,
    VLC_MODULE_NAME,
    VLC_MODULE_SHORTNAME,
    VLC_MODULE_DESCRIPTION,
    VLC_MODULE_HELP,
    VLC_MODULE_TEXTDOMAIN,
    /* Insert new VLC_MODULE_* here */

    /* DO NOT EVER REMOVE, INSERT OR REPLACE ANY ITEM! It would break the ABI!
     * Append new items at the end ONLY. */
    VLC_CONFIG_NAME=0x1000,
    /* command line name (args=const char *) */

    VLC_CONFIG_VALUE,
    /* actual value (args=int64_t/double/const char *) */

    VLC_CONFIG_RANGE,
    /* minimum value (args=int64_t/double/const char * twice) */

    VLC_CONFIG_ADVANCED,
    /* enable advanced flag (args=none) */

    VLC_CONFIG_VOLATILE,
    /* don't write variable to storage (args=none) */

    VLC_CONFIG_PERSISTENT_OBSOLETE,
    /* unused (ignored) */

    VLC_CONFIG_PRIVATE,
    /* hide from user (args=none) */

    VLC_CONFIG_REMOVED,
    /* tag as no longer supported (args=none) */

    VLC_CONFIG_CAPABILITY,
    /* capability for a module or list thereof (args=const char*) */

    VLC_CONFIG_SHORTCUT,
    /* one-character (short) command line option name (args=char) */

    VLC_CONFIG_OLDNAME_OBSOLETE,
    /* unused (ignored) */

    VLC_CONFIG_SAFE,
    /* tag as modifiable by untrusted input item "sources" (args=none) */

    VLC_CONFIG_DESC,
    /* description (args=const char *, const char *, const char *) */

    VLC_CONFIG_LIST_OBSOLETE,
    /* unused (ignored) */

    VLC_CONFIG_ADD_ACTION_OBSOLETE,
    /* unused (ignored) */

    VLC_CONFIG_LIST,
    /* list of suggested values
     * (args=size_t, const <type> *, const char *const *) */

    VLC_CONFIG_LIST_CB,
    /* callback for suggested values
     * (args=const char *, size_t (*)(vlc_object_t *, <type> **, char ***)) */

    /* Insert new VLC_CONFIG_* here */
};

/** VLC plugin */
typedef struct vlc_plugin_t
{
    struct vlc_plugin_t *next;
    module_t *module;
    unsigned modules_count;

    const char *textdomain; /**< gettext domain (or NULL) */

    /**
    * Variables set by the module to store its config options
    */
    struct
    {
        module_config_t *items; /**< Table of configuration parameters */
        size_t size; /**< Size of items table */
        size_t count; /**< Number of configuration items */
        size_t booleans; /**< Number of boolean config items */
    } conf;

} vlc_plugin_t;

/* Configuration hint types */
#define CONFIG_HINT_CATEGORY                0x02  /* Start of new category */
#define CONFIG_HINT_USAGE                   0x05  /* Usage information */

#define CONFIG_CATEGORY                     0x06 /* Set category */
#define CONFIG_SUBCATEGORY                  0x07 /* Set subcategory */
#define CONFIG_SECTION                      0x08 /* Start of new section */

/* Configuration item types */
#define CONFIG_ITEM_FLOAT                   0x20  /* Float option */
#define CONFIG_ITEM_INTEGER                 0x40  /* Integer option */
#define CONFIG_ITEM_RGB                     0x41  /* RGB color option */
#define CONFIG_ITEM_BOOL                    0x60  /* Bool option */
#define CONFIG_ITEM_STRING                  0x80  /* String option */
#define CONFIG_ITEM_PASSWORD                0x81  /* Password option (*) */
#define CONFIG_ITEM_KEY                     0x82  /* Hot key option */
#define CONFIG_ITEM_MODULE                  0x84  /* Module option */
#define CONFIG_ITEM_MODULE_CAT              0x85  /* Module option */
#define CONFIG_ITEM_MODULE_LIST             0x86  /* Module option */
#define CONFIG_ITEM_MODULE_LIST_CAT         0x87  /* Module option */
#define CONFIG_ITEM_LOADFILE                0x8C  /* Read file option */
#define CONFIG_ITEM_SAVEFILE                0x8D  /* Written file option */
#define CONFIG_ITEM_DIRECTORY               0x8E  /* Directory option */
#define CONFIG_ITEM_FONT                    0x8F  /* Font option */

#define CONFIG_ITEM(x) (((x) & ~0xF) != 0)

/* Categories and subcategories */
#define CAT_INTERFACE 1
#define SUBCAT_INTERFACE_GENERAL 101
#define SUBCAT_INTERFACE_MAIN 102
#define SUBCAT_INTERFACE_CONTROL 103
#define SUBCAT_INTERFACE_HOTKEYS 104

#define CAT_AUDIO 2
#define SUBCAT_AUDIO_GENERAL 201
#define SUBCAT_AUDIO_AOUT 202
#define SUBCAT_AUDIO_AFILTER 203
#define SUBCAT_AUDIO_VISUAL 204
#define SUBCAT_AUDIO_MISC 205
#define SUBCAT_AUDIO_RESAMPLER 206

#define CAT_VIDEO 3
#define SUBCAT_VIDEO_GENERAL 301
#define SUBCAT_VIDEO_VOUT 302
#define SUBCAT_VIDEO_VFILTER 303
#define SUBCAT_VIDEO_SUBPIC 305
#define SUBCAT_VIDEO_SPLITTER 306

#define CAT_INPUT 4
#define SUBCAT_INPUT_GENERAL 401
#define SUBCAT_INPUT_ACCESS 402
#define SUBCAT_INPUT_DEMUX 403
#define SUBCAT_INPUT_VCODEC 404
#define SUBCAT_INPUT_ACODEC 405
#define SUBCAT_INPUT_SCODEC 406
#define SUBCAT_INPUT_STREAM_FILTER 407

#define CAT_SOUT 5
#define SUBCAT_SOUT_GENERAL 501
#define SUBCAT_SOUT_STREAM 502
#define SUBCAT_SOUT_MUX 503
#define SUBCAT_SOUT_ACO 504
#define SUBCAT_SOUT_PACKETIZER 505
#define SUBCAT_SOUT_VOD 507
#define SUBCAT_SOUT_RENDERER 508

#define CAT_ADVANCED 6
#define SUBCAT_ADVANCED_MISC 602
#define SUBCAT_ADVANCED_NETWORK 603

#define CAT_PLAYLIST 7
#define SUBCAT_PLAYLIST_GENERAL 701
#define SUBCAT_PLAYLIST_SD 702
#define SUBCAT_PLAYLIST_EXPORT 703

/*****************************************************************************
 * Add a few defines. You do not want to read this section. Really.
 *****************************************************************************/

/* Explanation:
 *
 * if linking a module statically, we will need:
 * #define MODULE_FUNC( zog ) module_foo_zog
 *
 * this can't easily be done with the C preprocessor, thus a few ugly hacks.
 */

/* I need to do _this_ to change « foo bar » to « module_foo_bar » ! */
#define CONCATENATE( y, z ) CRUDE_HACK( y, z )
#define CRUDE_HACK( y, z )  y##__##z

#define __VLC_SYMBOL( symbol )  CONCATENATE( symbol, MODULE_NAME )

#define CDECL_SYMBOL
#define DLL_SYMBOL

#if defined( __cplusplus )
#   define EXTERN_SYMBOL           extern "C"
#else
#   define EXTERN_SYMBOL
#endif

EXTERN_SYMBOL typedef int (*vlc_set_cb) (void *, void *, int, ...);

#define vlc_plugin_set(...) vlc_set (opaque,   NULL, __VA_ARGS__)
#define vlc_module_set(...) vlc_set (opaque, module, __VA_ARGS__)
#define vlc_config_set(...) vlc_set (opaque, config, __VA_ARGS__)

/*
 * InitModule: this function is called once and only once, when the module
 * is looked at for the first time. We get the useful data from it, for
 * instance the module name, its shortcuts, its capabilities... we also create
 * a copy of its config because the module can be unloaded at any time.
 */
#define vlc_module_begin() \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_set_cb, void *); \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_set_cb vlc_set, void *opaque) \
{ \
    module_t *module; \
    module_config_t *config = NULL; \
    if (vlc_plugin_set (VLC_MODULE_CREATE, &module)) \
        goto error; \
    if (vlc_module_set (VLC_MODULE_NAME, (MODULE_STRING))) \
        goto error;

#define vlc_module_end() \
    (void) config; \
    return 0; \
error: \
    return -1; \
}

#define add_submodule( ) \
    if (vlc_plugin_set (VLC_MODULE_CREATE, &module)) \
        goto error;

#define add_shortcut( ... ) \
{ \
    const char *shortcuts[] = { __VA_ARGS__ }; \
    if (vlc_module_set (VLC_MODULE_SHORTCUT, \
                        sizeof(shortcuts)/sizeof(shortcuts[0]), shortcuts)) \
        goto error; \
}

#define set_shortname( shortname ) \
    if (vlc_module_set (VLC_MODULE_SHORTNAME, (const char *)(shortname))) \
        goto error;

#define set_description( desc ) \
    if (vlc_module_set (VLC_MODULE_DESCRIPTION, (const char *)(desc))) \
        goto error;

#define set_help( help ) \
    if (vlc_module_set (VLC_MODULE_HELP, (const char *)(help))) \
        goto error;

#define set_capability( cap, score ) \
    if (vlc_module_set (VLC_MODULE_CAPABILITY, (const char *)(cap)) \
     || vlc_module_set (VLC_MODULE_SCORE, (int)(score))) \
        goto error;

#define set_callbacks( activate, deactivate ) \
    if (vlc_module_set(VLC_MODULE_CB_OPEN, #activate, (void *)(activate)) \
     || vlc_module_set(VLC_MODULE_CB_CLOSE, #deactivate, \
                       (void *)(deactivate))) \
        goto error;

#define cannot_unload_broken_library( ) \
    if (vlc_module_set (VLC_MODULE_NO_UNLOAD)) \
        goto error;

#define set_text_domain( dom ) \
    if (vlc_plugin_set (VLC_MODULE_TEXTDOMAIN, (dom))) \
        goto error;

/*****************************************************************************
 * Macros used to build the configuration structure.
 *
 * Note that internally we support only 3 types of config data: int, float
 *   and string.
 *   The other types declared here just map to one of these 3 basic types but
 *   have the advantage of also providing very good hints to a configuration
 *   interface so as to make it more user friendly.
 * The configuration structure also includes category hints. These hints can
 *   provide a configuration interface with some very useful data and again
 *   allow for a more user friendly interface.
 *****************************************************************************/

#define add_type_inner( type ) \
    vlc_plugin_set (VLC_CONFIG_CREATE, (type), &config);

#define add_typedesc_inner( type, text, longtext ) \
    add_type_inner( type ) \
    vlc_config_set (VLC_CONFIG_DESC, \
                    (const char *)(text), (const char *)(longtext));

#define add_typeadv_inner( type, text, longtext, advc ) \
    add_typedesc_inner( type, text, longtext ) \
    if (advc) vlc_config_set (VLC_CONFIG_ADVANCED);

#define add_typename_inner( type, name, text, longtext, advc ) \
    add_typeadv_inner( type, text, longtext, advc ) \
    vlc_config_set (VLC_CONFIG_NAME, (const char *)(name));

#define add_string_inner( type, name, text, longtext, advc, v ) \
    add_typename_inner( type, name, text, longtext, advc ) \
    vlc_config_set (VLC_CONFIG_VALUE, (const char *)(v));

#define add_int_inner( type, name, text, longtext, advc, v ) \
    add_typename_inner( type, name, text, longtext, advc ) \
    vlc_config_set (VLC_CONFIG_VALUE, (int64_t)(v));


#define set_category( i_id ) \
    add_type_inner( CONFIG_CATEGORY ) \
    vlc_config_set (VLC_CONFIG_VALUE, (int64_t)(i_id));

#define set_subcategory( i_id ) \
    add_type_inner( CONFIG_SUBCATEGORY ) \
    vlc_config_set (VLC_CONFIG_VALUE, (int64_t)(i_id));

#define set_section( text, longtext ) \
    add_typedesc_inner( CONFIG_SECTION, text, longtext )

#define add_category_hint( text, longtext, advc ) \
    add_typeadv_inner( CONFIG_HINT_CATEGORY, text, longtext, advc )

#define add_usage_hint( text ) \
    add_typedesc_inner( CONFIG_HINT_USAGE, text, NULL )

#define add_string( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_STRING, name, text, longtext, advc, \
                      value )

#define add_password( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_PASSWORD, name, text, longtext, advc, \
                      value )

#define add_loadfile( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_LOADFILE, name, text, longtext, advc, \
                      value )

#define add_savefile( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_SAVEFILE, name, text, longtext, advc, \
                      value )

#define add_directory( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_DIRECTORY, name, text, longtext, advc, \
                      value )

#define add_font( name, value, text, longtext, advc )\
    add_string_inner( CONFIG_ITEM_FONT, name, text, longtext, advc, \
                      value )

#define add_module( name, psz_caps, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_MODULE, name, text, longtext, advc, \
                      value ) \
    vlc_config_set (VLC_CONFIG_CAPABILITY, (const char *)(psz_caps));

#define add_module_list( name, psz_caps, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_MODULE_LIST, name, text, longtext, advc, \
                      value ) \
    vlc_config_set (VLC_CONFIG_CAPABILITY, (const char *)(psz_caps));

#define add_module_cat( name, i_subcategory, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_MODULE_CAT, name, text, longtext, advc, \
                      value ) \
    change_integer_range (i_subcategory /* gruik */, 0);

#define add_module_list_cat( name, i_subcategory, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_MODULE_LIST_CAT, name, text, longtext, \
                      advc, value ) \
    change_integer_range (i_subcategory /* gruik */, 0);

#define add_integer( name, value, text, longtext, advc ) \
    add_int_inner( CONFIG_ITEM_INTEGER, name, text, longtext, advc, value )

#define add_rgb( name, value, text, longtext, advc ) \
    add_int_inner( CONFIG_ITEM_RGB, name, text, longtext, advc, value ) \
    change_integer_range( 0, 0xFFFFFF )

#define add_key( name, value, text, longtext, advc ) \
    add_string_inner( CONFIG_ITEM_KEY, "global-" name, text, longtext, advc, \
                   KEY_UNSET ) \
    add_string_inner( CONFIG_ITEM_KEY, name, text, longtext, advc, value )

#define add_integer_with_range( name, value, i_min, i_max, text, longtext, advc ) \
    add_integer( name, value, text, longtext, advc ) \
    change_integer_range( i_min, i_max )

#define add_float( name, v, text, longtext, advc ) \
    add_typename_inner( CONFIG_ITEM_FLOAT, name, text, longtext, advc ) \
    vlc_config_set (VLC_CONFIG_VALUE, (double)(v));

#define add_float_with_range( name, value, f_min, f_max, text, longtext, advc ) \
    add_float( name, value, text, longtext, advc ) \
    change_float_range( f_min, f_max )

#define add_bool( name, v, text, longtext, advc ) \
    add_typename_inner( CONFIG_ITEM_BOOL, name, text, longtext, advc ) \
    if (v) vlc_config_set (VLC_CONFIG_VALUE, (int64_t)true);

/* For removed option */
#define add_obsolete_inner( name, type ) \
    add_type_inner( type ) \
    vlc_config_set (VLC_CONFIG_NAME, (const char *)(name)); \
    vlc_config_set (VLC_CONFIG_REMOVED);

#define add_obsolete_bool( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_BOOL )

#define add_obsolete_integer( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_INTEGER )

#define add_obsolete_float( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_FLOAT )

#define add_obsolete_string( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_STRING )

/* Modifier macros for the config options (used for fine tuning) */

#define change_short( ch ) \
    vlc_config_set (VLC_CONFIG_SHORTCUT, (int)(ch));

#define change_string_list( list, list_text ) \
    vlc_config_set (VLC_CONFIG_LIST, \
                    (size_t)(sizeof (list) / sizeof (char *)), \
                    (const char *const *)(list), \
                    (const char *const *)(list_text));

#define change_string_cb( cb ) \
    vlc_config_set (VLC_CONFIG_LIST_CB, #cb, (void *)(cb));

#define change_integer_list( list, list_text ) \
    vlc_config_set (VLC_CONFIG_LIST, \
                    (size_t)(sizeof (list) / sizeof (int)), \
                    (const int *)(list), \
                    (const char *const *)(list_text));

#define change_integer_cb( cb ) \
    vlc_config_set (VLC_CONFIG_LIST_CB, #cb, (cb));

#define change_integer_range( minv, maxv ) \
    vlc_config_set (VLC_CONFIG_RANGE, (int64_t)(minv), (int64_t)(maxv));

#define change_float_range( minv, maxv ) \
    vlc_config_set (VLC_CONFIG_RANGE, (double)(minv), (double)(maxv));

/* For options that are saved but hidden from the preferences panel */
#define change_private() \
    vlc_config_set (VLC_CONFIG_PRIVATE);

/* For options that cannot be saved in the configuration */
#define change_volatile() \
    change_private() \
    vlc_config_set (VLC_CONFIG_VOLATILE);

#define change_safe() \
    vlc_config_set (VLC_CONFIG_SAFE);

#endif
