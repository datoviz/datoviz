/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query family registry                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "internal.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef const DvzSceneQueryFamilyOps* (*DvzSceneQueryFamilyOpsFn)(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const DvzSceneQueryFamilyOpsFn QUERY_FAMILY_OPS[] = {
    _dvz_scene_query_point_ops,     _dvz_scene_query_pixel_ops,
    _dvz_scene_query_marker_ops,    _dvz_scene_query_sphere_ops,
    _dvz_scene_query_vector_ops,    _dvz_scene_query_segment_ops,
    _dvz_scene_query_path_ops,      _dvz_scene_query_primitive_ops,
    _dvz_scene_query_mesh_ops,      _dvz_scene_query_image_ops,
    _dvz_scene_query_labels_ops,    _dvz_scene_query_volume_ops,
    _dvz_scene_query_text_ops,      _dvz_scene_query_glyph_ops,
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the query family corresponding to one retained visual type.
 *
 * @param type retained visual type
 * @return query visual family, or NONE when there is no query family
 */
static DvzSceneVisualFamily _query_registry_family_for_visual_type(DvzVisualType type)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return DVZ_SCENE_VISUAL_FAMILY_POINT;
    case DVZ_VISUAL_TYPE_PIXEL:
        return DVZ_SCENE_VISUAL_FAMILY_PIXEL;
    case DVZ_VISUAL_TYPE_MARKER:
        return DVZ_SCENE_VISUAL_FAMILY_MARKER;
    case DVZ_VISUAL_TYPE_SPHERE:
        return DVZ_SCENE_VISUAL_FAMILY_SPHERE;
    case DVZ_VISUAL_TYPE_VECTOR:
        return DVZ_SCENE_VISUAL_FAMILY_VECTOR;
    case DVZ_VISUAL_TYPE_SEGMENT:
        return DVZ_SCENE_VISUAL_FAMILY_SEGMENT;
    case DVZ_VISUAL_TYPE_PATH:
        return DVZ_SCENE_VISUAL_FAMILY_PATH;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE;
    case DVZ_VISUAL_TYPE_MESH:
        return DVZ_SCENE_VISUAL_FAMILY_MESH;
    case DVZ_VISUAL_TYPE_IMAGE:
        return DVZ_SCENE_VISUAL_FAMILY_IMAGE;
    case DVZ_VISUAL_TYPE_LABELS:
        return DVZ_SCENE_VISUAL_FAMILY_LABELS;
    case DVZ_VISUAL_TYPE_VOLUME:
        return DVZ_SCENE_VISUAL_FAMILY_VOLUME;
    case DVZ_VISUAL_TYPE_TEXT:
        return DVZ_SCENE_VISUAL_FAMILY_TEXT;
    case DVZ_VISUAL_TYPE_GLYPH:
        return DVZ_SCENE_VISUAL_FAMILY_GLYPH;
    default:
        return DVZ_SCENE_VISUAL_FAMILY_NONE;
    }
}



/**
 * Return the number of registered query visual families.
 *
 * @return registry entry count
 */
uint32_t _dvz_scene_query_registry_count(void)
{
    return DVZ_ARRAY_COUNT(QUERY_FAMILY_OPS);
}



/**
 * Return one registered query family operation table.
 *
 * @param index registry index
 * @return the family ops, or NULL when index is out of bounds
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_get(uint32_t index)
{
    if (index >= DVZ_ARRAY_COUNT(QUERY_FAMILY_OPS))
        return NULL;
    return QUERY_FAMILY_OPS[index]();
}



/**
 * Find a registered query family operation table.
 *
 * @param family public scene visual family
 * @return the family ops, or NULL when the family is not registered
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find(DvzSceneVisualFamily family)
{
    for (uint32_t i = 0; i < _dvz_scene_query_registry_count(); i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_get(i);
        if (ops != NULL && ops->family == family)
            return ops;
    }
    return NULL;
}



/**
 * Find the query family operation table for one retained visual type.
 *
 * @param type retained visual type
 * @return the family ops, or NULL when the type has no registered query family
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find_visual_type(DvzVisualType type)
{
    DvzSceneVisualFamily family = _query_registry_family_for_visual_type(type);
    if (family == DVZ_SCENE_VISUAL_FAMILY_NONE)
        return NULL;
    return _dvz_scene_query_registry_find(family);
}
