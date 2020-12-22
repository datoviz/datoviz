#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _common_data(VklPanel* panel, VklVisual* visual, VklViewportClip clip)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    // Binding 0: MVP binding
    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 0, panel->br_mvp);

    // Binding 1: viewport
    VklBufferRegions br_viewport = {0};
    switch (clip)
    {
    case VKL_VIEWPORT_INNER:
        br_viewport = panel->br_inner;
        break;
    case VKL_VIEWPORT_OUTER:
        br_viewport = panel->br_outer;
        break;
    case VKL_VIEWPORT_FULL:
        br_viewport = panel->br_full;
        break;
    default:
        log_warn("viewport type %d not implemented", clip);
        break;
    }

    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 1, br_viewport);

    // Binding 2: color texture
    // TODO
    vkl_visual_data_texture(visual, VKL_PROP_COLOR_TEXTURE, 0, 1, 1, 1, NULL);
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
            vkl_panel_update(panel);
            ASSERT(is_obj_created(&panel->obj));

            // Find the panel viewport.
            viewport = vkl_panel_viewport(panel, VKL_VIEWPORT_FULL);
            vkl_cmd_viewport(cmds, img_idx, viewport.viewport);
            // log_debug("%d %d %d %d", cmds, i, j, img_idx);
            // Go through all visuals in the panel.
            for (uint32_t k = 0; k < panel->visual_count; k++)
            {
                vkl_visual_fill_event(
                    panel->visuals[k], ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
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
        if (panel->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
        {
            viewport = panel->viewport_inner;
            for (uint32_t j = 0; j < panel->visual_count; j++)
            {
                // TODO: data coords
                vkl_visual_update(panel->visuals[j], viewport, (VklDataCoords){0}, NULL);
            }
            panel->obj.status = VKL_OBJECT_STATUS_CREATED;
        }
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
        VklViewport viewport = controller->panel->viewport_inner;
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

    VklViewport viewport = panel->viewport_inner;

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
    panel->controller = controller;
    panel->scene = scene;
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
    *visual = vkl_visual_builtin(scene->canvas, type, flags);

    // Add it to the panel.
    vkl_panel_visual(panel, visual);

    // Bind the common buffers (MVP, viewport, color texture).
    // TODO: viewport type
    _common_data(panel, visual, VKL_VIEWPORT_INNER);

    return visual;
}



/*************************************************************************************************/
/*  Scene destruction                                                                            */
/*************************************************************************************************/

void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
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
