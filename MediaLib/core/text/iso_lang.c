/*****************************************************************************
 * iso_lang.c: function to decode language code (in dvd or a52 for instance).
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include "vlc_iso_lang.h"

#include <stdio.h>

/*****************************************************************************
 * Local tables
 *****************************************************************************/
#include "common/core/text/iso-639_def.h"

static const iso639_lang_t unknown_language =
    { "Unknown", "??", "???", "???" };

const iso639_lang_t * GetLang_1( const char * psz_code )
{
    const iso639_lang_t *p_lang;

    for( p_lang = p_languages; p_lang->psz_eng_name; p_lang++ )
        if( !strncasecmp( p_lang->psz_iso639_1, psz_code, 2 ) )
            return p_lang;

    return &unknown_language;
}

const iso639_lang_t * GetLang_2T( const char * psz_code )
{
    const iso639_lang_t *p_lang;

    for( p_lang = p_languages; p_lang->psz_eng_name; p_lang++ )
        if( !strncasecmp( p_lang->psz_iso639_2T, psz_code, 3 ) )
            return p_lang;

    return &unknown_language;
}

const iso639_lang_t * GetLang_2B( const char * psz_code )
{
    const iso639_lang_t *p_lang;

    for( p_lang = p_languages; p_lang->psz_eng_name; p_lang++ )
        if( !strncasecmp( p_lang->psz_iso639_2B, psz_code, 3 ) )
            return p_lang;

    return &unknown_language;
}

