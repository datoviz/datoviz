#include "../include/visky/panel.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklMVP MVP_ID = {
    GLM_MAT4_IDENTITY_INIT, //
    GLM_MAT4_IDENTITY_INIT, //
    GLM_MAT4_IDENTITY_INIT};



static void _check_viewport(VklViewport* viewport)
{
    ASSERT(viewport != NULL);

    ASSERT(viewport->size_screen[0] > 0);
    ASSERT(viewport->size_screen[1] > 0);
    ASSERT(viewport->size_framebuffer[0] > 0);
    ASSERT(viewport->size_framebuffer[1] > 0);
    ASSERT(viewport->viewport.width > 0);
    ASSERT(viewport->viewport.height > 0);
    ASSERT(viewport->dpi_scaling > 0);
}



static void _update_viewport(VklPanel* panel)
{
    VklCanvas* canvas = panel->grid->canvas;
    ASSERT(canvas != NULL);

    VklViewport* viewport = &panel->viewport;

    // Size in screen coordinates.
    viewport->size_screen[0] = panel->width * canvas->window->width;
    viewport->size_screen[1] = panel->height * canvas->window->height;
    viewport->offset_screen[0] = panel->x * canvas->window->width;
    viewport->offset_screen[1] = panel->y * canvas->window->height;

    // Size in framebuffer pixel coordinates.
    float win_width = panel->grid->canvas->swapchain.images->width;
    float win_height = panel->grid->canvas->swapchain.images->height;

    viewport->offset_framebuffer[0] = viewport->viewport.x = panel->x * win_width;
    viewport->offset_framebuffer[1] = viewport->viewport.y = panel->y * win_height;
    viewport->size_framebuffer[0] = viewport->viewport.width = panel->width * win_width;
    viewport->size_framebuffer[1] = viewport->viewport.height = panel->height * win_height;

    _check_viewport(viewport);
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
    VklPanel* panel = vkl_container_iter(&grid->panels);
    while (panel != NULL)
    {
        vkl_panel_update(panel);
        panel = vkl_container_iter(&grid->panels);
    }
    // NOTE: not sure if this is needed? Decommenting causes the command buffers to be recorded
    // twice.
    // vkl_canvas_to_refill(grid->canvas, true);
    //
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
    VklPanel* panel = vkl_container_iter(&grid->panels);
    while (panel != NULL)
    {
        if (panel->row == row && panel->col == col)
            return panel;
        panel = vkl_container_iter(&grid->panels);
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
    grid.panels = vkl_container(VKL_MAX_PANELS, sizeof(VklPanel), VKL_OBJECT_TYPE_PANEL);

    _update_grid_panels(&grid, VKL_GRID_HORIZONTAL);
    _update_grid_panels(&grid, VKL_GRID_VERTICAL);

    return grid;
}



void vkl_grid_destroy(VklGrid* grid)
{
    ASSERT(grid != NULL);
    vkl_container_destroy(&grid->panels);
}



VklPanel* vkl_panel(VklGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);
    VklCanvas* canvas = grid->canvas;
    ASSERT(canvas != NULL);
    // VklContext* ctx = canvas->gpu->context;

    VklPanel* panel = _get_panel(grid, row, col);
    if (panel != NULL)
        return panel;

    panel = vkl_container_alloc(&grid->panels);
    obj_created(&panel->obj);

    panel->grid = grid;
    panel->row = row;
    panel->col = col;
    panel->hspan = 1;
    panel->vspan = 1;

    panel->viewport.dpi_scaling = VKL_DEFAULT_DPI_SCALING;

    // NOTE: for now just use a single command buffer, as using multiple command buffers
    // is complicated since we need to use multiple render passes and framebuffers.
    panel->cmds = &grid->canvas->cmds_render;
    // Tag the VklCommands instance with the panel index, so that the REFILL callback knows
    // which VklCommands corresponds to which panel.
    // panel->cmds = vkl_canvas_commands(
    //     grid->canvas, VKL_DEFAULT_QUEUE_RENDER, VKL_COMMANDS_GROUP_PANELS, grid->panel_count -
    //     1);

    // MVP uniform buffer.
    uint32_t n = canvas->swapchain.img_count;
    panel->br_mvp = vkl_ctx_buffers(
        canvas->gpu->context, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, n, sizeof(VklMVP));
    // Initialize with identity matrices. Will be later updated by the scene controllers at every
    // frame.
    vkl_upload_buffers_immediate(canvas, panel->br_mvp, true, 0, panel->br_mvp.size, &MVP_ID);

    // Update the VklViewport.
    vkl_panel_update(panel);
    return panel;
}



void vkl_panel_update(VklPanel* panel)
{
    ASSERT(panel != NULL);
    VklGrid* grid = panel->grid;
    // VklContext* ctx = grid->canvas->gpu->context;

    if (panel->mode == VKL_PANEL_GRID)
    {
        panel->x = grid->xs[panel->col];
        panel->y = grid->ys[panel->row];
        panel->width = grid->widths[panel->col] * panel->hspan;
        panel->height = grid->heights[panel->row] * panel->vspan;
    }
    // Update the viewport structures.
    _update_viewport(panel);

    // NOTE: it is up to the scene to update the VklViewport struct on the GPU, for each visual
}



void vkl_panel_dpi_scaling(VklPanel* panel, float scaling)
{
    ASSERT(panel != NULL);
    scaling = CLIP(scaling, .1, 100);
    ASSERT(scaling > 0);
    panel->viewport.dpi_scaling = scaling;
    vkl_panel_update(panel);
}



void vkl_panel_margins(VklPanel* panel, vec4 margins)
{
    ASSERT(panel != NULL);
    glm_vec4_copy(margins, panel->viewport.margins);
    vkl_panel_update(panel);
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



void vkl_panel_visual(VklPanel* panel, VklVisual* visual)
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

    vkl_panel_update(panel);
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
    // vkl_canvas_to_refill(panel->grid->canvas, true);
}



void vkl_panel_span(VklPanel* panel, VklGridAxis axis, uint32_t span)
{
    ASSERT(panel != NULL);
    if (axis == VKL_GRID_HORIZONTAL)
        panel->hspan = span;
    else if (axis == VKL_GRID_VERTICAL)
        panel->vspan = span;

    _update_grid_panels(panel->grid, axis);
    // vkl_canvas_to_refill(panel->grid->canvas, true);
}



void vkl_panel_cell(VklPanel* panel, uint32_t row, uint32_t col)
{
    ASSERT(panel != NULL);
    panel->row = row;
    panel->col = col;

    _update_grid_panels(panel->grid, VKL_GRID_HORIZONTAL);
    _update_grid_panels(panel->grid, VKL_GRID_VERTICAL);
    // vkl_canvas_to_refill(panel->grid->canvas, true);
}



VklViewport vkl_panel_viewport(VklPanel* panel)
{
    ASSERT(panel != NULL);
    return panel->viewport;
}



VklPanel* vkl_panel_at(VklGrid* grid, vec2 pos)
{
    ASSERT(grid != NULL);
    float x = pos[0];
    float y = pos[1];

    VklPanel* panel = vkl_container_iter(&grid->panels);
    while (panel != NULL)
    {
        if (panel->x <= x && x <= panel->x + panel->width && //
            panel->y <= y && y <= panel->y + panel->height)
            return panel;
        panel = vkl_container_iter(&grid->panels);
    }
    return NULL;
}



void vkl_panel_destroy(VklPanel* panel)
{
    ASSERT(panel != NULL);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        vkl_visual_destroy(panel->visuals[i]);
    }
    obj_destroyed(&panel->obj);
}
