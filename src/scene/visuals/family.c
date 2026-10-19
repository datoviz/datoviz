/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family descriptors                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "_visual_family.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the retained storage name for a public visual attribute name.
 *
 * @param type the visual type
 * @param name the public attribute name
 * @return the retained storage name
 */
const char* _visual_family_attr_storage_name(DvzVisualType type, const char* name)
{
    ANN(name);
    if (strcmp(name, "size") == 0)
        return "size";
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    if (ops != NULL && ops->attr_storage_name != NULL)
        name = ops->attr_storage_name(name);
    if (ops != NULL && ops->attr_alias_public != NULL && ops->attr_alias_storage != NULL &&
        strcmp(ops->attr_alias_public, name) == 0)
        return ops->attr_alias_storage;
    return name;
}



/**
 * Return one visual-family attribute descriptor.
 *
 * @param type the visual type
 * @param name the public or retained attribute name
 * @return the attribute descriptor, or NULL when unsupported
 */
const DvzVisualFamilyAttrDesc*
_visual_family_attr_desc(DvzVisualType type, const char* name)
{
    ANN(name);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    if (ops == NULL)
        return NULL;
    name = _visual_family_attr_storage_name(type, name);
    for (uint32_t i = 0; i < ops->attr_count; i++)
    {
        if (strcmp(ops->attrs[i].name, name) == 0)
            return &ops->attrs[i];
    }
    return NULL;
}



/**
 * Return a human-readable list of expected attributes for one visual family.
 *
 * @param type the visual type
 * @return expected attribute list
 */
const char* _visual_family_attr_expected(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL && ops->expected_attrs != NULL ? ops->expected_attrs
                                                      : "position, color, diameter_px, selection";
}



/**
 * Return whether a semantic source is accepted by a visual-family attribute.
 *
 * @param type the visual type
 * @param name the public or retained attribute name
 * @param source the semantic source
 * @return whether the source is supported
 */
bool _visual_family_attr_source_supported(
    DvzVisualType type, const char* name, DvzVisualAttrSource source)
{
    const DvzVisualFamilyAttrDesc* desc = _visual_family_attr_desc(type, name);
    if (desc == NULL)
        return false;
    if (source < DVZ_VISUAL_ATTR_SOURCE_PER_ITEM || source > DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)
        return false;
    return (desc->source_mask & (1u << source)) != 0;
}



/**
 * Return whether a visual attribute is authored in logical screen pixels.
 *
 * @param type the visual type
 * @param name the public or retained attribute name
 * @return whether the attribute requires screen-scale lowering
 */
bool _visual_family_attr_is_screen_space(DvzVisualType type, const char* name)
{
    ANN(name);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    if (ops == NULL)
        return false;
    name = _visual_family_attr_storage_name(type, name);
    if (strcmp(name, "size") == 0)
        return !ops->size_attr_is_data_space;
    return strcmp(name, "line_width") == 0 || strcmp(name, "sigma") == 0;
}



/**
 * Return whether a visual family accepts continuous scales on scalar float color attributes.
 *
 * @param type the visual type
 * @return whether scalar color scales are supported
 */
bool _visual_family_supports_scalar_color_scale(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL && ops->supports_scalar_color_scale;
}



/**
 * Mix one observed revision into a non-authoritative diagnostic signature.
 *
 * @param hash current fingerprint
 * @param value observed identity or revision
 * @return updated fingerprint
 */
static uint64_t _visual_diagnostic_mix(uint64_t hash, uint64_t value)
{
    return (hash ^ value) * UINT64_C(1099511628211);
}



/**
 * Expose observed visual and family-owned resource revisions for diagnostics.
 *
 * This fingerprint is deliberately not a complete cache-validity contract. GPU writes and some
 * sampled-field changes have no matching CPU revision; unknown mutations remain unattributed.
 *
 * @param visual visual to inspect, or NULL
 * @return non-authoritative diagnostic fingerprint, or zero for NULL
 */
uint64_t _visual_family_diagnostic_signature(const DvzVisual* visual)
{
    if (visual == NULL)
        return 0;
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = _visual_diagnostic_mix(hash, visual->id);
    hash = _visual_diagnostic_mix(hash, visual->material.version);
    hash = _visual_diagnostic_mix(hash, visual->local_transform_version);
    hash = _visual_diagnostic_mix(hash, visual->attr_count);
    for (uint32_t ai = 0; ai < visual->attr_count; ai++)
        hash = _visual_diagnostic_mix(hash, visual->attrs[ai].version);
    const DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state != NULL && state->item_state_bank != NULL)
    {
        const DvzItemStateBuffer* bank = state->item_state_bank;
        if (bank->words != NULL)
            hash = _visual_diagnostic_mix(hash, bank->words->content_revision);
        if (bank->style != NULL)
            hash = _visual_diagnostic_mix(hash, bank->style->content_revision);
    }
    if (state != NULL && state->buffer != NULL)
        hash = _visual_diagnostic_mix(hash, state->buffer->content_revision);
    if (state != NULL)
        hash = _visual_diagnostic_mix(hash, state->texture.version);
    if (state != NULL && state->field != NULL && state->field->buffer != NULL)
        hash = _visual_diagnostic_mix(hash, state->field->buffer->content_revision);
    return hash;
}
