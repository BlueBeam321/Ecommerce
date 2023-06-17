/*****************************************************************************
 * getaddrinfo.c: getaddrinfo/getnameinfo replacement functions
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>

#include <stddef.h> /* size_t */
#include <string.h> /* strlen(), memcpy(), memset(), strchr() */
#include <stdlib.h> /* malloc(), free(), strtoul() */
#include <errno.h>
#include <assert.h>

#include <sys/types.h>
#include <vlc_network.h>

int vlc_getnameinfo( const struct sockaddr *sa, int salen,
                     char *host, int hostlen, int *portnum, int flags )
{
    char psz_servbuf[6], *psz_serv;
    int i_servlen, i_val;

    flags |= NI_NUMERICSERV;
    if( portnum != NULL )
    {
        psz_serv = psz_servbuf;
        i_servlen = sizeof( psz_servbuf );
    }
    else
    {
        psz_serv = NULL;
        i_servlen = 0;
    }

    i_val = getnameinfo(sa, salen, host, hostlen, psz_serv, i_servlen, flags);

    if( portnum != NULL )
        *portnum = atoi( psz_serv );

    return i_val;
}


/**
 * Resolves a host name to a list of socket addresses (like getaddrinfo()).
 *
 * @param node host name to resolve (encoded as UTF-8), or NULL
 * @param i_port port number for the socket addresses
 * @param p_hints parameters (see getaddrinfo() manual page)
 * @param res pointer set to the resulting chained list.
 * @return 0 on success, a getaddrinfo() error otherwise.
 * On failure, *res is undefined. On success, it must be freed with
 * freeaddrinfo().
 */
int vlc_getaddrinfo (const char *node, unsigned port,
                     const struct addrinfo *hints, struct addrinfo **res)
{
    char hostbuf[NI_MAXHOST], portbuf[6], *servname;

    /*
     * In VLC, we always use port number as integer rather than strings
     * for historical reasons (and portability).
     */
    if (port != 0)
    {
        if (port > 65535)
            return EAI_SERVICE;
        /* cannot overflow */
        snprintf (portbuf, sizeof (portbuf), "%u", port);
        servname = portbuf;
    }
    else
        servname = NULL;

    /*
     * VLC extensions :
     * - accept the empty string as unspecified host (i.e. NULL)
     * - ignore square brackets (for IPv6 numerals)
     */
    if (node != NULL)
    {
        if (node[0] == '[')
        {
            size_t len = strlen (node + 1);
            if ((len <= sizeof (hostbuf)) && (node[len] == ']'))
            {
                assert (len > 0);
                memcpy (hostbuf, node + 1, len - 1);
                hostbuf[len - 1] = '\0';
                node = hostbuf;
            }
        }
        if (node[0] == '\0')
            node = NULL;
    }

    return getaddrinfo (node, servname, hints, res);
}

#if defined (_WIN32) || defined (__OS2__) \
 || defined (__ANDROID__) || defined (__APPLE__) \
 || defined (__native_client__)
int vlc_getaddrinfo_i11e(const char *node, unsigned port, const struct addrinfo *hints, struct addrinfo **res)
{
    return vlc_getaddrinfo(node, port, hints, res);
}
#endif
