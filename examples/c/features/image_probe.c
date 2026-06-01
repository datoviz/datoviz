/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_probe - polished image probe with colorbar and live readout.
 *
 * Scenario: image_probe
 * Style: features, graphite_cyan, 1280x960 capture target
 *
 * Build:  just example-c features/image_probe
 * Run:    ./build/examples/c/features/image_probe
 * Smoke:  ./build/examples/c/features/image_probe 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH               1280u
#define HEIGHT              960u
#define FIELD_WIDTH         256u
#define FIELD_HEIGHT        192u
#define PROBE_X             0.68f
#define PROBE_Y             0.56f
#define PROBE_REQUEST_ID    1u
#define PROBE_RING_SEGMENTS 28u
#define PROBE_SEGMENTS      (PROBE_RING_SEGMENTS + 4u)
#define PROBE_CARD_TEXT     "value --"
#define COLORMAP_LUT_SIZE   256u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ImageProbeState ImageProbeState;

struct ImageProbeState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzVisual* probe_segments;
    DvzVisual* probe_dot;
    DvzOverlayCard* probe_card;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    double last_value;
    bool last_hit;
    bool has_last_result;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a float to the unit interval.
 *
 * @param value input value
 * @return clamped value
 */
static float _clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}



/**
 * Convert a normalized float channel to an 8-bit channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel
 */
static uint8_t _u8(float value)
{
    return (uint8_t)(255.0f * _clamp01(value) + 0.5f);
}



/**
 * Return a deterministic synthetic microscopy-like scalar sample.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return normalized sample value
 */
static float _sample_field(float x, float y)
{
    float value = 0.11f + 0.05f * sinf(TAU * (2.3f * x + 0.35f * y));
    value += 0.04f * cosf(TAU * (0.55f * x - 3.6f * y));

    const float filament = sinf(TAU * (x * 1.15f + 0.22f * sinf(TAU * y)));
    value += 0.18f * expf(-18.0f * (filament - 0.18f) * (filament - 0.18f));

    const float centers[8][3] = {
        {0.16f, 0.22f, 0.050f}, {0.31f, 0.71f, 0.042f}, {0.46f, 0.38f, 0.035f},
        {0.58f, 0.84f, 0.040f}, {0.70f, 0.56f, 0.038f}, {0.78f, 0.24f, 0.046f},
        {0.86f, 0.69f, 0.035f}, {0.24f, 0.50f, 0.030f},
    };
    for (uint32_t i = 0; i < 8u; i++)
    {
        const float dx = x - centers[i][0];
        const float dy = y - centers[i][1];
        const float sigma = centers[i][2];
        const float d2 = (dx * dx + 1.4f * dy * dy) / (2.0f * sigma * sigma);
        value += (0.20f + 0.06f * (float)(i % 3u)) * expf(-d2);
    }

    const float hot_dx = x - 0.69f;
    value +=
        0.30f * expf(-(hot_dx * hot_dx) / (2.0f * 0.115f * 0.115f)) *
        (0.78f + 0.22f * cosf(TAU * (y - 0.48f)));

    const float hot_dy = y - 0.57f;
    value += 0.50f * expf(-(hot_dx * hot_dx + hot_dy * hot_dy) / (2.0f * 0.022f * 0.022f));

    const float mirror_dy = y - 0.43f;
    value += 0.42f * expf(-(hot_dx * hot_dx + mirror_dy * mirror_dy) / (2.0f * 0.024f * 0.024f));

    return _clamp01(value);
}



/**
 * Map one scalar sample to the graphite/cyan/amber release palette.
 *
 * @param value normalized scalar value
 * @return output RGBA8 color
 */
static DvzColor _probe_colormap(float value)
{
    const float v = _clamp01(value);
    float r = 0.07f;
    float g = 0.10f;
    float b = 0.14f;

    if (v < 0.58f)
    {
        const float t = v / 0.58f;
        r = 0.07f + 0.18f * t;
        g = 0.10f + 0.56f * t;
        b = 0.14f + 0.66f * t;
    }
    else
    {
        const float t = (v - 0.58f) / 0.42f;
        r = 0.25f + 0.75f * t;
        g = 0.66f + 0.06f * t;
        b = 0.80f - 0.78f * t;
    }

    return dvz_color_rgba(_u8(r), _u8(g), _u8(b), 255u);
}



/**
 * Fill the custom colormap LUT.
 *
 * @param colors output RGBA8 LUT
 */
static void _fill_probe_colormap(DvzColor colors[COLORMAP_LUT_SIZE])
{
    ANN(colors);

    for (uint32_t i = 0; i < COLORMAP_LUT_SIZE; i++)
    {
        const float t =
            COLORMAP_LUT_SIZE > 1u ? (float)i / (float)(COLORMAP_LUT_SIZE - 1u) : 0.0f;
        colors[i] = _probe_colormap(t);
    }
}



/**
 * Fill the probe image scalar field.
 *
 * @param values output normalized scalar field
 */
static void _fill_probe_field(float values[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(values);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            values[y * FIELD_WIDTH + x] = _sample_field(u, v);
        }
    }
}



/**
 * Set a normalized data domain on the image panel.
 *
 * @param panel target panel
 * @return true when both domain calls succeed
 */
static bool _set_probe_domain(DvzPanel* panel)
{
    ANN(panel);
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0);
    return rc == 0;
}



/**
 * Add the scalar image visual and enable pixel-query readback.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param scale color scale bound to the scalar image
 * @param values scalar field values
 * @return true when the image was added
 */
static bool _add_probe_image(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, float values[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(values);

    vec3 data_positions[4] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec3 visual_positions[4] = {{0}};
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, 4);
    if (rc != 0)
        return false;

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", visual_positions, 4) != 0)
        return false;
    if (dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0)
        return false;
    if (dvz_visual_set_scale(image, "colormap", scale) != 0)
        return false;

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = values,
                       .bytes_per_row = FIELD_WIDTH * sizeof(float),
                       .rows_per_image = FIELD_HEIGHT,
                   }))
    {
        return false;
    }
    if (!dvz_visual_set_field(image, "field", field))
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/**
 * Convert a panel-local probe position to the normalized image data domain.
 *
 * @param panel target panel
 * @param panel_x panel-local X coordinate in logical pixels
 * @param panel_y panel-local Y coordinate in logical pixels
 * @param out_x output normalized data X
 * @param out_y output normalized data Y
 * @return true when the position is inside the plot rectangle
 */
static bool
_probe_panel_to_data(DvzPanel* panel, double panel_x, double panel_y, float* out_x, float* out_y)
{
    if (panel == NULL || out_x == NULL || out_y == NULL)
        return false;

    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
        return false;

    const double x = (panel_x - (double)plot.x) / (double)plot.width;
    const double y = 1.0 - (panel_y - (double)plot.y) / (double)plot.height;
    if (x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0)
        return false;

    *out_x = (float)x;
    *out_y = (float)y;
    return true;
}



/**
 * Convert a normalized image data position to a panel-local probe position.
 *
 * @param panel target panel
 * @param x normalized data X
 * @param y normalized data Y
 * @param out_panel output panel-local logical pixels
 * @return true when the position was converted
 */
static bool _probe_data_to_panel(DvzPanel* panel, float x, float y, float out_panel[2])
{
    if (panel == NULL || out_panel == NULL)
        return false;

    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
        return false;

    out_panel[0] = plot.x + x * plot.width;
    out_panel[1] = plot.y + (1.0f - y) * plot.height;
    return true;
}



/**
 * Fill data-space crosshair and ring segments around a probe point.
 *
 * @param panel target panel
 * @param x normalized probe X coordinate
 * @param y normalized probe Y coordinate
 * @param starts output segment starts
 * @param ends output segment ends
 * @param colors output segment colors
 * @param widths output segment widths
 * @return true when marker geometry was filled
 */
static bool _fill_probe_marker(
    DvzPanel* panel, float x, float y, vec3 starts[PROBE_SEGMENTS], vec3 ends[PROBE_SEGMENTS],
    DvzColor colors[PROBE_SEGMENTS], float widths[PROBE_SEGMENTS])
{
    ANN(panel);
    ANN(starts);
    ANN(ends);
    ANN(colors);
    ANN(widths);

    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
        return false;

    const DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    const float gap_x = 6.0f / plot.width;
    const float gap_y = 6.0f / plot.height;
    const float arm_x = 20.0f / plot.width;
    const float arm_y = 20.0f / plot.height;
    const vec3 cross_starts[4] = {
        {x - arm_x, y, 0.02f},
        {x + gap_x, y, 0.02f},
        {x, y - arm_y, 0.02f},
        {x, y + gap_y, 0.02f},
    };
    const vec3 cross_ends[4] = {
        {x - gap_x, y, 0.02f},
        {x + arm_x, y, 0.02f},
        {x, y - gap_y, 0.02f},
        {x, y + arm_y, 0.02f},
    };

    for (uint32_t i = 0; i < 4u; i++)
    {
        starts[i][0] = cross_starts[i][0];
        starts[i][1] = cross_starts[i][1];
        starts[i][2] = cross_starts[i][2];
        ends[i][0] = cross_ends[i][0];
        ends[i][1] = cross_ends[i][1];
        ends[i][2] = cross_ends[i][2];
        colors[i] = cyan;
        colors[i].a = 245u;
        widths[i] = 1.8f;
    }

    const float rx = 12.0f / plot.width;
    const float ry = 12.0f / plot.height;
    for (uint32_t i = 0; i < PROBE_RING_SEGMENTS; i++)
    {
        const uint32_t k = i + 4u;
        const float a0 = TAU * (float)i / (float)PROBE_RING_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)PROBE_RING_SEGMENTS;
        starts[k][0] = x + rx * cosf(a0);
        starts[k][1] = y + ry * sinf(a0);
        starts[k][2] = 0.02f;
        ends[k][0] = x + rx * cosf(a1);
        ends[k][1] = y + ry * sinf(a1);
        ends[k][2] = 0.02f;
        colors[k] = cyan;
        colors[k].a = 225u;
        widths[k] = 1.7f;
    }
    return true;
}



/**
 * Update the live probe marker to a normalized data position.
 *
 * @param state image probe example state
 * @param x normalized probe X coordinate
 * @param y normalized probe Y coordinate
 */
static void _update_probe_marker(ImageProbeState* state, float x, float y)
{
    if (state == NULL || state->panel == NULL || state->probe_segments == NULL ||
        state->probe_dot == NULL)
        return;

    DvzPanel* panel = state->panel;
    ANN(panel);

    vec3 starts[PROBE_SEGMENTS] = {{0}};
    vec3 ends[PROBE_SEGMENTS] = {{0}};
    vec3 visual_starts[PROBE_SEGMENTS] = {{0}};
    vec3 visual_ends[PROBE_SEGMENTS] = {{0}};
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    if (!_fill_probe_marker(panel, x, y, starts, ends, colors, widths))
        return;

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)starts, (float*)visual_starts, PROBE_SEGMENTS);
    if (rc != 0)
        return;
    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)ends, (float*)visual_ends, PROBE_SEGMENTS);
    if (rc != 0)
        return;

    DvzVisualDataUpdate segment_updates[] = {
        {.attr_name = "position_start", .data = visual_starts, .item_count = PROBE_SEGMENTS},
        {.attr_name = "position_end", .data = visual_ends, .item_count = PROBE_SEGMENTS},
    };
    if (dvz_visual_set_data_many(state->probe_segments, segment_updates, 2) != 0)
        return;

    vec3 dot_data[1] = {{x, y, 0.03f}};
    vec3 dot_visual[1] = {{0}};
    rc = dvz_panel_data_to_visual_positions(panel, (const float*)dot_data, (float*)dot_visual, 1);
    if (rc == 0)
        (void)dvz_visual_set_data(state->probe_dot, "position", dot_visual, 1);
}



/**
 * Update the live probe marker from a panel-local cursor position.
 *
 * @param state image probe example state
 */
static void _update_probe_marker_from_cursor(ImageProbeState* state)
{
    if (state == NULL || !state->cursor_valid)
        return;

    float x = 0.0f;
    float y = 0.0f;
    if (!_probe_panel_to_data(state->panel, state->cursor_x, state->cursor_y, &x, &y))
        return;
    _update_probe_marker(state, x, y);
}



/**
 * Add the visible live probe marker.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_segments output segment visual
 * @param out_dot output center dot visual
 * @return true when the marker was added
 */
static bool _add_probe_marker(
    DvzScene* scene, DvzPanel* panel, DvzVisual** out_segments, DvzVisual** out_dot)
{
    ANN(scene);
    ANN(panel);
    ANN(out_segments);
    ANN(out_dot);

    vec3 starts[PROBE_SEGMENTS] = {{0}};
    vec3 ends[PROBE_SEGMENTS] = {{0}};
    vec3 visual_starts[PROBE_SEGMENTS] = {{0}};
    vec3 visual_ends[PROBE_SEGMENTS] = {{0}};
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    if (!_fill_probe_marker(panel, PROBE_X, PROBE_Y, starts, ends, colors, widths))
        return false;

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)starts, (float*)visual_starts, PROBE_SEGMENTS);
    if (rc != 0)
        return false;
    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)ends, (float*)visual_ends, PROBE_SEGMENTS);
    if (rc != 0)
        return false;

    DvzVisual* marker = dvz_segment(scene, 0);
    if (marker == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = visual_starts, .item_count = PROBE_SEGMENTS},
        {.attr_name = "position_end", .data = visual_ends, .item_count = PROBE_SEGMENTS},
        {.attr_name = "color", .data = colors, .item_count = PROBE_SEGMENTS},
        {.attr_name = "stroke_width", .data = widths, .item_count = PROBE_SEGMENTS},
    };
    if (dvz_visual_set_data_many(marker, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(marker, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_visual_set_depth_test(marker, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, marker, NULL) != 0)
        return false;

    vec3 dot_data[1] = {{PROBE_X, PROBE_Y, 0.03f}};
    vec3 dot_visual[1] = {{0}};
    rc = dvz_panel_data_to_visual_positions(panel, (const float*)dot_data, (float*)dot_visual, 1);
    if (rc != 0)
        return false;

    DvzVisual* dot = dvz_point(scene, 0);
    if (dot == NULL)
        return false;
    DvzColor dot_color[1] = {example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)};
    dot_color[0].a = 245u;
    float dot_diameter[1] = {6.0f};
    DvzVisualDataUpdate dot_updates[] = {
        {.attr_name = "position", .data = dot_visual, .item_count = 1},
        {.attr_name = "color", .data = dot_color, .item_count = 1},
        {.attr_name = "diameter", .data = dot_diameter, .item_count = 1},
    };
    if (dvz_visual_set_data_many(dot, dot_updates, 3) != 0)
        return false;
    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    point_style.stroke_width = 0.0f;
    if (dvz_point_set_style(dot, &point_style) != 0)
        return false;
    if (dvz_visual_set_depth_test(dot, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, dot, NULL) != 0)
        return false;

    *out_segments = marker;
    *out_dot = dot;
    return true;
}



/**
 * Create the shared scalar color scale.
 *
 * @param scene scene owning scale resources
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_probe_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "intensity",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 2, .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_view_range(scale, 0.0, 1.0);

    DvzColor colors[COLORMAP_LUT_SIZE] = {0};
    _fill_probe_colormap(colors);
    DvzColormap* colormap =
        dvz_colormap_custom(scene, "graphite_cyan_amber", colors, COLORMAP_LUT_SIZE);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add the colorbar for the shared scale.
 *
 * @param panel panel receiving the colorbar
 * @param scale scale bound to the colorbar
 * @return created colorbar, or NULL on failure
 */
static DvzColorbar* _add_probe_colorbar(DvzPanel* panel, DvzScale* scale)
{
    ANN(panel);
    ANN(scale);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "intensity",
            .reserve_px = 96.0f,
            .ramp_width_px = 26.0f,
            .plot_gap_px = 12.0f,
            .tick_length_px = 6.0f,
            .label_gap_px = 6.0f,
        });
    if (colorbar != NULL)
        dvz_colorbar_set_format(
            colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 2, .trim_trailing_zeros = true});
    return colorbar;
}



/**
 * Create the compact live probe readout card.
 *
 * @param panel panel receiving the overlay
 * @return created overlay card, or NULL on failure
 */
static DvzOverlayCard* _add_probe_card(DvzPanel* panel)
{
    ANN(panel);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return NULL;

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.background_color = dvz_color_rgba(panel_bg.r, panel_bg.g, panel_bg.b, 232);
    style.text_color = text;
    style.padding_px[0] = 10.0f;
    style.padding_px[1] = 6.0f;
    style.min_width_px = 104.0f;
    style.height_px = 28.0f;
    style.glyph_advance_px = 7.0f;
    style.text_size_px = 13.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = 56u;

    float anchor[2] = {(float)WIDTH * PROBE_X, (float)HEIGHT * (1.0f - PROBE_Y)};
    (void)_probe_data_to_panel(panel, PROBE_X, PROBE_Y, anchor);

    return dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){
            .text = PROBE_CARD_TEXT,
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_PIXEL,
            .anchor_px = {anchor[0], anchor[1]},
            .offset_px = {14.0f, 14.0f},
            .style = &style,
        });
}


/**
 * Resolve one scalar probe value from a query result.
 *
 * @param state image probe example state
 * @param query query result
 * @param out_value output scalar value
 * @return true when a scalar value was resolved
 */
static bool
_query_probe_value(ImageProbeState* state, const DvzQueryResult* query, double* out_value)
{
    if (state == NULL || query == NULL || out_value == NULL)
        return false;
    if (query->status != DVZ_QUERY_STATUS_HIT || !query->hit)
        return false;

    float x = 0.0f;
    float y = 0.0f;
    if (!_probe_panel_to_data(
            state->panel, query->panel_position[0], query->panel_position[1], &x, &y))
    {
        return false;
    }
    *out_value = (double)_sample_field(x, y);
    return true;
}



/**
 * Update the compact live probe readout card from one query result.
 *
 * @param state image query example state
 * @param query query result to display
 */
static void _update_probe_card(ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || state->probe_card == NULL || query == NULL)
        return;

    float anchor[2] = {(float)query->panel_position[0], (float)query->panel_position[1]};
    float offset[2] = {14.0f, 14.0f};
    dvz_overlay_card_set_layout(state->probe_card, anchor, offset);

    char text[128] = {0};
    double value = 0.0;
    if (_query_probe_value(state, query, &value))
    {
        int n = dvz_snprintf(text, sizeof(text), "value %.3f", value);
        if (n <= 0 || (size_t)n >= sizeof(text))
            return;
    }
    else
    {
        int n = dvz_snprintf(text, sizeof(text), "value --");
        if (n <= 0 || (size_t)n >= sizeof(text))
            return;
    }
    dvz_overlay_card_set_text(state->probe_card, text);
}



/**
 * Return whether a query result differs enough from the last printed value.
 *
 * @param state image query example state
 * @param query query result to compare
 * @return true when the result should be printed
 */
static bool _query_changed(ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;
    if (!query->hit)
        return false;

    double value = 0.0;
    if (!_query_probe_value(state, query, &value))
        return true;
    double delta = value - state->last_value;
    if (delta < 0.0)
        delta = -delta;
    return delta >= 1e-3;
}



/**
 * Remember the last printed query result.
 *
 * @param state image query example state
 * @param query query result to store
 */
static void _store_query_result(ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;

    state->has_last_result = true;
    state->last_hit = query->hit;
    double value = 0.0;
    if (_query_probe_value(state, query, &value))
        state->last_value = value;
}



/**
 * Queue one pixel query at the latest probe position.
 *
 * @param state image query example state
 */
static void _queue_probe(ImageProbeState* state)
{
    if (state == NULL || !state->cursor_valid)
        return;

    const int rc = dvz_panel_query(
        state->panel, state->cursor_x, state->cursor_y,
        &(DvzQueryRequest){
            .request_id = PROBE_REQUEST_ID,
            .target = DVZ_SCENE_TARGET_PIXEL,
        });
    if (rc != 0)
        dvz_fprintf(stderr, "dvz_panel_query() failed\n");
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest cursor position and move the live probe marker.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data image probe example state
 */
static void
_image_probe_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
    _update_probe_marker_from_cursor(state);
    if (state->probe_card != NULL)
    {
        float anchor[2] = {(float)state->cursor_x, (float)state->cursor_y};
        float offset[2] = {14.0f, 14.0f};
        dvz_overlay_card_set_layout(state->probe_card, anchor, offset);
    }
}



/**
 * Poll image query results, update the live readout, and queue the next probe.
 *
 * @param win view whose frame just completed
 * @param user_data image probe example state
 */
static void _image_probe_frame(DvzView* win, void* user_data)
{
    (void)win;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        _update_probe_card(state, &query);

        if (!_query_changed(state, &query))
            continue;

        double value = 0.0;
        if (_query_probe_value(state, &query, &value))
        {
            dvz_fprintf(
                stdout, "probe value=%0.3f panel=(%0.1f,%0.1f)\n", value,
                query.panel_position[0], query.panel_position[1]);
        }
        else
        {
            dvz_fprintf(
                stdout, "probe miss panel=(%0.1f,%0.1f)\n", query.panel_position[0],
                query.panel_position[1]);
        }
        _store_query_result(state, &query);
    }

    _queue_probe(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.045f, .right = 0.030f, .bottom = 0.055f,
                                        .top = 0.045f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");
    ok = _set_probe_domain(panel);
    EXAMPLE_CHECK(ok, "setting image probe domain failed");

    example_graphite_cyan_set_panel_background(panel);

    DvzScale* scale = _add_probe_scale(scene);
    EXAMPLE_CHECK(scale != NULL, "adding probe scale failed");

    DvzColorbar* colorbar = _add_probe_colorbar(panel, scale);
    EXAMPLE_CHECK(colorbar != NULL, "adding probe colorbar failed");

    float values[FIELD_WIDTH * FIELD_HEIGHT] = {0};
    _fill_probe_field(values);

    ok = _add_probe_image(scene, panel, scale, values);
    EXAMPLE_CHECK(ok, "adding probe image failed");

    DvzVisual* probe_segments = NULL;
    DvzVisual* probe_dot = NULL;
    ok = _add_probe_marker(scene, panel, &probe_segments, &probe_dot);
    EXAMPLE_CHECK(ok, "adding probe marker failed");

    DvzOverlayCard* probe_card = _add_probe_card(panel);
    EXAMPLE_CHECK(probe_card != NULL, "adding probe readout card failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "image_probe");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    float initial_probe_px[2] = {(float)WIDTH * PROBE_X, (float)HEIGHT * (1.0f - PROBE_Y)};
    (void)_probe_data_to_panel(panel, PROBE_X, PROBE_Y, initial_probe_px);

    ImageProbeState state = {
        .scene = scene,
        .panel = panel,
        .probe_segments = probe_segments,
        .probe_dot = probe_dot,
        .probe_card = probe_card,
        .cursor_valid = true,
        .cursor_x = initial_probe_px[0],
        .cursor_y = initial_probe_px[1],
    };
    dvz_input_subscribe_pointer(router, _image_probe_pointer, &state);
    dvz_view_set_frame_callback(win, _image_probe_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
