/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example common helpers                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "_compat.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define EXAMPLE_CHECK(condition, message)                                                        \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            dvz_fprintf(stderr, "%s\n", message);                                                \
            goto cleanup;                                                                         \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool example_parse_u32(const char* text, uint32_t* out);

bool example_arg_has(int argc, char** argv, const char* name);

bool example_arg_value(int argc, char** argv, const char* name, const char** out);

bool example_arg_value_prefix(int argc, char** argv, const char* prefix, const char** out);

void example_outpath(const char* exe, const char* name, char* out, size_t size);

bool example_recording_path(int argc, char** argv, const char* default_path, char* out, size_t size);

uint32_t example_frame_count_from_text(const char* text);

uint32_t example_frame_count(int argc, char** argv);

uint32_t example_frame_count_any(int argc, char** argv);
