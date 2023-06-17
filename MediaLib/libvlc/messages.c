/*****************************************************************************
 * messages.c: messages interface
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_interface.h>
#include <vlc_charset.h>

#include <stdlib.h>
#include <stdarg.h>                                       /* va_list for BSD */
//#include <unistd.h>
#include <assert.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <vlc_internal.h>

struct vlc_logger_t
{
    VLC_COMMON_MEMBERS
    vlc_rwlock_t lock;
    vlc_log_cb log;
    void *sys;
};

static void vlc_vaLogCallback(libvlc_int_t *vlc, int type,
                              const vlc_log_t *item, const char *format,
                              va_list ap)
{
    vlc_logger_t *logger = libvlc_priv(vlc)->logger;
    int canc;

    assert(logger != NULL);
    canc = vlc_savecancel();
    vlc_rwlock_rdlock(&logger->lock);
    logger->log(logger->sys, type, item, format, ap);
    vlc_rwlock_unlock(&logger->lock);
    vlc_restorecancel(canc);
}

static void vlc_LogCallback(libvlc_int_t *vlc, int type, const vlc_log_t *item,
                            const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vlc_vaLogCallback(vlc, type, item, format, ap);
    va_end(ap);
}

#ifdef _WIN32
static void Win32DebugOutputMsg(void *, int, const vlc_log_t *, const char *, va_list);
#elif defined(__ANDROID__)
static void AndroidDebugOutputMsg(void *, int, const vlc_log_t *, const char *, va_list);
#endif

/**
 * Emit a log message. This function is the variable argument list equivalent
 * to vlc_Log().
 */
void vlc_vaLog (vlc_object_t *obj, int type,
                const char *file, unsigned line, const char *func,
                const char *format, va_list args)
{
    if (obj != NULL && obj->obj.flags & OBJECT_FLAGS_QUIET)
        return;

    /* Fill message information fields */
    vlc_log_t msg;

    msg.i_object_id = (uintptr_t)obj;
    msg.psz_object_type = (obj != NULL) ? obj->obj.object_type : "generic";
    msg.psz_header = NULL;
    msg.file = file;
    msg.line = line;
    msg.func = func;
    msg.tid = vlc_thread_id();

    for (vlc_object_t *o = obj; o != NULL; o = o->obj.parent)
    {
        if (o->obj.header != NULL)
        {
            msg.psz_header = o->obj.header;
            break;
        }
    }

#ifdef _WIN32
    va_list ap;
    va_copy (ap, args);
    Win32DebugOutputMsg (NULL, type, &msg, format, ap);
    va_end (ap);
#elif defined(__ANDROID__)
    va_list ap;
    va_copy (ap, args);
    AndroidDebugOutputMsg(NULL, type, &msg, format, ap);
    va_end(ap);
#endif

    /* Pass message to the callback */
    if (obj != NULL)
        vlc_vaLogCallback(obj->obj.libvlc, type, &msg, format, args);
}

/**
 * Emit a log message.
 * \param obj VLC object emitting the message or NULL
 * \param type VLC_MSG_* message type (info, error, warning or debug)
 * \param module name of module from which the message come
 * \param file source module file name (normally __FILE__) or NULL
 * \param line function call source line number (normally __LINE__) or 0
 * \param func calling function name (normally __func__) or NULL
 * \param format printf-like message format
 */
void vlc_Log(vlc_object_t *obj, int type,
             const char *file, unsigned line, const char *func,
             const char *format, ... )
{
    va_list ap;

    va_start(ap, format);
    vlc_vaLog(obj, type, file, line, func, format, ap);
    va_end(ap);
}

#ifdef _WIN32
static const char msg_type[4][9] = { "", " error", " warning", " debug" };

static void Win32DebugOutputMsg (void* d, int type, const vlc_log_t *p_item, const char *format, va_list dol)
{
    const signed char *pverbose = d;
    if (pverbose && (*pverbose < 0 || *pverbose < (type - VLC_MSG_ERR)))
        return;

    va_list dol2;
    va_copy (dol2, dol);
    int msg_len = vsnprintf(NULL, 0, format, dol2);
    va_end (dol2);

    if (msg_len <= 0)
        return;

    char *msg = malloc(msg_len + 1 + 1);
    if (!msg)
        return;

    msg_len = vsnprintf(msg, msg_len + 1, format, dol);
    if (msg_len > 0)
    {
        if (msg[msg_len-1] != '\n')
        {
            msg[msg_len] = '\n';
            msg[msg_len + 1] = '\0';
        }
        char* psz_msg = NULL;
        if (asprintf(&psz_msg, "%s%s: %s", p_item->psz_object_type, msg_type[type], msg) > 0)
        {
            wchar_t* wmsg = ToWide(psz_msg);
            OutputDebugStringW(wmsg);
            free(wmsg);
            free(psz_msg);
        }
    }
    free(msg);
}
#elif defined(__ANDROID__)
static void AndroidDebugOutputMsg(void* d, int type, const vlc_log_t *p_item, const char *format, va_list dol)
{
    //const signed char *pverbose = d;
    //if (pverbose && (*pverbose < 0 || *pverbose < (type - VLC_MSG_ERR)))
    //    return;

    va_list dol2;
    va_copy(dol2, dol);
    int msg_len = vsnprintf(NULL, 0, format, dol2);
    va_end(dol2);

    if (msg_len <= 0)
        return;

    char *msg = malloc(msg_len + 1 + 1);
    if (!msg)
        return;

    msg_len = vsnprintf(msg, msg_len + 1, format, dol);
    if (msg_len > 0)
    {
        if (msg[msg_len - 1] != '\n')
        {
            msg[msg_len] = '\n';
            msg[msg_len + 1] = '\0';
        }
        char* psz_msg = NULL;
        if (asprintf(&psz_msg, "%s: %s", p_item->psz_object_type, msg) > 0)
        {
            int msg_type[] = {
                ANDROID_LOG_INFO,
                ANDROID_LOG_ERROR,
                ANDROID_LOG_WARN,
                ANDROID_LOG_DEBUG,
            };
            __android_log_print(msg_type[type], "LibVLC", psz_msg);
            free(psz_msg);
        }
    }
    free(msg);
}
#endif

typedef struct vlc_log_early_t
{
    struct vlc_log_early_t *next;
    int type;
    vlc_log_t meta;
    char *msg;
} vlc_log_early_t;

typedef struct
{
    vlc_mutex_t lock;
    vlc_log_early_t *head;
    vlc_log_early_t **tailp;
} vlc_logger_early_t;

static void vlc_vaLogEarly(void *d, int type, const vlc_log_t *item,
                           const char *format, va_list ap)
{
    vlc_logger_early_t *sys = d;

    vlc_log_early_t *log = malloc(sizeof (*log));
    if (unlikely(log == NULL))
        return;

    log->next = NULL;
    log->type = type;
    log->meta.i_object_id = item->i_object_id;
    /* NOTE: Object types MUST be static constant - no need to copy them. */
    log->meta.psz_object_type = item->psz_object_type;
    log->meta.psz_header = item->psz_header ? strdup(item->psz_header) : NULL;
    log->meta.file = item->file;
    log->meta.line = item->line;
    log->meta.func = item->func;

    if (vasprintf(&log->msg, format, ap) == -1)
        log->msg = NULL;

    vlc_mutex_lock(&sys->lock);
    assert(sys->tailp != NULL);
    assert(*(sys->tailp) == NULL);
    *(sys->tailp) = log;
    sys->tailp = &log->next;
    vlc_mutex_unlock(&sys->lock);
}

static int vlc_LogEarlyOpen(vlc_logger_t *logger)
{
    vlc_logger_early_t *sys = malloc(sizeof (*sys));

    if (unlikely(sys == NULL))
        return -1;

    vlc_mutex_init(&sys->lock);
    sys->head = NULL;
    sys->tailp = &sys->head;

    logger->log = vlc_vaLogEarly;
    logger->sys = sys;
    return 0;
}

static void vlc_LogEarlyClose(vlc_logger_t *logger, void *d)
{
    libvlc_int_t *vlc = logger->obj.libvlc;
    vlc_logger_early_t *sys = d;

    /* Drain early log messages */
    for (vlc_log_early_t *log = sys->head, *next; log != NULL; log = next)
    {
        vlc_LogCallback(vlc, log->type, &log->meta, "%s", (log->msg != NULL) ? log->msg : "message lost");
        free(log->msg);
        next = log->next;
        free(log);
    }

    vlc_mutex_destroy(&sys->lock);
    free(sys);
}

static void vlc_vaLogDiscard(void *d, int type, const vlc_log_t *item,
                             const char *format, va_list ap)
{
    (void) d; (void) type; (void) item; (void) format; (void) ap;
}

/**
 * Performs initialization of the messages logging subsystem.
 *
 * Early log messages will be stored in memory until the subsystem is fully
 * initialized with vlc_LogInit(). This enables logging before the
 * configuration and modules bank are ready.
 *
 * \return 0 on success, -1 on error.
 */
int vlc_LogPreinit(libvlc_int_t *vlc)
{
    vlc_logger_t *logger = vlc_custom_create(vlc, sizeof (*logger), "logger");

    libvlc_priv(vlc)->logger = logger;

    if (unlikely(logger == NULL))
        return -1;

    vlc_rwlock_init(&logger->lock);

    if (vlc_LogEarlyOpen(logger))
    {
        logger->log = vlc_vaLogDiscard;
        return -1;
    }

    return 0;
}

/**
 * Initializes the messages logging subsystem and drain the early messages to
 * the configured log.
 *
 * \return 0 on success, -1 on error.
 */
int vlc_LogInit(libvlc_int_t *vlc)
{
    vlc_logger_t *logger = libvlc_priv(vlc)->logger;
    if (unlikely(logger == NULL))
        return -1;

    vlc_log_cb cb = vlc_vaLogDiscard;
    void *sys = NULL, *early_sys = NULL;

    vlc_rwlock_wrlock(&logger->lock);
    if (logger->log == vlc_vaLogEarly)
        early_sys = logger->sys;

    logger->log = cb;
    logger->sys = sys;
    vlc_rwlock_unlock(&logger->lock);

    if (early_sys != NULL)
        vlc_LogEarlyClose(logger, early_sys);

    return 0;
}

/**
 * Sets the message logging callback.
 * \param cb message callback, or NULL to clear
 * \param data data pointer for the message callback
 */
void vlc_LogSet(libvlc_int_t *vlc, vlc_log_cb cb, void *opaque)
{
    vlc_logger_t *logger = libvlc_priv(vlc)->logger;

    if (unlikely(logger == NULL))
        return;

    void *sys;

    if (cb == NULL)
        cb = vlc_vaLogDiscard;

    vlc_rwlock_wrlock(&logger->lock);
    sys = logger->sys;

    logger->log = cb;
    logger->sys = opaque;
    vlc_rwlock_unlock(&logger->lock);
}

void vlc_LogDeinit(libvlc_int_t *vlc)
{
    vlc_logger_t *logger = libvlc_priv(vlc)->logger;

    if (unlikely(logger == NULL))
        return;

    /* Flush early log messages (corner case: no call to vlc_LogInit()) */
    if (logger->log == vlc_vaLogEarly)
    {
        logger->log = vlc_vaLogDiscard;
        vlc_LogEarlyClose(logger, logger->sys);
    }

    vlc_rwlock_destroy(&logger->lock);
    vlc_object_release(logger);
    libvlc_priv(vlc)->logger = NULL;
}
