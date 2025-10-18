/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Grid                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/grid.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DEFAULT_SCALE     1.0f
#define DEFAULT_LINEWIDTH 0.01f
#define DEFAULT_COLOR     {0.5f, 0.5f, 0.5f, 1.0f}
#define DEFAULT_ELEVATION 0



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_grid(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Shaders.
    dvz_visual_shader(visual, "grid");

    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);

    // Common slots.
    _common_setup(visual);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT); // binding = 2

    // Grid parameters.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzGridParams));
    ANN(params);

    // Param attributes.
    dvz_params_attr(params, 0, FIELD(DvzGridParams, color));
    dvz_params_attr(params, 1, FIELD(DvzGridParams, linewidth));
    dvz_params_attr(params, 2, FIELD(DvzGridParams, scale));
    dvz_params_attr(params, 3, FIELD(DvzGridParams, elevation));

    // Default parameter values.
    dvz_visual_param(visual, 2, 0, (vec4[]){DEFAULT_COLOR});
    dvz_visual_param(visual, 2, 1, (float[]){DEFAULT_LINEWIDTH});
    dvz_visual_param(visual, 2, 2, (float[]){DEFAULT_SCALE});
    dvz_visual_param(visual, 2, 3, (float[]){DEFAULT_ELEVATION});

    dvz_visual_alloc(visual, 6, 6, 0);

    return visual;
}



void dvz_grid_color(DvzVisual* visual, vec4 value)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, &value);
}



void dvz_grid_linewidth(DvzVisual* visual, float value)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, &value);
}



void dvz_grid_scale(DvzVisual* visual, float value)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 2, &value);
}



void dvz_grid_elevation(DvzVisual* visual, float value)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 3, &value);
}



void dvz_grid_destroy(DvzVisual* visual)
{
    ANN(visual);
    dvz_visual_destroy(visual);
}
