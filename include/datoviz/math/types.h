/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mathematical types                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_2PI 6.28318530717958647692
// #define M_PI_2 1.57079632679489650726

#define M_INV_255 0.00392156862745098

#define EPSILON 1e-10

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t _ZERO_OFFSET[3] = {0, 0, 0};
#pragma GCC diagnostic pop
#define DVZ_ZERO_OFFSET _ZERO_OFFSET



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b) MAX(MIN((x), (b)), (a))

// #define TO_BYTE(x)   (uint8_t)round(CLIP((x), 0, 1) * 255)
// #define FROM_BYTE(x) ((x) / 255.0)

#define _DMAT4_IDENTITY_INIT                                                                      \
    {                                                                                             \
        {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, { 0.0, 0.0, 0.0, 1.0 }  \
    }



/*************************************************************************************************/
/*  ID                                                                                           */
/*************************************************************************************************/

typedef uint64_t DvzId;



/*************************************************************************************************/
/*  Byte size                                                                                    */
/*************************************************************************************************/

#define GB 1073741824
#define MB 1048576
#define KB 1024

static char _PRETTY_SIZE[64] = {0};

typedef uint64_t DvzSize;

static inline char* pretty_size(DvzSize size)
{
    if (size <= 8192)
    {
        snprintf(_PRETTY_SIZE, 64, "%" PRIu64 " bytes", size);
        return _PRETTY_SIZE;
    }
    float s = (float)size;
    const char* u;
    if (size >= GB)
    {
        s /= GB;
        u = "GB";
    }
    else if (size >= MB)
    {
        s /= MB;
        u = "MB";
    }
    else if (size >= KB)
    {
        s /= KB;
        u = "KB";
    }
    else
    {
        u = "bytes";
    }
    snprintf(_PRETTY_SIZE, 64, "%.1f %s", s, u);
    return _PRETTY_SIZE;
}



/*************************************************************************************************/
/*  8-bit integers                                                                               */
/*************************************************************************************************/

typedef uint8_t cvec2[2];
typedef uint8_t cvec3[3];
typedef uint8_t cvec4[4]; // used for color index



/*************************************************************************************************/
/*  16-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int16_t svec2[2];
typedef int16_t svec3[3];
typedef int16_t svec4[4]; // used for glyph as vec4 of uint16

/* Unsigned */
typedef uint16_t usvec2[2];
typedef uint16_t usvec3[3];
typedef uint16_t usvec4[4]; // used for glyph as vec4 of uint16



/*************************************************************************************************/
/*  32-bit integers                                                                              */
/*************************************************************************************************/

/* Unsigned */
typedef uint32_t uvec2[2];
typedef uint32_t uvec3[3];
typedef uint32_t uvec4[4];

/* Index */
typedef uint32_t DvzIndex;



/*************************************************************************************************/
/*  Single-precision floating-point numbers                                                      */
/*************************************************************************************************/

// NOTE: copy from cglm

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#include <stdalign.h>
#endif

#if defined(_MSC_VER)
/* do not use alignment for older visual studio versions */
#if _MSC_VER < 1913 /*  Visual Studio 2017 version 15.6  */
#define CGLM_ALL_UNALIGNED
#define CGLM_ALIGN(X) /* no alignment */
#else
#define CGLM_ALIGN(X) __declspec(align(X))
#endif
#else
#define CGLM_ALIGN(X) __attribute((aligned(X)))
#endif

#ifndef CGLM_ALL_UNALIGNED
#define CGLM_ALIGN_IF(X) CGLM_ALIGN(X)
#else
#define CGLM_ALIGN_IF(X) /* no alignment */
#endif

#ifdef __AVX__
#define CGLM_ALIGN_MAT CGLM_ALIGN(32)
#else
#define CGLM_ALIGN_MAT CGLM_ALIGN(16)
#endif

#ifndef CGLM_HAVE_BUILTIN_ASSUME_ALIGNED

#if defined(__has_builtin)
#if __has_builtin(__builtin_assume_aligned)
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 1
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 1
#endif
#endif

#ifndef CGLM_HAVE_BUILTIN_ASSUME_ALIGNED
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 0
#endif

#endif

#if CGLM_HAVE_BUILTIN_ASSUME_ALIGNED
#define CGLM_ASSUME_ALIGNED(expr, alignment) __builtin_assume_aligned((expr), (alignment))
#else
#define CGLM_ASSUME_ALIGNED(expr, alignment) (expr)
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type) ((type*)CGLM_ASSUME_ALIGNED((expr), alignof(type)))
#elif defined(_MSC_VER)
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type)                                                   \
    ((type*)CGLM_ASSUME_ALIGNED((expr), __alignof(type)))
#else
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type)                                                   \
    ((type*)CGLM_ASSUME_ALIGNED((expr), __alignof__(type)))
#endif

// Signed 32-bit integers.
typedef int32_t ivec2[2];
typedef int32_t ivec3[3];
typedef int32_t ivec4[4];

typedef float vec2[2];
typedef float vec3[3];
typedef CGLM_ALIGN_IF(16) float vec4[4];

typedef vec3 mat3[3];
typedef vec2 mat3x2[3];
typedef vec4 mat3x4[3];

typedef CGLM_ALIGN_IF(16) vec2 mat2[2];
typedef vec3 mat2x3[2];
typedef vec4 mat2x4[2];

typedef CGLM_ALIGN_MAT vec4 mat4[4];
typedef vec2 mat4x2[4];
typedef vec3 mat4x3[4];

typedef vec2 fvec2;
typedef vec3 fvec3;
typedef vec4 fvec4;



/*************************************************************************************************/
/*  Double-precision floating-point numbers                                                      */
/*************************************************************************************************/

/* Array types */
typedef double dvec2[2];
typedef double dvec3[3];
typedef double dvec4[4];

/* Matrix types */
typedef dvec2 dmat2[2];
typedef dvec3 dmat3[3];
typedef dvec4 dmat4[4];
