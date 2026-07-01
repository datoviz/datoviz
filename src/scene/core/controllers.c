/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene controllers and panel input                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controllers.h"
#include "datoviz/math/_cglm.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "interaction/internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_FLY_MAX_DT 0.1



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static void _scene_controller_links_destroy_for_controller(DvzController* controller);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure one panel has an owned camera.
 *
 * @param panel the panel
 * @return the panel camera, or NULL on allocation failure
 */
static DvzCamera* _scene_panel_ensure_camera(DvzPanel* panel)
{
    ANN(panel);
    if (panel->camera == NULL)
    {
        DvzCameraDesc camera_desc = dvz_camera_desc();
        panel->camera = _dvz_camera(&camera_desc);
        if (panel->camera != NULL)
        {
            panel->active_view_kind = DVZ_PANEL_VIEW_KIND_3D;
            _scene_panel_view3d_dirty(panel);
        }
    }
    return panel->camera;
}



/**
 * Destroy one scene-owned controller payload.
 *
 * @param controller the controller
 */
void _scene_controller_destroy(DvzController* controller)
{
    if (controller == NULL || !controller->active)
        return;
    _scene_controller_links_destroy_for_controller(controller);
    if (controller->panzoom != NULL)
    {
        dvz_panzoom_destroy(controller->panzoom);
        controller->panzoom = NULL;
    }
    if (controller->arcball != NULL)
    {
        dvz_arcball_destroy(controller->arcball);
        controller->arcball = NULL;
    }
    if (controller->fly != NULL)
    {
        dvz_fly_destroy(controller->fly);
        controller->fly = NULL;
    }
    if (controller->turntable != NULL)
    {
        dvz_turntable_destroy(controller->turntable);
        controller->turntable = NULL;
    }
    controller->scene = NULL;
    controller->type = DVZ_CONTROLLER_TYPE_NONE;
    controller->active = false;
}



/**
 * Return whether one fly controller still needs frame updates.
 *
 * @param fly the fly payload
 * @return whether the fly payload is active
 */
static bool _scene_fly_active(const DvzFly* fly)
{
    if (fly == NULL)
        return false;
    return fly->key_forward || fly->key_backward || fly->key_left || fly->key_right ||
           fly->key_up || fly->key_down || fly->interacting || fly->pivot_marker_time_left > 0.0;
}



/**
 * Return whether one controller is already present in a small pointer set.
 *
 * @param controllers controller pointer set
 * @param count number of valid entries
 * @param controller controller to search
 * @return whether the controller was found
 */
static bool _scene_controller_seen(
    DvzController* const* controllers, uint32_t count, const DvzController* controller)
{
    ANN(controllers);
    if (controller == NULL)
        return true;
    for (uint32_t i = 0; i < count; i++)
    {
        if (controllers[i] == controller)
            return true;
    }
    return false;
}



/**
 * Synchronize the transient fly pivot marker visual for one panel.
 *
 * @param panel the panel
 * @return whether synchronization succeeded
 */
bool _scene_panel_sync_fly_pivot_marker(DvzPanel* panel)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL ||
        panel->fly == NULL)
    {
        return true;
    }

    DvzFly* fly = panel->fly;
    bool visible = fly->has_pivot && fly->pivot_marker_visible;
    if (panel->fly_pivot_marker_visual == NULL && !visible)
        return true;

    if (panel->fly_pivot_marker_visual == NULL)
    {
        DvzVisual* marker = dvz_point(panel->figure->scene, 0);
        if (marker == NULL)
            return false;
        DvzPointStyleDesc style = dvz_point_style_desc();
        style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
        style.stroke_width_px = 2.0f;
        style.edge_color = dvz_color_rgba(0, 0, 0, 220);
        if (dvz_point_set_style(marker, &style) != 0)
            return false;
        if (dvz_visual_set_depth_test(marker, false) != 0)
            return false;
        DvzVisualAttachDesc attach = dvz_visual_attach_desc();
        attach.z_layer = 10000;
        attach.controller_mode = DVZ_CONTROLLER_APPLY;
        attach.coord_space = DVZ_COORD_VIEW;
        if (dvz_panel_add_visual(panel, marker, &attach) != 0)
        {
            return false;
        }
        panel->fly_pivot_marker_visual = marker;
    }

    DvzVisual* marker = panel->fly_pivot_marker_visual;
    dvz_visual_set_visible(marker, visible);
    if (!visible)
        return true;

    float position[1][3] = {{
        fly->pivot[0],
        fly->pivot[1],
        fly->pivot[2],
    }};
    DvzColor color[1] = {{255, 220, 32, 235}};
    float size[1] = {14.0f};
    return dvz_visual_set_data(marker, "position", position, 1) == 0 &&
           dvz_visual_set_data(marker, "color", color, 1) == 0 &&
           dvz_visual_set_data(marker, "size", size, 1) == 0;
}



/**
 * Return the dimension mask used by one panel for a controller.
 *
 * @param panel the panel
 * @param controller the controller
 * @return dimension mask, or DVZ_DIM_MASK_NONE
 */
static DvzDimMask _scene_panel_controller_dims(
    const DvzPanel* panel, const DvzController* controller)
{
    ANN(panel);
    if (controller == NULL)
        return DVZ_DIM_MASK_NONE;
    DvzDimMask dims = DVZ_DIM_MASK_NONE;
    if (panel->controllers[DVZ_DIM_X] == controller)
        dims |= DVZ_DIM_MASK_X;
    if (panel->controllers[DVZ_DIM_Y] == controller)
        dims |= DVZ_DIM_MASK_Y;
    if (panel->controllers[DVZ_DIM_Z] == controller)
        dims |= DVZ_DIM_MASK_Z;
    return dims;
}



/**
 * Return whether a panel currently borrows one controller.
 *
 * @param panel the panel
 * @param controller the controller
 * @return whether the panel has at least one binding to the controller
 */
static bool _scene_panel_has_controller(const DvzPanel* panel, const DvzController* controller)
{
    return _scene_panel_controller_dims(panel, controller) != DVZ_DIM_MASK_NONE;
}



/**
 * Notify every figure with at least one panel bound to a controller.
 *
 * @param controller the controller
 */
static void _scene_notify_controller_figures(const DvzController* controller)
{
    if (controller == NULL || controller->scene == NULL)
        return;
    DvzScene* scene = controller->scene;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        bool bound = false;
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
            bound = bound || _scene_panel_has_controller(&figure->panels[pi], controller);
        if (bound)
            _scene_notify_request_frame(figure);
    }
}


/**
 * Detach one controller from all panel bindings and notify affected figures.
 *
 * @param controller the controller
 */
static void _scene_controller_detach_from_panels(DvzController* controller)
{
    if (controller == NULL || controller->scene == NULL)
        return;

    DvzScene* scene = controller->scene;
    DvzFigure* affected[DVZ_SCENE_MAX_FIGURES] = {0};
    uint32_t affected_count = 0;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        if (figure->scene != scene)
            continue;

        bool figure_affected = false;
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            if (panel->figure != figure)
                continue;

            bool panel_affected = false;
            for (uint32_t dim = 0; dim < 3; dim++)
            {
                if (panel->controllers[dim] == controller)
                {
                    panel->controllers[dim] = NULL;
                    panel_affected = true;
                }
            }
            if (!panel_affected)
                continue;

            if (panel->panzoom == controller->panzoom)
                panel->panzoom = NULL;
            if (panel->arcball == controller->arcball)
                panel->arcball = NULL;
            if (panel->fly == controller->fly)
                panel->fly = NULL;
            if (panel->turntable == controller->turntable)
                panel->turntable = NULL;
            figure_affected = true;
        }
        if (figure_affected && affected_count < DVZ_SCENE_MAX_FIGURES)
            affected[affected_count++] = figure;
    }

    for (uint32_t i = 0; i < affected_count; i++)
        _scene_notify_request_frame(affected[i]);
}



/**
 * Return whether one controller link endpoint is usable.
 *
 * @param controller the controller
 * @return whether the endpoint can participate in propagation
 */
static bool _scene_controller_link_endpoint_valid(const DvzController* controller)
{
    return controller != NULL && controller->active && controller->scene != NULL;
}



/**
 * Return whether the requested link components are supported by two controllers.
 *
 * @param source the source controller
 * @param target the target controller
 * @param components component bitmask
 * @return whether the link components are compatible
 */
static bool _scene_controller_link_components_valid(
    const DvzController* source, const DvzController* target, uint32_t components)
{
    if (!_scene_controller_link_endpoint_valid(source) ||
        !_scene_controller_link_endpoint_valid(target) || source->type != target->type ||
        components == DVZ_CONTROLLER_LINK_NONE)
    {
        return false;
    }

    uint32_t supported = 0;
    switch (source->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        supported =
            DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM |
            DVZ_CONTROLLER_LINK_EXTENT_X | DVZ_CONTROLLER_LINK_EXTENT_Y;
        break;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        supported =
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN |
            DVZ_CONTROLLER_LINK_ZOOM;
        break;
    default:
        return false;
    }
    return (components & ~supported) == 0;
}



/**
 * Copy selected panzoom state from one controller payload to another.
 *
 * @param source source panzoom payload
 * @param target target panzoom payload
 * @param components component bitmask
 */
static DvzPanel* _scene_controller_first_panel(DvzScene* scene, const DvzController* controller)
{
    if (scene == NULL || controller == NULL)
        return NULL;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            if (panel->panzoom == controller->panzoom)
                return panel;
            for (uint32_t dim = 0; dim < 2; dim++)
            {
                if (panel->controllers[dim] == controller)
                    return panel;
            }
        }
    }
    return NULL;
}


/**
 * Copy one resolved extent dimension into a target panzoom payload.
 *
 * @param extent source visible extent
 * @param target_panel target panel
 * @param target target panzoom payload
 * @param dim target dimension
 * @return whether the resolved extent was copied
 */
static bool _scene_controller_link_copy_panzoom_extent_dim(
    const float extent[4], const DvzPanel* target_panel, DvzPanzoom* target, uint32_t dim)
{
    ANN(extent);
    ANN(target_panel);
    ANN(target);
    DvzPanelView2DResolved target_view = {0};
    if (dim > 1 || !_scene_panel_view2d_resolve(target_panel, &target_view))
        return false;

    const uint32_t lo = dim == 0 ? 0 : 2;
    const uint32_t hi = dim == 0 ? 1 : 3;
    const float base_min = target_view.view_extent[lo];
    const float base_max = target_view.view_extent[hi];
    const float src_min = extent[lo];
    const float src_max = extent[hi];
    if (!(base_max > base_min) || !(src_max > src_min))
        return false;

    const float base_center = 0.5f * (base_min + base_max);
    const float base_half = 0.5f * (base_max - base_min);
    const float src_center = 0.5f * (src_min + src_max);
    const float src_half = 0.5f * (src_max - src_min);
    if (!(base_half > 0.0f) || !(src_half > 0.0f))
        return false;

    target->pan[dim] = (base_center - src_center) / base_half;
    target->pan_center[dim] = target->pan[dim];
    target->zoom[dim] = base_half / src_half;
    target->zoom_center[dim] = target->zoom[dim];
    return true;
}


/**
 * Copy selected panzoom state from one controller payload to another.
 *
 * @param source_controller source controller
 * @param target_controller target controller
 * @param components component bitmask
 */
static void _scene_controller_link_copy_panzoom(
    DvzController* source_controller, DvzController* target_controller, uint32_t components)
{
    ANN(source_controller);
    ANN(target_controller);
    DvzPanzoom* source = source_controller->panzoom;
    DvzPanzoom* target = target_controller->panzoom;
    ANN(source);
    ANN(target);
    if ((components & DVZ_CONTROLLER_LINK_PAN) != 0)
    {
        glm_vec2_copy(source->pan, target->pan);
        glm_vec2_copy(source->pan_center, target->pan_center);
    }
    if ((components & DVZ_CONTROLLER_LINK_ZOOM) != 0)
    {
        glm_vec2_copy(source->zoom, target->zoom);
        glm_vec2_copy(source->zoom_center, target->zoom_center);
    }
    DvzPanel* source_panel = _scene_controller_first_panel(source_controller->scene, source_controller);
    DvzPanel* target_panel = _scene_controller_first_panel(target_controller->scene, target_controller);
    float source_extent[4] = {0};
    bool has_resolved_extent =
        source_panel != NULL && target_panel != NULL &&
        _scene_panel_panzoom_extent(source_panel, source_extent);
    if ((components & DVZ_CONTROLLER_LINK_EXTENT_X) != 0)
    {
        if (
            !has_resolved_extent ||
            !_scene_controller_link_copy_panzoom_extent_dim(source_extent, target_panel, target, 0))
        {
            target->pan[0] = source->pan[0];
            target->pan_center[0] = source->pan_center[0];
            target->zoom[0] = source->zoom[0];
            target->zoom_center[0] = source->zoom_center[0];
        }
    }
    if ((components & DVZ_CONTROLLER_LINK_EXTENT_Y) != 0)
    {
        if (
            !has_resolved_extent ||
            !_scene_controller_link_copy_panzoom_extent_dim(source_extent, target_panel, target, 1))
        {
            target->pan[1] = source->pan[1];
            target->pan_center[1] = source->pan_center[1];
            target->zoom[1] = source->zoom[1];
            target->zoom_center[1] = source->zoom_center[1];
        }
    }
}



/**
 * Copy selected arcball state from one controller payload to another.
 *
 * @param source source arcball payload
 * @param target target arcball payload
 * @param components component bitmask
 */
static void _scene_controller_link_copy_arcball(
    DvzArcball* source, DvzArcball* target, uint32_t components)
{
    ANN(source);
    ANN(target);
    if ((components & DVZ_CONTROLLER_LINK_ROTATION) != 0)
    {
        mat4 model = GLM_MAT4_IDENTITY_INIT;
        dvz_arcball_model(source, model);
        glm_mat4_copy(model, target->mat);
        glm_quat_identity(target->rotation);
        target->interacting = false;
    }
    if ((components & DVZ_CONTROLLER_LINK_PAN) != 0)
    {
        glm_vec2_copy(source->pan, target->pan);
        glm_vec2_copy(source->pan_center, target->pan_center);
    }
    if ((components & DVZ_CONTROLLER_LINK_ZOOM) != 0)
        target->zoom = source->zoom;
}



/**
 * Return whether one controller endpoint is actively interacting.
 *
 * @param controller the controller
 * @return whether the controller is active for two-way link direction
 */
static bool _scene_controller_interacting(const DvzController* controller)
{
    if (!_scene_controller_link_endpoint_valid(controller))
        return false;
    switch (controller->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        return controller->panzoom != NULL && controller->panzoom->interacting;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        return controller->arcball != NULL && controller->arcball->interacting;
    default:
        return false;
    }
}


/**
 * Copy selected controller state from one endpoint to another.
 *
 * @param source source controller
 * @param target target controller
 * @param components component bitmask
 * @return whether propagation was applied
 */
static bool _scene_controller_link_copy(
    DvzController* source, DvzController* target, uint32_t components)
{
    if (!_scene_controller_link_endpoint_valid(source) ||
        !_scene_controller_link_endpoint_valid(target) ||
        !_scene_controller_link_components_valid(source, target, components))
    {
        return false;
    }

    switch (source->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        if (source->panzoom == NULL || target->panzoom == NULL)
            return false;
        _scene_controller_link_copy_panzoom(source, target, components);
        return true;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        if (source->arcball == NULL || target->arcball == NULL)
            return false;
        _scene_controller_link_copy_arcball(source->arcball, target->arcball, components);
        return true;
    default:
        return false;
    }
}


/**
 * Apply one controller link.
 *
 * @param link the active link
 * @return the controller mutated by propagation, or NULL when no propagation was applied
 */
static DvzController* _scene_controller_link_apply(DvzControllerLink* link)
{
    if (link == NULL || !link->active ||
        !_scene_controller_link_endpoint_valid(link->source) ||
        !_scene_controller_link_endpoint_valid(link->target))
    {
        return NULL;
    }

    if (!_scene_controller_link_components_valid(link->source, link->target, link->components))
        return NULL;

    const bool source_interacting = _scene_controller_interacting(link->source);
    const bool target_interacting = _scene_controller_interacting(link->target);

    switch (link->mode)
    {
    case DVZ_CONTROLLER_LINK_ONE_WAY:
        if (target_interacting && !source_interacting)
            return NULL;
        return _scene_controller_link_copy(link->source, link->target, link->components)
                   ? link->target
                   : NULL;
    case DVZ_CONTROLLER_LINK_TWO_WAY:
        if (source_interacting && target_interacting)
            return NULL;
        if (target_interacting)
        {
            return _scene_controller_link_copy(link->target, link->source, link->components)
                       ? link->source
                       : NULL;
        }
        return _scene_controller_link_copy(link->source, link->target, link->components)
                   ? link->target
                   : NULL;
    default:
        return NULL;
    }
}



/**
 * Disable all links referencing one controller.
 *
 * @param controller the controller being destroyed
 */
static void _scene_controller_links_destroy_for_controller(DvzController* controller)
{
    if (controller == NULL || controller->scene == NULL)
        return;
    DvzScene* scene = controller->scene;
    for (uint32_t i = 0; i < scene->controller_link_count; i++)
    {
        DvzControllerLink* link = &scene->controller_links[i];
        if (!link->active)
            continue;
        if (link->source == controller || link->target == controller)
            dvz_controller_link_destroy(link);
    }
}



/**
 * Propagate all active controller links in a scene.
 *
 * @param scene the scene
 */
void _dvz_scene_controller_links_propagate(DvzScene* scene)
{
    if (scene == NULL)
        return;
    for (uint32_t i = 0; i < scene->controller_link_count; i++)
    {
        DvzControllerLink* link = &scene->controller_links[i];
        DvzController* mutated = _scene_controller_link_apply(link);
        if (mutated != NULL)
            _scene_notify_controller_figures(mutated);
    }
}



/**
 * Propagate active controller links whose source is one controller.
 *
 * @param source the source controller
 */
static void _scene_controller_links_propagate_from(DvzController* source)
{
    if (!_scene_controller_link_endpoint_valid(source))
        return;
    DvzScene* scene = source->scene;
    for (uint32_t i = 0; i < scene->controller_link_count; i++)
    {
        DvzControllerLink* link = &scene->controller_links[i];
        if (!link->active)
            continue;
        if (link->source != source && (link->mode != DVZ_CONTROLLER_LINK_TWO_WAY ||
                                       link->target != source))
        {
            continue;
        }
        DvzController* mutated = _scene_controller_link_apply(link);
        if (mutated != NULL)
            _scene_notify_controller_figures(mutated);
    }
}



/**
 * Apply one camera controller state to all bound panel cameras.
 *
 * @param controller the controller
 */
static void _scene_apply_controller_to_bound_panels(DvzController* controller)
{
    if (controller == NULL || controller->scene == NULL)
        return;
    DvzScene* scene = controller->scene;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            if (!_scene_panel_has_controller(panel, controller) || panel->camera == NULL)
                continue;
            bool applied = false;
            if (controller->type == DVZ_CONTROLLER_TYPE_FLY && controller->fly != NULL)
            {
                DvzCamera* previous_camera = controller->fly->camera;
                dvz_fly_set_camera(controller->fly, panel->camera);
                controller->fly->camera = previous_camera;
                (void)_scene_panel_sync_fly_pivot_marker(panel);
                applied = true;
            }
            else if (
                controller->type == DVZ_CONTROLLER_TYPE_TURNTABLE &&
                controller->turntable != NULL)
            {
                DvzCamera* previous_camera = controller->turntable->camera;
                dvz_turntable_set_camera(controller->turntable, panel->camera);
                controller->turntable->camera = previous_camera;
                applied = true;
            }
            if (applied)
            {
                panel->active_view_kind = DVZ_PANEL_VIEW_KIND_3D;
                _scene_panel_view3d_dirty(panel);
            }
        }
    }
}



/**
 * Convert a pointer event from figure coordinates to panel-local coordinates.
 *
 * @param ev input event
 * @param x panel x origin
 * @param y panel y origin
 * @param out output event
 */
static void _scene_panel_local_pointer(
    const DvzPointerEvent* ev, float x, float y, DvzPointerEvent* out)
{
    ANN(ev);
    ANN(out);
    *out = *ev;
    out->pos[0] -= x;
    out->pos[1] -= y;
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        out->content.d.press_pos[0] -= x;
        out->content.d.press_pos[1] -= y;
        out->content.d.last_pos[0] -= x;
        out->content.d.last_pos[1] -= y;
    }
}



/**
 * Convert a pointer event from logical window coordinates to figure coordinates.
 *
 * @param panel panel receiving input
 * @param router input router carrying the last resize event
 * @param ev input event
 * @param out output event in figure coordinates
 */
static void _scene_panel_pointer_to_figure_coordinates(
    const DvzPanel* panel, const DvzInputRouter* router, const DvzPointerEvent* ev,
    DvzPointerEvent* out)
{
    ANN(panel);
    ANN(ev);
    ANN(out);
    *out = *ev;

    DvzInputResizeEvent resize = {0};
    bool has_resize = router != NULL && dvz_input_router_last_resize(router, &resize);
    const bool has_event_window =
        isfinite(ev->window_size[0]) && isfinite(ev->window_size[1]) &&
        ev->window_size[0] > 0.0f && ev->window_size[1] > 0.0f;
    const float window_width =
        has_event_window ? ev->window_size[0] : (has_resize ? (float)resize.window_width : 0.0f);
    const float window_height =
        has_event_window ? ev->window_size[1] : (has_resize ? (float)resize.window_height : 0.0f);
    const float content_scale_x =
        has_resize && isfinite(resize.content_scale_x) && resize.content_scale_x > 0.0f ?
            resize.content_scale_x :
            ev->content_scale;
    const float content_scale_y =
        has_resize && isfinite(resize.content_scale_y) && resize.content_scale_y > 0.0f ?
            resize.content_scale_y :
            ev->content_scale;

    (void)dvz_figure_window_to_layout(
        panel->figure, ev->pos[0], ev->pos[1], window_width, window_height, content_scale_x,
        content_scale_y, &out->pos[0], &out->pos[1]);
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        (void)dvz_figure_window_to_layout(
            panel->figure, ev->content.d.press_pos[0], ev->content.d.press_pos[1],
            window_width, window_height, content_scale_x, content_scale_y,
            &out->content.d.press_pos[0], &out->content.d.press_pos[1]);
        (void)dvz_figure_window_to_layout(
            panel->figure, ev->content.d.last_pos[0], ev->content.d.last_pos[1],
            window_width, window_height, content_scale_x, content_scale_y,
            &out->content.d.last_pos[0], &out->content.d.last_pos[1]);

        float zero_x = 0.0f;
        float zero_y = 0.0f;
        float shift_x = out->content.d.shift[0];
        float shift_y = out->content.d.shift[1];
        if (dvz_figure_window_to_layout(
                panel->figure, 0.0f, 0.0f, window_width, window_height, content_scale_x,
                content_scale_y, &zero_x, &zero_y) &&
            dvz_figure_window_to_layout(
                panel->figure, ev->content.d.shift[0], ev->content.d.shift[1], window_width,
                window_height, content_scale_x, content_scale_y, &shift_x, &shift_y))
        {
            out->content.d.shift[0] = shift_x - zero_x;
            out->content.d.shift[1] = shift_y - zero_y;
        }
    }
}



/**
 * Return whether a pointer event targets a panel.
 *
 * @param panel the panel
 * @param ev pointer event
 * @param captured whether a controller has drag capture
 * @return whether the event should be dispatched through the panel
 */
static bool _scene_panel_pointer_targets(
    const DvzPanel* panel, const DvzPointerEvent* ev, bool captured)
{
    ANN(panel);
    ANN(ev);
    if (captured && ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
        return true;

    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &w, &h);
    vec2 pos = {ev->pos[0], ev->pos[1]};
    if (ev->type == DVZ_POINTER_EVENT_DRAG && ev->content.d.is_press_valid)
    {
        pos[0] = ev->content.d.press_pos[0];
        pos[1] = ev->content.d.press_pos[1];
    }
    return pos[0] >= x && pos[0] < x + w && pos[1] >= y && pos[1] < y + h;
}



/**
 * Dispatch one pointer event through a panel-local controller binding.
 *
 * @param panel the panel
 * @param controller the controller
 * @param ev pointer event in figure coordinates
 * @return whether the event was consumed
 */
static bool _scene_panel_dispatch_pointer_controller(
    DvzPanel* panel, DvzController* controller, const DvzPointerEvent* ev)
{
    ANN(panel);
    ANN(controller);
    ANN(ev);
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &w, &h);

    bool consumed = false;
    bool links_propagated = false;
    DvzPanzoom* transient_panzoom_interaction = NULL;
    bool old_panzoom_interacting = false;
    switch (controller->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
    {
        DvzPanzoom* panzoom = controller->panzoom;
        if (panzoom == NULL)
            return false;
        vec2 old_origin = {panzoom->viewport_origin[0], panzoom->viewport_origin[1]};
        vec2 old_size = {panzoom->viewport_size[0], panzoom->viewport_size[1]};
        bool old_has_viewport = panzoom->has_viewport;
        int old_flags = panzoom->flags;
        DvzDimMask dims = _scene_panel_controller_dims(panel, controller);
        if ((dims & DVZ_DIM_MASK_X) == 0)
            panzoom->flags |= DVZ_PANZOOM_FLAGS_FIXED_X;
        if ((dims & DVZ_DIM_MASK_Y) == 0)
            panzoom->flags |= DVZ_PANZOOM_FLAGS_FIXED_Y;
        dvz_panzoom_viewport(panzoom, x, y, w, h);
        const bool transient_interaction =
            ev->type == DVZ_POINTER_EVENT_WHEEL || ev->type == DVZ_POINTER_EVENT_DOUBLE_CLICK;
        if (transient_interaction)
        {
            old_panzoom_interacting = panzoom->interacting;
            panzoom->interacting = true;
            transient_panzoom_interaction = panzoom;
        }
        consumed = dvz_panzoom_pointer(panzoom, ev);
        if (transient_interaction && consumed)
            panzoom->interacting = true;
        glm_vec2_copy(old_origin, panzoom->viewport_origin);
        glm_vec2_copy(old_size, panzoom->viewport_size);
        panzoom->has_viewport = old_has_viewport;
        panzoom->flags = old_flags;
        break;
    }
    case DVZ_CONTROLLER_TYPE_ARCBALL:
    {
        DvzArcball* arcball = controller->arcball;
        if (arcball == NULL || !_scene_panel_pointer_targets(panel, ev, arcball->interacting))
            return false;
        vec2 old_size = {arcball->viewport_size[0], arcball->viewport_size[1]};
        mat4 old_view = GLM_MAT4_IDENTITY_INIT;
        glm_mat4_copy(arcball->view, old_view);
        bool old_has_view = arcball->has_view;
        DvzPointerEvent local = {0};
        _scene_panel_local_pointer(ev, x, y, &local);
        dvz_arcball_resize(arcball, w, h);
        if (panel->camera != NULL)
        {
            DvzMVP camera_mvp = {0};
            glm_mat4_identity(camera_mvp.model);
            glm_mat4_identity(camera_mvp.view);
            glm_mat4_identity(camera_mvp.proj);
            dvz_camera_mvp(panel->camera, &camera_mvp);
            _dvz_arcball_view(arcball, camera_mvp.view);
        }
        else
        {
            _dvz_arcball_clear_view(arcball);
        }
        consumed = dvz_arcball_pointer(arcball, &local);
        if (consumed)
        {
            _scene_controller_links_propagate_from(controller);
            links_propagated = true;
        }
        glm_vec2_copy(old_size, arcball->viewport_size);
        glm_mat4_copy(old_view, arcball->view);
        arcball->has_view = old_has_view;
        break;
    }
    case DVZ_CONTROLLER_TYPE_FLY:
    {
        DvzFly* fly = controller->fly;
        if (fly == NULL || _scene_panel_ensure_camera(panel) == NULL)
            return false;
        vec2 old_origin = {fly->viewport_origin[0], fly->viewport_origin[1]};
        vec2 old_size = {fly->viewport_size[0], fly->viewport_size[1]};
        bool old_has_viewport = fly->has_viewport;
        DvzCamera* old_camera = fly->camera;
        dvz_fly_viewport(fly, x, y, w, h);
        dvz_fly_set_camera(fly, panel->camera);
        consumed = dvz_fly_pointer(fly, ev);
        glm_vec2_copy(old_origin, fly->viewport_origin);
        glm_vec2_copy(old_size, fly->viewport_size);
        fly->has_viewport = old_has_viewport;
        fly->camera = old_camera;
        if (consumed)
            _scene_apply_controller_to_bound_panels(controller);
        break;
    }
    case DVZ_CONTROLLER_TYPE_TURNTABLE:
    {
        DvzTurntable* turntable = controller->turntable;
        if (
            turntable == NULL || _scene_panel_ensure_camera(panel) == NULL ||
            !_scene_panel_pointer_targets(panel, ev, turntable->interacting))
            return false;
        vec2 old_origin = {turntable->viewport_origin[0], turntable->viewport_origin[1]};
        vec2 old_size = {turntable->viewport_size[0], turntable->viewport_size[1]};
        bool old_has_viewport = turntable->has_viewport;
        DvzCamera* old_camera = turntable->camera;
        DvzPointerEvent local = {0};
        _scene_panel_local_pointer(ev, x, y, &local);
        dvz_turntable_viewport(turntable, 0.0f, 0.0f, w, h);
        dvz_turntable_set_camera(turntable, panel->camera);
        consumed = dvz_turntable_pointer(turntable, &local);
        glm_vec2_copy(old_origin, turntable->viewport_origin);
        glm_vec2_copy(old_size, turntable->viewport_size);
        turntable->has_viewport = old_has_viewport;
        turntable->camera = old_camera;
        if (consumed)
            _scene_apply_controller_to_bound_panels(controller);
        break;
    }
    default:
        break;
    }
    if (consumed)
    {
        if (!links_propagated)
            _scene_controller_links_propagate_from(controller);
        _scene_notify_controller_figures(controller);
    }
    if (transient_panzoom_interaction != NULL)
        transient_panzoom_interaction->interacting = old_panzoom_interacting;
    return consumed;
}



/**
 * Dispatch one pointer event through all controller bindings on a panel.
 *
 * @param panel the panel
 * @param ev pointer event in figure coordinates
 * @return whether any controller consumed the event
 */
static bool _scene_panel_dispatch_pointer(DvzPanel* panel, const DvzPointerEvent* ev)
{
    ANN(panel);
    ANN(ev);
    DvzController* seen[3] = {0};
    uint32_t seen_count = 0;
    bool consumed = false;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        DvzController* controller = panel->controllers[dim];
        if (_scene_controller_seen(seen, seen_count, controller))
            continue;
        seen[seen_count++] = controller;
        consumed =
            _scene_panel_dispatch_pointer_controller(panel, controller, ev) || consumed;
    }
    if (panel->item_interaction != NULL)
    {
        bool inside = _scene_panel_pointer_targets(panel, ev, false);
        if (inside)
        {
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;
            _scene_panel_pixel_rect(panel, &x, &y, &w, &h);
            DvzPointerEvent local = {0};
            _scene_panel_local_pointer(ev, x, y, &local);
            (void)_scene_item_interaction_pointer(panel->item_interaction, &local);
        }
        else
            _scene_item_interaction_pointer_leave(panel->item_interaction);
    }
    return consumed;
}



/**
 * Dispatch one keyboard event through all keyboard-aware controller bindings on a panel.
 *
 * @param panel the panel
 * @param ev keyboard event
 * @return whether any controller consumed the event
 */
static bool _scene_panel_dispatch_keyboard(DvzPanel* panel, const DvzKeyboardEvent* ev)
{
    ANN(panel);
    ANN(ev);
    DvzController* seen[3] = {0};
    uint32_t seen_count = 0;
    bool consumed = false;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        DvzController* controller = panel->controllers[dim];
        if (_scene_controller_seen(seen, seen_count, controller))
            continue;
        seen[seen_count++] = controller;
        if (controller->type != DVZ_CONTROLLER_TYPE_FLY || controller->fly == NULL)
            continue;
        consumed = dvz_fly_keyboard(controller->fly, ev) || consumed;
        _scene_notify_controller_figures(controller);
    }
    return consumed;
}



/**
 * Route input events through one panel's bound controller set.
 *
 * @param router input router
 * @param ev input event
 * @param user_data panel pointer
 */
static void _scene_panel_input_callback(
    DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    DvzPanel* panel = (DvzPanel*)user_data;
    if (panel == NULL || ev == NULL)
        return;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
    {
        DvzPointerEvent pointer = {0};
        _scene_panel_pointer_to_figure_coordinates(panel, router, &ev->content.pointer, &pointer);
        (void)_scene_panel_dispatch_pointer(panel, &pointer);
    }
    else if (ev->type == DVZ_INPUT_EVENT_KEYBOARD)
        (void)_scene_panel_dispatch_keyboard(panel, &ev->content.keyboard);
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE && panel->camera != NULL)
    {
        float w = 0.0f;
        float h = 0.0f;
        _scene_panel_pixel_size(panel, &w, &h);
        dvz_camera_resize(panel->camera, w, h);
    }
}


/**
 * Allocate one scene-owned controller slot.
 *
 * @param scene the scene
 * @param type controller type
 * @return the active controller slot, or NULL
 */
static DvzController* _scene_controller(DvzScene* scene, DvzControllerType type)
{
    ANN(scene);
    DvzController* controller = NULL;
    for (uint32_t i = 0; i < scene->controller_count; i++)
    {
        if (!scene->controllers[i].active)
        {
            controller = &scene->controllers[i];
            break;
        }
    }
    if (controller == NULL)
    {
        if (scene->controller_count >= DVZ_SCENE_MAX_CONTROLLERS)
            return NULL;
        controller = &scene->controllers[scene->controller_count++];
    }

    dvz_memset(controller, sizeof(DvzController), 0, sizeof(DvzController));
    controller->scene = scene;
    controller->id = _scene_next_id(scene);
    controller->type = type;
    controller->active = true;
    return controller;
}


DvzId dvz_controller_id(const DvzController* controller)
{
    return controller != NULL && controller->scene != NULL && controller->active ? controller->id
                                                                                : DVZ_ID_NONE;
}



/**
 * Create a scene-owned panzoom controller.
 *
 * @param scene the scene
 * @param desc panzoom descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DvzController* dvz_panzoom(DvzScene* scene, const DvzPanzoomDesc* desc)
{
    ANN(scene);
    DvzPanzoomDesc default_desc = dvz_panzoom_desc();
    if (desc == NULL)
        desc = &default_desc;

    DvzController* controller = _scene_controller(scene, DVZ_CONTROLLER_TYPE_PANZOOM);
    if (controller == NULL)
        return NULL;
    controller->panzoom = dvz_panzoom_create(desc);
    if (controller->panzoom == NULL)
    {
        _scene_controller_destroy(controller);
        return NULL;
    }
    return controller;
}



/**
 * Create a scene-owned arcball controller.
 *
 * @param scene the scene
 * @param desc arcball descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DvzController* dvz_arcball(DvzScene* scene, const DvzArcballDesc* desc)
{
    ANN(scene);
    DvzArcballDesc default_desc = dvz_arcball_desc();
    if (desc == NULL)
        desc = &default_desc;

    DvzController* controller = _scene_controller(scene, DVZ_CONTROLLER_TYPE_ARCBALL);
    if (controller == NULL)
        return NULL;
    controller->arcball = dvz_arcball_create(desc);
    if (controller->arcball == NULL)
    {
        _scene_controller_destroy(controller);
        return NULL;
    }
    return controller;
}



/**
 * Create a scene-owned fly controller.
 *
 * @param scene the scene
 * @param desc fly descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DvzController* dvz_fly(DvzScene* scene, const DvzFlyDesc* desc)
{
    ANN(scene);
    DvzController* controller = _scene_controller(scene, DVZ_CONTROLLER_TYPE_FLY);
    if (controller == NULL)
        return NULL;
    controller->fly = dvz_fly_create(desc);
    if (controller->fly == NULL)
    {
        _scene_controller_destroy(controller);
        return NULL;
    }
    return controller;
}



/**
 * Create a scene-owned turntable controller.
 *
 * @param scene the scene
 * @param desc turntable descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DvzController* dvz_turntable(DvzScene* scene, const DvzTurntableDesc* desc)
{
    ANN(scene);
    DvzController* controller = _scene_controller(scene, DVZ_CONTROLLER_TYPE_TURNTABLE);
    if (controller == NULL)
        return NULL;
    controller->turntable = dvz_turntable_create(desc);
    if (controller->turntable == NULL)
    {
        _scene_controller_destroy(controller);
        return NULL;
    }
    return controller;
}



/**
 * Return the type of a scene-owned controller.
 *
 * @param controller the controller
 * @return the controller type, or DVZ_CONTROLLER_TYPE_NONE
 */
DvzControllerType dvz_controller_type(const DvzController* controller)
{
    if (controller == NULL || !controller->active)
        return DVZ_CONTROLLER_TYPE_NONE;
    return controller->type;
}


/**
 * Destroy a scene-owned controller.
 *
 * @param controller the controller
 */
void dvz_controller_destroy(DvzController* controller)
{
    if (controller == NULL || !controller->active)
        return;
    _scene_controller_detach_from_panels(controller);
    _scene_controller_destroy(controller);
}


/**
 * Create a scene-owned controller state link.
 *
 * @param scene the scene
 * @param source the source controller
 * @param target the target controller
 * @param components bitmask of DvzControllerLinkComponent values
 * @param mode link propagation mode
 * @return the scene-owned link handle, or NULL on validation error
 */
DvzControllerLink* dvz_controller_link(
    DvzScene* scene, DvzController* source, DvzController* target, uint32_t components,
    DvzControllerLinkMode mode)
{
    if (scene == NULL || source == NULL || target == NULL || source == target)
        return NULL;
    if (mode != DVZ_CONTROLLER_LINK_ONE_WAY && mode != DVZ_CONTROLLER_LINK_TWO_WAY)
        return NULL;
    if (source->scene != scene || target->scene != scene)
        return NULL;
    if (!_scene_controller_link_components_valid(source, target, components))
        return NULL;

    DvzControllerLink* link = NULL;
    for (uint32_t i = 0; i < scene->controller_link_count; i++)
    {
        if (!scene->controller_links[i].active)
        {
            link = &scene->controller_links[i];
            break;
        }
    }
    if (link == NULL)
    {
        if (scene->controller_link_count >= DVZ_SCENE_MAX_CONTROLLER_LINKS)
            return NULL;
        link = &scene->controller_links[scene->controller_link_count++];
    }

    dvz_memset(link, sizeof(DvzControllerLink), 0, sizeof(DvzControllerLink));
    link->scene = scene;
    link->source = source;
    link->target = target;
    link->components = components;
    link->mode = mode;
    link->active = true;

    DvzController* mutated = _scene_controller_link_apply(link);
    if (mutated != NULL)
        _scene_notify_controller_figures(mutated);
    return link;
}



/**
 * Destroy a scene-owned controller state link.
 *
 * @param link the link
 */
void dvz_controller_link_destroy(DvzControllerLink* link)
{
    if (link == NULL || !link->active)
        return;
    link->scene = NULL;
    link->source = NULL;
    link->target = NULL;
    link->components = DVZ_CONTROLLER_LINK_NONE;
    link->mode = DVZ_CONTROLLER_LINK_ONE_WAY;
    link->active = false;
}



/**
 * Return the panzoom payload of a panzoom controller.
 *
 * @param controller the controller
 * @return the borrowed panzoom payload, or NULL for the wrong family
 */
DvzPanzoom* dvz_controller_panzoom(DvzController* controller)
{
    if (controller == NULL || !controller->active ||
        controller->type != DVZ_CONTROLLER_TYPE_PANZOOM)
    {
        return NULL;
    }
    return controller->panzoom;
}



/**
 * Return the arcball payload of an arcball controller.
 *
 * @param controller the controller
 * @return the borrowed arcball payload, or NULL for the wrong family
 */
DvzArcball* dvz_controller_arcball(DvzController* controller)
{
    if (controller == NULL || !controller->active ||
        controller->type != DVZ_CONTROLLER_TYPE_ARCBALL)
    {
        return NULL;
    }
    return controller->arcball;
}



/**
 * Return the fly payload of a fly controller.
 *
 * @param controller the controller
 * @return the borrowed fly payload, or NULL for the wrong family
 */
DvzFly* dvz_controller_fly(DvzController* controller)
{
    if (controller == NULL || !controller->active || controller->type != DVZ_CONTROLLER_TYPE_FLY)
        return NULL;
    return controller->fly;
}



/**
 * Return the turntable payload of a turntable controller.
 *
 * @param controller the controller
 * @return the borrowed turntable payload, or NULL for the wrong family
 */
DvzTurntable* dvz_controller_turntable(DvzController* controller)
{
    if (controller == NULL || !controller->active ||
        controller->type != DVZ_CONTROLLER_TYPE_TURNTABLE)
    {
        return NULL;
    }
    return controller->turntable;
}


/**
 * Advance fly controllers attached to a figure.
 *
 * @param figure the figure
 * @param dt elapsed time in seconds
 * @return whether any fly controller still needs animation frames
 */
bool _dvz_figure_fly_update(DvzFigure* figure, double dt)
{
    if (figure == NULL)
        return false;

    double update_dt = dt;
    if (update_dt > DVZ_SCENE_FLY_MAX_DT)
        update_dt = DVZ_SCENE_FLY_MAX_DT;

    DvzController* updated[DVZ_SCENE_MAX_PANELS] = {0};
    uint32_t updated_count = 0;
    bool active = false;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzPanel* panel = &figure->panels[i];
        DvzController* controller = panel->controllers[DVZ_DIM_X];
        if (controller == NULL || controller->type != DVZ_CONTROLLER_TYPE_FLY)
            continue;

        DvzFly* fly = dvz_controller_fly(controller);
        if (fly == NULL)
            continue;
        if (_scene_fly_active(fly))
            active = true;
        if (_scene_controller_seen(updated, updated_count, controller))
            continue;
        dvz_fly_update(fly, update_dt);
        updated[updated_count++] = controller;
    }

    for (uint32_t i = 0; i < updated_count; i++)
        _scene_apply_controller_to_bound_panels(updated[i]);
    return active;
}


/**
 * Bind a scene-owned controller to one panel.
 *
 * @param panel the panel
 * @param controller the scene-owned controller
 * @param dims dimension mask
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL ||
        controller == NULL || !controller->active)
    {
        return -1;
    }
    if (controller->scene != panel->figure->scene)
        return -1;

    switch (controller->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
    {
        const DvzDimMask invalid = dims & ~((DvzDimMask)DVZ_DIM_MASK_XY);
        if (invalid != 0 || dims == DVZ_DIM_MASK_NONE)
            return -1;
        DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
        if (panzoom == NULL)
            return -1;
        if ((dims & DVZ_DIM_MASK_X) != 0)
            panel->controllers[DVZ_DIM_X] = controller;
        if ((dims & DVZ_DIM_MASK_Y) != 0)
            panel->controllers[DVZ_DIM_Y] = controller;
        panel->panzoom = panzoom;
        break;
    }
    case DVZ_CONTROLLER_TYPE_ARCBALL:
    {
        if (dims != DVZ_DIM_MASK_XYZ)
            return -1;
        DvzArcball* arcball = dvz_controller_arcball(controller);
        if (arcball == NULL)
            return -1;
        panel->controllers[DVZ_DIM_X] = controller;
        panel->controllers[DVZ_DIM_Y] = controller;
        panel->controllers[DVZ_DIM_Z] = controller;
        panel->arcball = arcball;
        break;
    }
    case DVZ_CONTROLLER_TYPE_FLY:
    {
        if (dims != DVZ_DIM_MASK_XYZ)
            return -1;
        DvzFly* fly = dvz_controller_fly(controller);
        if (fly == NULL)
            return -1;
        DvzCamera* camera = _scene_panel_ensure_camera(panel);
        if (camera == NULL)
            return -1;
        panel->controllers[DVZ_DIM_X] = controller;
        panel->controllers[DVZ_DIM_Y] = controller;
        panel->controllers[DVZ_DIM_Z] = controller;
        panel->fly = fly;
        float w = 0.0f;
        float h = 0.0f;
        _scene_panel_pixel_size(panel, &w, &h);
        dvz_camera_resize(camera, w, h);
        _scene_apply_controller_to_bound_panels(controller);
        break;
    }
    case DVZ_CONTROLLER_TYPE_TURNTABLE:
    {
        if (dims != DVZ_DIM_MASK_XYZ)
            return -1;
        DvzTurntable* turntable = dvz_controller_turntable(controller);
        if (turntable == NULL)
            return -1;
        DvzCamera* camera = _scene_panel_ensure_camera(panel);
        if (camera == NULL)
            return -1;
        panel->controllers[DVZ_DIM_X] = controller;
        panel->controllers[DVZ_DIM_Y] = controller;
        panel->controllers[DVZ_DIM_Z] = controller;
        panel->turntable = turntable;
        float w = 0.0f;
        float h = 0.0f;
        _scene_panel_pixel_size(panel, &w, &h);
        dvz_camera_resize(camera, w, h);
        _scene_apply_controller_to_bound_panels(controller);
        break;
    }
    default:
        return -1;
    }
    _scene_notify_request_frame(panel->figure);
    return 0;
}



/**
 * Route an input router through one panel's bound controllers.
 *
 * @param panel the panel
 * @param router input router to subscribe to, or NULL to disconnect
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_connect_input(DvzPanel* panel, DvzInputRouter* router)
{
    if (panel == NULL)
        return -1;
    if (panel->input_router == router)
        return 0;
    if (panel->input_router != NULL)
        dvz_input_unsubscribe_event(
            panel->input_router, _scene_panel_input_callback, panel);
    panel->input_router = router;
    if (router != NULL)
        dvz_input_subscribe_event(router, _scene_panel_input_callback, panel);
    return 0;
}



/**
 * Return the controller bound to one panel dimension.
 *
 * @param panel the panel
 * @param dim the dimension
 * @return the borrowed controller handle, or NULL
 */
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim)
{
    if (panel == NULL || dim < DVZ_DIM_X || dim > DVZ_DIM_Z)
        return NULL;
    return panel->controllers[dim];
}



/**
 * Set or replace the camera attached to a panel.
 *
 * @param panel the panel
 * @param desc the camera descriptor, or NULL for defaults
 * @return the panel-owned camera
 */
DvzCamera* dvz_panel_set_camera(DvzPanel* panel, const DvzCameraDesc* desc)
{
    ANN(panel);
    if (panel->camera != NULL)
        dvz_camera_destroy(panel->camera);
    panel->camera = _dvz_camera(desc);
    if (panel->camera != NULL)
    {
        panel->active_view_kind = DVZ_PANEL_VIEW_KIND_3D;
        float w = 0.0f;
        float h = 0.0f;
        _scene_panel_pixel_size(panel, &w, &h);
        dvz_camera_resize(panel->camera, w, h);
        DvzController* seen[3] = {0};
        uint32_t seen_count = 0;
        for (uint32_t dim = 0; dim < 3; dim++)
        {
            DvzController* controller = panel->controllers[dim];
            if (_scene_controller_seen(seen, seen_count, controller))
                continue;
            seen[seen_count++] = controller;
            if (controller->type == DVZ_CONTROLLER_TYPE_FLY ||
                controller->type == DVZ_CONTROLLER_TYPE_TURNTABLE)
            {
                _scene_apply_controller_to_bound_panels(controller);
            }
        }
    }
    _scene_panel_view3d_dirty(panel);
    return panel->camera;
}


/**
 * Return the camera attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned camera, or NULL
 */
DvzCamera* dvz_panel_camera(DvzPanel* panel)
{
    ANN(panel);
    return panel->camera;
}
