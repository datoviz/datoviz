/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family registry                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "registry/registry.h"

#include "_assertions.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {DVZ_VISUAL_TYPE_POINT, "point"},
    {DVZ_VISUAL_TYPE_PIXEL, "pixel"},
    {DVZ_VISUAL_TYPE_MARKER, "marker"},
    {DVZ_VISUAL_TYPE_SEGMENT, "segment"},
    {DVZ_VISUAL_TYPE_PATH, "path"},
    {DVZ_VISUAL_TYPE_IMAGE, "image"},
    {DVZ_VISUAL_TYPE_MESH, "mesh"},
    {DVZ_VISUAL_TYPE_VOLUME, "volume"},
    {DVZ_VISUAL_TYPE_PRIMITIVE, "primitive"},
    {DVZ_VISUAL_TYPE_SPHERE, "sphere"},
    {DVZ_VISUAL_TYPE_GLYPH, "glyph"},
    {DVZ_VISUAL_TYPE_TEXT, "text"},
    {DVZ_VISUAL_TYPE_LABELS, "labels"},
    {DVZ_VISUAL_TYPE_SPLAT, "splat"},
    {DVZ_VISUAL_TYPE_VECTOR, "vector"},
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the registered visual-family operations for one visual type.
 *
 * @param type visual type
 * @return registered operations, or NULL when the type is not active
 */
const DvzVisualFamilyOps* _scene_visual_family_ops(DvzVisualType type)
{
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS); i++)
    {
        if (VISUAL_FAMILY_OPS[i].type == type)
            return &VISUAL_FAMILY_OPS[i];
    }
    return NULL;
}



/**
 * Return the number of registered visual-family operation records.
 *
 * @return registered operation count
 */
uint32_t _scene_visual_family_ops_count(void)
{
    return DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS);
}



/**
 * Return one registered visual-family operation record by registry index.
 *
 * @param index registry index
 * @return registered operations, or NULL when the index is invalid
 */
const DvzVisualFamilyOps* _scene_visual_family_ops_at(uint32_t index)
{
    if (index >= DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS))
        return NULL;
    return &VISUAL_FAMILY_OPS[index];
}



/**
 * Return whether one visual type has registered visual-family operations.
 *
 * @param type visual type
 * @return whether the type has registered operations
 */
bool _scene_visual_family_ops_registered(DvzVisualType type)
{
    return _scene_visual_family_ops(type) != NULL;
}
