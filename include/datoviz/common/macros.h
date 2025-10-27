/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common macros                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <string.h>



/*************************************************************************************************/
/*  Export                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_EXPORT
#if defined(_MSC_VER) || CC_MSVC
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
/*  Portable __FILE_NAME__ fallback                                                              */
/*************************************************************************************************/

// Some compilers (GCC/Clang) provide __FILE_NAME__ which is the basename of __FILE__.
// MSVC does not define it, so provide a compatible fallback that computes a const char*
// at runtime from the __FILE__ string literal. This keeps logging/asserts concise.
#ifndef __FILE_NAME__
#define __FILE_NAME__                                                                             \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1                                        \
                             : (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__))
#endif



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



#ifdef LANG_CPP
#define INIT(t, n) t n = {};
#else
#define INIT(t, n) t n = {0};
#endif



/*************************************************************************************************/
/*  Assumptions                                                                                  */
/*************************************************************************************************/

#if CC_MSVC
#define DVZ_ASSUME(x) __assume(x)
#elif CC_CLANG
#define DVZ_ASSUME(x) __builtin_assume(x)
#elif CC_GCC
#define DVZ_ASSUME(x)                                                                             \
    do                                                                                            \
    {                                                                                             \
        if (!(x))                                                                                 \
        {                                                                                         \
            __builtin_unreachable();                                                              \
        }                                                                                         \
    } while (0)
#else
#define DVZ_ASSUME(x) ((void)0)
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
                        _Pragma("GCC diagnostic ignored \"-Wunused\"") _Pragma(                   \
                            "GCC diagnostic ignored \"-Wunused-parameter\"")                      \
                            _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"") _Pragma(      \
                                "GCC diagnostic ignored \"-Wswitch-default\"")                    \
                                _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"") _Pragma(   \
                                    "GCC diagnostic ignored \"-Wmissing-field-initializers\"")

#define MUTE_OFF _Pragma("GCC diagnostic pop")
#elif CC_CLANG
#define MUTE_ON                                                                                   \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")    \
        _Pragma("clang diagnostic ignored \"-Wcast-qual\"") _Pragma(                              \
            "clang diagnostic ignored \"-Wredundant-decls\"")                                     \
            _Pragma("clang diagnostic ignored \"-Wcast-qual\"") _Pragma(                          \
                "clang diagnostic ignored \"-Wstrict-overflow\"")                                 \
                _Pragma("clang diagnostic ignored \"-Wswitch-default\"") _Pragma(                 \
                    "clang diagnostic ignored \"-Wcast-align\"")                                  \
                    _Pragma("clang diagnostic ignored \"-Wundef\"") _Pragma(                      \
                        "clang diagnostic ignored \"-Wmissing-braces\"")                          \
                        _Pragma("clang diagnostic ignored \"-Wnullability-extension\"") _Pragma(  \
                            "clang diagnostic ignored \"-Wunguarded-availability-new\"")          \
                            _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")     \
                                _Pragma(                                                          \
                                    "clang diagnostic ignored \"-Wnullability-completeness\"")
_Pragma("clang diagnostic ignored \"-Wmissing-field-initializers\"")
    _Pragma("clang diagnostic ignored \"-Wunused-private-field\"")

#define MUTE_OFF _Pragma("clang diagnostic pop")
#else
// MSVC: TODO
#define MUTE_ON
// #pragma warning(push)
#define MUTE_OFF
// #pragma warning(pop)
#endif



#if CC_CLANG
#define MUTE_NONLITERAL_ON                                                                        \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#define MUTE_NONLITERAL_OFF _Pragma("clang diagnostic pop")
#else
#define MUTE_NONLITERAL_ON
#define MUTE_NONLITERAL_OFF
#endif
