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
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <libgen.h>
#include <stdbool.h>
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
__attribute__((constructor)) static void set_vk_driver_files(void)
{
#if OS_MACOS
    char file_path[1024] = {0};
    strncpy(file_path, __FILE__, sizeof(file_path));

    char path[1024] = {0};
    snprintf(
        path, 1024, "%s%s", dirname(file_path),
        "/../libs/vulkan/macos/MoltenVK_icd.json:/usr/local/lib/datoviz/MoltenVK_icd.json");
    setenv("VK_DRIVER_FILES", path, 1);
// log_error("Setting VK_DRIVER_FILES to %s", path);
#endif
}
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



// Null ID
#define DVZ_ID_NONE 0



// Box
#define DVZ_BOX_NDC (DvzBox){-1, +1, -1, +1, -1, +1}


/*************************************************************************************************/
/*  Colors                                                                                       */
/*************************************************************************************************/

// Colors
#define DVZ_COLOR_CVEC4 1

#define COLOR_UINT_MAX  255
#define COLOR_FLOAT_MAX 1.0

#define ALPHA_U2F(x) (x / 255.0)
#define ALPHA_F2U(x) ((uint8_t)round(CLIP((x), 0.0, 1.0) * 255.0))

#define COLOR_U2FV(r8, g8, b8, a8) ALPHA_U2F(r8), ALPHA_U2F(g8), ALPHA_U2F(b8), ALPHA_U2F(a8)
#define COLOR_U2F(rgba8)           COLOR_U2FV(rgba8[0], rgba8[1], rgba8[2], rgba8[3])

#define COLOR_F2UV(rf, gf, bf, af) ALPHA_F2U(rf), ALPHA_F2U(gf), ALPHA_F2U(bf), ALPHA_F2U(af)
#define COLOR_F2U(rgbaf)           COLOR_F2UV(rgbaf[0], rgbaf[1], rgbaf[2], rgbaf[3])



#if DVZ_COLOR_CVEC4

#define DvzColor         cvec4
#define DvzAlpha         uint8_t
#define DVZ_FORMAT_COLOR DVZ_FORMAT_R8G8B8A8_UNORM
#define COLOR_MAX        COLOR_UINT_MAX

// convert from default (either uint or float depending on DVZ_COLOR_CVEC4) to float
#define COLOR_D2F(rgba8)           COLOR_U2F(rgba8)
#define COLOR_D2FV(r8, g8, b8, a8) COLOR_U2FV(r8, g8, b8, a8)

// from default to uint
#define COLOR_D2U(rgba8)           rgba8
#define COLOR_D2UV(r8, g8, b8, a8) r8, g8, b8, a8

// from uint to default
#define COLOR_U2D(rgba8)           rgba8
#define COLOR_U2DV(r8, g8, b8, a8) r8, g8, b8, a8

// from float to default
#define COLOR_F2D(rgbaf) COLOR_F2U(rgbaf)

// from default to float
#define ALPHA_D2F(a) ALPHA_U2F(a)

// from float to default
#define ALPHA_F2D(a) ALPHA_F2U(a)

// from default to uint
#define ALPHA_D2U(a) a

// from uint to default
#define ALPHA_U2D(a) a

#else

#define DvzColor                   vec4
#define DvzAlpha                   float
#define DVZ_FORMAT_COLOR           DVZ_FORMAT_R32G32B32A32_SFLOAT
#define COLOR_MAX                  COLOR_FLOAT_MAX

// convert from default (either uint or float depending on DVZ_COLOR_CVEC4) to float
#define COLOR_D2F(rgbf)            rgbf
#define COLOR_D2FV(rf, gf, bf, af) rf, gf, bf, af

// from default to uint
#define COLOR_D2U(rgbf)            COLOR_F2U(rgbf)
#define COLOR_D2UV(rf, gf, bf, af) COLOR_F2UV(rf, gf, bf, af)

// from uint to default
#define COLOR_U2D(rgba8)           COLOR_U2F(rgba8)
#define COLOR_U2DV(rf, gf, bf, af) COLOR_U2FV(rf, gf, bf, af)

// from float to default
#define COLOR_F2D(rgbaf)           rgbaf

// from default to float
#define ALPHA_D2F(a)               a

// from float to default
#define ALPHA_F2D(a)               a

// from default to uint
#define ALPHA_D2U(a)               ALPHA_F2U(a)

// from uint to default
#define ALPHA_U2D(a)               ALPHA_U2F(a)

#endif

// HACK: other modules may define symbols with the same name
#ifdef WHITE
#undef WHITE
#endif

#ifdef BLACK
#undef BLACK
#endif

#ifdef GRAY
#undef GRAY
#endif

#define ALPHA_MAX      COLOR_MAX
#define GRAY(v8)       COLOR_U2DV(v8, v8, v8, 255)
#define WHITE          GRAY(255)
#define BLACK          GRAY(0)
#define RED            COLOR_U2DV(255, 0, 0, 255)
#define GREEN          COLOR_U2DV(0, 255, 0, 255)
#define BLUE           COLOR_U2DV(0, 0, 255, 255)
#define CYAN           COLOR_U2DV(0, 255, 255, 255)
#define PURPLE         COLOR_U2DV(255, 0, 255, 255)
#define YELLOW         COLOR_U2DV(0, 255, 255, 255)
#define WHITE_ALPHA(a) COLOR_U2DV(255, 255, 255, a)



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

#define REALLOC(T, x, s)                                                                          \
    {                                                                                             \
        T _new = (T)realloc((x), (s));                                                            \
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



/*************************************************************************************************/
/*  Error handling                                                                               */
/*************************************************************************************************/

EXTERN_C_ON

// Error callback function type.
typedef void (*DvzErrorCallback)(const char* message);

extern char error_message[2048];
extern DvzErrorCallback error_callback;

DVZ_EXPORT extern void
dvz_assert(bool assertion, const char* message, const char* filename, int line);

EXTERN_C_OFF



/*************************************************************************************************/
/*  Assertions                                                                                   */
/*************************************************************************************************/

#ifndef ASSERT
#if DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) dvz_assert(x, #x, __FILE_NAME__, __LINE__)
#endif
#endif



// ASSERT NOT NULL
#ifndef ANN
#define ANN(x) ASSERT((x) != NULL);
#endif



#endif
