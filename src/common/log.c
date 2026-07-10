/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

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
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#if OS_LINUX
// #include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif OS_MACOS
#include <pthread.h>
#elif OS_WINDOWS
#include <windows.h>
#endif

#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "mutex_internal.h"

#if OS_WINDOWS
MUTE_ON
// #include "ansicolor-w32.h"
MUTE_OFF
#endif

static struct
{
    void* udata;
    log_LockFn lock;
    void* intercept_udata;
    log_InterceptFn intercept;
    FILE* fp;
    int level;
    int quiet;
} L;

static const char* level_names[] = {"T", "D", "I", "W", "E", "F"};

static const char* level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};

static bool _log_color_enabled(void)
{
    const char* env = getenv("DVZ_LOG_COLOR");
    if (env != NULL)
    {
        if (strcmp(env, "0") == 0 || strcmp(env, "false") == 0 || strcmp(env, "FALSE") == 0 ||
            strcmp(env, "off") == 0 || strcmp(env, "OFF") == 0)
            return false;
        if (strcmp(env, "1") == 0 || strcmp(env, "true") == 0 || strcmp(env, "TRUE") == 0 ||
            strcmp(env, "on") == 0 || strcmp(env, "ON") == 0)
            return true;
    }

    if (getenv("NO_COLOR") != NULL)
        return false;

    return true;
}

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

static uint64_t get_thread_idx(void)
{
    uint64_t tid = 0;
#if OS_MACOS
    // macOS
    pthread_threadid_np(NULL, &tid);
#elif OS_WINDOWS
    // Windows
    // Use native WinAPI to obtain a stable thread ID.
    tid = (uint64_t)(GetCurrentThreadId());
#elif defined(__EMSCRIPTEN__)
    tid = 1;
#else
    // Linux
    tid = (uint64_t)(syscall(__NR_gettid));
#endif
    assert(tid != 0);

    return tid;
}

void log_set_udata(void* udata) { L.udata = udata; }

void log_set_lock(log_LockFn fn) { L.lock = fn; }

void log_set_fp(FILE* fp) { L.fp = fp; }

static int log_level_explicitly_set;

void log_set_level(int level)
{
    // log_debug("set log level to %d", level);
    log_level_explicitly_set = 1;
    L.level = level;
}

void log_set_quiet(int enable) { L.quiet = enable ? 1 : 0; }

/**
 * Register a log interception callback.
 *
 * @param fn callback invoked with fully formatted log messages
 * @param udata opaque pointer forwarded to the callback
 * @return void this function does not return a value
 */
void log_set_intercept(log_InterceptFn fn, void* udata)
{
    L.intercept = fn;
    L.intercept_udata = udata;
}

void log_log(int level, const char* file, int line, const char* fmt, ...)
{
    // On compilers without __attribute__((constructor)) (MSVC), _log_init() never runs, so the
    // zero-initialized level stays at LOG_TRACE and floods stderr in embedded/library builds.
    // Pick up DVZ_LOG_LEVEL lazily unless a level was already set.
    if (!log_level_explicitly_set)
    {
        log_set_level_env();
    }
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

    char msg[2048] = {0};
    va_list msg_args;
    va_start(msg_args, fmt);
    MUTE_NONLITERAL_ON
    dvz_vsnprintf(msg, sizeof(msg), fmt, msg_args);
    MUTE_NONLITERAL_OFF
    va_end(msg_args);

    bool suppress_output = false;
    if (L.intercept)
    {
        suppress_output = L.intercept(L.intercept_udata, level, file, line, msg) != 0;
    }

    /* Log to stderr */
    if (!L.quiet && !suppress_output)
    {
        char buf[24] = {0};
        clock_t uptime = (clock() / (CLOCKS_PER_SEC / 1000)) % 1000;
        buf[strftime(buf, sizeof(buf), "%H:%M:%S.    ", lt)] = '\0';
        // HH:MM:SS.MMS(thread_id)
        dvz_snprintf(&buf[9], 12, "%03d T%01u", (int)uptime, tid);

        bool use_color = _log_color_enabled();
        if (use_color)
        {
            dvz_fprintf(
                stderr, "%s %s%-1s\x1b[0m \x1b[90m%18s:%04d:\x1b[0m %s", buf,
                level_colors[level], level_names[level], file, line, level_colors[level]);
        }
        else
        {
            dvz_fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
        }
        dvz_fprintf(stderr, "%s", msg);
        if (use_color)
            dvz_fprintf(stderr, "\x1b[0m");
        dvz_fprintf(stderr, "\n");
        fflush(stderr);
    }

    /* Log to file */
    if (L.fp && !suppress_output)
    {
        char buf[32] = {0};
        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
        dvz_fprintf(L.fp, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
        dvz_fprintf(L.fp, "%s", msg);
        dvz_fprintf(L.fp, "\n");
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



/**
 * Set the logger level from the DVZ_LOG_LEVEL environment variable.
 *
 * Accepts string names ("trace", "debug", "info", "warn", "error", "fatal")
 * or integer values ("0"–"5").
 */
void log_set_level_env(void)
{
    const char* level = getenv("DVZ_LOG_LEVEL");
    int level_int = DVZ_DEFAULT_LOG_LEVEL;
    if (level != NULL)
    {
        if (strcmp(level, "trace") == 0)       level_int = LOG_TRACE;
        else if (strcmp(level, "debug") == 0)  level_int = LOG_DEBUG;
        else if (strcmp(level, "info") == 0)   level_int = LOG_INFO;
        else if (strcmp(level, "warn") == 0)   level_int = LOG_WARN;
        else if (strcmp(level, "error") == 0)  level_int = LOG_ERROR;
        else if (strcmp(level, "fatal") == 0)  level_int = LOG_FATAL;
        else                                    level_int = (int)strtol(level, NULL, 10);
    }
    log_set_level(level_int);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((constructor))
static void _log_init(void) { log_set_level_env(); }
#endif
