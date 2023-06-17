/*****************************************************************************
 * mime.c
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_mime.h>

#include <string.h>

static const struct
{
    const char psz_ext[8];
    const char *psz_mime;
} ext_mime[] =
{
    { ".htm",   "text/html" },
    { ".html",  "text/html" },
    { ".txt",   "text/plain" },
    { ".xml",   "text/xml" },
    { ".dtd",   "text/dtd" },

    { ".css",   "text/css" },

    /* image mime */
    { ".gif",   "image/gif" },
    { ".jpe",   "image/jpeg" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".png",   "image/png" },
    { ".pct",   "image/x-pict" },
    /* same as modules/mux/mpjpeg.c here: */
    { ".mpjpeg","multipart/x-mixed-replace; boundary=7b3cc56e5f51db803f790dad720ed50a" },

    /* media mime */
    { ".avi",   "video/avi" },
    { ".asf",   "video/x-ms-asf" },
    { ".m1a",   "audio/mpeg" },
    { ".m2a",   "audio/mpeg" },
    { ".m1v",   "video/mpeg" },
    { ".m2v",   "video/mpeg" },
    { ".mp2",   "audio/mpeg" },
    { ".mp3",   "audio/mpeg" },
    { ".mpa",   "audio/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".mpeg",  "video/mpeg" },
    { ".mpe",   "video/mpeg" },
    { ".mov",   "video/quicktime" },
    { ".moov",  "video/quicktime" },
    { ".oga",   "audio/ogg" },
    { ".ogg",   "application/ogg" },
    { ".ogm",   "application/ogg" },
    { ".ogv",   "video/ogg" },
    { ".ogx",   "application/ogg" },
    { ".opus",  "audio/ogg; codecs=opus" },
    { ".spx",   "audio/ogg" },
    { ".wav",   "audio/wav" },
    { ".wma",   "audio/x-ms-wma" },
    { ".wmv",   "video/x-ms-wmv" },
    { ".webm",  "video/webm" },

    /* end */
    { "",       "" }
};

const char *vlc_mime_Ext2Mime( const char *psz_url )
{

    char *psz_ext;

    psz_ext = strrchr( psz_url, '.' );
    if( psz_ext )
    {
        int i;

        for( i = 0; ext_mime[i].psz_ext[0] ; i++ )
        {
            if( !strcasecmp( ext_mime[i].psz_ext, psz_ext ) )
            {
                return ext_mime[i].psz_mime;
            }
        }
    }
    return "application/octet-stream";
}

