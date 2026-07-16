/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* cortical_activity - This example animates a human auditory dSPM estimate on cortex.
 *
 * What to look for: measured MEG trials from an audiovisual experiment were averaged and mapped
 * onto the participant's inflated cortical surface with a noise-normalized minimum-norm inverse.
 * Activity emerges around auditory cortex near 100 ms after the left-ear tone. The magma overlay,
 * numeric colorbar, and time readout share one fixed scale across the entire animation.
 *
 * dSPM is a model-derived, dimensionless source estimate. It is not a direct measurement of
 * neuronal firing or absolute current amplitude.
 *
 * Scenario: showcases_cortical_activity
 * Style: showcase, graphite_cyan shell with magma scientific scale, 1280x720 window target
 *
 * Build:   just example-c showcases/cortical_activity
 * Prepare: uv run --isolated --with mne==1.12.1 --with mne-bids==0.19.0 \
 *          --with nibabel==5.4.2 --with numpy==2.3.4 --with scipy==1.18.0 --with requests \
 *          python tools/data/prepare_cortical_activity.py
 * Run:     ./build/examples/c/showcases/cortical_activity --live
 * Smoke:   ./build/examples/c/showcases/cortical_activity --png
 * Control: space pauses; left/right arrows scrub while paused
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/input.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define CORTICAL_ACTIVITY_PATH                                                                    \
    ".cache/datoviz/examples/cortical_activity/prepared/cortical_activity.bin"
#define CORTICAL_ACTIVITY_PREPARE_COMMAND                                                         \
    "uv run --isolated --with mne==1.12.1 --with mne-bids==0.19.0 --with nibabel==5.4.2 "         \
    "--with numpy==2.3.4 --with scipy==1.18.0 --with requests "                                   \
    "python tools/data/prepare_cortical_activity.py"
#define CORTICAL_ACTIVITY_MAGIC        "DVZCTA1"
#define CORTICAL_ACTIVITY_VERSION      1u
#define CORTICAL_ACTIVITY_HEADER_SIZE  80u
#define CORTICAL_ACTIVITY_MAX_VERTICES 1000000u
#define CORTICAL_ACTIVITY_MAX_INDICES  10000000u
#define CORTICAL_ACTIVITY_MAX_TIMES    10000u

#define ACTIVITY_LOOP_SECONDS 3.6
#define SCRUB_STEP_FRAMES     4u
#define READOUT_SIZE          192u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct CorticalActivityData
{
    uint32_t time_count;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t hemisphere_vertex_count[2];
    uint32_t hemisphere_index_count[2];
    float display_min;
    float display_mid;
    float display_max;
    float* times_ms;
    vec3* inflated;
    vec3* pial;
    vec3* normals;
    DvzIndex* indices;
    float* values;
} CorticalActivityData;


typedef struct CorticalActivityState
{
    CorticalActivityData data;
    DvzVisual* mesh;
    DvzColormap* colormap;
    DvzOverlayCard* readout;
    DvzColor* colors;
    bool paused;
    uint32_t manual_frame;
    float current_time_ms;
} CorticalActivityState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_cortical_activity_scenario(void);



/*************************************************************************************************/
/*  Binary loading                                                                               */
/*************************************************************************************************/

/**
 * Read one little-endian uint32.
 *
 * @param bytes four input bytes
 * @return decoded value
 */
static uint32_t _u32_le(const uint8_t bytes[4])
{
    ANN(bytes);
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u) | ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[3] << 24u);
}


/**
 * Read one little-endian float32.
 *
 * @param bytes four input bytes
 * @return decoded value
 */
static float _f32_le(const uint8_t bytes[4])
{
    const uint32_t value = _u32_le(bytes);
    float out = 0.0f;
    memcpy(&out, &value, sizeof(out));
    return out;
}


/**
 * Read an exact byte count from a binary stream.
 *
 * @param fp input stream
 * @param data output buffer
 * @param size byte count
 * @return whether the entire payload was read
 */
static bool _read_exact(FILE* fp, void* data, uint64_t size)
{
    ANN(fp);
    ANN(data);
    if (size > SIZE_MAX)
        return false;
    return fread(data, 1, (size_t)size, fp) == (size_t)size;
}


/**
 * Release all arrays owned by a cortical-activity payload.
 *
 * @param data payload
 */
static void _activity_data_destroy(CorticalActivityData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->values);
    dvz_free(data->indices);
    dvz_free(data->normals);
    dvz_free(data->pial);
    dvz_free(data->inflated);
    dvz_free(data->times_ms);
    memset(data, 0, sizeof(*data));
}


/**
 * Load and validate the prepared cortical-activity bundle.
 *
 * @param path binary bundle path
 * @param data output payload
 * @return whether loading succeeded
 */
static bool _activity_data_load(const char* path, CorticalActivityData* data)
{
    ANN(path);
    ANN(data);
    memset(data, 0, sizeof(*data));

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
    {
        dvz_fprintf(
            stderr,
            "cortical_activity: missing prepared data. Run `%s` from the repository "
            "root.\n",
            CORTICAL_ACTIVITY_PREPARE_COMMAND);
        return false;
    }

    bool ok = false;
    uint8_t header[CORTICAL_ACTIVITY_HEADER_SIZE] = {0};
    if (!_read_exact(fp, header, sizeof(header)))
        goto cleanup;
    if (memcmp(header, CORTICAL_ACTIVITY_MAGIC, strlen(CORTICAL_ACTIVITY_MAGIC)) != 0)
        goto cleanup;

    const uint32_t version = _u32_le(&header[8]);
    const uint32_t header_size = _u32_le(&header[12]);
    const uint32_t hemisphere_count = _u32_le(&header[16]);
    data->time_count = _u32_le(&header[20]);
    data->vertex_count = _u32_le(&header[24]);
    data->index_count = _u32_le(&header[28]);
    const uint32_t position_components = _u32_le(&header[32]);
    data->display_min = _f32_le(&header[48]);
    data->display_mid = _f32_le(&header[52]);
    data->display_max = _f32_le(&header[56]);
    data->hemisphere_vertex_count[0] = _u32_le(&header[60]);
    data->hemisphere_index_count[0] = _u32_le(&header[64]);
    data->hemisphere_vertex_count[1] = _u32_le(&header[68]);
    data->hemisphere_index_count[1] = _u32_le(&header[72]);

    if (version != CORTICAL_ACTIVITY_VERSION || header_size != CORTICAL_ACTIVITY_HEADER_SIZE ||
        hemisphere_count != 2u || position_components != 3u || data->time_count < 2u ||
        data->time_count > CORTICAL_ACTIVITY_MAX_TIMES || data->vertex_count == 0u ||
        data->vertex_count > CORTICAL_ACTIVITY_MAX_VERTICES || data->index_count == 0u ||
        data->index_count > CORTICAL_ACTIVITY_MAX_INDICES || data->index_count % 3u != 0u ||
        data->hemisphere_vertex_count[0] + data->hemisphere_vertex_count[1] !=
            data->vertex_count ||
        data->hemisphere_index_count[0] + data->hemisphere_index_count[1] != data->index_count ||
        !(data->display_min < data->display_mid && data->display_mid < data->display_max))
    {
        goto cleanup;
    }

    const uint64_t time_bytes = (uint64_t)data->time_count * sizeof(float);
    const uint64_t vector_bytes = (uint64_t)data->vertex_count * sizeof(vec3);
    const uint64_t index_bytes = (uint64_t)data->index_count * sizeof(DvzIndex);
    const uint64_t value_count = (uint64_t)data->time_count * data->vertex_count;
    if (value_count > SIZE_MAX / sizeof(float))
        goto cleanup;
    const uint64_t value_bytes = value_count * sizeof(float);
    const uint64_t expected_size =
        CORTICAL_ACTIVITY_HEADER_SIZE + time_bytes + 3u * vector_bytes + index_bytes + value_bytes;
    if (expected_size > LONG_MAX || fseek(fp, 0, SEEK_END) != 0 ||
        ftell(fp) != (long)expected_size ||
        fseek(fp, CORTICAL_ACTIVITY_HEADER_SIZE, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    data->times_ms = (float*)dvz_calloc(data->time_count, sizeof(float));
    data->inflated = (vec3*)dvz_calloc(data->vertex_count, sizeof(vec3));
    data->pial = (vec3*)dvz_calloc(data->vertex_count, sizeof(vec3));
    data->normals = (vec3*)dvz_calloc(data->vertex_count, sizeof(vec3));
    data->indices = (DvzIndex*)dvz_calloc(data->index_count, sizeof(DvzIndex));
    data->values = (float*)dvz_calloc(value_count, sizeof(float));
    if (data->times_ms == NULL || data->inflated == NULL || data->pial == NULL ||
        data->normals == NULL || data->indices == NULL || data->values == NULL)
    {
        goto cleanup;
    }
    if (!_read_exact(fp, data->times_ms, time_bytes) ||
        !_read_exact(fp, data->inflated, vector_bytes) ||
        !_read_exact(fp, data->pial, vector_bytes) ||
        !_read_exact(fp, data->normals, vector_bytes) ||
        !_read_exact(fp, data->indices, index_bytes) ||
        !_read_exact(fp, data->values, value_bytes))
    {
        goto cleanup;
    }

    for (uint32_t i = 1; i < data->time_count; i++)
    {
        if (!(data->times_ms[i] > data->times_ms[i - 1u]))
            goto cleanup;
    }
    for (uint32_t i = 0; i < data->index_count; i++)
    {
        if (data->indices[i] >= data->vertex_count)
            goto cleanup;
    }
    ok = true;

cleanup:
    fclose(fp);
    if (!ok)
    {
        dvz_fprintf(stderr, "cortical_activity: invalid prepared bundle `%s`.\n", path);
        _activity_data_destroy(data);
    }
    return ok;
}



/*************************************************************************************************/
/*  Visual mapping                                                                               */
/*************************************************************************************************/

/**
 * Clamp a scalar to the unit interval.
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
 * Mix two 8-bit colors.
 *
 * @param a first color
 * @param b second color
 * @param t interpolation parameter
 * @return interpolated opaque color
 */
static DvzColor _color_mix(DvzColor a, DvzColor b, float t)
{
    t = _clamp01(t);
    const float s = 1.0f - t;
    return dvz_color_rgba(
        (uint8_t)(s * a.r + t * b.r + 0.5f), (uint8_t)(s * a.g + t * b.g + 0.5f),
        (uint8_t)(s * a.b + t * b.b + 0.5f), 255u);
}


/**
 * Update mesh colors and the time readout for one continuous data time.
 *
 * @param state showcase state
 * @param time_ms requested time in milliseconds
 * @return whether visual state was updated
 */
static bool _set_activity_time(CorticalActivityState* state, float time_ms)
{
    if (state == NULL || state->mesh == NULL || state->colors == NULL ||
        state->data.time_count < 2u)
    {
        return false;
    }

    const CorticalActivityData* data = &state->data;
    if (time_ms < data->times_ms[0])
        time_ms = data->times_ms[0];
    if (time_ms > data->times_ms[data->time_count - 1u])
        time_ms = data->times_ms[data->time_count - 1u];

    uint32_t upper = 1u;
    while (upper < data->time_count && data->times_ms[upper] < time_ms)
        upper++;
    if (upper >= data->time_count)
        upper = data->time_count - 1u;
    const uint32_t lower = upper - 1u;
    const float interval = data->times_ms[upper] - data->times_ms[lower];
    const float alpha = interval > 0.0f ? (time_ms - data->times_ms[lower]) / interval : 0.0f;
    const float* values0 = data->values + (uint64_t)lower * data->vertex_count;
    const float* values1 = data->values + (uint64_t)upper * data->vertex_count;
    const DvzColor anatomy = dvz_color_rgb(54u, 61u, 70u);

    for (uint32_t i = 0; i < data->vertex_count; i++)
    {
        const float value = (1.0f - alpha) * values0[i] + alpha * values1[i];
        if (value <= data->display_min)
        {
            state->colors[i] = anatomy;
            continue;
        }

        const double t =
            _clamp01((value - data->display_min) / (data->display_max - data->display_min));
        DvzColor activity = anatomy;
        (void)dvz_colormap_sample(state->colormap, t, &activity);
        const float visibility =
            _clamp01((value - data->display_min) / (data->display_mid - data->display_min));
        state->colors[i] = _color_mix(anatomy, activity, visibility);
    }
    if (dvz_visual_set_data(state->mesh, "color", state->colors, data->vertex_count) != 0)
        return false;

    state->current_time_ms = time_ms;
    if (state->readout != NULL)
    {
        char readout[READOUT_SIZE] = {0};
        dvz_snprintf(
            readout, sizeof(readout),
            "%3.0f ms after left-ear tone%s  |  MEG dSPM  |  OpenNeuro ds000248 v1.2.4", time_ms,
            state->paused ? "  paused" : "");
        (void)dvz_overlay_card_set_text(state->readout, readout);
    }
    return true;
}


/**
 * Create the fixed scientific color scale used by mesh and colorbar.
 *
 * @param scene owning scene
 * @param data activity payload
 * @param out_colormap output scene-owned colormap
 * @return created scale, or NULL
 */
static DvzScale*
_activity_scale(DvzScene* scene, const CorticalActivityData* data, DvzColormap** out_colormap)
{
    ANN(scene);
    ANN(data);
    ANN(out_colormap);

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_MAGMA);
    if (colormap == NULL)
        return NULL;
    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "dSPM",
               });
    if (scale == NULL ||
        dvz_scale_set_domain(scale, data->display_min, data->display_max) != DVZ_OK ||
        dvz_scale_set_view_range(scale, data->display_min, data->display_max) != DVZ_OK ||
        dvz_scale_set_colormap(scale, colormap) != DVZ_OK)
    {
        return NULL;
    }
    *out_colormap = colormap;
    return scale;
}


/**
 * Add the activity colorbar.
 *
 * @param panel target panel
 * @param scale fixed dSPM scale
 * @param data activity payload
 * @return whether the colorbar was created
 */
static bool _add_colorbar(DvzPanel* panel, DvzScale* scale, const CorticalActivityData* data)
{
    ANN(panel);
    ANN(scale);
    ANN(data);

    DvzColorbarDesc desc = dvz_colorbar_desc();
    desc.title = "dSPM";
    desc.reserve_px = 88.0f;
    desc.ramp_width_px = 18.0f;
    desc.edge_offset_px = 20.0f;
    desc.plot_gap_px = 12.0f;
    desc.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzColorbar* colorbar = dvz_colorbar(panel, scale, &desc);
    if (colorbar == NULL)
        return false;

    const double tick_values[] = {data->display_min, data->display_mid, data->display_max};
    DvzColorbarTicks ticks = dvz_colorbar_ticks();
    ticks.count = DVZ_ARRAY_COUNT(tick_values);
    ticks.values = tick_values;
    return dvz_colorbar_set_ticks(colorbar, &ticks) == DVZ_OK;
}


/**
 * Add the dynamic provenance and time readout.
 *
 * @param panel target panel
 * @return created overlay card, or NULL
 */
static DvzOverlayCard* _add_readout(DvzPanel* panel)
{
    ANN(panel);
    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return NULL;

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.background_color = dvz_color_rgba(panel_bg.r, panel_bg.g, panel_bg.b, 224u);
    style.text_color = text;
    style.padding_px[0] = 12.0f;
    style.padding_px[1] = 7.0f;
    style.min_width_px = 620.0f;
    style.height_px = 34.0f;
    style.glyph_advance_px = 7.5f;
    style.text_size_px = 14.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = READOUT_SIZE;

    DvzOverlayCard* card = dvz_overlay_card(
        overlay, &(DvzOverlayCardDesc){
                     DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
                     .text = "Auditory cortical activity",
                     .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT,
                     .offset_px = {30.0f, -42.0f},
                 });
    if (card == NULL || dvz_overlay_card_set_style(card, &style) != DVZ_OK)
        return NULL;
    return card;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the cortical-activity showcase.
 *
 * @param ctx scenario context
 * @param out_user output state
 * @return whether setup succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    *out_user = NULL;

    CorticalActivityState* state =
        (CorticalActivityState*)dvz_calloc(1, sizeof(CorticalActivityState));
    if (state == NULL)
        return false;
    *out_user = state;
    if (!_activity_data_load(CORTICAL_ACTIVITY_PATH, &state->data))
        return false;

    state->colors = (DvzColor*)dvz_calloc(state->data.vertex_count, sizeof(DvzColor));
    if (state->colors == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);
    if (example_set_default_3d_camera(panel, 0.55f) == NULL)
        return false;

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.alpha_to_coverage = false;
    if (dvz_panel_set_msaa(panel, &msaa) != DVZ_OK)
        return false;

    DvzScale* scale = _activity_scale(ctx->scene, &state->data, &state->colormap);
    if (scale == NULL || !_add_colorbar(panel, scale, &state->data))
        return false;

    state->mesh = dvz_mesh(ctx->scene, 0);
    if (state->mesh == NULL)
        return false;
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.35f;
    material.light_direction[1] = +0.55f;
    material.light_direction[2] = +0.75f;
    material.phong.ambient = 0.58f;
    material.phong.diffuse = 0.52f;
    material.phong.specular = 0.04f;
    material.phong.shininess = 12.0f;
    if (dvz_visual_set_material(state->mesh, &material) != 0)
        return false;

    DvzGeometry* geometry = dvz_geometry(state->data.vertex_count, state->data.index_count);
    if (geometry == NULL)
        return false;
    for (uint32_t i = 0; i < state->data.vertex_count; i++)
    {
        geometry->positions[i][0] = state->data.inflated[i][0];
        geometry->positions[i][1] = state->data.inflated[i][1];
        geometry->positions[i][2] = state->data.inflated[i][2];
        geometry->normals[i][0] = state->data.normals[i][0];
        geometry->normals[i][1] = state->data.normals[i][1];
        geometry->normals[i][2] = state->data.normals[i][2];
        geometry->colors[i] = dvz_color_rgb(54u, 61u, 70u);
    }
    memcpy(
        geometry->indices, state->data.indices,
        (uint64_t)state->data.index_count * sizeof(DvzIndex));
    const int mesh_result = dvz_mesh_set_geometry(state->mesh, geometry);
    dvz_geometry_destroy(geometry);
    if (mesh_result != 0 || dvz_panel_add_visual(panel, state->mesh, NULL) != 0 ||
        dvz_scenario_set_primary_visual(ctx, state->mesh) != 0)
    {
        return false;
    }

    state->readout = _add_readout(panel);
    if (state->readout == NULL)
        return false;
    state->manual_frame = 0u;
    const float initial_time_ms = 103.2f;
    if (!_set_activity_time(state, initial_time_ms))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL ||
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
    {
        return false;
    }
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    vec3 angles = {-0.10f, +0.02f, +0.02f};
    dvz_arcball_initial(arcball, angles);
    (void)dvz_arcball_zoom(arcball, 1.0f);
    return true;
}


/**
 * Advance the scientific time animation.
 *
 * @param ctx scenario context
 * @param user showcase state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    CorticalActivityState* state = (CorticalActivityState*)user;
    if (ctx == NULL || state == NULL || state->data.time_count < 2u)
        return;

    if (ctx->preview_mode)
    {
        const double phase =
            dvz_scenario_preview_phase(ctx, DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP);
        const float first = state->data.times_ms[0];
        const float last = state->data.times_ms[state->data.time_count - 1u];
        (void)_set_activity_time(state, first + (float)phase * (last - first));
        return;
    }
    if (state->paused)
    {
        (void)_set_activity_time(state, state->data.times_ms[state->manual_frame]);
        return;
    }
    if (ctx->frame_index == 0u)
        return;

    const double phase = fmod(ctx->time, ACTIVITY_LOOP_SECONDS) / ACTIVITY_LOOP_SECONDS;
    const float first = state->data.times_ms[0];
    const float last = state->data.times_ms[state->data.time_count - 1u];
    (void)_set_activity_time(state, first + (float)phase * (last - first));
}


/**
 * Handle pause and keyboard scrubbing in the live view.
 *
 * @param ctx scenario context
 * @param event routed event
 * @param user showcase state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    (void)ctx;
    CorticalActivityState* state = (CorticalActivityState*)user;
    if (event == NULL || state == NULL || event->kind != DVZ_SCENARIO_EVENT_KEY ||
        event->content.key.type != DVZ_KEYBOARD_EVENT_PRESS)
    {
        return;
    }

    const uint32_t key = event->content.key.key;
    if (key == DVZ_KEY_SPACE)
    {
        state->paused = !state->paused;
        uint32_t nearest = 0u;
        float distance = INFINITY;
        for (uint32_t i = 0; i < state->data.time_count; i++)
        {
            const float d = fabsf(state->data.times_ms[i] - state->current_time_ms);
            if (d < distance)
            {
                distance = d;
                nearest = i;
            }
        }
        state->manual_frame = nearest;
    }
    else if (key == DVZ_KEY_LEFT || key == DVZ_KEY_RIGHT)
    {
        state->paused = true;
        if (key == DVZ_KEY_LEFT)
        {
            state->manual_frame = state->manual_frame > SCRUB_STEP_FRAMES
                                      ? state->manual_frame - SCRUB_STEP_FRAMES
                                      : 0u;
        }
        else
        {
            const uint32_t last = state->data.time_count - 1u;
            state->manual_frame = state->manual_frame + SCRUB_STEP_FRAMES < last
                                      ? state->manual_frame + SCRUB_STEP_FRAMES
                                      : last;
        }
    }
    else
    {
        return;
    }
    (void)_set_activity_time(state, state->data.times_ms[state->manual_frame]);
}


/**
 * Destroy the cortical-activity showcase state.
 *
 * @param ctx scenario context
 * @param user showcase state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    CorticalActivityState* state = (CorticalActivityState*)user;
    if (state == NULL)
        return;
    dvz_free(state->colors);
    _activity_data_destroy(&state->data);
    dvz_free(state);
}


/**
 * Return the cortical-activity showcase scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_cortical_activity_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_cortical_activity",
        .title = "Human Auditory Cortical Activity",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL |
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
 * Run the cortical-activity showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_cortical_activity_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
