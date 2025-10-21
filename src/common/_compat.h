/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Portable wrappers around optional secure CRT extensions                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#ifndef __STDC_WANT_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>



/*************************************************************************************************/
/*  Guarded wrappers                                                                             */
/*************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

static inline int
dvz_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
#if defined(_MSC_VER)
    return _vsnprintf_s(buffer, size, _TRUNCATE, format, args);
#elif defined(__STDC_LIB_EXT1__)
    return vsnprintf_s(buffer, size, _TRUNCATE, format, args);
#else
    (void)size;
    return vsnprintf(buffer, size, format, args); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#endif
}



static inline int
dvz_snprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = dvz_vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}



static inline int
dvz_vfprintf(FILE* stream, const char* format, va_list args)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return vfprintf_s(stream, format, args);
#else
    return vfprintf(stream, format, args); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#endif
}



static inline int
dvz_fprintf(FILE* stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = dvz_vfprintf(stream, format, args);
    va_end(args);
    return ret;
}



static inline int
dvz_memcpy(void* destination, size_t destination_size, const void* source, size_t count)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return memcpy_s(destination, destination_size, source, count);
#else
    (void)destination_size;
    memcpy(destination, source, count); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    return 0;
#endif
}



static inline int
dvz_memset(void* destination, size_t destination_size, int value, size_t count)
{
#if defined(__STDC_LIB_EXT1__)
    return memset_s(destination, destination_size, value, count);
#elif defined(_MSC_VER)
#if _MSC_VER >= 1900
    return memset_s(destination, destination_size, value, count);
#else
    (void)destination_size;
    memset(destination, value, count); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    return 0;
#endif
#else
    (void)destination_size;
    memset(destination, value, count); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    return 0;
#endif
}



static inline int
dvz_memmove(void* destination, size_t destination_size, const void* source, size_t count)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return memmove_s(destination, destination_size, source, count);
#else
    (void)destination_size;
    memmove(destination, source, count); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    return 0;
#endif
}

#ifdef __cplusplus
}
#endif
