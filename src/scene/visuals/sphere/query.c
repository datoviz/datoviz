/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sphere query policy                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../../query/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return sphere visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_sphere_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "sphere",
        .family = DVZ_SCENE_VISUAL_FAMILY_SPHERE,
        .pick_capabilities = DVZ_PICK_CAPABILITY_OBJECT | DVZ_PICK_CAPABILITY_ITEM,
    };
    return &ops;
}
