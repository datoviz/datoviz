/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Primitive query policy                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../../query/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return primitive visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_primitive_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "primitive",
        .family = DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE,
        .pick_capabilities = DVZ_PICK_CAPABILITY_OBJECT | DVZ_PICK_CAPABILITY_ITEM |
                             DVZ_PICK_CAPABILITY_VERTEX | DVZ_PICK_CAPABILITY_FACE,
    };
    return &ops;
}
