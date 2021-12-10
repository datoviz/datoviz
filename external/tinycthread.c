/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
Copyright (c) 2012 Marcus Geelnard
Copyright (c) 2013-2016 Evan Nemerson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include "tinycthread.h"
#include <stdlib.h>

/* Platform specific includes */
#if defined(_TTHREAD_POSIX_)
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(_TTHREAD_WIN32_)
#include <process.h>
#include <sys/timeb.h>
#endif

/* Standard, good-to-have defines */
#ifndef NULL
#define NULL (void*)0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif


int tct_mtx_init(tct_mtx_t* tct_mtx, int type)
{
#if defined(_TTHREAD_WIN32_)
    tct_mtx->mAlreadyLocked = FALSE;
    tct_mtx->mRecursive = type & tct_mtx_recursive;
    tct_mtx->mTimed = type & tct_mtx_timed;
    if (!tct_mtx->mTimed)
    {
        InitializeCriticalSection(&(tct_mtx->mHandle.cs));
    }
    else
    {
        tct_mtx->mHandle.mut = CreateMutex(NULL, FALSE, NULL);
        if (tct_mtx->mHandle.mut == NULL)
        {
            return tct_thrd_error;
        }
    }
    return tct_thrd_success;
#else
    int ret;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    if (type & tct_mtx_recursive)
    {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    }
    ret = pthread_mutex_init(tct_mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return ret == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

void tct_mtx_destroy(tct_mtx_t* tct_mtx)
{
#if defined(_TTHREAD_WIN32_)
    if (!tct_mtx->mTimed)
    {
        DeleteCriticalSection(&(tct_mtx->mHandle.cs));
    }
    else
    {
        CloseHandle(tct_mtx->mHandle.mut);
    }
#else
    pthread_mutex_destroy(tct_mtx);
#endif
}

int tct_mtx_lock(tct_mtx_t* tct_mtx)
{
#if defined(_TTHREAD_WIN32_)
    if (!tct_mtx->mTimed)
    {
        EnterCriticalSection(&(tct_mtx->mHandle.cs));
    }
    else
    {
        switch (WaitForSingleObject(tct_mtx->mHandle.mut, INFINITE))
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_ABANDONED:
        default:
            return tct_thrd_error;
        }
    }

    if (!tct_mtx->mRecursive)
    {
        while (tct_mtx->mAlreadyLocked)
            Sleep(1); /* Simulate deadlock... */
        tct_mtx->mAlreadyLocked = TRUE;
    }
    return tct_thrd_success;
#else
    return pthread_mutex_lock(tct_mtx) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

int tct_mtx_timedlock(tct_mtx_t* tct_mtx, const struct timespec* ts)
{
#if defined(_TTHREAD_WIN32_)
    struct timespec current_ts;
    DWORD timeoutMs;

    if (!tct_mtx->mTimed)
    {
        return tct_thrd_error;
    }

    timespec_get(&current_ts, TIME_UTC);

    if ((current_ts.tv_sec > ts->tv_sec) ||
        ((current_ts.tv_sec == ts->tv_sec) && (current_ts.tv_nsec >= ts->tv_nsec)))
    {
        timeoutMs = 0;
    }
    else
    {
        timeoutMs = (DWORD)(ts->tv_sec - current_ts.tv_sec) * 1000;
        timeoutMs += (ts->tv_nsec - current_ts.tv_nsec) / 1000000;
        timeoutMs += 1;
    }

    /* TODO: the timeout for WaitForSingleObject doesn't include time
       while the computer is asleep. */
    switch (WaitForSingleObject(tct_mtx->mHandle.mut, timeoutMs))
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_TIMEOUT:
        return tct_thrd_timedout;
    case WAIT_ABANDONED:
    default:
        return tct_thrd_error;
    }

    if (!tct_mtx->mRecursive)
    {
        while (tct_mtx->mAlreadyLocked)
            Sleep(1); /* Simulate deadlock... */
        tct_mtx->mAlreadyLocked = TRUE;
    }

    return tct_thrd_success;
#elif defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS >= 200112L) && defined(_POSIX_THREADS) &&      \
    (_POSIX_THREADS >= 200112L)
    switch (pthread_mutex_timedlock(tct_mtx, ts))
    {
    case 0:
        return tct_thrd_success;
    case ETIMEDOUT:
        return tct_thrd_timedout;
    default:
        return tct_thrd_error;
    }
#else
    int rc;
    struct timespec cur, dur;

    /* Try to acquire the lock and, if we fail, sleep for 5ms. */
    while ((rc = pthread_mutex_trylock(tct_mtx)) == EBUSY)
    {
        timespec_get(&cur, TIME_UTC);

        if ((cur.tv_sec > ts->tv_sec) ||
            ((cur.tv_sec == ts->tv_sec) && (cur.tv_nsec >= ts->tv_nsec)))
        {
            break;
        }

        dur.tv_sec = ts->tv_sec - cur.tv_sec;
        dur.tv_nsec = ts->tv_nsec - cur.tv_nsec;
        if (dur.tv_nsec < 0)
        {
            dur.tv_sec--;
            dur.tv_nsec += 1000000000;
        }

        if ((dur.tv_sec != 0) || (dur.tv_nsec > 5000000))
        {
            dur.tv_sec = 0;
            dur.tv_nsec = 5000000;
        }

        nanosleep(&dur, NULL);
    }

    switch (rc)
    {
    case 0:
        return tct_thrd_success;
    case ETIMEDOUT:
    case EBUSY:
        return tct_thrd_timedout;
    default:
        return tct_thrd_error;
    }
#endif
}

int tct_mtx_trylock(tct_mtx_t* tct_mtx)
{
#if defined(_TTHREAD_WIN32_)
    int ret;

    if (!tct_mtx->mTimed)
    {
        ret = TryEnterCriticalSection(&(tct_mtx->mHandle.cs)) ? tct_thrd_success : tct_thrd_busy;
    }
    else
    {
        ret = (WaitForSingleObject(tct_mtx->mHandle.mut, 0) == WAIT_OBJECT_0) ? tct_thrd_success
                                                                              : tct_thrd_busy;
    }

    if ((!tct_mtx->mRecursive) && (ret == tct_thrd_success))
    {
        if (tct_mtx->mAlreadyLocked)
        {
            LeaveCriticalSection(&(tct_mtx->mHandle.cs));
            ret = tct_thrd_busy;
        }
        else
        {
            tct_mtx->mAlreadyLocked = TRUE;
        }
    }
    return ret;
#else
    return (pthread_mutex_trylock(tct_mtx) == 0) ? tct_thrd_success : tct_thrd_busy;
#endif
}

int tct_mtx_unlock(tct_mtx_t* tct_mtx)
{
#if defined(_TTHREAD_WIN32_)
    tct_mtx->mAlreadyLocked = FALSE;
    if (!tct_mtx->mTimed)
    {
        LeaveCriticalSection(&(tct_mtx->mHandle.cs));
    }
    else
    {
        if (!ReleaseMutex(tct_mtx->mHandle.mut))
        {
            return tct_thrd_error;
        }
    }
    return tct_thrd_success;
#else
    return pthread_mutex_unlock(tct_mtx) == 0 ? tct_thrd_success : tct_thrd_error;
    ;
#endif
}

#if defined(_TTHREAD_WIN32_)
#define _CONDITION_EVENT_ONE 0
#define _CONDITION_EVENT_ALL 1
#endif

int tct_cnd_init(tct_cnd_t* cond)
{
#if defined(_TTHREAD_WIN32_)
    cond->mWaitersCount = 0;

    /* Init critical section */
    InitializeCriticalSection(&cond->mWaitersCountLock);

    /* Init events */
    cond->mEvents[_CONDITION_EVENT_ONE] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (cond->mEvents[_CONDITION_EVENT_ONE] == NULL)
    {
        cond->mEvents[_CONDITION_EVENT_ALL] = NULL;
        return tct_thrd_error;
    }
    cond->mEvents[_CONDITION_EVENT_ALL] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (cond->mEvents[_CONDITION_EVENT_ALL] == NULL)
    {
        CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
        cond->mEvents[_CONDITION_EVENT_ONE] = NULL;
        return tct_thrd_error;
    }

    return tct_thrd_success;
#else
    return pthread_cond_init(cond, NULL) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

void tct_cnd_destroy(tct_cnd_t* cond)
{
#if defined(_TTHREAD_WIN32_)
    if (cond->mEvents[_CONDITION_EVENT_ONE] != NULL)
    {
        CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
    }
    if (cond->mEvents[_CONDITION_EVENT_ALL] != NULL)
    {
        CloseHandle(cond->mEvents[_CONDITION_EVENT_ALL]);
    }
    DeleteCriticalSection(&cond->mWaitersCountLock);
#else
    pthread_cond_destroy(cond);
#endif
}

int tct_cnd_signal(tct_cnd_t* cond)
{
#if defined(_TTHREAD_WIN32_)
    int haveWaiters;

    /* Are there any waiters? */
    EnterCriticalSection(&cond->mWaitersCountLock);
    haveWaiters = (cond->mWaitersCount > 0);
    LeaveCriticalSection(&cond->mWaitersCountLock);

    /* If we have any waiting threads, send them a signal */
    if (haveWaiters)
    {
        if (SetEvent(cond->mEvents[_CONDITION_EVENT_ONE]) == 0)
        {
            return tct_thrd_error;
        }
    }

    return tct_thrd_success;
#else
    return pthread_cond_signal(cond) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

int tct_cnd_broadcast(tct_cnd_t* cond)
{
#if defined(_TTHREAD_WIN32_)
    int haveWaiters;

    /* Are there any waiters? */
    EnterCriticalSection(&cond->mWaitersCountLock);
    haveWaiters = (cond->mWaitersCount > 0);
    LeaveCriticalSection(&cond->mWaitersCountLock);

    /* If we have any waiting threads, send them a signal */
    if (haveWaiters)
    {
        if (SetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0)
        {
            return tct_thrd_error;
        }
    }

    return tct_thrd_success;
#else
    return pthread_cond_broadcast(cond) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

#if defined(_TTHREAD_WIN32_)
static int _tct_cnd_timedwait_win32(tct_cnd_t* cond, tct_mtx_t* tct_mtx, DWORD timeout)
{
    DWORD result;
    int lastWaiter;

    /* Increment number of waiters */
    EnterCriticalSection(&cond->mWaitersCountLock);
    ++cond->mWaitersCount;
    LeaveCriticalSection(&cond->mWaitersCountLock);

    /* Release the mutex while waiting for the condition (will decrease
       the number of waiters when done)... */
    tct_mtx_unlock(tct_mtx);

    /* Wait for either event to become signaled due to tct_cnd_signal() or
       tct_cnd_broadcast() being called */
    result = WaitForMultipleObjects(2, cond->mEvents, FALSE, timeout);
    if (result == WAIT_TIMEOUT)
    {
        /* The mutex is locked again before the function returns, even if an error occurred */
        tct_mtx_lock(tct_mtx);
        return tct_thrd_timedout;
    }
    else if (result == WAIT_FAILED)
    {
        /* The mutex is locked again before the function returns, even if an error occurred */
        tct_mtx_lock(tct_mtx);
        return tct_thrd_error;
    }

    /* Check if we are the last waiter */
    EnterCriticalSection(&cond->mWaitersCountLock);
    --cond->mWaitersCount;
    lastWaiter = (result == (WAIT_OBJECT_0 + _CONDITION_EVENT_ALL)) && (cond->mWaitersCount == 0);
    LeaveCriticalSection(&cond->mWaitersCountLock);

    /* If we are the last waiter to be notified to stop waiting, reset the event */
    if (lastWaiter)
    {
        if (ResetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0)
        {
            /* The mutex is locked again before the function returns, even if an error occurred */
            tct_mtx_lock(tct_mtx);
            return tct_thrd_error;
        }
    }

    /* Re-acquire the mutex */
    tct_mtx_lock(tct_mtx);

    return tct_thrd_success;
}
#endif

int tct_cnd_wait(tct_cnd_t* cond, tct_mtx_t* tct_mtx)
{
#if defined(_TTHREAD_WIN32_)
    return _tct_cnd_timedwait_win32(cond, tct_mtx, INFINITE);
#else
    return pthread_cond_wait(cond, tct_mtx) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

int tct_cnd_timedwait(tct_cnd_t* cond, tct_mtx_t* tct_mtx, const struct timespec* ts)
{
#if defined(_TTHREAD_WIN32_)
    struct timespec now;
    if (timespec_get(&now, TIME_UTC) == TIME_UTC)
    {
        unsigned long long nowInMilliseconds = now.tv_sec * 1000 + now.tv_nsec / 1000000;
        unsigned long long tsInMilliseconds = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
        DWORD delta = (tsInMilliseconds > nowInMilliseconds)
                          ? (DWORD)(tsInMilliseconds - nowInMilliseconds)
                          : 0;
        return _tct_cnd_timedwait_win32(cond, tct_mtx, delta);
    }
    else
        return tct_thrd_error;
#else
    int ret;
    ret = pthread_cond_timedwait(cond, tct_mtx, ts);
    if (ret == ETIMEDOUT)
    {
        return tct_thrd_timedout;
    }
    return ret == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

#if defined(_TTHREAD_WIN32_)
struct TinyCThreadTSSData
{
    void* value;
    tss_t key;
    struct TinyCThreadTSSData* next;
};

static tss_dtor_t _tinycthread_tss_dtors[1088] = {
    NULL,
};


// https://github.com/tinycthread/tinycthread/issues/52
#if defined(_MSC_VER) &&                                                                          \
    (!defined(STDC_NO_THREADS) || defined(STDC_NO_THREADS) && STDC_NO_THREADS == 1)
#define _Thread_local __declspec(thread)
#else
#if defined(GNUC) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(IBMCPP)
#define _Thread_local __thread
#endif
#endif

static _Thread_local struct TinyCThreadTSSData* _tinycthread_tss_head = NULL;
static _Thread_local struct TinyCThreadTSSData* _tinycthread_tss_tail = NULL;

static void _tinycthread_tss_cleanup(void);

static void _tinycthread_tss_cleanup(void)
{
    struct TinyCThreadTSSData* data;
    int iteration;
    unsigned int again = 1;
    void* value;

    for (iteration = 0; iteration < TSS_DTOR_ITERATIONS && again > 0; iteration++)
    {
        again = 0;
        for (data = _tinycthread_tss_head; data != NULL; data = data->next)
        {
            if (data->value != NULL)
            {
                value = data->value;
                data->value = NULL;

                if (_tinycthread_tss_dtors[data->key] != NULL)
                {
                    again = 1;
                    _tinycthread_tss_dtors[data->key](value);
                }
            }
        }
    }

    while (_tinycthread_tss_head != NULL)
    {
        data = _tinycthread_tss_head->next;
        free(_tinycthread_tss_head);
        _tinycthread_tss_head = data;
    }
    _tinycthread_tss_head = NULL;
    _tinycthread_tss_tail = NULL;
}

static void NTAPI _tinycthread_tss_callback(PVOID h, DWORD dwReason, PVOID pv)
{
    (void)h;
    (void)pv;

    if (_tinycthread_tss_head != NULL &&
        (dwReason == DLL_THREAD_DETACH || dwReason == DLL_PROCESS_DETACH))
    {
        _tinycthread_tss_cleanup();
    }
}

#if defined(_MSC_VER)
#ifdef _M_X64
#pragma const_seg(".CRT$XLB")
#else
#pragma data_seg(".CRT$XLB")
#endif
PIMAGE_TLS_CALLBACK p_thread_callback = _tinycthread_tss_callback;
#ifdef _M_X64
#pragma data_seg()
#else
#pragma const_seg()
#endif
#else
PIMAGE_TLS_CALLBACK p_thread_callback __attribute__((section(".CRT$XLB"))) =
    _tinycthread_tss_callback;
#endif

#endif /* defined(_TTHREAD_WIN32_) */

/** Information to pass to the new thread (what to run). */
typedef struct
{
    tct_thrd_start_t mFunction; /**< Pointer to the function to be executed. */
    void* mArg;                 /**< Function argument for the thread function. */
} _thread_start_info;

/* Thread wrapper function. */
#if defined(_TTHREAD_WIN32_)
static DWORD WINAPI _tct_thrd_wrapper_function(LPVOID aArg)
#elif defined(_TTHREAD_POSIX_)
static void* _tct_thrd_wrapper_function(void* aArg)
#endif
{
    tct_thrd_start_t fun;
    void* arg;
    int res;

    /* Get thread startup information */
    _thread_start_info* ti = (_thread_start_info*)aArg;
    fun = ti->mFunction;
    arg = ti->mArg;

    /* The thread is responsible for freeing the startup information */
    free((void*)ti);

    /* Call the actual client thread function */
    res = fun(arg);

#if defined(_TTHREAD_WIN32_)
    if (_tinycthread_tss_head != NULL)
    {
        _tinycthread_tss_cleanup();
    }

    return (DWORD)res;
#else
    return (void*)(intptr_t)res;
#endif
}

int tct_thrd_create(tct_thrd_t* thr, tct_thrd_start_t func, void* arg)
{
    /* Fill out the thread startup information (passed to the thread wrapper,
       which will eventually free it) */
    _thread_start_info* ti = (_thread_start_info*)malloc(sizeof(_thread_start_info));
    if (ti == NULL)
    {
        return tct_thrd_nomem;
    }
    ti->mFunction = func;
    ti->mArg = arg;

    /* Create the thread */
#if defined(_TTHREAD_WIN32_)
    *thr = CreateThread(NULL, 0, _tct_thrd_wrapper_function, (LPVOID)ti, 0, NULL);
#elif defined(_TTHREAD_POSIX_)
    if (pthread_create(thr, NULL, _tct_thrd_wrapper_function, (void*)ti) != 0)
    {
        *thr = 0;
    }
#endif

    /* Did we fail to create the thread? */
    if (!*thr)
    {
        free(ti);
        return tct_thrd_error;
    }

    return tct_thrd_success;
}

tct_thrd_t tct_thrd_current(void)
{
#if defined(_TTHREAD_WIN32_)
    return GetCurrentThread();
#else
    return pthread_self();
#endif
}

int tct_thrd_detach(tct_thrd_t thr)
{
#if defined(_TTHREAD_WIN32_)
    /* https://stackoverflow.com/questions/12744324/how-to-detach-a-thread-on-windows-c#answer-12746081
     */
    return CloseHandle(thr) != 0 ? tct_thrd_success : tct_thrd_error;
#else
    return pthread_detach(thr) == 0 ? tct_thrd_success : tct_thrd_error;
#endif
}

int tct_thrd_equal(tct_thrd_t thr0, tct_thrd_t thr1)
{
#if defined(_TTHREAD_WIN32_)
    return GetThreadId(thr0) == GetThreadId(thr1);
#else
    return pthread_equal(thr0, thr1);
#endif
}

void tct_thrd_exit(int res)
{
#if defined(_TTHREAD_WIN32_)
    if (_tinycthread_tss_head != NULL)
    {
        _tinycthread_tss_cleanup();
    }

    ExitThread((DWORD)res);
#else
    pthread_exit((void*)(intptr_t)res);
#endif
}

int tct_thrd_join(tct_thrd_t thr, int* res)
{
#if defined(_TTHREAD_WIN32_)
    DWORD dwRes;

    if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
    {
        return tct_thrd_error;
    }
    if (res != NULL)
    {
        if (GetExitCodeThread(thr, &dwRes) != 0)
        {
            *res = (int)dwRes;
        }
        else
        {
            return tct_thrd_error;
        }
    }
    CloseHandle(thr);
#elif defined(_TTHREAD_POSIX_)
    void* pres;
    if (pthread_join(thr, &pres) != 0)
    {
        return tct_thrd_error;
    }
    if (res != NULL)
    {
        *res = (int)(intptr_t)pres;
    }
#endif
    return tct_thrd_success;
}

int tct_thrd_sleep(const struct timespec* duration, struct timespec* remaining)
{
#if !defined(_TTHREAD_WIN32_)
    int res = nanosleep(duration, remaining);
    if (res == 0)
    {
        return 0;
    }
    else if (errno == EINTR)
    {
        return -1;
    }
    else
    {
        return -2;
    }
#else
    struct timespec start;
    DWORD t;

    timespec_get(&start, TIME_UTC);

    t = SleepEx(
        (DWORD)(
            duration->tv_sec * 1000 + duration->tv_nsec / 1000000 +
            (((duration->tv_nsec % 1000000) == 0) ? 0 : 1)),
        TRUE);

    if (t == 0)
    {
        return 0;
    }
    else
    {
        if (remaining != NULL)
        {
            timespec_get(remaining, TIME_UTC);
            remaining->tv_sec -= start.tv_sec;
            remaining->tv_nsec -= start.tv_nsec;
            if (remaining->tv_nsec < 0)
            {
                remaining->tv_nsec += 1000000000;
                remaining->tv_sec -= 1;
            }
        }

        return (t == WAIT_IO_COMPLETION) ? -1 : -2;
    }
#endif
}

void tct_thrd_yield(void)
{
#if defined(_TTHREAD_WIN32_)
    Sleep(0);
#else
    sched_yield();
#endif
}

int tss_create(tss_t* key, tss_dtor_t dtor)
{
#if defined(_TTHREAD_WIN32_)
    *key = TlsAlloc();
    if (*key == TLS_OUT_OF_INDEXES)
    {
        return tct_thrd_error;
    }
    _tinycthread_tss_dtors[*key] = dtor;
#else
    if (pthread_key_create(key, dtor) != 0)
    {
        return tct_thrd_error;
    }
#endif
    return tct_thrd_success;
}

void tss_delete(tss_t key)
{
#if defined(_TTHREAD_WIN32_)
    struct TinyCThreadTSSData* data = (struct TinyCThreadTSSData*)TlsGetValue(key);
    struct TinyCThreadTSSData* prev = NULL;
    if (data != NULL)
    {
        if (data == _tinycthread_tss_head)
        {
            _tinycthread_tss_head = data->next;
        }
        else
        {
            prev = _tinycthread_tss_head;
            if (prev != NULL)
            {
                while (prev->next != data)
                {
                    prev = prev->next;
                }
            }
        }

        if (data == _tinycthread_tss_tail)
        {
            _tinycthread_tss_tail = prev;
        }

        free(data);
    }
    _tinycthread_tss_dtors[key] = NULL;
    TlsFree(key);
#else
    pthread_key_delete(key);
#endif
}

void* tss_get(tss_t key)
{
#if defined(_TTHREAD_WIN32_)
    struct TinyCThreadTSSData* data = (struct TinyCThreadTSSData*)TlsGetValue(key);
    if (data == NULL)
    {
        return NULL;
    }
    return data->value;
#else
    return pthread_getspecific(key);
#endif
}

int tss_set(tss_t key, void* val)
{
#if defined(_TTHREAD_WIN32_)
    struct TinyCThreadTSSData* data = (struct TinyCThreadTSSData*)TlsGetValue(key);
    if (data == NULL)
    {
        data = (struct TinyCThreadTSSData*)malloc(sizeof(struct TinyCThreadTSSData));
        if (data == NULL)
        {
            return tct_thrd_error;
        }

        data->value = NULL;
        data->key = key;
        data->next = NULL;

        if (_tinycthread_tss_tail != NULL)
        {
            _tinycthread_tss_tail->next = data;
        }
        else
        {
            _tinycthread_tss_tail = data;
        }

        if (_tinycthread_tss_head == NULL)
        {
            _tinycthread_tss_head = data;
        }

        if (!TlsSetValue(key, data))
        {
            free(data);
            return tct_thrd_error;
        }
    }
    data->value = val;
#else
    if (pthread_setspecific(key, val) != 0)
    {
        return tct_thrd_error;
    }
#endif
    return tct_thrd_success;
}

#if defined(_TTHREAD_EMULATE_TIMESPEC_GET_)
int _tthread_timespec_get(struct timespec* ts, int base)
{
#if defined(_TTHREAD_WIN32_)
    struct _timeb tb;
#elif !defined(CLOCK_REALTIME)
    struct timeval tv;
#endif

    if (base != TIME_UTC)
    {
        return 0;
    }

#if defined(_TTHREAD_WIN32_)
    _ftime_s(&tb);
    ts->tv_sec = (time_t)tb.time;
    ts->tv_nsec = 1000000L * (long)tb.millitm;
#elif defined(CLOCK_REALTIME)
    base = (clock_gettime(CLOCK_REALTIME, ts) == 0) ? base : 0;
#else
    gettimeofday(&tv, NULL);
    ts->tv_sec = (time_t)tv.tv_sec;
    ts->tv_nsec = 1000L * (long)tv.tv_usec;
#endif

    return base;
}
#endif /* _TTHREAD_EMULATE_TIMESPEC_GET_ */

#if defined(_TTHREAD_WIN32_)
void call_once(once_flag* flag, void (*func)(void))
{
    /* The idea here is that we use a spin lock (via the
       InterlockedCompareExchange function) to restrict access to the
       critical section until we have initialized it, then we use the
       critical section to block until the callback has completed
       execution. */
    while (flag->status < 3)
    {
        switch (flag->status)
        {
        case 0:
            if (InterlockedCompareExchange(&(flag->status), 1, 0) == 0)
            {
                InitializeCriticalSection(&(flag->lock));
                EnterCriticalSection(&(flag->lock));
                flag->status = 2;
                func();
                flag->status = 3;
                LeaveCriticalSection(&(flag->lock));
                return;
            }
            break;
        case 1:
            break;
        case 2:
            EnterCriticalSection(&(flag->lock));
            LeaveCriticalSection(&(flag->lock));
            break;
        }
    }
}
#endif /* defined(_TTHREAD_WIN32_) */

#ifdef __cplusplus
}
#endif
