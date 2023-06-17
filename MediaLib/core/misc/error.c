/*****************************************************************************
 * error.c: error handling routine
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>

#include <string.h>

/*****************************************************************************
 * vlc_error: strerror() equivalent
 *****************************************************************************
 * This function returns a string describing the error code passed in the
 * argument. A list of all errors can be found in include/vlc_common.h.
 *****************************************************************************/
char const * vlc_error ( int i_err )
{
    switch( i_err )
    {
        case VLC_SUCCESS:
            return "no error";

        case VLC_ENOMEM:
            return "not enough memory";
        case VLC_ETIMEOUT:
            return "timeout";

        case VLC_ENOMOD:
            return "module not found";

        case VLC_ENOOBJ:
            return "object not found";

        case VLC_ENOVAR:
            return "variable not found";
        case VLC_EBADVAR:
            return "bad variable value";

        case VLC_EGENERIC:
            return "generic error";
        default:
            return "unknown error";
    }
}

#ifdef _WIN32

#ifndef WSA_QOS_EUNKNOWNPSOBJ
# define WSA_QOS_EUNKNOWNPSOBJ 11024L
#endif

typedef struct
{
    int code;
    const char *msg;
} wsaerrmsg_t;

static const wsaerrmsg_t wsaerrmsg[] =
{
    { WSAEINTR, "Interrupted function call" },
    { WSAEBADF, "File handle is not valid" },
    { WSAEACCES, "Access denied" },
    { WSAEFAULT, "Invalid memory address" },
    { WSAEINVAL, "Invalid argument" },
    { WSAEMFILE, "Too many open sockets" },
    { WSAEWOULDBLOCK, "Resource temporarily unavailable" },
    { WSAEINPROGRESS, "Operation now in progress" },
    { WSAEALREADY, "Operation already in progress" },
    { WSAENOTSOCK, "Non-socket handle specified" },
    { WSAEDESTADDRREQ, "Missing destination address" },
    { WSAEMSGSIZE, "Message too long" },
    { WSAEPROTOTYPE, "Protocol wrong type for socket", },
    { WSAENOPROTOOPT, "Option not supported by protocol" },
    { WSAEPROTONOSUPPORT, "Protocol not supported" },
    { WSAESOCKTNOSUPPORT, "Socket type not supported" },
    { WSAEOPNOTSUPP, "Operation not supported" },
    { WSAEPFNOSUPPORT, "Protocol family not supported" },
    { WSAEAFNOSUPPORT, "Address family not supported by protocol family" },
    { WSAEADDRINUSE, "Address already in use" },
    { WSAEADDRNOTAVAIL, "Cannot assign requested address" },
    { WSAENETDOWN, "Network is down" },
    { WSAENETUNREACH, "Network unreachable" },
    { WSAENETRESET, "Network dropped connection on reset" },
    { WSAECONNABORTED, "Software caused connection abort" },
    { WSAECONNRESET, "Connection reset by peer" },
    { WSAENOBUFS, "No buffer space available (not enough memory)" },
    { WSAEISCONN, "Socket is already connected" },
    { WSAENOTCONN, "Socket is not connected" },
    { WSAESHUTDOWN, "Cannot send after socket shutdown" },
    { WSAETOOMANYREFS, "Too many references" },
    { WSAETIMEDOUT, "Connection timed out" },
    { WSAECONNREFUSED, "Connection refused by peer" },
    { WSAELOOP, "Cannot translate name" },
    { WSAENAMETOOLONG, "Name too long" },
    { WSAEHOSTDOWN, "Remote host is down" },
    { WSAEHOSTUNREACH, "No route to host (unreachable)" },
    { WSAENOTEMPTY, "Directory not empty" },
    { WSAEPROCLIM, "Too many processes" },
    { WSAEUSERS, "User quota exceeded" },
    { WSAEDQUOT, "Disk quota exceeded" },
    { WSAESTALE, "Stale file handle reference" },
    { WSAEREMOTE, "Item is remote", },
    { WSASYSNOTREADY, "Network subsystem is unavailable (network stack not ready)" },
    { WSAVERNOTSUPPORTED, "Winsock.dll version out of range (network stack version not supported" },
    { WSANOTINITIALISED, "Network not initialized" },
    { WSAEDISCON, "Graceful shutdown in progress" },
    { WSAENOMORE, "No more results" },
    { WSAECANCELLED, "Call has been cancelled" },
    { WSAEINVALIDPROCTABLE, "Procedure call table is invalid" },
    { WSAEINVALIDPROVIDER, "Service provider is invalid" },
    { WSAEPROVIDERFAILEDINIT, "Service provider failed to initialize" },
    { WSASYSCALLFAILURE, "System call failure" },
    { WSASERVICE_NOT_FOUND, "Service not found" },
    { WSATYPE_NOT_FOUND, "Class type not found" },
    { WSA_E_NO_MORE, "No more results" },
    { WSA_E_CANCELLED, "Call was cancelled" },
    { WSAEREFUSED, "Database query was refused" },
    { WSAHOST_NOT_FOUND, "Host not found" },
    { WSATRY_AGAIN, "Nonauthoritative host not found (temporary hostname error)" },
    { WSANO_RECOVERY, "Non-recoverable hostname error" },
    { WSANO_DATA, "Valid name, no data record of requested type" },
    { WSA_QOS_RECEIVERS, "QOS receivers" },
    { WSA_QOS_SENDERS, "QOS senders" },
    { WSA_QOS_NO_SENDERS, "No QOS senders" },
    { WSA_QOS_NO_RECEIVERS, "QOS no receivers" },
    { WSA_QOS_REQUEST_CONFIRMED, "QOS request confirmed" },
    { WSA_QOS_ADMISSION_FAILURE, "QOS admission error" },
    { WSA_QOS_POLICY_FAILURE, "QOS policy failure" },
    { WSA_QOS_BAD_STYLE, "QOS bad style" },
    { WSA_QOS_BAD_OBJECT, "QOS bad object" },
    { WSA_QOS_TRAFFIC_CTRL_ERROR, "QOS traffic control error" },
    { WSA_QOS_GENERIC_ERROR, "QOS generic error" },
    { WSA_QOS_ESERVICETYPE, "QOS service type error" },
    { WSA_QOS_EFLOWSPEC, "QOS flowspec error" },
    { WSA_QOS_EPROVSPECBUF, "Invalid QOS provider buffer" },
    { WSA_QOS_EFILTERSTYLE, "Invalid QOS filter style" },
    { WSA_QOS_EFILTERTYPE, "Invalid QOS filter type" },
    { WSA_QOS_EFILTERCOUNT, "Incorrect QOS filter count" },
    { WSA_QOS_EOBJLENGTH, "Invalid QOS object length" },
    { WSA_QOS_EFLOWCOUNT, "Incorrect QOS flow count" },
    { WSA_QOS_EUNKNOWNPSOBJ, "Unrecognized QOS object" },
    { WSA_QOS_EPOLICYOBJ, "Invalid QOS policy object" },
    { WSA_QOS_EFLOWDESC, "Invalid QOS flow descriptor" },
    { WSA_QOS_EPSFLOWSPEC, "Invalid QOS provider-specific flowspec" },
    { WSA_QOS_EPSFILTERSPEC, "Invalid QOS provider-specific filterspec" },
    { WSA_QOS_ESDMODEOBJ, "Invalid QOS shape discard mode object" },
    { WSA_QOS_ESHAPERATEOBJ, "Invalid QOS shaping rate object" },
    { WSA_QOS_RESERVED_PETYPE, "Reserved policy QOS element type" },
    { 0, NULL }
    /* Winsock2 error codes are missing, they "never" occur */
};

const char *vlc_strerror_c(int errnum)
{
    /* C run-time errors */
    if ((unsigned)errnum < (unsigned)_sys_nerr)
        return _sys_errlist[errnum];

    /* Windows socket errors */
    for (const wsaerrmsg_t *e = wsaerrmsg; e->msg != NULL; e++)
        if (e->code == errnum)
            return e->msg;

    return "Unknown error";
}

const char *vlc_strerror(int errnum)
{
    return /*vlc_gettext*/(vlc_strerror_c(errnum));
}
#elif defined(__ANDROID__)
const char* vlc_strerror(int errnum)
{
    return vlc_strerror_c(errnum);
}

const char* vlc_strerror_c(int errnum)
{
    static __thread char android_buf[100];
    strerror_r(errnum, android_buf, 100);
    return android_buf;
}
#else
#include <errno.h>
#include <locale.h>
#include <assert.h>

static const char *vlc_strerror_l(int errnum, const char *lname)
{
    int saved_errno = errno;
    locale_t loc = newlocale(LC_MESSAGES_MASK, lname, (locale_t)0);

    if (unlikely(loc == (locale_t)0))
    {
        if (errno == ENOENT) /* fallback to POSIX locale */
            loc = newlocale(LC_MESSAGES_MASK, "C", (locale_t)0);

        if (unlikely(loc == (locale_t)0))
        {
            assert(errno != EINVAL && errno != ENOENT);
            errno = saved_errno;
            return "Error message unavailable";
        }
        errno = saved_errno;
    }

    const char *buf = strerror_l(errnum, loc);

    freelocale(loc);
    return buf;
}

/**
* Formats an error message in the current locale.
* @param errnum error number (as in errno.h)
* @return A string pointer, valid until the next call to a function of the
* strerror() family in the same thread. This function cannot fail.
*/
const char *vlc_strerror(int errnum)
{
    /* We cannot simply use strerror() here, since it is not thread-safe. */
    return vlc_strerror_l(errnum, "");
}

/**
* Formats an error message in the POSIX/C locale (i.e. American English).
* @param errnum error number (as in errno.h)
* @return A string pointer, valid until the next call to a function of the
* strerror() family in the same thread. This function cannot fail.
*/
const char *vlc_strerror_c(int errnum)
{
    return vlc_strerror_l(errnum, "C");
}
#endif