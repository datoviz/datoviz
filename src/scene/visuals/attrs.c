/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual attrs */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_scene_resource_key.h"
#include "_visual_family.h"
#include "_visual_internal.h"
#include "bindings_internal.h"
#include "datoviz/scene.h"
#include "domain/field_internal.h"
#include "sample_profile.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Declare the semantic source for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param source the semantic source
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_attr_source(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrSource source)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual metadata"))
        return -1;
    if (source < DVZ_VISUAL_ATTR_SOURCE_PER_ITEM || source > DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)
    {
        log_error("invalid visual attribute source %d", (int)source);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;
    if (!_attr_source_supported(visual->type, attr_name, source))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }
    if (attr->data != NULL && attr->item_count > 0 && source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error(
            "visual attribute '%s' already has dense per-item data and cannot switch source",
            attr_name);
        return -1;
    }

    attr->source = source;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return the semantic source for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return the semantic source
 */
DvzVisualAttrSource dvz_visual_attr_source(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    return visual->attrs[idx].source;
}



/**
 * Declare the expected update frequency for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param mutability the update-frequency hint
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_attr_mutability(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrMutability mutability)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual metadata"))
        return -1;
    if (mutability < DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC ||
        mutability > DVZ_VISUAL_ATTR_MUTABILITY_STREAMING)
    {
        log_error("invalid visual attribute mutability %d", (int)mutability);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }

    attr->mutability = mutability;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return the expected update frequency for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return the mutability hint
 */
DvzVisualAttrMutability dvz_visual_attr_mutability(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC;
    return visual->attrs[idx].mutability;
}



/**
 * Bind a scene buffer as one per-item visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param buffer the scene buffer, or NULL to clear
 * @param byte_offset byte offset into the buffer
 * @param item_count number of attribute items
 * @return whether the binding was updated
 */
bool dvz_visual_set_attr_buffer(
    DvzVisual* visual, const char* attr_name, DvzSceneBuffer* buffer, uint64_t byte_offset,
    uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (buffer != NULL && buffer->scene != visual->scene)
    {
        log_error("cannot bind an attribute buffer from a different scene");
        return false;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind visual attribute buffer"))
        return false;

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return false;
    if (!_attr_source_supported(visual->type, attr_name, DVZ_VISUAL_ATTR_SOURCE_PER_ITEM))
        return false;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return false;
    }

    if (buffer == NULL)
    {
        attr->buffer = NULL;
        attr->buffer_byte_offset = 0;
        if (attr->data == NULL)
            attr->item_count = 0;
        _scene_notify_visual_changed(visual);
        return true;
    }

    if (item_count == 0)
    {
        log_error("visual attribute buffer '%s' requires item_count > 0", attr_name);
        return false;
    }
    if (byte_offset != 0)
    {
        log_error("visual attribute buffer '%s' byte offsets are not supported yet", attr_name);
        return false;
    }
    if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_VERTEX) == 0)
    {
        log_error("visual attribute buffer '%s' requires VERTEX usage", attr_name);
        return false;
    }
    if (buffer->desc.stride != item_size)
    {
        log_error(
            "visual attribute buffer '%s' stride %u does not match item size %u", attr_name,
            buffer->desc.stride, item_size);
        return false;
    }
    if (attr->data != NULL)
    {
        log_error(
            "visual attribute '%s' already has dense data and cannot bind a buffer", attr_name);
        return false;
    }
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error("visual attribute buffer '%s' requires PER_ITEM source", attr_name);
        return false;
    }
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return false;

    uint64_t byte_size = 0;
    uint64_t byte_end = 0;
    if (_dvz_mul_u64_overflows(item_count, buffer->desc.stride, &byte_size) ||
        _dvz_add_u64_overflows(byte_offset, byte_size, &byte_end) ||
        byte_end > buffer->desc.byte_size)
    {
        log_error(
            "visual attribute buffer '%s' range exceeds buffer size (%" PRIu64 " > %" PRIu64 ")",
            attr_name, byte_end, buffer->desc.byte_size);
        return false;
    }

    attr->buffer = buffer;
    attr->buffer_byte_offset = byte_offset;
    attr->item_count = item_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = 0;
    _visual_bump_version(&attr->version);
    _scene_notify_visual_changed(visual);
    return true;
}



/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * Image and volume visuals accept the `"colormap"` slot. Labels visuals, and label-volume
 * render modes, accept the `"labels"` slot.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale)
{
    ANN(visual);
    ANN(slot_name);
    if (scale != NULL && scale->scene != visual->scene)
    {
        log_error("cannot bind a scale from a different scene");
        return -1;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_VOLUME &&
        visual->type != DVZ_VISUAL_TYPE_LABELS)
    {
        log_error("dvz_visual_set_scale is only supported for image, volume, and labels visuals");
        return -1;
    }
    DvzSceneSampleProfile bound_profile = {0};
    bool has_bound_profile =
        visual->field != NULL &&
        _scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &bound_profile);
    const bool labels =
        visual->type == DVZ_VISUAL_TYPE_LABELS ||
        (visual->type == DVZ_VISUAL_TYPE_VOLUME && has_bound_profile &&
         _scene_sample_profile_is_integer_label(&bound_profile));
    const char* expected_slot = labels ? "labels" : "colormap";
    if (strcmp(slot_name, expected_slot) != 0)
    {
        log_error("unsupported visual scale slot '%s' (expected '%s')", slot_name, expected_slot);
        return -1;
    }
    if (labels && scale != NULL && scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("labels visuals require a categorical scale");
        return -1;
    }
    if (!labels && scale != NULL && scale->kind != DVZ_SCALE_CONTINUOUS)
    {
        log_error("image and volume visuals require a continuous scale");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scale"))
        return -1;
    _scene_release_visual_scale(visual);
    if (scale != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_SCALE, slot_name, scale, false);
    if (has_bound_profile &&
        (_scene_sample_profile_uses_continuous_colorizer(&bound_profile) ||
         _scene_sample_profile_is_integer_label(&bound_profile)))
    {
        _scene_visual_texture_mark_clean(visual);
        visual->texture.dirty = true;
        _visual_bump_version(&visual->texture.version);
    }
    _scene_notify_visual_changed(visual);
    return 0;
}
