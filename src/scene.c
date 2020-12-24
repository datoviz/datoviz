#include "../include/visky/scene.h"
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

        for (uint32_t j = 0; j < grid->panel_count; j++)
        {
            panel = &grid->panels[j];

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

                    DBG(k);

                    // Update visual VklViewport struct and upload it.
                    _update_visual_viewport(panel, visual);

                    vkl_visual_fill_event(
                        visual, ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
                }
            }
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

    VklPanel* panel = NULL;
    VklViewport viewport = {0};

    // Go through all panels that need to be updated.
    bool to_update = false;
    for (uint32_t i = 0; i < scene->grid.panel_count; i++)
    {
        panel = &scene->grid.panels[i];

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
            if (to_update || panel->visuals[j]->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            {
                // TODO: data coords
                vkl_visual_update(panel->visuals[j], viewport, (VklDataCoords){0}, NULL);
            }
        }
        panel->obj.status = VKL_OBJECT_STATUS_CREATED;
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

    VklPanel* panel = NULL;
    // VklViewport viewport = {0};
    VklInteract* interact = NULL;
    VklController* controller = NULL;
    VklBufferRegions* br = NULL;

    // Go through all panels that need to be updated.
    for (uint32_t i = 0; i < scene->grid.panel_count; i++)
    {
        panel = &scene->grid.panels[i];
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
            // NOTE: we need to update the uniform buffer at every frame
            br = &panel->br_mvp;

            // GPU transfer happens here:
            void* aligned = aligned_repeat(br->size, &interact->mvp, 1, br->alignment);
            vkl_buffer_regions_upload(br, canvas->swapchain.img_idx, aligned);
            FREE(aligned);
        }
    }
}



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

static void _tick_format(double value, char* out_text) { snprintf(out_text, 16, "%.1f", value); }

static void _axes_data(VklController* controller)
{
    ASSERT(controller != NULL);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    const uint32_t N = 4 * 5 + 1;
    axes->xticks = calloc(N, sizeof(float));
    axes->yticks = calloc(N, sizeof(float));
    axes->str_buf = calloc(N * 16, sizeof(char));
    axes->text = calloc(N, sizeof(char*));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -2 + 4 * (float)i / (N - 1);
        axes->xticks[i] = t;
        axes->yticks[i] = t;
        axes->text[i] = &axes->str_buf[16 * i];
        _tick_format(t, axes->text[i]);
    }

    ASSERT(controller->visual_count == 2);
    VklVisual* visualx = controller->visuals[0];
    VklVisual* visualy = controller->visuals[1];
    float* xticks = axes->xticks;
    float* yticks = axes->yticks;

    // Minor ticks.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, yticks);

    // Major ticks.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, yticks);

    // Grid.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, yticks);

    // Lim.
    float lim[] = {-1};
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);

    // Text.
    vkl_visual_data(visualx, VKL_PROP_TEXT, 0, N, axes->text);
    vkl_visual_data(visualy, VKL_PROP_TEXT, 0, N, axes->text);
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

    VklVisual* visualx = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_X);
    VklVisual* visualy = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_Y);

    vkl_controller_visual(controller, visualx);
    vkl_controller_visual(controller, visualy);

    visualx->priority = VKL_MAX_VISUAL_PRIORITY;
    visualy->priority = VKL_MAX_VISUAL_PRIORITY;

    visualx->clip[0] = VKL_VIEWPORT_OUTER;
    visualy->clip[0] = VKL_VIEWPORT_OUTER;

    visualx->clip[1] = VKL_VIEWPORT_OUTER_BOTTOM;
    visualy->clip[1] = VKL_VIEWPORT_OUTER_LEFT;

    visualx->transform[0] = VKL_TRANSFORM_AXIS_X;
    visualx->transform[1] = VKL_TRANSFORM_AXIS_X;

    visualy->transform[0] = VKL_TRANSFORM_AXIS_Y;
    visualy->transform[1] = VKL_TRANSFORM_AXIS_Y;

    // Text params.
    VklFontAtlas* atlas = vkl_font_atlas(ctx);
    ASSERT(strlen(atlas->font_str) > 0);
    vkl_visual_texture(visualx, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);
    vkl_visual_texture(visualy, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);

    VklGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    vkl_visual_data_buffer(visualx, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);
    vkl_visual_data_buffer(visualy, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);

    // Add the axes data.
    _axes_data(controller);
}

static void _axes_destroy(VklController* controller)
{
    ASSERT(controller != NULL);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    FREE(axes->xticks);
    FREE(axes->yticks);
    FREE(axes->text);
    FREE(axes->str_buf);
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
        log_error("not implemented yet");
        // TODO
        // ll[0] = axes->xscale_orig.vmin;
        // ll[1] = axes->yscale_orig.vmin;
        // ur[0] = axes->xscale_orig.vmax;
        // ur[1] = axes->yscale_orig.vmax;
        // panzoom = axes->panzoom_inner;
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

    INSTANCES_INIT(
        VklVisual, canvas->scene, visuals, max_visuals, //
        VKL_MAX_VISUALS, VKL_OBJECT_TYPE_VISUAL)

    INSTANCES_INIT(
        VklController, canvas->scene, controllers, max_controllers, //
        VKL_MAX_CONTROLLERS, VKL_OBJECT_TYPE_CONTROLLER)

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _scene_fill, canvas->scene);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _scene_frame, canvas->scene);
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
        log_error("unknown controller type");
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

    case VKL_CONTROLLER_PANZOOM:
        vkl_controller_interact(&controller, VKL_INTERACT_PANZOOM);
        break;

    case VKL_CONTROLLER_ARCBALL:
        vkl_controller_interact(&controller, VKL_INTERACT_ARCBALL);
        break;

    case VKL_CONTROLLER_AXES_2D:
        vkl_controller_interact(&controller, VKL_INTERACT_PANZOOM);
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
    INSTANCE_NEW(VklController, controller, scene->controllers, scene->max_controllers)
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
    INSTANCE_NEW(VklVisual, visual, scene->visuals, scene->max_visuals)

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

    // Destroy all panels.
    for (uint32_t i = 0; i < scene->grid.panel_count; i++)
    {
        if (scene->grid.panels[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        // Destroy all visuals in the panel.
        vkl_panel_destroy(&scene->grid.panels[i]);
    }

    // Destroy all controllers.
    for (uint32_t i = 0; i < scene->max_controllers; i++)
    {
        if (scene->controllers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_controller_destroy(&scene->controllers[i]);
    }
    INSTANCES_DESTROY(scene->visuals)
    obj_destroyed(&scene->obj);
    FREE(scene);
}
