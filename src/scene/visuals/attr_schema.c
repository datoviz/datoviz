/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual attribute schema */
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
#include "_scene_resource_key.h"
#include "_visual_family.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"
#include "sample_profile.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const char* _attr_storage_name(DvzVisualType type, const char* name)
{
    ANN(name);
    return _visual_family_attr_storage_name(type, name);
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
    const DvzVisualFamilyAttrDesc* desc = _visual_family_attr_desc(DVZ_VISUAL_TYPE_MESH, name);
    return (desc != NULL && desc->instance) || strcmp(name, "instance_color") == 0 ||
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
    const DvzVisualFamilyAttrDesc* desc = _visual_family_attr_desc(type, name);
    return desc != NULL ? desc->item_size : 0;
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

    const char* expected = _visual_family_attr_expected(type);

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
    if (_visual_family_attr_source_supported(type, name, source))
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
        if (
            _visual_family_state(visual)->mesh_default_color && strcmp(attr_name, "position") == 0 &&
            strcmp(attr->name, "color") == 0)
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

