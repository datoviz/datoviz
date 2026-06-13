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
#include <string.h>

#include "_assertions.h"
#include "axis_internal.h"



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
 * Return a conservative approximate text extent for reserve calculations.
 *
 * @param text UTF-8 text
 * @param size font size in logical pixels
 * @return approximate horizontal text extent in logical pixels
 */
static float _axis_text_extent_px(const char* text, float size)
{
    if (text == NULL || text[0] == '\0')
        return 0.0f;
    return 0.62f * size * (float)strlen(text);
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
    if (axis->tick_count > 0)
    {
        for (uint32_t i = 0; i < axis->tick_count; i++)
        {
            tick_extent = fmaxf(
                tick_extent,
                tick_gap + _axis_text_extent_px(axis->text_labels[i], tick_size));
        }
    }
    else
    {
        tick_extent += tick_gap + 4.0f * tick_size;
    }

    float label_extent = 0.0f;
    if (axis->label[0] != '\0')
    {
        if (axis->dim == DVZ_DIM_X)
            label_extent = label_gap + label_size;
        else
            label_extent = label_gap + _axis_text_extent_px(axis->label, label_size);
    }

    return tick_extent + label_extent + 4.0f;
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
    if (x_axis->panel == panel)
        reserve.bottom_px = _axis_reserve_px(x_axis);
    if (y_axis->panel == panel)
        reserve.left_px = _axis_reserve_px(y_axis);
    _scene_panel_set_axis_reserve(panel, &reserve);
}
