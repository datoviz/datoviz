/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* coordinate_system - This example shows the Datoviz 3D coordinate convention.
 *
 * Scenario: feature.coordinate_system
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/coordinate_system
 * Run:    ./build/examples/c/features/coordinate_system --live
 * Smoke:  ./build/examples/c/features/coordinate_system --png
 *
 * What to look for: the axis triad uses red for X, green for Y, and blue for Z, with labels and a
 * reference grid providing orientation cues. The live example binds a turntable controller, so the
 * axes can be inspected from different viewpoints while the world-up direction remains legible.
 * This is useful when checking imported coordinates, camera setup, and the sign or orientation of
 * 3D scientific data.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/geom.h"
#include "datoviz/math/vec.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

static const double AXIS_LENGTH = 1.45;
static const float GRID_SIZE = 10.0f;
static const float GRID_SPACING = 0.25f;
static const float CAMERA_NEAR_CLIP = 0.005f;
static const float CAMERA_FAR_CLIP = 100.0f;
static const float TURNTABLE_MIN_DISTANCE = 0.03f;
static const float TURNTABLE_MAX_DISTANCE = 24.0f;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_coordinate_system_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the standard axis color.
 *
 * @param axis axis index, 0=X, 1=Y, 2=Z
 * @return RGBA color
 */
static DvzColor _axis_color(uint32_t axis)
{
    switch (axis)
    {
    case 0:
        return dvz_color_rgba(230, 64, 64, 255);
    case 1:
        return dvz_color_rgba(52, 190, 88, 255);
    case 2:
        return dvz_color_rgba(66, 132, 255, 255);
    default:
        return dvz_color_rgba(220, 226, 235, 255);
    }
}



/**
 * Build an axis transform for a Z-axis arrow.
 *
 * @param axis axis index, 0=X, 1=Y, 2=Z
 * @param out output transform
 */
static void _axis_transform(uint32_t axis, dmat4 out)
{
    dvz_dmat4_identity(out);
    if (axis == 0)
    {
        out[0][0] = 0.0;
        out[0][1] = 0.0;
        out[0][2] = -1.0;
        out[1][0] = 0.0;
        out[1][1] = 1.0;
        out[1][2] = 0.0;
        out[2][0] = 1.0;
        out[2][1] = 0.0;
        out[2][2] = 0.0;
    }
    else if (axis == 1)
    {
        out[0][0] = 1.0;
        out[0][1] = 0.0;
        out[0][2] = 0.0;
        out[1][0] = 0.0;
        out[1][1] = 0.0;
        out[1][2] = -1.0;
        out[2][0] = 0.0;
        out[2][1] = 1.0;
        out[2][2] = 0.0;
    }
}



/**
 * Upload one geometry as a lit retained mesh and destroy the CPU geometry copy.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param geometry geometry to upload
 * @return true when the mesh was added
 */
static bool _add_geometry(DvzScene* scene, DvzPanel* panel, DvzGeometry* geometry)
{
    if (geometry == NULL)
        return false;

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.phong.ambient = 0.46f;
    material.phong.diffuse = 0.70f;
    material.phong.specular = 0.18f;
    material.phong.shininess = 18.0f;

    const bool ok = dvz_visual_set_material(mesh, &material) == 0 &&
                    dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    dvz_geometry_destroy(geometry);
    return ok;
}



/**
 * Add one colored 3D arrow axis.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param axis axis index, 0=X, 1=Y, 2=Z
 * @return true when the axis was added
 */
static bool _add_axis_arrow(DvzScene* scene, DvzPanel* panel, uint32_t axis)
{
    DvzGeometry* arrow = dvz_geometry_arrow(&(DvzGeometryArrowDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryArrowDesc),
        .center = {0.0, 0.0, 0.5 * AXIS_LENGTH},
        .length = AXIS_LENGTH,
        .shaft_radius = 0.035,
        .head_radius = 0.105,
        .head_length = 0.26,
        .sectors = 64,
        .color = _axis_color(axis),
    });
    if (arrow == NULL)
        return false;

    dmat4 transform = _DMAT4_IDENTITY_INIT;
    _axis_transform(axis, transform);
    if (dvz_geometry_transform(arrow, transform) != 0)
    {
        dvz_geometry_destroy(arrow);
        return false;
    }
    return _add_geometry(scene, panel, arrow);
}



/**
 * Add a small origin sphere.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the origin marker was added
 */
static bool _add_origin(DvzScene* scene, DvzPanel* panel)
{
    return _add_geometry(
        scene, panel,
        dvz_geometry_sphere(&(DvzGeometrySphereDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
            .center = {0.0, 0.0, 0.0},
            .radius = 0.075,
            .rings = 24,
            .sectors = 48,
            .color = dvz_color_rgba(230, 236, 244, 255),
        }));
}



/**
 * Add a muted XZ reference grid through the origin.
 *
 * @param panel panel receiving the grid
 * @return true when the grid was added
 */
static bool _add_reference_grid(DvzPanel* panel)
{
    DvzReferenceGridDesc grid = dvz_reference_grid_desc();
    grid.plane = DVZ_REFERENCE_GRID_XZ;
    grid.origin[0] = 0.0f;
    grid.origin[1] = 0.0f;
    grid.origin[2] = 0.0f;
    grid.size[0] = GRID_SIZE;
    grid.size[1] = GRID_SIZE;
    grid.spacing = GRID_SPACING;
    grid.major_every = 4;
    grid.minor_color = dvz_color_rgba(74, 86, 98, 95);
    grid.major_color = dvz_color_rgba(116, 132, 148, 145);
    grid.axis_color = dvz_color_rgba(176, 190, 204, 185);
    grid.minor_width_px = 1.0f;
    grid.major_width_px = 1.5f;
    grid.axis_width_px = 2.0f;
    grid.depth_test = true;
    return dvz_reference_grid(panel, &grid) != NULL;
}



/**
 * Add one world-positioned axis label.
 *
 * @param panel target panel
 * @param string label string
 * @param position world position
 * @param color text color
 * @return true when the label was added
 */
static bool
_add_label(DvzPanel* panel, const char* string, const double position[3], DvzColor color)
{
    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;

    DvzTextStyle style = dvz_text_style();
    style.size_px = 46.0f;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = color.a;
    if (dvz_text_set_style(text, &style) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_WORLD;
    placement.position[0] = position[0];
    placement.position[1] = position[1];
    placement.position[2] = position[2];
    placement.offset[0] = 0.0f;
    placement.offset[1] = -8.0f;
    placement.text_anchor[0] = 0.5f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    placement.depth_test = false;
    dvz_text_set_placement(text, &placement);
    dvz_text_set_string(text, string);
    return true;
}



/**
 * Add RGB axis labels at arrow tips.
 *
 * @param panel target panel
 * @return true when all labels were added
 */
static bool _add_axis_labels(DvzPanel* panel)
{
    const double pad = 0.18;
    const double x[3] = {AXIS_LENGTH + pad, 0.0, 0.0};
    const double y[3] = {0.0, AXIS_LENGTH + pad, 0.0};
    const double z[3] = {0.0, 0.0, AXIS_LENGTH + pad};
    return _add_label(panel, "X", x, _axis_color(0)) &&
           _add_label(panel, "Y", y, _axis_color(1)) && _add_label(panel, "Z", z, _axis_color(2));
}



/**
 * Return the default camera descriptor used for static smoke captures and live interaction.
 *
 * @return camera descriptor
 */
static DvzCameraDesc _camera_desc(void)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 1.15f;
    camera.view.eye[1] = 1.75f;
    camera.view.eye[2] = 4.75f;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = 0.0f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 1.0f;
    camera.view.up[2] = 0.0f;
    camera.projection.fov_y = 0.74f;
    camera.projection.near_clip = CAMERA_NEAR_CLIP;
    camera.projection.far_clip = CAMERA_FAR_CLIP;
    return camera;
}



/**
 * Attach a turntable controller so the live example is mouse-inspectable.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @return true when the controller was bound
 */
static bool _bind_turntable(DvzScenarioContext* ctx, DvzPanel* panel)
{
    DvzTurntableDesc desc = dvz_turntable_desc();
    DvzCameraDesc camera = _camera_desc();
    desc.initial_view = camera.view;
    desc.min_distance = TURNTABLE_MIN_DISTANCE;
    desc.max_distance = TURNTABLE_MAX_DISTANCE;

    DvzController* controller = dvz_turntable(ctx->scene, &desc);
    if (controller == NULL)
        return false;
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    if (turntable == NULL)
        return false;
    dvz_turntable_pivot(turntable, (vec3){0.0f, 0.0f, 0.0f});
    return dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the coordinate-system proof scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = _camera_desc();
    if (dvz_panel_set_camera_desc(panel, &camera) != 0)
        return false;

    return _add_reference_grid(panel) && _add_axis_arrow(ctx->scene, panel, 0) &&
           _add_axis_arrow(ctx->scene, panel, 1) && _add_axis_arrow(ctx->scene, panel, 2) &&
           _add_origin(ctx->scene, panel) && _add_axis_labels(panel) &&
           _bind_turntable(ctx, panel);
}



/**
 * Return the coordinate-system scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_coordinate_system_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_coordinate_system",
        .title = "Coordinate System",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |
                        DVZ_SCENARIO_REQ_CONTROLLER,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the coordinate-system feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_coordinate_system_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
