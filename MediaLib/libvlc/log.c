/*****************************************************************************
 * log.c: libvlc new API log functions
 *****************************************************************************/

#include "libvlc_internal.h"
#include <vlc/vlc.h>
#include <vlc_interface.h>

#include <assert.h>
/*** Logging core dispatcher ***/

void libvlc_log_get_context(const libvlc_log_t *ctx,
                            const char ** file,
                            unsigned * line)
{
    if (file != NULL)
        *file = ctx->file;
    if (line != NULL)
        *line = ctx->line;
}

void libvlc_log_get_object(const libvlc_log_t *ctx,
                           const char ** name,
                           const char ** header,
                           uintptr_t * id)
{
    if (name != NULL)
        *name = (ctx->psz_object_type != NULL)
                ? ctx->psz_object_type : "generic";
    if (header != NULL)
        *header = ctx->psz_header;
    if (id != NULL)
        *id = ctx->i_object_id;
}

static void libvlc_logf (void *data, int level, const vlc_log_t *item,
                         const char *fmt, va_list ap)
{
    libvlc_instance_t *inst = data;

    switch (level)
    {
        case VLC_MSG_INFO: level = LIBVLC_NOTICE;  break;
        case VLC_MSG_ERR:  level = LIBVLC_ERROR;   break;
        case VLC_MSG_WARN: level = LIBVLC_WARNING; break;
        case VLC_MSG_DBG:  level = LIBVLC_DEBUG;   break;
    }

    inst->log.cb (inst->log.data, level, item, fmt, ap);
}

void libvlc_log_unset (libvlc_instance_t *inst)
{
    vlc_LogSet (inst->p_libvlc_int, NULL, NULL);
}

void libvlc_log_set (libvlc_instance_t *inst, libvlc_log_cb cb, void *data)
{
    libvlc_log_unset (inst); /* <- Barrier before modifying the callback */
    inst->log.cb = cb;
    inst->log.data = data;
    vlc_LogSet (inst->p_libvlc_int, libvlc_logf, inst);
}

/*** Helpers for logging to files ***/
static void libvlc_log_file (void *data, int level, const libvlc_log_t *log,
                             const char *fmt, va_list ap)
{
    FILE *stream = data;

    flockfile (stream);
    vfprintf (stream, fmt, ap);
    fputc ('\n', stream);
    funlockfile (stream);
    (void) level; (void) log;
}

void libvlc_log_set_file (libvlc_instance_t *inst, FILE *stream)
{
    libvlc_log_set (inst, libvlc_log_file, stream);
}

/*** Stubs for the old interface ***/
unsigned libvlc_get_log_verbosity( const libvlc_instance_t *p_instance )
{
    (void) p_instance;
    return -1;
}

void libvlc_set_log_verbosity( libvlc_instance_t *p_instance, unsigned level )
{
    (void) p_instance;
    (void) level;
}

libvlc_log_t *libvlc_log_open( libvlc_instance_t *p_instance )
{
    (void) p_instance;
    return malloc(sizeof(libvlc_log_t));
}

void libvlc_log_close( libvlc_log_t *p_log )
{
    free(p_log);
}

libvlc_log_iterator_t *libvlc_log_get_iterator( const libvlc_log_t *p_log )
{
    return (p_log != NULL) ? malloc(1) : NULL;
}

void libvlc_log_iterator_free( libvlc_log_iterator_t *p_iter )
{
    free( p_iter );
}
