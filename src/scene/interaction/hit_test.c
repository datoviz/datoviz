/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query coordinates                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "_assertions.h"
#include "_scene.h"
#include "query/internal.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Shift an apply MVP so one NDC coordinate becomes the centered request target.
 *
 * @param mvp the MVP to update
 * @param ndc the NDC delta
 */
static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc)
{
    ANN(mvp);
    mvp->proj[3][0] -= ndc[0];
    mvp->proj[3][1] -= ndc[1];
}



/*************************************************************************************************/
/*  Panel coordinates                                                                            */
/*************************************************************************************************/

/**
 * Convert a panel-local request coordinate to NDC.
 *
 * @param figure the figure
 * @param panel the panel
 * @param x the panel-local x coordinate
 * @param y the panel-local y coordinate
 * @param out_ndc the output NDC coordinate
 * @return true when the request is inside the panel
 */
bool _scene_query_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc)
{
    ANN(figure);
    ANN(panel);
    ANN(out_ndc);
    if (figure->width == 0 || figure->height == 0)
        return false;

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;

    double px = x / panel_width;
    double py = y / panel_height;
    if (px < 0.0 || px > 1.0 || py < 0.0 || py > 1.0)
        return false;

    out_ndc[0] = (float)(2.0 * px - 1.0);
    out_ndc[1] = (float)(1.0 - 2.0 * py);
    return true;
}



/**
 * Resolve the stable public panel id within one figure.
 *
 * @param figure the figure
 * @param panel the panel
 * @return the 1-based public panel id, or 1 when not found
 */
uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel)
{
    ANN(figure);
    ANN(panel);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        if (&figure->panels[pi] == panel)
            return (uint64_t)pi + 1;
    }
    return 1;
}



/**
 * Build an apply MVP that recenters one panel-local request onto the readback pixel.
 *
 * Image sample queries read back one pixel from a synthetic 1x1 target. The request coordinate is
 * therefore shifted onto the target center.
 *
 * @param panel the panel
 * @param request_ndc the requested panel-local NDC coordinate
 * @param out the destination MVP
 */
void _scene_request_apply_mvp(const DvzPanel* panel, const vec2 request_ndc, DvzMVP* out)
{
    ANN(panel);
    ANN(request_ndc);
    ANN(out);
    _scene_panel_apply_mvp(panel, out);
    vec2 target_ndc = {0.0f, 0.0f};
    vec2 delta = {request_ndc[0] - target_ndc[0], -request_ndc[1] - target_ndc[1]};
    _scene_center_apply_mvp(out, delta);
}



/**
 * Build a request MVP including a visual-local model transform.
 *
 * @param panel panel receiving the query
 * @param visual visual being queried, or NULL
 * @param request_ndc request coordinate in panel-local NDC
 * @param out destination MVP
 */
void _scene_request_visual_mvp(
    const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc, DvzMVP* out)
{
    ANN(panel);
    ANN(request_ndc);
    ANN(out);
    _scene_request_apply_mvp(panel, request_ndc, out);
    if (visual != NULL && visual->has_local_transform)
    {
        mat4 local = GLM_MAT4_IDENTITY_INIT;
        mat4 composed = GLM_MAT4_IDENTITY_INIT;
        for (uint32_t col = 0; col < 4; col++)
        {
            for (uint32_t row = 0; row < 4; row++)
                local[col][row] = visual->local_transform[col][row];
        }
        glm_mat4_mul(out->model, local, composed);
        glm_mat4_copy(composed, out->model);
    }
}
