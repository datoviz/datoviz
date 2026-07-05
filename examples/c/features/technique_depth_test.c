/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_test - side-by-side visual depth-test toggle with overlapping 3D points.
 *
 * Scenario: feature.depth_test
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_depth_test
 * Run:    ./build/examples/c/features/technique_depth_test --live
 * Smoke:  ./build/examples/c/features/technique_depth_test --png
 *
 * One panel keeps depth testing enabled. The other disables depth testing on the same retained
 * point visual, so later far points overdraw nearer points.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 8u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_depth_test_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add overlapping 3D points to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param depth_test_enabled whether the visual should test scene depth
 * @return true when the visual was added
 */
static bool _add_depth_points(DvzScene* scene, DvzPanel* panel, bool depth_test_enabled)
{
    const float s = 0.15f;
    const vec3 positions[POINT_COUNT] = {
        {-s, -s, +s}, {+s, -s, +s}, {-s, +s, +s}, {+s, +s, +s},
        {-s, -s, -s}, {+s, -s, -s}, {-s, +s, -s}, {+s, +s, -s},
    };
    float diameters[POINT_COUNT] = {0};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        diameters[i] = 100.0f;
    }
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
    };
    for (uint32_t i = 0; i < POINT_COUNT; i++)
        colors[i].a = 248;

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.stroke_width_px = 2.5f;
    style.edge_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.edge_color.a = 255;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, depth_test_enabled) != 0)
        return false;

    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Apply the shared camera used by both comparison panels.
 *
 * @param panel target panel
 * @return true when the camera was applied
 */
static bool _set_depth_camera(DvzPanel* panel)
{
    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = 0.00f;
    camera_desc.view.eye[1] = 0.00f;
    camera_desc.view.eye[2] = 10.00f;
    camera_desc.projection.fov_y = 0.20f;
    camera_desc.projection.near_clip = 0.05f;
    camera_desc.projection.far_clip = 100.0f;
    return dvz_panel_set_camera_desc(panel, &camera_desc) == 0;
}



/**
 * Bind an arcball controller to one comparison panel.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @return controller, or NULL on failure
 */
static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, (vec3){0.5f, 0.5f, 0.0f});
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the visual depth-test scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}) != DVZ_OK)
        return false;
    if (dvz_grid_set_gutter(grid, 30.0f, 0.0f) != DVZ_OK)
        return false;

    DvzPanel* depth_on = dvz_grid_panel(grid, 0, 0);
    DvzPanel* depth_off = dvz_grid_panel(grid, 0, 1);
    if (depth_on == NULL || depth_off == NULL)
        return false;
    example_graphite_cyan_set_panel_background(depth_on);
    example_graphite_cyan_set_panel_background(depth_off);

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;

    DvzLabelDesc on_label = dvz_label_desc();
    on_label.text = "depth test on";
    DvzAnnotation* annotation = dvz_annotation_label(depth_on, &on_label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    DvzLabelDesc off_label = on_label;
    off_label.text = "depth test off";
    annotation = dvz_annotation_label(depth_off, &off_label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    if (!_set_depth_camera(depth_on))
        return false;
    if (!_set_depth_camera(depth_off))
        return false;
    DvzController* on_controller = _bind_arcball(ctx, depth_on);
    DvzController* off_controller = _bind_arcball(ctx, depth_off);
    if (on_controller == NULL || off_controller == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, on_controller, off_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    if (!_add_depth_points(ctx->scene, depth_on, true))
        return false;
    if (!_add_depth_points(ctx->scene, depth_off, false))
        return false;

    return true;
}



/**
 * Return the depth-test scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_depth_test_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_depth_test",
        .title = "Depth Test Toggle",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the visual depth-test feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_depth_test_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
