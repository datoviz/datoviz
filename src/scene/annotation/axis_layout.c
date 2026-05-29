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
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
    return 0.0f;
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
