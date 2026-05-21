/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example common helpers                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_common.h"

#include <limits.h>
#include <stdlib.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse a bounded unsigned 32-bit integer.
 *
 * @param text input text
 * @param out parsed value output
 * @return true when the full string is a valid unsigned integer
 */
bool example_parse_u32(const char* text, uint32_t* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;

    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0'))
        return false;

    if (value > UINT32_MAX)
        value = UINT32_MAX;

    *out = (uint32_t)value;
    return true;
}



/**
 * Parse one command-line token as a frame count.
 *
 * @param text command-line text, or NULL
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count_from_text(const char* text)
{
    uint32_t out = 0;
    return example_parse_u32(text, &out) ? out : 0;
}



/**
 * Parse the first positional argument as a frame count.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;

    return example_frame_count_from_text(argv[1]);
}



/**
 * Parse the first numeric command-line argument as a frame count.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count_any(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;

    uint32_t out = 0;
    for (int i = 1; i < argc; i++)
    {
        if (example_parse_u32(argv[i], &out))
            return out;
    }
    return 0;
}
