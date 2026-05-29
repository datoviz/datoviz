/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Glyph visual API                                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a glyph visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_glyph(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_GLYPH, flags);
    if (visual != NULL)
    {
        _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        _visual_family_state(visual)->glyph_atlas_encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
        _visual_family_state(visual)->glyph_distance_range_px = 4.0f;
    }
    return visual;
}
