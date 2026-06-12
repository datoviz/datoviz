/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene orientation gizmo                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/orientation_gizmo_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ORIENTATION_GIZMO_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate one orientation-gizmo descriptor.
 *
 * @param desc descriptor, or NULL
 * @return whether the descriptor is valid
 */
static bool _orientation_gizmo_desc_validate(const DvzOrientationGizmoDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            desc, DvzOrientationGizmoDesc, DVZ_ORIENTATION_GIZMO_DESC_KNOWN_FLAGS))
    {
        log_error("invalid orientation gizmo descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Resolve the source controller for one gizmo.
 *
 * @param source_panel source panel
 * @param desc descriptor
 * @return source controller, or NULL if unavailable
 */
static DvzController* _orientation_gizmo_source_controller(
    DvzPanel* source_panel, const DvzOrientationGizmoDesc* desc)
{
    ANN(source_panel);
    ANN(desc);
    if (desc->source_controller != NULL)
        return desc->source_controller;
    return dvz_panel_controller(source_panel, DVZ_DIM_X);
}


/**
 * Resolve the inset panel descriptor from the source panel and placement.
 *
 * @param source_panel source panel
 * @param placement placement descriptor
 * @param out output normalized panel descriptor
 * @return whether the descriptor was resolved
 */
static bool _orientation_gizmo_panel_desc(
    const DvzPanel* source_panel, const DvzPlacement* placement, DvzPanelDesc* out)
{
    ANN(source_panel);
    ANN(placement);
    ANN(out);
    if (source_panel->figure == NULL || source_panel->figure->width == 0 ||
        source_panel->figure->height == 0)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(source_panel, &panel_x, &panel_y, &panel_width, &panel_height);

    DvzRect panel_rect = {
        .x = panel_x,
        .y = panel_y,
        .width = panel_width,
        .height = panel_height,
    };
    DvzRect figure_rect = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)source_panel->figure->width,
        .height = (float)source_panel->figure->height,
    };
    DvzRect rect = {0};
    if (!dvz_placement_resolve(placement, &panel_rect, &figure_rect, &rect))
        return false;

    const float x = panel_x + rect.x;
    const float y = panel_y + rect.y;
    if (rect.width <= 0.0f || rect.height <= 0.0f)
        return false;
    *out = (DvzPanelDesc){
        .x = x / figure_rect.width,
        .y = y / figure_rect.height,
        .width = rect.width / figure_rect.width,
        .height = rect.height / figure_rect.height,
    };
    return true;
}


/**
 * Refresh one gizmo inset panel layout from its placement.
 *
 * @param gizmo orientation gizmo
 * @return whether the layout is valid
 */
static bool _orientation_gizmo_refresh_layout(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    if (!gizmo->active || gizmo->source_panel == NULL || gizmo->panel == NULL)
        return false;

    DvzPanelDesc desc = {0};
    if (!_orientation_gizmo_panel_desc(gizmo->source_panel, &gizmo->desc.placement, &desc))
        return false;
    if (!dvz_panel_set_desc(gizmo->panel, desc))
        return false;

    if (gizmo->panel->camera != NULL)
    {
        float width = 0.0f;
        float height = 0.0f;
        _scene_panel_pixel_size(gizmo->panel, &width, &height);
        dvz_camera_resize(gizmo->panel->camera, width, height);
    }
    return true;
}


/**
 * Populate the retained segment visual for one gizmo.
 *
 * @param gizmo orientation gizmo
 * @return whether data upload succeeded
 */
static bool _orientation_gizmo_update_axes(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    ANN(gizmo->axes_visual);
    vec3 start[3] = {{0}};
    vec3 end[3] = {
        {gizmo->desc.axis_length, 0.0f, 0.0f},
        {0.0f, gizmo->desc.axis_length, 0.0f},
        {0.0f, 0.0f, gizmo->desc.axis_length},
    };
    DvzColor color[3] = {
        gizmo->desc.x_color,
        gizmo->desc.y_color,
        gizmo->desc.z_color,
    };
    float width[3] = {
        gizmo->desc.axis_width_px,
        gizmo->desc.axis_width_px,
        gizmo->desc.axis_width_px,
    };

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = start, .item_count = 3},
        {.attr_name = "position_end", .data = end, .item_count = 3},
        {.attr_name = "color", .data = color, .item_count = 3},
        {.attr_name = "stroke_width", .data = width, .item_count = 3},
    };
    return dvz_visual_set_data_many(gizmo->axes_visual, updates, 4) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default orientation-gizmo descriptor.
 *
 * @return default orientation-gizmo descriptor
 */
DvzOrientationGizmoDesc dvz_orientation_gizmo_desc(void)
{
    return (DvzOrientationGizmoDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzOrientationGizmoDesc),
        .source_controller = NULL,
        .placement =
            (DvzPlacement){
                .space = DVZ_PLACEMENT_SPACE_PANEL,
                .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
                .vertical_anchor = DVZ_VERTICAL_ANCHOR_BOTTOM,
                .offset_x_px = -16.0f,
                .offset_y_px = -16.0f,
                .width_px = 150.0f,
                .height_px = 150.0f,
            },
        .show_axes = true,
        .axis_length = 0.82f,
        .axis_width_px = 5.0f,
        .x_color = {242, 80, 86, 255},
        .y_color = {86, 196, 126, 255},
        .z_color = {78, 150, 250, 255},
    };
}


/**
 * Create a passive orientation gizmo attached to one source panel.
 *
 * @param panel source panel
 * @param desc descriptor, or NULL for defaults
 * @return the orientation gizmo, or NULL on validation/allocation error
 */
DvzOrientationGizmo* dvz_orientation_gizmo(
    DvzPanel* panel, const DvzOrientationGizmoDesc* desc)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (!_orientation_gizmo_desc_validate(desc))
        return NULL;
    DvzOrientationGizmoDesc resolved = desc != NULL ? *desc : dvz_orientation_gizmo_desc();
    if (!isfinite(resolved.axis_length) || resolved.axis_length <= 0.0f ||
        !isfinite(resolved.axis_width_px) || resolved.axis_width_px <= 0.0f)
        return NULL;

    DvzScene* scene = panel->figure->scene;
    DvzController* source_controller = _orientation_gizmo_source_controller(panel, &resolved);
    if (source_controller == NULL || source_controller->scene != scene ||
        source_controller->type != DVZ_CONTROLLER_TYPE_ARCBALL)
    {
        log_error("orientation gizmo requires an arcball source controller");
        return NULL;
    }

    DvzOrientationGizmo* gizmo = NULL;
    for (uint32_t i = 0; i < scene->orientation_gizmo_count; i++)
    {
        if (!scene->orientation_gizmos[i].active)
        {
            gizmo = &scene->orientation_gizmos[i];
            break;
        }
    }
    if (gizmo == NULL)
    {
        if (scene->orientation_gizmo_count >= DVZ_SCENE_MAX_ORIENTATION_GIZMOS)
            return NULL;
        gizmo = &scene->orientation_gizmos[scene->orientation_gizmo_count++];
    }
    dvz_memset(gizmo, sizeof(DvzOrientationGizmo), 0, sizeof(DvzOrientationGizmo));
    gizmo->scene = scene;
    gizmo->source_panel = panel;
    gizmo->source_controller = source_controller;
    gizmo->desc = resolved;
    gizmo->active = true;
    gizmo->visible = true;
    gizmo->version = 1;

    DvzPanelDesc panel_desc = {0};
    if (!_orientation_gizmo_panel_desc(panel, &resolved.placement, &panel_desc))
        goto fail;
    gizmo->panel = dvz_panel(panel->figure, panel_desc);
    if (gizmo->panel == NULL)
        goto fail;

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = 0.0f;
    camera.eye[2] = 3.0f;
    camera.target[0] = 0.0f;
    camera.target[1] = 0.0f;
    camera.target[2] = 0.0f;
    camera.fov_y = 0.64f;
    camera.near = 0.05f;
    camera.far = 20.0f;
    if (dvz_panel_set_camera(gizmo->panel, &camera) == NULL)
        goto fail;
    dvz_panel_set_background_color(gizmo->panel, 0.0f, 0.0f, 0.0f, 0.0f);

    gizmo->axes_visual = dvz_segment(scene, 0);
    if (gizmo->axes_visual == NULL)
        goto fail;
    if (dvz_segment_set_caps(gizmo->axes_visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_TRIANGLE_OUT) != 0)
        goto fail;
    if (!_orientation_gizmo_update_axes(gizmo))
        goto fail;
    dvz_visual_set_visible(gizmo->axes_visual, resolved.show_axes);
    if (dvz_panel_add_visual(gizmo->panel, gizmo->axes_visual, NULL) != 0)
        goto fail;

    gizmo->controller = dvz_arcball(scene, NULL);
    if (gizmo->controller == NULL)
        goto fail;
    if (dvz_panel_bind_controller(gizmo->panel, gizmo->controller, DVZ_DIM_MASK_XYZ) != 0)
        goto fail;
    gizmo->link = dvz_controller_link(
        scene, source_controller, gizmo->controller, DVZ_CONTROLLER_LINK_ROTATION,
        DVZ_CONTROLLER_LINK_ONE_WAY);
    if (gizmo->link == NULL)
        goto fail;

    _scene_notify_request_frame(panel->figure);
    return gizmo;

fail:
    dvz_orientation_gizmo_destroy(gizmo);
    return NULL;
}


/**
 * Destroy an orientation gizmo.
 *
 * @param gizmo the orientation gizmo
 */
void dvz_orientation_gizmo_destroy(DvzOrientationGizmo* gizmo)
{
    if (gizmo == NULL || !gizmo->active)
        return;
    DvzFigure* figure = gizmo->source_panel != NULL ? gizmo->source_panel->figure : NULL;
    if (gizmo->link != NULL)
        dvz_controller_link_destroy(gizmo->link);
    if (gizmo->controller != NULL)
        dvz_controller_destroy(gizmo->controller);
    if (gizmo->axes_visual != NULL)
        dvz_visual_set_visible(gizmo->axes_visual, false);
    if (gizmo->panel != NULL)
        dvz_panel_destroy(gizmo->panel);
    dvz_memset(gizmo, sizeof(DvzOrientationGizmo), 0, sizeof(DvzOrientationGizmo));
    _scene_notify_request_frame(figure);
}


/**
 * Set orientation-gizmo visibility.
 *
 * @param gizmo the orientation gizmo
 * @param visible whether the gizmo should be visible
 */
void dvz_orientation_gizmo_set_visible(DvzOrientationGizmo* gizmo, bool visible)
{
    if (gizmo == NULL || !gizmo->active)
        return;
    if (gizmo->visible == visible)
        return;
    gizmo->visible = visible;
    if (gizmo->axes_visual != NULL)
        dvz_visual_set_visible(gizmo->axes_visual, visible && gizmo->desc.show_axes);
    gizmo->version = gizmo->version == UINT64_MAX ? 1 : gizmo->version + 1;
    _scene_notify_request_frame(gizmo->source_panel != NULL ? gizmo->source_panel->figure : NULL);
}


/**
 * Refresh layout for all active orientation gizmos owned by one figure.
 *
 * @param figure the figure
 */
void _scene_prepare_orientation_gizmos(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->orientation_gizmo_count; i++)
    {
        DvzOrientationGizmo* gizmo = &scene->orientation_gizmos[i];
        if (!gizmo->active || gizmo->source_panel == NULL || gizmo->source_panel->figure != figure)
            continue;
        (void)_orientation_gizmo_refresh_layout(gizmo);
    }
}
