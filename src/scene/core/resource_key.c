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
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
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
 * @param buffer_id the immutable scene-buffer identity
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_buffer(DvzId buffer_id, char* out, size_t out_size)
{
    return _format_key(out, out_size, "b%" PRIu64, buffer_id);
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
 * Format a colorbar-derived visual id when the visual is owned by a retained colorbar.
 *
 * @param figure the figure being emitted
 * @param visual the visual to identify
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the visual is colorbar-derived and the key was written
 */
static bool _scene_colorbar_visual_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, char* out, size_t out_size)
{
    if (figure == NULL || figure->scene == NULL || visual == NULL || out == NULL || out_size == 0)
        return false;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->colorbar_count; i++)
    {
        const DvzColorbar* colorbar = &scene->colorbars[i];
        if (colorbar->scene != scene || colorbar->panel == NULL ||
            colorbar->panel->figure != figure)
        {
            continue;
        }
        if (visual == colorbar->ramp_visual)
            return _format_key(out, out_size, "colorbar.%u.ramp", i);
        if (visual == colorbar->tick_visual)
            return _format_key(out, out_size, "colorbar.%u.ticks", i);
        if (visual == colorbar->text_visual)
            return _format_key(out, out_size, "colorbar.%u.labels", i);
        if (colorbar->text_visual != NULL &&
            visual == _visual_family_state(colorbar->text_visual)->text.glyph_visual)
        {
            return _format_key(out, out_size, "colorbar.%u.labels.glyph", i);
        }
    }
    return false;
}


/**
 * Format a legend-derived visual id when the visual is owned by a retained legend.
 *
 * @param figure the figure being emitted
 * @param visual the visual to identify
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the visual is legend-derived and the key was written
 */
static bool _scene_legend_visual_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, char* out, size_t out_size)
{
    if (figure == NULL || figure->scene == NULL || visual == NULL || out == NULL || out_size == 0)
        return false;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->legend_count; i++)
    {
        const DvzLegend* legend = &scene->legends[i];
        if (legend->scene != scene || legend->panel == NULL || legend->panel->figure != figure)
            continue;
        if (visual == legend->mark_visual)
            return _format_key(out, out_size, "legend.%u.marks", i);
        if (visual == legend->text_visual)
            return _format_key(out, out_size, "legend.%u.labels", i);
        if (legend->text_visual != NULL && visual == _visual_family_state(legend->text_visual)->text.glyph_visual)
            return _format_key(out, out_size, "legend.%u.labels.glyph", i);
    }
    return false;
}



/**
 * Format a visual resource id, preserving semantic debug provenance when available.
 *
 * @param figure the figure being emitted
 * @param visual the visual to identify
 * @param visual_index the figure-local visual index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written
 */
bool _scene_visual_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size)
{
    if (_scene_colorbar_visual_resource_key(figure, visual, out, out_size))
        return true;
    if (_scene_legend_visual_resource_key(figure, visual, out, out_size))
        return true;
    return _scene_resource_key_visual(visual_index, out, out_size);
}



/**
 * Format a visual attribute resource id, preserving semantic debug provenance when available.
 *
 * @param figure the figure being emitted
 * @param visual the visual that owns the attribute
 * @param visual_index the figure-local visual index
 * @param attr_name the visual attribute name
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written
 */
bool _scene_visual_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    const char* attr_name, char* out, size_t out_size)
{
    ANN(attr_name);
    char visual_id[128] = {0};
    if (_scene_colorbar_visual_resource_key(figure, visual, visual_id, sizeof(visual_id)))
        return _format_key(out, out_size, "%s.%s", visual_id, attr_name);
    if (_scene_legend_visual_resource_key(figure, visual, visual_id, sizeof(visual_id)))
        return _format_key(out, out_size, "%s.%s", visual_id, attr_name);
    return _scene_resource_key_visual_attr(visual_index, attr_name, out, out_size);
}



/**
 * Format a visual texture resource id, preserving semantic debug provenance when available.
 *
 * @param figure the figure being emitted
 * @param visual the visual that owns the texture
 * @param visual_index the figure-local visual index
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written
 */
bool _scene_visual_texture_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size)
{
    return _scene_visual_attr_resource_key(figure, visual, visual_index, "texture", out, out_size);
}



/**
 * Format an encoded visual id carrying a shared index-buffer id.
 *
 * @param figure the figure being emitted
 * @param visual the visual to identify
 * @param visual_index the figure-local visual index
 * @param buffer_id the immutable scene-buffer identity
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written
 */
bool _scene_visual_indexed_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzId buffer_id, char* out, size_t out_size)
{
    char visual_id[128] = {0};
    if (_scene_colorbar_visual_resource_key(figure, visual, visual_id, sizeof(visual_id)))
    {
        return _format_key(out, out_size, "%s#index=b%" PRIu64, visual_id, buffer_id);
    }
    if (_scene_legend_visual_resource_key(figure, visual, visual_id, sizeof(visual_id)))
    {
        return _format_key(out, out_size, "%s#index=b%" PRIu64, visual_id, buffer_id);
    }
    return _scene_resource_key_visual_indexed(visual_index, buffer_id, out, out_size);
}



/**
 * Format an encoded visual id carrying a shared index-buffer id.
 *
 * @param visual_index the figure-local visual index
 * @param buffer_id the immutable scene-buffer identity
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_visual_indexed(
    uint32_t visual_index, DvzId buffer_id, char* out, size_t out_size)
{
    return _format_key(out, out_size, "v%u#index=b%" PRIu64, visual_index, buffer_id);
}



/**
 * Format a panel-scoped graph resource or pass id.
 *
 * @param panel_id the encoded panel id
 * @param suffix the graph resource/pass suffix without a leading dot
 * @param out the output key buffer
 * @param out_size the output buffer capacity
 * @return whether the key was written without truncation
 */
bool _scene_resource_key_panel_graph(
    const char* panel_id, const char* suffix, char* out, size_t out_size)
{
    ANN(panel_id);
    ANN(suffix);
    return _format_key(out, out_size, "%s.%s", panel_id, suffix);
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



/**
 * Return whether a graph resource id ends with an exact suffix.
 *
 * @param resource_id the graph resource id
 * @param suffix the expected suffix, including its leading separator
 * @return whether the resource id has the suffix
 */
bool _scene_resource_id_has_suffix(const char* resource_id, const char* suffix)
{
    ANN(suffix);
    if (resource_id == NULL)
        return false;
    size_t resource_len = strlen(resource_id);
    size_t suffix_len = strlen(suffix);
    if (suffix_len == 0 || suffix_len > resource_len)
        return false;
    return strcmp(resource_id + resource_len - suffix_len, suffix) == 0;
}



/**
 * Return whether a graph resource id uses the scene depth naming marker.
 *
 * @param resource_id the graph resource id
 * @return whether the resource id denotes a depth-family graph resource
 */
bool _scene_resource_id_has_depth_marker(const char* resource_id)
{
    return resource_id != NULL && strstr(resource_id, ".depth") != NULL;
}
