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

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Guarded wrappers                                                                             */
/*************************************************************************************************/

EXTERN_C_ON

static inline int dvz_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
#if defined(_MSC_VER)
    return _vsnprintf_s(buffer, size, _TRUNCATE, format, args);
#elif defined(__STDC_LIB_EXT1__)
    return vsnprintf_s(buffer, size, _TRUNCATE, format, args);
#else
    (void)size;
    return vsnprintf( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        buffer, size, format, args);
#endif
}



static inline int dvz_snprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = dvz_vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}



static inline int dvz_vfprintf(FILE* stream, const char* format, va_list args)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return vfprintf_s(stream, format, args);
#else
    return vfprintf( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        stream, format, args);
#endif
}



static inline int dvz_fprintf(FILE* stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = dvz_vfprintf(stream, format, args);
    va_end(args);
    return ret;
}



static inline int
dvz_memcpy(void* destination, size_t destination_size, const void* source, size_t size)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return memcpy_s(destination, destination_size, source, size);
#else
    (void)destination_size;
    memcpy( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        destination, source, size);
    return 0;
#endif
}



static inline int dvz_memset(void* destination, size_t destination_size, int value, size_t size)
{
#if defined(__STDC_LIB_EXT1__)
    return memset_s(destination, destination_size, value, size);
#elif defined(_MSC_VER)
    (void)destination_size;
    memset( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        destination, value, size);
    return 0;
#else
    (void)destination_size;
    memset( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        destination, value, size);
    return 0;
#endif
}



static inline int
dvz_memmove(void* destination, size_t destination_size, const void* source, size_t size)
{
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
    return memmove_s(destination, destination_size, source, size);
#else
    (void)destination_size;
    memmove( // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        destination, source, size);
    return 0;
#endif
}



EXTERN_C_OFF
