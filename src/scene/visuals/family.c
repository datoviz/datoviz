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
