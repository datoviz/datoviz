/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* edl - interactive point-cloud EDL example.
 *
 * Build:  just example-c techniques/edl
 * Run:    ./build/examples/c/techniques/edl
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define POINT_COUNT 9216u
#define WIDTH       1000u
#define HEIGHT      760u
#define ROTATION_SPEED_RAD_PER_SEC 0.28f
#define CUE_DISTANCE_MIN 0.0f
#define CUE_DISTANCE_MAX 6.0f
#define CUE_DISTANCE_EPS 1e-4f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct EdlExampleState
{
    DvzPanel* panel;
    DvzVisual* visual;
    DvzExampleVisualSpin spin;
    float* sizes;
    uint32_t point_count;
    bool edl_enabled;
    bool depth_cue_enabled;
    bool spin_enabled;
    DvzDepthCueMode depth_cue_mode;
    float radius;
    float strength;
    float depth_scale;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    float depth_cue_background[4];
    float point_size;
} EdlExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic 3D point shell with depth-rich local structure.
 *
 * @param positions point positions
 * @param colors point colors
 * @param sizes point sizes
 * @param count number of points to fill
 */
static void _build_points(
    vec3* positions, DvzColor* colors, float* sizes, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    const uint32_t rings = 96;
    const uint32_t per_ring = 96;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t ring = i / per_ring;
        uint32_t col = i % per_ring;
        float u = (float)ring / (float)(rings - 1);
        float v = -1.0f + 2.0f * u;
        float theta = TAU * (float)col / (float)per_ring + 0.055f * (float)ring;
        float equator = sqrtf(fmaxf(0.0f, 1.0f - v * v));
        float wave = 0.55f + 0.18f * sinf(5.0f * theta + 7.0f * v) +
                     0.12f * cosf(9.0f * theta - 3.0f * v);

        positions[i][0] = wave * equator * cosf(theta);
        positions[i][1] = 0.72f * v;
        positions[i][2] = wave * equator * sinf(theta);

        float hue = 0.5f + 0.5f * sinf(theta + 1.8f * v);
        colors[i] = dvz_color_rgb(
            (uint8_t)(45.0f + 160.0f * hue), (uint8_t)(85.0f + 130.0f * (1.0f - u)),
            (uint8_t)(150.0f + 80.0f * u));
        sizes[i] = 5.5f;
    }
}


/**
 * Apply the retained point-size control to the visual.
 *
 * @param state example state
 */
static void _apply_point_size(EdlExampleState* state)
{
    ANN(state);
    ANN(state->visual);
    ANN(state->sizes);

    for (uint32_t i = 0; i < state->point_count; i++)
        state->sizes[i] = state->point_size;
    if (dvz_visual_set_data(state->visual, "diameter_px", state->sizes, state->point_count) != 0)
        dvz_fprintf(stderr, "failed to update point size\n");
}


/**
 * Apply the retained spin control to the scene animation.
 *
 * @param state example state
 */
static void _apply_spin(EdlExampleState* state)
{
    ANN(state);
    if (state->spin.animation == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin.animation, 0.0);
    else
        dvz_anim_stop(state->spin.animation);
}



/**
 * Apply the retained EDL state to the panel.
 *
 * @param state example state
 */
static void _apply_edl(EdlExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->edl_enabled)
    {
        (void)dvz_panel_set_edl(state->panel, NULL);
        return;
    }

    DvzEdlDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc),
        .radius = state->radius,
        .strength = state->strength,
        .depth_scale = state->depth_scale,
    };
    if (!dvz_panel_set_edl(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_edl() failed\n");
}


/**
 * Return a short label for a depth-cue mode.
 *
 * @param mode depth-cue mode
 * @return display label
 */
static const char* _depth_cue_mode_label(DvzDepthCueMode mode)
{
    switch (mode)
    {
    case DVZ_DEPTH_CUE_FADE_TO_BACKGROUND:
        return "Fade";
    case DVZ_DEPTH_CUE_DESATURATE:
        return "Desaturate";
    case DVZ_DEPTH_CUE_DARKEN:
        return "Darken";
    case DVZ_DEPTH_CUE_NONE:
    default:
        return "None";
    }
}


/**
 * Cycle to the next supported depth-cue mode.
 *
 * @param mode current depth-cue mode
 * @return next depth-cue mode
 */
static DvzDepthCueMode _depth_cue_mode_next(DvzDepthCueMode mode)
{
    switch (mode)
    {
    case DVZ_DEPTH_CUE_FADE_TO_BACKGROUND:
        return DVZ_DEPTH_CUE_DESATURATE;
    case DVZ_DEPTH_CUE_DESATURATE:
        return DVZ_DEPTH_CUE_DARKEN;
    case DVZ_DEPTH_CUE_DARKEN:
    case DVZ_DEPTH_CUE_NONE:
    default:
        return DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    }
}


/**
 * Apply the retained depth-cue state to the point visual.
 *
 * @param state example state
 */
static void _apply_depth_cue(EdlExampleState* state)
{
    ANN(state);
    ANN(state->visual);

    if (!state->depth_cue_enabled)
    {
        if (dvz_visual_set_depth_cue(state->visual, NULL) != 0)
            dvz_fprintf(stderr, "dvz_visual_set_depth_cue(NULL) failed\n");
        return;
    }

    if (state->depth_cue_near < CUE_DISTANCE_MIN)
        state->depth_cue_near = CUE_DISTANCE_MIN;
    if (state->depth_cue_near > CUE_DISTANCE_MAX - CUE_DISTANCE_EPS)
        state->depth_cue_near = CUE_DISTANCE_MAX - CUE_DISTANCE_EPS;
    if (state->depth_cue_far > CUE_DISTANCE_MAX)
        state->depth_cue_far = CUE_DISTANCE_MAX;
    if (state->depth_cue_far <= state->depth_cue_near + CUE_DISTANCE_EPS)
        state->depth_cue_far = state->depth_cue_near + CUE_DISTANCE_EPS;

    DvzDepthCueDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
        .mode = state->depth_cue_mode,
        .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
        .near_depth = state->depth_cue_near,
        .far_depth = state->depth_cue_far,
        .strength = state->depth_cue_strength,
        .background_color = {
            state->depth_cue_background[0],
            state->depth_cue_background[1],
            state->depth_cue_background[2],
            state->depth_cue_background[3],
        },
    };
    if (dvz_visual_set_depth_cue(state->visual, &desc) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_depth_cue() failed\n");
}



/**
 * Reset the example controls to useful point-cloud defaults.
 *
 * @param state example state
 */
static void _reset_edl(EdlExampleState* state)
{
    ANN(state);
    state->edl_enabled = true;
    state->depth_cue_enabled = true;
    state->depth_cue_mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    state->radius = 2.0f;
    state->strength = 70.0f;
    state->depth_scale = 1.0f;
    state->depth_cue_near = 2.4f;
    state->depth_cue_far = 3.8f;
    state->depth_cue_strength = 0.35f;
    state->depth_cue_background[0] = 0.035f;
    state->depth_cue_background[1] = 0.045f;
    state->depth_cue_background[2] = 0.055f;
    state->depth_cue_background[3] = 1.0f;
    state->point_size = 5.5f;
    _apply_edl(state);
    _apply_depth_cue(state);
    _apply_point_size(state);
}



/**
 * Build the live EDL controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _edl_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    EdlExampleState* state = (EdlExampleState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool cue_changed = false;
    bool point_changed = false;
    bool spin_changed = false;
    if (dvz_gui_begin(gui, "Eye-Dome Lighting", NULL, 0))
    {
        point_changed |=
            dvz_gui_slider_float(gui, "Point size", &state->point_size, 1.0f, 24.0f);
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        changed |= dvz_gui_checkbox(gui, "Enable EDL", &state->edl_enabled);
        changed |= dvz_gui_slider_float(gui, "Radius", &state->radius, 1.0f, 8.0f);
        changed |= dvz_gui_slider_float(gui, "Strength", &state->strength, 0.0f, 160.0f);
        changed |= dvz_gui_slider_float(gui, "Depth scale", &state->depth_scale, 0.1f, 8.0f);
        cue_changed |= dvz_gui_checkbox(gui, "Depth cue", &state->depth_cue_enabled);
        char mode_label[64];
        dvz_snprintf(
            mode_label, sizeof(mode_label), "Mode: %s",
            _depth_cue_mode_label(state->depth_cue_mode));
        if (dvz_gui_button(gui, mode_label))
        {
            state->depth_cue_mode = _depth_cue_mode_next(state->depth_cue_mode);
            cue_changed = true;
        }
        cue_changed |=
            dvz_gui_slider_float(
                gui, "Cue near", &state->depth_cue_near, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        cue_changed |=
            dvz_gui_slider_float(
                gui, "Cue far", &state->depth_cue_far, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue strength", &state->depth_cue_strength, 0.0f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_edl(state);
            changed = false;
            cue_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_edl(state);
    if (cue_changed)
        _apply_depth_cue(state);
    if (point_changed)
        _apply_point_size(state);
    if (spin_changed)
        _apply_spin(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3* positions = NULL;
    DvzColor* colors = NULL;
    float* sizes = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 3.0f;
    camera_desc.projection.near_clip = 0.1f;
    camera_desc.projection.far_clip = 100.0f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*positions));
    colors = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(DvzColor));
    sizes = (float*)dvz_calloc(POINT_COUNT, sizeof(float));
    EXAMPLE_CHECK(positions != NULL && colors != NULL && sizes != NULL, "point allocation failed");
    _build_points(positions, colors, sizes, POINT_COUNT);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = sizes, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.035f, 0.045f, 0.055f, 1.0f));

    EdlExampleState gui_state = {
        .panel = panel,
        .visual = visual,
        .sizes = sizes,
        .point_count = POINT_COUNT,
    };
    _reset_edl(&gui_state);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "edl");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.45f, -0.12f, +0.25f});

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    gui_state.spin.rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(gui_state.spin.rotation != NULL, "dvz_track_rotation() failed");
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = gui_state.spin.rotation;
    gui_state.spin.animation = dvz_anim_visual_transform(scene, visual, &transform_desc);
    EXAMPLE_CHECK(gui_state.spin.animation != NULL, "dvz_anim_visual_transform() failed");
    dvz_anim_set_speed(gui_state.spin.animation, ROTATION_SPEED_RAD_PER_SEC);
    gui_state.spin_enabled = true;
    _apply_spin(&gui_state);

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _edl_gui, &gui_state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_track_destroy(gui_state.spin.rotation);
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
