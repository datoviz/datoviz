/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene resource key helpers                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Format a key and report whether it fit in the destination buffer.
 *
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @param format the printf-style key format
 * @param args the format arguments
 * @return whether the key was written without truncation
 */
static bool _format_key_v(char* out, size_t out_size, const char* format, va_list args)
{
    ANN(out);
    ANN(format);
    if (out_size == 0)
        return false;
    int ret = dvz_vsnprintf(out, out_size, format, args);
    if (ret < 0 || (size_t)ret >= out_size)
    {
        out[0] = '\0';
        return false;
    }
    return true;
}



/**
 * Format a key and report whether it fit in the destination buffer.
 *
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @param format the printf-style key format
 * @return whether the key was written without truncation
 */
static bool _format_key(char* out, size_t out_size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool ok = _format_key_v(out, out_size, format, args);
    va_end(args);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Format a retained visual id.
 *
 * @param visual_index the figure-local visual index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual(uint32_t visual_index, char* out, size_t out_size)
{
    return _format_key(out, out_size, "v%u", visual_index);
}



/**
 * Format a retained scene-buffer id.
 *
 * @param buffer_index the scene-local buffer index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_buffer(uint32_t buffer_index, char* out, size_t out_size)
{
    return _format_key(out, out_size, "b%u", buffer_index);
}



/**
 * Format a visual data resource id from an existing visual id and data tag.
 *
 * @param visual_id the encoded visual id without index-buffer suffix
 * @param data_tag the visual data tag
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual_data(
    const char* visual_id, const char* data_tag, char* out, size_t out_size)
{
    ANN(visual_id);
    ANN(data_tag);
    return _format_key(out, out_size, "%s_%s", visual_id, data_tag);
}



/**
 * Format a visual attribute resource id from a visual index and attribute name.
 *
 * @param visual_index the figure-local visual index
 * @param attr_name the visual attribute name
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual_attr(
    uint32_t visual_index, const char* attr_name, char* out, size_t out_size)
{
    ANN(attr_name);
    char visual_id[32] = {0};
    if (!_scene_resource_key_visual(visual_index, visual_id, sizeof(visual_id)))
        return false;
    return _scene_resource_key_visual_data(visual_id, attr_name, out, out_size);
}



/**
 * Format a visual texture resource id.
 *
 * @param visual_index the figure-local visual index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual_texture(uint32_t visual_index, char* out, size_t out_size)
{
    return _scene_resource_key_visual_attr(visual_index, "texture", out, out_size);
}



/**
 * Format an encoded visual id carrying a shared index-buffer id.
 *
 * @param visual_index the figure-local visual index
 * @param buffer_index the scene-local buffer index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual_indexed(
    uint32_t visual_index, uint32_t buffer_index, char* out, size_t out_size)
{
    return _format_key(out, out_size, "v%u#index=b%u", visual_index, buffer_index);
}



/**
 * Split an encoded visual id with an optional shared index-buffer suffix.
 *
 * @param encoded the encoded visual id
 * @param visual_id output visual id
 * @param visual_id_size output visual id capacity
 * @param index_id output shared index id
 * @param index_id_size output shared index id capacity
 */
void _scene_resource_key_split_visual(
    const char* encoded, char* visual_id, size_t visual_id_size, char* index_id,
    size_t index_id_size)
{
    ANN(visual_id);
    ANN(index_id);
    if (visual_id_size == 0 || index_id_size == 0)
        return;
    visual_id[0] = '\0';
    index_id[0] = '\0';
    if (encoded == NULL)
        return;

    const char* marker = strstr(encoded, "#index=");
    if (marker == NULL)
    {
        dvz_strlcpy(visual_id, encoded, visual_id_size);
        return;
    }

    size_t visual_len = (size_t)(marker - encoded);
    if (visual_len >= visual_id_size)
        visual_len = visual_id_size - 1;
    if (visual_len > 0)
        dvz_memcpy(visual_id, visual_id_size, encoded, visual_len);
    visual_id[visual_len] = '\0';
    dvz_strlcpy(index_id, marker + strlen("#index="), index_id_size);
}
