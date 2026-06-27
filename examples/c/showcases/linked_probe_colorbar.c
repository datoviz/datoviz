/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* linked_probe_colorbar - linked image panels with one probe readout and shared colorbar.
 *
 * Scenario: linked_panels_probe_colorbar
 * Style: showcase workflow, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c showcases/linked_probe_colorbar
 * Run:    ./build/examples/c/showcases/linked_probe_colorbar --live
 * Smoke:  ./build/examples/c/showcases/linked_probe_colorbar --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_showcase_linked_probe_colorbar_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH               1600u
#define HEIGHT              1200u
#define FIELD_WIDTH         256u
#define FIELD_HEIGHT        192u
#define PROBE_X             0.68f
#define PROBE_Y             0.56f
#define PROBE_REQUEST_ID    11u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ProbeMarker
{
    DvzPanel* panel;
    DvzVisual* visual;
} ProbeMarker;


typedef struct LinkedProbeState
{
    DvzScene* scene;
    float* measurement;
    float* derived;
    DvzPanel* source_panel;
    DvzPanel* derived_panel;
    ProbeMarker source_marker;
    ProbeMarker derived_marker;
    DvzText* readout;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    double last_raw;
    double last_derived;
    bool last_hit;
    bool has_last_result;
} LinkedProbeState;



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
 * Return a deterministic synthetic measurement sample.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return normalized sample value
 */
static float _sample_measurement(float x, float y)
{
    float value = 0.13f + 0.07f * sinf(TAU * (2.3f * x + 0.35f * y));
    value += 0.05f * cosf(TAU * (0.55f * x - 3.6f * y));

    const float filament = sinf(TAU * (x * 1.15f + 0.22f * sinf(TAU * y)));
    value += 0.19f * expf(-18.0f * (filament - 0.18f) * (filament - 0.18f));

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
    const float hot_dy = y - 0.57f;
    value += 0.40f * expf(-(hot_dx * hot_dx + hot_dy * hot_dy) / (2.0f * 0.030f * 0.030f));
    return _clamp01(value);
}



/**
 * Return a second deterministic field derived from the measurement.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return normalized sample value
 */
static float _sample_derived(float x, float y)
{
    float value = 0.18f + 0.72f * _sample_measurement(x, y);
    value -= 0.16f * expf(-((x - 0.69f) * (x - 0.69f)) / (2.0f * 0.100f * 0.100f));
    value += 0.11f * sinf(TAU * (0.85f * x + 1.40f * y));
    value += 0.07f * cosf(TAU * (2.20f * x - 0.45f * y));
    return _clamp01(value);
}



/**
 * Fill both scalar fields used by the workflow.
 *
 * @param measurement output measurement field
 * @param derived output derived field
 */
static void _fill_fields(
    float measurement[FIELD_WIDTH * FIELD_HEIGHT], float derived[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(measurement);
    ANN(derived);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            measurement[y * FIELD_WIDTH + x] = _sample_measurement(u, v);
            derived[y * FIELD_WIDTH + x] = _sample_derived(u, v);
        }
    }
}



/**
 * Set the normalized image data domain on one panel.
 *
 * @param panel target panel
 * @return true when both dimensions were set
 */
static bool _set_image_domain(DvzPanel* panel)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0);
    return rc == 0;
}



/**
 * Create the continuous scale shared by both images and the colorbar.
 *
 * @param scene scene owning scale resources
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "intensity",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                              .precision = 2,
                              .trim_trailing_zeros = true},
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
 * Add one scalar image panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param scale shared scalar color scale
 * @param values scalar field data
 * @param queryable whether the image should accept pixel-query requests
 * @return true when the image was added
 */
static bool _add_image(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, float values[FIELD_WIDTH * FIELD_HEIGHT],
    bool queryable)
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
    if (queryable)
        dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/**
 * Move one marker to a normalized data position.
 *
 * @param marker marker to update
 * @param x normalized data X
 * @param y normalized data Y
 * @return true when the marker was updated
 */
static bool _update_probe_marker(ProbeMarker* marker, float x, float y)
{
    if (marker == NULL || marker->panel == NULL || marker->visual == NULL)
        return false;

    vec3 marker_data[1] = {{x, y, 0.04f}};
    return dvz_visual_set_data(marker->visual, "position", marker_data, 1) == 0;
}


/**
 * Add one marker visual pair to a panel.
 *
 * @param scene scene owning visuals
 * @param panel panel receiving marker visuals
 * @param out output marker
 * @return true when the marker was added
 */
static bool _add_probe_marker(DvzScene* scene, DvzPanel* panel, ProbeMarker* out)
{
    ANN(scene);
    ANN(panel);
    ANN(out);

    ProbeMarker marker = {.panel = panel};
    marker.visual = dvz_marker(scene, 0);
    if (marker.visual == NULL)
        return false;
    vec3 marker_data[1] = {{PROBE_X, PROBE_Y, 0.04f}};
    DvzColor marker_color[1] = {example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING)};
    marker_color[0].a = 245u;
    float marker_diameter[1] = {34.0f};
    float marker_angle[1] = {0.0f};
    uint32_t marker_shape[1] = {DVZ_MARKER_SHAPE_TARGET};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = marker_data, .item_count = 1},
        {.attr_name = "color", .data = marker_color, .item_count = 1},
        {.attr_name = "diameter_px", .data = marker_diameter, .item_count = 1},
        {.attr_name = "angle", .data = marker_angle, .item_count = 1},
        {.attr_name = "shape", .data = marker_shape, .item_count = 1},
    };
    if (dvz_visual_set_data_many(marker.visual, updates, 5) != 0)
        return false;
    DvzMarkerStyle marker_style = dvz_marker_style();
    marker_style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    marker_style.stroke_width_px = 0.0f;
    if (dvz_marker_set_style(marker.visual, &marker_style) != 0)
        return false;
    if (dvz_visual_set_depth_test(marker.visual, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, marker.visual, NULL) != 0)
        return false;

    *out = marker;
    return true;
}



/**
 * Set or refresh the readout text from one normalized data coordinate.
 *
 * @param state workflow state
 * @param x normalized data X
 * @param y normalized data Y
 * @param hit whether the latest query hit the source image
 */
static void _update_readout(LinkedProbeState* state, float x, float y, bool hit)
{
    if (state == NULL || state->readout == NULL)
        return;

    char text[160] = {0};
    if (hit)
    {
        const float measurement = _sample_measurement(x, y);
        const float derived = _sample_derived(x, y);
        dvz_snprintf(
            text, sizeof(text), "x %.3f  y %.3f    measurement %.3f    derived %.3f", x, y,
            measurement, derived);
    }
    else
    {
        dvz_snprintf(text, sizeof(text), "probe outside image");
    }
    dvz_text_set_string(state->readout, text);
}



/**
 * Add the retained screen-space readout text.
 *
 * @param panel panel receiving the text object
 * @return created text object, or NULL on failure
 */
static DvzText* _add_readout(DvzPanel* panel)
{
    ANN(panel);

    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return NULL;

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzTextStyle style = dvz_text_style();
    style.size_px = 20.0f;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = 255u;
    if (dvz_text_set_style(text, &style) != 0)
        return NULL;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 78.0f;
    placement.position[1] = 56.0f;
    placement.position[2] = 0.0f;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    placement.depth_test = false;
    dvz_text_set_placement(text, &placement);

    dvz_text_set_string(text, "x 0.680  y 0.560    measurement 0.000    derived 0.000");
    return text;
}



/**
 * Add a small panel label.
 *
 * @param panel target panel
 * @param label panel label
 * @param role text color role
 * @return true when the label was added
 */
static bool _add_panel_label(DvzPanel* panel, const char* label, ExampleStyleColorRole role)
{
    ANN(panel);
    ANN(label);

    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;
    DvzColor color = example_graphite_cyan_color(role);
    DvzTextStyle style = dvz_text_style();
    style.size_px = 22.0f;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = 255u;
    if (dvz_text_set_style(text, &style) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 96.0f;
    placement.position[1] = 74.0f;
    placement.position[2] = 0.0f;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    placement.depth_test = false;
    dvz_text_set_placement(text, &placement);
    dvz_text_set_string(text, label);
    return true;
}



/**
 * Add linear axes to one normalized image panel.
 *
 * @param panel target panel
 * @return true when both axes were configured
 */
static bool _add_axes(DvzPanel* panel)
{
    ANN(panel);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    ExampleAxisStyleOptions options = example_graphite_cyan_axis_options();
    options.label_size_px = 15.0f;
    options.tick_size_px = 12.0f;
    options.x_label_gap_px = 30.0f;
    options.y_label_gap_px = 42.0f;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, &options))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, &options))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 5;
    ticks.min_pixel_spacing = 90.0f;
    ticks.minor_per_interval = 1;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks))
        return false;
    return dvz_axis_set_tick_policy(y_axis, &ticks);
}



/**
 * Add the shared vertical colorbar to the derived panel.
 *
 * @param panel panel receiving the colorbar
 * @param scale shared scale
 * @return true when the colorbar was added
 */
static bool _add_colorbar(DvzPanel* panel, DvzScale* scale)
{
    ANN(panel);
    ANN(scale);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "intensity",
            .reserve_px = 112.0f,
            .ramp_width_px = 28.0f,
            .plot_gap_px = 14.0f,
            .tick_length_px = 6.0f,
            .label_gap_px = 7.0f,
        });
    if (colorbar == NULL)
        return false;
    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                      .precision = 2,
                      .trim_trailing_zeros = true});
    return true;
}



/**
 * Bind one panzoom controller to both image panels.
 *
 * @param ctx scenario context
 * @param source source panel
 * @param derived derived panel
 * @return true when controller binding is ready
 */
static bool _bind_linked_panzoom(
    DvzScenarioContext* ctx, DvzPanel* source, DvzPanel* derived)
{
    ANN(ctx);
    ANN(source);
    ANN(derived);

    DvzController* panzoom = dvz_panzoom(ctx->scene, NULL);
    if (panzoom == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, source, panzoom, DVZ_DIM_MASK_XY) != 0)
        return false;
    return dvz_scenario_bind_controller(ctx, derived, panzoom, DVZ_DIM_MASK_XY) == 0;
}



/**
 * Queue one source-panel pixel query at the current probe coordinate.
 *
 * @param state workflow state
 */
static void _queue_probe(LinkedProbeState* state)
{
    if (state == NULL || !state->cursor_valid || state->source_panel == NULL)
        return;

    DvzQueryRequest request = dvz_query_request();
    request.request_id = PROBE_REQUEST_ID;
    request.target = DVZ_SCENE_TARGET_PIXEL;
    if (dvz_scenario_panel_query(
            state->source_panel, state->cursor_x, state->cursor_y, &request) != 0)
    {
        dvz_fprintf(stderr, "dvz_scenario_panel_query() failed\n");
    }
}



/**
 * Return whether a query result differs from the last displayed value.
 *
 * @param state workflow state
 * @param query query result
 * @param x normalized data X
 * @param y normalized data Y
 * @return true when the readout should be refreshed
 */
static bool _query_changed(
    LinkedProbeState* state, const DvzQueryResult* query, float x, float y)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;
    if (!query->hit)
        return false;

    const double raw = (double)_sample_measurement(x, y);
    const double derived = (double)_sample_derived(x, y);
    const double raw_delta = fabs(raw - state->last_raw);
    const double derived_delta = fabs(derived - state->last_derived);
    return raw_delta >= 1e-3 || derived_delta >= 1e-3;
}



/**
 * Store the last displayed query result.
 *
 * @param state workflow state
 * @param query query result
 * @param x normalized data X
 * @param y normalized data Y
 */
static void _store_query_result(
    LinkedProbeState* state, const DvzQueryResult* query, float x, float y)
{
    if (state == NULL || query == NULL)
        return;
    state->has_last_result = true;
    state->last_hit = query->hit;
    if (query->hit)
    {
        state->last_raw = (double)_sample_measurement(x, y);
        state->last_derived = (double)_sample_derived(x, y);
    }
}



/**
 * Move both probe markers and queue a source-panel query.
 *
 * @param state workflow state
 * @param x normalized data X
 * @param y normalized data Y
 */
static void _set_probe(LinkedProbeState* state, float x, float y)
{
    if (state == NULL)
        return;

    (void)_update_probe_marker(&state->source_marker, x, y);
    (void)_update_probe_marker(&state->derived_marker, x, y);

    double panel_px[2] = {0};
    if (dvz_panel_data_to_position(
            state->source_panel, DVZ_PANEL_COORD_PANEL_PX, (const double[2]){x, y}, panel_px))
    {
        state->cursor_valid = true;
        state->cursor_x = panel_px[0];
        state->cursor_y = panel_px[1];
        _queue_probe(state);
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Keep both linked probe markers at the latest cursor coordinate.
 *
 * @param event portable pointer event
 * @param user_data workflow state
 */
static void _linked_probe_pointer(
    const DvzScenarioPointerEvent* event, void* user_data)
{
    LinkedProbeState* state = (LinkedProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_CLICK)
        return;

    double data[2] = {0};
    if (dvz_panel_position_to_data(
            state->source_panel, DVZ_PANEL_COORD_FIGURE_PX,
            (const double[2]){event->x, event->y}, data) ||
        dvz_panel_position_to_data(
            state->derived_panel, DVZ_PANEL_COORD_FIGURE_PX,
            (const double[2]){event->x, event->y}, data))
    {
        if (data[0] >= 0.0 && data[0] <= 1.0 && data[1] >= 0.0 && data[1] <= 1.0)
            _set_probe(state, (float)data[0], (float)data[1]);
    }
}



/**
 * Poll query results and update the retained readout.
 *
 * @param ctx scenario context
 * @param user_data workflow state
 */
static void _linked_probe_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    LinkedProbeState* state = (LinkedProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        double data[2] = {0};
        const bool in_panel = dvz_panel_position_to_data(
            state->source_panel, DVZ_PANEL_COORD_PANEL_PX, query.panel_position, data);
        if (
            in_panel && data[0] >= 0.0 && data[0] <= 1.0 && data[1] >= 0.0 &&
            data[1] <= 1.0 && _query_changed(state, &query, (float)data[0], (float)data[1]))
        {
            _update_readout(state, (float)data[0], (float)data[1], query.hit);
            _store_query_result(state, &query, (float)data[0], (float)data[1]);
        }
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
        _linked_probe_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize the linked probe and colorbar workflow scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    LinkedProbeState* state = (LinkedProbeState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->scene = ctx->scene;
    if (out_user != NULL)
        *out_user = state;
    state->measurement =
        (float*)dvz_calloc((DvzSize)FIELD_WIDTH * FIELD_HEIGHT, sizeof(float));
    state->derived = (float*)dvz_calloc((DvzSize)FIELD_WIDTH * FIELD_HEIGHT, sizeof(float));
    EXAMPLE_CHECK(
        state->measurement != NULL && state->derived != NULL, "linked field allocation failed");
    _fill_fields(state->measurement, state->derived);
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    ok = dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 42.0f, .right_px = 34.0f, .top_px = 46.0f,
                                 .bottom_px = 34.0f});
    EXAMPLE_CHECK(ok, "dvz_grid_set_margins() failed");
    ok = dvz_grid_set_gutter(grid, 28.0f, 0.0f);
    EXAMPLE_CHECK(ok, "dvz_grid_set_gutter() failed");

    DvzPanel* source = dvz_grid_panel(grid, 0, 0);
    DvzPanel* derived_panel = dvz_grid_panel(grid, 0, 1);
    EXAMPLE_CHECK(source != NULL && derived_panel != NULL, "dvz_grid_panel() failed");

    example_graphite_cyan_set_panel_background(source);
    example_graphite_cyan_set_panel_background(derived_panel);

    ok = _set_image_domain(source);
    EXAMPLE_CHECK(ok, "_set_image_domain(source) failed");
    ok = _set_image_domain(derived_panel);
    EXAMPLE_CHECK(ok, "_set_image_domain(derived) failed");

    DvzScale* scale = _add_scale(ctx->scene);
    EXAMPLE_CHECK(scale != NULL, "adding shared color scale failed");

    ok = _add_image(ctx->scene, source, scale, state->measurement, true);
    EXAMPLE_CHECK(ok, "adding source image failed");
    ok = _add_image(ctx->scene, derived_panel, scale, state->derived, false);
    EXAMPLE_CHECK(ok, "adding derived image failed");

    ok = _add_axes(source);
    EXAMPLE_CHECK(ok, "_add_axes(source) failed");
    ok = _add_axes(derived_panel);
    EXAMPLE_CHECK(ok, "_add_axes(derived) failed");

    ok = _add_colorbar(derived_panel, scale);
    EXAMPLE_CHECK(ok, "_add_colorbar() failed");

    ProbeMarker source_marker = {0};
    ProbeMarker derived_marker = {0};
    ok = _add_probe_marker(ctx->scene, source, &source_marker);
    EXAMPLE_CHECK(ok, "_add_probe_marker(source) failed");
    ok = _add_probe_marker(ctx->scene, derived_panel, &derived_marker);
    EXAMPLE_CHECK(ok, "_add_probe_marker(derived) failed");

    ok = _add_panel_label(source, "measurement", EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    EXAMPLE_CHECK(ok, "_add_panel_label(source) failed");
    ok = _add_panel_label(derived_panel, "derived", EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    EXAMPLE_CHECK(ok, "_add_panel_label(derived) failed");

    DvzText* readout = _add_readout(source);
    EXAMPLE_CHECK(readout != NULL, "_add_readout() failed");

    ok = _bind_linked_panzoom(ctx, source, derived_panel);
    EXAMPLE_CHECK(ok, "_bind_linked_panzoom() failed");

    state->source_panel = source;
    state->derived_panel = derived_panel;
    state->source_marker = source_marker;
    state->derived_marker = derived_marker;
    state->readout = readout;
    _update_readout(state, PROBE_X, PROBE_Y, true);
    _set_probe(state, PROBE_X, PROBE_Y);

    ok = true;
cleanup:
    return ok;
}



/**
 * Destroy the linked probe and colorbar workflow scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    LinkedProbeState* state = (LinkedProbeState*)user;
    if (state == NULL)
        return;
    dvz_free(state->derived);
    dvz_free(state->measurement);
    dvz_free(state);
}



DvzScenarioSpec dvz_showcase_linked_probe_colorbar_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "linked_panels_probe_colorbar",
        .title = "linked_probe_colorbar",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_IMAGE_VISUAL | DVZ_SCENARIO_REQ_MARKER_VISUAL |
                        DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_QUERY_READBACK |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _linked_probe_post_frame,
        .destroy = _scenario_destroy,
    };
}



/**
 * Run the linked probe and colorbar workflow through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_linked_probe_colorbar_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
