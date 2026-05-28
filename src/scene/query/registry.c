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
