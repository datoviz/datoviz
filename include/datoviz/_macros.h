/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common macros                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MACROS
#define DVZ_HEADER_MACROS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../datoviz_macros.h"



/*************************************************************************************************/
/*  Build macros                                                                                 */
/*************************************************************************************************/

#ifndef SPIRV_DIR
#define SPIRV_DIR ""
#endif



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])

#ifdef LANG_CPP
#define INIT(t, n) t n = {};
#else
#define INIT(t, n) t n = {0};
#endif

#define fsizeof(type, member) sizeof(((type*)0)->member)



#if CC_CLANG
#define MUTE_NONLITERAL_ON                                                                        \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#define MUTE_NONLITERAL_OFF _Pragma("clang diagnostic pop")
#else
#define MUTE_NONLITERAL_ON
#define MUTE_NONLITERAL_OFF
#endif



/*************************************************************************************************/
/*  Environment variables                                                                        */
/*************************************************************************************************/

#define IF_VERBOSE if (getenv("DVZ_VERBOSE") && (strncmp(getenv("DVZ_VERBOSE"), "req", 3) == 0))

static inline bool checkenv(const char* x)
{
    char* env = getenv(x);
    return env && strnlen(env, 1) >= 1 && (strncmp(env, "0", 1) != 0);
}

static inline int32_t getenvint(const char* x)
{
    char* env = getenv(x);
    if (env == NULL)
        return -1;
    return atoi(env);
}



#endif
