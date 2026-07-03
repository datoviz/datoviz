/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* view_size_policies - explicit canvas/window/framebuffer size policy selection.
 *
 * Scenario: feature.view_size_policies
 * Style: features, graphite_cyan
 *
 * Build:  just example-c features/view_size_policies
 * Run:    ./build/examples/c/features/view_size_policies --policy reference
 * Smoke:  ./build/examples/c/features/view_size_policies --policy pixel --frames 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static const char* _policy_name(DvzViewSizePolicy policy)
{
    switch (policy)
    {
    case DVZ_VIEW_SIZE_FRAMEBUFFER_PX:
        return "pixel_exact";
    case DVZ_VIEW_SIZE_HOST_LOGICAL_PX:
        return "host_logical_px";
    case DVZ_VIEW_SIZE_REFERENCE_PX:
        return "reference_px";
    case DVZ_VIEW_SIZE_PHYSICAL_MM:
        return "physical_mm";
    default:
        return "unknown";
    }
}



static DvzViewSizeDesc _parse_size_policy(int argc, char** argv)
{
    const char* policy = "reference";
    (void)example_arg_value(argc, argv, "--policy", &policy);

    if (strcmp(policy, "pixel") == 0 || strcmp(policy, "pixel_exact") == 0)
        return dvz_view_size_desc_framebuffer_px(WIDTH, HEIGHT);
    if (strcmp(policy, "host") == 0 || strcmp(policy, "host_logical") == 0 ||
        strcmp(policy, "host_logical_px") == 0)
        return dvz_view_size_desc_host_logical_px(WIDTH, HEIGHT);
    if (strcmp(policy, "physical") == 0 || strcmp(policy, "physical_mm") == 0)
        return dvz_view_size_desc_physical_mm(338.7, 190.5, 96.0);
    return dvz_view_size_desc_reference_px((double)WIDTH, (double)HEIGHT, 96.0);
}



static bool _parse_frame_count(int argc, char** argv, uint32_t* out)
{
    if (out == NULL)
        return false;

    const char* value = NULL;
    if (example_arg_value(argc, argv, "--frames", &value))
        return example_parse_u32(value, out);

    *out = example_frame_count_any(argc, argv);
    return true;
}



static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[POINT_COUNT] = {
        {-0.70f, -0.35f, 0.0f},
        {-0.35f, +0.26f, 0.0f},
        {+0.00f, +0.00f, 0.0f},
        {+0.35f, -0.24f, 0.0f},
        {+0.70f, +0.36f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        {245, 245, 245, 255},
        {47, 197, 205, 255},
        {250, 219, 82, 255},
        {223, 84, 116, 255},
        {110, 137, 241, 255},
    };
    float diameters[POINT_COUNT] = {26.0f, 34.0f, 46.0f, 58.0f, 70.0f};

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, DVZ_ARRAY_COUNT(updates)) != 0)
        return false;
    DvzPointStyleDesc style = dvz_point_style_desc();
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



static void _print_resolved(const DvzResolvedViewSize* resolved)
{
    dvz_fprintf(stdout, "policy: %s\n", _policy_name(resolved->requested_policy));
    dvz_fprintf(
        stdout, "canvas_px: %.1fx%.1f\n", resolved->canvas_width_px, resolved->canvas_height_px);
    dvz_fprintf(
        stdout,
        "host_logical_px: %ux%u\n", resolved->host_logical_width,
        resolved->host_logical_height);
    dvz_fprintf(
        stdout,
        "framebuffer_px: %ux%u\n", resolved->framebuffer_width,
        resolved->framebuffer_height);
    dvz_fprintf(
        stdout,
        "framebuffer_per_canvas_px: %.3fx%.3f\n", resolved->framebuffer_per_canvas_px_x,
        resolved->framebuffer_per_canvas_px_y);
    if (resolved->target_width_mm > 0.0 && resolved->target_height_mm > 0.0)
        dvz_fprintf(
            stdout,
            "target_physical_mm: %.1fx%.1f\n", resolved->target_width_mm,
            resolved->target_height_mm);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    uint32_t frame_count = 0;

    EXAMPLE_CHECK(_parse_frame_count(argc, argv, &frame_count), "invalid --frames value");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzViewSizeDesc size = _parse_size_policy(argc, argv);
    DvzResolvedViewSize initial = dvz_view_size_resolve(&size, DVZ_VIEW_GLFW);

    DvzFigure* figure = dvz_figure(
        scene, initial.host_logical_width, initial.host_logical_height, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create figure/panel");
    example_graphite_cyan_set_panel_background(panel);
    EXAMPLE_CHECK(_add_points(scene, panel), "failed to add points");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_GLFW);
    desc.size = size;
    desc.title = "View Size Policies";
    DvzView* view = dvz_view(app, figure, &desc);
    EXAMPLE_CHECK(view != NULL, "dvz_view() failed");
    EXAMPLE_CHECK(dvz_view_panzoom(view, panel, NULL) != NULL, "dvz_view_panzoom() failed");

    DvzResolvedViewSize resolved = dvz_view_resolved_size(view);
    _print_resolved(&resolved);

    dvz_app_run(app, frame_count);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
