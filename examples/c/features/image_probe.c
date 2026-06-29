/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_probe - scalar image pixel-query proof with a live probe marker.
 *
 * Scenario: image_probe
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/image_probe
 * Run:    ./build/examples/c/features/image_probe --live
 * Smoke:  ./build/examples/c/features/image_probe --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_image_probe_scenario(void);


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH               1600u
#define HEIGHT              1200u
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
    DvzVisual* probe_segments;
    DvzVisual* probe_dot;
    float* values;
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
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", data_positions, 4) != 0)
        return false;
    if (dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0)
        return false;
    if (dvz_visual_set_scale(image, "color", scale) != 0)
        return false;

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
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
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    if (!_fill_probe_marker(panel, x, y, starts, ends, colors, widths))
        return;

    DvzVisualDataUpdate segment_updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = PROBE_SEGMENTS},
        {.attr_name = "position_end", .data = ends, .item_count = PROBE_SEGMENTS},
    };
    if (dvz_visual_set_data_many(state->probe_segments, segment_updates, 2) != 0)
        return;

    vec3 dot_data[1] = {{x, y, 0.03f}};
    (void)dvz_visual_set_data(state->probe_dot, "position", dot_data, 1);
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

    double data[2] = {0};
    if (!dvz_panel_position_to_data(
            state->panel, DVZ_PANEL_COORD_PANEL_PX,
            (const double[2]){state->cursor_x, state->cursor_y}, data))
    {
        return;
    }
    _update_probe_marker(state, (float)data[0], (float)data[1]);
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
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    if (!_fill_probe_marker(panel, PROBE_X, PROBE_Y, starts, ends, colors, widths))
        return false;

    DvzVisual* marker = dvz_segment(scene, 0);
    if (marker == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = PROBE_SEGMENTS},
        {.attr_name = "position_end", .data = ends, .item_count = PROBE_SEGMENTS},
        {.attr_name = "color", .data = colors, .item_count = PROBE_SEGMENTS},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PROBE_SEGMENTS},
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

    DvzVisual* dot = dvz_point(scene, 0);
    if (dot == NULL)
        return false;
    DvzColor dot_color[1] = {example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)};
    dot_color[0].a = 245u;
    float dot_diameter[1] = {6.0f};
    DvzVisualDataUpdate dot_updates[] = {
        {.attr_name = "position", .data = dot_data, .item_count = 1},
        {.attr_name = "color", .data = dot_color, .item_count = 1},
        {.attr_name = "diameter_px", .data = dot_diameter, .item_count = 1},
    };
    if (dvz_visual_set_data_many(dot, dot_updates, 3) != 0)
        return false;
    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    point_style.stroke_width_px = 0.0f;
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

    DvzColormap* colormap = example_graphite_cyan_colormap(scene);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
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

    double data[2] = {0};
    if (!dvz_panel_position_to_data(
            state->panel, DVZ_PANEL_COORD_PANEL_PX, query->panel_position, data))
    {
        return false;
    }
    *out_value = (double)_sample_field((float)data[0], (float)data[1]);
    return true;
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

    DvzQueryRequest request = dvz_query_request();
    request.request_id = PROBE_REQUEST_ID;
    request.target = DVZ_SCENE_TARGET_PIXEL;

    const int rc = dvz_panel_query(state->panel, state->cursor_x, state->cursor_y, &request);
    if (rc != 0)
        dvz_fprintf(stderr, "dvz_panel_query() failed\n");
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest cursor position and move the live probe marker.
 *
 * @param event portable pointer event
 * @param user_data image probe example state
 */
static void _image_probe_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_CLICK)
        return;

    double data[2] = {0};
    double panel_px[2] = {0};
    state->cursor_valid =
        dvz_panel_transform_point(
            state->panel, DVZ_PANEL_COORD_FIGURE_PX, DVZ_PANEL_COORD_PANEL_PX,
            (const double[2]){event->x, event->y}, panel_px) &&
        dvz_panel_position_to_data(state->panel, DVZ_PANEL_COORD_PANEL_PX, panel_px, data);
    if (state->cursor_valid)
    {
        state->cursor_x = panel_px[0];
        state->cursor_y = panel_px[1];
    }
    _update_probe_marker_from_cursor(state);
}



/**
 * Poll image query results and queue the next probe.
 *
 * @param ctx scenario context
 * @param user_data image probe example state
 */
static void _image_probe_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
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



/**
 * Handle portable scenario events.
 *
 * @param ctx scenario context
 * @param event portable event
 * @param user scenario state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    (void)ctx;
    if (event == NULL)
        return;
    if (event->kind == DVZ_SCENARIO_EVENT_POINTER)
        _image_probe_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the scalar image probe feature scenario.
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

    ImageProbeState* state = (ImageProbeState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    state->values = (float*)dvz_calloc(FIELD_WIDTH * FIELD_HEIGHT, sizeof(*state->values));
    if (state->values == NULL)
        return false;
    _fill_probe_field(state->values);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;

    bool ok = _set_probe_domain(panel);
    if (!ok)
        return false;

    example_graphite_cyan_set_panel_background(panel);

    DvzScale* scale = _add_probe_scale(ctx->scene);
    if (scale == NULL)
        return false;

    ok = _add_probe_image(ctx->scene, panel, scale, state->values);
    if (!ok)
        return false;

    DvzVisual* probe_segments = NULL;
    DvzVisual* probe_dot = NULL;
    ok = _add_probe_marker(ctx->scene, panel, &probe_segments, &probe_dot);
    if (!ok)
        return false;

    double initial_probe_px[2] = {0};
    if (!dvz_panel_data_to_position(
            panel, DVZ_PANEL_COORD_PANEL_PX, (const double[2]){PROBE_X, PROBE_Y},
            initial_probe_px))
    {
        return false;
    }

    state->scene = ctx->scene;
    state->panel = panel;
    state->probe_segments = probe_segments;
    state->probe_dot = probe_dot;
    state->cursor_valid = true;
    state->cursor_x = initial_probe_px[0];
    state->cursor_y = initial_probe_px[1];
    return true;
}



/**
 * Destroy the scalar image probe feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ImageProbeState* state = (ImageProbeState*)user;
    if (state == NULL)
        return;
    dvz_free(state->values);
    dvz_free(state);
}



/**
 * Return the scalar image probe scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_image_probe_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_image_probe",
        .title = "image_probe",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_IMAGE_VISUAL | DVZ_SCENARIO_REQ_QUERY_READBACK |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _image_probe_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the scalar image probe feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_image_probe_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
