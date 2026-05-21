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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const char* _attr_storage_name(DvzVisualType type, const char* name)
{
    ANN(name);
    if (strcmp(name, "size") == 0)
        return "size";
    if ((type == DVZ_VISUAL_TYPE_POINT || type == DVZ_VISUAL_TYPE_MARKER) &&
        strcmp(name, "diameter") == 0)
    {
        return "size";
    }
    if (type == DVZ_VISUAL_TYPE_PIXEL && strcmp(name, "pixel_size") == 0)
        return "size";
    if (type == DVZ_VISUAL_TYPE_SPHERE && strcmp(name, "radius") == 0)
        return "size";
    if ((type == DVZ_VISUAL_TYPE_SEGMENT || type == DVZ_VISUAL_TYPE_PATH) &&
        strcmp(name, "stroke_width") == 0)
    {
        return "line_width";
    }
    return name;
}


/**
 * Return whether an attribute advances per mesh instance instead of per vertex.
 *
 * @param name the retained attribute name
 * @return whether the attribute is per-instance
 */
bool _attr_is_instance_attribute(const char* name)
{
    ANN(name);
    return strcmp(name, "instance_transform") == 0 || strcmp(name, "instance_color") == 0 ||
           strcmp(name, "instance_id") == 0;
}



/**
 * Return the byte size of one supported visual attribute item.
 *
 * @param type the visual type
 * @param name the attribute name
 * @return the item size in bytes, or zero when unsupported
 */
uint32_t _attr_item_size(DvzVisualType type, const char* name)
{
    name = _attr_storage_name(type, name);
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
    case DVZ_VISUAL_TYPE_SPHERE:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "size") == 0)
            return sizeof(float);
        if ((type == DVZ_VISUAL_TYPE_POINT || type == DVZ_VISUAL_TYPE_MARKER) &&
            strcmp(name, "selection") == 0)
            return sizeof(uint8_t);
        if (type == DVZ_VISUAL_TYPE_MARKER && strcmp(name, "angle") == 0)
            return sizeof(float);
        if (type == DVZ_VISUAL_TYPE_MARKER && strcmp(name, "shape") == 0)
            return sizeof(uint32_t);
        break;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "normal") == 0)
            return 3 * sizeof(float);
        if (type == DVZ_VISUAL_TYPE_MESH && strcmp(name, "instance_transform") == 0)
            return 16 * sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_PATH:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "line_width") == 0)
            return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_SEGMENT:
        if (strcmp(name, "position_start") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "position_end") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "line_width") == 0)
            return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_IMAGE:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "extent") == 0)
            return 2 * sizeof(float);
        if (strcmp(name, "anchor") == 0)
            return 2 * sizeof(float);
        if (strcmp(name, "tex_rect") == 0)
            return 4 * sizeof(float);
        if (strcmp(name, "texcoords") == 0)
            return 2 * sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_TEXT:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "anchor") == 0)
            return 2 * sizeof(float);
        if (strcmp(name, "size") == 0)
            return sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "angle") == 0)
            return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_GLYPH:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "bounds") == 0)
            return 4 * sizeof(float);
        if (strcmp(name, "texcoords") == 0)
            return 4 * sizeof(float);
        if (strcmp(name, "color") == 0)
            return 4 * sizeof(uint8_t);
        if (strcmp(name, "angle") == 0)
            return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_VOLUME:
        if (strcmp(name, "position") == 0)
            return 3 * sizeof(float);
        if (strcmp(name, "texcoords") == 0)
            return 3 * sizeof(float);
        break;
    default:
        break;
    }
    return 0;
}



/**
 * Validate that one attribute is supported by a visual family.
 *
 * @param type the visual type
 * @param name the attribute name
 * @param item_size output item byte size
 * @return true when supported
 */
bool _attr_supported(DvzVisualType type, const char* name, uint32_t* item_size)
{
    ANN(name);
    ANN(item_size);
    *item_size = _attr_item_size(type, name);
    if (*item_size != 0)
        return true;

    const char* expected = "position, color, diameter, selection";
    if (type == DVZ_VISUAL_TYPE_MARKER)
        expected = "position, color, diameter, selection, angle, shape";
    else if (type == DVZ_VISUAL_TYPE_PIXEL)
        expected = "position, color, pixel_size";
    else if (type == DVZ_VISUAL_TYPE_SPHERE)
        expected = "position, color, radius";
    else if (type == DVZ_VISUAL_TYPE_PRIMITIVE)
        expected = "position, color, normal";
    else if (type == DVZ_VISUAL_TYPE_MESH)
        expected = "position, color, normal, instance_transform";
    else if (type == DVZ_VISUAL_TYPE_PATH)
        expected = "position, color, stroke_width";
    else if (type == DVZ_VISUAL_TYPE_SEGMENT)
        expected = "position_start, position_end, color, stroke_width";
    else if (type == DVZ_VISUAL_TYPE_IMAGE)
        expected = "position, extent, anchor, tex_rect, texcoords";
    else if (type == DVZ_VISUAL_TYPE_TEXT)
        expected = "text strings plus position, anchor, size, color, angle";
    else if (type == DVZ_VISUAL_TYPE_GLYPH)
        expected = "position, bounds, texcoords, color, angle, plus a bound 2D field";
    else if (type == DVZ_VISUAL_TYPE_VOLUME)
        expected = "position, texcoords, plus a bound 3D field";

    log_error(
        "unsupported %s visual attribute '%s' (expected one of: %s)", _visual_type_name(type),
        name, expected);
    return false;
}



/**
 * Validate that one semantic source is accepted by a visual attribute.
 *
 * @param type the visual type
 * @param name the attribute name
 * @param source the semantic source
 * @return true when the source is accepted
 */
bool _attr_source_supported(DvzVisualType type, const char* name, DvzVisualAttrSource source)
{
    ANN(name);
    uint32_t item_size = 0;
    if (!_attr_supported(type, name, &item_size))
        return false;
    name = _attr_storage_name(type, name);

    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
        return true;

    bool is_color = strcmp(name, "color") == 0;
    bool is_size = strcmp(name, "size") == 0;
    bool is_line_width = strcmp(name, "line_width") == 0;

    if (source == DVZ_VISUAL_ATTR_SOURCE_CONSTANT && (is_color || is_size || is_line_width))
        return true;
    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_GROUP && type != DVZ_VISUAL_TYPE_SEGMENT &&
        (is_color || is_size))
        return true;
    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_SPAN && type == DVZ_VISUAL_TYPE_PATH && is_color)
        return true;

    log_error(
        "%s visual attribute '%s' does not accept source %d", _visual_type_name(type), name,
        (int)source);
    return false;
}



/**
 * Return whether an attribute appears in a batch update list.
 *
 * @param updates update descriptors
 * @param update_count number of update descriptors
 * @param attr_name attribute name
 * @return whether the attribute is present
 */
bool _visual_data_update_contains_attr(
    DvzVisualType type, const DvzVisualDataUpdate* updates, uint32_t update_count,
    const char* attr_name)
{
    ANN(updates);
    ANN(attr_name);
    for (uint32_t i = 0; i < update_count; i++)
    {
        if (updates[i].attr_name != NULL &&
            strcmp(_attr_storage_name(type, updates[i].attr_name), attr_name) == 0)
            return true;
    }
    return false;
}



/**
 * Find the index of one visual attribute by name.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the attribute index, or -1 when absent
 */
int _attr_index(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    name = _attr_storage_name(visual->type, name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}



/**
 * Return an existing visual attribute slot or create one.
 *
 * @param visual the visual
 * @param name the attribute name
 * @param item_size the attribute item size in bytes
 * @return the attribute slot, or NULL when the visual has no free slot
 */
DvzVisualAttr* _attr_get_or_create(DvzVisual* visual, const char* name, uint32_t item_size)
{
    ANN(visual);
    ANN(name);
    name = _attr_storage_name(visual->type, name);
    int idx = _attr_index(visual, name);
    if (idx >= 0)
        return &visual->attrs[idx];
    if (visual->attr_count >= DVZ_SCENE_MAX_ITEM_ATTRS)
        return NULL;
    DvzVisualAttr* attr = &visual->attrs[visual->attr_count++];
    dvz_strlcpy(attr->name, name, sizeof(attr->name));
    attr->item_size = item_size;
    attr->source = DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    attr->mutability = DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC;
    return attr;
}



/**
 * Check that a new attribute item count matches existing dense attributes.
 *
 * @param visual the visual
 * @param attr_name the updated attribute name
 * @param item_count the updated item count
 * @return true when the count is accepted
 */
bool _visual_attr_count_consistent(
    const DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (item_count == 0)
        return false;

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        bool attr_has_payload = attr->data != NULL || attr->buffer != NULL;
        if (_attr_is_instance_attribute(attr_name) || _attr_is_instance_attribute(attr->name))
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_MESH && visual->mesh_default_color &&
            strcmp(attr_name, "position") == 0 && strcmp(attr->name, "color") == 0)
        {
            continue;
        }
        if (strcmp(attr->name, attr_name) == 0 || attr->item_count == 0 || !attr_has_payload)
            continue;
        if (attr->item_count == item_count)
            continue;

        log_error(
            "%s visual attribute '%s' item_count %u does not match existing attribute '%s' "
            "item_count %u",
            _visual_type_name(visual->type), attr_name, item_count, attr->name, attr->item_count);
        return false;
    }
    return true;
}



/**
 * Return the scene-global index of a figure visual.
 *
 * @param figure the figure
 * @param visual the visual
 * @param out_index output visual index
 * @return true when the visual belongs to the figure scene
 */
bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index)
{
    ANN(out_index);
    *out_index = 0;
    if (figure == NULL || figure->scene == NULL || visual == NULL)
        return false;
    if (visual->scene != figure->scene)
        return false;
    for (uint32_t i = 0; i < figure->scene->visual_count; i++)
    {
        if (&figure->scene->visuals[i] == visual)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}



/**
 * Return one mutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
DvzVisualBinding* _visual_binding(DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    visual->bindings[idx].kind = kind;
    return &visual->bindings[idx];
}



/**
 * Return one immutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
const DvzVisualBinding* _visual_binding_const(const DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    return &visual->bindings[idx];
}



/**
 * Assign one visual binding and keep legacy convenience fields in sync.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @param slot_name the binding slot name, or NULL to clear
 * @param resource the bound resource, or NULL to clear
 * @param owned whether the visual owns the resource
 */
void _visual_binding_assign(
    DvzVisual* visual, DvzVisualBindingKind kind, const char* slot_name, void* resource,
    bool owned)
{
    ANN(visual);
    DvzVisualBinding* binding = _visual_binding(visual, kind);
    ANN(binding);
    binding->resource = resource;
    binding->owned = owned;
    dvz_memset(binding->slot, sizeof(binding->slot), 0, sizeof(binding->slot));
    if (slot_name != NULL && resource != NULL)
        dvz_strlcpy(binding->slot, slot_name, sizeof(binding->slot));

    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        visual->field = (DvzSampledField*)resource;
        visual->field_owned = owned;
        dvz_memset(visual->field_slot, sizeof(visual->field_slot), 0, sizeof(visual->field_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->field_slot, slot_name, sizeof(visual->field_slot));
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        visual->buffer = (DvzSceneBuffer*)resource;
        dvz_memset(
            visual->buffer_slot, sizeof(visual->buffer_slot), 0, sizeof(visual->buffer_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->buffer_slot, slot_name, sizeof(visual->buffer_slot));
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        visual->scale = (DvzScale*)resource;
        dvz_memset(visual->scale_slot, sizeof(visual->scale_slot), 0, sizeof(visual->scale_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->scale_slot, slot_name, sizeof(visual->scale_slot));
        break;
    default:
        break;
    }
}



/**
 * Clear one visual binding.
 *
 * @param visual the visual
 * @param kind the binding kind
 */
void _visual_binding_clear(DvzVisual* visual, DvzVisualBindingKind kind)
{
    _visual_binding_assign(visual, kind, NULL, NULL, false);
}



/**
 * Clear one visual scale binding.
 *
 * @param visual the visual
 */



/**
 * Advance a retained visual payload version.
 *
 * @param version the version counter
 */
void _visual_bump_version(uint64_t* version)
{
    ANN(version);
    *version = *version == UINT64_MAX ? 1 : *version + 1;
}



/**
 * Ensure a mesh has a default opaque-white color attribute.
 *
 * @param visual the mesh visual
 * @param item_count the color item count
 * @return true on success
 */
bool _mesh_ensure_default_color(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MESH || item_count == 0)
        return true;

    DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
    if (color == NULL)
        return false;
    if (color->data != NULL && color->item_count == item_count)
        return true;
    if (color->data != NULL)
    {
        dvz_free(color->data);
        color->data = NULL;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, color->item_size, &byte_size))
        return false;
    color->data = dvz_malloc(byte_size);
    if (color->data == NULL)
        return false;
    dvz_memset(color->data, byte_size, 255, byte_size);
    color->item_count = item_count;
    color->dirty_first_item = 0;
    color->dirty_item_count = item_count;
    _visual_bump_version(&color->version);
    visual->mesh_default_color = true;
    return true;
}



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
 * Replace one dense visual attribute payload.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param data the attribute data
 * @param item_count the number of attribute items
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_data(
    DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' requires item_count > 0", attr_name);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error(
            "visual attribute '%s' dense data requires PER_ITEM source; use source-specific data",
            attr_name);
        return -1;
    }
    if (attr->buffer != NULL)
    {
        log_error("visual attribute '%s' already has a bound buffer", attr_name);
        return -1;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }

    /* Reallocate if total size changed */
    if (attr->data != NULL && attr->item_count != item_count)
    {
        dvz_free(attr->data);
        attr->data = NULL;
    }
    if (attr->data == NULL)
    {
        attr->data = dvz_malloc(byte_size);
        if (attr->data == NULL)
        {
            log_error(
                "visual attribute '%s' allocation failed for %" PRIu64 " bytes", attr_name,
                byte_size);
            attr->item_count = 0;
            attr->dirty_first_item = 0;
            attr->dirty_item_count = 0;
            return -1;
        }
    }

    dvz_memcpy(attr->data, byte_size, data, byte_size);
    attr->item_count = item_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = item_count; /* whole buffer dirty */
    _visual_bump_version(&attr->version);
    if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "position") == 0)
    {
        DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
        if (color == NULL)
            return -1;
        if (color->data == NULL || visual->mesh_default_color)
        {
            if (!_mesh_ensure_default_color(visual, item_count))
            {
                log_error("mesh default color allocation failed");
                return -1;
            }
        }
    }
    else if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "color") == 0)
    {
        visual->mesh_default_color = false;
    }
    if (visual->type == DVZ_VISUAL_TYPE_PATH && strcmp(attr_name, "line_width") == 0)
        visual->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Replace one variable-length string attribute on a visual.
 *
 * @param visual the visual
 * @param attr_name the string attribute name
 * @param strings string array
 * @param item_count number of strings
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_strings(
    DvzVisual* visual, const char* attr_name, const char* const* strings, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(strings);
    if (visual->type != DVZ_VISUAL_TYPE_TEXT || strcmp(attr_name, "text") != 0)
    {
        log_error("visual string attribute '%s' is only supported on text visuals", attr_name);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual strings"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual string attribute '%s' requires item_count > 0", attr_name);
        return -1;
    }

    char** copy = (char**)dvz_calloc(item_count, sizeof(char*));
    if (copy == NULL)
    {
        log_error("text visual string table allocation failed");
        return -1;
    }
    for (uint32_t i = 0; i < item_count; i++)
    {
        const char* src = strings[i] != NULL ? strings[i] : "";
        size_t len = strlen(src);
        if (len >= DVZ_SCENE_LABEL_SIZE)
            len = DVZ_SCENE_LABEL_SIZE - 1u;
        copy[i] = (char*)dvz_calloc((DvzSize)len + 1u, 1);
        if (copy[i] == NULL)
        {
            for (uint32_t j = 0; j < i; j++)
                dvz_free(copy[j]);
            dvz_free(copy);
            log_error("text visual string allocation failed");
            return -1;
        }
        dvz_memcpy(copy[i], len, src, len);
        copy[i][len] = '\0';
    }

    if (visual->text.strings != NULL)
    {
        for (uint32_t i = 0; i < visual->text.string_count; i++)
            dvz_free(visual->text.strings[i]);
        dvz_free(visual->text.strings);
    }
    visual->text.strings = copy;
    visual->text.string_count = item_count;
    visual->text.strings_version++;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Atomically replace several dense visual attribute payloads.
 *
 * @param visual the visual
 * @param updates attribute update descriptors
 * @param update_count number of update descriptors
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_data_many(
    DvzVisual* visual, const DvzVisualDataUpdate* updates, uint32_t update_count)
{
    ANN(visual);
    ANN(updates);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (update_count == 0)
    {
        log_error("visual batch data update requires update_count > 0");
        return -1;
    }

    typedef struct PreparedUpdate
    {
        int attr_idx;
        uint32_t item_size;
        uint64_t byte_size;
        const char* attr_name;
        void* data;
    } PreparedUpdate;

    PreparedUpdate* prepared =
        (PreparedUpdate*)dvz_calloc((DvzSize)update_count, sizeof(PreparedUpdate));
    if (prepared == NULL)
    {
        log_error("visual batch data update allocation failed");
        return -1;
    }

    uint32_t batch_item_count = 0;
    uint32_t new_attr_count = 0;
    for (uint32_t i = 0; i < update_count; i++)
    {
        const DvzVisualDataUpdate* update = &updates[i];
        const char* attr_name =
            update->attr_name != NULL ? _attr_storage_name(visual->type, update->attr_name) : NULL;
        if (update->attr_name == NULL || update->data == NULL || update->item_count == 0)
        {
            log_error("visual batch data update contains an invalid descriptor");
            dvz_free(prepared);
            return -1;
        }

        for (uint32_t j = 0; j < i; j++)
        {
            if (strcmp(prepared[j].attr_name, attr_name) == 0)
            {
                log_error("visual batch data update repeats attribute '%s'", update->attr_name);
                dvz_free(prepared);
                return -1;
            }
        }

        uint32_t item_size = 0;
        if (!_attr_supported(visual->type, update->attr_name, &item_size))
        {
            dvz_free(prepared);
            return -1;
        }

        if (i == 0)
            batch_item_count = update->item_count;
        else if (update->item_count != batch_item_count)
        {
            log_error(
                "visual batch data update attribute '%s' item_count %u does not match batch "
                "item_count %u",
                update->attr_name, update->item_count, batch_item_count);
            dvz_free(prepared);
            return -1;
        }

        int attr_idx = _attr_index(visual, update->attr_name);
        if (attr_idx >= 0)
        {
            DvzVisualAttr* attr = &visual->attrs[attr_idx];
            if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
            {
                log_error(
                    "visual attribute '%s' dense data requires PER_ITEM source; use "
                    "source-specific data",
                    update->attr_name);
                dvz_free(prepared);
                return -1;
            }
            if (attr->buffer != NULL)
            {
                log_error("visual attribute '%s' already has a bound buffer", update->attr_name);
                dvz_free(prepared);
                return -1;
            }
        }
        else
        {
            new_attr_count++;
        }

        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(update->item_count, item_size, &byte_size))
        {
            log_error(
                "visual attribute '%s' byte size overflow for item_count=%u item_size=%u",
                update->attr_name, update->item_count, item_size);
            dvz_free(prepared);
            return -1;
        }

        prepared[i].attr_idx = attr_idx;
        prepared[i].item_size = item_size;
        prepared[i].byte_size = byte_size;
        prepared[i].attr_name = attr_name;
    }

    if (visual->attr_count + new_attr_count > DVZ_SCENE_MAX_ITEM_ATTRS)
    {
        log_error("visual batch data update exceeds the maximum attribute count");
        dvz_free(prepared);
        return -1;
    }

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        bool attr_has_payload = attr->data != NULL || attr->buffer != NULL;
        if (attr->item_count == 0 || !attr_has_payload)
            continue;
        if (_attr_is_instance_attribute(attr->name))
            continue;
        if (_visual_data_update_contains_attr(visual->type, updates, update_count, attr->name))
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_MESH && visual->mesh_default_color &&
            strcmp(attr->name, "color") == 0 &&
            _visual_data_update_contains_attr(visual->type, updates, update_count, "position"))
        {
            continue;
        }
        if (attr->item_count == batch_item_count)
            continue;

        log_error(
            "%s visual batch data update item_count %u omits existing attribute '%s' "
            "item_count %u",
            _visual_type_name(visual->type), batch_item_count, attr->name, attr->item_count);
        dvz_free(prepared);
        return -1;
    }

    for (uint32_t i = 0; i < update_count; i++)
    {
        prepared[i].data = dvz_malloc(prepared[i].byte_size);
        if (prepared[i].data == NULL)
        {
            log_error(
                "visual attribute '%s' allocation failed for %" PRIu64 " bytes",
                prepared[i].attr_name, prepared[i].byte_size);
            for (uint32_t j = 0; j < i; j++)
                dvz_free(prepared[j].data);
            dvz_free(prepared);
            return -1;
        }
        dvz_memcpy(
            prepared[i].data, prepared[i].byte_size, updates[i].data, prepared[i].byte_size);
    }

    bool mesh_position_updated = false;
    bool mesh_color_updated = false;
    bool path_line_width_updated = false;
    for (uint32_t i = 0; i < update_count; i++)
    {
        DvzVisualAttr* attr =
            prepared[i].attr_idx >= 0
                ? &visual->attrs[prepared[i].attr_idx]
                : _attr_get_or_create(visual, prepared[i].attr_name, prepared[i].item_size);
        if (attr == NULL)
        {
            for (uint32_t j = i; j < update_count; j++)
                dvz_free(prepared[j].data);
            dvz_free(prepared);
            return -1;
        }

        dvz_free(attr->data);
        attr->data = prepared[i].data;
        prepared[i].data = NULL;
        attr->item_count = updates[i].item_count;
        attr->dirty_first_item = 0;
        attr->dirty_item_count = updates[i].item_count;
        _visual_bump_version(&attr->version);

        if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(prepared[i].attr_name, "position") == 0)
            mesh_position_updated = true;
        if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(prepared[i].attr_name, "color") == 0)
            mesh_color_updated = true;
        if (visual->type == DVZ_VISUAL_TYPE_PATH &&
            strcmp(prepared[i].attr_name, "line_width") == 0)
            path_line_width_updated = true;
    }

    if (mesh_color_updated)
        visual->mesh_default_color = false;
    if (path_line_width_updated)
        visual->material_params_dirty = true;
    if (mesh_position_updated)
    {
        DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
        if (color == NULL)
        {
            dvz_free(prepared);
            return -1;
        }
        if (color->data == NULL || visual->mesh_default_color)
        {
            if (!_mesh_ensure_default_color(visual, batch_item_count))
            {
                log_error("mesh default color allocation failed");
                dvz_free(prepared);
                return -1;
            }
        }
    }

    dvz_free(prepared);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Replace a subrange of one dense visual attribute payload.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param data the source data
 * @param first_item the first item to replace
 * @param item_count the number of items to replace
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_data_range(
    DvzVisual* visual, const char* attr_name, const void* data, uint32_t first_item,
    uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    attr_name = _attr_storage_name(visual->type, attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' range update requires item_count > 0", attr_name);
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
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error("visual attribute '%s' dense range update requires PER_ITEM source", attr_name);
        return -1;
    }
    if (attr->buffer != NULL)
    {
        log_error("visual attribute '%s' range update cannot target a bound buffer", attr_name);
        return -1;
    }

    /* The attribute must already be fully allocated */
    if (attr->data == NULL || attr->item_count == 0)
    {
        log_error(
            "visual attribute '%s' range update requires prior full allocation with "
            "dvz_visual_set_data()",
            attr_name);
        return -1;
    }
    uint64_t item_end = 0;
    if (_dvz_add_u64_overflows(first_item, item_count, &item_end))
    {
        log_error(
            "visual attribute '%s' range update overflow for first_item=%u item_count=%u",
            attr_name, first_item, item_count);
        return -1;
    }
    if (item_end > attr->item_count)
    {
        log_error(
            "visual attribute '%s' range update [%u, %" PRIu64 ") exceeds item_count %u",
            attr_name, first_item, item_end, attr->item_count);
        return -1;
    }

    uint64_t byte_offset = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(first_item, item_size, &byte_offset))
    {
        log_error(
            "visual attribute '%s' byte offset overflow for first_item=%u item_size=%u", attr_name,
            first_item, item_size);
        return -1;
    }
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }
    dvz_memcpy((uint8_t*)attr->data + byte_offset, byte_size, data, byte_size);

    /* Extend dirty range to cover the new update */
    if (attr->dirty_item_count == 0)
    {
        attr->dirty_first_item = first_item;
        attr->dirty_item_count = item_count;
    }
    else
    {
        uint64_t old_end = 0;
        uint64_t new_end = 0;
        if (_dvz_add_u64_overflows(attr->dirty_first_item, attr->dirty_item_count, &old_end))
            return -1;
        if (_dvz_add_u64_overflows(first_item, item_count, &new_end))
            return -1;
        uint64_t merged_first =
            attr->dirty_first_item < first_item ? attr->dirty_first_item : first_item;
        uint64_t merged_end = old_end > new_end ? old_end : new_end;
        attr->dirty_first_item = merged_first;
        attr->dirty_item_count = merged_end - merged_first;
    }
    if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "color") == 0)
        visual->mesh_default_color = false;
    if (visual->type == DVZ_VISUAL_TYPE_PATH && strcmp(attr_name, "line_width") == 0)
        visual->material_params_dirty = true;
    _visual_bump_version(&attr->version);
    _scene_notify_visual_changed(visual);
    return 0;
}



int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale)
{
    ANN(visual);
    ANN(slot_name);
    if (scale != NULL && scale->scene != visual->scene)
    {
        log_error("cannot bind a scale from a different scene");
        return -1;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_visual_set_scale is only supported for image and volume visuals");
        return -1;
    }
    if (strcmp(slot_name, "colormap") != 0)
    {
        log_error("unsupported visual scale slot '%s' (expected 'colormap')", slot_name);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scale"))
        return -1;
    _scene_release_visual_scale(visual);
    if (scale != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_SCALE, slot_name, scale, false);
    if (visual->field != NULL && _field_format_is_scalar(visual->field->desc.format))
    {
        _scene_visual_texture_mark_clean(visual);
        visual->texture.dirty = true;
        _visual_bump_version(&visual->texture.version);
    }
    _scene_notify_visual_changed(visual);
    return 0;
}



/*************************************************************************************************/
/*  Visual names                                                                                 */
/*************************************************************************************************/

/**
 * Return the debug name of one visual type.
 *
 * @param type the visual type
 * @return the visual type name
 */
const char* _visual_type_name(DvzVisualType type)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return "point";
    case DVZ_VISUAL_TYPE_PIXEL:
        return "pixel";
    case DVZ_VISUAL_TYPE_MARKER:
        return "marker";
    case DVZ_VISUAL_TYPE_SEGMENT:
        return "segment";
    case DVZ_VISUAL_TYPE_PATH:
        return "path";
    case DVZ_VISUAL_TYPE_IMAGE:
        return "image";
    case DVZ_VISUAL_TYPE_TEXT:
        return "text";
    case DVZ_VISUAL_TYPE_GLYPH:
        return "glyph";
    case DVZ_VISUAL_TYPE_MESH:
        return "mesh";
    case DVZ_VISUAL_TYPE_VOLUME:
        return "volume";
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return "primitive";
    case DVZ_VISUAL_TYPE_SPHERE:
        return "sphere";
    default:
        return "unknown";
    }
}
