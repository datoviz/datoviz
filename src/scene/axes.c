/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/axes.h"
#include "_cglm.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_types.h"
#include "scene/axis.h"
#include "scene/ref.h"
#include "scene/scene.h"
#include "scene/ticks.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

DvzAxes*
dvz_axes_2D(DvzPanel* panel, double xmin, double xmax, double ymin, double ymax, int flags)
{
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);
    ANN(panel->figure->scene);

    DvzBatch* batch = panel->figure->scene->batch;
    ANN(batch);

    DvzAxes* axes = (DvzAxes*)calloc(1, sizeof(DvzAxes));
    ANN(axes);

    float font_size = 18;
    axes->af = dvz_atlas_font(font_size);
    axes->axis[DVZ_DIM_X] = dvz_axis(batch, &axes->af, DVZ_DIM_X, 0);
    axes->axis[DVZ_DIM_Y] = dvz_axis(batch, &axes->af, DVZ_DIM_Y, 0);

    DvzAxis* xaxis = axes->axis[DVZ_DIM_X];
    DvzAxis* yaxis = axes->axis[DVZ_DIM_Y];

    ANN(xaxis);
    ANN(yaxis);

    // Panel ref.
    // WARNING: the axis also has a pointer to ref
    DvzRef* ref = dvz_panel_ref(panel, 0);
    dvz_ref_set(ref, DVZ_DIM_X, xmin, xmax);
    dvz_ref_set(ref, DVZ_DIM_Y, ymin, ymax);

    // Margins.
    float m = 100;
    dvz_panel_margins(panel, m, m, m, m);

    double range_size = panel->view->shape[0] - 2 * m;
    double glyph_size = font_size;

    dvz_axis_size(xaxis, range_size, glyph_size);
    dvz_axis_horizontal(xaxis, 0);
    dvz_axis_label(xaxis, "Axis", 10, DVZ_ORIENTATION_DEFAULT);

    // NOTE: by default there should be a default ref with NDC bounds

    dvz_axis_update(xaxis, xmin, xmax);
    dvz_axis_update(yaxis, ymin, ymax);

    dvz_axis_panel(xaxis, panel);
    dvz_axis_panel(yaxis, panel);

    // TODO: grid

    /*
    resize callback
    check vulkan warning on destruction if pipeline not used therefore not created
    customizable default axis spec
    */

    return axes;
}



DvzAxis* dvz_axes_axis(DvzAxes* axes, DvzDim dim)
{
    ANN(axes);
    ASSERT((uint32_t)dim < DVZ_DIM_COUNT);
    return axes->axis[(uint32_t)dim];
}



void dvz_axes_update(DvzAxes* axes)
{
    ANN(axes);
    /*
    // TODO
    dvz_axis_size()
    dvz_axis_update(dim) for each dim
    when command buffer is being rebuilt, call axis_update()
    */
}



void dvz_axes_destroy(DvzAxes* axes)
{
    ANN(axes);

    // Destroy the font atlas.
    dvz_font_destroy(axes->af.font);
    dvz_atlas_destroy(axes->af.atlas);

    // Destroy the axes that were created by dvz_axes().
    for (uint32_t i = 0; i < DVZ_DIM_COUNT; i++)
    {
        if (axes->axis[i])
        {
            dvz_axis_destroy(axes->axis[i]);
        }
    }
    FREE(axes);
}
