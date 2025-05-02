/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Baker                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/baker.h"
#include "datoviz_protocol.h"
#include "scene/array.h"
#include "scene/dual.h"



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _check_sizes(DvzBaker* baker)
{
    DvzSize sizes[DVZ_MAX_VERTEX_BINDINGS] = {0};

    // Compute the sum of the sizes of all attributes in each vertex binding.
    for (uint32_t attr_idx = 0; attr_idx < baker->attr_count; attr_idx++)
    {
        uint32_t binding_idx = baker->vertex_attrs[attr_idx].binding_idx;
        ASSERT(binding_idx < DVZ_MAX_VERTEX_BINDINGS);
        sizes[binding_idx] += baker->vertex_attrs[attr_idx].item_size;
    }

    // The vertex binding stride should be larger than, or equal, to that sum.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        ASSERT(baker->vertex_bindings[binding_idx].stride >= sizes[binding_idx]);
    }
}



static void _create_vertex_binding(DvzBaker* baker, uint32_t binding_idx, uint32_t vertex_count)
{
    ANN(baker);
    ASSERT(binding_idx < baker->binding_count);
    ASSERT(vertex_count > 0);

    DvzBakerVertex* bv = &baker->vertex_bindings[binding_idx];
    ANN(bv);

    log_trace(
        "create baker vertex binding #%d with %d vertices, stride %" PRId64
        ", create dat and array",
        binding_idx, vertex_count, bv->stride);

    if (bv->shared)
    {
        log_trace("skipping creation of shared dat for vertex binding #%d", binding_idx);
        return;
    }
    int dual_flags = ((baker->flags & DVZ_BAKER_FLAGS_VERTEX_MAPPABLE) == 0)
                         ? DVZ_DAT_FLAGS_PERSISTENT_STAGING
                         : DVZ_DAT_FLAGS_MAPPABLE;
    bv->dual = dvz_dual_vertex(baker->batch, vertex_count, bv->stride, dual_flags);
    // NOTE; mark the dual as needing to be destroyed by the library
    bv->dual.need_destroy = true;
}



static void _create_index(DvzBaker* baker, uint32_t index_count)
{
    ANN(baker);
    ASSERT(index_count > 0);

    log_trace("create index buffer with %d vertices, create dat and array", index_count);
    if (baker->index_shared)
    {
        log_trace("skipping creation of dat for shared index buffer");
        return;
    }
    int dual_flags = ((baker->flags & DVZ_BAKER_FLAGS_INDEX_MAPPABLE) == 0)
                         ? DVZ_DAT_FLAGS_PERSISTENT_STAGING
                         : DVZ_DAT_FLAGS_MAPPABLE;
    baker->index = dvz_dual_index(baker->batch, index_count, dual_flags);
    // NOTE; mark the dual as needing to be destroyed by the library
    baker->index.need_destroy = true;
}



static void _create_indirect(DvzBaker* baker, bool indexed)
{
    ANN(baker);

    log_trace("create %sindirect buffer, create dat and array", indexed ? "indexed " : "");
    baker->indirect = dvz_dual_indirect(baker->batch, indexed);
    // NOTE; mark the dual as needing to be destroyed by the library
    baker->indirect.need_destroy = true;
}



/*************************************************************************************************/
/*  Baker life cycle                                                                             */
/*************************************************************************************************/

DvzBaker* dvz_baker(DvzBatch* batch, int flags)
{
    ANN(batch);
    DvzBaker* baker = (DvzBaker*)calloc(1, sizeof(DvzBaker));
    baker->batch = batch;

    // 00xx: which attributes should be in a different buf (8 max)
    // xx00: which attributes should be constants
    baker->flags = flags;

    return baker;
}



void dvz_baker_create(DvzBaker* baker, uint32_t index_count, uint32_t vertex_count)
{
    ANN(baker);
    log_trace(
        "create the dat, arrays, %d bindings, %d descriptors", //
        baker->binding_count, baker->slot_count);

    // Check size consistency.
    _check_sizes(baker);

    // Create the vertex bindings.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        _create_vertex_binding(baker, binding_idx, vertex_count);
    }

    // Create the uniform dats for the dat descriptors.
    // for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    // {
    //     // NOTE: we don't do anything for textures for now, we let the visual handle them
    //     directly. _create_descriptor(baker, slot_idx);
    // }

    // Create the index buffer.
    if (index_count > 0)
    {
        _create_index(baker, index_count);
    }

    // Create the indirect buffer.
    if (baker->is_indirect)
    {
        _create_indirect(baker, index_count > 0);
    }
}



// emit the dat update commands to synchronize the dual arrays on the GPU
void dvz_baker_update(DvzBaker* baker)
{
    ANN(baker);

    // Update the vertex bindings duals.
    DvzBakerVertex* bv = NULL;
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        bv = &baker->vertex_bindings[binding_idx];
        if (!bv->shared)
            dvz_dual_update(&bv->dual);
    }

    // Update the index dual.
    if (baker->index.array != NULL)
        dvz_dual_update(&baker->index);

    // // Update the descriptor duals.
    // DvzBakerDescriptor* bd = NULL;
    // for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    // {
    //     bd = &baker->descriptors[slot_idx];
    //     if (!bd->shared)
    //         dvz_dual_update(&bd->u.dat.dual);
    // }
}



void dvz_baker_destroy(DvzBaker* baker)
{
    ANN(baker);

    // Destroy all duals.
    DvzBakerVertex* bv = NULL;
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        bv = &baker->vertex_bindings[binding_idx];
        // NOTE: this will destroy the dual's array only if dual.need_destroy=true (which only
        // occurs if the dual was created with one of the helper functions in dual.c).
        dvz_dual_destroy(&bv->dual);
        // NOTE: the dat is not destroyed at the moment.
    }

    // DvzBakerDescriptor* bd = NULL;
    // for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    // {
    //     bd = &baker->descriptors[slot_idx];
    //     // NOTE: same as above.
    //     if (bd->type == DVZ_SLOT_DAT)
    //         dvz_dual_destroy(&bd->u.dat.dual);

    //     // TODO: tex destruction
    //     // else if (bd->type == DVZ_SLOT_TEX)
    //     //     dvz_destroy_tex(&bd->u.tex.tex);
    // }

    FREE(baker);
}



/*************************************************************************************************/
/*  Baker specification                                                                          */
/*************************************************************************************************/

// declare a vertex binding
void dvz_baker_vertex(DvzBaker* baker, uint32_t binding_idx, DvzSize stride)
{
    ANN(baker);
    ASSERT(binding_idx < DVZ_MAX_VERTEX_BINDINGS);
    ASSERT(stride > 0);

    baker->vertex_bindings[binding_idx].binding_idx = binding_idx;
    baker->vertex_bindings[binding_idx].stride = stride;
    baker->binding_count = MAX(baker->binding_count, binding_idx + 1);

    log_trace("declare vertex binding #%d with stride %d", binding_idx, stride);
}



// declare a GLSL attribute
void dvz_baker_attr(
    DvzBaker* baker, uint32_t attr_idx, uint32_t binding_idx, DvzSize offset, DvzSize item_size)
{
    ANN(baker);
    ASSERT(attr_idx < DVZ_MAX_VERTEX_ATTRS);

    baker->vertex_attrs[attr_idx].binding_idx = binding_idx;
    baker->vertex_attrs[attr_idx].offset = offset;
    baker->vertex_attrs[attr_idx].item_size = item_size;
    baker->attr_count = MAX(baker->attr_count, attr_idx + 1);

    log_trace(
        "declare vertex attr #%d (binding #%d) with offset %d and size %d", //
        attr_idx, binding_idx, offset, item_size);
}



void dvz_baker_indirect(DvzBaker* baker)
{
    ANN(baker);
    baker->is_indirect = true;
}



/*************************************************************************************************/
/*  Baker sharing                                                                                */
/*************************************************************************************************/

void dvz_baker_share_vertex(DvzBaker* baker, uint32_t binding_idx)
{
    ANN(baker);
    // ANN(dual);
    ASSERT(binding_idx < baker->binding_count);

    DvzBakerVertex* bv = &baker->vertex_bindings[binding_idx];
    ANN(bv);

    log_trace("set shared dual for vertex binding #%d", binding_idx);
    // bv->dual = *dual;
    bv->shared = true;
}



void dvz_baker_share_index(DvzBaker* baker)
{
    ANN(baker);
    // ANN(dual);

    log_trace("set shared dual for index buffer");
    // baker->index = *dual;
    baker->index_shared = true;
}



/*************************************************************************************************/
/*  Baker data                                                                                   */
/*************************************************************************************************/

void dvz_baker_data(DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, void* data)
{
    dvz_baker_repeat(baker, attr_idx, first, count, 1, data);
}



void dvz_baker_resize(DvzBaker* baker, uint32_t vertex_count, uint32_t index_count)
{
    ANN(baker);
    log_trace("resize the baker to %d vertices and %d indices", vertex_count, index_count);

    // Resize the vertex bindings.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        // Resize the underlying dual array.
        dvz_array_resize(baker->vertex_bindings[binding_idx].dual.array, vertex_count);

        // Emit the dual's dat resize commands.
        dvz_dual_resize(&baker->vertex_bindings[binding_idx].dual, vertex_count);
    }

    // Resizing the index buffer.

    // Resize the underlying dual array.
    dvz_array_resize(baker->index.array, index_count);

    // Emit the dual's dat resize commands.
    dvz_dual_resize(&baker->index, index_count);
}



void dvz_baker_repeat(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, uint32_t repeats,
    void* data)
{
    ANN(baker);
    if (baker->attr_count == 0)
    {
        log_error("unitialized baker (attribute #%d), have you allocated the visual?", attr_idx);
        return;
    }
    ASSERT(attr_idx < baker->attr_count);
    ASSERT(count > 0);
    ANN(data);

    DvzBakerAttr* attr = &baker->vertex_attrs[attr_idx];
    uint32_t binding_idx = attr->binding_idx;
    ASSERT(binding_idx < baker->binding_count);

    DvzBakerVertex* vertex = &baker->vertex_bindings[binding_idx];

    DvzDual* dual = &vertex->dual;
    if (dual == NULL)
    {
        log_error("dual is null, please call dvz_baker_create()");
        return;
    }
    ANN(dual);
    if (dual->array == NULL)
    {
        log_error("dual's array is null");
        return;
    }

    if (first + count * repeats > dual->array->item_count)
    {
        log_error(
            "baker vertex array is too small (%d) to hold the vertices (%d)",
            dual->array->item_count, first + count * repeats);
        return;
    }

    DvzSize offset = attr->offset;
    DvzSize item_size = attr->item_size;
    ASSERT(item_size > 0);

    DvzSize col_size = vertex->stride;
    ASSERT(col_size > 0);

    // log_info("%d %d %d %d %d", offset, item_size, first, count, repeats);
    dvz_dual_column(dual, offset, item_size, first, count, repeats, data);
}



void dvz_baker_index(DvzBaker* baker, uint32_t first, uint32_t count, DvzIndex* data)
{
    ANN(baker);
    ASSERT(count > 0);
    ANN(data);

    DvzDual* dual = &baker->index;
    if (dual == NULL)
    {
        log_error("dual is null, please set up an index buffer");
        return;
    }
    ANN(dual);

    if (dual->array == NULL)
    {
        log_error("index dual's array is null");
        return;
    }
    ANN(dual->array);

    if (first + count > dual->array->item_count)
    {
        log_error(
            "baker index array is too small (%d) to hold the indices (%d)",
            dual->array->item_count, first + count);
        return;
    }

    dvz_dual_data(dual, first, count, (void*)data);
}



void dvz_baker_quads(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, vec4* tl_br)
{
    ANN(baker);
    ANN(tl_br);
    ASSERT(count > 0);

    // Quad triangulation with 3 triangles = 6 vertices.
    vec2* quads = (vec2*)calloc(6 * count, sizeof(vec2));
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        x0 = tl_br[i][0];
        y0 = tl_br[i][1];
        x1 = tl_br[i][2];
        y1 = tl_br[i][3];

        quads[6 * i + 0][0] = x0; // top left
        quads[6 * i + 0][1] = y0;
        quads[6 * i + 1][0] = x0; // bottom left
        quads[6 * i + 1][1] = y1;
        quads[6 * i + 2][0] = x1; // bottom right
        quads[6 * i + 2][1] = y1;
        quads[6 * i + 3][0] = x1; // bottom right
        quads[6 * i + 3][1] = y1;
        quads[6 * i + 4][0] = x1; // top right
        quads[6 * i + 4][1] = y0;
        quads[6 * i + 5][0] = x0; // top left
        quads[6 * i + 5][1] = y0;
    }
    dvz_baker_repeat(baker, attr_idx, 6 * first, 6 * count, 1, quads);
    FREE(quads);
}



void dvz_baker_unindex(DvzBaker* baker)
{
    // NOTE: OBSOLETE FUNCTION TO DELETE?
    // This function is still mostly untested and probably doesn't work right now.
    // It might not be very useful now that there is dvz_shape_unindex().
    // Can probably be deleted.

    ANN(baker);

    // Index dual.
    DvzDual* index = &baker->index;
    ANN(index);
    ANN(index->array);

    // Index array.
    DvzIndex* indices = (DvzIndex*)index->array->data;
    ANN(indices);

    // Number of indices.
    uint32_t index_count = index->array->item_count;
    ASSERT(index_count > 0);

    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        // Vertex binding.
        DvzBakerVertex* baker_vertex = &baker->vertex_bindings[binding_idx];
        ANN(baker_vertex);

        // Vertex dual.
        DvzDual* vertex = &baker_vertex->dual;
        ANN(vertex->array);

        // Vertex array.
        void* vertices_orig = vertex->array->data;
        ANN(vertices_orig);

        // Number of vertices.
        uint32_t vertex_count = vertex->array->item_count;
        ASSERT(vertex_count > 0);

        // Item size.
        DvzSize vertex_size = vertex->array->item_size;
        ASSERT(vertex_size > 0);

        // Copy the indexed vertices;
        void* vertices = calloc(index_count, vertex_size);
        DvzIndex vertex_idx = 0;
        for (uint32_t i = 0; i < index_count; i++)
        {
            vertex_idx = indices[i];
            ASSERT(vertex_idx < vertex_count);
            memcpy(
                (void*)((uint64_t)vertices + vertex_size * i),               //
                (void*)((uint64_t)vertices_orig + vertex_size * vertex_idx), //
                vertex_size);
        }
        dvz_dual_data(vertex, 0, index_count, vertices);
        FREE(vertices);
    }

    // Disable index buffer.
    index->array->item_count = 0;
}
