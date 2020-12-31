#include "../include/visky/scene.h"
#include "../external/exwilk.h"
#include "../include/visky/canvas.h"
#include "../include/visky/panel.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _update_visual_viewport(VklPanel* panel, VklVisual* visual)
{
    visual->viewport = panel->viewport;
    // Each graphics pipeline in the visual has its own transform/clip viewport options
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        visual->viewport.transform = visual->transform[pidx];
        visual->viewport.clip = visual->clip[pidx];
        vkl_visual_data_buffer(visual, VKL_SOURCE_TYPE_VIEWPORT, pidx, 0, 1, 1, &visual->viewport);
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



static void _panel_to_update(VklPanel* panel)
{
    ASSERT(panel != NULL);
    panel->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    if (panel->scene != NULL)
        panel->scene->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}



static void _scene_fill(VklCanvas* canvas, VklPrivateEvent ev)
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
    uint32_t img_idx = 0;

    // Go through all the current command buffers.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        cmds = ev.u.rf.cmds[i];
        img_idx = ev.u.rf.img_idx;

        log_trace("visual fill cmd %d begin %d", i, img_idx);
        vkl_visual_fill_begin(canvas, cmds, img_idx);

        panel = vkl_container_iter(&grid->panels);
        while (panel != NULL)
        {
            // Update the panel.
            vkl_panel_update(panel);
            ASSERT(is_obj_created(&panel->obj));

            // Find the panel viewport.
            viewport = vkl_panel_viewport(panel);
            vkl_cmd_viewport(cmds, img_idx, viewport.viewport);

            // Go through all visuals in the panel.
            VklVisual* visual = NULL;
            for (int priority = -panel->prority_max; priority <= panel->prority_max; priority++)
            {
                for (uint32_t k = 0; k < panel->visual_count; k++)
                {
                    visual = panel->visuals[k];
                    if (visual->priority != priority)
                        continue;
                    // Update visual VklViewport struct and upload it.
                    _update_visual_viewport(panel, visual);

                    vkl_visual_fill_event(
                        visual, ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
                }
            }

            panel = vkl_container_iter(&grid->panels);
        }

        vkl_visual_fill_end(canvas, cmds, img_idx);
    }
}



static void _scene_frame(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklScene* scene = (VklScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    VklViewport viewport = {0};

    // Go through all panels that need to be updated.
    bool to_update = false;

    VklPanel* panel = vkl_container_iter(&grid->panels);
    VklVisual* visual = NULL;
    VklSource* source = NULL;
    while (panel != NULL)
    {
        // Interactivity.
        if (panel->controller != NULL && panel->controller->callback != NULL)
        {
            // TODO: event struct
            panel->controller->callback(panel->controller, (VklEvent){0});
        }

        // Update all visuals in the panel, using the panel's viewport.
        to_update = panel->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE;
        viewport = panel->viewport;
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            visual = panel->visuals[j];

            // First frame: initialize prev_vertex_count and prev_index_count.
            if (canvas->frame_idx == 0)
            {
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    source = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
                    visual->prev_vertex_count[pidx] = source->arr.item_count;

                    source = vkl_bake_source(visual, VKL_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL)
                        visual->prev_index_count[pidx] = source->arr.item_count;
                }
            }

            if (to_update || visual->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            {
                // TODO: data coords
                vkl_visual_update(visual, viewport, (VklDataCoords){0}, NULL);

                // Detect whether the vertex/index count has changed, in which case we'll need a
                // full refill in the same frame as the data upload. To signal this to the canvas
                // we'll set it to the NEED_FULL_UPDATE status. At the end of the frame, after the
                // pending transfer tasks have completed, we'll trigger a full refill of the canvas
                // with full GPU wait on the RENDER queue.
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    // Detect a change in vertex_count.
                    source = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
                    if (source->arr.item_count != visual->prev_vertex_count[pidx])
                    {
                        log_info("automatic detection of a change in vertex count, will trigger "
                                 "full refill");
                        canvas->obj.status = VKL_OBJECT_STATUS_NEED_FULL_UPDATE;
                        visual->prev_vertex_count[pidx] = source->arr.item_count;
                    }

                    // Detect a change in index_count.
                    source = vkl_bake_source(visual, VKL_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL && source->arr.item_count != visual->prev_index_count[pidx])
                    {
                        log_info("automatic detection of a change in index count, will trigger "
                                 "full refill");
                        canvas->obj.status = VKL_OBJECT_STATUS_NEED_FULL_UPDATE;
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



static void _upload_mvp(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklScene* scene = (VklScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    VklInteract* interact = NULL;
    VklController* controller = NULL;
    VklBufferRegions* br = NULL;

    // Go through all panels that need to be updated.
    VklPanel* panel = vkl_container_iter(&grid->panels);
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
            br = &panel->br_mvp;

            // GPU transfer happens here:
            VklPointer pointer = aligned_repeat(br->size, &interact->mvp, 1, br->alignment);
            vkl_buffer_regions_upload(br, canvas->swapchain.img_idx, pointer.pointer);
            ALIGNED_FREE(pointer)
        }
        panel = vkl_container_iter(&grid->panels);
    }
}



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

static void _tick_format(double value, char* out_text) { snprintf(out_text, 16, "%.1f", value); }

static void _axes_ticks(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    VklCanvas* canvas = controller->panel->grid->canvas;

    VklArray* arr = &axes->ticks[coord];
    VklArray* text = &axes->text[coord];
    char* buf = axes->buf[coord];

    // Find the ticks given the range.
    double vmin = axes->range[coord][0];
    double vmax = axes->range[coord][1];
    double vlen = vmax - vmin;
    ASSERT(vlen > 0);

    double vmin0 = vmin - vlen;
    double vmax0 = vmax + vlen;
    ASSERT(vmin0 < vmax0);

    // TODO: improve
    int32_t N = 3 * 12;
    VklAxesContext context = {0};
    context.glyph_size[0] = 12;
    context.glyph_size[1] = 12;
    context.coord = coord;
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    context.viewport_size[0] = size[0];
    context.viewport_size[1] = size[1];
    R r = wilk_ext(vmin0, vmax0, N, false, context);
    vmin0 = r.lmin;
    vmax0 = r.lmax;
    double dv = r.lstep;
    N = (int32_t)ceil((vmax0 - vmin0) / dv);
    ASSERT(N > 0);
    vkl_array_resize(arr, (uint32_t)N);
    vkl_array_resize(text, (uint32_t)N);

    double v = 0;
    for (uint32_t i = 0; i < (uint32_t)N; i++)
    {
        v = vmin0 + dv * i;
        ((float*)arr->data)[i] = v;
        ((char**)text->data)[i] = &buf[16 * i];
        _tick_format(v, &buf[16 * i]);
    }
}

static void _axes_upload(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(controller->visual_count == 2);

    VklVisual* visual = controller->visuals[coord];
    VklArray* arr = &axes->ticks[coord];

    uint32_t N = arr->item_count;
    float* ticks = arr->data;
    char** text = axes->text[coord].data;
    ASSERT(axes->text[coord].item_count == N);

    // TODO: more minor ticks between the major ticks.
    float lim[] = {-1};
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visual, VKL_PROP_TEXT, 0, N, text);
}

static void _axes_ticks_init(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        // Init structures.
        // TODO: constants
        axes->buf[coord] = calloc(128 * 16, sizeof(char)); // max ticks * max glyphs per tick
        axes->ticks[coord] = vkl_array(0, VKL_DTYPE_FLOAT);
        axes->text[coord] = vkl_array(0, VKL_DTYPE_STR);

        // Set the initial range.
        axes->range[coord][0] = -1;
        axes->range[coord][1] = +1;

        // Compute the ticks for these ranges.
        _axes_ticks(controller, coord);

        // Upload the data.
        _axes_upload(controller, coord);
    }
}

static void _axes_collision(VklController* controller, bool* update)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(update != NULL);

    // TODO
    // set update depending on whether there is a collision on the labels on that axis
    update[0] = (controller->panel->grid->canvas->frame_idx % 5000) == 0;
    update[1] = (controller->panel->grid->canvas->frame_idx % 5000) == 0;
}

static void _axes_range(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // set axes->range depending on coord
    VklTransform tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};
    tr = vkl_transform(controller->panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
    vkl_transform_apply(&tr, ll, pos_ll);
    vkl_transform_apply(&tr, ur, pos_ur);
    axes->range[coord][0] = pos_ll[coord];
    axes->range[coord][1] = pos_ur[coord];
    // log_info("%.3f %.3f", axes->range[coord][0], axes->range[coord][1]);
}

static void _axes_callback(VklController* controller, VklEvent ev)
{
    ASSERT(controller != NULL);
    _default_controller_callback(controller, ev);
    if (!controller->interacts[0].is_active)
        return;
    VklCanvas* canvas = controller->panel->grid->canvas;

    // Check label collision
    // DEBUG
    // bool update[2] = {true, true}; // whether X and Y axes must be updated or not
    bool update[2] = {false, false}; // whether X and Y axes must be updated or not
    // _axes_collision(controller, update);
    // if (!update[0] && !update[1])
    //     return;

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        if (!update[coord])
            continue;
        _axes_range(controller, coord);
        _axes_ticks(controller, coord);
        _axes_upload(controller, coord);
        canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    }
}

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
        VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, (int)coord);
        vkl_controller_visual(controller, visual);
        visual->priority = VKL_MAX_VISUAL_PRIORITY;

        visual->clip[0] = VKL_VIEWPORT_OUTER;
        visual->clip[1] = coord == 0 ? VKL_VIEWPORT_OUTER_BOTTOM : VKL_VIEWPORT_OUTER_LEFT;

        visual->transform[0] = visual->transform[1] =
            coord == 0 ? VKL_TRANSFORM_AXIS_X : VKL_TRANSFORM_AXIS_Y;

        // Text params.
        VklFontAtlas* atlas = vkl_font_atlas(ctx);
        ASSERT(strlen(atlas->font_str) > 0);
        vkl_visual_texture(visual, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);

        VklGraphicsTextParams params = {0};
        params.grid_size[0] = (int32_t)atlas->rows;
        params.grid_size[1] = (int32_t)atlas->cols;
        params.tex_size[0] = (int32_t)atlas->width;
        params.tex_size[1] = (int32_t)atlas->height;
        vkl_visual_data_buffer(visual, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);
    }
    // Add the axes data.
    _axes_ticks_init(controller);
}

static void _axes_destroy(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t i = 0; i < 2; i++)
    {
        vkl_array_destroy(&axes->ticks[i]);
        vkl_array_destroy(&axes->text[i]);
        FREE(axes->buf[i]);
    }
}



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VklTransform vkl_transform_inv(VklTransform tr)
{
    ASSERT(tr.scale[0] != 0);
    ASSERT(tr.scale[1] != 0);

    VklTransform tri = {0};
    tri.scale[0] = 1. / tr.scale[0];
    tri.scale[1] = 1. / tr.scale[1];
    tri.shift[0] = -tr.scale[0] * tr.shift[0];
    tri.shift[1] = -tr.scale[1] * tr.shift[1];
    return tri;
}



VklTransform vkl_transform_mul(VklTransform tr0, VklTransform tr1)
{
    VklTransform trm = {0};
    trm.scale[0] = tr0.scale[0] * tr1.scale[0];
    trm.scale[1] = tr0.scale[1] * tr1.scale[1];
    trm.shift[0] = tr0.shift[0] + tr1.shift[0] / tr0.scale[0];
    trm.shift[1] = tr0.shift[1] + tr1.shift[1] / tr0.scale[1];
    return trm;
}



VklTransform vkl_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout)
{
    VklTransform tr = {0};
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



void vkl_transform_apply(VklTransform* tr, dvec2 in, dvec2 out)
{
    ASSERT(tr != NULL);
    if (tr->scale[0] != 0)
        out[0] = tr->scale[0] * (in[0] - tr->shift[0]);
    if (tr->scale[1] != 0)
        out[1] = tr->scale[1] * (in[1] - tr->shift[1]);
}



VklTransform vkl_transform(VklPanel* panel, VklCDS source, VklCDS target)
{
    ASSERT(panel != NULL);
    VklTransform tr = {{1, 1}, {0, 0}}; // identity
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
        return vkl_transform_inv(vkl_transform(panel, target, source));
    }
    else if (target - source >= 2)
    {
        for (uint32_t k = source; k <= target - 1; k++)
        {
            tr = vkl_transform_mul(tr, vkl_transform(panel, (VklCDS)k, (VklCDS)(k + 1)));
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

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _scene_fill, canvas->scene);

    // HACK: we use a param of 1 here as a way of putting a lower priority, so that the
    // _scene_frame callback is called *after* the user FRAME callbacks. If the user callbacks call
    // vkl_visual_data(), the _scene_frame() callback will be called directly afterwards, in the
    // same frame.
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 1, _scene_frame, canvas->scene);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _upload_mvp, canvas->scene);


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
    /*
    TODO
    the callback should start to check whether the associated panel has the focus
    it can call interact callbacks
    update the data coords
    */
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
    VklPanel* panel = vkl_container_iter(&grid->panels);
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
