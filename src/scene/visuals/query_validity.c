/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scientific query validity inputs                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../query/internal.h"
#include "_alloc.h"
#include "_visual_internal.h"
#include "scene_emit/internal.h"
#include <math.h>

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve the position-independent alpha factor used by material-shaded visuals.
 *
 * @param visual retained visual
 * @return base-color alpha times clamped material opacity
 */
static float _scene_query_material_alpha(const DvzVisual* visual)
{
    return visual->material.base_color_factor[3] *
           fmaxf(0.0f, fminf(1.0f, visual->material.opacity));
}

/**
 * Prepare effective alpha from the visual upload's retained color lowering.
 *
 * @param visual retained visual
 * @param count source vertex count
 * @param material whether the visible family evaluates material alpha
 * @param out_alpha output owned alpha array
 * @return whether the bounded input array was prepared
 */
bool _scene_query_color_alpha(
    const DvzVisual* visual, uint64_t count, bool material, float** out_alpha)
{
    const DvzVisualAttr* color = NULL;
    if (!_dvz_scene_query_dense_attr(visual, "color", sizeof(DvzColor), &color) ||
        color->item_count != count || count > SIZE_MAX / sizeof(DvzColor) ||
        count > SIZE_MAX / sizeof(float))
        return false;
    DvzColor* colors = dvz_calloc(count, sizeof(DvzColor));
    float* alpha = dvz_calloc(count, sizeof(float));
    if (colors == NULL || alpha == NULL ||
        !_scene_visual_color_rgba(visual, color, 0, count, colors))
    {
        dvz_free(colors);
        dvz_free(alpha);
        return false;
    }
    float factor = material ? _scene_query_material_alpha(visual) : 1.0f;
    for (uint64_t i = 0; i < count; i++)
        alpha[i] = (float)colors[i].a / 255.0f * factor;
    dvz_free(colors);
    *out_alpha = alpha;
    return true;
}

/**
 * Check that retained nonempty query inputs have bounded CPU upload payloads.
 *
 * @param visual retained visual
 * @param names normalized required input names
 * @param count input name count
 * @return whether inputs can be uploaded by the supported query profile
 */
bool _scene_query_dense_inputs_supported(
    const DvzVisual* visual, const char* const* names, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        int index = _attr_index(visual, names[i]);
        if (index >= 0 && visual->attrs[index].item_count > 0 &&
            (visual->attrs[index].data == NULL || visual->attrs[index].item_count > UINT32_MAX))
            return false;
    }
    return true;
}
