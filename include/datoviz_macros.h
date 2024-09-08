/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_MACROS
#define DVZ_HEADER_PUBLIC_MACROS



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz_version.h"



/*************************************************************************************************/
/*  VK_DRIVER_FILES env variable for macOS MoltenVK                                              */
/*************************************************************************************************/

// macOS NOTE: if INCLUDE_VK_DRIVER_FILES is #defined, set the vulkan driver files to the path
// to the MoltenVK_icd.json file.
#ifdef INCLUDE_VK_DRIVER_FILES
#if OS_MACOS
__attribute__((constructor)) static void set_vk_driver_files(void)
{
    char file_path[1024] = {0};
    strncpy(file_path, __FILE__, sizeof(file_path));

    char path[1024] = {0};
    snprintf(
        path, 1024, "%s%s", dirname(file_path),
        "/../libs/vulkan/macos/MoltenVK_icd.json:/usr/local/lib/datoviz/MoltenVK_icd.json");
    setenv("VK_DRIVER_FILES", path, 1);
    // log_error("Setting VK_DRIVER_FILES to %s", path);
}
#endif
#endif



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_EXPORT
#if CC_MSVC
#ifdef DVZ_SHARED
#define DVZ_EXPORT __declspec(dllexport)
#else
#define DVZ_EXPORT __declspec(dllimport)
#endif
#define DVZ_INLINE __forceinline
#else
#define DVZ_EXPORT __attribute__((visibility("default")))
#define DVZ_INLINE static inline __attribute((always_inline))
#endif
#endif

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 0
#endif



/*************************************************************************************************/
/*  Assertions                                                                                   */
/*************************************************************************************************/

#ifndef ASSERT
#if DEBUG
#define ASSERT(x) assert((x))
#else
#define ASSERT(x)                                                                                 \
    if (!(x))                                                                                     \
    fprintf(stderr, "CRITICAL ERROR, PLEASE REPORT (%s:%d, %s)\n", __FILE_NAME__, __LINE__, #x)
#endif
#endif

#ifndef ANN
#define ANN(x) ASSERT((x) != NULL);
#endif

#define DVZ_ID_NONE 0



/*************************************************************************************************/
/*  Memory management                                                                            */
/*************************************************************************************************/

#define FREE(x)                                                                                   \
    if ((x) != NULL)                                                                              \
    {                                                                                             \
        free((x));                                                                                \
        (x) = NULL;                                                                               \
    }

#define ALIGNED_FREE(x)                                                                           \
    if (x.aligned)                                                                                \
        aligned_free(x.pointer);                                                                  \
    else                                                                                          \
        FREE(x.pointer)

#define REALLOC(x, s)                                                                             \
    {                                                                                             \
        void* _new = realloc((x), (s));                                                           \
        if (_new == NULL)                                                                         \
            exit(1);                                                                              \
        else                                                                                      \
            x = _new;                                                                             \
    }



/*************************************************************************************************/
/*  C or C++                                                                                     */
/*************************************************************************************************/

#ifdef __cplusplus
#define LANG_CPP
#define EXTERN_C_ON                                                                               \
    extern "C"                                                                                    \
    {
#define EXTERN_C_OFF }
#else
#define LANG_C
#define EXTERN_C_ON
#define EXTERN_C_OFF
#endif



/*************************************************************************************************/
/*  Mute macros                                                                                  */
/*************************************************************************************************/

#if CC_GCC
#define MUTE_ON                                                                                   \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")        \
        _Pragma("GCC diagnostic ignored \"-Wundef\"")                                             \
            _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                                     \
                _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"")                           \
                    _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                             \
                        _Pragma("GCC diagnostic ignored \"-Wunused\"")                            \
                            _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")              \
                                _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"")           \
                                    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")        \
                                        _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"")

#define MUTE_OFF _Pragma("GCC diagnostic pop")
#elif CC_CLANG
#define MUTE_ON                                                                                   \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")    \
        _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                                       \
            _Pragma("clang diagnostic ignored \"-Wredundant-decls\"") _Pragma(                    \
                "clang diagnostic ignored \"-Wcast-qual\"")                                       \
                _Pragma("clang diagnostic ignored \"-Wstrict-overflow\"") _Pragma(                \
                    "clang diagnostic ignored \"-Wswitch-default\"")                              \
                    _Pragma("clang diagnostic ignored \"-Wcast-align\"") _Pragma(                 \
                        "clang diagnostic ignored \"-Wundef\"")                                   \
                        _Pragma("clang diagnostic ignored \"-Wmissing-braces\"") _Pragma(         \
                            "clang diagnostic ignored \"-Wnullability-extension\"")               \
                            _Pragma("clang diagnostic ignored \"-Wunguarded-availability-new\"")  \
                                _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

#define MUTE_OFF _Pragma("clang diagnostic pop")
#else
// MSVC: TODO
#define MUTE_ON
// #pragma warning(push)
#define MUTE_OFF
// #pragma warning(pop)
#endif



#endif
