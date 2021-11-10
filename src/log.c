/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if OS_LINUX
// #include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "_log.h"
#include "_macros.h"
#include "_mutex.h"

#define MAX_THREADS 64

#if OS_WIN32
MUTE_ON
// #include "ansicolor-w32.h"
MUTE_OFF
#endif

static struct
{
    void* udata;
    log_LockFn lock;
    FILE* fp;
    int level;
    int quiet;
} L;

static const char* level_names[] = {"T", "D", "I", "W", "E", "F"};

#ifdef LOG_USE_COLOR
static const char* level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void lock(void)
{
    if (L.lock)
    {
        L.lock(L.udata, 1);
    }
}

static void unlock(void)
{
    if (L.lock)
    {
        L.lock(L.udata, 0);
    }
}

static uint64_t _THREADS[MAX_THREADS];

static uint64_t get_thread_idx()
{
    uint64_t tid = 0;
#if OS_MACOS
    // macOS
    pthread_threadid_np(NULL, &tid);
#elif OS_WIN32
    // Windows
    // TODO
#else
    // Linux
    tid = (uint64_t)(syscall(__NR_gettid));
#endif
    assert(tid != 0);

    // Relative thread idx lookup.
    for (uint32_t i = 0; i < MAX_THREADS; i++)
    {
        if (_THREADS[i] == 0)
        {
            _THREADS[i] = tid;
            return i;
        }
        if (_THREADS[i] == tid)
            return i;
    }

    return tid;
}

void log_set_udata(void* udata) { L.udata = udata; }

void log_set_lock(log_LockFn fn) { L.lock = fn; }

void log_set_fp(FILE* fp) { L.fp = fp; }

void log_set_level(int level)
{
    // log_debug("set log level to %d", level);
    L.level = level;
}

void log_set_quiet(int enable) { L.quiet = enable ? 1 : 0; }

void log_log(int level, const char* file, int line, const char* fmt, ...)
{
    if (level < L.level)
    {
        return;
    }

    /* Acquire lock */
    lock();

    /* Get current time */
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    uint32_t tid = get_thread_idx() % 1000;

    /* Log to stderr */
    if (!L.quiet)
    {
        va_list args;
        char buf[24];
        clock_t uptime = (clock() / (CLOCKS_PER_SEC / 1000)) % 1000;
        buf[strftime(buf, sizeof(buf), "%H:%M:%S.    ", lt)] = '\0';
        // HH:MM:SS.MMS(thread_id)
        snprintf(&buf[9], 12, "%03d T%01u", (int)uptime, tid);

#ifdef LOG_USE_COLOR
        fprintf(
            stderr, "%s %s%-1s\x1b[0m \x1b[90m%18s:%04d:\x1b[0m %s", buf, level_colors[level],
            level_names[level], file, line, level_colors[level]);
#else
        fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
#endif
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
#ifdef LOG_USE_COLOR
        fprintf(stderr, "\x1b[0m");
#endif
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    /* Log to file */
    if (L.fp)
    {
        va_list args;
        char buf[32];
        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
        fprintf(L.fp, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
        va_start(args, fmt);
        vfprintf(L.fp, fmt, args);
        va_end(args);
        fprintf(L.fp, "\n");
        fflush(L.fp);
    }

    /* Release lock */
    unlock();
}



// Use a mutex for the logging lock, prevent multiple threads from simultaneously writing to the
// standard output.
static DvzMutex mutex;

static void _lock(void* udata, int lock)
{
    if (lock)
        dvz_mutex_lock(&mutex);
    else
        dvz_mutex_unlock(&mutex);
}

void log_set_level_env(void)
{
    const char* level = getenv("DVZ_LOG_LEVEL");
    int level_int = DVZ_DEFAULT_LOG_LEVEL;
    if (level != NULL)
        level_int = strtol(level, NULL, 10);
    log_set_level(level_int);

    log_set_lock(_lock);
}
