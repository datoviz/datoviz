/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* edl compares a dense 3D point cloud with and without Eye-Dome Lighting.
 *
 * What to look for: both panels upload the same depth-rich point cloud with position, color, and
 * size arrays, but only the right panel enables EDL. In live mode, use the GUI to adjust EDL
 * radius, strength, depth scale, and arcball state while both views stay linked. EDL is useful for
 * point clouds because it strengthens local depth discontinuities without changing the underlying
 * point data.
 *
 * Scenario: feature.edl
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_edl
 * Run:    ./build/examples/c/features/technique_edl --live
 * Smoke:  ./build/examples/c/features/technique_edl --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_gui_controls.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 192u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct EdlDemoState
{
    DvzPanel* lit_panel;
    DvzArcball* arcball;
    DvzExampleGuiEdlControls edl;
    vec3 arcball_angles;
    vec2 arcball_pan;
    float arcball_zoom;
} EdlDemoState;



static DvzEdlDesc _edl_desc_from_controls(const DvzExampleGuiEdlControls* controls)
{
    DvzEdlDesc edl = dvz_edl_desc();
    if (controls == NULL)
        return edl;

    edl.radius = controls->radius;
    edl.strength = controls->strength;
    edl.depth_scale = controls->depth_scale;
    return edl;
}



static void _apply_edl(EdlDemoState* state)
{
    if (state == NULL || state->lit_panel == NULL)
        return;

    if (!state->edl.enabled)
    {
        (void)dvz_panel_set_edl(state->lit_panel, NULL);
        return;
    }

    DvzEdlDesc edl = _edl_desc_from_controls(&state->edl);
    (void)dvz_panel_set_edl(state->lit_panel, &edl);
}



static void _apply_arcball(EdlDemoState* state)
{
    if (state == NULL || state->arcball == NULL)
        return;

    dvz_arcball_set(state->arcball, state->arcball_angles);
    dvz_arcball_zoom(state->arcball, state->arcball_zoom);
    dvz_arcball_pan(state->arcball, state->arcball_pan);
}



static void _sync_arcball_controls(EdlDemoState* state)
{
    if (state == NULL || state->arcball == NULL)
        return;

    dvz_arcball_angles(state->arcball, state->arcball_angles);
    DvzArcballState arcball = {0};
    if (dvz_arcball_state(state->arcball, &arcball))
    {
        state->arcball_zoom = arcball.zoom;
        state->arcball_pan[0] = arcball.pan[0];
        state->arcball_pan[1] = arcball.pan[1];
    }
}



static void _print_settings(const EdlDemoState* state)
{
    if (state == NULL)
        return;

    dvz_fprintf(stderr, "technique_edl settings:\n");
    dvz_fprintf(stderr, "dvz_arcball_set(arcball, (vec3){%+.6ff, %+.6ff, %+.6ff});\n",
            state->arcball_angles[0], state->arcball_angles[1], state->arcball_angles[2]);
    dvz_fprintf(stderr, "dvz_arcball_zoom(arcball, %.6ff);\n", state->arcball_zoom);
    dvz_fprintf(stderr, "dvz_arcball_pan(arcball, (vec2){%+.6ff, %+.6ff});\n",
            state->arcball_pan[0], state->arcball_pan[1]);
    dvz_fprintf(stderr, "edl.radius = %.6ff;\n", state->edl.radius);
    dvz_fprintf(stderr, "edl.strength = %.6ff;\n", state->edl.strength);
    dvz_fprintf(stderr, "edl.depth_scale = %.6ff;\n", state->edl.depth_scale);
}




static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel, vec3 angles)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, angles);
    return controller;
}

/**
 * Add one dense depth-rich point cloud to a panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_depth_cloud(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float sizes[POINT_COUNT] = {0};

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float layer = (float)(i % 12u);
        const float row = (float)(i / 12u);
        const float t = POINT_COUNT > 1u ? (float)i / (float)(POINT_COUNT - 1u) : 0.0f;
        const float theta = TAU * (0.19f * row + 0.083333f * layer);
        const float radius = 0.16f + 0.82f * sqrtf(t);
        const float wave = 0.34f * sinf(TAU * (3.0f * t + 0.083333f * layer));

        positions[i][0] = 0.78f * radius * cosf(theta);
        positions[i][1] = 0.68f * radius * sinf(theta);
        positions[i][2] = -0.80f + 1.60f * (layer / 11.0f) + wave;

        DvzColor color = example_graphite_cyan_color(
            layer < 4.0f   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
            : layer < 8.0f ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                            : EXAMPLE_STYLE_COLOR_WARNING);
        color.a = 255u;
        colors[i] = color;
        sizes[i] = 22.0f + 12.0f * (0.5f + 0.5f * sinf(TAU * (4.0f * t)));
    }

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "size", .data = sizes, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;
    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.edge_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    style.edge_color.a = 245u;
    style.stroke_width_px = 2.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the EDL feature scenario.
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

    EdlDemoState* state = (EdlDemoState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

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

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* lit = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || lit == NULL)
        return false;
    state->lit_panel = lit;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(lit);

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    label_style.size_px = EXAMPLE_PANEL_LABEL_LARGE_SIZE;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_LARGE_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_LARGE_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;
    DvzLabelDesc label = dvz_label_desc();
    label.text = "plain depth";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "EDL resolve";
    annotation = dvz_annotation_label(lit, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(lit, 1.0f) == NULL)
        return false;
    state->arcball_angles[0] = +0.616f;
    state->arcball_angles[1] = -0.403f;
    state->arcball_angles[2] = +0.339f;
    state->arcball_zoom = 1.0f;
    DvzController* plain_controller = _bind_arcball(ctx, plain, state->arcball_angles);
    DvzController* lit_controller = _bind_arcball(ctx, lit, state->arcball_angles);
    if (plain_controller == NULL || lit_controller == NULL)
        return false;
    state->arcball = dvz_controller_arcball(plain_controller);
    if (state->arcball == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, lit_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;

    if (!_add_depth_cloud(ctx->scene, plain) || !_add_depth_cloud(ctx->scene, lit))
        return false;

    state->edl = (DvzExampleGuiEdlControls){
        .enabled = true,
        .radius = 6.941f,
        .strength = 112.941f,
        .depth_scale = 1.361f,
    };
    _apply_edl(state);
    return true;
}



static void _edl_gui(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    EdlDemoState* state = (EdlDemoState*)user_data;
    if (gui == NULL || state == NULL)
        return;

    if (dvz_gui_begin(gui, "EDL calibration", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Lighting");
        if (example_gui_edl(gui, &state->edl))
            _apply_edl(state);

        dvz_gui_separator_text(gui, "Arcball");
        _sync_arcball_controls(state);
        bool arcball_changed = false;
        arcball_changed |=
            dvz_gui_slider_float3(gui, "Angles", state->arcball_angles, -3.14159f, +3.14159f);
        arcball_changed |= dvz_gui_slider_float(gui, "Zoom", &state->arcball_zoom, 0.20f, 4.0f);
        arcball_changed |=
            dvz_gui_slider_float2(gui, "Pan", state->arcball_pan, -2.0f, +2.0f);
        if (arcball_changed)
            _apply_arcball(state);

        if (dvz_gui_button(gui, "Reset arcball"))
        {
            state->arcball_angles[0] = +0.616f;
            state->arcball_angles[1] = -0.403f;
            state->arcball_angles[2] = +0.339f;
            state->arcball_zoom = 1.0f;
            state->arcball_pan[0] = 0.0f;
            state->arcball_pan[1] = 0.0f;
            _apply_arcball(state);
        }
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        if (dvz_gui_button(gui, "Print settings"))
        {
            _sync_arcball_controls(state);
            _print_settings(state);
        }
    }
    dvz_gui_end(gui);
}



static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    EdlDemoState* state = (EdlDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return true;
    dvz_view_set_gui_callback(view, _edl_gui, state);
    return true;
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the EDL scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _edl_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_edl",
        .title = "Eye-Dome Lighting",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static bool _cli_wants_live_gui(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        if (strcmp(argv[i], "--live") == 0 || strcmp(argv[i], "--live-record") == 0)
            return true;
    }
    return false;
}



/**
 * Run the Eye-Dome Lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _edl_scenario();
    if (_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
