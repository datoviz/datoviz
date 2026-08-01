/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene frame demand                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "controller_internal.h"
#include "frame_demand_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one fly controller still needs frame updates.
 *
 * @param fly the fly payload
 * @return whether the fly payload is active
 */
bool _scene_fly_active(const DvzFly* fly)
{
    if (fly == NULL)
        return false;
    return fly->key_forward || fly->key_backward || fly->key_left || fly->key_right ||
           fly->key_up || fly->key_down || fly->interacting || fly->pivot_marker_time_left > 0.0;
}



/**
 * Return demand contributed by one active controller binding.
 *
 * @param panel the bound panel
 * @param controller the controller binding
 * @return the controller frame demand
 */
static DvzFrameDemandFlags
_scene_panel_controller_frame_demand(const DvzPanel* panel, const DvzController* controller)
{
    if (panel == NULL || controller == NULL || !controller->active ||
        controller->scene != panel->figure->scene)
    {
        return DVZ_FRAME_DEMAND_NONE;
    }

    switch (controller->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        return panel->panzoom == controller->panzoom && controller->panzoom != NULL &&
                       controller->panzoom->interacting
                   ? DVZ_FRAME_DEMAND_INTERACTION
                   : DVZ_FRAME_DEMAND_NONE;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        return panel->arcball == controller->arcball && controller->arcball != NULL &&
                       controller->arcball->interacting
                   ? DVZ_FRAME_DEMAND_INTERACTION
                   : DVZ_FRAME_DEMAND_NONE;
    case DVZ_CONTROLLER_TYPE_FLY:
        return panel->fly == controller->fly && _scene_fly_active(controller->fly)
                   ? DVZ_FRAME_DEMAND_INTERACTION
                   : DVZ_FRAME_DEMAND_NONE;
    case DVZ_CONTROLLER_TYPE_TURNTABLE:
        return panel->turntable == controller->turntable && controller->turntable != NULL &&
                       controller->turntable->interacting
                   ? DVZ_FRAME_DEMAND_INTERACTION
                   : DVZ_FRAME_DEMAND_NONE;
    default:
        return DVZ_FRAME_DEMAND_NONE;
    }
}



/**
 * Return the ongoing frame demand of controllers bound to one figure.
 *
 * @param figure the figure
 * @return the aggregated frame demand flags
 */
DvzFrameDemandFlags _scene_figure_frame_demand(const DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return DVZ_FRAME_DEMAND_NONE;

    DvzFrameDemandFlags demand = DVZ_FRAME_DEMAND_NONE;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        const DvzPanel* panel = &figure->panels[i];
        if (panel->figure != figure)
            continue;
        for (uint32_t dim = 0; dim < 3; dim++)
        {
            demand |= _scene_panel_controller_frame_demand(panel, panel->controllers[dim]);
        }
    }
    return demand;
}
