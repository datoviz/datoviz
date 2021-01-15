#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/panel.h"
#include "../include/visky/transforms.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"
#include "visuals_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _viewport_print(VklViewport v)
{
    log_info(
        "viewport clip %d, dpi %.2f, interact axis %d, margin top %.2f, wf %d, ws %d, viewport "
        "%.2f %.2f %.2f %.2f %.2f %.2f",
        v.clip, v.dpi_scaling, v.interact_axis, v.margins[0], v.size_framebuffer[0],
        v.size_screen[0], v.viewport.width, v.viewport.height, v.viewport.x, v.viewport.y,
        v.viewport.minDepth, v.viewport.maxDepth);
}



static void _panel_to_update(VklPanel* panel)
{
    ASSERT(panel != NULL);
    panel->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    if (panel->scene != NULL)
        panel->scene->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}



// Return the box surrounding all POS props of a visual.
static VklBox _visual_box(VklVisual* visual)
{
    ASSERT(visual != NULL);

    VklProp* prop = NULL;
    VklArray* arr = NULL;

    // The POS props that will need to be transformed.
    uint32_t n_pos_props = 0;

    VklBox boxes[32] = {0}; // max number of props of the same type

    // Gather all non-empty POS props, and get the bounding box on each.
    for (uint32_t i = 0; i < 32; i++)
    {
        prop = vkl_prop_get(visual, VKL_PROP_POS, i);
        if (prop == NULL)
            break;
        arr = &prop->arr_orig;
        if (arr->item_count == 0)
        {
            continue;
        }
        boxes[n_pos_props++] = _box_bounding(arr);
    }

    // Merge the boxes of the visual.
    VklBox box = _box_merge(n_pos_props, boxes);
    return box;
}



// Renormalize a POS prop.
static void _transform_pos_prop(VklDataCoords coords, VklProp* prop)
{
    VklArray* arr = NULL;
    VklArray* arr_tr = NULL;

    arr = &prop->arr_orig;
    arr_tr = &prop->arr_trans;
    if (arr->item_count == 0)
    {
        log_warn("empty POS prop, skipping renormalization");
        return;
    }

    *arr_tr = vkl_array(arr->item_count, arr->dtype);
    vkl_transform(coords, arr, arr_tr);
}



// Renormalize all POS props of all visuals in the panel.
static void _panel_renormalize(VklPanel* panel, VklBox box)
{
    log_debug("renormalize all visuals in panel");
    ASSERT(panel != NULL);
    VklVisual* visual = NULL;
    VklProp* prop = NULL;

    // Update the data coords box.
    panel->data_coords.box = box;

    // Go through all visuals in the panel.
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        visual = panel->visuals[i];

        // NOTE: skip visuals that should not be transformed.
        if ((visual->flags & VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE) != 0)
            continue;

        // Go through all visual props.
        prop = vkl_container_iter_init(&visual->props);
        while (prop != NULL)
        {
            if (prop->prop_type == VKL_PROP_POS)
            {
                // Transform all POS props with the panel data coordinates.
                _transform_pos_prop(panel->data_coords, prop);

                // Mark the visual has needing data update.
                visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
            }

            prop = vkl_container_iter(&visual->props);
        }
    }
}



// Update the VklPanel.data_coords struct when a new visual is added.
static void _panel_visual_added(VklPanel* panel, VklVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    VklDataCoords* coords = &panel->data_coords;

    // Get the visual box.
    VklBox box = _visual_box(visual);

    // Merge the visual box with the existing box.
    box = _box_merge(2, (VklBox[]){coords->box, box});

    // Make the box square if needed.
    if ((coords->flags & VKL_TRANSFORM_FLAGS_FIXED_ASPECT) != 0)
        box = _box_cube(box);

    // If the panel box has changed, renormalize all visuals.
    if (memcmp(&box, &coords->box, sizeof(VklBox)) != 0)
    {
        // Renormalize all visuals in the panel.
        _panel_renormalize(panel, box);

        // Mark the panel as NEED_UPDATE so that the new data gets uploaded to the GPU.
        _panel_to_update(panel);
    }
}



// Update the VklPanel.data_coords struct as a function of all of the visuals data.
static void _panel_normalize(VklPanel* panel)
{
    ASSERT(panel != NULL);
    log_debug("full panel normalization on %d visuals", panel->visual_count);

    VklDataCoords* coords = &panel->data_coords;
    VklBox* boxes = calloc(panel->visual_count, sizeof(VklBox));

    // Get the bounding box of each visual.
    for (uint32_t i = 0; i < panel->visual_count; i++)
        boxes[i] = _visual_box(panel->visuals[i]);

    // Merge the visual box with the existing box.
    VklBox box = _box_merge(panel->visual_count, boxes);

    // Make the box square if needed.
    if ((coords->flags & VKL_TRANSFORM_FLAGS_FIXED_ASPECT) != 0)
        box = _box_cube(box);

    // Renormalize all visuals in the panel.
    _panel_renormalize(panel, box);

    // Mark the panel as NEED_UPDATE so that the new data gets uploaded to the GPU.
    _panel_to_update(panel);

    FREE(boxes);
}



static void _update_visual_viewport(VklPanel* panel, VklVisual* visual)
{
    visual->viewport = panel->viewport;
    // Each graphics pipeline in the visual has its own transform/clip viewport options
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        visual->viewport.interact_axis = visual->interact_axis[pidx];
        visual->viewport.clip = visual->clip[pidx];
        ASSERT(visual->viewport.viewport.minDepth < visual->viewport.viewport.maxDepth);
        // NOTE: here we make the assumption that there is exactly 1 viewport per graphics
        // pipeline, such that the source idx corresponds to the pipeline idx.
        // _viewport_print(visual->viewport);
        vkl_visual_data_source(visual, VKL_SOURCE_TYPE_VIEWPORT, pidx, 0, 1, 1, &visual->viewport);
        // HACK: this call should not set the visual status to NEED_UPDATE
        visual->obj.status = VKL_OBJECT_STATUS_CREATED;
    }
}



static void _common_data(VklPanel* panel, VklVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    // Binding 0: MVP binding
    vkl_visual_buffer(visual, VKL_SOURCE_TYPE_MVP, 0, panel->br_mvp);

    // Binding 1: viewport
    _update_visual_viewport(panel, visual);
}



static void _scene_fill(VklCanvas* canvas, VklEvent ev)
{
    log_debug("scene fill");
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklScene* scene = (VklScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;

    VklViewport viewport = {0};
    VklCommands* cmds = NULL;
    VklPanel* panel = NULL;
    VklVisual* visual = NULL;
    uint32_t img_idx = 0;

    // Go through all the current command buffers.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        cmds = ev.u.rf.cmds[i];
        img_idx = ev.u.rf.img_idx;

        log_trace("visual fill cmd %d begin %d", i, img_idx);
        vkl_visual_fill_begin(canvas, cmds, img_idx);

        panel = vkl_container_iter_init(&grid->panels);
        while (panel != NULL)
        {
            // Update the panel.
            vkl_panel_update(panel);
            ASSERT(is_obj_created(&panel->obj));

            // Find the panel viewport.
            viewport = vkl_panel_viewport(panel);
            vkl_cmd_viewport(cmds, img_idx, viewport.viewport);

            // Update visual VklViewport struct and upload it, only once per visual.
            // TODO: move this to a RESIZE callback instead
            if (img_idx == 0)
                for (uint32_t k = 0; k < panel->visual_count; k++)
                    _update_visual_viewport(panel, panel->visuals[k]);

            // Go through all visuals in the panel.
            visual = NULL;
            for (int priority = -panel->prority_max; priority <= panel->prority_max; priority++)
            {
                for (uint32_t k = 0; k < panel->visual_count; k++)
                {
                    visual = panel->visuals[k];
                    if (visual->priority != priority)
                        continue;

                    vkl_visual_fill_event(
                        visual, ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
                }
            }

            panel = vkl_container_iter(&grid->panels);
        }
        vkl_visual_fill_end(canvas, cmds, img_idx);
    }
}



static void _scene_frame(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklScene* scene = (VklScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    VklViewport viewport = {0};

    // Go through all panels that need to be updated.
    bool to_update = false;

    VklPanel* panel = vkl_container_iter_init(&grid->panels);
    VklSource* source = NULL;
    VklVisual* visual = NULL;
    while (panel != NULL)
    {
        // Interactivity.
        if (panel->controller != NULL && panel->controller->callback != NULL)
        {
            // TODO: event struct
            panel->controller->callback(panel->controller, (VklEvent){0});
        }

        // TODO
        // // Handle floating panels.
        // if (panel->mode == VKL_PANEL_FLOATING &&               //
        //     canvas->mouse.cur_state == VKL_MOUSE_STATE_DRAG && //
        //     vkl_panel_contains(panel, canvas->mouse.press_pos))
        // {
        //     float x = canvas->mouse.cur_pos[0] / canvas->window->width;
        //     float y = canvas->mouse.cur_pos[1] / canvas->window->height;
        //     log_info("moving panel to %.1fx%.1f", x, y);
        //     vkl_panel_pos(panel, x, y);
        // }

        // Initial normalization of all visuals in the panel.
        if (canvas->frame_idx == 0)
            _panel_normalize(panel);

        // Update all visuals in the panel, using the panel's viewport.
        to_update = panel->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE;
        viewport = panel->viewport;
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            visual = panel->visuals[j];

            // First frame:
            // Initialize prev_vertex_count and prev_index_count.
            if (canvas->frame_idx == 0)
            {
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
                    visual->prev_vertex_count[pidx] = source->arr.item_count;

                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL)
                        visual->prev_index_count[pidx] = source->arr.item_count;
                }
            }

            // Update visual data if needed.
            if (visual->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            {
                vkl_visual_update(visual, viewport, panel->data_coords, NULL);
                visual->obj.status = VKL_OBJECT_STATUS_CREATED;
            }

            // Detect whether the vertex/index count has changed, in which case we'll need a
            // full refill in the same frame as the data upload.
            if (to_update)
            {
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    // Detect a change in vertex_count.
                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
                    if (source->arr.item_count != visual->prev_vertex_count[pidx])
                    {
                        log_debug("automatic detection of a change in vertex count, will trigger "
                                  "full refill");
                        vkl_canvas_to_refill(canvas);
                        visual->prev_vertex_count[pidx] = source->arr.item_count;
                    }

                    // Detect a change in index_count.
                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL && source->arr.item_count != visual->prev_index_count[pidx])
                    {
                        log_debug("automatic detection of a change in index count, will trigger "
                                  "full refill");
                        vkl_canvas_to_refill(canvas);
                        visual->prev_index_count[pidx] = source->arr.item_count;
                    }
                }
            }
        }
        panel->obj.status = VKL_OBJECT_STATUS_CREATED;

        panel = vkl_container_iter(&grid->panels);
    }
    scene->obj.status = VKL_OBJECT_STATUS_CREATED;
}



static void _upload_mvp(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklScene* scene = (VklScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    VklInteract* interact = NULL;
    VklController* controller = NULL;
    // VklBufferRegions* br = NULL;

    // Go through all panels that need to be updated.
    VklPanel* panel = vkl_container_iter_init(&grid->panels);
    while (panel != NULL)
    {
        if (panel->controller == NULL)
            continue;
        controller = panel->controller;

        // Go through all interact of the controllers.
        // TODO: only 1 interact to be supported?
        for (uint32_t j = 0; j < controller->interact_count; j++)
        {
            // Multiple interacts not yet supported.
            ASSERT(j == 0);
            interact = &controller->interacts[j];

            // NOTE: update MVP.time here.
            interact->mvp.time = canvas->clock.elapsed;

            // NOTE: we need to update the uniform buffer at every frame
            vkl_upload_buffers(canvas, panel->br_mvp, 0, panel->br_mvp.size, &interact->mvp);
        }
        panel = vkl_container_iter(&grid->panels);
    }
}



static void _default_controller_callback(VklController* controller, VklEvent ev)
{
    VklScene* scene = controller->panel->scene;
    VklCanvas* canvas = scene->canvas;

    // Controller interactivity.
    VklInteract* interact = NULL;

    // Use all interact of the controllers.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        interact = &controller->interacts[i];
        // float delay = canvas->clock.elapsed - interact->last_update;

        // Update the interact using the current panel's viewport.
        VklViewport viewport = controller->panel->viewport;
        vkl_interact_update(interact, viewport, &canvas->mouse, &canvas->keyboard);
        // NOTE: the CPU->GPU transfer occurs at every frame, in another callback below
    }
}



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

// Create the ticks context.
static VklAxesContext _axes_context(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);

    ASSERT(controller->panel != NULL);
    ASSERT(controller->panel->grid != NULL);

    // Canvas size, used in tick computation.
    VklCanvas* canvas = controller->panel->grid->canvas;
    ASSERT(canvas != NULL);
    float dpi_scaling = controller->panel->viewport.dpi_scaling;
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    VklViewport* viewport = &controller->panel->viewport;
    vec4 m = {0};
    glm_vec4_copy(viewport->margins, m);

    // Make axes context.
    VklAxesContext ctx = {0};
    ctx.coord = coord;
    ctx.extensions = 1; // extend the range once on the left/right and top/bottom
    ctx.size_viewport = size[coord] - m[1 - coord] - m[3 - coord]; // remove the margins
    ctx.scale_orig = controller->interacts[0].u.p.zoom[coord];

    // TODO: improve determination of glyph size
    float font_size = controller->u.axes_2D.font_size;
    ASSERT(font_size > 0);
    VklFontAtlas* atlas = &canvas->gpu->context->font_atlas;
    ASSERT(atlas->glyph_width > 0);
    ASSERT(atlas->glyph_height > 0);
    ctx.size_glyph = coord == VKL_AXES_COORD_X
                         ? font_size * atlas->glyph_width / atlas->glyph_height
                         : font_size;
    ctx.size_glyph *= dpi_scaling;

    return ctx;
}



// Recompute the tick locations as a function of the current axis range in data coordinates.
static void _axes_ticks(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // Find the ticks given the range.
    double vmin = axes->panzoom_range[coord][0];
    double vmax = axes->panzoom_range[coord][1];
    double vlen = vmax - vmin;
    ASSERT(vlen > 0);

    // // Extended range for tolerancy during panzoom.
    // double vmin0 = vmin - vlen;
    // double vmax0 = vmax + vlen;
    // ASSERT(vmin0 < vmax0);

    // Prepare context for tick computation.
    VklAxesContext ctx = _axes_context(controller, coord);

    // Determine the tick number and positions.
    axes->ticks[coord] = vkl_ticks(vmin, vmax, ctx);

    // We keep track of the context.
    axes->ctx[coord] = ctx;
}



// Update the axes visual's data as a function of the computed ticks.
static void _axes_upload(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(controller->visual_count == 2);

    VklVisual* visual = controller->visuals[coord];

    VklAxesTicks* axticks = &axes->ticks[coord];
    uint32_t N = axticks->value_count;

    // Convert ticks from double to float.
    float* ticks = calloc(N, sizeof(float));
    for (uint32_t i = 0; i < N; i++)
        ticks[i] = axticks->values[i];

    // Minor ticks.
    float* minor_ticks = calloc((N - 1) * 4, sizeof(float));
    uint32_t k = 0;
    for (uint32_t i = 0; i < N - 1; i++)
    {
        for (uint32_t j = 1; j <= 4; j++)
        {
            minor_ticks[k++] =
                axticks->values[i] + j * (axticks->values[i + 1] - axticks->values[i]) / 5.;
        }
    }
    ASSERT(k == (N - 1) * 4);

    // Prepare text values.
    char** text = calloc(N, sizeof(char*));
    for (uint32_t i = 0; i < N; i++)
        text[i] = &axticks->labels[i * MAX_GLYPHS_PER_TICK];

    // Set visual data.
    float lim[] = {-1};
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, 4 * (N - 1), minor_ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visual, VKL_PROP_TEXT, 0, N, text);

    FREE(ticks);
    FREE(text);
}



// Initialize the ticks positions and visual.
static void _axes_ticks_init(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;

    // NOTE: get the font size which was set by in builtin_visuals.c as a prop.
    VklProp* prop = vkl_prop_get(controller->visuals[0], VKL_PROP_TEXT_SIZE, 0);
    ASSERT(prop != NULL);
    float* font_size = vkl_prop_item(prop, 0);
    axes->font_size = *font_size;
    ASSERT(axes->font_size > 0);

    ASSERT(axes != NULL);

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        // TODO: initial data coordinates from the panel
        axes->panzoom_range[coord][0] = -1;
        axes->panzoom_range[coord][1] = +1;
        axes->tick_range[coord][0] = -1;
        axes->tick_range[coord][1] = +1;

        // Check the initial range.
        ASSERT(axes->tick_range[coord][0] < axes->tick_range[coord][1]);

        // Compute the ticks for these ranges.
        _axes_ticks(controller, coord);

        // Upload the data.
        _axes_upload(controller, coord);
    }
}



// Determine the coords that need to be updated during panzoom because of overlapping labels.
static void _axes_collision(VklController* controller, bool* update)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(update != NULL);

    // Determine whether the ticks are overlapping, if so we should recompute the ticks (zooming)
    // Same if there are less than N visible labels (dezooming)
    for (uint32_t i = 0; i < 2; i++)
    {
        // VklAxisCoord coord = (VklAxisCoord)i;
        VklAxesTicks* ticks = &axes->ticks[i];
        ASSERT(ticks != NULL);

        // NOTE: make a copy because we'll use a temporary context object when computing the
        // overlap.
        VklAxesContext ctx = axes->ctx[i];
        // ctx.labels = ticks->labels;
        ASSERT(controller->interacts != NULL);
        ASSERT(controller->interact_count >= 1);
        float scale = controller->interacts[0].u.p.zoom[i] / ctx.scale_orig;
        ASSERT(scale > 0);
        ctx.size_viewport *= scale;
        ASSERT(ctx.size_viewport > 0);
        // ASSERT(ctx.labels != NULL);

        // Check whether there are overlapping labels (dezooming).
        double min_distance = min_distance_labels(ticks, &ctx);

        // Check whether the current view is outside the computed ticks (panning);
        bool outside =
            axes->panzoom_range[i][0] <= ticks->lmin || axes->panzoom_range[i][1] >= ticks->lmax;

        double rel_space = min_distance / (ctx.size_viewport / scale);

        // if (i == 0)
        //     log_info(
        //         "coord %d min_d %.3f, rel_space %.3f, outside %d", //
        //         i, min_distance, rel_space, outside);
        // Recompute the ticks on the current axis?
        update[i] = min_distance <= 0 || rel_space >= .5 || outside;
    }
}



// Update axes->range struct as a function of the current panzoom.
static void _axes_range(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // set axes->range depending on coord
    VklTransformOLD tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};
    tr = vkl_transform_old(controller->panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
    // TODO: transform to data coordinates instead of GPU coordinates.
    vkl_transform_apply(&tr, ll, pos_ll);
    vkl_transform_apply(&tr, ur, pos_ur);
    axes->panzoom_range[coord][0] = pos_ll[coord];
    axes->panzoom_range[coord][1] = pos_ur[coord];
    // log_info("%.3f %.3f", axes->range[coord][0], axes->range[coord][1]);
}



// Callback called at every frame.
static void _axes_callback(VklController* controller, VklEvent ev)
{
    ASSERT(controller != NULL);
    _default_controller_callback(controller, ev);
    ASSERT(controller->interacts != NULL);
    ASSERT(controller->interact_count >= 1);
    ASSERT(controller->panel != NULL);
    ASSERT(controller->panel->grid != NULL);

    VklCanvas* canvas = controller->panel->grid->canvas;
    ASSERT(canvas != NULL);

    if (!controller->interacts[0].is_active && !canvas->resized)
        return;

    // Check label collision
    // DEBUG
    // bool update[2] = {true, true}; // whether X and Y axes must be updated or not
    bool update[2] = {false, false}; // whether X and Y axes must be updated or not
    _axes_range(controller, 0);
    _axes_range(controller, 1);
    _axes_collision(controller, update);
    // Force axes ticks refresh when resizing.
    if (canvas->resized)
    {
        update[0] = true;
        update[1] = true;
    }

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        if (!update[coord])
            continue;
        _axes_ticks(controller, coord);
        _axes_upload(controller, coord);
        canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    }
}



// Add axes to a panel.
static void _add_axes(VklController* controller)
{
    ASSERT(controller != NULL);
    VklPanel* panel = controller->panel;
    ASSERT(panel != NULL);
    panel->controller = controller;
    VklContext* ctx = panel->grid->canvas->gpu->context;
    ASSERT(ctx != NULL);

    vkl_panel_margins(panel, (vec4){25, 25, 100, 100});

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        VklVisual* visual = vkl_scene_visual(
            panel, VKL_VISUAL_AXES_2D, VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE | (int)coord);
        vkl_controller_visual(controller, visual);
        visual->priority = VKL_MAX_VISUAL_PRIORITY;

        visual->clip[0] = VKL_VIEWPORT_OUTER;
        visual->clip[1] = coord == 0 ? VKL_VIEWPORT_OUTER_BOTTOM : VKL_VIEWPORT_OUTER_LEFT;

        visual->interact_axis[0] = visual->interact_axis[1] =
            coord == 0 ? VKL_INTERACT_AXIS_X : VKL_INTERACT_AXIS_Y;

        // Text params.
        VklFontAtlas* atlas = &ctx->font_atlas;
        ASSERT(strlen(atlas->font_str) > 0);
        vkl_visual_texture(visual, VKL_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

        VklGraphicsTextParams params = {0};
        params.grid_size[0] = (int32_t)atlas->rows;
        params.grid_size[1] = (int32_t)atlas->cols;
        params.tex_size[0] = (int32_t)atlas->width;
        params.tex_size[1] = (int32_t)atlas->height;
        vkl_visual_data_source(visual, VKL_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);
    }
    // Add the axes data.
    _axes_ticks_init(controller);
}



// Destroy the axes objects.
static void _axes_destroy(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t i = 0; i < 2; i++)
    {
        vkl_ticks_destroy(&axes->ticks[i]);
    }
}



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VklTransformOLD vkl_transform_inv(VklTransformOLD tr)
{
    ASSERT(tr.scale[0] != 0);
    ASSERT(tr.scale[1] != 0);

    VklTransformOLD tri = {0};
    tri.scale[0] = 1. / tr.scale[0];
    tri.scale[1] = 1. / tr.scale[1];
    tri.shift[0] = -tr.scale[0] * tr.shift[0];
    tri.shift[1] = -tr.scale[1] * tr.shift[1];
    return tri;
}



VklTransformOLD vkl_transform_mul(VklTransformOLD tr0, VklTransformOLD tr1)
{
    VklTransformOLD trm = {0};
    trm.scale[0] = tr0.scale[0] * tr1.scale[0];
    trm.scale[1] = tr0.scale[1] * tr1.scale[1];
    trm.shift[0] = tr0.shift[0] + tr1.shift[0] / tr0.scale[0];
    trm.shift[1] = tr0.shift[1] + tr1.shift[1] / tr0.scale[1];
    return trm;
}



VklTransformOLD vkl_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout)
{
    VklTransformOLD tr = {0};
    tr.scale[0] = tr.scale[1] = 1;
    if (qin[0] != pin[0])
        tr.scale[0] = (qout[0] - pout[0]) / (qin[0] - pin[0]);
    if (qin[1] != pin[1])
        tr.scale[1] = (qout[1] - pout[1]) / (qin[1] - pin[1]);
    if (qout[0] != pout[0])
        tr.shift[0] = (pin[0] * qout[0] - pout[0] * qin[0]) / (qout[0] - pout[0]);
    if (qout[1] != pout[1])
        tr.shift[1] = (pin[1] * qout[1] - pout[1] * qin[1]) / (qout[1] - pout[1]);
    return tr;
}



void vkl_transform_apply(VklTransformOLD* tr, dvec2 in, dvec2 out)
{
    ASSERT(tr != NULL);
    if (tr->scale[0] != 0)
        out[0] = tr->scale[0] * (in[0] - tr->shift[0]);
    if (tr->scale[1] != 0)
        out[1] = tr->scale[1] * (in[1] - tr->shift[1]);
}



VklTransformOLD vkl_transform_old(VklPanel* panel, VklCDS source, VklCDS target)
{
    ASSERT(panel != NULL);
    VklTransformOLD tr = {{1, 1}, {0, 0}}; // identity
    dvec2 NDC0 = {-1, -1};
    dvec2 NDC1 = {+1, +1};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    VklPanzoom* panzoom = NULL;
    VklCanvas* canvas = panel->scene->canvas;

    if (panel->controller->type == VKL_CONTROLLER_AXES_2D)
    {
        // log_error("not implemented yet");
        // TODO
        // ll[0] = axes->xscale_orig.vmin;
        // ll[1] = axes->yscale_orig.vmin;
        // ur[0] = axes->xscale_orig.vmax;
        // ur[1] = axes->yscale_orig.vmax;
        // panzoom = axes->panzoom_inner;
        panzoom = &panel->controller->interacts[0].u.p;
    }
    else if (panel->controller->type == VKL_CONTROLLER_PANZOOM)
    {
        panzoom = &panel->controller->interacts[0].u.p;
    }
    else
    {
        log_error("controller other than axes 2D and panzoom not yet supported");
        return tr;
    }

    VklViewport viewport = panel->viewport;

    if (source == target)
    {
        return tr;
    }
    else if (source > target)
    {
        return vkl_transform_inv(vkl_transform_old(panel, target, source));
    }
    else if (target - source >= 2)
    {
        for (uint32_t k = source; k <= target - 1; k++)
        {
            tr = vkl_transform_mul(tr, vkl_transform_old(panel, (VklCDS)k, (VklCDS)(k + 1)));
        }
    }
    else if (target - source == 1)
    {
        switch (source)
        {

        case VKL_CDS_DATA:
            // linear normalization based on axes range
            ASSERT(target == VKL_CDS_GPU);
            {
                tr = vkl_transform_interp(ll, NDC0, ur, NDC1);
            }
            break;

        case VKL_CDS_GPU:
            // apply panzoom
            ASSERT(target == VKL_CDS_PANZOOM);
            {
                ASSERT(panzoom != NULL);
                ASSERT(panzoom->zoom[0] != 0);
                ASSERT(panzoom->zoom[1] != 0);
                dvec2 p = {panzoom->camera_pos[0], panzoom->camera_pos[1]};
                dvec2 s = {panzoom->zoom[0], panzoom->zoom[1]};
                tr.scale[0] = s[0];
                tr.scale[1] = s[1];
                tr.shift[0] = p[0]; // / s[0];
                tr.shift[1] = p[1]; // / s[1];
            }
            break;

        case VKL_CDS_PANZOOM:
            // using inner viewport
            ASSERT(target == VKL_CDS_PANEL);
            {
                // Margins.
                // double cw = panel->scene->canvas->size.framebuffer_width;
                // double ch = panel->scene->canvas->size.framebuffer_height;
                // uvec2 size = {0};
                // vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
                double cw = viewport.viewport.width;
                double ch = viewport.viewport.height;
                double mt = 2 * viewport.margins[0] / ch;
                double mr = 2 * viewport.margins[1] / cw;
                double mb = 2 * viewport.margins[2] / ch;
                double ml = 2 * viewport.margins[3] / cw;

                tr = vkl_transform_interp(
                    NDC0, (dvec2){-1 + ml, -1 + mb}, NDC1, (dvec2){+1 - mr, +1 - mt});
            }
            break;

        case VKL_CDS_PANEL:
            // multiply by canvas size
            ASSERT(target == VKL_CDS_CANVAS_NDC);
            {
                // From outer to inner viewport.
                ll[0] = -1 + 2 * viewport.viewport.x;
                ll[1] = +1 - 2 * (viewport.viewport.y + viewport.viewport.height);
                ur[0] = -1 + 2 * (viewport.viewport.x + viewport.viewport.width);
                ur[1] = +1 - 2 * viewport.viewport.y;

                tr = vkl_transform_interp(NDC0, ll, NDC1, ur);
            }
            break;

        case VKL_CDS_CANVAS_NDC:
            // multiply by canvas size
            ASSERT(target == VKL_CDS_CANVAS_PX);
            {
                uvec2 size = {0};
                vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
                tr = vkl_transform_interp(NDC0, (dvec2){0, size[1]}, NDC1, (dvec2){size[0], 0});
            }
            break;

        default:
            log_error("unknown coordinate systems");
            break;
        }
    }
    ASSERT(tr.scale[0] != 0);
    ASSERT(tr.scale[1] != 0);
    return tr;
}



/*************************************************************************************************/
/*  Scene creation                                                                               */
/*************************************************************************************************/

VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols)
{
    ASSERT(canvas != NULL);
    canvas->scene = calloc(1, sizeof(VklScene));
    canvas->scene->canvas = canvas;
    canvas->scene->grid = vkl_grid(canvas, n_rows, n_cols);

    canvas->scene->visuals =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklVisual), VKL_OBJECT_TYPE_VISUAL);
    canvas->scene->controllers = vkl_container(
        VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklController), VKL_OBJECT_TYPE_CONTROLLER);

    vkl_event_callback(
        canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _scene_fill, canvas->scene);

    // HACK: we use a param of 1 here as a way of putting a lower priority, so that the
    // _scene_frame callback is called *after* the user FRAME callbacks. If the user callbacks call
    // vkl_visual_data(), the _scene_frame() callback will be called directly afterwards, in the
    // same frame.
    vkl_event_callback(
        canvas, VKL_EVENT_FRAME, 1, VKL_EVENT_MODE_SYNC, _scene_frame, canvas->scene);

    vkl_event_callback(
        canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _upload_mvp, canvas->scene);


    return canvas->scene;
}



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

VklController vkl_controller(VklPanel* panel)
{
    ASSERT(panel != NULL);
    VklController controller = {0};
    controller.panel = panel;
    controller.callback = _default_controller_callback;
    obj_created(&controller.obj);
    return controller;
}



void vkl_controller_visual(VklController* controller, VklVisual* visual)
{
    ASSERT(controller != NULL);
    controller->visuals[controller->visual_count++] = visual;
}



void vkl_controller_interact(VklController* controller, VklInteractType type)
{
    ASSERT(controller != NULL);
    VklCanvas* canvas = controller->panel->grid->canvas;
    controller->interacts[controller->interact_count++] = vkl_interact_builtin(canvas, type);
}



void vkl_controller_callback(VklController* controller, VklControllerCallback callback)
{
    ASSERT(controller != NULL);
    controller->callback = callback;
}



void vkl_controller_update(VklController* controller)
{
    ASSERT(controller != NULL);
    //
    /*
    TODO
    called whenever the visuals need to refresh their data with the current viewport and data
    coords
    loop over all visuals in the panel
    call vkl_visual_update()
    */
}



void vkl_controller_destroy(VklController* controller)
{
    if (controller->obj.status == VKL_OBJECT_STATUS_DESTROYED)
        return;
    ASSERT(controller != NULL);

    // TODO: controller destruction callback
    switch (controller->type)
    {
    case VKL_CONTROLLER_AXES_2D:
        _axes_destroy(controller);
        break;

    default:
        break;
    }

    // Destroy the interacts.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        vkl_interact_destroy(&controller->interacts[i]);
    }
    obj_destroyed(&controller->obj);
}



VklController vkl_controller_builtin(VklPanel* panel, VklControllerType type, int flags)
{
    ASSERT(panel != NULL);
    VklController controller = vkl_controller(panel);
    controller.type = type;

    switch (type)
    {

    case VKL_CONTROLLER_NONE:
        break;

    case VKL_CONTROLLER_PANZOOM:
        vkl_controller_interact(&controller, VKL_INTERACT_PANZOOM);
        break;

    case VKL_CONTROLLER_ARCBALL:
        vkl_controller_interact(&controller, VKL_INTERACT_ARCBALL);
        break;

    case VKL_CONTROLLER_CAMERA:
        vkl_controller_interact(&controller, VKL_INTERACT_FLY);
        break;

    case VKL_CONTROLLER_AXES_2D:
        vkl_controller_interact(&controller, VKL_INTERACT_PANZOOM);
        vkl_controller_callback(&controller, _axes_callback);
        _add_axes(&controller);
        break;

    default:
        log_error("unknown controller type");
        break;
    }

    return controller;
}



/*************************************************************************************************/
/*  High-level functions                                                                         */
/*************************************************************************************************/

VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags)
{
    ASSERT(scene != NULL);
    VklPanel* panel = vkl_panel(&scene->grid, row, col);
    VklController* controller = vkl_container_alloc(&scene->controllers);
    *controller = vkl_controller_builtin(panel, type, flags);
    controller->flags = flags;
    panel->controller = controller;
    // TODO: update panel->data_coords.transform depending on the flags
    panel->scene = scene;
    panel->prority_max = VKL_MAX_VISUAL_PRIORITY;
    // At initialization, must update all visuals data.
    _panel_to_update(panel);
    return panel;
}



VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller != NULL);
    VklScene* scene = panel->grid->canvas->scene;
    VklVisual* visual = vkl_container_alloc(&scene->visuals);

    // Create the visual.
    *visual = vkl_visual(panel->grid->canvas);
    vkl_visual_builtin(visual, type, flags);

    // Add it to the panel.
    vkl_panel_visual(panel, visual);

    // Bind the common buffers (MVP, viewport, color texture).
    _common_data(panel, visual);

    // Put all graphics pipeline in the inner viewport by default.
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
        visual->clip[pidx] = VKL_VIEWPORT_INNER;

    // Update the panel data coords as a function of the visual's data.
    if (scene->canvas->app->is_running)
        _panel_visual_added(panel, visual);

    return visual;
}



/*************************************************************************************************/
/*  Scene destruction                                                                            */
/*************************************************************************************************/

void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    // Destroy all panels.
    VklPanel* panel = vkl_container_iter_init(&grid->panels);
    while (panel != NULL)
    {
        if (panel->obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        // This also destroys all visuals in the panel.
        vkl_panel_destroy(panel);
        panel = vkl_container_iter(&grid->panels);
    }

    // Destroy all controllers.
    CONTAINER_DESTROY_ITEMS(VklController, scene->controllers, vkl_controller_destroy)
    vkl_container_destroy(&scene->controllers);

    vkl_container_destroy(&scene->visuals);
    obj_destroyed(&scene->obj);
    FREE(scene);
}
