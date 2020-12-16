#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// static const mat4 MAT4_IDENTITY = GLM_MAT4_IDENTITY_INIT;

static void _common_data(VklVisual* visual)
{
    // vkl_visual_data(visual, VKL_PROP_MODEL, 0, 1, MAT4_IDENTITY);
    // vkl_visual_data(visual, VKL_PROP_VIEW, 0, 1, MAT4_IDENTITY);
    // vkl_visual_data(visual, VKL_PROP_PROJ, 0, 1, MAT4_IDENTITY);

    // vkl_upload_buffers_immediate(
    //             visual->canvas, interact->br, true, 0, interact->br.size, &interact->mvp);

    // TODO: color texture
    vkl_visual_data_texture(visual, VKL_PROP_COLOR_TEXTURE, 0, 1, 1, 1, NULL);

    // TODO: viewport uniform
    vkl_visual_data_buffer(visual, VKL_SOURCE_UNIFORM, 1, 0, 1, 1, NULL);
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

        vkl_visual_fill_begin(canvas, cmds, img_idx);

        // We only fill the PANEL command buffers.
        for (uint32_t j = 0; j < grid->panel_count; j++)
        {
            panel = &grid->panels[j];
            ASSERT(is_obj_created(&panel->obj));

            // Find the panel viewport.
            viewport = vkl_panel_viewport(panel);
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
            log_debug("update data for visuals in panel %d,%d", panel->row, panel->col);
            viewport = vkl_panel_viewport(panel);
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
        float delay = canvas->clock.elapsed - interact->last_update;

        // Update the interact using the current panel's viewport.
        VklViewport viewport = vkl_panel_viewport(controller->panel);
        vkl_interact_update(interact, viewport, &canvas->mouse, &canvas->keyboard);

        if (interact->to_update && delay > VKY_INTERACT_MIN_DELAY)
        {
            vkl_upload_buffers_immediate(
                canvas, interact->br, true, 0, interact->br.size, &interact->mvp);
            interact->last_update = canvas->clock.elapsed;
        }
    }
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
    ASSERT(controller != NULL);
    //
}



VklController vkl_controller_builtin(VklPanel* panel, VklControllerType type, int flags)
{
    ASSERT(panel != NULL);
    VklController controller = vkl_controller(panel);

    switch (type)
    {

    case VKL_CONTROLLER_PANZOOM:
        vkl_controller_interact(&controller, VKL_INTERACT_PANZOOM);
        // TODO: default callback that just calls the callbacks of all interacts
        break;

    default:
        break;
    }

    return controller;
}



/*************************************************************************************************/
/*  High-level functions                                                                         */
/*************************************************************************************************/

VklController* vkl_panel_controller(VklPanel* panel, VklControllerType type, int flags)
{
    ASSERT(panel != NULL);
    VklScene* scene = panel->grid->canvas->scene;
    INSTANCE_NEW(VklController, controller, scene->controllers, scene->max_controllers)
    *controller = vkl_controller_builtin(panel, type, flags);
    panel->controller = controller;
    return controller;
}



VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags)
{
    ASSERT(scene != NULL);
    VklPanel* panel = vkl_panel(&scene->grid, row, col);
    vkl_panel_controller(panel, type, flags);

    // At initialization, must update all visuals data.
    panel->scene = scene;
    _panel_to_update(panel);

    return panel;
}



VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags)
{
    ASSERT(panel != NULL);
    // TODO
    ASSERT(panel->controller != NULL);
    VklScene* scene = panel->grid->canvas->scene;
    INSTANCE_NEW(VklVisual, visual, scene->visuals, scene->max_visuals)
    *visual = vkl_visual_builtin(scene->canvas, type, flags);
    vkl_panel_visual(panel, visual, VKL_VIEWPORT_INNER);

    // MVP, viewport, color texture binding data.
    _common_data(visual);

    // TODO: make sure the first interact exists before binding the buffer, otherwise create one?
    VklInteract* interact = &panel->controller->interacts[0];
    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 0, interact->br);
    vkl_upload_buffers_immediate(
        scene->canvas, interact->br, true, 0, interact->br.size, &interact->mvp);

    vkl_canvas_callback(scene->canvas, VKL_PRIVATE_EVENT_REFILL, 0, _scene_fill, scene);
    vkl_canvas_callback(scene->canvas, VKL_PRIVATE_EVENT_FRAME, 0, _scene_frame, scene);

    return visual;
}



/*************************************************************************************************/
/*  Scene destruction                                                                            */
/*************************************************************************************************/

void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
    INSTANCES_DESTROY(scene->visuals)
    obj_destroyed(&scene->obj);
    FREE(scene);
}
