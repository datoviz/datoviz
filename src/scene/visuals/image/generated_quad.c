/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image generated-quad cache builders                                                          */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "image/internal.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one visual has CPU-side data for an attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return whether the attribute exists and has data
 */
static bool _image_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}



/**
 * Resize an image generated-quad cache array.
 *
 * @param ptr input/output array pointer
 * @param count item count
 * @param item_size byte size of one item
 * @return whether the allocation succeeded
 */
static bool _image_generated_quad_cache_resize(void** ptr, uint64_t count, uint64_t item_size)
{
    ANN(ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
        return false;
    void* grown = dvz_realloc(*ptr, (size_t)bytes);
    if (grown == NULL && bytes > 0)
        return false;
    *ptr = grown;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether an image visual uses per-item rectangles.
 *
 * @param visual the image visual
 * @return whether the visual has visual-space or pixel-space rectangle attributes
 */
bool _image_uses_generated_quads(const DvzVisual* visual)
{
    ANN(visual);
    bool image_like =
        visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_LABELS;
    return image_like && (_image_has_attr_data(visual, "extent") ||
                          _image_has_attr_data(visual, "extent_px"));
}



/**
 * Rebuild one image visual's derived six-vertex rectangle upload cache.
 *
 * @param figure parent figure
 * @param visual the image visual
 * @return whether the cache is ready for upload
 */
bool _image_generated_quad_cache_rebuild(const DvzFigure* figure, DvzVisual* visual)
{
    ANN(figure);
    ANN(visual);
    if (!_image_uses_generated_quads(visual))
    {
        log_error("image-like visual per-item rectangles require extent attributes");
        return false;
    }

    bool has_visual_rect =
        _image_has_attr_data(visual, "position") && _image_has_attr_data(visual, "extent");
    bool has_pixel_rect = visual->type == DVZ_VISUAL_TYPE_IMAGE &&
                          _image_has_attr_data(visual, "position_px") &&
                          _image_has_attr_data(visual, "extent_px");
    if (has_visual_rect == has_pixel_rect)
    {
        log_error(
            "image visual per-item rectangles require exactly one of position/extent or "
            "position_px/extent_px");
        return false;
    }

    const char* position_name = has_pixel_rect ? "position_px" : "position";
    const char* extent_name = has_pixel_rect ? "extent_px" : "extent";
    DvzVisualAttr* position_attr = &visual->attrs[_attr_index(visual, position_name)];
    DvzVisualAttr* extent_attr = &visual->attrs[_attr_index(visual, extent_name)];
    const uint64_t item_count = position_attr->item_count;
    if (item_count == 0 || extent_attr->item_count != item_count)
    {
        log_error("image visual rectangle position and extent item counts must match");
        return false;
    }

    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 6, &vertex_count) || vertex_count > UINT32_MAX)
    {
        log_error("image visual item count is too large");
        return false;
    }

    DvzImageGpuCache* cache = &_visual_family_state(visual)->image_gpu;
    if (!_image_generated_quad_cache_resize(
            (void**)&cache->position, vertex_count, 3 * sizeof(float)) ||
        !_image_generated_quad_cache_resize(
            (void**)&cache->texcoords, vertex_count, 2 * sizeof(float)))
    {
        log_error("failed to allocate image visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)position_attr->data;
    const float* extent = (const float*)extent_attr->data;
    const int anchor_idx = _attr_index(visual, "anchor");
    const int tex_rect_idx = _attr_index(visual, "tex_rect");
    const float* anchor = anchor_idx >= 0 ? (const float*)visual->attrs[anchor_idx].data : NULL;
    const float* tex_rect =
        tex_rect_idx >= 0 ? (const float*)visual->attrs[tex_rect_idx].data : NULL;
    float pixel_scale_x = 1.0f;
    float pixel_scale_y = 1.0f;
    if (has_pixel_rect)
    {
        pixel_scale_x =
            figure->device_scale_x > 0.0f ? figure->device_scale_x * figure->render_scale : 1.0f;
        pixel_scale_y =
            figure->device_scale_y > 0.0f ? figure->device_scale_y * figure->render_scale : 1.0f;
        if (pixel_scale_x <= 0.0f || !isfinite(pixel_scale_x))
            pixel_scale_x = 1.0f;
        if (pixel_scale_y <= 0.0f || !isfinite(pixel_scale_y))
            pixel_scale_y = 1.0f;
    }

    for (uint64_t i = 0; i < item_count; i++)
    {
        const float x = position[3 * i + 0] * pixel_scale_x;
        const float y = position[3 * i + 1] * pixel_scale_y;
        const float z = position[3 * i + 2];
        const float w = extent[2 * i + 0] * pixel_scale_x;
        const float h = extent[2 * i + 1] * pixel_scale_y;
        const float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
        const float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;

        const float x0 = x - 0.5f * (ax + 1.0f) * w;
        const float x1 = x0 + w;
        const float y0 = y - 0.5f * (ay + 1.0f) * h;
        const float y1 = y0 + h;

        const float u0 = tex_rect != NULL ? tex_rect[4 * i + 0] : 0.0f;
        const float v0 = tex_rect != NULL ? tex_rect[4 * i + 1] : 0.0f;
        const float u1 = tex_rect != NULL ? tex_rect[4 * i + 2] : 1.0f;
        const float v1 = tex_rect != NULL ? tex_rect[4 * i + 3] : 1.0f;

        const float quad_pos[6][3] = {
            {x0, y0, z}, {x0, y1, z}, {x1, y0, z}, {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
        };
        /* Generated image quads use top-origin UV bounds, matching RGBA row upload order. */
        const float quad_uv[6][2] = {
            {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
        };

        for (uint32_t j = 0; j < 6; j++)
        {
            uint64_t dst = 6 * i + j;
            dvz_memcpy(
                &cache->position[3 * dst], 3 * sizeof(float), quad_pos[j], 3 * sizeof(float));
            dvz_memcpy(
                &cache->texcoords[2 * dst], 2 * sizeof(float), quad_uv[j], 2 * sizeof(float));
        }
    }

    cache->item_count = item_count;
    cache->vertex_count = vertex_count;
    cache->pixel_space = has_pixel_rect;
    cache->dirty = false;
    return true;
}



/**
 * Fill generated image-quad upload payload descriptors.
 *
 * @param figure parent figure
 * @param visual the image-like visual
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether payload descriptors were written
 */
bool _image_generated_quad_upload_payloads(
    const DvzFigure* figure, DvzVisual* visual, DvzVisualUploadPayload* out_payloads,
    uint32_t* out_count)
{
    ANN(figure);
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    (void)figure;
    *out_count = 0;
    DvzImageGpuCache* cache = &_visual_family_state(visual)->image_gpu;

    out_payloads[0] = (DvzVisualUploadPayload){
        .name = "position",
        .data = cache->position,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[1] = (DvzVisualUploadPayload){
        .name = "texcoords",
        .data = cache->texcoords,
        .item_size = 2 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    *out_count = 2;
    return true;
}



/**
 * Resolve dirty generated-quad upload payloads for one image-like visual.
 *
 * @param figure parent figure
 * @param visual the image-like visual
 * @param attrs_dirty whether retained visual attributes have a pending dirty range
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @param out_handled whether generated quads own dense attribute upload handling
 * @return whether the payload decision succeeded
 */
bool _image_generated_quad_derived_upload_payloads(
    const DvzFigure* figure, DvzVisual* visual, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count, bool* out_handled)
{
    ANN(figure);
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    ANN(out_handled);
    *out_count = 0;
    *out_handled = _image_uses_generated_quads(visual);
    if (!*out_handled)
        return true;

    bool dirty = _visual_family_state(visual)->image_gpu.dirty || attrs_dirty;
    if (!dirty)
        return true;

    return _image_generated_quad_cache_rebuild(figure, visual) &&
           _image_generated_quad_upload_payloads(figure, visual, out_payloads, out_count);
}
