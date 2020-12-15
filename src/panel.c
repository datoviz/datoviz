#include "../include/visky/panel.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _update_grid_panels(VklGrid* grid, VklGridAxis axis)
{
    ASSERT(grid != NULL);

    bool h = axis == VKL_GRID_HORIZONTAL;
    uint32_t n = h ? grid->n_rows : grid->n_cols;
    float total = 0.0f;

    float xs[VKL_GRID_MAX_COLS] = {0};
    float ys[VKL_GRID_MAX_ROWS] = {0};

    for (uint32_t i = 0; i < n; i++)
    {
        float s = h ? grid->widths[i] : grid->heights[i];
        if (s == 0.0f)
            s = 1.0f / n;
        ASSERT(s > 0);
        if (h)
        {
            xs[i] = total;
            grid->widths[i] = s;
        }
        else
        {
            ys[i] = total;
            grid->heights[i] = s;
        }
        total += s;
    }
    // Renormalize in [0, 1].
    for (uint32_t i = 0; i < n; i++)
    {
        if (h)
            grid->widths[i] /= total;
        else
            grid->heights[i] /= total;
    }

    // Update the panel positions and sizes.
    VklPanel* panel = NULL;
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        panel = &grid->panels[i];
        if (panel->mode == VKL_PANEL_INSET)
            continue;
        ASSERT(panel->mode == VKL_PANEL_GRID);
        panel->x = xs[panel->col];
        panel->y = ys[panel->row];
        panel->width = grid->widths[panel->col];
        panel->height = grid->heights[panel->row];
    }

    // TODO: hide panels that are overlapped by hspan/vspan-ed panels
    // by setting STATUS_INACTIVE (with log warn)
}



static float _to_normalized_unit(VklPanel* panel, VklGridAxis axis, float size)
{
    ASSERT(panel != NULL);
    VklCanvas* canvas = panel->grid->canvas;
    bool h = axis == VKL_GRID_HORIZONTAL;
    switch (panel->size_unit)
    {
    case VKL_PANEL_UNIT_NORMALIZED:
        return size;
        break;
    case VKL_PANEL_UNIT_FRAMEBUFFER:
        return size / (h ? canvas->swapchain.images->width : canvas->swapchain.images->height);
        break;
    case VKL_PANEL_UNIT_SCREEN:
        return size / (h ? canvas->window->width : canvas->window->height);
        break;
    default:
        break;
    }
    log_error("unable to convert size to normalized unit");
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklGrid vkl_grid(VklCanvas* canvas, uint32_t rows, uint32_t cols)
{
    ASSERT(canvas != NULL);
    VklGrid grid = {0};
    grid.canvas = canvas;
    grid.n_rows = rows;
    grid.n_cols = cols;
    return grid;
}



VklPanel* vkl_panel(VklGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);
    VklPanel panel = {0};
    panel.grid = grid;
    panel.row = row;
    panel.col = col;
    panel.cmds = vkl_commands(
        grid->canvas->gpu, VKL_DEFAULT_QUEUE_RENDER, grid->canvas->swapchain.img_count);
    grid->panels[grid->panel_count++] = panel;
    return &grid->panels[grid->panel_count - 1];
}



void vkl_panel_unit(VklPanel* panel, VklPanelSizeUnit unit)
{
    ASSERT(panel != NULL);
    panel->size_unit = unit;
}



void vkl_panel_mode(VklPanel* panel, VklPanelMode mode)
{

    ASSERT(panel != NULL);
    panel->mode = mode;
}



void vkl_panel_visual(VklPanel* panel, VklVisual* visual, VklViewportType viewport)
{

    ASSERT(panel != NULL);
    ASSERT(visual != NULL);
    panel->visuals[panel->visual_count++] = visual;
}



void vkl_panel_pos(VklPanel* panel, float x, float y)
{
    ASSERT(panel != NULL);
    panel->mode = VKL_PANEL_INSET;
    panel->x = x;
    panel->y = y;

    _update_grid_panels(panel->grid, VKL_GRID_HORIZONTAL);
    _update_grid_panels(panel->grid, VKL_GRID_VERTICAL);
}



void vkl_panel_size(VklPanel* panel, VklGridAxis axis, float size)
{
    ASSERT(panel != NULL);

    if (panel->mode == VKL_PANEL_INSET)
    {
        if (axis == VKL_GRID_HORIZONTAL)
            panel->width = size;

        else if (axis == VKL_GRID_VERTICAL)
            panel->height = size;
    }

    else if (panel->mode == VKL_PANEL_GRID)
    {
        VklGrid* grid = panel->grid;

        // The grid widths and heights are always in normalized coordinates.
        size = _to_normalized_unit(panel, axis, size);
        if (axis == VKL_GRID_HORIZONTAL)
            grid->widths[panel->col] = size;

        else if (axis == VKL_GRID_VERTICAL)
            grid->heights[panel->row] = size;

        _update_grid_panels(grid, axis);
    }
}



void vkl_panel_span(VklPanel* panel, VklGridAxis axis, uint32_t span)
{
    ASSERT(panel != NULL);
    if (axis == VKL_GRID_HORIZONTAL)
        panel->hspan = span;
    else if (axis == VKL_GRID_VERTICAL)
        panel->vspan = span;

    _update_grid_panels(panel->grid, axis);
}



void vkl_panel_cell(VklPanel* panel, uint32_t row, uint32_t col)
{
    ASSERT(panel != NULL);
    panel->row = row;
    panel->col = col;

    _update_grid_panels(panel->grid, VKL_GRID_HORIZONTAL);
    _update_grid_panels(panel->grid, VKL_GRID_VERTICAL);
}



VklViewport vkl_panel_viewport(VklPanel* panel)
{

    ASSERT(panel != NULL);
    VklViewport viewport = {0};
    float win_width = panel->grid->canvas->swapchain.images->width;
    float win_height = panel->grid->canvas->swapchain.images->height;
    viewport.viewport.x = panel->x * win_width;
    viewport.viewport.y = panel->y * win_height;
    viewport.viewport.width = panel->width * win_width;
    viewport.viewport.height = panel->height * win_height;
    return viewport;
}



void vkl_panel_destroy(VklPanel* panel)
{

    ASSERT(panel != NULL);
    // TODO
}
