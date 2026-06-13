/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene 2D axis layout reserve                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include "_assertions.h"
#include "axis_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define AXIS_EDGE_RESERVE_PX 18.0f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve one axis style size with a default fallback.
 *
 * @param value configured size
 * @param fallback fallback size
 * @return resolved size in logical pixels
 */
static float _axis_style_size(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}



/**
 * Return one axis reserve contribution in logical pixels.
 *
 * @param axis the axis
 * @return reserve contribution in pixels
 */
static float _axis_reserve_px(const DvzAxis* axis)
{
    ANN(axis);
    if (!axis->enabled)
        return 0.0f;
    if (axis->style.reserve_px > 0.0f && isfinite(axis->style.reserve_px))
        return axis->style.reserve_px;

    float tick_gap = _axis_style_size(axis->style.tick_gap_px, AXIS_TEXT_TICK_GAP);
    float label_gap = _axis_style_size(axis->style.label_gap_px, AXIS_TEXT_LABEL_GAP);
    float tick_size = _axis_style_size(axis->style.tick_size_px, AXIS_TEXT_TICK_SIZE);
    float label_size = _axis_style_size(axis->style.label_size_px, AXIS_TEXT_LABEL_SIZE);
    float major_tick_length = _axis_style_size(axis->style.major_tick_length, 0.0f);
    float minor_tick_length = _axis_style_size(axis->style.minor_tick_length, 0.0f);
    float tick_length = fmaxf(major_tick_length, minor_tick_length);

    float tick_extent = 0.0f;
    if (axis->style.show_major_ticks || axis->style.show_minor_ticks)
        tick_extent = tick_length;
    if (axis->dim == DVZ_DIM_X)
        tick_extent = fmaxf(tick_extent, tick_gap + tick_size);
    else
        tick_extent = fmaxf(tick_extent, tick_gap + 2.25f * tick_size);

    float label_extent = 0.0f;
    if (axis->label[0] != '\0')
        label_extent = label_gap + label_size;

    return tick_extent + label_extent + 4.0f;
}


static bool _axis_has_explicit_reserve(const DvzAxis* axis)
{
    ANN(axis);
    return axis->style.reserve_px > 0.0f && isfinite(axis->style.reserve_px);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Refresh aggregate attached axis reserve for one panel.
 *
 * @param panel the panel
 */
void _scene_panel_refresh_axis_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzPanelReserve reserve = {0};
    DvzAxis* x_axis = &panel->axes[DVZ_DIM_X];
    DvzAxis* y_axis = &panel->axes[DVZ_DIM_Y];
    if (x_axis->panel == panel && x_axis->enabled)
    {
        float edge = _axis_has_explicit_reserve(x_axis) ? 0.0f : AXIS_EDGE_RESERVE_PX;
        reserve.bottom_px = _axis_reserve_px(x_axis) + edge;
        reserve.right_px = fmaxf(reserve.right_px, edge);
    }
    if (y_axis->panel == panel && y_axis->enabled)
    {
        float edge = _axis_has_explicit_reserve(y_axis) ? 0.0f : AXIS_EDGE_RESERVE_PX;
        reserve.left_px = _axis_reserve_px(y_axis) + edge;
        reserve.top_px = fmaxf(reserve.top_px, edge);
    }
    _scene_panel_set_axis_reserve(panel, &reserve);
}
