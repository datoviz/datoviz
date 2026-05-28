/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image query quad helpers                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "image/internal.h"
#include "../../query/internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Allocate one temporary image-like query buffer with checked size arithmetic.
 *
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
static bool _image_query_alloc(void** out_ptr, uint64_t count, uint64_t item_size)
{
    ANN(out_ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
    {
        log_error("image query request buffer size overflow");
        return false;
    }
    void* ptr = dvz_calloc((size_t)count, (size_t)item_size);
    if (ptr == NULL && bytes > 0)
    {
        log_error("image query request buffer allocation failed");
        return false;
    }
    *out_ptr = ptr;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one retained attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
bool _image_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out_attr);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[attr_idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    *out_attr = attr;
    return true;
}



/**
 * Build temporary generated rectangle query geometry for image-like visuals.
 *
 * @param visual image-like visual
 * @param scratch output scratch plan storage
 * @param include_ids whether to fill query item IDs
 * @param include_texcoords whether to fill generated texture coordinates
 * @param out_vertex_count output derived vertex count
 * @return true when derived buffers were created
 */
bool _image_query_generated_rect_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, bool include_ids,
    bool include_texcoords, uint64_t* out_vertex_count)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);

    const DvzVisualAttr* pos_attr = NULL;
    const DvzVisualAttr* extent_attr = NULL;
    if (!_image_query_attr(visual, "position", sizeof(vec3), &pos_attr) ||
        !_image_query_attr(visual, "extent", sizeof(vec2), &extent_attr))
    {
        return false;
    }
    if (extent_attr->item_count != pos_attr->item_count)
        return false;

    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows(pos_attr->item_count, 6, &vertex_count) ||
        vertex_count > UINT32_MAX)
    {
        log_error("image query request buffer size overflow");
        return false;
    }

    if (!_image_query_alloc((void**)&scratch->query_positions, vertex_count, sizeof(vec3)) ||
        (include_ids &&
         !_image_query_alloc((void**)&scratch->query_ids, vertex_count, sizeof(uint32_t))) ||
        (include_texcoords &&
         !_image_query_alloc((void**)&scratch->query_texcoords, vertex_count, sizeof(vec2))))
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    const float* extent = (const float*)extent_attr->data;
    const DvzVisualAttr* anchor_attr = NULL;
    bool has_anchor = _image_query_attr(visual, "anchor", sizeof(vec2), &anchor_attr);
    if (has_anchor && anchor_attr->item_count != pos_attr->item_count)
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }
    const float* anchor = has_anchor ? (const float*)anchor_attr->data : NULL;

    const DvzVisualAttr* tex_rect_attr = NULL;
    bool has_tex_rect = _image_query_attr(visual, "tex_rect", 4 * sizeof(float), &tex_rect_attr);
    if (has_tex_rect && tex_rect_attr->item_count != pos_attr->item_count)
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }
    const float* tex_rect = has_tex_rect ? (const float*)tex_rect_attr->data : NULL;

    for (uint64_t i = 0; i < pos_attr->item_count; i++)
    {
        float x = position[3 * i + 0];
        float y = position[3 * i + 1];
        float z = position[3 * i + 2];
        float w = extent[2 * i + 0];
        float h = extent[2 * i + 1];
        float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
        float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;
        float x0 = x - 0.5f * (ax + 1.0f) * w;
        float x1 = x0 + w;
        float y0 = y - 0.5f * (ay + 1.0f) * h;
        float y1 = y0 + h;
        float u0 = tex_rect != NULL ? tex_rect[4 * i + 0] : 0.0f;
        float v0 = tex_rect != NULL ? tex_rect[4 * i + 1] : 0.0f;
        float u1 = tex_rect != NULL ? tex_rect[4 * i + 2] : 1.0f;
        float v1 = tex_rect != NULL ? tex_rect[4 * i + 3] : 1.0f;
        const float quad_pos[6][3] = {
            {x0, y0, z}, {x0, y1, z}, {x1, y0, z},
            {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
        };
        const float quad_uv[6][2] = {
            {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
        };
        for (uint32_t j = 0; j < 6; j++)
        {
            uint64_t dst = 6 * i + j;
            dvz_memcpy(scratch->query_positions[dst], sizeof(vec3), quad_pos[j], sizeof(vec3));
            if (include_ids)
                scratch->query_ids[dst] = (uint32_t)i + 1u;
            if (include_texcoords)
                dvz_memcpy(
                    scratch->query_texcoords[dst], sizeof(vec2), quad_uv[j], sizeof(vec2));
        }
    }

    *out_vertex_count = vertex_count;
    return true;
}
