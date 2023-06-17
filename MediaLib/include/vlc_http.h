/*****************************************************************************
 * vlc_http.h: Shared code for HTTP clients
 *****************************************************************************/

#ifndef VLC_HTTP_H
#define VLC_HTTP_H 1

/**
 * \file
 * This file defines functions, structures, enums and macros shared between
 * HTTP clients.
 */

#include <vlc_url.h>
#include <vlc_arrays.h>

/* RFC 2617: Basic and Digest Access Authentication */
typedef struct vlc_http_auth_t
{
    char *psz_realm;
    char *psz_domain;
    char *psz_nonce;
    char *psz_opaque;
    char *psz_stale;
    char *psz_algorithm;
    char *psz_qop;
    int i_nonce;
    char *psz_cnonce;
    char *psz_HA1; /* stored H(A1) value if algorithm = "MD5-sess" */
} vlc_http_auth_t;


VLC_API void vlc_http_auth_Init( vlc_http_auth_t * );
VLC_API void vlc_http_auth_Deinit( vlc_http_auth_t * );
VLC_API void vlc_http_auth_ParseWwwAuthenticateHeader
            ( vlc_object_t *, vlc_http_auth_t * , const char * );
VLC_API int vlc_http_auth_ParseAuthenticationInfoHeader
            ( vlc_object_t *, vlc_http_auth_t *,
              const char *, const char *,
              const char *, const char *,
              const char * );
VLC_API char *vlc_http_auth_FormatAuthorizationHeader
            ( vlc_object_t *, vlc_http_auth_t *,
              const char *, const char *,
              const char *, const char * ) VLC_USED;

/* RFC 6265: cookies */

typedef struct vlc_http_cookie_jar_t vlc_http_cookie_jar_t;

VLC_API vlc_http_cookie_jar_t * vlc_http_cookies_new( void ) VLC_USED;
VLC_API void vlc_http_cookies_destroy( vlc_http_cookie_jar_t * p_jar );

/**
 * Parse a value of an incoming Set-Cookie header and append the
 * cookie to the cookie jar if appropriate.
 *
 * @param jar cookie jar object
 * @param cookie header field value of Set-Cookie
 * @return true, if the cookie was added, false otherwise
 */
VLC_API bool vlc_http_cookies_store( vlc_http_cookie_jar_t *jar,
    const char *cookie, const char *host, const char *path );

/**
 * Returns a cookie value that match the given URL.
 *
 * @param p_jar a cookie jar
 * @param p_url the URL for which the cookies are returned
 * @return A string consisting of semicolon-separated cookie NAME=VALUE pairs.
 */
VLC_API char *vlc_http_cookies_fetch( vlc_http_cookie_jar_t *jar, bool secure,
                                      const char *host, const char *path );

#endif /* VLC_HTTP_H */
