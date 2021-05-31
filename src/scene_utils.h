#ifndef DVZ_SCENE_UTILS_HEADER
#define DVZ_SCENE_UTILS_HEADER

#include "../include/datoviz/scene.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Test                                                                                         */
/*************************************************************************************************/

static inline bool _is_visual_to_transform(DvzVisual* visual)
{
    return (visual->flags & DVZ_VISUAL_FLAGS_TRANSFORM_NONE) == 0;
}



static inline bool _is_aspect_fixed(DvzDataCoords* coords)
{
    return (coords->flags & DVZ_TRANSFORM_FLAGS_FIXED_ASPECT) != 0;
}



static bool _has_item_count_changed(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    bool has_changed = false;
    DvzSource* source = NULL;
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        // Detect a change in vertex_count.
        source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, pidx);
        if (source->arr.item_count != visual->prev_vertex_count[pidx])
        {
            // log_debug("automatic detection of a change in vertex count, will trigger full
            // refill");
            has_changed = true;
            visual->prev_vertex_count[pidx] = source->arr.item_count;
        }

        // Detect a change in index_count.
        source = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, pidx);
        if (source != NULL && source->arr.item_count != visual->prev_index_count[pidx])
        {
            // log_debug("automatic detection of a change in index count, will trigger full
            // refill");
            has_changed = true;
            visual->prev_index_count[pidx] = source->arr.item_count;
        }
    }
    return has_changed;
}



static inline bool _has_obj_changed(DvzObject* obj)
{
    ASSERT(obj != NULL);
    return obj->request > DVZ_VISUAL_REQUEST_SET;
}



static inline bool _has_coords_changed(DvzDataCoords* coords, DvzBox* box)
{
    return memcmp(box, &coords->box, sizeof(DvzBox)) != 0;
}



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
    ASSERT(prop != NULL);
    ASSERT(prop->prop_type == DVZ_PROP_POS);

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
    log_trace("normalizing POS prop, %d items", arr->item_count);
    // _box_print(coords.box);
    *arr_tr = dvz_array(arr->item_count, arr->dtype);
    dvz_transform_pos(coords, arr, arr_tr, false);
}



static DvzBox _compute_panel_box(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    DvzDataCoords* coords = &panel->data_coords;
    ASSERT(coords != NULL);

    // We'll compute the box surrounding each visual, and we'll merge them.
    DvzBox* boxes = calloc(panel->visual_count, sizeof(DvzBox));
    // number of boxes to compute, depends on the number of visuals to be transformed
    uint32_t count = 0;

    // Get the bounding box of each visual.
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        ASSERT(panel->visuals[i] != NULL);

        // NOTE: skip visuals that should not be transformed.
        if (_is_visual_to_transform(panel->visuals[i]))
        {
            boxes[count++] = _visual_box(panel->visuals[i]);
        }
    }

    // Merge the visual box with the existing box.
    DvzBox box = _box_merge(count, boxes);
    // _box_print(box);
    FREE(boxes);

    // Make the box square if needed.
    if (_is_aspect_fixed(coords))
        box = _box_cube(box);

    _check_box(box);
    return box;
}



static void _init_item_count_change_detection(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    DvzSource* source = NULL;
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



// Update the GPU viewport struct of a visual.
static void _update_visual_viewport(DvzPanel* panel, DvzVisual* visual)
{
    visual->viewport = panel->viewport;
    log_trace("update visual viewport");
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
/*  Scene update enqueueing                                                                      */
/*************************************************************************************************/

// Enqueue a scene update.
static void _scene_update_enqueue(DvzScene* scene, DvzSceneUpdate update)
{
    // log_trace("enqueue scene update of type %d", update.type);
    ASSERT(scene != NULL);
    DvzFifo* fifo = &scene->update_fifo;
    ASSERT(fifo != NULL);
    DvzSceneUpdate* up = (DvzSceneUpdate*)calloc(1, sizeof(DvzSceneUpdate));
    *up = update;
    dvz_fifo_enqueue(fifo, up);
}



static void _enqueue_visual_changed(DvzPanel* panel, DvzVisual* visual)
{
    log_trace("enqueue visual changed");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);
    ASSERT(visual != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_VISUAL_CHANGED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    up.visual = visual;
    _scene_update_enqueue(scene, up);
}



static void _enqueue_visual_added(DvzPanel* panel, DvzVisual* visual)
{
    log_trace("enqueue visual added");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);
    ASSERT(visual != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_VISUAL_ADDED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    up.visual = visual;
    _scene_update_enqueue(scene, up);
}



static void _enqueue_prop_changed(DvzPanel* panel, DvzVisual* visual, DvzProp* prop)
{
    log_trace("enqueue prop changed");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);
    ASSERT(visual != NULL);
    ASSERT(prop != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_PROP_CHANGED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    up.visual = visual;
    up.prop = prop;
    up.source = prop->source;
    _scene_update_enqueue(scene, up);
}



static void _enqueue_item_count_changed(DvzPanel* panel, DvzVisual* visual)
{
    log_trace("enqueue item count changed");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);
    ASSERT(visual != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_ITEM_COUNT_CHANGED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    up.visual = visual;
    _scene_update_enqueue(scene, up);
}



static void _enqueue_panel_changed(DvzPanel* panel)
{
    log_trace("enqueue panel changed");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_PANEL_CHANGED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    _scene_update_enqueue(scene, up);
}



static void _enqueue_coords_changed(DvzPanel* panel)
{
    log_trace("enqueue coords changed");
    ASSERT(panel != NULL);
    DvzScene* scene = panel->scene;
    ASSERT(scene != NULL);

    DvzSceneUpdate up = {0};
    up.type = DVZ_SCENE_UPDATE_COORDS_CHANGED;
    up.scene = scene;
    up.canvas = scene->canvas;
    up.panel = panel;
    _scene_update_enqueue(scene, up);
}



/*************************************************************************************************/
/*  Processing scene updates                                                                     */
/*************************************************************************************************/

// Called when a prop's data has changed.
// Change the visual and source request, to be picked up by dvz_visual_data() later.
static void _process_prop_changed(DvzSceneUpdate up)
{
    ASSERT(up.panel != NULL);
    DvzDataCoords coords = up.panel->data_coords;

    // if POS prop, we do data normalization
    ASSERT(up.prop != NULL);
    ASSERT(up.visual != NULL);
    if (up.prop->prop_type == DVZ_PROP_POS && _is_visual_to_transform(up.visual))
    {
        _transform_pos_prop(coords, up.prop);

        if ((up.visual->flags & DVZ_VISUAL_FLAGS_TRANSFORM_BOX_INIT) == 0)
        {
            // Recompute the visual box.
            DvzBox box = _visual_box(up.visual);

            // Make the box square if needed.
            if (_is_aspect_fixed(&coords))
                box = _box_cube(box);

            // If the new box has changed, renormalize all visuals.
            if (_has_coords_changed(&coords, &box))
            {
                // Update the data coords.
                up.panel->data_coords.box = box;
                _enqueue_coords_changed(up.panel);
            }
        }
    }

    // Mark the visual and source has needing update, for dvz_visual_update()
    ASSERT(up.source != NULL);
    _source_set_changed(up.source, true);
}



// Called when the box coords has changed and ALL visuals in a panel must be renormalized.
static void _process_coords_changed(DvzSceneUpdate up)
{
    log_trace("process coords changed");

    DvzPanel* panel = up.panel;
    ASSERT(panel != NULL);

    // We'll iterate through all visuals.
    DvzVisual* visual = NULL;

    // We'll iterate through all props of each visual.
    DvzProp* prop = NULL;
    DvzContainerIterator iter;

    // Go through all visuals in the panel.
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        visual = panel->visuals[i];
        ASSERT(visual != NULL);

        // NOTE: skip visuals that should not be transformed.
        if (!_is_visual_to_transform(visual))
        {
            log_trace("skip visual transform when processing coords changed");
            continue;
        }

        // Go through all visual props.
        iter = dvz_container_iterator(&visual->props);
        while (iter.item != NULL)
        {
            prop = iter.item;
            ASSERT(prop != NULL);

            // Transform all POS props with the panel data coordinates.
            if (prop->prop_type == DVZ_PROP_POS)
            {
                _enqueue_prop_changed(panel, visual, prop);
            }

            dvz_container_iter(&iter);
        }
    }

    // Update the axes.
    if (panel->controller->type == DVZ_CONTROLLER_AXES_2D)
    {
        ASSERT(panel->controller != NULL);
        _axes_set(panel->controller, panel->data_coords.box);
        _axes_refresh(panel->controller, true);
    }
}



// Called when a new visual is added.
static void _process_visual_added(DvzSceneUpdate up)
{
    log_trace("process visual added");

    DvzVisual* visual = up.visual;
    ASSERT(visual != NULL);

    DvzPanel* panel = up.panel;
    ASSERT(panel != NULL);

    // Update the visual viewport.
    _update_visual_viewport(panel, visual);

    // Compute box of the new visual, taking visual transform flags into account
    if (!_is_visual_to_transform(visual))
        return;

    // Get the visual box.
    DvzDataCoords coords = panel->data_coords;
    DvzBox box = _visual_box(visual);

    // Take existing box of the panel and merge it with new box.
    box = _box_merge(2, (DvzBox[]){coords.box, box});

    // Make the box square if needed.
    if (_is_aspect_fixed(&coords))
        box = _box_cube(box);

    // If the panel box has changed, renormalize all visuals.
    if (_has_coords_changed(&coords, &box))
    {
        // This function renormalizes all visuals.
        panel->data_coords.box = box;
        _enqueue_coords_changed(panel);
    }

    // Once added, we need to trigger the visual data upload.
    _enqueue_visual_changed(panel, visual);
}



// Called when visual data has changed.
static void _process_visual_changed(DvzSceneUpdate up)
{
    log_trace("process visual changed");

    DvzVisual* visual = up.visual;
    ASSERT(visual != NULL);
    DvzPanel* panel = up.panel;
    ASSERT(panel != NULL);

    // Visual data GPU upload.
    dvz_visual_update(visual, panel->viewport, panel->data_coords, NULL);

    // Detect whether the number of vertices/indices has changed, in which case a command buffer
    // refill will be needed.
    if (_has_item_count_changed(visual))
    {
        _enqueue_item_count_changed(panel, visual);
    }

    // TODO: recompute the bounding box when changing the data?
    // // If the panel box has changed, renormalize all visuals.
    // if (_has_coords_changed(&coords, &box))
    // {
    //     // This function renormalizes all visuals.
    //     _enqueue_coords_changed(panel);
    // }
}



// Called when the visibility of a visual has changed.
static void _process_visibility_changed(DvzSceneUpdate up)
{
    ASSERT(up.canvas != NULL);
    // Refill command buffer.
    dvz_canvas_to_refill(up.canvas);
}



// Called when the number of vertices/indices has changed.
static void _process_item_count_changed(DvzSceneUpdate up)
{
    ASSERT(up.canvas != NULL);
    // Refill command buffer.
    dvz_canvas_to_refill(up.canvas);
}



// Called when a panel has changed.
static void _process_panel_changed(DvzSceneUpdate up)
{
    log_trace("process panel changed");
    DvzPanel* panel = up.panel;
    ASSERT(panel != NULL);

    // Recompute the panel viewport.
    dvz_panel_update(panel);
    ASSERT(dvz_obj_is_created(&panel->obj));

    // Updat the GPU panel struct for all visuals in the panel.
    for (uint32_t k = 0; k < panel->visual_count; k++)
        _update_visual_viewport(panel, panel->visuals[k]);

    // // The panel no longer needs to be updated.
    // panel->obj.request = 0;

    // Refill command buffer.
    ASSERT(up.canvas != NULL);
    dvz_canvas_to_refill(up.canvas);
}



// Called when an interact has changed.
static void _process_interact_changed(DvzSceneUpdate up)
{
    // TODO
}



// // Called when a canvas has been resized.
// static void _process_canvas_resized(DvzSceneUpdate up)
// {
//     DvzScene* scene = up.scene;
//     ASSERT(scene != NULL);

//     DvzGrid* grid = &scene->grid;
//     ASSERT(grid != NULL);

//     // Go through all panels and recompute their viewport.
//     DvzPanel* panel = NULL;
//     DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
//     while (iter.item != NULL)
//     {
//         panel = iter.item;
//         ASSERT(panel != NULL);
//         up.panel = panel;
//         _process_panel_changed(up);
//         dvz_container_iter(&iter);
//     }
// }



// Called when anything in the scene has changed.
static void _process_scene_update(DvzSceneUpdate up)
{
    switch (up.type)
    {

    case DVZ_SCENE_UPDATE_VISUAL_ADDED:
        _process_visual_added(up);
        break;

    case DVZ_SCENE_UPDATE_VISUAL_CHANGED:
        _process_visual_changed(up);
        break;

    case DVZ_SCENE_UPDATE_PROP_CHANGED:
        _process_prop_changed(up);
        break;

    case DVZ_SCENE_UPDATE_VISIBILITY_CHANGED:
        _process_visibility_changed(up);
        break;

    case DVZ_SCENE_UPDATE_ITEM_COUNT_CHANGED:
        _process_item_count_changed(up);
        break;

    case DVZ_SCENE_UPDATE_PANEL_CHANGED:
        _process_panel_changed(up);
        break;

    case DVZ_SCENE_UPDATE_INTERACT_CHANGED:
        _process_interact_changed(up);
        break;

    case DVZ_SCENE_UPDATE_COORDS_CHANGED:
        _process_coords_changed(up);
        break;

        // case DVZ_SCENE_UPDATE_CANVAS_RESIZED:
        //     _process_canvas_resized(up);
        //     break;

    default:
        break;
    }
}



/*************************************************************************************************/
/*  Scene updates                                                                                */
/*************************************************************************************************/

static void _callback_controllers(DvzScene* scene)
{
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;

    // Go through all panels that need to be updated.
    DvzPanel* panel = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);

    // Go through all panels in the scene to detect the scene updates.
    while (iter.item != NULL)
    {
        panel = iter.item;

        // Interactivity.
        if (panel->controller != NULL && panel->controller->callback != NULL)
        {
            // TODO: event struct
            panel->controller->callback(panel->controller, (DvzEvent){0});
        }
        dvz_container_iter(&iter);
    }
}



static void _enqueue_all_visuals_changed(DvzScene* scene)
{
    log_trace("enqueue all visuals changed");

    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;

    // Go through all panels that need to be updated.
    DvzPanel* panel = NULL;
    DvzContainerIterator iter_panel = dvz_container_iterator(&grid->panels);
    DvzVisual* visual = NULL;
    DvzContainerIterator iter_prop;
    DvzProp* prop = NULL;

    // Go through all panels in the scene to detect the scene updates.
    while (iter_panel.item != NULL)
    {
        panel = iter_panel.item;

        // Determine what has changed in the scene since last frame:

        // Process panel and visual requests.
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            visual = panel->visuals[j];

            // // Require panel update?
            // if (panel->obj.request == 1)
            //     _enqueue_panel_changed(panel);

            // Process visual upload.
            if (visual->obj.request == DVZ_VISUAL_REQUEST_UPLOAD)
            {
                // Check if the POS props have been changed.
                iter_prop = dvz_container_iterator(&visual->props);
                while (iter_prop.item != NULL)
                {
                    prop = iter_prop.item;
                    if (prop->prop_type == DVZ_PROP_POS &&
                        prop->obj.request == DVZ_VISUAL_REQUEST_UPLOAD)
                    {
                        _enqueue_prop_changed(panel, visual, prop);
                        prop->obj.request = DVZ_VISUAL_REQUEST_SET;
                    }
                    dvz_container_iter(&iter_prop);
                }

                _enqueue_visual_changed(panel, visual);
            }
        }
        dvz_container_iter(&iter_panel);
    }
}



// Dequeue a scene update.
static DvzSceneUpdate _scene_update_dequeue(DvzScene* scene)
{
    log_trace("dequeue scene update");

    ASSERT(scene != NULL);
    DvzFifo* fifo = &scene->update_fifo;
    ASSERT(fifo != NULL);
    DvzSceneUpdate* item = (DvzSceneUpdate*)dvz_fifo_dequeue(fifo, false);
    DvzSceneUpdate out;
    out.type = DVZ_SCENE_UPDATE_NONE;
    if (item == NULL)
        return out;
    ASSERT(item != NULL);
    out = *item;
    FREE(item);
    return out;
}



// Process all pending scene updates.
static void _process_scene_updates(DvzScene* scene)
{
    ASSERT(scene != NULL);
    DvzFifo* fifo = &scene->update_fifo;

    // Find all visuals that need update, and enqueue them.
    _enqueue_all_visuals_changed(scene);

    // Iteratively process the scene updates, which can trigger more visuals changes.
    DvzSceneUpdate up = {0};
    uint32_t i = 0;
    while (dvz_fifo_size(fifo) > 0 && i <= 1000) // HACK: avoid infinite loop
    {
        log_trace("scene update pass #%d", i);

        // Process all pending updates.
        up = _scene_update_dequeue(scene);
        while (up.type != DVZ_SCENE_UPDATE_NONE)
        {
            _process_scene_update(up);
            up = _scene_update_dequeue(scene);
        }

        // Find all visuals that need update, and enqueue them.
        _enqueue_all_visuals_changed(scene);

        i++;
    }
}



/*************************************************************************************************/
/*  Scene callbacks                                                                              */
/*************************************************************************************************/

// At initialization, we must initialize the item count change detector.
static void _scene_init(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);

    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);

    DvzGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    // Go through all panels in the scene.
    DvzPanel* panel = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    while (iter.item != NULL)
    {
        panel = iter.item;

        // Trigger normalization of all visuals initially in the panel.
        panel->data_coords.box = _compute_panel_box(panel);
        _enqueue_coords_changed(panel);

        // Go through all visuals.
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            // Init the item change detection.
            _init_item_count_change_detection(panel->visuals[j]);
        }
        dvz_container_iter(&iter);
    }
}



// Refill the command buffer with all panels and visuals.
// NOTE: the panel viewports must have been updated first.
static void _scene_fill(DvzCanvas* canvas, DvzEvent ev)
{
    log_trace("scene fill");
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

            // Find the panel viewport.
            viewport = dvz_panel_viewport(panel);
            dvz_cmd_viewport(cmds, img_idx, viewport.viewport);

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



static void _scene_resize(DvzCanvas* canvas, DvzEvent ev)
{
    log_trace("scene resize");

    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;

    // Go through all panels and recompute their viewport.
    DvzPanel* panel = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzSceneUpdate up = {0};
    up.canvas = canvas;
    while (iter.item != NULL)
    {
        panel = iter.item;
        ASSERT(panel != NULL);
        up.panel = panel;
        _process_panel_changed(up);
        dvz_container_iter(&iter);
    }
}



// Called at every frame, this important function checks if there are any scene updates, and
// processes them if so. It also calls the controller callbacks for every panel.
static void _scene_frame(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);

    DvzScene* scene = (DvzScene*)ev.user_data;
    ASSERT(scene != NULL);

    // Call the controller callbacks of all panels.
    _callback_controllers(scene);

    // Process the scene updates.
    _process_scene_updates(scene);
}



// Upload the MVP struct to the panels.
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

            // IMPORTANT: we **must** update the uniform buffer at every frame.
            dvz_canvas_buffers(canvas, panel->br_mvp, 0, panel->br_mvp.size, &interact->mvp);
        }
        dvz_container_iter(&iter);
    }
}



#ifdef __cplusplus
}
#endif

#endif
