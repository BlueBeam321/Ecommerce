/*****************************************************************************
 * thread.c
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_atomic.h>

#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#if defined(__SunOS)
#include <sys/processor.h>
#include <sys/pset.h>
#endif

#if defined(_WIN32) || defined(__ANDROID__)
static void vlc_cancel_addr_prepare(void *addr)
{
    /* Let thread subsystem on address to broadcast for cancellation */
    vlc_cancel_addr_set(addr);
    vlc_cleanup_push(vlc_cancel_addr_clear, addr);
    vlc_testcancel();
    vlc_cleanup_pop();
}

static void vlc_cancel_addr_finish(void *addr)
{
    vlc_cancel_addr_clear(addr);
    /* Act on cancellation as potential wake-up source */
    vlc_testcancel();
}
#endif

#if defined(_WIN32)
#include <limits.h>
#include <stdalign.h>
#include <mmsystem.h>

/*** Static mutex and condition variable ***/
static CRITICAL_SECTION super_mutex;
static CONDITION_VARIABLE super_variable;
#define IS_INTERRUPTIBLE        1

/*** Threads ***/
static DWORD thread_key;

struct vlc_thread
{
    HANDLE id;

    bool killable;
    atomic_bool killed;
    vlc_cleanup_t* cleaners;

    void* (*entry)(void*);
    void* data;

    struct
    {
        atomic_int* addr;
        CRITICAL_SECTION lock;
    } wait;
};

/*** Condition variables (low-level) ***/
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
static VOID(WINAPI *InitializeConditionVariable_)(PCONDITION_VARIABLE);
#define InitializeConditionVariable InitializeConditionVariable_
static BOOL(WINAPI *SleepConditionVariableCS_)(PCONDITION_VARIABLE,
    PCRITICAL_SECTION, DWORD);
#define SleepConditionVariableCS SleepConditionVariableCS_
static VOID(WINAPI *WakeAllConditionVariable_)(PCONDITION_VARIABLE);
#define WakeAllConditionVariable WakeAllConditionVariable_

static void WINAPI DummyConditionVariable(CONDITION_VARIABLE *cv)
{
    (void)cv;
}

static BOOL WINAPI SleepConditionVariableFallback(CONDITION_VARIABLE *cv,
    CRITICAL_SECTION *cs,
    DWORD ms)
{
    (void)cv;
    LeaveCriticalSection(cs);
    SleepEx(ms > 5 ? 5 : ms, TRUE);
    EnterCriticalSection(cs);
    return ms != 0;
}
#endif

void vlc_mutex_init(vlc_mutex_t *p_mutex)
{
    InitializeCriticalSection(&p_mutex->mutex);
    p_mutex->dynamic = true;
}

void vlc_mutex_init_recursive(vlc_mutex_t *p_mutex)
{
    InitializeCriticalSection(&p_mutex->mutex);
    p_mutex->dynamic = true;
}

void vlc_mutex_destroy(vlc_mutex_t *p_mutex)
{
    assert(p_mutex->dynamic);
    DeleteCriticalSection(&p_mutex->mutex);
}

void vlc_mutex_lock(vlc_mutex_t *p_mutex)
{
    if (!p_mutex->dynamic)
    {
        EnterCriticalSection(&super_mutex);
        while (p_mutex->locked)
        {
            p_mutex->contention++;
            SleepConditionVariableCS(&super_variable, &super_mutex, INFINITE);
            p_mutex->contention--;
        }
        p_mutex->locked = true;
        LeaveCriticalSection(&super_mutex);
        return;
    }
    EnterCriticalSection(&p_mutex->mutex);
}

int vlc_mutex_trylock(vlc_mutex_t *p_mutex)
{
    if (!p_mutex->dynamic)
    {
        int ret = EBUSY;
        EnterCriticalSection(&super_mutex);
        if (!p_mutex->locked)
        {
            p_mutex->locked = true;
            ret = 0;
        }
        LeaveCriticalSection(&super_mutex);
        return ret;
    }

    return TryEnterCriticalSection(&p_mutex->mutex) ? 0 : EBUSY;
}

void vlc_mutex_unlock(vlc_mutex_t *p_mutex)
{
    if (!p_mutex->dynamic)
    {
        EnterCriticalSection(&super_mutex);
        assert(p_mutex->locked);
        p_mutex->locked = false;
        if (p_mutex->contention)
            WakeAllConditionVariable(&super_variable);
        LeaveCriticalSection(&super_mutex);
        return;
    }

    LeaveCriticalSection(&p_mutex->mutex);
}

static int cond_wait_delay(vlc_cond_t *cond, vlc_mutex_t *mutex, mtime_t delay)
{
    atomic_uint value = atomic_load_explicit(&cond->value, memory_order_relaxed);
    while (value & 1)
    {
        if (atomic_compare_exchange_weak_explicit(&cond->value, &value,
            value + 1,
            memory_order_relaxed,
            memory_order_relaxed))
            value++;
    }

    vlc_cancel_addr_prepare(&cond->value);
    vlc_mutex_unlock(mutex);

    if (delay > 0)
        value = vlc_addr_timedwait(&cond->value, value, delay);
    else
        value = 0;

    vlc_mutex_lock(mutex);
    vlc_cancel_addr_finish(&cond->value);

    return value ? 0 : ETIMEDOUT;
}

void vlc_cond_init (vlc_cond_t *p_condvar)
{
    atomic_init(&p_condvar->value, 0);
}

void vlc_cond_init_daytime (vlc_cond_t *p_condvar)
{
    vlc_cond_init(p_condvar);
}

void vlc_cond_destroy (vlc_cond_t *p_condvar)
{
    (void) p_condvar;
}

void vlc_cond_signal (vlc_cond_t *p_condvar)
{
    atomic_fetch_or_explicit(&p_condvar->value, 1, memory_order_relaxed);
    vlc_addr_signal(&p_condvar->value);
}

void vlc_cond_broadcast (vlc_cond_t *p_condvar)
{
    atomic_fetch_or_explicit(&p_condvar->value, 1, memory_order_relaxed);
    vlc_addr_broadcast(&p_condvar->value);
}

void vlc_cond_wait (vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex)
{
    atomic_uint value = atomic_load_explicit(&p_condvar->value, memory_order_relaxed);
    while (value & 1)
    {
        if (atomic_compare_exchange_weak_explicit(&p_condvar->value, &value,
            value + 1,
            memory_order_relaxed,
            memory_order_relaxed))
            value++;
    }

    vlc_cancel_addr_prepare(&p_condvar->value);
    vlc_mutex_unlock(p_mutex);

    vlc_addr_wait(&p_condvar->value, value);

    vlc_mutex_lock(p_mutex);
    vlc_cancel_addr_finish(&p_condvar->value);
}

int vlc_cond_timedwait (vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex, mtime_t deadline)
{
    return cond_wait_delay(p_condvar, p_mutex, deadline - mdate());
}

int vlc_cond_timedwait_daytime (vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex, time_t deadline)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    deadline -= ts.tv_sec * CLOCK_FREQ;
    deadline -= ts.tv_nsec / (1000000000 / CLOCK_FREQ);

    return cond_wait_delay(p_condvar, p_mutex, deadline);
}

#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
static inline HANDLE *vlc_sem_handle_p(vlc_sem_t *sem)
{
    return (HANDLE *)sem;
}
#define vlc_sem_handle(sem) (*vlc_sem_handle_p(sem))
#endif

#define READER_MASK LONG_MAX
#define WRITER_BIT  LONG_MIN

void vlc_sem_init(vlc_sem_t *sem, unsigned value)
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    HANDLE handle = CreateSemaphore(NULL, value, 0x7fffffff, NULL);
    if (handle == NULL)
        abort();

    vlc_sem_handle(sem) = handle;
#else
    vlc_mutex_init (&sem->lock);
    vlc_cond_init (&sem->wait);
    sem->value = value;
#endif
}

void vlc_sem_destroy(vlc_sem_t *sem)
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    CloseHandle(vlc_sem_handle(sem));
#else
    vlc_cond_destroy(&sem->wait);
    vlc_mutex_destroy (&sem->lock);
#endif
}

int vlc_sem_post(vlc_sem_t *sem)
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    ReleaseSemaphore(vlc_sem_handle(sem), 1, NULL);
    return 0; /* FIXME */
#else
    int ret = 0;
    vlc_mutex_lock (&sem->lock);
    if (likely(sem->value != UINT_MAX))
        sem->value++;
    else
        ret = EOVERFLOW;
    vlc_mutex_unlock (&sem->lock);
    vlc_cond_signal(&sem->wait);
    return ret;
#endif
}

void vlc_sem_wait(vlc_sem_t *sem)
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    HANDLE handle = vlc_sem_handle(sem);
    DWORD result;

    do
    {
        vlc_testcancel();
        result = WaitForSingleObjectEx(handle, INFINITE, TRUE);

        /* Semaphore abandoned would be a bug. */
        assert(result != WAIT_ABANDONED_0);
    } while (result == WAIT_IO_COMPLETION || result == WAIT_FAILED);
#else
    vlc_mutex_lock(&sem->lock);
    mutex_cleanup_push(&sem->lock);
    while (!sem->value)
        vlc_cond_wait(&sem->wait, &sem->lock);
    sem->value--;
    vlc_cleanup_pop();
    vlc_mutex_unlock(&sem->lock);
#endif
}

void vlc_rwlock_init(vlc_rwlock_t *lock)
{
    vlc_mutex_init(&lock->mutex);
    vlc_cond_init (&lock->wait);
    lock->state = 0;
}

void vlc_rwlock_destroy (vlc_rwlock_t *lock)
{
    vlc_cond_destroy(&lock->wait);
    vlc_mutex_destroy (&lock->mutex);
}

void vlc_rwlock_rdlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock (&lock->mutex);
    /* Recursive read-locking is allowed.
    * Ensure that there is no active writer. */
    while (lock->state < 0)
    {
        assert(lock->state == WRITER_BIT);
        mutex_cleanup_push(&lock->mutex);
        vlc_cond_wait(&lock->wait, &lock->mutex);
        vlc_cleanup_pop();
    }
    if (unlikely(lock->state >= READER_MASK))
        abort(); /* An overflow is certainly a recursion bug. */
    lock->state++;
    vlc_mutex_unlock(&lock->mutex);
}

void vlc_rwlock_wrlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock(&lock->mutex);
    /* Wait until nobody owns the lock in any way. */
    while (lock->state != 0)
    {
        mutex_cleanup_push(&lock->mutex);
        vlc_cond_wait(&lock->wait, &lock->mutex);
        vlc_cleanup_pop();
    }
    lock->state = WRITER_BIT;
    vlc_mutex_unlock(&lock->mutex);
}

void vlc_rwlock_unlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock(&lock->mutex);
    if (lock->state < 0)
    {   /* Write unlock */
        assert(lock->state == WRITER_BIT);
        /* Let reader and writer compete. OS scheduler decides who wins. */
        lock->state = 0;
        vlc_cond_broadcast(&lock->wait);
    }
    else
    {   /* Read unlock */
        assert(lock->state > 0);
        /* If there are no readers left, wake up one pending writer. */
        if (--lock->state == 0)
            vlc_cond_signal(&lock->wait);
    }
    vlc_mutex_unlock(&lock->mutex);
}

struct vlc_threadvar
{
    DWORD                 id;
    void(*destroy) (void *);
    struct vlc_threadvar *prev;
    struct vlc_threadvar *next;
} *vlc_threadvar_last = NULL;

int vlc_threadvar_create(vlc_threadvar_t *p_tls, void(*destr) (void *))
{
    struct vlc_threadvar *var = malloc (sizeof (*var));
    if (unlikely(var == NULL))
        return errno;

    var->id = TlsAlloc();
    if (var->id == TLS_OUT_OF_INDEXES)
    {
        free(var);
        return EAGAIN;
    }
    var->destroy = destr;
    var->next = NULL;
    *p_tls = var;

    EnterCriticalSection(&super_mutex);
    var->prev = vlc_threadvar_last;
    if (var->prev)
        var->prev->next = var;

    vlc_threadvar_last = var;
    LeaveCriticalSection(&super_mutex);
    return 0;
}

void vlc_threadvar_delete (vlc_threadvar_t *p_tls)
{
    struct vlc_threadvar *var = *p_tls;

    EnterCriticalSection(&super_mutex);
    if (var->prev != NULL)
        var->prev->next = var->next;

    if (var->next != NULL)
        var->next->prev = var->prev;
    else
        vlc_threadvar_last = var->prev;

    LeaveCriticalSection(&super_mutex);

    TlsFree(var->id);
    free(var);
}

int vlc_threadvar_set (vlc_threadvar_t key, void *value)
{
    int saved = GetLastError();
    if (!TlsSetValue(key->id, value))
        return ENOMEM;
    SetLastError(saved);
    return 0;
}

void *vlc_threadvar_get (vlc_threadvar_t key)
{
    int saved = GetLastError();
    void *value = TlsGetValue(key->id);
    SetLastError(saved);
    return value;
}

static void vlc_thread_destroy(vlc_thread_t th)
{
    DeleteCriticalSection(&th->wait.lock);
    free(th);
}

static unsigned __stdcall vlc_entry (void *p)
{
    struct vlc_thread *th = p;

    TlsSetValue(thread_key, th);
    th->killable = true;
    th->data = th->entry (th->data);
    TlsSetValue(thread_key, NULL);

    if (th->id == NULL) /* Detached thread */
        vlc_thread_destroy(th);
    return 0;
}

static int vlc_clone_attr(vlc_thread_t *p_handle, bool detached, void *(*entry) (void *), void *data, char* th_name, int priority)
{
    struct vlc_thread *th = malloc (sizeof (*th));
    if (unlikely(th == NULL))
        return ENOMEM;
    th->entry = entry;
    th->data = data;
    th->killable = false;
    atomic_init(&th->killed, false);
    th->cleaners = NULL;
    th->wait.addr = NULL;
    InitializeCriticalSection(&th->wait.lock);

    /* When using the MSVCRT C library you have to use the _beginthreadex
    * function instead of CreateThread, otherwise you'll end up with
    * memory leaks and the signal functions not working (see Microsoft
    * Knowledge Base, article 104641) */
    uintptr_t h = _beginthreadex (NULL, 0, vlc_entry, th, 0, NULL);
    if (h == 0)
    {
        int err = errno;
        free (th);
        return err;
    }

    if (detached)
    {
        CloseHandle((HANDLE)h);
        th->id = NULL;
    }
    else
        th->id = (HANDLE)h;

    if (p_handle != NULL)
        *p_handle = th;

    if (priority)
        SetThreadPriority (th->id, priority);

    return 0;
}

#if IS_INTERRUPTIBLE
/* APC procedure for thread cancellation */
static void CALLBACK vlc_cancel_self (ULONG_PTR self)
{
    (void) self;
}
#endif

int vlc_clone(vlc_thread_t *th, void *(*entry) (void *), void *data, char* th_name, int priority)
{
    return vlc_clone_attr(th, false, entry, data, th_name, priority);
}

void vlc_join(vlc_thread_t th, void **result)
{
    DWORD ret;
    do {
        vlc_testcancel();
        ret = WaitForSingleObjectEx(th->id, INFINITE, TRUE);
        assert(ret != WAIT_ABANDONED_0);
    } while (ret == WAIT_IO_COMPLETION || ret == WAIT_FAILED);

    if (result != NULL)
        *result = th->data;
    CloseHandle(th->id);
    vlc_thread_destroy(th);
}

int vlc_clone_detach(vlc_thread_t *th, void *(*entry) (void *), void *data, int priority)
{
    vlc_thread_t th2;
    if (th == NULL)
        th = &th2;

    return vlc_clone_attr(th, true, entry, data, NULL, priority);
}

vlc_thread_t vlc_thread_self (void)
{
    return TlsGetValue(thread_key);
}

unsigned long vlc_thread_id (void)
{
    return GetCurrentThreadId();
}

int vlc_set_priority (vlc_thread_t th, int priority)
{
    if (!SetThreadPriority(th->id, priority))
        return VLC_EGENERIC;
    return VLC_SUCCESS;
}

void vlc_cancel(vlc_thread_t th)
{
    atomic_store_explicit(&th->killed, true, memory_order_relaxed);

    EnterCriticalSection(&th->wait.lock);
    if (th->wait.addr != NULL)
    {
        atomic_fetch_or_explicit(th->wait.addr, 1, memory_order_relaxed);
        vlc_addr_broadcast(th->wait.addr);
    }
    LeaveCriticalSection(&th->wait.lock);

#if IS_INTERRUPTIBLE
    QueueUserAPC(vlc_cancel_self, th->id, (uintptr_t)th);
#endif
}

int vlc_savecancel (void)
{
    struct vlc_thread *th = vlc_thread_self();
    if (th == NULL)
        return false; /* Main thread - cannot be canceled anyway */

    int state = th->killable;
    th->killable = false;
    return state;
}

void vlc_restorecancel (int state)
{
    struct vlc_thread *th = vlc_thread_self();
    assert (state == false || state == true);

    if (th == NULL)
        return; /* Main thread - cannot be canceled anyway */

    assert (!th->killable);
    th->killable = state != 0;
}

void vlc_testcancel (void)
{
    struct vlc_thread *th = vlc_thread_self();
    if (th == NULL)
        return; /* Main thread - cannot be canceled anyway */
    if (!th->killable)
        return;
    if (!atomic_load_explicit(&th->killed, memory_order_relaxed))
        return;

    th->killable = true; /* Do not re-enter cancellation cleanup */

    for (vlc_cleanup_t *p = th->cleaners; p != NULL; p = p->next)
        p->proc(p->data);

    th->data = NULL; /* TODO: special value? */
    if (th->id == NULL) /* Detached thread */
        vlc_thread_destroy(th);
    _endthreadex(0);
}

void vlc_control_cancel (int cmd, ...)
{
    va_list ap;

    struct vlc_thread *th = vlc_thread_self();
    if (th == NULL)
        return; /* Main thread - cannot be canceled anyway */

    va_start(ap, cmd);
    switch (cmd)
    {
    case VLC_CLEANUP_PUSH:
    {
        /* cleaner is a pointer to the caller stack, no need to allocate
        * and copy anything. As a nice side effect, this cannot fail. */
        vlc_cleanup_t *cleaner = va_arg(ap, vlc_cleanup_t *);
        cleaner->next = th->cleaners;
        th->cleaners = cleaner;
        break;
    }

    case VLC_CLEANUP_POP:
    {
        th->cleaners = th->cleaners->next;
        break;
    }

    case VLC_CANCEL_ADDR_SET:
    {
        void *addr = va_arg(ap, void *);

        EnterCriticalSection(&th->wait.lock);
        assert(th->wait.addr == NULL);
        th->wait.addr = addr;
        LeaveCriticalSection(&th->wait.lock);
        break;
    }

    case VLC_CANCEL_ADDR_CLEAR:
    {
        void *addr = va_arg(ap, void *);
        EnterCriticalSection(&th->wait.lock);
        assert(th->wait.addr == addr);
        th->wait.addr = NULL;
        LeaveCriticalSection(&th->wait.lock);
        break;
    }
    }
    va_end(ap);
}

static union
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN7)
    struct
    {
        BOOL(*query) (PULONGLONG);
    } interrupt;
#endif
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    struct
    {
        ULONGLONG(*get) (void);
    } tick;
#endif
    struct
    {
        LARGE_INTEGER freq;
    } perf;

    struct
    {
        MMRESULT(WINAPI *timeGetDevCaps)(LPTIMECAPS ptc, UINT cbtc);
        DWORD(WINAPI *timeGetTime)(void);
    } multimedia;
} clk;

static mtime_t mdate_interrupt(void)
{
    ULONGLONG ts;
    BOOL ret;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    ret = QueryUnbiasedInterruptTime(&ts);
#else
    ret = clk.interrupt.query(&ts);
#endif
    if (unlikely(!ret))
        abort();

    /* hundreds of nanoseconds */
    return ts / (10000000 / CLOCK_FREQ);
}

static mtime_t mdate_tick(void)
{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    ULONGLONG ts = GetTickCount64();
#else
    ULONGLONG ts = clk.tick.get();
#endif

    /* milliseconds */
    return ts * (CLOCK_FREQ / 1000);
}

static mtime_t mdate_multimedia(void)
{
    DWORD ts = clk.multimedia.timeGetTime();

    /* milliseconds */
    return (mtime_t)ts * (CLOCK_FREQ / 1000);
}

static mtime_t mdate_perf(void)
{
    /* We don't need the real date, just the value of a high precision timer */
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter))
        abort();

    /* Convert to from (1/freq) to microsecond resolution */
    /* We need to split the division to avoid 63-bits overflow */
    lldiv_t d = lldiv(counter.QuadPart, clk.perf.freq.QuadPart);

    return (d.quot * 1000000) + ((d.rem * 1000000) / clk.perf.freq.QuadPart);
}

static mtime_t mdate_wall(void)
{
    FILETIME ts;
    ULARGE_INTEGER s;

#if 0 //(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    GetSystemTimePreciseAsFileTime(&ts);
#else
    GetSystemTimeAsFileTime(&ts);
#endif
    s.LowPart = ts.dwLowDateTime;
    s.HighPart = ts.dwHighDateTime;
    /* hundreds of nanoseconds */
    return s.QuadPart / (10000000 / CLOCK_FREQ);
}

static mtime_t mdate_default(void)
{
    vlc_threads_setup(NULL);
    return mdate_perf();
}

static mtime_t(*mdate_selected) (void) = mdate_default;


mtime_t mdate(void)
{
    return mdate_selected();
}

#undef mwait
void mwait (mtime_t deadline)
{
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    mtime_t delay;

    vlc_testcancel();
    while ((delay = (deadline - mdate())) > 0)
    {
        delay = (delay + 999) / 1000;
        if (unlikely(delay > 0x7fffffff))
            delay = 0x7fffffff;

        SleepEx(delay, TRUE);
        vlc_testcancel();
    }
#else
    mtime_t delay;
    atomic_int value = ATOMIC_VAR_INIT(0);
    vlc_cancel_addr_prepare(&value);
    while ((delay = (deadline - mdate())) > 0)
    {
        vlc_addr_timedwait(&value, 0, delay);
        vlc_testcancel();
    }
    vlc_cancel_addr_finish(&value);
#endif
}

#undef msleep
void msleep (mtime_t delay)
{
    mwait(mdate() + delay);
}

unsigned vlc_GetCPUCount(void)
{
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);
    return systemInfo.dwNumberOfProcessors;
}

#if 1 //(_WIN32_WINNT < _WIN32_WINNT_WIN8)
static BOOL(WINAPI *WaitOnAddress_)(VOID volatile *, PVOID, SIZE_T, DWORD);
#define WaitOnAddress (*WaitOnAddress_)
static VOID(WINAPI *WakeByAddressAll_)(PVOID);
#define WakeByAddressAll (*WakeByAddressAll_)
static VOID(WINAPI *WakeByAddressSingle_)(PVOID);
#define WakeByAddressSingle (*WakeByAddressSingle_)

static struct wait_addr_bucket
{
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE wait;
} wait_addr_buckets[32];

static struct wait_addr_bucket *wait_addr_get_bucket(void volatile *addr)
{
    uintptr_t u = (uintptr_t)addr;

    return wait_addr_buckets + ((u >> 3) % ARRAY_SIZE(wait_addr_buckets));
}

static void vlc_wait_addr_init(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(wait_addr_buckets); i++)
    {
        struct wait_addr_bucket *bucket = wait_addr_buckets + i;

        InitializeCriticalSection(&bucket->lock);
        InitializeConditionVariable(&bucket->wait);
    }
}

static void vlc_wait_addr_deinit(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(wait_addr_buckets); i++)
    {
        struct wait_addr_bucket *bucket = wait_addr_buckets + i;

        DeleteCriticalSection(&bucket->lock);
    }
}

static BOOL WINAPI WaitOnAddressFallback(void volatile *addr, void *value,
    SIZE_T size, DWORD ms)
{
    struct wait_addr_bucket *bucket = wait_addr_get_bucket(addr);
    uint64_t futex, val = 0;
    BOOL ret = 0;

    EnterCriticalSection(&bucket->lock);

    switch (size)
    {
    case 1:
        futex = atomic_load_explicit((atomic_char *)addr,
            memory_order_relaxed);
        val = *(const char *)value;
        break;
    case 2:
        futex = atomic_load_explicit((atomic_short *)addr,
            memory_order_relaxed);
        val = *(const short *)value;
        break;
    case 4:
        futex = atomic_load_explicit((atomic_int *)addr,
            memory_order_relaxed);
        val = *(const int *)value;
        break;
    case 8:
        futex = atomic_load_explicit((atomic_llong *)addr,
            memory_order_relaxed);
        val = *(const long long *)value;
        break;
    default:
        vlc_assert_unreachable();
    }

    if (futex == val)
        ret = SleepConditionVariableCS(&bucket->wait, &bucket->lock, ms);

    LeaveCriticalSection(&bucket->lock);
    return ret;
}

static void WINAPI WakeByAddressFallback(void *addr)
{
    struct wait_addr_bucket *bucket = wait_addr_get_bucket(addr);

    /* Acquire the bucket critical section (only) to enforce proper sequencing.
    * The critical section does not protect any actual memory object. */
    EnterCriticalSection(&bucket->lock);
    /* No other threads can hold the lock for this bucket while it is held
    * here. Thus any other thread either:
    * - is already sleeping in SleepConditionVariableCS(), and to be woken up
    *   by the following WakeAllConditionVariable(), or
    * - has yet to retrieve the value at the wait address (with the
    *   'switch (size)' block). */
    LeaveCriticalSection(&bucket->lock);
    /* At this point, other threads can retrieve the value at the wait address.
    * But the value will have already been changed by our call site, thus
    * (futex == val) will be false, and the threads will not go to sleep. */

    /* Wake up any thread that was already sleeping. Since there are more than
    * one wait address per bucket, all threads must be woken up :-/ */
    WakeAllConditionVariable(&bucket->wait);
}
#endif

void vlc_addr_wait(void *addr, unsigned val)
{
    WaitOnAddress(addr, &val, sizeof(val), -1);
}

bool vlc_addr_timedwait(void *addr, unsigned val, mtime_t delay)
{
    delay = (delay + 999) / 1000;

    if (delay > 0x7fffffff)
    {
        WaitOnAddress(addr, &val, sizeof(val), 0x7fffffff);
        return true; /* woke up early, claim spurious wake-up */
    }

    return WaitOnAddress(addr, &val, sizeof(val), delay);
}

void vlc_addr_signal(void *addr)
{
    WakeByAddressSingle(addr);
}

void vlc_addr_broadcast(void *addr)
{
    WakeByAddressAll(addr);
}

static CRITICAL_SECTION setup_lock; /* FIXME: use INIT_ONCE */

static BOOL SelectClockSource(void *data)
{
    vlc_object_t *obj = data;
    const char *name = "multimedia";
    char *str = NULL;
    if (str != NULL)
        name = str;
    if (!strcmp(name, "interrupt"))
    {
        msg_Dbg(obj, "using interrupt time as clock source");
#if (_WIN32_WINNT < _WIN32_WINNT_WIN7)
        HANDLE h = GetModuleHandle(_T("kernel32.dll"));
        if (unlikely(h == NULL))
            return FALSE;
        clk.interrupt.query = (void *)GetProcAddress(h,
            "QueryUnbiasedInterruptTime");
        if (unlikely(clk.interrupt.query == NULL))
            abort();
#endif
        mdate_selected = mdate_interrupt;
    }
    else if (!strcmp(name, "tick"))
    {
        msg_Dbg(obj, "using Windows time as clock source");
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
        HANDLE h = GetModuleHandle(_T("kernel32.dll"));
        if (unlikely(h == NULL))
            return FALSE;
        clk.tick.get = (void *)GetProcAddress(h, "GetTickCount64");
        if (unlikely(clk.tick.get == NULL))
            return FALSE;
#endif
        mdate_selected = mdate_tick;
    }
    else if (!strcmp(name, "multimedia"))
    {
        TIMECAPS caps;
        MMRESULT(WINAPI * timeBeginPeriod)(UINT);

        HMODULE hWinmm = LoadLibrary(TEXT("winmm.dll"));
        if (!hWinmm)
            goto perf;

        clk.multimedia.timeGetDevCaps = (void*)GetProcAddress(hWinmm, "timeGetDevCaps");
        clk.multimedia.timeGetTime = (void*)GetProcAddress(hWinmm, "timeGetTime");
        if (!clk.multimedia.timeGetDevCaps || !clk.multimedia.timeGetTime)
            goto perf;

        msg_Dbg(obj, "using multimedia timers as clock source");
        if (clk.multimedia.timeGetDevCaps(&caps, sizeof(caps)) != MMSYSERR_NOERROR)
            goto perf;
        msg_Dbg(obj, " min period: %u ms, max period: %u ms",
            caps.wPeriodMin, caps.wPeriodMax);
        mdate_selected = mdate_multimedia;

        timeBeginPeriod = (void*)GetProcAddress(hWinmm, "timeBeginPeriod");
        if (timeBeginPeriod != NULL)
            timeBeginPeriod(5);
    }
    else if (!strcmp(name, "perf"))
    {
    perf:
        msg_Dbg(obj, "using performance counters as clock source");
        if (!QueryPerformanceFrequency(&clk.perf.freq))
            abort();
        msg_Dbg(obj, " frequency: %llu Hz", clk.perf.freq.QuadPart);
        mdate_selected = mdate_perf;
    }
    else if (!strcmp(name, "wall"))
    {
        msg_Dbg(obj, "using system time as clock source");
        mdate_selected = mdate_wall;
    }
    else
    {
        msg_Err(obj, "invalid clock source \"%s\"", name);
        abort();
    }
    free(str);
    return TRUE;
}

void vlc_threads_setup(vlc_object_t* vlc)
{
    EnterCriticalSection(&setup_lock);
    if (mdate_selected != mdate_default)
    {
        LeaveCriticalSection(&setup_lock);
        return;
    }

    if (!SelectClockSource((vlc != NULL) ? VLC_OBJECT(vlc) : NULL))
        abort();
    assert(mdate_selected != mdate_default);

    /*if (var_InheritBool(vlc, "high-priority"))
    {
        if (SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)
            || SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
            msg_Dbg(vlc, "raised process priority");
        else
            msg_Dbg(vlc, "could not raise process priority");
    }*/

    LeaveCriticalSection(&setup_lock);
}

#define LOOKUP(s) (((s##_) = (void *)GetProcAddress(h, #s)) != NULL)
extern vlc_rwlock_t config_lock;
void vlc_thread_win32_init()
{
#if 1 //(_WIN32_WINNT < _WIN32_WINNT_WIN8)
    HANDLE h = GetModuleHandle(TEXT("kernel32.dll"));
    if (unlikely(h == NULL))
        return FALSE;

    if (!LOOKUP(WaitOnAddress)
        || !LOOKUP(WakeByAddressAll) || !LOOKUP(WakeByAddressSingle))
    {
# if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
        if (!LOOKUP(InitializeConditionVariable)
            || !LOOKUP(SleepConditionVariableCS)
            || !LOOKUP(WakeAllConditionVariable))
        {
            InitializeConditionVariable_ = DummyConditionVariable;
            SleepConditionVariableCS_ = SleepConditionVariableFallback;
            WakeAllConditionVariable_ = DummyConditionVariable;
        }
# endif
        vlc_wait_addr_init();
        WaitOnAddress_ = WaitOnAddressFallback;
        WakeByAddressAll_ = WakeByAddressFallback;
        WakeByAddressSingle_ = WakeByAddressFallback;
    }
#endif
    thread_key = TlsAlloc();
    if (unlikely(thread_key == TLS_OUT_OF_INDEXES))
        return FALSE;
    InitializeCriticalSection(&setup_lock);
    InitializeCriticalSection(&super_mutex);
    InitializeConditionVariable(&super_variable);
    vlc_rwlock_init(&config_lock);
    vlc_CPU_init();
}

void vlc_thread_win32_deinit()
{
    vlc_rwlock_destroy(&config_lock);
    DeleteCriticalSection(&super_mutex);
    DeleteCriticalSection(&setup_lock);
    TlsFree(thread_key);
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    if (WaitOnAddress_ == WaitOnAddressFallback)
        vlc_wait_addr_deinit();
#endif
}

#elif defined(__ANDROID__)
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_WAKE_PRIVATE FUTEX_WAKE
#define FUTEX_WAIT_PRIVATE FUTEX_WAIT
#endif

struct vlc_thread
{
    pthread_t thread;
    vlc_sem_t finished;

    void *(*entry)(void*);
    void *data;

    struct
    {
        void *addr; /// Non-null if waiting on futex
        vlc_mutex_t lock ; /// Protects futex address
    } wait;

    atomic_bool killed;
    bool killable;
};
static thread_local struct vlc_thread* thread = NULL;

#ifndef NDEBUG
static void vlc_thread_fatal_print (const char *action, int error, const char *function, const char *file, unsigned line)
{
    char buf[1000];
    const char *msg;

    switch (strerror_r (error, buf, sizeof (buf)))
    {
    case 0:
        msg = buf;
        break;
    case ERANGE: /* should never happen */
        msg = "unknown (too big to display)";
        break;
    default:
        msg = "unknown (invalid error number)";
        break;
    }

    msg_Err((vlc_object_t*)NULL, "LibVLC fatal error %s (%d) in thread %lu "
        "at %s:%u in %s\n Error message: %s\n",
        action, error, vlc_thread_id (), file, line, function, msg);
    //fflush (stderr);
}

# define VLC_THREAD_ASSERT( action ) do { \
    if (unlikely(val)) { \
        vlc_thread_fatal_print (action, val, __FUNCTION__, __FILE__, __LINE__); \
        assert (!action); \
            } \
} while(0)
#else
# define VLC_THREAD_ASSERT( action ) ((void)val)
#endif

void vlc_mutex_init( vlc_mutex_t *p_mutex )
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init (&attr);
#ifdef NDEBUG
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_DEFAULT);
#else
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    pthread_mutex_init (p_mutex, &attr);
    pthread_mutexattr_destroy( &attr );
}

void vlc_mutex_init_recursive( vlc_mutex_t *p_mutex )
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init (p_mutex, &attr);
    pthread_mutexattr_destroy( &attr );
}

void vlc_mutex_destroy (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_destroy( p_mutex );
    VLC_THREAD_ASSERT ("destroying mutex");
}

#ifndef NDEBUG
void vlc_assert_locked (vlc_mutex_t *p_mutex)
{
    assert (pthread_mutex_lock (p_mutex) == EDEADLK);
}
#endif

void vlc_mutex_lock (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_lock( p_mutex );
    VLC_THREAD_ASSERT ("locking mutex");
}

int vlc_mutex_trylock (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_trylock( p_mutex );

    if (val != EBUSY)
        VLC_THREAD_ASSERT ("locking mutex");
    return val;
}

void vlc_mutex_unlock (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_unlock( p_mutex );
    VLC_THREAD_ASSERT ("unlocking mutex");
}

static inline atomic_uint *vlc_cond_value(vlc_cond_t *cond)
{
    return (atomic_uint *)&cond->value;
}

void vlc_cond_init(vlc_cond_t *cond)
{
    /* Initial value is irrelevant but set it for happy debuggers */
    atomic_init(vlc_cond_value(cond), 0);
}

void vlc_cond_init_daytime(vlc_cond_t *cond)
{
    vlc_cond_init(cond);
}

void vlc_cond_destroy(vlc_cond_t *cond)
{
    (void) cond;
}

void vlc_cond_signal(vlc_cond_t *cond)
{
    atomic_fetch_or_explicit(vlc_cond_value(cond), 1, memory_order_relaxed);
    vlc_addr_signal(&cond->value);
}

void vlc_cond_broadcast(vlc_cond_t *cond)
{
    atomic_fetch_or_explicit(vlc_cond_value(cond), 1, memory_order_relaxed);
    vlc_addr_broadcast(&cond->value);
}

void vlc_cond_wait(vlc_cond_t *cond, vlc_mutex_t *mutex)
{
    unsigned int value = atomic_load_explicit(vlc_cond_value(cond), memory_order_relaxed);
    while (value & 1)
    {
        if (atomic_compare_exchange_weak_explicit(vlc_cond_value(cond), &value,
            value + 1,
            memory_order_relaxed,
            memory_order_relaxed))
            value++;
    }

    vlc_cancel_addr_prepare(&cond->value);
    vlc_mutex_unlock(mutex);

    vlc_addr_wait(&cond->value, value);

    vlc_mutex_lock(mutex);
    vlc_cancel_addr_finish(&cond->value);
}

static int vlc_cond_wait_delay(vlc_cond_t *cond, vlc_mutex_t *mutex, mtime_t delay)
{
    unsigned value = atomic_load_explicit(vlc_cond_value(cond),
        memory_order_relaxed);
    while (value & 1)
    {
        if (atomic_compare_exchange_weak_explicit(vlc_cond_value(cond), &value,
            value + 1,
            memory_order_relaxed,
            memory_order_relaxed))
            value++;
    }

    vlc_cancel_addr_prepare(&cond->value);
    vlc_mutex_unlock(mutex);

    if (delay > 0)
        value = vlc_addr_timedwait(&cond->value, value, delay);
    else
        value = 0;

    vlc_mutex_lock(mutex);
    vlc_cancel_addr_finish(&cond->value);

    return value ? 0 : ETIMEDOUT;
}

int vlc_cond_timedwait(vlc_cond_t *cond, vlc_mutex_t *mutex, mtime_t deadline)
{
    return vlc_cond_wait_delay(cond, mutex, deadline - mdate());
}

int vlc_cond_timedwait_daytime(vlc_cond_t *cond, vlc_mutex_t *mutex,
    time_t deadline)
{
    struct timespec ts;

    timespec_get(&ts, TIME_UTC);
    deadline -= ts.tv_sec * CLOCK_FREQ;
    deadline -= ts.tv_nsec / (1000000000 / CLOCK_FREQ);

    return vlc_cond_wait_delay(cond, mutex, deadline);
}

#define READER_MASK LONG_MAX
#define WRITER_BIT  LONG_MIN

void vlc_rwlock_init (vlc_rwlock_t *lock)
{
    vlc_mutex_init (&lock->mutex);
    vlc_cond_init (&lock->wait);
    lock->state = 0;
}

void vlc_rwlock_destroy (vlc_rwlock_t *lock)
{
    vlc_cond_destroy (&lock->wait);
    vlc_mutex_destroy (&lock->mutex);
}

void vlc_rwlock_rdlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock (&lock->mutex);
    /* Recursive read-locking is allowed.
    * Ensure that there is no active writer. */
    while (lock->state < 0)
    {
        assert (lock->state == WRITER_BIT);
        mutex_cleanup_push (&lock->mutex);
        vlc_cond_wait (&lock->wait, &lock->mutex);
        vlc_cleanup_pop ();
    }
    if (unlikely(lock->state >= READER_MASK))
        abort (); /* An overflow is certainly a recursion bug. */
    lock->state++;
    vlc_mutex_unlock (&lock->mutex);
}

void vlc_rwlock_wrlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock (&lock->mutex);
    /* Wait until nobody owns the lock in any way. */
    while (lock->state != 0)
    {
        mutex_cleanup_push (&lock->mutex);
        vlc_cond_wait (&lock->wait, &lock->mutex);
        vlc_cleanup_pop ();
    }
    lock->state = WRITER_BIT;
    vlc_mutex_unlock (&lock->mutex);
}

void vlc_rwlock_unlock (vlc_rwlock_t *lock)
{
    vlc_mutex_lock (&lock->mutex);
    if (lock->state < 0)
    {
        assert (lock->state == WRITER_BIT);
        /* Let reader and writer compete. OS scheduler decides who wins. */
        lock->state = 0;
        vlc_cond_broadcast (&lock->wait);
    }
    else
    {
        assert (lock->state > 0);
        /* If there are no readers left, wake up one pending writer. */
        if (--lock->state == 0)
            vlc_cond_signal (&lock->wait);
    }
    vlc_mutex_unlock (&lock->mutex);
}

void vlc_sem_init (vlc_sem_t *sem, unsigned value)
{
    vlc_mutex_init (&sem->lock);
    vlc_cond_init (&sem->wait);
    sem->value = value;
}

void vlc_sem_destroy (vlc_sem_t *sem)
{
    vlc_cond_destroy (&sem->wait);
    vlc_mutex_destroy (&sem->lock);
}

int vlc_sem_post (vlc_sem_t *sem)
{
    int ret = 0;

    vlc_mutex_lock (&sem->lock);
    if (likely(sem->value != UINT_MAX))
        sem->value++;
    else
        ret = EOVERFLOW;
    vlc_mutex_unlock (&sem->lock);
    vlc_cond_signal (&sem->wait);

    return ret;
}

void vlc_sem_wait (vlc_sem_t *sem)
{
    vlc_mutex_lock (&sem->lock);
    mutex_cleanup_push (&sem->lock);
    while (!sem->value)
        vlc_cond_wait (&sem->wait, &sem->lock);
    sem->value--;
    vlc_cleanup_pop ();
    vlc_mutex_unlock (&sem->lock);
}

int vlc_threadvar_create (vlc_threadvar_t *key, void (*destr) (void *))
{
    return pthread_key_create (key, destr);
}

void vlc_threadvar_delete (vlc_threadvar_t *p_tls)
{
    pthread_key_delete (*p_tls);
}

int vlc_threadvar_set (vlc_threadvar_t key, void *value)
{
    return pthread_setspecific (key, value);
}

void *vlc_threadvar_get (vlc_threadvar_t key)
{
    return pthread_getspecific (key);
}

vlc_thread_t vlc_thread_self (void)
{
    return thread;
}

unsigned long vlc_thread_id (void)
{
    return -1;
}

void vlc_threads_setup (vlc_object_t *p_libvlc)
{
    (void)p_libvlc;
}

static void clean_detached_thread(void *data)
{
    struct vlc_thread *th = data;

    /* release thread handle */
    vlc_mutex_destroy(&th->wait.lock);
    free(th);
}

static void *detached_thread(void *data)
{
    vlc_thread_t th = data;

    thread = th;

    vlc_cleanup_push(clean_detached_thread, th);
    th->entry(th->data);
    vlc_cleanup_pop();
    clean_detached_thread(th);
    return NULL;
}

static void finish_joinable_thread(void *data)
{
    vlc_thread_t th = data;

    vlc_sem_post(&th->finished);
}

static void *joinable_thread(void *data)
{
    vlc_thread_t th = data;
    void *ret;

    vlc_cleanup_push(finish_joinable_thread, th);
    thread = th;
    ret = th->entry(th->data);
    vlc_cleanup_pop();
    vlc_sem_post(&th->finished);

    return ret;
}

static int vlc_clone_attr(vlc_thread_t *th, void *(*entry)(void *), void *data, char* th_name, bool detach)
{
    vlc_thread_t th2 = malloc(sizeof(struct vlc_thread));
    if (unlikely(th2 == NULL))
        return ENOMEM;

    int ret;

    sigset_t oldset;
    {
        sigset_t set;
        sigemptyset (&set);
        sigdelset(&set, SIGHUP);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGTERM);

        sigaddset(&set, SIGPIPE); /* We don't want this one, really! */
        pthread_sigmask(SIG_BLOCK, &set, &oldset);
    }

    if (!detach)
        vlc_sem_init(&th2->finished, 0);
    atomic_store(&th2->killed, false);
    th2->killable = true;
    th2->entry = entry;
    th2->data = data;
    th2->wait.addr = NULL;
    vlc_mutex_init(&th2->wait.lock);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&th2->thread, &attr, detach ? detached_thread : joinable_thread, th2);
    pthread_attr_destroy(&attr);

    if (th_name)
        pthread_setname_np(th2->thread, th_name);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    *th = th2;
    return ret;
}

int vlc_clone(vlc_thread_t *th, void *(*entry) (void *), void *data, char* th_name, int priority)
{
    (void) priority;
    return vlc_clone_attr(th, entry, data, th_name, false);
}

void vlc_join(vlc_thread_t handle, void **result)
{
    vlc_sem_wait(&handle->finished);
    vlc_sem_destroy(&handle->finished);

    int val = pthread_join (handle->thread, result);
    VLC_THREAD_ASSERT ("joining thread");
    clean_detached_thread(handle);
}

int vlc_clone_detach(vlc_thread_t *th, void *(*entry)(void *), void *data, int priority)
{
    vlc_thread_t dummy;
    if (th == NULL)
        th = &dummy;

    (void) priority;
    return vlc_clone_attr(th, entry, data, NULL, true);
}

int vlc_set_priority(vlc_thread_t th, int priority)
{
    (void) th; (void) priority;
    return VLC_SUCCESS;
}

void vlc_cancel(vlc_thread_t thread_id)
{
    atomic_int *addr;

    atomic_store(&thread_id->killed, true);

    vlc_mutex_lock(&thread_id->wait.lock);
    addr = thread_id->wait.addr;
    if (addr != NULL)
    {
        atomic_fetch_or_explicit(addr, 1, memory_order_relaxed);
        vlc_addr_broadcast(addr);
    }
    vlc_mutex_unlock(&thread_id->wait.lock);
}

int vlc_savecancel(void)
{
    if (!thread) /* not created by VLC, can't be canceled */
        return true;

    int oldstate = thread->killable;
    thread->killable = false;
    return oldstate;
}

void vlc_restorecancel(int state)
{
    if (!thread) /* not created by VLC, can't be canceled */
        return;

    thread->killable = state;
}

void vlc_testcancel(void)
{
    if (!thread) /* not created by VLC, can't be canceled */
        return;
    if (!thread->killable)
        return;
    if (!atomic_load(&thread->killed))
        return;

    pthread_exit(NULL);
}

void vlc_control_cancel(int cmd, ...)
{
    vlc_thread_t th = vlc_thread_self();
    va_list ap;

    if (th == NULL)
        return;

    va_start(ap, cmd);
    switch (cmd)
    {
    case VLC_CANCEL_ADDR_SET:
    {
        void *addr = va_arg(ap, void *);

        //msg_Dbg(NULL, "VLC_CANCEL_ADDR_SET, %p, %p", th->wait.addr, addr);
        vlc_mutex_lock(&th->wait.lock);
        assert(th->wait.addr == NULL);
        th->wait.addr = addr;
        vlc_mutex_unlock(&th->wait.lock);
        break;
    }

    case VLC_CANCEL_ADDR_CLEAR:
    {
        void *addr = va_arg(ap, void *);

        //msg_Dbg(NULL, "VLC_CANCEL_ADDR_CLEAR, %p, %p", th->wait.addr, addr);
        vlc_mutex_lock(&th->wait.lock);
        assert(th->wait.addr == addr);
        th->wait.addr = NULL;
        (void) addr;
        vlc_mutex_unlock(&th->wait.lock);
        break;
    }

    default:
        vlc_assert_unreachable ();
    }
    va_end(ap);
}

mtime_t mdate (void)
{
    struct timespec ts;

    if (unlikely(clock_gettime (CLOCK_MONOTONIC, &ts) != 0))
        abort ();

    return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);
}

unsigned vlc_GetCPUCount(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

#undef mwait
void mwait(mtime_t deadline)
{
    mtime_t delay;
    atomic_int value = ATOMIC_VAR_INIT(0);

    vlc_cancel_addr_prepare(&value);

    while ((delay = (deadline - mdate())) > 0)
    {
        vlc_addr_timedwait(&value, 0, delay);
        vlc_testcancel();
    }

    vlc_cancel_addr_finish(&value);
}

#undef msleep
void msleep(mtime_t delay)
{
    mwait(mdate() + delay);
}

static int sys_futex(void *addr, int op, unsigned val,
    const struct timespec *to, void *addr2, int val3)
{
    return syscall(__NR_futex, addr, op, val, to, addr2, val3);
}

static int vlc_futex_wake(void *addr, int nr)
{
    return sys_futex(addr, FUTEX_WAKE_PRIVATE, nr, NULL, NULL, 0);
}

static int vlc_futex_wait(void *addr, unsigned val, const struct timespec *to)
{
    return sys_futex(addr, FUTEX_WAIT_PRIVATE, val, to, NULL, 0);
}

void vlc_addr_signal(void *addr)
{
    vlc_futex_wake(addr, 1);
}

void vlc_addr_broadcast(void *addr)
{
    vlc_futex_wake(addr, INT_MAX);
}

void vlc_addr_wait(void *addr, unsigned val)
{
    vlc_futex_wait(addr, val, NULL);
}

bool vlc_addr_timedwait(void *addr, unsigned val, mtime_t delay)
{
    lldiv_t d = lldiv(delay, CLOCK_FREQ);
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };

    return (vlc_futex_wait(addr, val, &ts) == 0 || errno != ETIMEDOUT);
}
#else
#include <unistd.h> /* fsync() */
#include <pthread.h>
#include <sched.h>
#include <sys/time.h> /* gettimeofday() */
#include <sys/syscall.h>
#include <linux/futex.h>

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_WAKE_PRIVATE FUTEX_WAKE
#define FUTEX_WAIT_PRIVATE FUTEX_WAIT
#endif

#if !defined (_POSIX_TIMERS)
#define _POSIX_TIMERS (-1)
#endif
#if !defined (_POSIX_CLOCK_SELECTION)
#define _POSIX_CLOCK_SELECTION (-1)
#endif
#if !defined (_POSIX_MONOTONIC_CLOCK)
#define _POSIX_MONOTONIC_CLOCK (-1)
#endif

#if (_POSIX_TIMERS > 0)
static unsigned vlc_clock_prec;

# if (_POSIX_MONOTONIC_CLOCK > 0) && (_POSIX_CLOCK_SELECTION > 0)
/* Compile-time POSIX monotonic clock support */
#  define vlc_clock_id (CLOCK_MONOTONIC)

# elif (_POSIX_MONOTONIC_CLOCK == 0) && (_POSIX_CLOCK_SELECTION > 0)
/* Run-time POSIX monotonic clock support (see clock_setup() below) */
static clockid_t vlc_clock_id;

# else
/* No POSIX monotonic clock support */
#   define vlc_clock_id (CLOCK_REALTIME)
#   warning Monotonic clock not available.Expect timing issues.

# endif /* _POSIX_MONOTONIC_CLOKC */

static void vlc_clock_setup_once(void)
{
# if (_POSIX_MONOTONIC_CLOCK == 0)
    long val = sysconf(_SC_MONOTONIC_CLOCK);
    assert(val != 0);
    vlc_clock_id = (val < 0) ? CLOCK_REALTIME : CLOCK_MONOTONIC;
# endif

    struct timespec res;
    if (unlikely(clock_getres(vlc_clock_id, &res) != 0 || res.tv_sec != 0))
        abort();
    vlc_clock_prec = (res.tv_nsec + 500) / 1000;
}

static pthread_once_t vlc_clock_once = PTHREAD_ONCE_INIT;

# define vlc_clock_setup() \
    pthread_once(&vlc_clock_once, vlc_clock_setup_once)

#else /* _POSIX_TIMERS */
# define vlc_clock_setup() (void)0
#endif /* _POSIX_TIMERS */
static struct timespec mtime_to_ts(mtime_t date)
{
    lldiv_t d = lldiv(date, CLOCK_FREQ);
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };
    return ts;
}


/**
* Print a back-trace to the standard error for debugging purpose.
*/
void vlc_trace(const char *fn, const char *file, unsigned line)
{
    fprintf(stderr, "at %s:%u in %s\n", file, line, fn);
    fflush(stderr); /* needed before switch to low-level I/O */
#ifdef HAVE_BACKTRACE
    void *stack[20];
    int len = backtrace(stack, sizeof(stack) / sizeof(stack[0]));
    backtrace_symbols_fd(stack, len, 2);
#endif
    fsync(2);
}

#ifndef NDEBUG
/**
* Reports a fatal error from the threading layer, for debugging purposes.
*/
static void vlc_thread_fatal(const char *action, int error, const char *function, const char *file, unsigned line)
{
    int canc = vlc_savecancel();;
    fprintf(stderr, "LibVLC fatal error %s (%d) in thread %lu ",
        action, error, vlc_thread_id());
    vlc_trace(function, file, line);
    perror("Thread error");
    fflush(stderr);

    vlc_restorecancel(canc);
    abort();
}

# define VLC_THREAD_ASSERT( action ) \
    if (unlikely(val)) \
        vlc_thread_fatal (action, val, __FUNCTION__, __FILE__, __LINE__)
#else
# define VLC_THREAD_ASSERT( action ) ((void)val)
#endif

#ifndef NDEBUG
# ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
# else
#  define RUNNING_ON_VALGRIND (0)
# endif

/**
* Asserts that a mutex is locked by the calling thread.
*/
#undef vlc_assert_locked
void vlc_assert_locked(vlc_mutex_t *p_mutex)
{
    if (RUNNING_ON_VALGRIND > 0)
        return;
    assert(pthread_mutex_lock(p_mutex) == EDEADLK);
}
#endif

void vlc_mutex_init(vlc_mutex_t *p_mutex)
{
    pthread_mutexattr_t attr;

    if (unlikely(pthread_mutexattr_init (&attr)))
        abort();
#ifdef NDEBUG
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_DEFAULT);
#else
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    if (unlikely(pthread_mutex_init (p_mutex, &attr)))
        abort();
    pthread_mutexattr_destroy( &attr );
}

void vlc_mutex_init_recursive( vlc_mutex_t *p_mutex )
{
    pthread_mutexattr_t attr;

    if (unlikely(pthread_mutexattr_init (&attr)))
        abort();
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (unlikely(pthread_mutex_init (p_mutex, &attr)))
        abort();
    pthread_mutexattr_destroy(&attr);
}

void vlc_mutex_destroy (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_destroy( p_mutex );
    VLC_THREAD_ASSERT ("destroying mutex");
}

void vlc_mutex_lock(vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_lock( p_mutex );
    VLC_THREAD_ASSERT ("locking mutex");
}

int vlc_mutex_trylock(vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_trylock( p_mutex );

    if (val != EBUSY)
        VLC_THREAD_ASSERT ("locking mutex");
    return val;
}

void vlc_mutex_unlock (vlc_mutex_t *p_mutex)
{
    int val = pthread_mutex_unlock( p_mutex );
    VLC_THREAD_ASSERT ("unlocking mutex");
}

void vlc_cond_init (vlc_cond_t *p_condvar)
{
    pthread_condattr_t attr;

    if (unlikely(pthread_condattr_init(&attr)))
        abort();
#if (_POSIX_CLOCK_SELECTION > 0)
    vlc_clock_setup();
    pthread_condattr_setclock(&attr, vlc_clock_id);
#endif
    if (unlikely(pthread_cond_init(p_condvar, &attr)))
        abort();
    pthread_condattr_destroy(&attr);
}

void vlc_cond_init_daytime(vlc_cond_t *p_condvar)
{
    if (unlikely(pthread_cond_init(p_condvar, NULL)))
        abort();
}

void vlc_cond_destroy(vlc_cond_t *p_condvar)
{
    int val = pthread_cond_destroy(p_condvar);
    VLC_THREAD_ASSERT("destroying condition");
}

void vlc_cond_signal(vlc_cond_t *p_condvar)
{
    int val = pthread_cond_signal(p_condvar);
    VLC_THREAD_ASSERT("signaling condition variable");
}

void vlc_cond_broadcast(vlc_cond_t *p_condvar)
{
    pthread_cond_broadcast(p_condvar);
}

void vlc_cond_wait(vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex)
{
    int val = pthread_cond_wait(p_condvar, p_mutex);
    VLC_THREAD_ASSERT("waiting on condition");
}

int vlc_cond_timedwait(vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex, mtime_t deadline)
{
    struct timespec ts = mtime_to_ts(deadline);
    int val = pthread_cond_timedwait(p_condvar, p_mutex, &ts);
    if (val != ETIMEDOUT)
        VLC_THREAD_ASSERT("timed-waiting on condition");
    return val;
}

int vlc_cond_timedwait_daytime(vlc_cond_t *p_condvar, vlc_mutex_t *p_mutex, time_t deadline)
{
    struct timespec ts = { deadline, 0 };
    int val = pthread_cond_timedwait(p_condvar, p_mutex, &ts);
    if (val != ETIMEDOUT)
        VLC_THREAD_ASSERT("timed-waiting on condition");
    return val;
}

void vlc_sem_init(vlc_sem_t *sem, unsigned value)
{
    if (unlikely(sem_init(sem, 0, value)))
        abort();
}

void vlc_sem_destroy(vlc_sem_t *sem)
{
    int val;
    if (likely(sem_destroy(sem) == 0))
        return;

    val = errno;
    VLC_THREAD_ASSERT("destroying semaphore");
}

int vlc_sem_post(vlc_sem_t *sem)
{
    int val;
    if (likely(sem_post(sem) == 0))
        return 0;

    val = errno;
    if (unlikely(val != EOVERFLOW))
        VLC_THREAD_ASSERT("unlocking semaphore");
    return val;
}

void vlc_sem_wait(vlc_sem_t *sem)
{
    int val;
    do
        if (likely(sem_wait(sem) == 0))
            return;
    while ((val = errno) == EINTR);
    VLC_THREAD_ASSERT("locking semaphore");
}

void vlc_rwlock_init (vlc_rwlock_t *lock)
{
    if (unlikely(pthread_rwlock_init(lock, NULL)))
        abort();
}

void vlc_rwlock_destroy(vlc_rwlock_t *lock)
{
    int val = pthread_rwlock_destroy(lock);
    VLC_THREAD_ASSERT("destroying R/W lock");
}

void vlc_rwlock_rdlock(vlc_rwlock_t *lock)
{
    int val = pthread_rwlock_rdlock(lock);
    VLC_THREAD_ASSERT("acquiring R/W lock for reading");
}

void vlc_rwlock_wrlock(vlc_rwlock_t *lock)
{
    int val = pthread_rwlock_wrlock(lock);
    VLC_THREAD_ASSERT("acquiring R/W lock for writing");
}

void vlc_rwlock_unlock(vlc_rwlock_t *lock)
{
    int val = pthread_rwlock_unlock(lock);
    VLC_THREAD_ASSERT("releasing R/W lock");
}

int vlc_threadvar_create(vlc_threadvar_t *p_tls, void(*destr) (void *))
{
    return pthread_key_create(p_tls, destr);
}

void vlc_threadvar_delete(vlc_threadvar_t *p_tls)
{
    pthread_key_delete(*p_tls);
}

int vlc_threadvar_set(vlc_threadvar_t key, void *value)
{
    return pthread_setspecific(key, value);
}

void *vlc_threadvar_get(vlc_threadvar_t key)
{
    return pthread_getspecific(key);
}

static bool rt_priorities = false;
static int rt_offset;

void vlc_threads_setup(vlc_object_t *vlc)
{
    static vlc_mutex_t lock = VLC_STATIC_MUTEX;
    static bool initialized = false;

    vlc_mutex_lock(&lock);
    /* Initializes real-time priorities before any thread is created,
    * just once per process. */
    if (!initialized)
    {
        if (var_InheritBool(vlc, "rt-priority"))
        {
            rt_offset = var_InheritInteger(vlc, "rt-offset");
            rt_priorities = true;
        }
        initialized = true;
    }
    vlc_mutex_unlock(&lock);
}


static int vlc_clone_attr(vlc_thread_t *th, pthread_attr_t *attr, void *(*entry) (void *), void *data, char* th_name, int priority)
{
    int ret;

    /* Block the signals that signals interface plugin handles.
    * If the LibVLC caller wants to handle some signals by itself, it should
    * block these before whenever invoking LibVLC. And it must obviously not
    * start the VLC signals interface plugin.
    *
    * LibVLC will normally ignore any interruption caused by an asynchronous
    * signal during a system call. But there may well be some buggy cases
    * where it fails to handle EINTR (bug reports welcome). Some underlying
    * libraries might also not handle EINTR properly.
    */
    sigset_t oldset;
    {
        sigset_t set;
        sigemptyset(&set);
        sigdelset(&set, SIGHUP);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGTERM);

        sigaddset(&set, SIGPIPE); /* We don't want this one, really! */
        pthread_sigmask(SIG_BLOCK, &set, &oldset);
    }

#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
 && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
 && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)
    if (rt_priorities)
    {
        struct sched_param sp = { .sched_priority = priority + rt_offset, };
        int policy;

        if (sp.sched_priority <= 0)
            sp.sched_priority += sched_get_priority_max(policy = SCHED_OTHER);
        else
            sp.sched_priority += sched_get_priority_min(policy = SCHED_RR);

        pthread_attr_setschedpolicy(attr, policy);
        pthread_attr_setschedparam(attr, &sp);
        pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    }
#else
    (void)priority;
#endif

    /* The thread stack size.
    * The lower the value, the less address space per thread, the highest
    * maximum simultaneous threads per process. Too low values will cause
    * stack overflows and weird crashes. Set with caution. Also keep in mind
    * that 64-bits platforms consume more stack than 32-bits one.
    *
    * Thanks to on-demand paging, thread stack size only affects address space
    * consumption. In terms of memory, threads only use what they need
    * (rounded up to the page boundary).
    *
    * For example, on Linux i386, the default is 2 mega-bytes, which supports
    * about 320 threads per processes. */
#define VLC_STACKSIZE (128 * sizeof (void *) * 1024)

#ifdef VLC_STACKSIZE
    ret = pthread_attr_setstacksize(attr, VLC_STACKSIZE);
    assert(ret == 0); /* fails iif VLC_STACKSIZE is invalid */
#endif

    ret = pthread_create(&th->handle, attr, entry, data);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    pthread_attr_destroy(attr);
    return ret;
}

int vlc_clone(vlc_thread_t *th, void *(*entry) (void *), void *data, char* th_name, int priority)
{
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    return vlc_clone_attr(th, &attr, entry, data, th_name, priority);
}

void vlc_join(vlc_thread_t th, void **result)
{
    int val = pthread_join(th.handle, result);
    VLC_THREAD_ASSERT("joining thread");
}


int vlc_clone_detach(vlc_thread_t *th, void *(*entry) (void *), void *data, int priority)
{
    vlc_thread_t dummy;
    pthread_attr_t attr;

    if (th == NULL)
        th = &dummy;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    return vlc_clone_attr(th, &attr, entry, data, NULL, priority);
}

vlc_thread_t vlc_thread_self(void)
{
    vlc_thread_t thread = { pthread_self() };
    return thread;
}

unsigned long vlc_thread_id(void)
{
    static __thread pid_t tid = 0;
    if (unlikely(tid == 0))
        tid = syscall(__NR_gettid);
    return tid;
}

int vlc_set_priority(vlc_thread_t th, int priority)
{
#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
 && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
 && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)
    if (rt_priorities)
    {
        struct sched_param sp = { .sched_priority = priority + rt_offset, };
        int policy;

        if (sp.sched_priority <= 0)
            sp.sched_priority += sched_get_priority_max(policy = SCHED_OTHER);
        else
            sp.sched_priority += sched_get_priority_min(policy = SCHED_RR);

        if (pthread_setschedparam(th.handle, policy, &sp))
            return VLC_EGENERIC;
    }
#else
    (void)th; (void)priority;
#endif
    return VLC_SUCCESS;
}

void vlc_cancel(vlc_thread_t th)
{
    pthread_cancel(th.handle);
}

int vlc_savecancel(void)
{
    int state;
    int val = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);

    VLC_THREAD_ASSERT("saving cancellation");
    return state;
}

void vlc_restorecancel(int state)
{
#ifndef NDEBUG
    int oldstate, val;

    val = pthread_setcancelstate(state, &oldstate);
    /* This should fail if an invalid value for given for state */
    VLC_THREAD_ASSERT("restoring cancellation");

    if (unlikely(oldstate != PTHREAD_CANCEL_DISABLE))
        vlc_thread_fatal("restoring cancellation while not disabled", EINVAL,
        __func__, __FILE__, __LINE__);
#else
    pthread_setcancelstate(state, NULL);
#endif
}

void vlc_testcancel(void)
{
    pthread_testcancel();
}

void vlc_control_cancel(int cmd, ...)
{
    (void)cmd;
    vlc_assert_unreachable();
}

mtime_t mdate(void)
{
#if (_POSIX_TIMERS > 0)
    struct timespec ts;

    vlc_clock_setup();
    if (unlikely(clock_gettime(vlc_clock_id, &ts) != 0))
        abort();

    return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);

#else
    struct timeval tv;

    if (unlikely(gettimeofday(&tv, NULL) != 0))
        abort();
    return (INT64_C(1000000) * tv.tv_sec) + tv.tv_usec;

#endif
}

#undef mwait
void mwait(mtime_t deadline)
{
#if (_POSIX_CLOCK_SELECTION > 0)
    vlc_clock_setup();
    /* If the deadline is already elapsed, or within the clock precision,
    * do not even bother the system timer. */
    deadline -= vlc_clock_prec;

    struct timespec ts = mtime_to_ts(deadline);

    while (clock_nanosleep(vlc_clock_id, TIMER_ABSTIME, &ts, NULL) == EINTR);

#else
    deadline -= mdate();
    if (deadline > 0)
        msleep(deadline);

#endif
}

#undef msleep
void msleep(mtime_t delay)
{
    struct timespec ts = mtime_to_ts(delay);
#if (_POSIX_CLOCK_SELECTION > 0)
    vlc_clock_setup();
    while (clock_nanosleep(vlc_clock_id, 0, &ts, &ts) == EINTR);
#else
    while (nanosleep(&ts, &ts) == -1)
        assert(errno == EINTR);
#endif
}

unsigned vlc_GetCPUCount(void)
{
#if defined(HAVE_SCHED_GETAFFINITY)
    cpu_set_t cpu;

    CPU_ZERO(&cpu);
    if (sched_getaffinity(0, sizeof(cpu), &cpu) < 0)
        return 1;

    return CPU_COUNT(&cpu);
#elif defined(__SunOS)
    unsigned count = 0;
    int type;
    u_int numcpus;
    processor_info_t cpuinfo;

    processorid_t *cpulist = vlc_alloc(sysconf(_SC_NPROCESSORS_MAX), sizeof(*cpulist));
    if (unlikely(cpulist == NULL))
        return 1;

    if (pset_info(PS_MYID, &type, &numcpus, cpulist) == 0)
    {
        for (u_int i = 0; i < numcpus; i++)
            if (processor_info(cpulist[i], &cpuinfo) == 0)
                count += (cpuinfo.pi_state == P_ONLINE);
    }
    else
        count = sysconf(_SC_NPROCESSORS_ONLN);
    free(cpulist);
    return count ? count : 1;
#elif defined(_SC_NPROCESSORS_CONF)
    return sysconf(_SC_NPROCESSORS_CONF);
#else
    return 1;
#endif
}

static int sys_futex(void *addr, int op, unsigned val,
    const struct timespec *to, void *addr2, int val3)
{
    return syscall(__NR_futex, addr, op, val, to, addr2, val3);
}

static int vlc_futex_wake(void *addr, int nr)
{
    return sys_futex(addr, FUTEX_WAKE_PRIVATE, nr, NULL, NULL, 0);
}

static int vlc_futex_wait(void *addr, unsigned val, const struct timespec *to)
{
    return sys_futex(addr, FUTEX_WAIT_PRIVATE, val, to, NULL, 0);
}

void vlc_addr_signal(void *addr)
{
    vlc_futex_wake(addr, 1);
}

void vlc_addr_broadcast(void *addr)
{
    vlc_futex_wake(addr, INT_MAX);
}

void vlc_addr_wait(void *addr, unsigned val)
{
    vlc_futex_wait(addr, val, NULL);
}

bool vlc_addr_timedwait(void *addr, unsigned val, mtime_t delay)
{
    lldiv_t d = lldiv(delay, CLOCK_FREQ);
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };

    return (vlc_futex_wait(addr, val, &ts) == 0 || errno != ETIMEDOUT);
}
#endif

void vlc_global_mutex(unsigned n, bool acquire)
{
    static vlc_mutex_t locks[] = {
        VLC_STATIC_MUTEX,
        VLC_STATIC_MUTEX,
        VLC_STATIC_MUTEX,
        VLC_STATIC_MUTEX,
        VLC_STATIC_MUTEX,
#ifdef _WIN32
        VLC_STATIC_MUTEX, // For MTA holder
#endif
    };
    assert(n < (sizeof(locks) / sizeof(locks[0])));

    vlc_mutex_t *lock = locks + n;
    if (acquire)
        vlc_mutex_lock(lock);
    else
        vlc_mutex_unlock(lock);
}