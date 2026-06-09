/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene placement helpers                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a rectangle has finite positive dimensions.
 *
 * @param rect rectangle descriptor
 * @return whether the rectangle is valid
 */
static bool _placement_rect_valid(const DvzRect* rect)
{
    return rect != NULL && isfinite(rect->x) && isfinite(rect->y) && isfinite(rect->width) &&
           isfinite(rect->height) && rect->width > 0.0f && rect->height > 0.0f;
}


/**
 * Return whether a placement descriptor is finite and sized.
 *
 * @param placement placement descriptor
 * @return whether the placement is valid
 */
static bool _placement_valid(const DvzPlacement* placement)
{
    return placement != NULL &&
           (placement->space == DVZ_PLACEMENT_SPACE_PANEL ||
            placement->space == DVZ_PLACEMENT_SPACE_FIGURE) &&
           placement->horizontal_anchor <= DVZ_HORIZONTAL_ANCHOR_RIGHT &&
           placement->vertical_anchor <= DVZ_VERTICAL_ANCHOR_BOTTOM &&
           isfinite(placement->offset_x_px) && isfinite(placement->offset_y_px) &&
           isfinite(placement->width_px) && isfinite(placement->height_px) &&
           placement->width_px >= 0.0f && placement->height_px >= 0.0f;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default panel-local placement descriptor.
 *
 * @return default placement descriptor
 */
DvzPlacement dvz_placement(void)
{
    return (DvzPlacement){
        .space = DVZ_PLACEMENT_SPACE_PANEL,
        .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_LEFT,
        .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
    };
}


/**
 * Return a fixed-size panel-corner placement descriptor.
 *
 * @param horizontal horizontal panel anchor
 * @param vertical vertical panel anchor
 * @param width_px widget width in logical pixels
 * @param height_px widget height in logical pixels
 * @param offset_x_px horizontal offset from the anchor in logical pixels
 * @param offset_y_px vertical offset from the anchor in logical pixels
 * @return placement descriptor
 */
DvzPlacement dvz_placement_panel_corner(
    DvzHorizontalAnchor horizontal, DvzVerticalAnchor vertical, float width_px, float height_px,
    float offset_x_px, float offset_y_px)
{
    DvzPlacement placement = dvz_placement();
    placement.horizontal_anchor = horizontal;
    placement.vertical_anchor = vertical;
    placement.width_px = width_px;
    placement.height_px = height_px;
    placement.offset_x_px = offset_x_px;
    placement.offset_y_px = offset_y_px;
    return placement;
}


/**
 * Resolve a placement to a panel-local rectangle.
 *
 * @param placement placement descriptor
 * @param panel_rect panel rectangle in figure pixels
 * @param figure_rect figure rectangle in figure pixels, or NULL to use the panel rectangle
 * @param out output panel-local rectangle
 * @return whether the placement could be resolved
 */
bool dvz_placement_resolve(
    const DvzPlacement* placement, const DvzRect* panel_rect, const DvzRect* figure_rect,
    DvzRect* out)
{
    if (!_placement_valid(placement) || !_placement_rect_valid(panel_rect) || out == NULL)
        return false;

    const DvzRect fallback_figure = *panel_rect;
    const DvzRect* figure = figure_rect != NULL ? figure_rect : &fallback_figure;
    if (!_placement_rect_valid(figure))
        return false;

    DvzRect space = {0.0f, 0.0f, panel_rect->width, panel_rect->height};
    if (placement->space == DVZ_PLACEMENT_SPACE_FIGURE)
    {
        space.x = figure->x - panel_rect->x;
        space.y = figure->y - panel_rect->y;
        space.width = figure->width;
        space.height = figure->height;
    }

    float x = space.x + placement->offset_x_px;
    if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_CENTER)
        x = space.x + 0.5f * (space.width - placement->width_px) + placement->offset_x_px;
    else if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_RIGHT)
        x = space.x + space.width - placement->width_px + placement->offset_x_px;

    float y = space.y + placement->offset_y_px;
    if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_CENTER)
        y = space.y + 0.5f * (space.height - placement->height_px) + placement->offset_y_px;
    else if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_BOTTOM)
        y = space.y + space.height - placement->height_px + placement->offset_y_px;

    *out = (DvzRect){
        .x = x,
        .y = y,
        .width = placement->width_px,
        .height = placement->height_px,
    };
    return true;
}
