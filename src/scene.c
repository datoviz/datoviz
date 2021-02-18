#include "../include/datoviz/scene.h"
#include "../include/datoviz/builtin_visuals.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/interact.h"
#include "../include/datoviz/panel.h"
#include "../include/datoviz/transforms.h"
#include "../include/datoviz/visuals.h"
#include "../include/datoviz/vklite.h"
#include "axes.h"
#include "interact_utils.h"
#include "scene_utils.h"
#include "visuals_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Scene creation                                                                               */
/*************************************************************************************************/

DvzScene* dvz_scene(DvzCanvas* canvas, uint32_t n_rows, uint32_t n_cols)
{
    ASSERT(canvas != NULL);
    canvas->scene = calloc(1, sizeof(DvzScene));
    canvas->scene->canvas = canvas;
    canvas->scene->grid = dvz_grid(canvas, n_rows, n_cols);

    canvas->scene->visuals =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzVisual), DVZ_OBJECT_TYPE_VISUAL);

    canvas->scene->controllers = dvz_container(
        DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzController), DVZ_OBJECT_TYPE_CONTROLLER);

    // Scene update FIFO queue.
    canvas->scene->update_fifo = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);

    // INIT callback
    dvz_event_callback(canvas, DVZ_EVENT_INIT, 0, DVZ_EVENT_MODE_SYNC, _scene_init, canvas->scene);

    // RESIZE callback
    dvz_event_callback(
        canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _scene_resize, canvas->scene);

    // REFILL callback
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _scene_fill, canvas->scene);

    // FRAME callbacks

    // HACK: we use a param of 1 here as a way of putting a lower priority, so that the
    // _scene_frame callback is called *after* the user FRAME callbacks. If the user callbacks call
    // dvz_visual_data(), the _scene_frame() callback will be called directly afterwards, in the
    // same frame.
    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 1, DVZ_EVENT_MODE_SYNC, _scene_frame, canvas->scene);

    // Upload the MVP struct to the panels.
    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _upload_mvp, canvas->scene);


    return canvas->scene;
}



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

DvzController dvz_controller(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    DvzController controller = {0};
    controller.panel = panel;
    controller.callback = _default_controller_callback;
    dvz_obj_created(&controller.obj);
    return controller;
}



void dvz_controller_visual(DvzController* controller, DvzVisual* visual)
{
    ASSERT(controller != NULL);
    controller->visuals[controller->visual_count++] = visual;
}



void dvz_controller_interact(DvzController* controller, DvzInteractType type)
{
    ASSERT(controller != NULL);
    DvzCanvas* canvas = controller->panel->grid->canvas;
    controller->interacts[controller->interact_count++] = dvz_interact_builtin(canvas, type);
}



void dvz_controller_callback(DvzController* controller, DvzControllerCallback callback)
{
    ASSERT(controller != NULL);
    controller->callback = callback;
}



void dvz_controller_update(DvzController* controller)
{
    ASSERT(controller != NULL);
    //
    /*
    TODO
    called whenever the visuals need to refresh their data with the current viewport and data
    coords
    loop over all visuals in the panel
    call dvz_visual_update()
    */
}



void dvz_controller_destroy(DvzController* controller)
{
    if (controller->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
        return;
    ASSERT(controller != NULL);

    // TODO: controller destruction callback
    switch (controller->type)
    {
    case DVZ_CONTROLLER_AXES_2D:
        _axes_destroy(controller);
        break;

    default:
        break;
    }

    // Destroy the interacts.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        dvz_interact_destroy(&controller->interacts[i]);
    }
    dvz_obj_destroyed(&controller->obj);
}



DvzController dvz_controller_builtin(DvzPanel* panel, DvzControllerType type, int flags)
{
    ASSERT(panel != NULL);
    DvzController controller = dvz_controller(panel);
    controller.type = type;
    controller.flags = flags;

    switch (type)
    {

    case DVZ_CONTROLLER_NONE:
        break;

    case DVZ_CONTROLLER_PANZOOM:
        dvz_controller_interact(&controller, DVZ_INTERACT_PANZOOM);
        break;

    case DVZ_CONTROLLER_ARCBALL:
        dvz_controller_interact(&controller, DVZ_INTERACT_ARCBALL);
        break;

    case DVZ_CONTROLLER_CAMERA:
        dvz_controller_interact(&controller, DVZ_INTERACT_FLY);
        break;

    case DVZ_CONTROLLER_AXES_2D:
        dvz_controller_interact(&controller, DVZ_INTERACT_PANZOOM);
        dvz_controller_callback(&controller, _axes_callback);
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

DvzPanel*
dvz_scene_panel(DvzScene* scene, uint32_t row, uint32_t col, DvzControllerType type, int flags)
{
    /*
    the flags gets passed to:
    - controller (controller params)
    - data coords (transform)
    */
    ASSERT(scene != NULL);
    DvzPanel* panel = dvz_panel(&scene->grid, row, col);
    panel->scene = scene;

    DvzController* controller = dvz_container_alloc(&scene->controllers);
    *controller = dvz_controller_builtin(panel, type, flags);
    controller->flags = flags;
    panel->controller = controller;

    // // HACK: white background if axes controller.
    // if (type == DVZ_CONTROLLER_AXES_2D)
    //     dvz_canvas_clear_color(scene->canvas, 1, 1, 1);

    // Set panel transform flags depending on the controller type.
    flags = _transform_flags(type, flags);
    panel->data_coords.flags = flags;

    panel->scene = scene;
    panel->prority_max = DVZ_MAX_VISUAL_PRIORITY;
    return panel;
}



static void _add_visual(DvzPanel* panel, DvzVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    // Add the visual to the panel.
    dvz_panel_visual(panel, visual);

    // Bind the common buffers (MVP, viewport).
    _common_data(panel, visual);

    // Put all graphics pipeline in the inner viewport by default.
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
        visual->clip[pidx] = DVZ_VIEWPORT_INNER;

    // Update the panel data coords as a function of the visual's data.
    if (panel->scene->canvas->app->is_running)
        _enqueue_visual_changed(panel, visual);
}



DvzVisual* dvz_blank_visual(DvzScene* scene, int flags)
{
    ASSERT(scene != NULL);
    DvzVisual* visual = dvz_container_alloc(&scene->visuals);
    *visual = dvz_visual(scene->canvas);
    visual->flags = flags;
    return visual;
}



DvzVisual* dvz_scene_visual(DvzPanel* panel, DvzVisualType type, int flags)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller != NULL);
    ASSERT(type != DVZ_VISUAL_CUSTOM);
    ASSERT(panel->scene != NULL);

    // Create a blank visual.
    DvzVisual* visual = dvz_blank_visual(panel->scene, flags);

    // Builtin visual.
    dvz_visual_builtin(visual, type, flags);

    // Add it to the panel.
    _add_visual(panel, visual);

    return visual;
}



void dvz_custom_visual(DvzPanel* panel, DvzVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    // Add common sources and props.
    _common_sources(visual);
    _common_props(visual);

    // Bind the scene data (mvp, viewport).
    _add_visual(panel, visual);
}



DvzGraphics* dvz_blank_graphics(DvzScene* scene, int flags)
{
    ASSERT(scene != NULL);
    DvzCanvas* canvas = scene->canvas;
    ASSERT(canvas != NULL);
    DvzGraphics* graphics = dvz_container_alloc(&canvas->graphics);
    *graphics = dvz_graphics(canvas->gpu);
    graphics->type = DVZ_GRAPHICS_CUSTOM;
    graphics->flags = flags;

    // Common slots.
    dvz_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    dvz_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport

    // Renderpass.
    dvz_graphics_renderpass(graphics, &canvas->renderpass, 0);

    return graphics;
}



void dvz_custom_graphics(DvzVisual* visual, DvzGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(dvz_obj_is_created(&graphics->obj));
    ASSERT(graphics->type == DVZ_GRAPHICS_CUSTOM);

    // Add the graphics to the visual.
    dvz_visual_graphics(visual, graphics);

    // Vertex buffer source.
    // HACK: only support 1 attribute binding at the moment.
    VkDeviceSize struct_size = graphics->vertex_bindings[0].stride;
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, struct_size, 0);

    // Other sources will be set by dvz_custom_visual().
}



/*************************************************************************************************/
/*  Interact functions                                                                           */
/*************************************************************************************************/

static DvzMVP* _panel_mvp(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    if (panel->controller->interact_count > 0)
        return &panel->controller->interacts[0].mvp;
    return NULL;
}



void dvz_camera_pos(DvzPanel* panel, vec3 pos)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller->type == DVZ_CONTROLLER_CAMERA);
    DvzMVP* mvp = _panel_mvp(panel);
    DvzCamera* camera = &panel->controller->interacts[0].u.c;
    glm_vec3_copy(pos, camera->eye);
    _camera_update_mvp(panel->viewport, camera, mvp);
}



void dvz_camera_look(DvzPanel* panel, vec3 center)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller->type == DVZ_CONTROLLER_CAMERA);
    DvzMVP* mvp = _panel_mvp(panel);
    DvzCamera* camera = &panel->controller->interacts[0].u.c;
    glm_vec3_sub(center, camera->eye, camera->forward);
    _camera_update_mvp(panel->viewport, camera, mvp);
}



void dvz_arcball_rotate(DvzPanel* panel, float angle, vec3 axis)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller->type == DVZ_CONTROLLER_ARCBALL);
    DvzMVP* mvp = _panel_mvp(panel);
    DvzArcball* arcball = &panel->controller->interacts[0].u.a;
    glm_quatv(arcball->rotation, angle, axis);
    _arcball_update_mvp(panel->viewport, arcball, mvp);
}



// void dvz_axes_flags(DvzPanel* panel, int flags)
// {
//     ASSERT(panel != NULL);
//     if (panel->controller->type != DVZ_CONTROLLER_AXES_2D)
//     {
//         log_error("panel doesn't have an axes 2D controller");
//         return;
//     }
//     DvzVisual* vx = panel->controller->visuals[0];
//     DvzVisual* vy = panel->controller->visuals[1];
// }



/*************************************************************************************************/
/*  Scene destruction                                                                            */
/*************************************************************************************************/

void dvz_scene_destroy(DvzScene* scene)
{
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    // Destroy all panels.
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzPanel* panel = NULL;
    while (iter.item != NULL)
    {
        panel = iter.item;
        if (panel->obj.status == DVZ_OBJECT_STATUS_NONE)
            break;
        // This also destroys all visuals in the panel.
        dvz_panel_destroy(panel);
        dvz_container_iter(&iter);
    }

    // Destroy all controllers.
    CONTAINER_DESTROY_ITEMS(DvzController, scene->controllers, dvz_controller_destroy)
    dvz_container_destroy(&scene->controllers);

    dvz_fifo_destroy(&scene->update_fifo);

    dvz_container_destroy(&scene->visuals);
    dvz_obj_destroyed(&scene->obj);
    FREE(scene);
}
