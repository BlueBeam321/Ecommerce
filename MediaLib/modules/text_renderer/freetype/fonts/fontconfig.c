/*****************************************************************************
 * fontconfig.c
 *****************************************************************************/

#include <assert.h>
#include <stdint.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_filter.h>                     /* filter_sys_t */

#include <fontconfig/fontconfig.h>

#include "../platform_fonts.h"

static FcConfig *config;
static uintptr_t refs;
static vlc_mutex_t lock = VLC_STATIC_MUTEX;

int FontConfig_Prepare( filter_t *p_filter )
{
    mtime_t ts;

    vlc_mutex_lock( &lock );
    if( refs++ > 0 )
    {
        vlc_mutex_unlock( &lock );
        return VLC_SUCCESS;
    }

    msg_Dbg( p_filter, "Building font databases.");
    ts = mdate();

#ifndef _WIN32
    config = FcInitLoadConfigAndFonts();
    if( unlikely(config == NULL) )
        refs = 0;

#else
    config = FcInitLoadConfig();
    if (FcConfigBuildFonts(config) == FcFalse)
        return VLC_ENOMEM;

#endif

    vlc_mutex_unlock( &lock );
    ts -= mdate();
    msg_Dbg( p_filter, "Took %ld microseconds", (long)ts );

    return (config != NULL) ? VLC_SUCCESS : VLC_EGENERIC;
}

void FontConfig_Unprepare(void)
{
    vlc_mutex_lock( &lock );
    assert( refs > 0 );
    if( --refs == 0 )
        FcConfigDestroy( config );

    vlc_mutex_unlock( &lock );
}

const vlc_family_t *FontConfig_GetFamily( filter_t *p_filter, const char *psz_family )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    char *psz_lc = ToLower( psz_family );

    if( unlikely( !psz_lc ) )
        return NULL;

    vlc_family_t *p_family =
            vlc_dictionary_value_for_key( &p_sys->family_map, psz_lc );

    if( p_family != kVLCDictionaryNotFound )
    {
        free( psz_lc );
        return p_family;
    }

    p_family = NewFamily( p_filter, psz_lc, &p_sys->p_families,
                          &p_sys->family_map, psz_lc );

    free( psz_lc );
    if( !p_family )
        return NULL;

    for( int i = 0; i < 4; ++i ) /* Iterate through FC_{SLANT,WEIGHT} combos */
    {
        bool const b_bold = i & 1;
        bool const b_italic = i & 2;

        int i_index = 0;
        FcResult result = FcResultMatch;
        FcPattern *pat, *p_pat;
        FcChar8* val_s;
        FcBool val_b;
        char *psz_fontfile = NULL;

        /* Create a pattern and fill it */
        pat = FcPatternCreate();
        if (!pat) continue;

        /* */
        FcPatternAddString( pat, FC_FAMILY, (const FcChar8*) psz_family );
        FcPatternAddBool( pat, FC_OUTLINE, FcTrue );
        FcPatternAddInteger( pat, FC_SLANT, b_italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
        FcPatternAddInteger( pat, FC_WEIGHT, b_bold ? FC_WEIGHT_EXTRABOLD : FC_WEIGHT_NORMAL );

        /* */
        FcDefaultSubstitute( pat );
        if( !FcConfigSubstitute( config, pat, FcMatchPattern ) )
        {
            FcPatternDestroy( pat );
            continue;
        }

        /* Find the best font for the pattern, destroy the pattern */
        p_pat = FcFontMatch( config, pat, &result );
        FcPatternDestroy( pat );
        if( !p_pat || result == FcResultNoMatch ) continue;

        /* Check the new pattern */
        if( ( FcResultMatch != FcPatternGetBool( p_pat, FC_OUTLINE, 0, &val_b ) )
            || ( val_b != FcTrue ) )
        {
            FcPatternDestroy( p_pat );
            continue;
        }
        if( FcResultMatch != FcPatternGetInteger( p_pat, FC_INDEX, 0, &i_index ) )
        {
            i_index = 0;
        }

        if( FcResultMatch != FcPatternGetString( p_pat, FC_FAMILY, 0, &val_s ) )
        {
            FcPatternDestroy( p_pat );
            continue;
        }

        if( FcResultMatch == FcPatternGetString( p_pat, FC_FILE, 0, &val_s ) )
            psz_fontfile = strdup( (const char*)val_s );

        FcPatternDestroy( p_pat );

        if( !psz_fontfile )
            continue;

        NewFont( psz_fontfile, i_index, b_bold, b_italic, p_family );
    }

    return p_family;
}

vlc_family_t *FontConfig_GetFallbacks( filter_t *p_filter, const char *psz_family,
                                       uni_char_t codepoint )
{

    VLC_UNUSED( codepoint );

    vlc_family_t *p_family = NULL;
    filter_sys_t *p_sys    = p_filter->p_sys;

    char *psz_lc = ToLower( psz_family );

    if( unlikely( !psz_lc ) )
        return NULL;

    p_family = vlc_dictionary_value_for_key( &p_sys->fallback_map, psz_lc );

    if( p_family != kVLCDictionaryNotFound )
    {
        free( psz_lc );
        return p_family;
    }
    else
        p_family = NULL;

    const char *psz_last_name = "";
    FcPattern  *p_pattern = FcPatternCreate();
    FcValue     family;
    family.type = FcTypeString;
    family.u.s = ( const FcChar8* ) psz_family;
    FcPatternAdd( p_pattern, FC_FAMILY, family, FcFalse );
    if( FcConfigSubstitute( config, p_pattern, FcMatchPattern ) == FcTrue )
    {
        FcDefaultSubstitute( p_pattern );
        FcResult result;
        FcFontSet* p_font_set = FcFontSort( config, p_pattern, FcTrue, NULL, &result );
        if( p_font_set )
        {
            for( int i = 0; i < p_font_set->nfont; ++i )
            {
                char* psz_name = NULL;
                FcPatternGetString( p_font_set->fonts[i],
                                    FC_FAMILY, 0, ( FcChar8** )( &psz_name ) );

                /* Avoid duplicate family names */
                if( strcasecmp( psz_last_name, psz_name ) )
                {
                    vlc_family_t *p_temp = NewFamily( p_filter, psz_name,
                                                      &p_family, NULL, NULL );

                    if( unlikely( !p_temp ) )
                    {
                        FcFontSetDestroy( p_font_set );
                        FcPatternDestroy( p_pattern );
                        if( p_family )
                            FreeFamilies( p_family, NULL );
                        free( psz_lc );
                        return NULL;
                    }

                    psz_last_name = p_temp->psz_name;
                }
            }
            FcFontSetDestroy( p_font_set );
        }
    }
    FcPatternDestroy( p_pattern );

    if( p_family )
        vlc_dictionary_insert( &p_sys->fallback_map, psz_lc, p_family );

    free( psz_lc );
    return p_family;
}
