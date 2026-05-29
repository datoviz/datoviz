/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_probe - polished image probe with colorbar and pinned readout.
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

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ImageProbeState ImageProbeState;

struct ImageProbeState
{
    DvzScene* scene;
    DvzPanel* panel;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    double last_rgba[4];
    bool last_hit;
    bool has_last_result;
    bool pin_next_result;
    uint32_t pinned_count;
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
 * @param out output RGBA8 color
 */
static void _colormap(float value, uint8_t out[4])
{
    ANN(out);
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

    out[0] = _u8(r);
    out[1] = _u8(g);
    out[2] = _u8(b);
    out[3] = 255u;
}



/**
 * Fill the probe image texture with a deterministic scalar field rendered as RGBA.
 *
 * @param pixels output RGBA8 texture
 */
static void _fill_probe_texture(uint8_t pixels[FIELD_WIDTH * FIELD_HEIGHT * 4])
{
    ANN(pixels);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            const float sample = _sample_field(u, v);
            const uint32_t k = 4u * (y * FIELD_WIDTH + x);
            _colormap(sample, &pixels[k]);
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
 * Add the image visual and enable pixel-query readback.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param pixels RGBA8 texture
 * @return true when the image was added
 */
static bool _add_probe_image(
    DvzScene* scene, DvzPanel* panel, uint8_t pixels[FIELD_WIDTH * FIELD_HEIGHT * 4])
{
    ANN(scene);
    ANN(panel);
    ANN(pixels);

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
    if (dvz_visual_set_texture(image, pixels, FIELD_WIDTH, FIELD_HEIGHT) != 0)
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/**
 * Fill data-space crosshair and ring segments around the deterministic probe point.
 *
 * @param starts output segment starts
 * @param ends output segment ends
 * @param colors output segment colors
 * @param widths output segment widths
 */
static void
_fill_probe_marker(vec3 starts[PROBE_SEGMENTS], vec3 ends[PROBE_SEGMENTS],
                   DvzColor colors[PROBE_SEGMENTS], float widths[PROBE_SEGMENTS])
{
    ANN(starts);
    ANN(ends);
    ANN(colors);
    ANN(widths);

    const DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    const DvzColor amber = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    const float x = PROBE_X;
    const float y = PROBE_Y;
    const float gap = 0.028f;
    const float arm = 0.105f;
    const vec3 cross_starts[4] = {
        {x - arm, y, 0.02f},
        {x + gap, y, 0.02f},
        {x, y - arm, 0.02f},
        {x, y + gap, 0.02f},
    };
    const vec3 cross_ends[4] = {
        {x - gap, y, 0.02f},
        {x + arm, y, 0.02f},
        {x, y - gap, 0.02f},
        {x, y + arm, 0.02f},
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
        widths[i] = 2.8f;
    }

    const float rx = 0.043f;
    const float ry = 0.043f * ((float)WIDTH / (float)HEIGHT);
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
        colors[k] = i % 2u == 0u ? cyan : amber;
        colors[k].a = 230u;
        widths[k] = 2.4f;
    }
}



/**
 * Add the visible deterministic probe marker.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the marker was added
 */
static bool _add_probe_marker(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 starts[PROBE_SEGMENTS] = {{0}};
    vec3 ends[PROBE_SEGMENTS] = {{0}};
    vec3 visual_starts[PROBE_SEGMENTS] = {{0}};
    vec3 visual_ends[PROBE_SEGMENTS] = {{0}};
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    _fill_probe_marker(starts, ends, colors, widths);

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
    return dvz_panel_add_visual(panel, marker, NULL) == 0;
}



/**
 * Create the shared color scale and matching colorbar.
 *
 * @param scene scene owning scale resources
 * @param panel panel receiving the colorbar
 * @return created colorbar, or NULL on failure
 */
static DvzColorbar* _add_probe_colorbar(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "intensity",
                   .format = {.precision = 2, .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_view_range(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return NULL;
    DvzColormapStop stops[4] = {
        {.position = 0.00, .rgba = {18, 22, 30, 255}},
        {.position = 0.42, .rgba = {35, 142, 180, 255}},
        {.position = 0.72, .rgba = {76, 201, 240, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 4);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "intensity",
            .reserve_px = 122.0f,
            .ramp_width_px = 26.0f,
            .plot_gap_px = 18.0f,
            .tick_length_px = 6.0f,
            .label_gap_px = 6.0f,
        });
    if (colorbar != NULL)
        dvz_colorbar_set_format(
            colorbar, &(DvzFormatDesc){.precision = 2, .trim_trailing_zeros = true});
    return colorbar;
}



/**
 * Return whether a query result differs enough from the last printed value.
 *
 * @param state image query example state
 * @param query query result to compare
 * @return true when the result should be printed
 */
static bool _query_changed(const ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;
    if (!query->hit)
        return false;

    for (uint32_t i = 0; i < 4u; i++)
    {
        double delta = query->vector[i] - state->last_rgba[i];
        if (delta < 0.0)
            delta = -delta;
        if (delta >= (1.0 / 255.0))
            return true;
    }
    return false;
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
    for (uint32_t i = 0; i < 4u; i++)
        state->last_rgba[i] = query->vector[i];
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
 * Record the latest cursor position and arm click-to-pin behavior.
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
    if (event->type == DVZ_POINTER_EVENT_CLICK)
        state->pin_next_result = true;
}



/**
 * Poll image query results, pin requested readouts, and queue the next probe.
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
        if (state->pin_next_result)
        {
            if (query.status == DVZ_QUERY_STATUS_HIT && query.hit)
            {
                DvzPinnedReadout* readout = dvz_pinned_readout_query(state->panel, &query);
                if (readout != NULL)
                {
                    dvz_pinned_readout_set_format(
                        readout, &(DvzFormatDesc){.precision = 3, .trim_trailing_zeros = true});
                    state->pinned_count++;
                    dvz_fprintf(stdout, "pinned image readout %u\n", state->pinned_count);
                }
                else
                {
                    dvz_fprintf(stderr, "dvz_pinned_readout_query() failed\n");
                }
            }
            state->pin_next_result = false;
        }

        if (!_query_changed(state, &query))
            continue;

        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.value_kind == DVZ_QUERY_VALUE_VEC4)
        {
            dvz_fprintf(
                stdout,
                "probe rgba=(%0.3f, %0.3f, %0.3f, %0.3f) panel=(%0.1f,%0.1f)\n",
                query.vector[0], query.vector[1], query.vector[2], query.vector[3],
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
        panel, &(DvzPanelLayoutReserve){.left = 0.045f, .right = 0.155f, .bottom = 0.055f,
                                        .top = 0.045f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");
    ok = _set_probe_domain(panel);
    EXAMPLE_CHECK(ok, "setting image probe domain failed");

    example_graphite_cyan_set_panel_background(panel);

    uint8_t pixels[FIELD_WIDTH * FIELD_HEIGHT * 4] = {0};
    _fill_probe_texture(pixels);

    ok = _add_probe_image(scene, panel, pixels);
    EXAMPLE_CHECK(ok, "adding probe image failed");

    ok = _add_probe_marker(scene, panel);
    EXAMPLE_CHECK(ok, "adding probe marker failed");

    DvzColorbar* colorbar = _add_probe_colorbar(scene, panel);
    EXAMPLE_CHECK(colorbar != NULL, "adding probe colorbar failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "image_probe");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    ImageProbeState state = {
        .scene = scene,
        .panel = panel,
        .cursor_valid = true,
        .cursor_x = (double)WIDTH * (double)PROBE_X,
        .cursor_y = (double)HEIGHT * (1.0 - (double)PROBE_Y),
        .pin_next_result = true,
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
