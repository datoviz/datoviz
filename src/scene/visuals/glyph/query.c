/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Glyph query policy                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../../query/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return glyph visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_glyph_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "glyph",
        .family = DVZ_SCENE_VISUAL_FAMILY_GLYPH,
    };
    return &ops;
}
