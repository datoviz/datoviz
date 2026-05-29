/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual query geometry helpers                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "query_geometry.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one source vertex index from direct or indexed visual geometry.
 *
 * @param visual visual carrying an optional index buffer binding
 * @param draw_index draw-order vertex index
 * @param vertex_count source vertex count
 * @param out_source_index output source vertex index
 * @return true when the source index is valid
 */
static bool _query_geometry_source_vertex_index(
    const DvzVisual* visual, uint64_t draw_index, uint64_t vertex_count,
    uint64_t* out_source_index)
{
    ANN(visual);
    ANN(out_source_index);
    uint64_t source_index = draw_index;
    if (visual->buffer != NULL && visual->buffer->data != NULL)
    {
        uint32_t stride = visual->buffer->desc.stride;
        uint64_t offset = 0;
        if (
            stride == 0 ||
            _dvz_mul_u64_overflows(draw_index, stride, &offset) ||
            offset > visual->buffer->desc.byte_size ||
            stride > visual->buffer->desc.byte_size - offset)
        {
            return false;
        }
        const uint8_t* data = (const uint8_t*)visual->buffer->data + offset;
        if (stride == sizeof(uint16_t))
        {
            uint16_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else if (stride == sizeof(uint32_t))
        {
            uint32_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else
            return false;
    }
    if (source_index >= vertex_count)
        return false;
    *out_source_index = source_index;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Build temporary query buffers for a primitive-topology visual.
 *
 * @param label diagnostic query family label
 * @param visual retained visual with dense position and optional index buffer
 * @param scratch output scratch storage
 * @param out_vertex_count output derived vertex count
 * @param out_topology output Vulkan draw topology
 * @return true when derived query buffers were created
 */
bool _scene_query_indexed_primitive_geometry(
    const char* label, const DvzVisual* visual, DvzSceneQueryScratch* scratch,
    uint64_t* out_vertex_count, uint32_t* out_topology)
{
    ANN(label);
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_topology);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_dvz_scene_query_dense_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    uint64_t vertex_count = pos_attr->item_count;

    uint64_t source_index_count = vertex_count;
    if (visual->buffer != NULL && visual->buffer->data != NULL &&
        visual->buffer->desc.byte_size > 0 && visual->buffer->desc.stride > 0)
    {
        uint32_t stride = visual->buffer->desc.stride;
        if (stride != sizeof(uint16_t) && stride != sizeof(uint32_t))
        {
            log_error("%s query request index stride must be 16-bit or 32-bit", label);
            return false;
        }
        if (visual->buffer->desc.byte_size % stride != 0)
        {
            log_error("%s query request index buffer size is not stride-aligned", label);
            return false;
        }
        source_index_count = visual->buffer->desc.byte_size / stride;
    }

    uint64_t primitive_count = 0;
    uint64_t draw_vertex_count = 0;
    uint32_t draw_topology = (uint32_t)visual->topology;
    switch (visual->topology)
    {
    case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
        primitive_count = source_index_count;
        draw_vertex_count = primitive_count;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
        primitive_count = source_index_count / 2;
        if (_dvz_mul_u64_overflows(primitive_count, 2, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        if (source_index_count < 2)
            return false;
        primitive_count = source_index_count - 1;
        if (_dvz_mul_u64_overflows(primitive_count, 2, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        primitive_count = source_index_count / 3;
        if (_dvz_mul_u64_overflows(primitive_count, 3, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        if (source_index_count < 3)
            return false;
        primitive_count = source_index_count - 2;
        if (_dvz_mul_u64_overflows(primitive_count, 3, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    default:
        return false;
    }
    if (primitive_count == 0 || draw_vertex_count == 0 || draw_vertex_count > UINT32_MAX)
        return false;

    if (!_dvz_scene_query_alloc(
            label, (void**)&scratch->query_positions, draw_vertex_count, sizeof(vec3)) ||
        !_dvz_scene_query_alloc(
            label, (void**)&scratch->query_ids, draw_vertex_count, sizeof(uint32_t)))
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    for (uint64_t prim = 0; prim < primitive_count; prim++)
    {
        uint64_t draw_indices[3] = {0, 0, 0};
        uint32_t prim_vertex_count = 1;
        switch (visual->topology)
        {
        case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
            draw_indices[0] = prim;
            prim_vertex_count = 1;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
            draw_indices[0] = 2 * prim + 0;
            draw_indices[1] = 2 * prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            draw_indices[0] = 3 * prim + 0;
            draw_indices[1] = 3 * prim + 1;
            draw_indices[2] = 3 * prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            draw_indices[0] = 0;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        default:
            _scene_query_scratch_destroy(scratch);
            return false;
        }

        for (uint32_t j = 0; j < prim_vertex_count; j++)
        {
            uint64_t source_index = 0;
            if (!_query_geometry_source_vertex_index(
                    visual, draw_indices[j], vertex_count, &source_index))
            {
                _scene_query_scratch_destroy(scratch);
                return false;
            }
            uint64_t dst = prim * prim_vertex_count + j;
            dvz_memcpy(
                scratch->query_positions[dst], sizeof(vec3), &position[3 * source_index],
                sizeof(vec3));
            scratch->query_ids[dst] = (uint32_t)prim + 1u;
        }
    }

    *out_vertex_count = draw_vertex_count;
    *out_topology = draw_topology;
    return true;
}
