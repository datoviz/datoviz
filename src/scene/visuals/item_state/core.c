/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "_visual_pipeline_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "registry/registry.h"
#include <math.h>
#include <string.h>

/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define ITEM_STATE_BITS 0x3fu
#define ITEM_STYLE_BITS ((uint32_t)(DVZ_ITEM_STATE_VISUAL_ALPHA | DVZ_ITEM_STATE_VISUAL_TINT))

/** Validate a color/alpha style.
 * @param style the style
 * @return whether the descriptor and values are valid
 */
static bool _style_valid(const DvzItemStateVisualStyle* style)
{
    return DVZ_STRUCT_VALID(style, DvzItemStateVisualStyle, 0) &&
           (style->visual_flags & ~ITEM_STYLE_BITS) == 0 && isfinite(style->alpha) &&
           style->alpha >= 0 && style->alpha <= 1 && isfinite(style->tint_mix) &&
           style->tint_mix >= 0 && style->tint_mix <= 1;
}



/** Pack one color/alpha style into the existing shader uniform ABI.
 * @param params packed style
 * @param tint packed tint
 * @param flags enabled effects
 * @param alpha alpha multiplier
 * @param color tint color
 * @param mix tint mix
 */
static void
_style_pack(float params[4], float tint[4], uint32_t flags, float alpha, DvzColor color, float mix)
{
    params[0] = (float)flags;
    params[1] = alpha;
    params[2] = mix;
    params[3] = 1;
    tint[0] = color.r / 255.0f;
    tint[1] = color.g / 255.0f;
    tint[2] = color.b / 255.0f;
    tint[3] = color.a / 255.0f;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/** Return a default fixed-cardinality state descriptor.
 * @return the descriptor
 */
DvzItemStateBufferDesc dvz_item_state_buffer_desc(void)
{
    return (DvzItemStateBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzItemStateBufferDesc)};
}



/** Set shared styles without changing query inputs.
 * @param buffer the bank
 * @param selection selection styles, or NULL for defaults
 * @param hovered hover style, or NULL for defaults
 * @param selection_active explicit contextual selection activity
 * @return DVZ_OK on success
 */
DvzResult dvz_item_state_buffer_style(
    DvzItemStateBuffer* buffer, const DvzSelectionVisualStyle* selection,
    const DvzItemStateVisualStyle* hovered, bool selection_active)
{
    if (buffer == NULL || buffer->scene == NULL ||
        !_scene_visual_mutation_allowed(buffer->scene, "update indexed state style"))
        return DVZ_ERROR;
    DvzSelectionVisualStyle sel = selection != NULL ? *selection : dvz_selection_visual_style();
    DvzItemStateVisualStyle hover = hovered != NULL ? *hovered : dvz_item_state_visual_style();
    if (!DVZ_STRUCT_VALID(&sel, DvzSelectionVisualStyle, 0) || !_style_valid(&hover) ||
        ((sel.selected_visual_flags | sel.unselected_visual_flags) & ~ITEM_STYLE_BITS) != 0 ||
        !isfinite(sel.selected_alpha) || sel.selected_alpha < 0 || sel.selected_alpha > 1 ||
        !isfinite(sel.unselected_alpha) || sel.unselected_alpha < 0 || sel.unselected_alpha > 1 ||
        !isfinite(sel.selected_tint_mix) || sel.selected_tint_mix < 0 ||
        sel.selected_tint_mix > 1 || !isfinite(sel.unselected_tint_mix) ||
        sel.unselected_tint_mix < 0 || sel.unselected_tint_mix > 1)
    {
        log_error("indexed state supports valid color/alpha styles only");
        return DVZ_ERROR;
    }
    DvzSceneItemStateStyleParams params = {0};
    _style_pack(
        params.selected, params.selected_tint, sel.selected_visual_flags, sel.selected_alpha,
        sel.selected_tint, sel.selected_tint_mix);
    _style_pack(
        params.unselected, params.unselected_tint,
        selection_active ? sel.unselected_visual_flags : 0, sel.unselected_alpha,
        sel.unselected_tint, sel.unselected_tint_mix);
    _style_pack(
        params.hovered, params.hovered_tint, hover.visual_flags, hover.alpha, hover.tint,
        hover.tint_mix);
    DvzSceneBuffer* style = buffer->style;
    if (memcmp(style->data, &params, sizeof(params)) == 0)
        return DVZ_OK;
    if (style->content_revision == UINT64_MAX)
        return DVZ_ERROR;
    dvz_memcpy(style->data, style->capacity, &params, sizeof(params));
    style->content_revision++;
    style->dirty = true;
    _scene_notify_buffer_changed(style);
    return DVZ_OK;
}



/** Create a scene-owned fixed-size copied word bank.
 * @param scene the owner
 * @param desc the descriptor
 * @return the bank, or NULL on error
 */
DvzItemStateBuffer* dvz_item_state_buffer(DvzScene* scene, const DvzItemStateBufferDesc* desc)
{
    uint64_t bytes = 0;
    uint64_t revision_bytes = 0;
    if (scene == NULL || desc == NULL || !DVZ_STRUCT_VALID(desc, DvzItemStateBufferDesc, 0) ||
        desc->slot_count == 0 ||
        _dvz_mul_u64_overflows(desc->slot_count, sizeof(uint32_t), &bytes) || bytes > SIZE_MAX ||
        _dvz_mul_u64_overflows(desc->slot_count, sizeof(uint64_t), &revision_bytes) ||
        revision_bytes > SIZE_MAX ||
        (scene->caps.max_buffer_size > 0 && bytes > scene->caps.max_buffer_size) ||
        !_scene_visual_mutation_allowed(scene, "create indexed state bank"))
        return NULL;
    for (uint32_t i = 0; desc->words != NULL && i < desc->slot_count; i++)
        if ((desc->words[i] & ~ITEM_STATE_BITS) != 0)
            return NULL;
    DvzItemStateBuffer* bank = NULL;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
        if (scene->item_state_banks[i].scene == NULL)
        {
            bank = &scene->item_state_banks[i];
            break;
        }
    if (bank == NULL)
        return NULL;
    uint64_t* revisions = dvz_calloc(desc->slot_count, sizeof(uint64_t));
    void* words = dvz_calloc(desc->slot_count, sizeof(uint32_t));
    if (words == NULL || revisions == NULL)
    {
        dvz_free(words);
        dvz_free(revisions);
        return NULL;
    }
    if (desc->words != NULL)
        dvz_memcpy(words, bytes, desc->words, bytes);
    DvzSceneBufferDesc resource = dvz_scene_buffer_desc();
    resource.usage = DVZ_SCENE_BUFFER_USAGE_STORAGE;
    resource.stride = sizeof(uint32_t);
    DvzSceneBuffer* word_buffer = dvz_scene_buffer(scene, &resource);
    resource.usage = DVZ_SCENE_BUFFER_USAGE_UNIFORM;
    resource.stride = sizeof(DvzSceneItemStateStyleParams);
    DvzSceneBuffer* style_buffer = dvz_scene_buffer(scene, &resource);
    if (word_buffer == NULL || style_buffer == NULL)
    {
        dvz_scene_buffer_destroy(word_buffer);
        dvz_scene_buffer_destroy(style_buffer);
        dvz_free(words);
        dvz_free(revisions);
        return NULL;
    }
    word_buffer->data = words;
    word_buffer->capacity = bytes;
    word_buffer->desc.byte_size = bytes;
    word_buffer->content_revision = 1;
    word_buffer->dirty = true;
    DvzSceneItemStateStyleParams initial = {0};
    if (dvz_scene_buffer_set_data(style_buffer, &initial, sizeof(initial)) != DVZ_OK)
    {
        dvz_scene_buffer_destroy(word_buffer);
        dvz_scene_buffer_destroy(style_buffer);
        dvz_free(revisions);
        return NULL;
    }
    *bank = (DvzItemStateBuffer){
        .scene = scene,
        .words = word_buffer,
        .style = style_buffer,
        .slot_count = desc->slot_count,
        .slot_revisions = revisions};
    word_buffer->item_state_bank = style_buffer->item_state_bank = bank;
    for (uint32_t i = 0; i < desc->slot_count; i++)
        revisions[i] = 1;
    (void)dvz_item_state_buffer_style(bank, NULL, NULL, false);
    return bank;
}



/** Atomically replace copied state words, with duplicate slots resolved in order.
 * @param buffer the bank
 * @param update_count number of replacements
 * @param slots slot indices
 * @param words complete replacement words
 * @return DVZ_OK on success
 */
DvzResult dvz_item_state_buffer_update(
    DvzItemStateBuffer* buffer, const uint32_t* slots, const uint32_t* words,
    uint32_t update_count)
{
    if (buffer == NULL || buffer->scene == NULL ||
        (update_count > 0 && (slots == NULL || words == NULL)) ||
        !_scene_visual_mutation_allowed(buffer->scene, "update indexed state words"))
        return DVZ_ERROR;
    for (uint32_t i = 0; i < update_count; i++)
        if (slots[i] >= buffer->slot_count || (words[i] & ~ITEM_STATE_BITS) != 0)
        {
            log_error("indexed state replacement has invalid slot or bits");
            return DVZ_ERROR;
        }
    if (update_count == 0)
        return DVZ_OK;
    DvzSceneBuffer* resource = buffer->words;
    if (resource->content_revision == UINT64_MAX)
        return DVZ_ERROR;
    uint32_t* retained = resource->data;
    bool changed = false;
    uint64_t revision = resource->content_revision + 1;
    for (uint32_t i = 0; i < update_count; i++)
    {
        if (retained[slots[i]] == words[i])
            continue;
        retained[slots[i]] = words[i];
        buffer->slot_revisions[slots[i]] = revision;
        changed = true;
    }
    if (changed)
    {
        resource->content_revision = revision;
        resource->dirty = true;
        _scene_notify_buffer_changed(resource);
    }
    return DVZ_OK;
}



/** Bind a copied per-vertex state index to an ordinary indexed triangle mesh.
 * @param visual the mesh
 * @param buffer the bank, or NULL to clear
 * @param indices copied per-vertex indices
 * @param vertex_count the exact position count
 * @return DVZ_OK on success
 */
DvzResult dvz_visual_set_item_state_buffer(
    DvzVisual* visual, DvzItemStateBuffer* buffer, const uint32_t* indices, uint32_t vertex_count)
{
    if (visual == NULL || visual->scene == NULL ||
        !_scene_visual_mutation_allowed(visual->scene, "bind indexed state bank"))
        return DVZ_ERROR;
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MESH)
        return DVZ_ERROR;
    if (buffer == NULL)
    {
        state->item_state_bank = NULL;
        int idx = _attr_index(visual, "state_index");
        if (idx >= 0)
        {
            dvz_free(visual->attrs[idx].data);
            visual->attrs[idx].data = NULL;
            visual->attrs[idx].item_count = visual->attrs[idx].dirty_item_count = 0;
        }
        _scene_notify_visual_redraw(visual);
        return DVZ_OK;
    }
    int pos = _attr_index(visual, "position");
    if (buffer->scene != visual->scene || indices == NULL || vertex_count == 0 || pos < 0 ||
        visual->attrs[pos].item_count != vertex_count || state->field != NULL ||
        state->texture.rgba != NULL || state->buffer == NULL ||
        state->topology != DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
        _scene_visual_has_dense_attr(visual, "item_state") ||
        _scene_visual_has_dense_attr(visual, "instance_transform") ||
        (visual->alpha_mode != DVZ_ALPHA_OPAQUE && visual->alpha_mode != DVZ_ALPHA_BLENDED &&
         visual->alpha_mode != DVZ_ALPHA_WBOIT) ||
        visual->scene_occluder)
    {
        log_error("indexed state requires an ordinary untextured indexed triangle mesh");
        return DVZ_ERROR;
    }
    for (uint32_t i = 0; i < vertex_count; i++)
        if (indices[i] >= buffer->slot_count)
            return DVZ_ERROR;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &bytes) || bytes > SIZE_MAX)
        return DVZ_ERROR;
    uint32_t* copied = dvz_malloc(bytes);
    if (copied == NULL)
        return DVZ_ERROR;
    dvz_memcpy(copied, bytes, indices, bytes);
    DvzVisualAttr* attr = _attr_get_or_create(visual, "state_index", sizeof(uint32_t));
    if (attr == NULL)
    {
        dvz_free(copied);
        return DVZ_ERROR;
    }
    dvz_free(attr->data);
    attr->data = copied;
    attr->buffer = NULL;
    attr->buffer_byte_offset = 0;
    attr->source = DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    attr->item_count = vertex_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = vertex_count;
    attr->version++;
    state->item_state_bank = buffer;
    _scene_notify_visual_redraw(visual);
    return DVZ_OK;
}



/** Clear consumers and retire the bank's existing scene resources.
 * @param buffer the bank
 */
void dvz_item_state_buffer_destroy(DvzItemStateBuffer* buffer)
{
    if (buffer == NULL || buffer->scene == NULL ||
        !_scene_visual_mutation_allowed(buffer->scene, "destroy indexed state bank"))
        return;
    DvzScene* scene = buffer->scene;
    for (uint32_t vi = 0; vi < scene->visual_count; vi++)
        if (_visual_family_state(&scene->visuals[vi]) != NULL &&
            _visual_family_state(&scene->visuals[vi])->item_state_bank == buffer)
            (void)dvz_visual_set_item_state_buffer(&scene->visuals[vi], NULL, NULL, 0);
    dvz_scene_buffer_destroy(buffer->words);
    dvz_scene_buffer_destroy(buffer->style);
    dvz_free(buffer->slot_revisions);
    *buffer = (DvzItemStateBuffer){0};
}


/** Reject conflicting authored attributes while a typed bank owns decoration.
 * @param visual the visual
 * @param name the attribute name
 * @return whether the generic mutation may proceed
 */
bool _visual_indexed_state_attr_mutation_allowed(const DvzVisual* visual, const char* name)
{
    if ((_visual_family_state(visual)->item_state_bank != NULL ||
         _scene_visual_has_dense_attr(visual, "fragment_clip")) &&
        (strcmp(name, "item_state") == 0 || strcmp(name, "instance_transform") == 0))
    {
        log_error("dense item_state and instancing conflict with typed mesh state or coverage");
        return false;
    }
    return true;
}
