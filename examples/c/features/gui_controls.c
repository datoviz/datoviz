/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_controls - curated Datoviz GUI controls mutating one retained point visual.
 *
 * Scenario: feature.gui_controls
 * Style: features, native GUI/app
 *
 * Build:  just example-c features/gui_controls
 * Run:    ./build/examples/c/features/gui_controls
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1000u
#define HEIGHT      700u
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiControlsState
{
    DvzVisual* point;
    float diameter;
    float color[4];
    bool visible;
    bool pulse;
    int palette;
    int glyph_count;
    float opacity;
    float jitter[2];
    float contrast[4];
    bool bloom_enabled;
    float bloom_radius;
    float bloom_threshold;
    bool contour_enabled;
    float contour_width;
    float contour_range[2];
    bool diagnostic_overlay;
    bool show_histogram;
    float clip_min[3];
    float clip_max[3];
    float light_direction[3];
} GuiControlsState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload the point colors and sizes controlled by the GUI.
 *
 * @param state GUI controls example state
 * @return true on success
 */
static bool _gui_controls_upload(GuiControlsState* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i].r = (uint8_t)(255.0f * state->color[0]);
        colors[i].g = (uint8_t)(255.0f * state->color[1]);
        colors[i].b = (uint8_t)(255.0f * state->color[2]);
        colors[i].a = (uint8_t)(255.0f * state->opacity);
        diameters[i] = state->diameter;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 2) == 0;
}



/**
 * Build the Datoviz GUI controls for one retained visual.
 *
 * @param gui GUI overlay
 * @param view app view
 * @param user_data GUI controls example state
 */
static void _gui_controls_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    GuiControlsState* state = (GuiControlsState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool visible_changed = false;
    if (dvz_gui_begin(gui, "Widget controls", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Marker");
        changed |=
            dvz_gui_slider_float(gui, "Diameter##gui_controls_marker_diameter", &state->diameter,
                                 8.0f, 96.0f);
        changed |= dvz_gui_color_edit4(gui, "Tint##gui_controls_marker_tint", state->color, 0);
        changed |=
            dvz_gui_slider_float(gui, "Alpha##gui_controls_marker_alpha", &state->opacity, 0.15f,
                                 1.0f);
        visible_changed |=
            dvz_gui_checkbox(gui, "Visible##gui_controls_marker_visible", &state->visible);
        (void)dvz_gui_checkbox(gui, "Pulse preview##gui_controls_marker_pulse", &state->pulse);

        dvz_gui_separator_text(gui, "Synthetic data");
        static const char* const palette_items[] = {"Cyan", "Amber", "Violet", "Slate"};
        (void)dvz_gui_combo(
            gui, "Palette##gui_controls_data_palette", &state->palette, palette_items, 4);
        (void)dvz_gui_slider_int(
            gui, "Sample count##gui_controls_data_sample_count", &state->glyph_count, 16, 256);
        (void)dvz_gui_slider_float2(
            gui, "Jitter XY##gui_controls_data_jitter_xy", state->jitter, -1.0f, 1.0f);
        (void)dvz_gui_slider_float4(
            gui, "Contrast curve##gui_controls_data_contrast_curve", state->contrast, 0.0f, 1.0f);

        if (dvz_gui_collapsing_header(gui, "Mock effects##gui_controls_effects_section", 0))
        {
            (void)dvz_gui_checkbox(
                gui, "Bloom enabled##gui_controls_effects_bloom_enabled",
                &state->bloom_enabled);
            (void)dvz_gui_slider_float_format(
                gui, "Bloom radius##gui_controls_effects_bloom_radius", &state->bloom_radius,
                0.5f, 12.0f, "%.1f px");
            (void)dvz_gui_slider_float(
                gui, "Bloom threshold##gui_controls_effects_bloom_threshold",
                &state->bloom_threshold, 0.0f, 1.0f);
            (void)dvz_gui_checkbox(
                gui, "Contours enabled##gui_controls_effects_contours_enabled",
                &state->contour_enabled);
            (void)dvz_gui_slider_float(
                gui, "Contour width##gui_controls_effects_contour_width",
                &state->contour_width, 0.25f, 5.0f);
            (void)dvz_gui_slider_range_float(
                gui, "Contour range##gui_controls_effects_contour_range",
                &state->contour_range[0], &state->contour_range[1], 0.0f, 1.0f, "%.2f");
        }

        if (dvz_gui_collapsing_header(gui, "Mock volume##gui_controls_volume_section", 0))
        {
            (void)dvz_gui_slider_float3(
                gui, "Light vector##gui_controls_volume_light_vector", state->light_direction,
                -1.0f, 1.0f);
            (void)dvz_gui_range_float(
                gui, "Clip X##gui_controls_volume_clip_x", &state->clip_min[0],
                &state->clip_max[0], 0.01f, 0.0f, 1.0f, "%.2f");
            (void)dvz_gui_range_float(
                gui, "Clip Y##gui_controls_volume_clip_y", &state->clip_min[1],
                &state->clip_max[1], 0.01f, 0.0f, 1.0f, "%.2f");
            (void)dvz_gui_range_float(
                gui, "Clip Z##gui_controls_volume_clip_z", &state->clip_min[2],
                &state->clip_max[2], 0.01f, 0.0f, 1.0f, "%.2f");
        }

        dvz_gui_separator_text(gui, "Diagnostics");
        (void)dvz_gui_checkbox(
            gui, "Overlay##gui_controls_diagnostics_overlay", &state->diagnostic_overlay);
        dvz_gui_same_line(gui, 0.0f, -1.0f);
        (void)dvz_gui_checkbox(
            gui, "Histogram##gui_controls_diagnostics_histogram", &state->show_histogram);
        if (dvz_gui_button(gui, "Reset mock values##gui_controls_diagnostics_reset"))
        {
            state->bloom_radius = 3.0f;
            state->bloom_threshold = 0.62f;
            state->contour_width = 1.4f;
            state->contour_range[0] = 0.18f;
            state->contour_range[1] = 0.82f;
            state->clip_min[0] = state->clip_min[1] = state->clip_min[2] = 0.05f;
            state->clip_max[0] = state->clip_max[1] = state->clip_max[2] = 0.95f;
        }
    }
    dvz_gui_end(gui);

    if (changed && !_gui_controls_upload(state))
        dvz_fprintf(stderr, "gui_controls: failed to upload visual data\n");
    if (visible_changed)
        dvz_visual_set_visible(state->point, state->visible);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(
        figure != NULL && panel != NULL && point != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    vec3 positions[POINT_COUNT] = {
        {-0.70f, -0.35f, 0.0f}, {-0.35f, +0.20f, 0.0f}, {+0.00f, -0.10f, 0.0f},
        {+0.35f, +0.35f, 0.0f}, {+0.70f, -0.20f, 0.0f},
    };
    GuiControlsState state = {
        .point = point,
        .diameter = 42.0f,
        .color = {0.28f, 0.78f, 1.00f, 1.00f},
        .visible = true,
        .pulse = true,
        .palette = 0,
        .glyph_count = 96,
        .opacity = 1.0f,
        .jitter = {0.12f, -0.08f},
        .contrast = {0.08f, 0.32f, 0.72f, 0.94f},
        .bloom_enabled = true,
        .bloom_radius = 3.0f,
        .bloom_threshold = 0.62f,
        .contour_enabled = false,
        .contour_width = 1.4f,
        .contour_range = {0.18f, 0.82f},
        .diagnostic_overlay = false,
        .show_histogram = true,
        .clip_min = {0.05f, 0.05f, 0.05f},
        .clip_max = {0.95f, 0.95f, 0.95f},
        .light_direction = {-0.35f, +0.55f, 0.75f},
    };
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(point, updates, 1);
    EXAMPLE_CHECK(rc == 0 && _gui_controls_upload(&state), "failed to upload point data");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    EXAMPLE_CHECK(dvz_point_set_style(point, &style) == 0, "dvz_point_set_style() failed");
    EXAMPLE_CHECK(
        dvz_visual_set_depth_test(point, false) == 0, "dvz_visual_set_depth_test() failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* view = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "gui_controls");
    EXAMPLE_CHECK(view != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(view, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(view, _gui_controls_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
