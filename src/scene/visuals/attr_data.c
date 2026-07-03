/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual attribute data */
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
#include "datoviz/scene.h"
#include "registry/registry.h"
#include "sample_profile.h"
#include "text/text_internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve the current logical item count for the point item-range slice.
 *
 * @param visual the visual
 * @param out_count output logical item count
 * @return whether the count was resolved
 */
static bool
_visual_item_range_logical_count(const DvzVisual* visual, uint64_t* out_count, bool log_errors)
{
    ANN(out_count);
    *out_count = 0;
    if (visual == NULL)
        return false;
    const DvzVisualFamilyOps* ops = visual->ops;
    if (ops == NULL || ops->item_range_attr_name == NULL)
    {
        if (log_errors)
            log_error(
                "%s visual item ranges are not supported in this v0.4 slice",
                ops != NULL ? ops->name : _visual_type_name(visual->type));
        return false;
    }

    int idx = _attr_index(visual, ops->item_range_attr_name);
    if (idx < 0 || visual->attrs[idx].item_count == 0)
    {
        if (log_errors)
            log_error(
                "%s visual item range requires a retained %s item count", ops->name,
                ops->item_range_attr_name);
        return false;
    }
    *out_count = visual->attrs[idx].item_count;
    return true;
}



/**
 * Validate family-specific dense attribute payload values before retaining them.
 *
 * @param visual the visual
 * @param attr_name storage attribute name
 * @param data packed attribute data
 * @param item_count number of items
 * @return whether the values are accepted
 */
bool _scene_splat_visual_validate_attr(
    const DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    (void)visual;

    if (strcmp(attr_name, "sigma") == 0)
    {
        const float* sigma = (const float*)data;
        for (uint32_t i = 0; i < item_count; i++)
        {
            float sx = sigma[2 * i + 0];
            float sy = sigma[2 * i + 1];
            if (!isfinite(sx) || !isfinite(sy) || sx <= 0.0f || sy <= 0.0f)
            {
                log_error("splat visual attribute 'sigma' requires finite positive components");
                return false;
            }
        }
        return true;
    }

    if (strcmp(attr_name, "angle") == 0)
    {
        const float* angles = (const float*)data;
        for (uint32_t i = 0; i < item_count; i++)
        {
            if (!isfinite(angles[i]))
            {
                log_error("splat visual attribute 'angle' requires finite values");
                return false;
            }
        }
    }

    return true;
}



/**
 * Apply mesh-specific dense attribute side effects.
 *
 * @param visual the visual
 * @param attr_name storage attribute name
 * @param item_count number of items in the written attribute
 * @return whether side effects succeeded
 */
bool _scene_mesh_visual_after_attr_set(DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    if (strcmp(attr_name, "position") == 0)
    {
        DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
        if (color == NULL)
            return false;
        if (color->data == NULL || _visual_family_state(visual)->mesh_default_color)
        {
            if (!_mesh_ensure_default_color(visual, item_count))
            {
                log_error("mesh default color allocation failed");
                return false;
            }
        }
    }
    else if (strcmp(attr_name, "color") == 0)
    {
        _visual_family_state(visual)->mesh_default_color = false;
    }
    return true;
}



/**
 * Apply stroke-family dense attribute side effects.
 *
 * @param visual the visual
 * @param attr_name storage attribute name
 * @param item_count number of items in the written attribute
 * @return whether side effects succeeded
 */
bool _scene_stroke_visual_after_attr_set(
    DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    (void)item_count;
    if (strcmp(attr_name, "line_width") == 0)
        _visual_family_state(visual)->material_params_dirty = true;
    return true;
}



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
    if (item_count == 0)
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
    _visual_family_state(visual)->mesh_default_color = true;
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
DvzResult dvz_visual_set_data(
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
    item_size = _visual_attr_item_size(visual, attr_name);
    if (item_size == 0)
        return -1;
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return -1;
    if (
        visual->ops != NULL && visual->ops->validate_attr != NULL &&
        !visual->ops->validate_attr(visual, attr_name, data, item_count))
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
    if (
        visual->ops != NULL && visual->ops->after_attr_set != NULL &&
        !visual->ops->after_attr_set(visual, attr_name, item_count))
        return -1;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return a read-only view of retained dense visual attribute data.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param out output data view
 * @return 0 when dense data is available, -1 otherwise
 */
DvzResult dvz_visual_data(const DvzVisual* visual, const char* attr_name, DvzVisualDataView* out)
{
    if (visual == NULL || attr_name == NULL || out == NULL)
        return -1;

    dvz_memset(out, sizeof(DvzVisualDataView), 0, sizeof(DvzVisualDataView));
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return -1;

    const DvzVisualAttr* attr = &visual->attrs[idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size == 0)
        return -1;
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM || attr->buffer != NULL)
        return -1;

    out->data = attr->data;
    out->item_count = attr->item_count;
    out->item_size = attr->item_size;
    out->source = attr->source;
    out->mutability = attr->mutability;
    out->version = attr->version;
    return 0;
}



/**
 * Set the active retained visual logical item range.
 *
 * @param visual the visual
 * @param first_item first logical item
 * @param item_count logical item count
 * @return 0 on success, -1 on error
 */
DvzResult dvz_visual_set_item_range(DvzVisual* visual, uint32_t first_item, uint32_t item_count)
{
    if (visual == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual item range"))
        return -1;

    uint64_t logical_count = 0;
    if (!_visual_item_range_logical_count(visual, &logical_count, true))
        return -1;

    uint64_t item_end = 0;
    if (_dvz_add_u64_overflows(first_item, item_count, &item_end))
    {
        log_error(
            "visual item range overflow for first_item=%u item_count=%u", first_item,
            item_count);
        return -1;
    }
    if (item_end > logical_count)
    {
        log_error(
            "visual item range [%u, %" PRIu64 ") exceeds logical item_count %" PRIu64,
            first_item, item_end, logical_count);
        return -1;
    }

    if (
        visual->has_item_range && visual->item_range_first == first_item &&
        visual->item_range_count == item_count)
    {
        return 0;
    }

    visual->has_item_range = true;
    visual->item_range_first = first_item;
    visual->item_range_count = item_count;
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Clear the active retained visual logical item range.
 *
 * @param visual the visual
 */
void dvz_visual_clear_item_range(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual item range"))
        return;
    if (!visual->has_item_range)
        return;

    visual->has_item_range = false;
    visual->item_range_first = 0;
    visual->item_range_count = 0;
    _scene_notify_visual_changed(visual);
}



/**
 * Return the effective retained visual logical item range.
 *
 * @param visual the visual
 * @param out output range
 * @return whether the effective range was resolved
 */
bool dvz_visual_get_item_range(const DvzVisual* visual, DvzItemRange* out)
{
    if (visual == NULL || out == NULL)
        return false;

    uint64_t logical_count = 0;
    if (!_visual_item_range_logical_count(visual, &logical_count, false) || logical_count > UINT32_MAX)
        return false;

    if (visual->has_item_range)
    {
        out->first_item = visual->item_range_first;
        out->item_count = visual->item_range_count;
    }
    else
    {
        out->first_item = 0;
        out->item_count = (uint32_t)logical_count;
    }
    return true;
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
DvzResult dvz_visual_set_strings(
    DvzVisual* visual, const char* attr_name, const char* const* strings, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(strings);
    if (
        visual->ops == NULL || strcmp(visual->ops->name, "text") != 0 ||
        strcmp(attr_name, "text") != 0)
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
        copy[i] = _scene_text_strdup(src);
        if (copy[i] == NULL)
        {
            for (uint32_t j = 0; j < i; j++)
                dvz_free(copy[j]);
            dvz_free(copy);
            return -1;
        }
    }

    if (_visual_family_state(visual)->text.strings != NULL)
    {
        for (uint32_t i = 0; i < _visual_family_state(visual)->text.string_count; i++)
            dvz_free(_visual_family_state(visual)->text.strings[i]);
        dvz_free(_visual_family_state(visual)->text.strings);
    }
    _visual_family_state(visual)->text.strings = copy;
    _visual_family_state(visual)->text.string_count = item_count;
    _visual_family_state(visual)->text.strings_version++;
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
DvzResult dvz_visual_set_data_many(
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
        item_size = _visual_attr_item_size(visual, update->attr_name);
        if (item_size == 0)
        {
            dvz_free(prepared);
            return -1;
        }

        bool update_instance_attr = _attr_is_instance_attribute(visual->type, update->attr_name);
        if (!update_instance_attr && batch_item_count == 0)
            batch_item_count = update->item_count;
        else if (!update_instance_attr && update->item_count != batch_item_count)
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
        if (
            visual->ops != NULL && visual->ops->validate_attr != NULL &&
            !visual->ops->validate_attr(visual, attr_name, update->data, update->item_count))
        {
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
        if (attr->item_count == 0 || !attr_has_payload || batch_item_count == 0)
            continue;
        if (_attr_is_instance_attribute(visual->type, attr->name))
            continue;
        if (_visual_data_update_contains_attr(visual->type, updates, update_count, attr->name))
            continue;
        if (
            _visual_family_state(visual)->mesh_default_color && strcmp(attr->name, "color") == 0 &&
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

        if (visual->ops != NULL && visual->ops->after_attr_set != NULL)
        {
            if (!visual->ops->after_attr_set(visual, prepared[i].attr_name, updates[i].item_count))
            {
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
DvzResult dvz_visual_set_data_range(
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
    item_size = _visual_attr_item_size(visual, attr_name);
    if (item_size == 0)
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
    if (visual->ops != NULL && visual->ops->after_attr_set != NULL)
    {
        if (!visual->ops->after_attr_set(visual, attr_name, item_count))
            return -1;
    }
    _visual_bump_version(&attr->version);
    _scene_notify_visual_changed(visual);
    return 0;
}
