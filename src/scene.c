#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/panel.h"
#include "../include/visky/transforms.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"
#include "axes.h"
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



static inline void _visual_request(VklVisual* visual, VklPanel* panel, VklVisualRequest req)
{
    if (visual != NULL)
    {
        visual->obj.request = req;
    }
    if (panel != NULL)
    {
        // Update the panel request when one of its visual need to be updated. Also mark the scene
        // as needing an update. The scene frame callback checks, at every frame, what needs to be
        // updated. The scene and panel requests lets it avoid doing a full scan at every frame.
        panel->obj.request = req;
        if (panel->scene != NULL)
            panel->scene->obj.request = req;
    }
    if (req == VKL_VISUAL_REQUEST_REFILL)
        vkl_canvas_to_refill(visual->canvas);
}



static inline void _visual_set(VklVisual* visual)
{
    ASSERT(visual != NULL);
    visual->obj.request = VKL_VISUAL_REQUEST_SET;
}



static inline bool _visual_has_request(VklVisual* visual)
{
    ASSERT(visual != NULL);
    return visual->obj.request != VKL_VISUAL_REQUEST_SET &&
           visual->obj.request != VKL_VISUAL_REQUEST_NOT_SET;
}



static inline void _panel_set(VklPanel* panel)
{
    ASSERT(panel != NULL);
    panel->obj.request = VKL_VISUAL_REQUEST_SET;
}



static inline bool _panel_has_request(VklPanel* panel)
{
    ASSERT(panel != NULL);
    return panel->obj.request == VKL_VISUAL_REQUEST_SET ||
           panel->obj.request == VKL_VISUAL_REQUEST_NOT_SET;
}



static inline void _scene_set(VklScene* scene)
{
    ASSERT(scene != NULL);
    scene->obj.request = VKL_VISUAL_REQUEST_SET;
}



static void _visual_detect_item_count_change(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklSource* source = NULL;
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        // Detect a change in vertex_count.
        source = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
        if (source->arr.item_count != visual->prev_vertex_count[pidx])
        {
            log_debug("automatic detection of a change in vertex count, will trigger full refill");
            _visual_request(visual, NULL, VKL_VISUAL_REQUEST_REFILL);
            visual->prev_vertex_count[pidx] = source->arr.item_count;
        }

        // Detect a change in index_count.
        source = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, pidx);
        if (source != NULL && source->arr.item_count != visual->prev_index_count[pidx])
        {
            log_debug("automatic detection of a change in index count, will trigger full refill");
            _visual_request(visual, NULL, VKL_VISUAL_REQUEST_REFILL);
            visual->prev_index_count[pidx] = source->arr.item_count;
        }
    }
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
        ASSERT(arr != NULL);
        if (arr->item_count == 0)
            continue;
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
                // visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
                _visual_request(visual, panel, VKL_VISUAL_REQUEST_UPLOAD);
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

    // NOTE: skip visuals that should not be transformed.
    if ((visual->flags & VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE) != 0)
        return;

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
    {
        ASSERT(panel->visuals[i] != NULL);
        // NOTE: skip visuals that should not be transformed.
        if ((panel->visuals[i]->flags & VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE) == 0)
        {
            boxes[i] = _visual_box(panel->visuals[i]);
        }
    }

    // Merge the visual box with the existing box.
    VklBox box = _box_merge(panel->visual_count, boxes);

    // Make the box square if needed.
    if ((coords->flags & VKL_TRANSFORM_FLAGS_FIXED_ASPECT) != 0)
        box = _box_cube(box);

    // Renormalize all visuals in the panel.
    _panel_renormalize(panel, box);

    // Update the axes.
    if (panel->controller->type == VKL_CONTROLLER_AXES_2D)
    {
        _axes_set(panel->controller, panel->data_coords.box);
    }

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
    // bool to_update = false;
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

        // Process panel and visual requests.

        // NOTE: vkl_visual_data() functions have no notion of panel and cannot update its request.
        // So we are forced to scan through all panels and visuals to find visuals that need an
        // update. That's why the following is commented out.
        // // Skip the panel if there is no request.
        // if (!_panel_has_request(panel))
        //     continue;

        // to_update = panel->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE;
        viewport = panel->viewport;
        for (uint32_t j = 0; j < panel->visual_count; j++)
        {
            visual = panel->visuals[j];

            // First frame:
            if (canvas->frame_idx == 0)
            {
                // Set all visuals to be updated.
                _visual_request(visual, panel, VKL_VISUAL_REQUEST_UPLOAD);

                // Initialize prev_vertex_count and prev_index_count.
                for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
                {
                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, pidx);
                    visual->prev_vertex_count[pidx] = source->arr.item_count;

                    source = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, pidx);
                    if (source != NULL)
                        visual->prev_index_count[pidx] = source->arr.item_count;
                }
            }

            // Skip the visual if there is no request.
            if (!_visual_has_request(visual))
                continue;

            // Process visual upload.
            if (visual->obj.request == VKL_VISUAL_REQUEST_UPLOAD)
            {
                // Update the visual's data.
                vkl_visual_update(visual, viewport, panel->data_coords, NULL);

                // Detect whether the number of vertices/indices has changed, in which case we need
                // a refill in the current frame.
                _visual_detect_item_count_change(visual);

                // The visual no longer needs UPLOAD.
                _visual_set(visual);
            }
        }

        // Mark the panel as no longer needing to be updated.
        _panel_set(panel);

        panel = vkl_container_iter(&grid->panels);
    }

    // Mark the scene as no longer needing to be updated.
    _scene_set(scene);
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



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

// VklTransformOLD vkl_transform_inv(VklTransformOLD tr)
// {
//     ASSERT(tr.scale[0] != 0);
//     ASSERT(tr.scale[1] != 0);

//     VklTransformOLD tri = {0};
//     tri.scale[0] = 1. / tr.scale[0];
//     tri.scale[1] = 1. / tr.scale[1];
//     tri.shift[0] = -tr.scale[0] * tr.shift[0];
//     tri.shift[1] = -tr.scale[1] * tr.shift[1];
//     return tri;
// }



// VklTransformOLD vkl_transform_mul(VklTransformOLD tr0, VklTransformOLD tr1)
// {
//     VklTransformOLD trm = {0};
//     trm.scale[0] = tr0.scale[0] * tr1.scale[0];
//     trm.scale[1] = tr0.scale[1] * tr1.scale[1];
//     trm.shift[0] = tr0.shift[0] + tr1.shift[0] / tr0.scale[0];
//     trm.shift[1] = tr0.shift[1] + tr1.shift[1] / tr0.scale[1];
//     return trm;
// }



// VklTransformOLD vkl_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout)
// {
//     VklTransformOLD tr = {0};
//     tr.scale[0] = tr.scale[1] = 1;
//     if (qin[0] != pin[0])
//         tr.scale[0] = (qout[0] - pout[0]) / (qin[0] - pin[0]);
//     if (qin[1] != pin[1])
//         tr.scale[1] = (qout[1] - pout[1]) / (qin[1] - pin[1]);
//     if (qout[0] != pout[0])
//         tr.shift[0] = (pin[0] * qout[0] - pout[0] * qin[0]) / (qout[0] - pout[0]);
//     if (qout[1] != pout[1])
//         tr.shift[1] = (pin[1] * qout[1] - pout[1] * qin[1]) / (qout[1] - pout[1]);
//     return tr;
// }



// void vkl_transform_apply(VklTransformOLD* tr, dvec2 in, dvec2 out)
// {
//     ASSERT(tr != NULL);
//     if (tr->scale[0] != 0)
//         out[0] = tr->scale[0] * (in[0] - tr->shift[0]);
//     if (tr->scale[1] != 0)
//         out[1] = tr->scale[1] * (in[1] - tr->shift[1]);
// }



// VklTransformOLD vkl_transform_old(VklPanel* panel, VklCDSOld source, VklCDSOld target)
// {
//     ASSERT(panel != NULL);
//     VklTransformOLD tr = {{1, 1}, {0, 0}}; // identity
//     dvec2 NDC0 = {-1, -1};
//     dvec2 NDC1 = {+1, +1};
//     dvec2 ll = {-1, -1};
//     dvec2 ur = {+1, +1};
//     VklPanzoom* panzoom = NULL;
//     VklCanvas* canvas = panel->scene->canvas;

//     if (panel->controller->type == VKL_CONTROLLER_AXES_2D)
//     {
//         // log_error("not implemented yet");
//         // TODO
//         // ll[0] = axes->xscale_orig.vmin;
//         // ll[1] = axes->yscale_orig.vmin;
//         // ur[0] = axes->xscale_orig.vmax;
//         // ur[1] = axes->yscale_orig.vmax;
//         // panzoom = axes->panzoom_inner;
//         panzoom = &panel->controller->interacts[0].u.p;
//     }
//     else if (panel->controller->type == VKL_CONTROLLER_PANZOOM)
//     {
//         panzoom = &panel->controller->interacts[0].u.p;
//     }
//     else
//     {
//         log_error("controller other than axes 2D and panzoom not yet supported");
//         return tr;
//     }

//     VklViewport viewport = panel->viewport;

//     if (source == target)
//     {
//         return tr;
//     }
//     else if (source > target)
//     {
//         return vkl_transform_inv(vkl_transform_old(panel, target, source));
//     }
//     else if (target - source >= 2)
//     {
//         for (uint32_t k = source; k <= target - 1; k++)
//         {
//             tr = vkl_transform_mul(tr, vkl_transform_old(panel, (VklCDSOld)k, (VklCDSOld)(k +
//             1)));
//         }
//     }
//     else if (target - source == 1)
//     {
//         switch (source)
//         {

//         case VKL_CDS_DATA:
//             // linear normalization based on axes range
//             ASSERT(target == VKL_CDS_GPU);
//             {
//                 tr = vkl_transform_interp(ll, NDC0, ur, NDC1);
//             }
//             break;

//         case VKL_CDS_GPU:
//             // apply panzoom
//             ASSERT(target == VKL_CDS_PANZOOM);
//             {
//                 ASSERT(panzoom != NULL);
//                 ASSERT(panzoom->zoom[0] != 0);
//                 ASSERT(panzoom->zoom[1] != 0);
//                 dvec2 p = {panzoom->camera_pos[0], panzoom->camera_pos[1]};
//                 dvec2 s = {panzoom->zoom[0], panzoom->zoom[1]};
//                 tr.scale[0] = s[0];
//                 tr.scale[1] = s[1];
//                 tr.shift[0] = p[0]; // / s[0];
//                 tr.shift[1] = p[1]; // / s[1];
//             }
//             break;

//         case VKL_CDS_PANZOOM:
//             // using inner viewport
//             ASSERT(target == VKL_CDS_PANEL);
//             {
//                 // Margins.
//                 // double cw = panel->scene->canvas->size.framebuffer_width;
//                 // double ch = panel->scene->canvas->size.framebuffer_height;
//                 // uvec2 size = {0};
//                 // vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
//                 double cw = viewport.viewport.width;
//                 double ch = viewport.viewport.height;
//                 double mt = 2 * viewport.margins[0] / ch;
//                 double mr = 2 * viewport.margins[1] / cw;
//                 double mb = 2 * viewport.margins[2] / ch;
//                 double ml = 2 * viewport.margins[3] / cw;

//                 tr = vkl_transform_interp(
//                     NDC0, (dvec2){-1 + ml, -1 + mb}, NDC1, (dvec2){+1 - mr, +1 - mt});
//             }
//             break;

//         case VKL_CDS_PANEL:
//             // multiply by canvas size
//             ASSERT(target == VKL_CDS_CANVAS_NDC);
//             {
//                 // From outer to inner viewport.
//                 ll[0] = -1 + 2 * viewport.viewport.x;
//                 ll[1] = +1 - 2 * (viewport.viewport.y + viewport.viewport.height);
//                 ur[0] = -1 + 2 * (viewport.viewport.x + viewport.viewport.width);
//                 ur[1] = +1 - 2 * viewport.viewport.y;

//                 tr = vkl_transform_interp(NDC0, ll, NDC1, ur);
//             }
//             break;

//         case VKL_CDS_CANVAS_NDC:
//             // multiply by canvas size
//             ASSERT(target == VKL_CDS_CANVAS_PX);
//             {
//                 uvec2 size = {0};
//                 vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
//                 tr = vkl_transform_interp(NDC0, (dvec2){0, size[1]}, NDC1, (dvec2){size[0], 0});
//             }
//             break;

//         default:
//             log_error("unknown coordinate systems");
//             break;
//         }
//     }
//     ASSERT(tr.scale[0] != 0);
//     ASSERT(tr.scale[1] != 0);
//     return tr;
// }



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
