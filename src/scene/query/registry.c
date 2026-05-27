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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_QUERY_FAMILY_COUNT 13



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the number of registered query visual families.
 *
 * @return registry entry count
 */
uint32_t _dvz_scene_query_registry_count(void)
{
    return DVZ_SCENE_QUERY_FAMILY_COUNT;
}



/**
 * Return one registered query family operation table.
 *
 * @param index registry index
 * @return the family ops, or NULL when index is out of bounds
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_get(uint32_t index)
{
    switch (index)
    {
    case 0:
        return _dvz_scene_query_point_ops();
    case 1:
        return _dvz_scene_query_pixel_ops();
    case 2:
        return _dvz_scene_query_marker_ops();
    case 3:
        return _dvz_scene_query_sphere_ops();
    case 4:
        return _dvz_scene_query_segment_ops();
    case 5:
        return _dvz_scene_query_path_ops();
    case 6:
        return _dvz_scene_query_primitive_ops();
    case 7:
        return _dvz_scene_query_mesh_ops();
    case 8:
        return _dvz_scene_query_image_ops();
    case 9:
        return _dvz_scene_query_labels_ops();
    case 10:
        return _dvz_scene_query_volume_ops();
    case 11:
        return _dvz_scene_query_text_ops();
    case 12:
        return _dvz_scene_query_glyph_ops();
    default:
        return NULL;
    }
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
