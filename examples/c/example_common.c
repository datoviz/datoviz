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

#include "datoviz/geom/types.h"
#include "datoviz/scene.h"

#include "_alloc.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>



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
 * Return whether a command-line token is present.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param name token to find
 * @return true when the token appears after argv[0]
 */
bool example_arg_has(int argc, char** argv, const char* name)
{
    if (argc < 2 || argv == NULL || name == NULL)
        return false;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL && strcmp(argv[i], name) == 0)
            return true;
    }
    return false;
}



/**
 * Return the value following a command-line token.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param name token to find
 * @param out value output
 * @return true when the token and a non-NULL following value are present
 */
bool example_arg_value(int argc, char** argv, const char* name, const char** out)
{
    if (argc < 3 || argv == NULL || name == NULL || out == NULL)
        return false;

    for (int i = 1; i + 1 < argc; i++)
    {
        if (argv[i] != NULL && strcmp(argv[i], name) == 0 && argv[i + 1] != NULL)
        {
            *out = argv[i + 1];
            return true;
        }
    }
    return false;
}



/**
 * Return the value suffix following a command-line token prefix.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param prefix token prefix to find
 * @param out value output
 * @return true when a token with the prefix is present
 */
bool example_arg_value_prefix(int argc, char** argv, const char* prefix, const char** out)
{
    if (argc < 2 || argv == NULL || prefix == NULL || out == NULL)
        return false;

    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0)
        return false;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL && strncmp(argv[i], prefix, prefix_len) == 0)
        {
            *out = argv[i] + prefix_len;
            return true;
        }
    }
    return false;
}



/**
 * Build an output path next to the example executable.
 *
 * @param exe executable path from argv[0]
 * @param name output file name
 * @param out destination path buffer
 * @param size destination path buffer size
 */
void example_outpath(const char* exe, const char* name, char* out, size_t size)
{
    if (name == NULL || out == NULL || size == 0)
        return;

    const char* slash = exe != NULL ? strrchr(exe, '/') : NULL;
    if (slash != NULL)
    {
        size_t prefix_len = (size_t)(slash - exe);
        if (prefix_len > (size_t)INT_MAX)
            prefix_len = (size_t)INT_MAX;
        dvz_snprintf(out, size, "%.*s/%s", (int)prefix_len, exe, name);
    }
    else
    {
        dvz_snprintf(out, size, "%s", name);
    }
}



/**
 * Return whether an example should record a DVZR stream.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param default_path default path next to the executable
 * @param out output recording path
 * @param size output buffer size
 * @return whether DVZR recording was requested
 */
bool example_recording_path(int argc, char** argv, const char* default_path, char* out, size_t size)
{
    if (argc < 2 || argv == NULL || default_path == NULL || out == NULL || size == 0)
        return false;

    const char* value = NULL;
    if (example_arg_has(argc, argv, "record"))
    {
        dvz_snprintf(out, size, "%s", default_path);
        return true;
    }
    if (example_arg_value_prefix(argc, argv, "record=", &value))
    {
        dvz_snprintf(out, size, "%s", value);
        return true;
    }
    if (example_arg_value(argc, argv, "--record", &value))
    {
        dvz_snprintf(out, size, "%s", value);
        return true;
    }
    return false;
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



/**
 * Upload one CPU geometry object into a mesh visual.
 *
 * @param visual target mesh visual
 * @param geometry CPU geometry object
 * @return true on success, false on error
 */
bool example_mesh_geometry(DvzVisual* visual, const DvzGeometry* geometry)
{
    return dvz_mesh_set_geometry(visual, geometry) == 0;
}
