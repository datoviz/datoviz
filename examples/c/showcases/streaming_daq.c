/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* streaming_daq - This example renders a simulated real-time data acquisition system.
 *
 * What to look for: 128 continuous extracellular traces combine correlated background activity,
 * spatially coherent unit spikes, and occasional population events in one persistent raw line-list
 * visual. A wall-clock producer thread emits fixed acquisition blocks into a bounded queue, while
 * the render thread updates only the newly written circular-buffer vertex range. Sparse event
 * markers and a live GUI expose acquisition timing, signal controls, and queue statistics.
 *
 * Scenario: showcases_streaming_daq
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Build:   just example-c showcases/streaming_daq
 * Run:     ./build/examples/c/showcases/streaming_daq --live
 * Smoke:   ./build/examples/c/showcases/streaming_daq --png
 * Video:   ./build/examples/c/showcases/streaming_daq --offscreen-record 180
 * Control: --live opens a left-docked GUI; space pauses; R resets acquisition; F resets panzoom
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/gui.h"
#include "datoviz/input.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"
#include "streaming_daq_model.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define TRACE_VERTICES_PER_INTERVAL 2u
#define CHANNEL_BANK_SIZE           16u
#define EVENT_MARKER_CAPACITY       16u
#define MAX_DRAIN_BLOCKS_PER_FRAME  16u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct StreamingDaqState
{
    DaqModel model;
    ExampleTuner tuner;
    DvzPanel* panel;
    DvzPanzoom* panzoom;
    DvzAxis* x_axis;
    DvzAxis* y_axis;
    DvzVisual* bands;
    DvzVisual* separators;
    DvzVisual* traces;
    DvzVisual* event_markers;
    DvzVisual* sweep_band;
    DvzVisual* cursor;
    vec3* trace_positions;
    DvzColor* trace_colors;
    uint32_t vertices_per_interval;
    uint32_t trace_vertex_count;

    bool paused;
    bool show_cursor;
    bool show_grid;
    bool show_bands;
    bool show_events;
    float gain;
    float noise;
    float spike_rate;
    float spike_amplitude;
    float synchrony;
    float dropout_percent;
    int block_size;

    double deterministic_sample_fraction;
    double smoothed_fps;
    uint64_t upload_bytes;
    uint32_t uploaded_vertex_count;
} StreamingDaqState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_streaming_daq_scenario(void);



/*************************************************************************************************/
/*  Geometry helpers                                                                             */
/*************************************************************************************************/

/**
 * Return the data-space row center for one channel.
 *
 * @param state showcase state
 * @param channel channel index
 * @return row center
 */
static float _row_y(const StreamingDaqState* state, uint32_t channel)
{
    return (float)(state->model.config.channel_count - 1u - channel);
}


/**
 * Return one stable trace color for a channel.
 *
 * @param state showcase state
 * @param channel channel index
 * @return trace color
 */
static DvzColor _channel_color(const StreamingDaqState* state, uint32_t channel)
{
    (void)state;
    static const DvzColor probe_palette[] = {
        {94, 213, 220, 218},
        {88, 193, 222, 212},
        {113, 222, 199, 216},
        {104, 187, 228, 210},
    };
    return probe_palette[(channel / 32u) % 4u];
}


/**
 * Return the first vertex of one physical display interval.
 *
 * @param state showcase state
 * @param interval physical interval index
 * @return first vertex index
 */
static uint32_t _interval_first_vertex(const StreamingDaqState* state, uint32_t interval)
{
    return interval * state->vertices_per_interval;
}


/**
 * Fill trace vertices for one physical display interval.
 *
 * @param state showcase state
 * @param interval physical interval index
 */
static void _fill_trace_interval(StreamingDaqState* state, uint32_t interval)
{
    const DaqConfig* config = &state->model.config;
    const uint32_t next = (interval + 1u) % config->display_sample_count;
    const uint32_t seam = (state->model.write_index + config->display_sample_count - 1u) %
                          config->display_sample_count;
    const bool valid = interval != config->display_sample_count - 1u && interval != seam &&
                       state->model.display_valid[interval] && state->model.display_valid[next];
    const float x0 = (float)interval / (float)config->sample_rate_hz;
    const float x1 = (float)(interval + 1u) / (float)config->sample_rate_hz;
    uint32_t vertex = _interval_first_vertex(state, interval);

    for (uint32_t channel = 0; channel < config->channel_count; channel++)
    {
        const float row = _row_y(state, channel);
        if (!valid)
        {
            for (uint32_t j = 0; j < TRACE_VERTICES_PER_INTERVAL; j++)
            {
                state->trace_positions[vertex + j][0] = x0;
                state->trace_positions[vertex + j][1] = row + 0.5f;
                state->trace_positions[vertex + j][2] = 0.0f;
            }
            vertex += TRACE_VERTICES_PER_INTERVAL;
            continue;
        }

        const float value0 =
            state->model.display_values[(uint64_t)interval * config->channel_count + channel];
        const float value1 =
            state->model.display_values[(uint64_t)next * config->channel_count + channel];
        const float y0 = row + 0.5f + 0.58f * state->gain * value0;
        const float y1 = row + 0.5f + 0.58f * state->gain * value1;
        state->trace_positions[vertex + 0u][0] = x0;
        state->trace_positions[vertex + 0u][1] = y0;
        state->trace_positions[vertex + 0u][2] = 0.0f;
        state->trace_positions[vertex + 1u][0] = x1;
        state->trace_positions[vertex + 1u][1] = y1;
        state->trace_positions[vertex + 1u][2] = 0.0f;
        vertex += TRACE_VERTICES_PER_INTERVAL;
    }
}


/**
 * Rebuild every retained trace position.
 *
 * @param state showcase state
 */
static void _fill_all_trace_positions(StreamingDaqState* state)
{
    for (uint32_t interval = 0; interval < state->model.config.display_sample_count; interval++)
        _fill_trace_interval(state, interval);
}


/**
 * Upload all trace positions after a global display change.
 *
 * @param state showcase state
 * @return whether the upload succeeded
 */
static bool _upload_all_traces(StreamingDaqState* state)
{
    _fill_all_trace_positions(state);
    DvzResult result = dvz_visual_set_data(
        state->traces, "position", state->trace_positions, state->trace_vertex_count);
    state->uploaded_vertex_count = state->trace_vertex_count;
    state->upload_bytes = (uint64_t)state->trace_vertex_count * sizeof(vec3);
    return result == DVZ_OK;
}


/**
 * Rebuild and upload one physical display-ring span.
 *
 * @param state showcase state
 * @param span physical sample span
 * @return whether the range upload succeeded
 */
static bool _upload_trace_span(StreamingDaqState* state, DaqDirtySpan span)
{
    if (span.sample_count == 0u)
        return true;
    const uint32_t first_interval = span.first_sample > 0u ? span.first_sample - 1u : 0u;
    uint64_t end = (uint64_t)span.first_sample + span.sample_count;
    if (end > state->model.config.display_sample_count)
        return false;
    const uint32_t end_interval = (uint32_t)end;
    if (end_interval <= first_interval)
        return true;

    for (uint32_t interval = first_interval; interval < end_interval; interval++)
        _fill_trace_interval(state, interval);
    const uint32_t interval_count = end_interval - first_interval;
    const uint32_t first_vertex = _interval_first_vertex(state, first_interval);
    const uint32_t vertex_count = interval_count * state->vertices_per_interval;
    DvzResult result = dvz_visual_set_data_range(
        state->traces, "position", first_vertex, &state->trace_positions[first_vertex],
        vertex_count);
    state->uploaded_vertex_count += vertex_count;
    state->upload_bytes += (uint64_t)vertex_count * sizeof(vec3);
    return result == DVZ_OK;
}


/**
 * Upload only trace intervals affected by one display-ring advance.
 *
 * @param state showcase state
 * @param dirty physical sample ranges changed by the model
 * @return whether the upload succeeded
 */
static bool _upload_dirty_traces(StreamingDaqState* state, const DaqDirtyRanges* dirty)
{
    state->uploaded_vertex_count = 0;
    state->upload_bytes = 0;
    if (dirty->advanced_sample_count == 0)
        return true;
    if (dirty->full)
        return _upload_all_traces(state);

    for (uint32_t i = 0; i < dirty->span_count; i++)
    {
        if (!_upload_trace_span(state, dirty->spans[i]))
            return false;
    }
    return true;
}


/**
 * Update sweep-band and cursor positions from the current write index.
 *
 * @param state showcase state
 * @return whether both overlay updates succeeded
 */
static bool _update_cursor(StreamingDaqState* state)
{
    const DaqConfig* config = &state->model.config;
    const float duration = (float)(config->display_sample_count - 1u) / config->sample_rate_hz;
    const float x = (float)state->model.write_index / config->sample_rate_hz;
    const float band_before = 0.012f * duration;
    const float band_after = 0.0035f * duration;
    const float x0 = fmaxf(0.0f, x - band_before);
    const float x1 = fminf(duration, x + band_after);
    const float y0 = -0.5f;
    const float y1 = (float)config->channel_count - 0.5f;
    vec3 band_positions[4] = {
        {x0, y0, 0.0f},
        {x1, y0, 0.0f},
        {x0, y1, 0.0f},
        {x1, y1, 0.0f},
    };
    vec3 cursor_positions[2] = {{x, y0, 0.0f}, {x, y1, 0.0f}};
    DvzResult band_result =
        dvz_visual_set_data_range(state->sweep_band, "position", 0u, band_positions, 4u);
    DvzResult cursor_result =
        dvz_visual_set_data_range(state->cursor, "position", 0u, cursor_positions, 2u);
    state->upload_bytes += sizeof(band_positions) + sizeof(cursor_positions);
    return band_result == DVZ_OK && cursor_result == DVZ_OK;
}


/**
 * Refresh the small fixed-capacity hardware-event overlay.
 *
 * @param state showcase state
 * @return whether both marker attributes were updated
 */
static bool _update_event_markers(StreamingDaqState* state)
{
    const uint32_t vertex_count = 2u * EVENT_MARKER_CAPACITY;
    vec3 positions[2u * EVENT_MARKER_CAPACITY] = {{0}};
    DvzColor colors[2u * EVENT_MARKER_CAPACITY] = {{0}};
    const float y0 = -0.5f;
    const float y1 = (float)state->model.config.channel_count - 0.5f;
    uint32_t marker = 0;
    for (uint32_t sample = 0; sample < state->model.config.display_sample_count; sample++)
    {
        DaqEventKind kind = (DaqEventKind)state->model.display_events[sample];
        if (kind == DAQ_EVENT_NONE || marker == EVENT_MARKER_CAPACITY)
            continue;
        const float x = (float)sample / state->model.config.sample_rate_hz;
        DvzColor color = kind == DAQ_EVENT_REWARD ? dvz_color_rgba(255, 179, 71, 150)
                         : kind == DAQ_EVENT_SYNC ? dvz_color_rgba(255, 103, 129, 145)
                                                  : dvz_color_rgba(132, 224, 210, 130);
        positions[2u * marker + 0u][0] = x;
        positions[2u * marker + 0u][1] = y0;
        positions[2u * marker + 1u][0] = x;
        positions[2u * marker + 1u][1] = y1;
        colors[2u * marker + 0u] = color;
        colors[2u * marker + 1u] = color;
        marker++;
    }
    for (; marker < EVENT_MARKER_CAPACITY; marker++)
    {
        positions[2u * marker + 0u][1] = y0;
        positions[2u * marker + 1u][1] = y0;
    }

    DvzResult position_result =
        dvz_visual_set_data_range(state->event_markers, "position", 0u, positions, vertex_count);
    DvzResult color_result =
        dvz_visual_set_data_range(state->event_markers, "color", 0u, colors, vertex_count);
    state->upload_bytes += sizeof(positions) + sizeof(colors);
    return position_result == DVZ_OK && color_result == DVZ_OK;
}


/**
 * Allocate and fill retained trace arrays.
 *
 * @param state showcase state
 * @return whether allocation succeeded
 */
static bool _allocate_trace_arrays(StreamingDaqState* state)
{
    state->vertices_per_interval = state->model.config.channel_count * TRACE_VERTICES_PER_INTERVAL;
    uint64_t vertex_count64 =
        (uint64_t)state->vertices_per_interval * state->model.config.display_sample_count;
    if (vertex_count64 == 0 || vertex_count64 > UINT32_MAX ||
        vertex_count64 > SIZE_MAX / sizeof(vec3) || vertex_count64 > SIZE_MAX / sizeof(DvzColor))
    {
        return false;
    }
    state->trace_vertex_count = (uint32_t)vertex_count64;
    state->trace_positions = (vec3*)dvz_calloc(state->trace_vertex_count, sizeof(vec3));
    state->trace_colors = (DvzColor*)dvz_calloc(state->trace_vertex_count, sizeof(DvzColor));
    if (state->trace_positions == NULL || state->trace_colors == NULL)
        return false;

    for (uint32_t interval = 0; interval < state->model.config.display_sample_count; interval++)
    {
        uint32_t vertex = _interval_first_vertex(state, interval);
        for (uint32_t channel = 0; channel < state->model.config.channel_count; channel++)
        {
            DvzColor color = _channel_color(state, channel);
            for (uint32_t j = 0; j < TRACE_VERTICES_PER_INTERVAL; j++)
                state->trace_colors[vertex++] = color;
        }
    }
    _fill_all_trace_positions(state);
    return true;
}



/*************************************************************************************************/
/*  Scene helpers                                                                                */
/*************************************************************************************************/

/**
 * Add alternating channel-bank bands behind the traces.
 *
 * @param state showcase state
 * @param scene owning scene
 * @return whether the band visual was added
 */
static bool _add_bands(StreamingDaqState* state, DvzScene* scene)
{
    const uint32_t bank_count =
        (state->model.config.channel_count + CHANNEL_BANK_SIZE - 1u) / CHANNEL_BANK_SIZE;
    const uint32_t shown_count = (bank_count + 1u) / 2u;
    const uint32_t vertex_count = shown_count * 6u;
    vec3* positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    if (positions == NULL || colors == NULL)
    {
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    const float duration = (float)(state->model.config.display_sample_count - 1u) /
                           state->model.config.sample_rate_hz;
    const DvzColor color = dvz_color_rgba(42, 52, 63, 92);
    uint32_t vertex = 0;
    for (uint32_t bank = 0; bank < bank_count; bank += 2u)
    {
        uint32_t first_channel = bank * CHANNEL_BANK_SIZE;
        uint32_t last_channel = first_channel + CHANNEL_BANK_SIZE - 1u;
        if (last_channel >= state->model.config.channel_count)
            last_channel = state->model.config.channel_count - 1u;
        float top = _row_y(state, first_channel) + 0.5f;
        float bottom = _row_y(state, last_channel) - 0.5f;
        const vec3 quad[6] = {
            {0.0f, bottom, 0.0f}, {duration, bottom, 0.0f}, {duration, top, 0.0f},
            {0.0f, bottom, 0.0f}, {duration, top, 0.0f},    {0.0f, top, 0.0f},
        };
        for (uint32_t j = 0; j < 6u; j++)
        {
            positions[vertex][0] = quad[j][0];
            positions[vertex][1] = quad[j][1];
            positions[vertex][2] = quad[j][2];
            colors[vertex++] = color;
        }
    }

    state->bands = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    DvzResult upload_result =
        state->bands != NULL ? dvz_visual_set_data_many(state->bands, updates, 2u) : DVZ_ERROR;
    DvzResult alpha_result = state->bands != NULL
                                 ? dvz_visual_set_alpha_mode(state->bands, DVZ_ALPHA_BLENDED)
                                 : DVZ_ERROR;
    DvzResult depth_result =
        state->bands != NULL ? dvz_visual_set_depth_test(state->bands, false) : DVZ_ERROR;
    DvzResult add_result =
        state->bands != NULL ? dvz_panel_add_visual(state->panel, state->bands, NULL) : DVZ_ERROR;
    dvz_free(colors);
    dvz_free(positions);
    return upload_result == DVZ_OK && alpha_result == DVZ_OK && depth_result == DVZ_OK &&
           add_result == DVZ_OK;
}


/**
 * Add muted separators between channel banks.
 *
 * @param state showcase state
 * @param scene owning scene
 * @return whether the separator visual was added
 */
static bool _add_separators(StreamingDaqState* state, DvzScene* scene)
{
    const uint32_t bank_count =
        (state->model.config.channel_count + CHANNEL_BANK_SIZE - 1u) / CHANNEL_BANK_SIZE;
    const uint32_t line_count = bank_count > 0u ? bank_count - 1u : 0u;
    const uint32_t vertex_count = 2u * line_count;
    vec3* positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    if (positions == NULL || colors == NULL)
    {
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }
    const float duration = (float)(state->model.config.display_sample_count - 1u) /
                           state->model.config.sample_rate_hz;
    const DvzColor color = dvz_color_rgba(75, 88, 101, 115);
    for (uint32_t bank = 1; bank < bank_count; bank++)
    {
        uint32_t channel = bank * CHANNEL_BANK_SIZE;
        float y = _row_y(state, channel) + 0.5f;
        positions[2u * (bank - 1u) + 0u][0] = 0.0f;
        positions[2u * (bank - 1u) + 0u][1] = y;
        positions[2u * (bank - 1u) + 1u][0] = duration;
        positions[2u * (bank - 1u) + 1u][1] = y;
        colors[2u * (bank - 1u) + 0u] = color;
        colors[2u * (bank - 1u) + 1u] = color;
    }

    state->separators = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    DvzResult upload_result = state->separators != NULL
                                  ? dvz_visual_set_data_many(state->separators, updates, 2u)
                                  : DVZ_ERROR;
    DvzResult depth_result = state->separators != NULL
                                 ? dvz_visual_set_depth_test(state->separators, false)
                                 : DVZ_ERROR;
    DvzResult add_result = state->separators != NULL
                               ? dvz_panel_add_visual(state->panel, state->separators, NULL)
                               : DVZ_ERROR;
    dvz_free(colors);
    dvz_free(positions);
    return upload_result == DVZ_OK && depth_result == DVZ_OK && add_result == DVZ_OK;
}


/**
 * Add the persistent raw trace visual.
 *
 * @param state showcase state
 * @param scene owning scene
 * @return whether the trace visual was added
 */
static bool _add_traces(StreamingDaqState* state, DvzScene* scene)
{
    state->traces = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0);
    if (state->traces == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position",
         .data = state->trace_positions,
         .item_count = state->trace_vertex_count},
        {.attr_name = "color",
         .data = state->trace_colors,
         .item_count = state->trace_vertex_count},
    };
    DvzResult upload_result = dvz_visual_set_data_many(state->traces, updates, 2u);
    DvzResult depth_result = dvz_visual_set_depth_test(state->traces, false);
    DvzResult add_result = dvz_panel_add_visual(state->panel, state->traces, NULL);
    return upload_result == DVZ_OK && depth_result == DVZ_OK && add_result == DVZ_OK;
}


/**
 * Add sparse stimulus, reward, and synchronization event markers.
 *
 * @param state showcase state
 * @param scene owning scene
 * @return whether the marker visual was added
 */
static bool _add_event_markers(StreamingDaqState* state, DvzScene* scene)
{
    const uint32_t vertex_count = 2u * EVENT_MARKER_CAPACITY;
    vec3 positions[2u * EVENT_MARKER_CAPACITY] = {{0}};
    DvzColor colors[2u * EVENT_MARKER_CAPACITY] = {{0}};
    state->event_markers = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0);
    if (state->event_markers == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    DvzResult upload_result = dvz_visual_set_data_many(state->event_markers, updates, 2u);
    DvzResult alpha_result = dvz_visual_set_alpha_mode(state->event_markers, DVZ_ALPHA_BLENDED);
    DvzResult depth_result = dvz_visual_set_depth_test(state->event_markers, false);
    DvzResult add_result = dvz_panel_add_visual(state->panel, state->event_markers, NULL);
    return upload_result == DVZ_OK && alpha_result == DVZ_OK && depth_result == DVZ_OK &&
           add_result == DVZ_OK && _update_event_markers(state);
}


/**
 * Add the dark sweep band and bright write cursor overlays.
 *
 * @param state showcase state
 * @param scene owning scene
 * @return whether both overlays were added
 */
static bool _add_cursor(StreamingDaqState* state, DvzScene* scene)
{
    const DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    const DvzColor band_colors[4] = {panel_bg, panel_bg, panel_bg, panel_bg};
    const DvzColor cursor_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    const DvzColor cursor_colors[2] = {cursor_color, cursor_color};
    vec3 initial_band[4] = {{0}};
    vec3 initial_cursor[2] = {{0}};

    state->sweep_band = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
    state->cursor = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0);
    if (state->sweep_band == NULL || state->cursor == NULL)
        return false;
    DvzVisualDataUpdate band_updates[] = {
        {.attr_name = "position", .data = initial_band, .item_count = 4u},
        {.attr_name = "color", .data = band_colors, .item_count = 4u},
    };
    DvzVisualDataUpdate cursor_updates[] = {
        {.attr_name = "position", .data = initial_cursor, .item_count = 2u},
        {.attr_name = "color", .data = cursor_colors, .item_count = 2u},
    };
    DvzResult band_upload = dvz_visual_set_data_many(state->sweep_band, band_updates, 2u);
    DvzResult cursor_upload = dvz_visual_set_data_many(state->cursor, cursor_updates, 2u);
    DvzResult band_depth = dvz_visual_set_depth_test(state->sweep_band, false);
    DvzResult cursor_depth = dvz_visual_set_depth_test(state->cursor, false);
    DvzResult band_add = dvz_panel_add_visual(state->panel, state->sweep_band, NULL);
    DvzResult cursor_add = dvz_panel_add_visual(state->panel, state->cursor, NULL);
    return band_upload == DVZ_OK && cursor_upload == DVZ_OK && band_depth == DVZ_OK &&
           cursor_depth == DVZ_OK && band_add == DVZ_OK && cursor_add == DVZ_OK &&
           _update_cursor(state);
}


/**
 * Configure readable quantitative axes for the acquisition display.
 *
 * @param state showcase state
 * @return whether axis setup succeeded
 */
static bool _configure_axes(StreamingDaqState* state)
{
    state->x_axis = dvz_panel_axis(state->panel, DVZ_DIM_X);
    state->y_axis = dvz_panel_axis(state->panel, DVZ_DIM_Y);
    if (state->x_axis == NULL || state->y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(state->x_axis, false, NULL) ||
        !example_graphite_cyan_apply_axis_style(state->y_axis, true, NULL))
    {
        return false;
    }

    const double tick_values[] = {
        _row_y(state, 0u) + 0.5,  _row_y(state, 32u) + 0.5,  _row_y(state, 64u) + 0.5,
        _row_y(state, 96u) + 0.5, _row_y(state, 127u) + 0.5,
    };
    const char* tick_labels[] = {"AP000", "AP032", "AP064", "AP096", "AP127"};
    DvzAxisTicks ticks = {
        DVZ_STRUCT_INIT_FIELDS(DvzAxisTicks),
        .count = 5u,
        .values = tick_values,
        .labels = tick_labels,
    };
    DvzResult ticks_result = dvz_axis_set_ticks(state->y_axis, &ticks);
    DvzResult x_grid_result = dvz_axis_set_grid(state->x_axis, true);
    DvzResult y_grid_result = dvz_axis_set_grid(state->y_axis, true);
    DvzResult x_label_result = dvz_axis_set_label(state->x_axis, "acquisition time (s)");
    DvzResult y_label_result = dvz_axis_set_label(state->y_axis, "probe channel");
    return ticks_result == DVZ_OK && x_grid_result == DVZ_OK && y_grid_result == DVZ_OK &&
           x_label_result == DVZ_OK && y_label_result == DVZ_OK;
}


/**
 * Reset acquisition state and refresh the retained geometry.
 *
 * @param state showcase state
 * @return whether reset and upload succeeded
 */
static bool _reset_acquisition(StreamingDaqState* state)
{
    if (!daq_model_reset(&state->model))
        return false;
    return _upload_all_traces(state) && _update_event_markers(state) && _update_cursor(state);
}



/*************************************************************************************************/
/*  GUI                                                                                          */
/*************************************************************************************************/

/**
 * Show one formatted acquisition statistic as unformatted GUI text.
 *
 * @param gui GUI overlay
 * @param format printf-style format string
 * @param value integer value
 */
static void _gui_stat_u64(DvzGui* gui, const char* format, uint64_t value)
{
    char text[128] = {0};
    dvz_snprintf(text, sizeof(text), format, value);
    dvz_gui_text(gui, text);
}


/**
 * Build the live acquisition tuner component.
 *
 * @param gui GUI overlay
 * @param user_data showcase state
 * @return whether any control changed
 */
static bool _gui_controls(DvzGui* gui, void* user_data)
{
    StreamingDaqState* state = (StreamingDaqState*)user_data;
    if (gui == NULL || state == NULL)
        return false;

    bool pause_changed = false;
    bool gain_changed = false;
    bool noise_changed = false;
    bool spike_rate_changed = false;
    bool spike_amplitude_changed = false;
    bool synchrony_changed = false;
    bool dropout_changed = false;
    bool block_changed = false;
    bool cursor_changed = false;
    bool grid_changed = false;
    bool bands_changed = false;
    bool events_changed = false;
    bool reset = false;
    bool fit = false;

    dvz_gui_separator_text(gui, "Acquisition");
    pause_changed |= dvz_gui_checkbox(gui, "Paused", &state->paused);
    reset |= dvz_gui_button(gui, "Reset buffer");
    dvz_gui_same_line(gui, 0.0f, 8.0f);
    fit |= dvz_gui_button(gui, "Fit view");
    block_changed |= dvz_gui_slider_int(gui, "Block samples", &state->block_size, 1, 64);
    dropout_changed |= dvz_gui_slider_float_format(
        gui, "Simulated dropout", &state->dropout_percent, 0.0f, 10.0f, "%.1f %%");

    dvz_gui_separator_text(gui, "Neural source");
    noise_changed |= dvz_gui_slider_float(gui, "Background noise", &state->noise, 0.0f, 3.0f);
    spike_rate_changed |= dvz_gui_slider_float(gui, "Firing rate", &state->spike_rate, 0.1f, 3.0f);
    spike_amplitude_changed |=
        dvz_gui_slider_float(gui, "Spike amplitude", &state->spike_amplitude, 0.0f, 3.0f);
    synchrony_changed |=
        dvz_gui_slider_float(gui, "Population synchrony", &state->synchrony, 0.0f, 3.0f);

    dvz_gui_separator_text(gui, "Display");
    gain_changed |= dvz_gui_slider_float(gui, "Trace gain", &state->gain, 0.25f, 2.5f);
    events_changed |= dvz_gui_checkbox(gui, "Event markers", &state->show_events);
    cursor_changed |= dvz_gui_checkbox(gui, "Sweep cursor", &state->show_cursor);
    grid_changed |= dvz_gui_checkbox(gui, "Grid", &state->show_grid);
    bands_changed |= dvz_gui_checkbox(gui, "Channel banks", &state->show_bands);

    dvz_gui_separator_text(gui, "Telemetry");
    DaqStats stats = {0};
    daq_model_stats(&state->model, &stats);
    char fps_text[128] = {0};
    char upload_text[128] = {0};
    dvz_snprintf(fps_text, sizeof(fps_text), "Render: %.1f FPS", state->smoothed_fps);
    dvz_snprintf(
        upload_text, sizeof(upload_text), "Upload: %.1f KiB (%u vertices)",
        (double)state->upload_bytes / 1024.0, state->uploaded_vertex_count);
    dvz_gui_text(gui, "128 channels | 10 kHz | 1.000 s ring");
    dvz_gui_text(gui, "28 units | spatial spike footprints");
    dvz_gui_text(gui, "One raw line-list trace draw");
    dvz_gui_text(gui, fps_text);
    dvz_gui_text(gui, upload_text);
    _gui_stat_u64(gui, "Generated samples: %" PRIu64, stats.generated_sample_count);
    _gui_stat_u64(gui, "Dropped samples: %" PRIu64, stats.dropped_sample_count);
    _gui_stat_u64(gui, "Queue overruns: %" PRIu64, stats.overrun_block_count);
    _gui_stat_u64(gui, "Display wraps: %" PRIu64, stats.wrap_count);
    char queue_text[128] = {0};
    dvz_snprintf(
        queue_text, sizeof(queue_text), "Queue depth: %u / %u", stats.queue_depth,
        DAQ_BLOCK_QUEUE_CAPACITY);
    dvz_gui_text(gui, queue_text);
    dvz_gui_text(gui, "Space: pause | R: reset | F: fit");

    if (pause_changed)
        daq_model_set_paused(&state->model, state->paused);
    if (block_changed)
        daq_model_set_block_size(&state->model, (uint32_t)state->block_size);
    if (dropout_changed)
    {
        uint32_t permille = (uint32_t)lroundf(10.0f * state->dropout_percent);
        daq_model_set_dropout(&state->model, permille);
    }
    if (noise_changed)
    {
        uint32_t permille = (uint32_t)lroundf(1000.0f * state->noise);
        daq_model_set_noise(&state->model, permille);
    }
    if (spike_rate_changed)
    {
        uint32_t permille = (uint32_t)lroundf(1000.0f * state->spike_rate);
        daq_model_set_spike_rate(&state->model, permille);
    }
    if (spike_amplitude_changed)
    {
        uint32_t permille = (uint32_t)lroundf(1000.0f * state->spike_amplitude);
        daq_model_set_spike_amplitude(&state->model, permille);
    }
    if (synchrony_changed)
    {
        uint32_t permille = (uint32_t)lroundf(1000.0f * state->synchrony);
        daq_model_set_synchrony(&state->model, permille);
    }
    if (gain_changed)
        (void)_upload_all_traces(state);
    if (cursor_changed)
    {
        (void)dvz_visual_set_visible(state->sweep_band, state->show_cursor);
        (void)dvz_visual_set_visible(state->cursor, state->show_cursor);
    }
    if (grid_changed)
    {
        (void)dvz_axis_set_grid(state->x_axis, state->show_grid);
        (void)dvz_axis_set_grid(state->y_axis, state->show_grid);
        (void)dvz_visual_set_visible(state->separators, state->show_grid);
    }
    if (bands_changed)
        (void)dvz_visual_set_visible(state->bands, state->show_bands);
    if (events_changed)
        (void)dvz_visual_set_visible(state->event_markers, state->show_events);
    if (reset)
        (void)_reset_acquisition(state);
    if (fit)
        (void)dvz_panzoom_reset(state->panzoom);
    return pause_changed || gain_changed || noise_changed || spike_rate_changed ||
           spike_amplitude_changed || synchrony_changed || dropout_changed || block_changed ||
           cursor_changed || grid_changed || bands_changed || events_changed || reset || fit;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the streaming DAQ scene and retained resources.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    StreamingDaqState* state = (StreamingDaqState*)dvz_calloc(1, sizeof(StreamingDaqState));
    if (state == NULL)
        return false;
    state->tuner = example_tuner("Streaming DAQ");
    state->gain = 1.0f;
    state->noise = 1.0f;
    state->spike_rate = 1.0f;
    state->spike_amplitude = 1.0f;
    state->synchrony = 1.0f;
    state->block_size = 64;
    state->show_cursor = true;
    state->show_grid = true;
    state->show_bands = true;
    state->show_events = true;

    DaqConfig config = daq_config_default();
    if (!daq_model_init(&state->model, &config) || !daq_model_prefill(&state->model) ||
        !_allocate_trace_arrays(state))
    {
        goto error;
    }

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;
    example_tuner_figure(&state->tuner, ctx->figure);
    state->panel = dvz_panel_full(ctx->figure);
    if (state->panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(state->panel);

    const double duration = (double)(config.display_sample_count - 1u) / config.sample_rate_hz;
    DvzResult x_domain = dvz_panel_set_domain(state->panel, DVZ_DIM_X, 0.0, duration);
    DvzResult y_domain =
        dvz_panel_set_domain(state->panel, DVZ_DIM_Y, -0.5, (double)config.channel_count - 0.5);
    if (x_domain != DVZ_OK || y_domain != DVZ_OK || !_add_bands(state, ctx->scene) ||
        !_add_separators(state, ctx->scene) || !_add_traces(state, ctx->scene) ||
        !_add_event_markers(state, ctx->scene) || !_add_cursor(state, ctx->scene) ||
        !_configure_axes(state))
    {
        goto error;
    }

    state->panzoom = dvz_scenario_panzoom(ctx, state->panel, NULL, DVZ_DIM_MASK_XY);
    if (state->panzoom == NULL)
        goto error;
#ifndef DVZ_EXAMPLE_NO_APP
    if (!example_tuner_add_component(
            &state->tuner, "Acquisition controls", state, NULL, _gui_controls, NULL, NULL, NULL))
    {
        goto error;
    }
#endif
    *out_user = state;
    return true;

error:
    daq_model_destroy(&state->model);
    dvz_free(state->trace_colors);
    dvz_free(state->trace_positions);
    dvz_free(state);
    return false;
}


/**
 * Consume acquisition data and update only changed trace intervals.
 *
 * @param ctx scenario context
 * @param user showcase state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    StreamingDaqState* state = (StreamingDaqState*)user;
    if (ctx == NULL || state == NULL)
        return;
    if (ctx->dt > 0.0)
    {
        const double fps = 1.0 / ctx->dt;
        state->smoothed_fps =
            state->smoothed_fps > 0.0 ? 0.92 * state->smoothed_fps + 0.08 * fps : fps;
    }

    DaqDirtyRanges dirty = {0};
    if (!state->paused)
    {
        if (state->model.producer_started)
        {
            (void)daq_model_drain(&state->model, MAX_DRAIN_BLOCKS_PER_FRAME, &dirty);
        }
        else
        {
            double fps = ctx->preview_mode && ctx->preview_fps > 0.0 ? ctx->preview_fps : 60.0;
            state->deterministic_sample_fraction += state->model.config.sample_rate_hz / fps;
            uint32_t sample_count = (uint32_t)state->deterministic_sample_fraction;
            state->deterministic_sample_fraction -= sample_count;
            if (sample_count > 0)
                (void)daq_model_advance(&state->model, sample_count, &dirty);
        }
    }
    else
    {
        state->upload_bytes = 0;
        state->uploaded_vertex_count = 0;
    }

    if (!_upload_dirty_traces(state, &dirty) || !_update_event_markers(state) ||
        !_update_cursor(state))
        dvz_fprintf(stderr, "streaming_daq: retained range update failed\n");
}


/**
 * Handle pause, reset, and fit keyboard shortcuts.
 *
 * @param ctx scenario context
 * @param event routed event
 * @param user showcase state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    (void)ctx;
    StreamingDaqState* state = (StreamingDaqState*)user;
    if (state == NULL || event == NULL || event->kind != DVZ_SCENARIO_EVENT_KEY ||
        event->content.key.type != DVZ_KEYBOARD_EVENT_PRESS)
    {
        return;
    }
    if (event->content.key.key == DVZ_KEY_SPACE)
    {
        state->paused = !state->paused;
        daq_model_set_paused(&state->model, state->paused);
    }
    else if (event->content.key.key == DVZ_KEY_R)
    {
        (void)_reset_acquisition(state);
    }
    else if (event->content.key.key == DVZ_KEY_F)
    {
        (void)dvz_panzoom_reset(state->panzoom);
    }
}


/**
 * Attach native Dear ImGui controls and start wall-clock acquisition.
 *
 * @param ctx scenario context
 * @param app owning app
 * @param view native view
 * @param user showcase state
 * @return whether native setup succeeded
 */
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    StreamingDaqState* state = (StreamingDaqState*)user;
    if (ctx == NULL || state == NULL || view == NULL ||
        ctx->presentation != DVZ_RUNNER_PRESENT_GLFW)
    {
        return true;
    }

    if (!example_tuner_attach(&state->tuner, view))
        return false;
    if (!daq_model_start(&state->model))
    {
        example_tuner_detach(&state->tuner);
        return false;
    }
    return true;
}


/**
 * Stop acquisition and release showcase-owned arrays.
 *
 * @param ctx scenario context
 * @param user showcase state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    StreamingDaqState* state = (StreamingDaqState*)user;
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    daq_model_destroy(&state->model);
    dvz_free(state->trace_colors);
    dvz_free(state->trace_positions);
    dvz_free(state);
}


/**
 * Return the streaming DAQ showcase scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_streaming_daq_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_streaming_daq",
        .title = "Streaming DAQ · 128 channels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .continuous_frames = true,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .event = _scenario_event,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the streaming DAQ showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_streaming_daq_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
