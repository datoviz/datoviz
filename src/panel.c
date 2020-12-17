#include "../include/visky/panel.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _update_panel(VklPanel* panel)
{
    ASSERT(panel != NULL);
    VklGrid* grid = panel->grid;

    if (panel->mode == VKL_PANEL_INSET)
        return;

    ASSERT(panel->mode == VKL_PANEL_GRID);

    panel->x = grid->xs[panel->col];
    panel->y = grid->ys[panel->row];
    panel->width = grid->widths[panel->col];
    panel->height = grid->heights[panel->row];
}



static void _update_grid_panels(VklGrid* grid, VklGridAxis axis)
{
    ASSERT(grid != NULL);

    bool h = axis == VKL_GRID_HORIZONTAL;
    uint32_t n = h ? grid->n_rows : grid->n_cols;
    float total = 0.0f;

    for (uint32_t i = 0; i < n; i++)
    {
        float s = h ? grid->widths[i] : grid->heights[i];
        if (s == 0.0f)
            s = 1.0f / n;
        ASSERT(s > 0);
        if (h)
        {
            grid->xs[i] = total;
            grid->widths[i] = s;
        }
        else
        {
            grid->ys[i] = total;
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
    for (uint32_t i = 0; i < grid->panel_count; i++)
        _update_panel(&grid->panels[i]);

    // NOTE: not sure if this is needed? Decommenting causes the command buffers to be recorded
    // twice.
    // vkl_canvas_to_refill(grid->canvas, true);

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



static VklPanel* _get_panel(VklGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);
    VklPanel* panel = NULL;
    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        panel = &grid->panels[i];
        if (panel->row == row && panel->col == col)
            return panel;
    }
    return NULL;
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

    _update_grid_panels(&grid, VKL_GRID_HORIZONTAL);
    _update_grid_panels(&grid, VKL_GRID_VERTICAL);

    return grid;
}



VklPanel* vkl_panel(VklGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);

    VklPanel* panel = _get_panel(grid, row, col);
    if (panel != NULL)
        return panel;

    panel = &grid->panels[grid->panel_count++];

    panel->obj.type = VKL_OBJECT_TYPE_PANEL;
    obj_created(&panel->obj);

    panel->grid = grid;
    panel->row = row;
    panel->col = col;

    // NOTE: for now just use a single command buffer, as using multiple command buffers
    // is complicated as need to use mupltiple render passes and framebuffers.
    panel->cmds = grid->canvas->commands;
    // Tag the VklCommands instance with the panel index, so that the REFILL callback knows
    // which VklCommands corresponds to which panel.
    // panel->cmds = vkl_canvas_commands(
    //     grid->canvas, VKL_DEFAULT_QUEUE_RENDER, VKL_COMMANDS_GROUP_PANELS, grid->panel_count -
    //     1);

    _update_panel(panel);
    return panel;
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
    VklCanvas* canvas = panel->grid->canvas;

    float win_width = panel->grid->canvas->swapchain.images->width;
    float win_height = panel->grid->canvas->swapchain.images->height;

    viewport.size_screen[0] = panel->width * canvas->window->width;
    viewport.size_screen[1] = panel->height * canvas->window->height;

    viewport.offset_screen[0] = panel->x * canvas->window->width;
    viewport.offset_screen[1] = panel->y * canvas->window->height;

    viewport.viewport.x = panel->x * win_width;
    viewport.viewport.y = panel->y * win_height;
    viewport.viewport.width = panel->width * win_width;
    viewport.viewport.height = panel->height * win_height;

    return viewport;
}



VklPanel* vkl_panel_at(VklGrid* grid, vec2 pos)
{
    ASSERT(grid != NULL);
    VklPanel* panel = NULL;
    float x = pos[0];
    float y = pos[1];

    for (uint32_t i = 0; i < grid->panel_count; i++)
    {
        panel = &grid->panels[i];
        if (panel->x <= x && x <= panel->x + panel->width && //
            panel->y <= y && y <= panel->y + panel->height)
            return panel;
    }

    return panel;
}



void vkl_panel_destroy(VklPanel* panel)
{

    ASSERT(panel != NULL);
    // TODO
}
