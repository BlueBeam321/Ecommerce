/*****************************************************************************
 * vlc_iso_lang.h: function to decode language code (in dvd or a52 for instance).
 *****************************************************************************/

/**
 * \file
 * This file defines functions and structures for iso639 language codes
 */

struct iso639_lang_t
{
    const char *psz_eng_name;    /* Description in English */
    const char psz_iso639_1[3];  /* ISO-639-1 (2 characters) code */
    const char psz_iso639_2T[4]; /* ISO-639-2/T (3 characters) English code */
    const char psz_iso639_2B[4]; /* ISO-639-2/B (3 characters) native code */
};

#if defined( __cplusplus )
extern "C" {
#endif
VLC_API const iso639_lang_t * GetLang_1( const char * );
VLC_API const iso639_lang_t * GetLang_2T( const char * );
VLC_API const iso639_lang_t * GetLang_2B( const char * );
#if defined( __cplusplus )
}
#endif

