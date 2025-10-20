/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stdio.h>



#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    typedef void (*log_LockFn)(void* udata, int lock);

    enum
    {
        LOG_TRACE,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_FATAL
    };

#ifdef DEBUG
#define DVZ_DEFAULT_LOG_LEVEL LOG_INFO
#else
// In RELEASE mode, by default (when DVZ_LOG_LEVEL env variable is not set), effectively
// disables all logging.
#define DVZ_DEFAULT_LOG_LEVEL 10
#endif

#define log_trace(...) log_log(LOG_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)

    void log_set_udata(void* udata);
    void log_set_lock(log_LockFn fn);
    void log_set_fp(FILE* fp);
    void log_set_level(int level);
    void log_set_quiet(int enable);

    void log_log(int level, const char* file, int line, const char* fmt, ...);

    void log_set_level_env(void);

#ifdef __cplusplus
}
#endif

#endif
