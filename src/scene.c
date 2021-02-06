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
#include "visuals_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _viewport_print(DvzViewport v)
{
    log_info(
        "viewport clip %d, interact axis %d, margin top %.2f, wf %d, ws %d, viewport "
        "%.2f %.2f %.2f %.2f %.2f %.2f",
        v.clip, v.interact_axis, v.margins[0], v.size_framebuffer[0], v.size_screen[0],
        v.viewport.width, v.viewport.height, v.viewport.x, v.viewport.y, v.viewport.minDepth,
        v.viewport.maxDepth);
}



static inline void _visual_request(DvzVisual* visual, DvzPanel* panel, DvzVisualRequest req)
{
    if (visual != NULL)
    {
        visual->obj.request = (int)req;
    }
    if (panel != NULL)
    {
        // Update the panel request when one of its visual need to be updated. Also mark the scene
        // as needing an update. The scene frame callback checks, at every frame, what needs to be
        // updated. The scene and panel requests lets it avoid doing a full scan at every frame.
        panel->obj.request = (int)req;
        if (panel->scene != NULL)
            panel->scene->obj.request = (int)req;
    }
    if (req == DVZ_VISUAL_REQUEST_REFILL)
        dvz_canvas_to_refill(visual->canvas);
}



static inline void _visual_set(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    visual->obj.request = DVZ_VISUAL_REQUEST_SET;
}



static inline bool _visual_has_request(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    return visual->obj.request != DVZ_VISUAL_REQUEST_SET &&
           visual->obj.request != DVZ_VISUAL_REQUEST_NOT_SET;
}



static inline void _panel_set(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    panel->obj.request = DVZ_VISUAL_REQUEST_SET;
}



static inline bool _panel_has_request(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    return panel->obj.request == DVZ_VISUAL_REQUEST_SET ||
           panel->obj.request == DVZ_VISUAL_REQUEST_NOT_SET;
}



static inline void _scene_set(DvzScene* scene)
{
    ASSERT(scene != NULL);
    scene->obj.request = DVZ_VISUAL_REQUEST_SET;
}



static void _visual_detect_item_count_change(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzSource* source = NULL;
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        // Detect a change in vertex_count.
        source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, pidx);
        if (source->arr.item_count != visual->prev_vertex_count[pidx])
        {
            log_debug("automatic detection of a change in vertex count, will trigger full refill");
            _visual_request(visual, NULL, DVZ_VISUAL_REQUEST_REFILL);
            visual->prev_vertex_count[pidx] = source->arr.item_count;
        }

        // Detect a change in index_count.
        source = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, pidx);
        if (source != NULL && source->arr.item_count != visual->prev_index_count[pidx])
        {
            log_debug("automatic detection of a change in index count, will trigger full refill");
            _visual_request(visual, NULL, DVZ_VISUAL_REQUEST_REFILL);
            visual->prev_index_count[pidx] = source->arr.item_count;
        }
    }
}



// Return the box surrounding all POS props of a visual.
static DvzBox _visual_box(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    DvzProp* prop = NULL;
    DvzArray* arr = NULL;

    // The POS props that will need to be transformed.
    uint32_t n_pos_props = 0;

    DvzBox boxes[32] = {0}; // max number of props of the same type

    // Gather all non-empty POS props, and get the bounding box on each.
    for (uint32_t i = 0; i < 32; i++)
    {
        prop = dvz_prop_get(visual, DVZ_PROP_POS, i);
        if (prop == NULL)
            break;
        arr = &prop->arr_orig;
        ASSERT(arr != NULL);
        if (arr->item_count == 0)
            continue;
        boxes[n_pos_props++] = _box_bounding(arr);
    }

    if (n_pos_props == 0)
        return DVZ_BOX_NDC;

    // Merge the boxes of the visual.
    DvzBox box = _box_merge(n_pos_props, boxes);
    return box;
}



// Renormalize a POS prop.
static void _transform_pos_prop(DvzDataCoords coords, DvzProp* prop)
{
    DvzArray* arr = NULL;
    DvzArray* arr_tr = NULL;

    arr = &prop->arr_orig;
    arr_tr = &prop->arr_trans;
    if (arr->item_count == 0)
    {
        log_warn("empty POS prop, skipping renormalization");
        return;
    }

    // Create the transformed prop array.
    *arr_tr = dvz_array(arr->item_count, arr->dtype);
    dvz_transform_data(coords, arr, arr_tr, false);
}



// Transpose a POS or NORMAL prop.
static void _transpose_pos(DvzCDSTranspose transpose, DvzProp* prop)
{
    if (transpose == DVZ_CDS_TRANSPOSE_NONE)
        return;

    ASSERT(prop != NULL);
    DvzArray* arr = &prop->arr_orig;
    ASSERT(arr != NULL);
    if (arr->item_count == 0)
        return;

    // Make the transposition.
    void* item = NULL;
    log_debug("transposing %d elements to CDS transpose %d", arr->item_count, transpose);
    for (uint32_t i = 0; i < arr->item_count; i++)
    {
        item = dvz_array_item(arr, i);
        if (prop->dtype == DVZ_DTYPE_DVEC3)
            _transpose_dvec3(transpose, (dvec3*)item, (dvec3*)item);
        else if (prop->dtype == DVZ_DTYPE_VEC3)
            _transpose_vec3(transpose, (vec3*)item, (vec3*)item);
    }
}



// Transpose the POS and NORMAL panels, if needed.
static void _transpose_visual(DvzPanel* panel, DvzVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    DvzProp* prop = NULL;
    // Go through all visual props.
    DvzContainerIterator iter = dvz_container_iterator(&visual->props);
    while (iter.item != NULL)
    {
        prop = iter.item;
        // CDS transposition.
        if (prop->obj.request == 0 &&
            (prop->prop_type == DVZ_PROP_POS || prop->prop_type == DVZ_PROP_NORMAL))
        {
            _transpose_pos(panel->data_coords.transpose, prop);
            prop->obj.request = 1; // HACK: we only transpose props once, after they've been set.
        }

        dvz_container_iter(&iter);
    }
}



// Renormalize all POS props of all visuals in the panel.
static void _panel_normalize_visuals(DvzPanel* panel, DvzBox box)
{
    ASSERT(panel != NULL);
    DvzVisual* visual = NULL;
    DvzProp* prop = NULL;
    DvzContainerIterator iter;

    // Update the data coords box.
    panel->data_coords.box = box;

    // Go through all visuals in the panel.
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        visual = panel->visuals[i];

        // NOTE: skip visuals that should not be transformed.
        if ((visual->flags & DVZ_VISUAL_FLAGS_TRANSFORM_NONE) != 0)
            continue;

        // Go through all visual props.
        iter = dvz_container_iterator(&visual->props);
        while (iter.item != NULL)
        {
            prop = iter.item;
            // Transform all POS props with the panel data coordinates.
            if (prop->prop_type == DVZ_PROP_POS)
            {
                _transform_pos_prop(panel->data_coords, prop);

                // Mark the visual has needing data update.
                // visual->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
                _visual_request(visual, panel, DVZ_VISUAL_REQUEST_UPLOAD);
            }

            dvz_container_iter(&iter);
        }
    }
}



// Update the DvzPanel.data_coords struct when a new visual is added.
static void _panel_visual_added(DvzPanel* panel, DvzVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    DvzDataCoords* coords = &panel->data_coords;

    // NOTE: skip visuals that should not be transformed.
    if ((visual->flags & DVZ_VISUAL_FLAGS_TRANSFORM_NONE) != 0)
        return;

    // Transpose the POS and NORMAL props, if needed.
    _transpose_visual(panel, visual);

    // Get the visual box.
    DvzBox box = _visual_box(visual);

    // Merge the visual box with the existing box.
    box = _box_merge(2, (DvzBox[]){coords->box, box});

    // Make the box square if needed.
    if ((coords->flags & DVZ_TRANSFORM_FLAGS_FIXED_ASPECT) != 0)
        box = _box_cube(box);

    // If the panel box has changed, renormalize all visuals.
    if (memcmp(&box, &coords->box, sizeof(DvzBox)) != 0)
    {
        // Renormalize all visuals in the panel.
        _panel_normalize_visuals(panel, box);
    }
}



// Update the DvzPanel.data_coords struct as a function of all of the visuals data.
static void _panel_normalize(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    log_debug("full panel normalization on %d visuals", panel->visual_count);

    DvzDataCoords* coords = &panel->data_coords;
    DvzBox* boxes = calloc(panel->visual_count, sizeof(DvzBox));
    uint32_t count = 0;

    // Get the bounding box of each visual.
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        ASSERT(panel->visuals[i] != NULL);

        // Transpose the POS and NORMAL props, if needed.
        _transpose_visual(panel, panel->visuals[i]);

        // NOTE: skip visuals that should not be transformed.
        if ((panel->visuals[i]->flags & DVZ_VISUAL_FLAGS_TRANSFORM_NONE) == 0)
        {
            boxes[count++] = _visual_box(panel->visuals[i]);
        }
    }

    // Merge the visual box with the existing box.
    DvzBox box = _box_merge(count, boxes);

    // Make the box square if needed.
    if ((coords->flags & DVZ_TRANSFORM_FLAGS_FIXED_ASPECT) != 0)
        box = _box_cube(box);

    // Renormalize all visuals in the panel.
    _panel_normalize_visuals(panel, box);

    // Update the axes.
    if (panel->controller->type == DVZ_CONTROLLER_AXES_2D)
    {
        _axes_set(panel->controller, panel->data_coords.box);
    }

    FREE(boxes);
}



static void _update_visual_viewport(DvzPanel* panel, DvzVisual* visual)
{
    visual->viewport = panel->viewport;
    // Each graphics pipeline in the visual has its own transform/clip viewport options
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        visual->viewport.interact_axis = (int32_t)visual->interact_axis[pidx];
        visual->viewport.clip = visual->clip[pidx];
        ASSERT(visual->viewport.viewport.minDepth < visual->viewport.viewport.maxDepth);
        // NOTE: here we make the assumption that there is exactly 1 viewport per graphics
        // pipeline, such that the source idx corresponds to the pipeline idx.
        // _viewport_print(visual->viewport);
        dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VIEWPORT, pidx, 0, 1, 1, &visual->viewport);
    }
}



// Bind the MVP and viewport buffers.
static void _common_data(DvzPanel* panel, DvzVisual* visual)
{
    ASSERT(panel != NULL);
    ASSERT(visual != NULL);

    // Binding 0: MVP binding
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_MVP, 0, panel->br_mvp);

    // Binding 1: viewport
    _update_visual_viewport(panel, visual);
}



static void _scene_fill(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("scene fill");
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;

    DvzViewport viewport = {0};
    DvzCommands* cmds = NULL;
    DvzPanel* panel = NULL;
    DvzContainerIterator iter;
    DvzVisual* visual = NULL;
    uint32_t img_idx = 0;

    // Go through all the current command buffers.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        cmds = ev.u.rf.cmds[i];
        img_idx = ev.u.rf.img_idx;

        log_trace("visual fill cmd %d begin %d", i, img_idx);
        dvz_visual_fill_begin(canvas, cmds, img_idx);

        iter = dvz_container_iterator(&grid->panels);
        while (iter.item != NULL)
        {
            panel = iter.item;
            // Update the panel.
            dvz_panel_update(panel);
            ASSERT(dvz_obj_is_created(&panel->obj));

            // Find the panel viewport.
            viewport = dvz_panel_viewport(panel);
            dvz_cmd_viewport(cmds, img_idx, viewport.viewport);

            // Update visual DvzViewport struct and upload it, only once per visual.
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

                    dvz_visual_fill_event(
                        visual, ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
                }
            }

            dvz_container_iter(&iter);
        }
        dvz_visual_fill_end(canvas, cmds, img_idx);
    }
}



static void _scene_frame(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;
    DvzViewport viewport = {0};

    // Go through all panels that need to be updated.
    // bool to_update = false;
    DvzPanel* panel = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzSource* source = NULL;
    DvzVisual* visual = NULL;
    while (iter.item != NULL)
    {
        panel = iter.item;

        // Interactivity.
        if (panel->controller != NULL && panel->controller->callback != NULL)
        {
            // TODO: event struct
            panel->controller->callback(panel->controller, (DvzEvent){0});
        }

        // TODO
        // // Handle floating panels.
        // if (panel->mode == DVZ_PANEL_FLOATING &&               //
        //     canvas->mouse.cur_state == DVZ_MOUSE_STATE_DRAG && //
        //     dvz_panel_contains(panel, canvas->mouse.press_pos))
        // {
        //     float x = canvas->mouse.cur_pos[0] / canvas->window->width;
        //     float y = canvas->mouse.cur_pos[1] / canvas->window->height;
        //     log_info("moving panel to %.1fx%.1f", x, y);
        //     dvz_panel_pos(panel, x, y);
        // }

        // Initial normalization of all visuals in the panel.
        if (canvas->frame_idx == 0)
            _panel_normalize(panel);

        // Process panel and visual requests.

        // NOTE: dvz_visual_data() functions have no notion of panel and cannot update its request.
        // So we are forced to scan through all panels and visuals to find visuals that need an
        // update. That's why the following is commented out.
        // // Skip the panel if there is no request.
        // if (!_panel_has_request(panel))
        //     continue;

        // to_update = panel->obj.status == DVZ_OBJECT_STATUS_NEED_UPDATE;
        viewport = panel->viewport;
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            visual = panel->visuals[j];

            // First frame:
            if (canvas->frame_idx == 0)
            {
                // Set all visuals to be updated.
                _visual_request(visual, panel, DVZ_VISUAL_REQUEST_UPLOAD);

                // Initialize prev_vertex_count and prev_index_count.
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, pidx);
                    visual->prev_vertex_count[pidx] = source->arr.item_count;

                    source = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL)
                        visual->prev_index_count[pidx] = source->arr.item_count;
                }
            }

            // Skip the visual if there is no request.
            if (!_visual_has_request(visual))
                continue;

            // Process visual upload.
            if (visual->obj.request == DVZ_VISUAL_REQUEST_UPLOAD)
            {
                // Update the visual's data.
                dvz_visual_update(visual, viewport, panel->data_coords, NULL);

                // Detect whether the number of vertices/indices has changed, in which case we need
                // a refill in the current frame.
                _visual_detect_item_count_change(visual);

                // The visual no longer needs UPLOAD.
                _visual_set(visual);
            }
        }

        // Mark the panel as no longer needing to be updated.
        _panel_set(panel);

        dvz_container_iter(&iter);
    }

    // Mark the scene as no longer needing to be updated.
    _scene_set(scene);
}



static void _upload_mvp(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    DvzInteract* interact = NULL;
    DvzController* controller = NULL;
    // DvzBufferRegions* br = NULL;

    // Go through all panels that need to be updated.
    DvzPanel* panel = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    while (iter.item != NULL)
    {
        panel = iter.item;
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
            dvz_upload_buffers(canvas, panel->br_mvp, 0, panel->br_mvp.size, &interact->mvp);
        }
        dvz_container_iter(&iter);
    }
}



static int _transform_flags(DvzControllerType type, int flags)
{
    switch (type)
    {

    case DVZ_CONTROLLER_ARCBALL:
    case DVZ_CONTROLLER_CAMERA:
        // 3D panels: fixed aspect
        flags |= DVZ_TRANSFORM_FLAGS_FIXED_ASPECT;
        break;

    default:
        break;
    }
    return flags;
}



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

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _scene_fill, canvas->scene);

    // HACK: we use a param of 1 here as a way of putting a lower priority, so that the
    // _scene_frame callback is called *after* the user FRAME callbacks. If the user callbacks call
    // dvz_visual_data(), the _scene_frame() callback will be called directly afterwards, in the
    // same frame.
    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 1, DVZ_EVENT_MODE_SYNC, _scene_frame, canvas->scene);

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

    // HACK: white background if axes controller.
    if (type == DVZ_CONTROLLER_AXES_2D)
        dvz_canvas_clear_color(scene->canvas, 1, 1, 1);

    // Set panel transform flags depending on the contrller type.
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
        _panel_visual_added(panel, visual);
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
    _camera_update_mvp(camera, mvp);
}



void dvz_camera_look(DvzPanel* panel, vec3 center)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller->type == DVZ_CONTROLLER_CAMERA);
    DvzMVP* mvp = _panel_mvp(panel);
    DvzCamera* camera = &panel->controller->interacts[0].u.c;
    glm_vec3_sub(center, camera->eye, camera->forward);
    _camera_update_mvp(camera, mvp);
}



void dvz_arcball_rotate(DvzPanel* panel, float angle, vec3 axis)
{
    ASSERT(panel != NULL);
    ASSERT(panel->controller->type == DVZ_CONTROLLER_ARCBALL);
    DvzMVP* mvp = _panel_mvp(panel);
    DvzArcball* arcball = &panel->controller->interacts[0].u.a;
    glm_quatv(arcball->rotation, angle, axis);
    _arcball_update_mvp(arcball, mvp);
}



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

    dvz_container_destroy(&scene->visuals);
    dvz_obj_destroyed(&scene->obj);
    FREE(scene);
}
