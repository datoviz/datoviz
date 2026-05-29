/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lookup helpers */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_family.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"
#include "registry/registry.h"
#include "sample_profile.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the scene-global index of a figure visual.
 *
 * @param figure the figure
 * @param visual the visual
 * @param out_index output visual index
 * @return true when the visual belongs to the figure scene
 */
bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index)
{
    ANN(out_index);
    *out_index = 0;
    if (figure == NULL || figure->scene == NULL || visual == NULL)
        return false;
    if (visual->scene != figure->scene)
        return false;
    for (uint32_t i = 0; i < figure->scene->visual_count; i++)
    {
        if (&figure->scene->visuals[i] == visual)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}



/*************************************************************************************************/
/*  Visual names                                                                                 */
/*************************************************************************************************/

/**
 * Return the debug name of one visual type.
 *
 * @param type the visual type
 * @return the visual type name
 */
const char* _visual_type_name(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL && ops->name != NULL ? ops->name : "unknown";
}
