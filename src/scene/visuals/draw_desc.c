/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual draw descriptors                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve draw-count metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_visual_default_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualDrawDesc), 0, sizeof(DvzSceneVisualDrawDesc));

    out->vertex_count = visual->vertex_count;
    out->instance_count = visual->instance_count;
    out->index_buffer_id = visual->index_buffer_id;
    out->index_format = visual->index_format;
    out->index_count = visual->index_count;
    out->indexed = visual->index_buffer_id != 0;
    return true;
}


/**
 * Resolve draw-count metadata through the visual-family registry.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualDrawDesc), 0, sizeof(DvzSceneVisualDrawDesc));

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)visual->visual_type);
    if (ops == NULL || ops->resolve_draw_desc == NULL)
        return false;
    return ops->resolve_draw_desc(visual, out);
}
