/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas window surface helpers                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include "_assertions.h"
#include "_log.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Refresh the cached window surface pointer.
 *
 * @param canvas canvas whose window surface should be refreshed
 */
void dvz_canvas_window_surface_refresh(DvzCanvas* canvas)
{
    ANN(canvas);
    if (!canvas->window)
    {
        canvas->surface = NULL;
        return;
    }
    canvas->surface = dvz_window_surface(canvas->window);
}



/**
 * Return a snapshot describing the cached surface.
 *
 * @param canvas canvas whose surface information is requested
 * @returns a filled surface info structure
 */
DvzCanvasSurfaceInfo dvz_canvas_window_surface_info(const DvzCanvas* canvas)
{
    ANN(canvas);
    DvzCanvasSurfaceInfo info = {
        .extent = {0, 0},
        .format = canvas->cfg.color_format,
        .scale_x = 1.0f,
        .scale_y = 1.0f,
    };
    if (canvas->surface)
    {
        info.extent = canvas->surface->extent;
        info.format = canvas->surface->format;
        info.scale_x = canvas->surface->scale_x;
        info.scale_y = canvas->surface->scale_y;
    }
    return info;
}
