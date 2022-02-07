#include "../include/datoviz/panel.h"
#include "../src/interact_utils.h"
#include "../src/transforms_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _check_viewport(DvzViewport* viewport)
{
    ASSERT(viewport != NULL);

    ASSERT(viewport->size_screen[0] > 0);
    ASSERT(viewport->size_screen[1] > 0);
    ASSERT(viewport->size_framebuffer[0] > 0);
    ASSERT(viewport->size_framebuffer[1] > 0);
    ASSERT(viewport->viewport.width > 0);
    ASSERT(viewport->viewport.height > 0);
    ASSERT(viewport->viewport.minDepth < viewport->viewport.maxDepth);
}



static void _update_viewport(DvzPanel* panel)
{
    DvzCanvas* canvas = panel->grid->canvas;
    ASSERT(canvas != NULL);

    DvzViewport* viewport = &panel->viewport;
    ASSERT(viewport != NULL);

    ASSERT(panel->width > 0);
    ASSERT(panel->height > 0);

    // Canvas size in screen pixels.
    uvec2 size_screen = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size_screen);
    ASSERT(size_screen[0] > 0);
    ASSERT(size_screen[1] > 0);

    // Size in screen coordinates.
    viewport->size_screen[0] = panel->width * size_screen[0];
    viewport->size_screen[1] = panel->height * size_screen[1];
    viewport->offset_screen[0] = panel->x * size_screen[0];
    viewport->offset_screen[1] = panel->y * size_screen[1];

    // Canvas size in framebuffer pixels.
    uvec2 size_framebuffer = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size_framebuffer);
    ASSERT(size_framebuffer[0] > 0);
    ASSERT(size_framebuffer[1] > 0);

    viewport->offset_framebuffer[0] = viewport->viewport.x = panel->x * size_framebuffer[0];
    viewport->offset_framebuffer[1] = viewport->viewport.y = panel->y * size_framebuffer[1];
    viewport->size_framebuffer[0] = viewport->viewport.width = panel->width * size_framebuffer[0];
    viewport->size_framebuffer[1] = viewport->viewport.height =
        panel->height * size_framebuffer[1];
    viewport->viewport.minDepth = 0;
    viewport->viewport.maxDepth = 1;

    _check_viewport(viewport);

    // Mark the panel as changed.
    panel->obj.request = DVZ_VISUAL_REQUEST_UPLOAD;
}



static double _array_sum(uint32_t count, double* array)
{
    double total = 0.0f;
    for (uint32_t i = 0; i < count; i++)
        total += array[i];
    return total;
}



static void _update_grid_panels(DvzGrid* grid, DvzGridAxis axis)
{
    ASSERT(grid != NULL);

    bool h = axis == DVZ_GRID_HORIZONTAL;
    uint32_t n = h ? grid->n_cols : grid->n_rows;
    double total = 0.0f;

    for (uint32_t i = 0; i < n; i++)
    {
        double s = h ? grid->widths[i] : grid->heights[i];
        if (s == 0.0f)
            s = 1.0f;
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

    // // Renormalize in [0, 1].
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     if (h)
    //     {
    //         grid->xs[i] /= total;
    //         grid->widths[i] /= total;
    //     }
    //     else
    //     {
    //         grid->ys[i] /= total;
    //         grid->heights[i] /= total;
    //     }
    // }

    // Update the panel positions and sizes.
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzPanel* panel = NULL;
    while (iter.item != NULL)
    {
        panel = iter.item;
        dvz_panel_update(panel);
        dvz_container_iter(&iter);
    }
    // NOTE: not sure if this is needed? Decommenting causes the command buffers to be recorded
    // twice.
    // dvz_canvas_to_refill(grid->canvas);
    //
    // TODO: hide panels that are overlapped by hspan/vspan-ed panels
    // by setting STATUS_INACTIVE (with log warn)
}



static float _to_normalized_unit(DvzPanel* panel, DvzGridAxis axis, float size)
{
    ASSERT(panel != NULL);
    DvzCanvas* canvas = panel->grid->canvas;
    bool h = axis == DVZ_GRID_HORIZONTAL;
    uvec2 wh = {0};

    switch (panel->size_unit)
    {
    case DVZ_PANEL_UNIT_NORMALIZED:
        return size;
        break;
    case DVZ_PANEL_UNIT_FRAMEBUFFER:

        // Canvas size in framebuffer pixels.
        dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, wh);
        ASSERT(wh[0] > 0);
        ASSERT(wh[1] > 0);

        return size / (h ? wh[0] : wh[1]);
        break;
    case DVZ_PANEL_UNIT_SCREEN:

        // Canvas size in screen pixels.
        dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, wh);
        ASSERT(wh[0] > 0);
        ASSERT(wh[1] > 0);

        return size / (h ? wh[0] : wh[1]);
        break;
    default:
        break;
    }
    log_error("unable to convert size to normalized unit");
    return 0;
}



static DvzPanel* _get_panel(DvzGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzPanel* panel = NULL;
    while (iter.item != NULL)
    {
        panel = iter.item;
        if (panel->row == row && panel->col == col)
            return panel;
        dvz_container_iter(&iter);
    }
    return NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzGrid dvz_grid(DvzCanvas* canvas, uint32_t row_count, uint32_t col_count)
{
    ASSERT(canvas != NULL);
    DvzGrid grid = {0};
    grid.canvas = canvas;
    grid.n_rows = row_count;
    grid.n_cols = col_count;
    grid.panels = dvz_container(DVZ_MAX_PANELS, sizeof(DvzPanel), DVZ_OBJECT_TYPE_PANEL);

    _update_grid_panels(&grid, DVZ_GRID_HORIZONTAL);
    _update_grid_panels(&grid, DVZ_GRID_VERTICAL);

    return grid;
}



void dvz_grid_destroy(DvzGrid* grid)
{
    ASSERT(grid != NULL);
    dvz_container_destroy(&grid->panels);
}



DvzPanel* dvz_panel(DvzGrid* grid, uint32_t row, uint32_t col)
{
    ASSERT(grid != NULL);
    DvzCanvas* canvas = grid->canvas;
    ASSERT(canvas != NULL);
    DvzContext* ctx = canvas->gpu->context;

    ASSERT(row < grid->n_rows);
    ASSERT(col < grid->n_cols);

    DvzPanel* panel = _get_panel(grid, row, col);
    if (panel != NULL)
        return panel;

    panel = dvz_container_alloc(&grid->panels);
    dvz_obj_created(&panel->obj);

    panel->grid = grid;
    panel->row = row;
    panel->col = col;
    panel->hspan = 1;
    panel->vspan = 1;
    panel->mode = DVZ_PANEL_GRID;

    // Default data coords.
    for (uint32_t i = 0; i < 3; i++)
    {
        panel->data_coords.box.p0[i] = -1;
        panel->data_coords.box.p1[i] = +1;
    }
    panel->data_coords.transform = DVZ_TRANSFORM_CARTESIAN;
    panel->data_coords.transpose = DVZ_CDS_TRANSPOSE_NONE;

    // NOTE: for now just use a single command buffer, as using multiple command buffers
    // is complicated since we need to use multiple render passes and framebuffers.
    panel->cmds = &grid->canvas->cmds_render;
    // Tag the DvzCommands instance with the panel index, so that the REFILL callback knows
    // which DvzCommands corresponds to which panel.
    // panel->cmds = dvz_canvas_commands(
    //     grid->canvas, DVZ_DEFAULT_QUEUE_RENDER, DVZ_COMMANDS_GROUP_PANELS, grid->panel_count -
    //     1);

    // MVP uniform buffer.
    uint32_t n = canvas->swapchain.img_count;
    panel->br_mvp = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, n, sizeof(DvzMVP));
    // Initialize with identity matrices. Will be later updated by the scene controllers at every
    // frame.
    // dvz_canvas_buffers(canvas, panel->br_mvp, 0, panel->br_mvp.size, &MVP_ID);

    // Update the DvzViewport.
    dvz_panel_update(panel);
    return panel;
}



void dvz_panel_update(DvzPanel* panel)
{
    ASSERT(panel != NULL);

    DvzGrid* grid = panel->grid;
    ASSERT(grid != NULL);

    ASSERT(panel->col < grid->n_cols);
    ASSERT(panel->row < grid->n_rows);

    if (panel->mode == DVZ_PANEL_GRID)
    {
        double wtot = _array_sum(grid->n_cols, grid->widths);
        double htot = _array_sum(grid->n_rows, grid->heights);

        panel->x = grid->xs[panel->col] / wtot;
        panel->y = grid->ys[panel->row] / htot;
        panel->width = grid->widths[panel->col] * panel->hspan / wtot;
        panel->height = grid->heights[panel->row] * panel->vspan / htot;
    }
    // Update the viewport structures.
    _update_viewport(panel);

    // NOTE: it is up to the scene to update the DvzViewport struct on the GPU, for each visual
}



void dvz_panel_margins(DvzPanel* panel, vec4 margins)
{
    ASSERT(panel != NULL);
    glm_vec4_copy(margins, panel->viewport.margins);
    dvz_panel_update(panel);
}



void dvz_panel_unit(DvzPanel* panel, DvzPanelSizeUnit unit)
{
    ASSERT(panel != NULL);
    panel->size_unit = unit;
}



void dvz_panel_mode(DvzPanel* panel, DvzPanelMode mode)
{

    ASSERT(panel != NULL);
    panel->mode = mode;
}



void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual)
{

    ASSERT(panel != NULL);
    ASSERT(visual != NULL);
    panel->visuals[panel->visual_count++] = visual;
}



void dvz_panel_pos(DvzPanel* panel, float x, float y)
{
    ASSERT(panel != NULL);
    panel->mode = DVZ_PANEL_INSET;
    panel->x = x;
    panel->y = y;

    dvz_panel_update(panel);
}



void dvz_panel_size(DvzPanel* panel, DvzGridAxis axis, float size)
{
    ASSERT(panel != NULL);

    if (panel->mode == DVZ_PANEL_INSET)
    {
        if (axis == DVZ_GRID_HORIZONTAL)
            panel->width = size;

        else if (axis == DVZ_GRID_VERTICAL)
            panel->height = size;
    }

    else if (panel->mode == DVZ_PANEL_GRID)
    {
        DvzGrid* grid = panel->grid;

        // The grid widths and heights are always in normalized coordinates.
        size = _to_normalized_unit(panel, axis, size);

        if (axis == DVZ_GRID_HORIZONTAL)
            grid->widths[panel->col] = size;

        else if (axis == DVZ_GRID_VERTICAL)
            grid->heights[panel->row] = size;

        _update_grid_panels(grid, axis);
    }
    // dvz_canvas_to_refill(panel->grid->canvas);
}



void dvz_panel_span(DvzPanel* panel, DvzGridAxis axis, uint32_t span)
{
    ASSERT(panel != NULL);
    if (axis == DVZ_GRID_HORIZONTAL)
        panel->hspan = span;
    else if (axis == DVZ_GRID_VERTICAL)
        panel->vspan = span;

    _update_grid_panels(panel->grid, axis);
    // dvz_canvas_to_refill(panel->grid->canvas);
}



void dvz_panel_cell(DvzPanel* panel, uint32_t row, uint32_t col)
{
    ASSERT(panel != NULL);
    panel->row = row;
    panel->col = col;

    _update_grid_panels(panel->grid, DVZ_GRID_HORIZONTAL);
    _update_grid_panels(panel->grid, DVZ_GRID_VERTICAL);
    // dvz_canvas_to_refill(panel->grid->canvas);
}



void dvz_panel_transpose(DvzPanel* panel, DvzCDSTranspose transpose)
{
    ASSERT(panel != NULL);
    panel->data_coords.transpose = transpose;
}



void dvz_panel_transform(DvzPanel* panel, DvzTransformType transform)
{
    ASSERT(panel != NULL);
    panel->data_coords.transform = transform;
    log_warn("panel transform not yet implemented");
}



void dvz_panel_lim_get(DvzPanel* panel, vec4 out)
{

    ASSERT(panel != NULL);

    dvec3 in_bl = {-1, +1, .5}, out_bl;
    dvec3 in_tr = {+1, -1, .5}, out_tr;

    DvzTransformChain tc = _transforms_cds(panel, DVZ_CDS_VULKAN, DVZ_CDS_DATA);
    _transforms_apply(&tc, in_bl, out_bl);
    _transforms_apply(&tc, in_tr, out_tr);

    out[0] = out_bl[0];
    out[1] = out_bl[1];
    out[2] = out_tr[0];
    out[3] = out_tr[1];
}



void dvz_panel_lim_set(DvzPanel* panel, vec4 lim)
{
    ASSERT(panel != NULL);

    dvec3 in_bl, out_bl;
    dvec3 in_tr, out_tr;

    in_bl[0] = lim[0];
    in_bl[1] = lim[1];
    in_tr[0] = lim[2];
    in_tr[1] = lim[3];

    DvzTransformChain tc = _transforms_cds(panel, DVZ_CDS_DATA, DVZ_CDS_SCENE);
    _transforms_apply(&tc, in_bl, out_bl);
    _transforms_apply(&tc, in_tr, out_tr);

    DvzPanzoom* p = &panel->controller->interacts[0].u.p;
    _panzoom_reset(p);
    p->camera_pos[0] = .5 * (out_bl[0] + out_tr[0]);
    p->camera_pos[1] = .5 * (out_bl[1] + out_tr[1]);
    p->zoom[0] = 2.0 / (out_tr[0] - out_bl[0]);
    p->zoom[1] = 2.0 / (out_tr[1] - out_bl[1]);
    _panzoom_update_mvp(panel->viewport, p, &panel->controller->interacts[0].mvp);
}



DvzViewport dvz_panel_viewport(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    return panel->viewport;
}



bool dvz_panel_contains(DvzPanel* panel, vec2 screen_pos)
{
    return _pos_in_viewport(panel->viewport, screen_pos);
}



DvzPanel* dvz_panel_at(DvzGrid* grid, vec2 screen_pos)
{
    ASSERT(grid != NULL);
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzPanel* panel = NULL;
    while (iter.item != NULL)
    {
        panel = iter.item;
        if (dvz_panel_contains(panel, screen_pos))
            return panel;
        dvz_container_iter(&iter);
    }
    return NULL;
}



void dvz_panel_link(DvzGrid* grid, DvzPanel* source, DvzPanel* target)
{
    ASSERT(grid != NULL);
    ASSERT(source != NULL);
    ASSERT(target != NULL);

    DvzPanelLink* link = &grid->links[grid->link_count++];
    link->grid = grid;
    link->source = source;
    link->target = target;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        dvz_visual_destroy(panel->visuals[i]);
    }
    dvz_obj_destroyed(&panel->obj);
}
