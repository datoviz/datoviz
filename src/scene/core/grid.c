/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene grid layout                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default grid track descriptor.
 *
 * @return a weight-based track with weight 1.0
 */
static DvzGridTrack _grid_track_default(void)
{
    return (DvzGridTrack){
        .mode = DVZ_GRID_SIZE_WEIGHT,
        .value = 1.0f,
    };
}




/**
 * Return whether one grid track size is valid.
 *
 * @param mode the size mode
 * @param value the mode value
 * @return whether the track descriptor is valid
 */
static bool _grid_track_valid(DvzGridSizeMode mode, float value)
{
    if (!isfinite(value))
        return false;
    if (mode == DVZ_GRID_SIZE_WEIGHT)
        return value > 0.0f;
    if (mode == DVZ_GRID_SIZE_FIXED_PX)
        return value >= 0.0f;
    return false;
}




/**
 * Return whether grid margins are valid.
 *
 * @param margins the margins
 * @return whether every margin is finite and non-negative
 */
static bool _grid_margins_valid(const DvzPanelReserve* margins)
{
    ANN(margins);
    return isfinite(margins->left_px) && margins->left_px >= 0.0f &&
           isfinite(margins->right_px) && margins->right_px >= 0.0f &&
           isfinite(margins->top_px) && margins->top_px >= 0.0f &&
           isfinite(margins->bottom_px) && margins->bottom_px >= 0.0f;
}




/**
 * Return whether one grid cell lies within the grid.
 *
 * @param grid the grid
 * @param cell the cell and span
 * @return whether the cell can be resolved
 */
static bool _grid_cell_valid(const DvzGrid* grid, DvzGridCell cell)
{
    ANN(grid);
    if (cell.row >= grid->rows || cell.col >= grid->cols)
        return false;
    if (cell.row_span == 0 || cell.col_span == 0)
        return false;
    if (cell.row_span > grid->rows - cell.row)
        return false;
    if (cell.col_span > grid->cols - cell.col)
        return false;
    return true;
}




/**
 * Mark one grid as requiring layout resolution.
 *
 * @param grid the grid
 */
static void _scene_grid_mark_dirty(DvzGrid* grid)
{
    if (grid == NULL)
        return;
    grid->dirty = true;
    if (grid->figure != NULL)
        _scene_notify_request_frame(grid->figure);
}




/**
 * Resolve one grid axis into pixel starts and sizes.
 *
 * @param tracks track descriptors
 * @param count number of tracks
 * @param extent total axis extent in logical pixels
 * @param leading leading margin in logical pixels
 * @param trailing trailing margin in logical pixels
 * @param gutter inter-track gutter in logical pixels
 * @param starts output track starts
 * @param sizes output track sizes
 * @return whether the axis was resolved
 */
static bool _grid_resolve_axis(
    const DvzGridTrack* tracks, uint32_t count, float extent, float leading, float trailing,
    float gutter, float* starts, float* sizes)
{
    ANN(tracks);
    ANN(starts);
    ANN(sizes);
    if (count == 0 || !isfinite(extent) || extent <= 0.0f || !isfinite(gutter) ||
        gutter < 0.0f)
    {
        return false;
    }

    float fixed_px = 0.0f;
    float weight = 0.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!_grid_track_valid(tracks[i].mode, tracks[i].value))
            return false;
        if (tracks[i].mode == DVZ_GRID_SIZE_FIXED_PX)
            fixed_px += tracks[i].value;
        else
            weight += tracks[i].value;
    }

    const float gutter_total = count > 1 ? gutter * (float)(count - 1) : 0.0f;
    const float content = extent - leading - trailing - gutter_total;
    const float remaining = content - fixed_px;
    if (!isfinite(content) || !isfinite(remaining) || content <= 0.0f || remaining < 0.0f)
        return false;
    if (weight <= 0.0f && remaining > 0.0f)
        return false;

    float cursor = leading;
    for (uint32_t i = 0; i < count; i++)
    {
        starts[i] = cursor;
        if (tracks[i].mode == DVZ_GRID_SIZE_FIXED_PX)
            sizes[i] = tracks[i].value;
        else
            sizes[i] = weight > 0.0f ? remaining * tracks[i].value / weight : 0.0f;
        if (!isfinite(sizes[i]) || sizes[i] < 0.0f)
            return false;
        cursor += sizes[i] + (i + 1 < count ? gutter : 0.0f);
    }
    return true;
}




/**
 * Create a retained grid layout object owned by a figure.
 *
 * @param figure the figure
 * @param rows number of rows
 * @param cols number of columns
 * @return the grid, or NULL
 */
DvzGrid* dvz_figure_grid(DvzFigure* figure, uint32_t rows, uint32_t cols)
{
    if (figure == NULL || rows == 0 || cols == 0 || rows > DVZ_SCENE_MAX_GRID_ROWS ||
        cols > DVZ_SCENE_MAX_GRID_COLS)
    {
        return NULL;
    }

    DvzGrid* grid = NULL;
    for (uint32_t i = 0; i < figure->grid_count; i++)
    {
        if (figure->grids[i].figure == NULL)
        {
            grid = &figure->grids[i];
            break;
        }
    }
    if (grid == NULL)
    {
        if (figure->grid_count >= DVZ_SCENE_MAX_GRIDS)
            return NULL;
        grid = &figure->grids[figure->grid_count++];
    }

    dvz_memset(grid, sizeof(DvzGrid), 0, sizeof(DvzGrid));
    grid->figure = figure;
    grid->rows = rows;
    grid->cols = cols;
    for (uint32_t row = 0; row < rows; row++)
        grid->row_sizes[row] = _grid_track_default();
    for (uint32_t col = 0; col < cols; col++)
        grid->col_sizes[col] = _grid_track_default();
    grid->dirty = true;
    return grid;
}


/**
 * Destroy a retained figure-owned grid layout object.
 *
 * @param grid the grid
 */
void dvz_grid_destroy(DvzGrid* grid)
{
    if (grid == NULL || grid->figure == NULL)
        return;

    DvzFigure* figure = grid->figure;
    for (uint32_t pi = 0; pi < grid->panel_count; pi++)
    {
        DvzPanel* panel = grid->panels[pi].panel;
        if (panel != NULL && panel->grid == grid)
        {
            panel->grid = NULL;
            panel->grid_cell = (DvzGridCell){0};
        }
    }

    dvz_memset(grid, sizeof(DvzGrid), 0, sizeof(DvzGrid));
    _scene_notify_request_frame(figure);
}




/**
 * Set fixed logical-pixel margins around one grid.
 *
 * @param grid the grid
 * @param margins grid margins, or NULL for zero margins
 * @return whether the margins were accepted
 */
bool dvz_grid_set_margins(DvzGrid* grid, const DvzPanelReserve* margins)
{
    if (grid == NULL)
        return false;
    DvzPanelReserve next = margins != NULL ? *margins : (DvzPanelReserve){0};
    if (!_grid_margins_valid(&next))
        return false;
    grid->margins = next;
    _scene_grid_mark_dirty(grid);
    return true;
}




/**
 * Set fixed logical-pixel gutters between grid columns and rows.
 *
 * @param grid the grid
 * @param x_px horizontal gutter in logical pixels
 * @param y_px vertical gutter in logical pixels
 * @return whether the gutters were accepted
 */
bool dvz_grid_set_gutter(DvzGrid* grid, float x_px, float y_px)
{
    if (grid == NULL || !isfinite(x_px) || !isfinite(y_px) || x_px < 0.0f || y_px < 0.0f)
        return false;
    grid->gutter_x_px = x_px;
    grid->gutter_y_px = y_px;
    _scene_grid_mark_dirty(grid);
    return true;
}




/**
 * Set one grid column size.
 *
 * @param grid the grid
 * @param col zero-based column index
 * @param mode size mode
 * @param value weight or fixed logical-pixel size
 * @return whether the size was accepted
 */
bool dvz_grid_col_size(DvzGrid* grid, uint32_t col, DvzGridSizeMode mode, float value)
{
    if (grid == NULL || col >= grid->cols || !_grid_track_valid(mode, value))
        return false;
    grid->col_sizes[col] = (DvzGridTrack){.mode = mode, .value = value};
    _scene_grid_mark_dirty(grid);
    return true;
}




/**
 * Set one grid row size.
 *
 * @param grid the grid
 * @param row zero-based row index
 * @param mode size mode
 * @param value weight or fixed logical-pixel size
 * @return whether the size was accepted
 */
bool dvz_grid_row_size(DvzGrid* grid, uint32_t row, DvzGridSizeMode mode, float value)
{
    if (grid == NULL || row >= grid->rows || !_grid_track_valid(mode, value))
        return false;
    grid->row_sizes[row] = (DvzGridTrack){.mode = mode, .value = value};
    _scene_grid_mark_dirty(grid);
    return true;
}




/**
 * Resolve one grid cell into a normalized figure-space panel rectangle.
 *
 * @param grid the grid
 * @param width figure width in logical pixels
 * @param height figure height in logical pixels
 * @param cell zero-based cell and span
 * @param out output normalized panel rectangle
 * @return whether the cell was resolved
 */
bool dvz_grid_resolve(
    const DvzGrid* grid, uint32_t width, uint32_t height, DvzGridCell cell, DvzPanelDesc* out)
{
    if (grid == NULL || out == NULL || width == 0 || height == 0 || !_grid_cell_valid(grid, cell))
        return false;

    float col_starts[DVZ_SCENE_MAX_GRID_COLS] = {0};
    float col_sizes[DVZ_SCENE_MAX_GRID_COLS] = {0};
    float row_starts[DVZ_SCENE_MAX_GRID_ROWS] = {0};
    float row_sizes[DVZ_SCENE_MAX_GRID_ROWS] = {0};

    if (!_grid_resolve_axis(
            grid->col_sizes, grid->cols, (float)width, grid->margins.left_px,
            grid->margins.right_px, grid->gutter_x_px, col_starts, col_sizes))
    {
        return false;
    }
    if (!_grid_resolve_axis(
            grid->row_sizes, grid->rows, (float)height, grid->margins.top_px,
            grid->margins.bottom_px, grid->gutter_y_px, row_starts, row_sizes))
    {
        return false;
    }

    const uint32_t last_col = cell.col + cell.col_span - 1;
    const uint32_t last_row = cell.row + cell.row_span - 1;
    const float x0 = col_starts[cell.col];
    const float y0 = row_starts[cell.row];
    const float x1 = col_starts[last_col] + col_sizes[last_col];
    const float y1 = row_starts[last_row] + row_sizes[last_row];

    *out = (DvzPanelDesc){
        .x = x0 / (float)width,
        .y = y0 / (float)height,
        .width = (x1 - x0) / (float)width,
        .height = (y1 - y0) / (float)height,
    };
    return out->width > 0.0f && out->height > 0.0f;
}




/**
 * Return whether one panel descriptor is finite and inside the figure.
 *
 * @param desc the descriptor
 * @return whether the descriptor is valid
 */
bool _panel_desc_valid(DvzPanelDesc desc)
{
    const float eps = 1e-6f;
    if (!isfinite(desc.x) || !isfinite(desc.y) || !isfinite(desc.width) ||
        !isfinite(desc.height))
    {
        return false;
    }
    if (desc.x < 0.0f || desc.y < 0.0f || desc.width <= 0.0f || desc.height <= 0.0f)
        return false;
    if (desc.x + desc.width > 1.0f + eps)
        return false;
    if (desc.y + desc.height > 1.0f + eps)
        return false;
    return true;
}




/**
 * Return whether two panel descriptors are effectively equal.
 *
 * @param a first descriptor
 * @param b second descriptor
 * @return whether the descriptors match
 */
static bool _panel_desc_equal(DvzPanelDesc a, DvzPanelDesc b)
{
    const float eps = 1e-6f;
    return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps &&
           fabsf(a.width - b.width) <= eps && fabsf(a.height - b.height) <= eps;
}




/**
 * Update a panel descriptor without changing grid ownership.
 *
 * @param panel the panel
 * @param desc the new descriptor
 * @return whether the descriptor was accepted
 */
bool _scene_panel_set_desc_internal(DvzPanel* panel, DvzPanelDesc desc)
{
    if (panel == NULL || !_panel_desc_valid(desc))
        return false;
    if (_panel_desc_equal(panel->desc, desc))
        return true;

    panel->desc = desc;
    if (panel->camera != NULL)
    {
        float panel_width = 0.0f;
        float panel_height = 0.0f;
        _scene_panel_pixel_size(panel, &panel_width, &panel_height);
        dvz_camera_resize(panel->camera, panel_width, panel_height);
    }
    (void)_scene_panel_refresh_border(panel);
    _panel_mark_layout_changed(panel);
    _scene_panel_view_dirty(panel);
    return true;
}




/**
 * Attach one panel to a retained grid cell.
 *
 * @param grid the grid
 * @param panel the panel
 * @param cell the retained cell attachment
 * @return whether the panel was attached
 */
static bool _scene_grid_attach_panel(DvzGrid* grid, DvzPanel* panel, DvzGridCell cell)
{
    if (grid == NULL || panel == NULL || grid->panel_count >= DVZ_SCENE_MAX_PANELS)
        return false;
    if (!_grid_cell_valid(grid, cell))
        return false;
    panel->grid = grid;
    panel->grid_cell = cell;
    grid->panels[grid->panel_count++] = (DvzGridPanel){.panel = panel, .cell = cell};
    _scene_grid_mark_dirty(grid);
    return true;
}


/**
 * Detach one panel from a retained grid attachment list.
 *
 * @param grid the grid
 * @param panel the panel
 * @return whether an attachment was removed
 */
bool _scene_grid_detach_panel(DvzGrid* grid, DvzPanel* panel)
{
    if (grid == NULL || panel == NULL)
        return false;

    bool removed = false;
    uint32_t dst = 0;
    for (uint32_t src = 0; src < grid->panel_count; src++)
    {
        DvzGridPanel attachment = grid->panels[src];
        if (attachment.panel == panel)
        {
            removed = true;
            continue;
        }
        if (dst != src)
            grid->panels[dst] = attachment;
        dst++;
    }
    for (uint32_t i = dst; i < grid->panel_count; i++)
        grid->panels[i] = (DvzGridPanel){0};
    grid->panel_count = dst;

    if (panel->grid == grid)
    {
        panel->grid = NULL;
        panel->grid_cell = (DvzGridCell){0};
        removed = true;
    }

    if (removed)
        _scene_grid_mark_dirty(grid);
    return removed;
}




/**
 * Recompute retained grid-owned panel rectangles for one figure.
 *
 * @param figure the figure
 * @return whether all active grid-owned panels were resolved
 */
bool _scene_figure_resolve_layouts(DvzFigure* figure)
{
    if (figure == NULL)
        return false;
    bool ok = true;
    for (uint32_t gi = 0; gi < figure->grid_count; gi++)
    {
        DvzGrid* grid = &figure->grids[gi];
        if (grid->figure != figure)
            continue;
        for (uint32_t pi = 0; pi < grid->panel_count; pi++)
        {
            DvzGridPanel* attachment = &grid->panels[pi];
            DvzPanel* panel = attachment->panel;
            if (panel == NULL || panel->figure != figure || panel->grid != grid)
                continue;

            DvzPanelDesc desc = {0};
            if (!dvz_grid_resolve(
                    grid, figure->width, figure->height, attachment->cell, &desc) ||
                !_scene_panel_set_desc_internal(panel, desc))
            {
                ok = false;
            }
        }
        if (ok)
            grid->dirty = false;
    }
    return ok;
}




/**
 * Create a grid-owned panel spanning contiguous cells.
 *
 * @param grid the grid
 * @param row zero-based origin row index
 * @param col zero-based origin column index
 * @param row_span number of rows covered by the panel
 * @param col_span number of columns covered by the panel
 * @return the panel, or NULL
 */
DvzPanel* dvz_grid_panel_span(
    DvzGrid* grid, uint32_t row, uint32_t col, uint32_t row_span, uint32_t col_span)
{
    if (grid == NULL || grid->figure == NULL)
        return NULL;
    DvzGridCell cell = {
        .row = row,
        .col = col,
        .row_span = row_span,
        .col_span = col_span,
    };
    DvzPanelDesc desc = {0};
    if (!dvz_grid_resolve(grid, grid->figure->width, grid->figure->height, cell, &desc))
        return NULL;

    DvzPanel* panel = dvz_panel(grid->figure, desc);
    if (panel == NULL)
        return NULL;
    if (!_scene_grid_attach_panel(grid, panel, cell))
    {
        dvz_panel_destroy(panel);
        return NULL;
    }
    return panel;
}




/**
 * Create a grid-owned panel for one cell.
 *
 * @param grid the grid
 * @param row zero-based row index
 * @param col zero-based column index
 * @return the panel, or NULL
 */
DvzPanel* dvz_grid_panel(DvzGrid* grid, uint32_t row, uint32_t col)
{
    return dvz_grid_panel_span(grid, row, col, 1, 1);
}
