/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common macros                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Environment variables                                                                        */
/*************************************************************************************************/

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
