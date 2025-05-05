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

#define DEFAULT_FONT_SIZE 18
#define DEFAULT_MARGIN    100



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

DvzAxes* dvz_axes_2D(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzAxes* axes = (DvzAxes*)calloc(1, sizeof(DvzAxes));
    ANN(axes);

    float font_size = DEFAULT_FONT_SIZE;
    dvz_atlas_font(font_size, &axes->af);
    axes->axis[DVZ_DIM_X] = dvz_axis(batch, &axes->af, DVZ_DIM_X, flags);
    axes->axis[DVZ_DIM_Y] = dvz_axis(batch, &axes->af, DVZ_DIM_Y, flags);

    DvzAxis* xaxis = axes->axis[DVZ_DIM_X];
    DvzAxis* yaxis = axes->axis[DVZ_DIM_Y];

    ANN(xaxis);
    ANN(yaxis);

    return axes;
}



void dvz_axes_resize(DvzAxes* axes, DvzView* view)
{
    ANN(axes);
    ANN(view);

    DvzAxis* xaxis = dvz_axes_axis(axes, DVZ_DIM_X);
    DvzAxis* yaxis = dvz_axes_axis(axes, DVZ_DIM_Y);

    ANN(xaxis);
    ANN(yaxis);

    float top = view->margins[0];
    float right = view->margins[1];
    float bottom = view->margins[2];
    float left = view->margins[3];

    // Sizes.
    double glyph_size = axes->af.font_size;
    ASSERT(glyph_size > 0);

    // x axis.
    {
        double range_size = view->shape[0] - left - right;
        if (range_size < 10 * glyph_size)
        {
            log_warn("axes range size too small");
            range_size = glyph_size = 1;
        }
        dvz_axis_size(xaxis, range_size, glyph_size);
    }

    // y axis.
    {
        double range_size = view->shape[1] - top - bottom;
        if (range_size < 10 * glyph_size)
        {
            log_warn("axes range size too small");
            range_size = glyph_size = 1;
        }
        dvz_axis_size(yaxis, range_size, glyph_size);
    }

    // log_info("resize %f %f", range_size, glyph_size);
}



DvzAxes* dvz_panel_axes_2D(DvzPanel* panel, double xmin, double xmax, double ymin, double ymax)
{
    ANN(panel);

    // Get or create the DvzAxes.
    if (panel->axes == NULL)
    {
        ANN(panel->figure);
        ANN(panel->figure->scene);

        DvzBatch* batch = panel->figure->scene->batch;
        ANN(batch);

        // NOTE: ensure we create a panzoom before adding the axes visuals, otherwise a mock
        // transform will be automatically added and we won't be able to add a panzoom transform
        // anymore.
        dvz_panel_panzoom(panel);

        panel->axes = dvz_axes_2D(batch, 0); // TODO: flags
    }
    ANN(panel->axes);

    DvzAxis* xaxis = dvz_axes_axis(panel->axes, DVZ_DIM_X);
    DvzAxis* yaxis = dvz_axes_axis(panel->axes, DVZ_DIM_Y);

    ANN(xaxis);
    ANN(yaxis);

    // Get the DvzRef.
    DvzRef* ref = dvz_panel_ref(panel);

    // Set the ref extent.
    dvz_ref_set(ref, DVZ_DIM_X, xmin, xmax);
    dvz_ref_set(ref, DVZ_DIM_Y, ymin, ymax);

    float m = DEFAULT_MARGIN;
    dvz_panel_margins(panel, 0, 0, m, m);

    // Set the margins.
    dvz_axes_resize(panel->axes, panel->view);

    // X axis parameters.
    dvz_axis_horizontal(xaxis, 0);
    dvz_axis_label(xaxis, "Axis", 10, DVZ_ORIENTATION_DEFAULT);

    // Y axis parameters.
    dvz_axis_vertical(yaxis, 0);
    // dvz_axis_label(yaxis, "Axis", 10, DVZ_ORIENTATION_DEFAULT);

    // NOTE: by default there should be a default ref with NDC bounds

    dvz_axis_update(xaxis, ref, xmin, xmax);
    dvz_axis_update(yaxis, ref, ymin, ymax);

    // Add the two DvzAxis to the panel.
    dvz_axis_panel(xaxis, panel);
    dvz_axis_panel(yaxis, panel);

    // TODO: grid

    /*
    resize callback
    check vulkan warning on destruction if pipeline not used therefore not created
    customizable default axis spec
    */

    return panel->axes;
}



DvzAxis* dvz_axes_axis(DvzAxes* axes, DvzDim dim)
{
    ANN(axes);
    ASSERT((uint32_t)dim < DVZ_DIM_COUNT);
    return axes->axis[(uint32_t)dim];
}



void dvz_axes_update(DvzAxes* axes, DvzRef* ref, DvzPanzoom* panzoom, bool force)
{
    ANN(axes);
    ANN(ref);

    DvzAxis* xaxis = dvz_axes_axis(axes, DVZ_DIM_X);
    DvzAxis* yaxis = dvz_axes_axis(axes, DVZ_DIM_Y);

    ANN(xaxis);
    ANN(yaxis);

    if (panzoom != NULL)
    {
        dvz_axis_on_panzoom(xaxis, panzoom, ref, force);
        dvz_axis_on_panzoom(yaxis, panzoom, ref, force);
    }
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
        if (axes->axis[i] != NULL)
        {
            dvz_axis_destroy(axes->axis[i]);
        }
    }
    FREE(axes);
}
