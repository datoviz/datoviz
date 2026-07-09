/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* wind_field - This example combines a synthetic wind-speed field with vectors and streamlines.
 *
 * What to look for: the sampled field stores wind speed over a kilometer-scale domain, vectors show
 * local direction and magnitude, streamlines trace the flow, and a fixed probe plus colorbar report
 * speed in m/s. During live playback, compare how the image, vector glyphs, and paths update
 * together from the same procedural wind model.
 *
 * This workflow is useful for geophysical or fluid-like data where scalar magnitude and direction
 * need to be read in one coordinated panel.
 *
 * Scenario: showcases_wind_field
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c showcases/wind_field
 * Run:    ./build/examples/c/showcases/wind_field --live
 * Smoke:  ./build/examples/c/showcases/wind_field --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_showcase_wind_field_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define FIELD_WIDTH  384u
#define FIELD_HEIGHT 240u

#define VECTOR_COLS  43u
#define VECTOR_ROWS  27u
#define VECTOR_COUNT (VECTOR_COLS * VECTOR_ROWS)

#define STREAMLINE_COUNT       76u
#define STREAMLINE_POINT_COUNT 128u
#define STREAMLINE_TOTAL_COUNT (STREAMLINE_COUNT * STREAMLINE_POINT_COUNT)

#define PROBE_SEGMENTS      36u
#define COLORMAP_LUT_SIZE   256u
#define ANIMATION_STRIDE    2u
#define ANIMATION_FPS       60.0f
#define DOMAIN_X_MIN_KM     -620.0f
#define DOMAIN_X_MAX_KM     +620.0f
#define DOMAIN_Y_MIN_KM     -390.0f
#define DOMAIN_Y_MAX_KM     +390.0f
#define PROBE_X_KM          +345.0f
#define PROBE_Y_KM          +14.0f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct WindSample
{
    float u;
    float v;
    float speed;
    float direction_deg;
} WindSample;


typedef struct WindShowcaseParams
{
    float time_scale;
    float speed_max_mps;
    float storm_center_x_km;
    float storm_center_y_km;
    float storm_drift_x_km;
    float storm_drift_y_km;
    float storm_drift_rate_x;
    float storm_drift_rate_y;
    float storm_drift_phase_y;
    float eye_radius_km;
    float vortex_strength_mps;
    float spiral_radius_km;
    float inflow_strength_mps;
    float breathing_amplitude;
    float breathing_rate;
    float background_u_mps;
    float background_u_y_gradient;
    float background_u_wave_mps;
    float background_u_wave_rate;
    float background_u_wave_phase;
    float background_v_mps;
    float background_v_wave_mps;
    float background_v_wave_k;
    float background_v_wave_rate;
    float background_v_wave_phase;
    float shear_strength_mps;
    float shear_wave_k;
    float shear_rate;
    float shear_y_center_km;
    float shear_y_radius_km;
    float cross_wind_strength_mps;
    float cross_wind_wave_k;
    float cross_wind_rate;
    float cross_wind_phase;
    float cross_wind_x_center_km;
    float cross_wind_x_radius_km;
    float terrain_friction;
    float scalar_terrain_mix_mps;
    float vector_scale;
    float vector_alpha_base;
    float vector_alpha_range;
    float vector_width_base_px;
    float vector_width_range_px;
    float streamline_seed_wobble_km;
    float streamline_seed_wobble_rate;
    float streamline_inner_rotation_rate;
    float streamline_inner_radius_km;
    float streamline_inner_radius_jitter_km;
    float streamline_inner_y_scale;
    float streamline_alpha_outer;
    float streamline_alpha_inner;
    float streamline_width_outer_px;
    float streamline_width_inner_px;
    float streamline_step_outer_km;
    float streamline_step_inner_km;
    float streamline_min_speed_mps;
} WindShowcaseParams;


typedef struct WindShowcaseState
{
    DvzPanel* panel;
    DvzScale* scale;
    DvzSampledField* field;
    DvzVisual* vectors;
    DvzVisual* streamlines;
    float* values;
    WindShowcaseParams params;
    ExampleTuner tuner;
    float current_time_s;
} WindShowcaseState;

/* Wind field settings tuned defaults */
static const WindShowcaseParams WIND_PARAMS_SHOWCASE = {
    .time_scale = 1.0f,
    .speed_max_mps = 80.0f,
    .storm_center_x_km = 10.0f,
    .storm_center_y_km = 30.0f,
    .storm_drift_x_km = 40.0f,
    .storm_drift_y_km = 20.0f,
    .storm_drift_rate_x = 0.25f,
    .storm_drift_rate_y = 0.50f,
    .storm_drift_phase_y = 1.8f,
    .eye_radius_km = 125.0f,
    .vortex_strength_mps = 76.0f,
    .spiral_radius_km = 550.0f,
    .inflow_strength_mps = -9.5f,
    .breathing_amplitude = 0.060f,
    .breathing_rate = 0.35f,
    .background_u_mps = 16.0f,
    .background_u_y_gradient = 0.005f,
    .background_u_wave_mps = 0.0f,
    .background_u_wave_rate = 0.40f,
    .background_u_wave_phase = 0.400000f,
    .background_v_mps = -8.0f,
    .background_v_wave_mps = 8.5f,
    .background_v_wave_k = 0.008000f,
    .background_v_wave_rate = 0.10f,
    .background_v_wave_phase = 0.700000f,
    .shear_strength_mps = 4.5f,
    .shear_wave_k = 0.005500f,
    .shear_rate = 0.15f,
    .shear_y_center_km = -125.000000f,
    .shear_y_radius_km = 310.000000f,
    .cross_wind_strength_mps = 4.5f,
    .cross_wind_wave_k = 0.007000f,
    .cross_wind_rate = 0.25f,
    .cross_wind_phase = -0.300000f,
    .cross_wind_x_center_km = -220.000000f,
    .cross_wind_x_radius_km = 520.000000f,
    .terrain_friction = 0.24f,
    .scalar_terrain_mix_mps = 6.5f,
    .vector_scale = 0.50f,
    .vector_alpha_base = 30.0f,
    .vector_alpha_range = 70.0f,
    .vector_width_base_px = 2.0f,
    .vector_width_range_px = 1.8f,
    .streamline_seed_wobble_km = 60.0f,
    .streamline_seed_wobble_rate = 0.50f,
    .streamline_inner_rotation_rate = 0.15f,
    .streamline_inner_radius_km = 106.000000f,
    .streamline_inner_radius_jitter_km = 4.100000f,
    .streamline_inner_y_scale = 0.780000f,
    .streamline_alpha_outer = 118.0f,
    .streamline_alpha_inner = 166.0f,
    .streamline_width_outer_px = 1.6f,
    .streamline_width_inner_px = 1.5f,
    .streamline_step_outer_km = 6.0f,
    .streamline_step_inner_km = 6.0f,
    .streamline_min_speed_mps = 7.0f,
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
 * Convert a float alpha value to an 8-bit channel.
 *
 * @param value alpha in [0, 255]
 * @return clamped 8-bit channel
 */
static uint8_t _alpha_u8(float value)
{
    if (value < 0.0f)
        return 0u;
    if (value > 255.0f)
        return 255u;
    return (uint8_t)(value + 0.5f);
}



/**
 * Snap a float to a display-oriented step.
 *
 * @param value input value
 * @param min lower bound
 * @param max upper bound
 * @param step snap increment
 * @return snapped and clamped value
 */
static float _snap_float_step(float value, float min, float max, float step)
{
    if (step > 0.0f)
        value = roundf(value / step) * step;
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}



/**
 * Show a wind-parameter slider with a rounded display increment.
 *
 * @param gui GUI
 * @param label slider label
 * @param value edited value
 * @param min lower bound
 * @param max upper bound
 * @param step snap increment
 * @param format display format
 * @return whether the value changed
 */
static bool _wind_gui_slider(
    DvzGui* gui, const char* label, float* value, float min, float max, float step,
    const char* format)
{
    if (!dvz_gui_slider_float_format(gui, label, value, min, max, format))
        return false;
    *value = _snap_float_step(*value, min, max, step);
    return true;
}



/**
 * Linearly interpolate two floats.
 *
 * @param a first value
 * @param b second value
 * @param t interpolation parameter
 * @return interpolated value
 */
static float _mix(float a, float b, float t)
{
    return a + (b - a) * t;
}



/**
 * Return a smooth pseudo terrain mask in the weather domain.
 *
 * The mask is not cartographic data; it gives the synthetic wind field a darker land/sea-like
 * structure and a weak friction term so arrows do not look like pure mathematical noise.
 *
 * @param x domain X coordinate in km
 * @param y domain Y coordinate in km
 * @return terrain mask in [0, 1]
 */
static float _terrain_mask(float x, float y)
{
    const float n0 = sinf(0.0062f * x + 0.0028f * y);
    const float n1 = sinf(0.0110f * x - 0.0075f * y + 1.8f);
    const float ridge = 0.45f * n0 + 0.30f * n1 + 0.20f * sinf(0.0048f * (x + 1.7f * y));
    return _clamp01(0.48f + 0.55f * ridge);
}



/**
 * Sample the synthetic weather wind field.
 *
 * @param x domain X coordinate in km
 * @param y domain Y coordinate in km
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 * @return wind sample with vector, speed, and direction
 */
static WindSample
_wind_sample(float x, float y, const WindShowcaseParams* params, float time_s)
{
    ANN(params);

    const float center_x =
        params->storm_center_x_km + params->storm_drift_x_km *
                                       sinf(params->storm_drift_rate_x * time_s);
    const float center_y =
        params->storm_center_y_km + params->storm_drift_y_km *
                                       cosf(params->storm_drift_rate_y * time_s +
                                            params->storm_drift_phase_y);
    const float dx = x - center_x;
    const float dy = y - center_y;
    const float r = sqrtf(dx * dx + dy * dy) + 1e-3f;

    const float eye_radius = fmaxf(params->eye_radius_km, 1e-3f);
    const float rr = r / eye_radius;
    const float breathing =
        1.0f + params->breathing_amplitude * sinf(params->breathing_rate * time_s);
    const float vortex =
        params->vortex_strength_mps * breathing * rr * expf(0.5f * (1.0f - rr * rr));
    const float spiral_radius = fmaxf(params->spiral_radius_km, 1e-3f);
    const float spiral = expf(-(r * r) / (2.0f * spiral_radius * spiral_radius));
    const float inflow = params->inflow_strength_mps * spiral;
    const float terrain = _terrain_mask(x, y);

    float u = params->background_u_mps + params->background_u_y_gradient * y +
              params->background_u_wave_mps *
                  sinf(params->background_u_wave_rate * time_s +
                       params->background_u_wave_phase);
    float v = params->background_v_mps +
              params->background_v_wave_mps *
                  sinf(
                      params->background_v_wave_k * x + params->background_v_wave_phase +
                      params->background_v_wave_rate * time_s);

    u += -vortex * dy / r + inflow * dx / r;
    v += +vortex * dx / r + inflow * dy / r;

    const float shear = params->shear_strength_mps *
                        sinf(params->shear_wave_k * (x - 0.8f * y) +
                             params->shear_rate * time_s);
    const float shear_dy = y - params->shear_y_center_km;
    const float shear_radius = fmaxf(params->shear_y_radius_km, 1e-3f);
    u += shear * expf(-(shear_dy * shear_dy) / (2.0f * shear_radius * shear_radius));
    const float cross_dx = x - params->cross_wind_x_center_km;
    const float cross_radius = fmaxf(params->cross_wind_x_radius_km, 1e-3f);
    v += params->cross_wind_strength_mps *
         sinf(
             params->cross_wind_wave_k * y + params->cross_wind_phase +
             params->cross_wind_rate * time_s) *
         expf(-(cross_dx * cross_dx) / (2.0f * cross_radius * cross_radius));

    const float friction = 1.0f - params->terrain_friction * terrain;
    u *= friction;
    v *= friction;

    const float speed = sqrtf(u * u + v * v);
    float dir = atan2f(u, v) * 180.0f / 3.14159265359f;
    if (dir < 0.0f)
        dir += 360.0f;

    return (WindSample){.u = u, .v = v, .speed = speed, .direction_deg = dir};
}



/**
 * Map one wind speed to the showcase colormap.
 *
 * @param speed_mps wind speed in m/s
 * @param params procedural model parameters
 * @return output RGBA8 color
 */
static DvzColor _wind_colormap(float speed_mps, const WindShowcaseParams* params)
{
    ANN(params);

    const float t = _clamp01(speed_mps / fmaxf(params->speed_max_mps, 1e-3f));
    DvzColor c0 = dvz_color_rgb(14, 17, 23);
    DvzColor c1 = dvz_color_rgb(23, 65, 92);
    DvzColor c2 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor c3 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzColor c4 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    DvzColor c5 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR);

    DvzColor a = c0;
    DvzColor b = c1;
    float local = t / 0.24f;
    if (t >= 0.24f && t < 0.52f)
    {
        a = c1;
        b = c2;
        local = (t - 0.24f) / 0.28f;
    }
    else if (t >= 0.52f && t < 0.72f)
    {
        a = c2;
        b = c3;
        local = (t - 0.52f) / 0.20f;
    }
    else if (t >= 0.72f && t < 0.90f)
    {
        a = c3;
        b = c4;
        local = (t - 0.72f) / 0.18f;
    }
    else if (t >= 0.90f)
    {
        a = c4;
        b = c5;
        local = (t - 0.90f) / 0.10f;
    }

    local = _clamp01(local);
    return dvz_color_rgba(
        _u8(_mix((float)a.r / 255.0f, (float)b.r / 255.0f, local)),
        _u8(_mix((float)a.g / 255.0f, (float)b.g / 255.0f, local)),
        _u8(_mix((float)a.b / 255.0f, (float)b.b / 255.0f, local)), 255u);
}



/**
 * Map one wind speed to a constrained cyan/mint/amber overlay color.
 *
 * @param speed_mps wind speed in m/s
 * @param alpha output alpha
 * @param midpoint normalized speed where mint is reached
 * @param gamma nonlinear contrast factor applied to normalized speed
 * @param params procedural model parameters
 * @return output RGBA8 color
 */
static DvzColor
_wind_flow_color(
    float speed_mps, uint8_t alpha, float midpoint, float gamma,
    const WindShowcaseParams* params)
{
    ANN(params);

    const float normalized = _clamp01(speed_mps / fmaxf(params->speed_max_mps, 1e-3f));
    const float t = powf(normalized, gamma);
    DvzColor a = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor b = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    float local = t / midpoint;
    if (t >= midpoint)
    {
        a = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        b = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
        local = (t - midpoint) / (1.0f - midpoint);
    }

    local = _clamp01(local);
    return dvz_color_rgba(
        _u8(_mix((float)a.r / 255.0f, (float)b.r / 255.0f, local)),
        _u8(_mix((float)a.g / 255.0f, (float)b.g / 255.0f, local)),
        _u8(_mix((float)a.b / 255.0f, (float)b.b / 255.0f, local)), alpha);
}



/**
 * Map one wind speed to an arrow color.
 *
 * @param speed_mps wind speed in m/s
 * @param alpha output alpha
 * @param params procedural model parameters
 * @return output RGBA8 color
 */
static DvzColor
_wind_arrow_color(float speed_mps, uint8_t alpha, const WindShowcaseParams* params)
{
    return _wind_flow_color(speed_mps, alpha, 0.44f, 0.78f, params);
}



/**
 * Map one wind speed to a streamline color.
 *
 * @param speed_mps wind speed in m/s
 * @param alpha output alpha
 * @param params procedural model parameters
 * @return output RGBA8 color
 */
static DvzColor
_wind_streamline_color(float speed_mps, uint8_t alpha, const WindShowcaseParams* params)
{
    return _wind_flow_color(speed_mps, alpha, 0.36f, 0.64f, params);
}



/**
 * Fill the custom wind-speed colormap LUT.
 *
 * @param colors output RGBA8 LUT
 * @param params procedural model parameters
 */
static void
_fill_wind_colormap(DvzColor colors[COLORMAP_LUT_SIZE], const WindShowcaseParams* params)
{
    ANN(colors);
    ANN(params);

    for (uint32_t i = 0; i < COLORMAP_LUT_SIZE; i++)
    {
        const float t =
            COLORMAP_LUT_SIZE > 1u ? (float)i / (float)(COLORMAP_LUT_SIZE - 1u) : 0.0f;
        colors[i] = _wind_colormap(t * params->speed_max_mps, params);
    }
}



/**
 * Fill the scalar wind-speed field.
 *
 * @param values output scalar field values in m/s
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 */
static void _fill_scalar_field(float* values, const WindShowcaseParams* params, float time_s)
{
    ANN(values);
    ANN(params);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        const float fy = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
        const float data_y = _mix(DOMAIN_Y_MIN_KM, DOMAIN_Y_MAX_KM, fy);
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float fx = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float data_x = _mix(DOMAIN_X_MIN_KM, DOMAIN_X_MAX_KM, fx);
            WindSample sample = _wind_sample(data_x, data_y, params, time_s);
            const float terrain = _terrain_mask(data_x, data_y);
            values[y * FIELD_WIDTH + x] =
                _clamp01(
                    (sample.speed + params->scalar_terrain_mix_mps * terrain) /
                    fmaxf(params->speed_max_mps, 1e-3f)) *
                params->speed_max_mps;
        }
    }
}



/**
 * Copy data-space positions while assigning one Z coordinate.
 *
 * @param data input data positions
 * @param out output data positions
 * @param count number of positions
 * @param z data Z coordinate to assign
 */
static void _copy_positions_with_z(const float* data, float* out, uint32_t count, float z)
{
    ANN(data);
    ANN(out);

    for (uint32_t i = 0; i < count; i++)
    {
        out[3 * i + 0] = data[3 * i + 0];
        out[3 * i + 1] = data[3 * i + 1];
        out[3 * i + 2] = z;
    }
}



/**
 * Return a DATA-space visual attachment descriptor for one draw layer.
 *
 * @param z_layer layer used for draw ordering
 * @return visual attachment descriptor
 */
static DvzVisualAttachDesc _wind_attach(int32_t z_layer)
{
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = z_layer;
    return attach;
}



/**
 * Add the scalar wind-speed image visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param scale scale bound to the scalar image
 * @param values scalar field values
 * @param out_field output sampled field
 * @return true when the image was added
 */
static bool _add_wind_image(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, float* values,
    DvzSampledField** out_field)
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(values);
    ANN(out_field);

    vec3 data_positions[4] = {
        {DOMAIN_X_MIN_KM, DOMAIN_Y_MIN_KM, 0.0f},
        {DOMAIN_X_MIN_KM, DOMAIN_Y_MAX_KM, 0.0f},
        {DOMAIN_X_MAX_KM, DOMAIN_Y_MIN_KM, 0.0f},
        {DOMAIN_X_MAX_KM, DOMAIN_Y_MAX_KM, 0.0f},
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
    if (dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = values,
                       .bytes_per_row = FIELD_WIDTH * sizeof(float),
                       .rows_per_image = FIELD_HEIGHT,
                   }) != DVZ_OK)
    {
        return false;
    }
    if (dvz_visual_set_field(image, "field", field) != DVZ_OK)
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    DvzVisualAttachDesc attach = _wind_attach(0);
    if (dvz_panel_add_visual(panel, image, &attach) != 0)
        return false;
    *out_field = field;
    return true;
}



/**
 * Fill straight wind-vector visual data.
 *
 * @param positions output vector positions in data coordinates
 * @param vectors output vector displacements in data coordinates
 * @param colors output vector colors
 * @param widths output vector stroke widths
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _fill_vectors(
    vec3* positions, vec3* vectors, DvzColor* colors, float* widths,
    const WindShowcaseParams* params, float time_s)
{
    ANN(positions);
    ANN(vectors);
    ANN(colors);
    ANN(widths);
    ANN(params);

    uint32_t idx = 0;
    const float step_x = (DOMAIN_X_MAX_KM - DOMAIN_X_MIN_KM) / (float)(VECTOR_COLS - 1u);
    const float step_y = (DOMAIN_Y_MAX_KM - DOMAIN_Y_MIN_KM) / (float)(VECTOR_ROWS - 1u);
    for (uint32_t row = 0; row < VECTOR_ROWS; row++)
    {
        for (uint32_t col = 0; col < VECTOR_COLS; col++)
        {
            const float x = DOMAIN_X_MIN_KM + (float)col * step_x;
            const float y = DOMAIN_Y_MIN_KM + (float)row * step_y;
            WindSample sample = _wind_sample(x, y, params, time_s);

            vec3 data_start[1] = {{x, y, 0.0f}};
            vec3 data_end[1] = {
                {x + params->vector_scale * sample.u, y + params->vector_scale * sample.v, 0.0f}};
            data_start[0][2] = 0.03f;
            data_end[0][2] = 0.03f;

            positions[idx][0] = data_start[0][0];
            positions[idx][1] = data_start[0][1];
            positions[idx][2] = data_start[0][2];
            vectors[idx][0] = data_end[0][0] - data_start[0][0];
            vectors[idx][1] = data_end[0][1] - data_start[0][1];
            vectors[idx][2] = 0.0f;

            colors[idx] = _wind_arrow_color(
                sample.speed,
                _alpha_u8(
                    params->vector_alpha_base +
                    params->vector_alpha_range *
                        _clamp01(sample.speed / fmaxf(params->vortex_strength_mps, 1e-3f))),
                params);
            widths[idx] =
                params->vector_width_base_px +
                params->vector_width_range_px *
                    _clamp01(sample.speed / fmaxf(params->speed_max_mps, 1e-3f));
            idx++;
        }
    }
    return true;
}



/**
 * Add the retained vector field.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_visual output vector visual
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _add_vectors(
    DvzScene* scene, DvzPanel* panel, DvzVisual** out_visual,
    const WindShowcaseParams* params, float time_s)
{
    ANN(scene);
    ANN(panel);
    ANN(out_visual);
    ANN(params);

    vec3* positions = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*positions));
    vec3* vectors = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*vectors));
    DvzColor* colors = (DvzColor*)dvz_calloc(VECTOR_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(VECTOR_COUNT, sizeof(*widths));
    if (positions == NULL || vectors == NULL || colors == NULL || widths == NULL)
        goto error;
    if (!_fill_vectors(positions, vectors, colors, widths, params, time_s))
        goto error;

    DvzVisual* visual = dvz_vector(scene, 0);
    if (visual == NULL)
        goto error;
    DvzVectorStyle style = dvz_vector_style();
    style.end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    if (dvz_vector_set_style(visual, &style) != 0)
        goto error;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VECTOR_COUNT},
        {.attr_name = "vector", .data = vectors, .item_count = VECTOR_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VECTOR_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = VECTOR_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        goto error;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        goto error;
    DvzVisualAttachDesc attach = _wind_attach(2);
    if (dvz_panel_add_visual(panel, visual, &attach) != 0)
        goto error;

    *out_visual = visual;
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(vectors);
    dvz_free(positions);
    return true;

error:
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(vectors);
    dvz_free(positions);
    return false;
}



/**
 * Fill streamline path data by integrating through the same wind field.
 *
 * @param positions output path positions in data coordinates
 * @param colors output path colors
 * @param widths output stroke widths
 * @param subpaths output subpath lengths
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool
_fill_streamlines(
    vec3* positions, DvzColor* colors, float* widths, uint32_t* subpaths,
    const WindShowcaseParams* params, float time_s)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);
    ANN(params);

    for (uint32_t line = 0; line < STREAMLINE_COUNT; line++)
    {
        subpaths[line] = STREAMLINE_POINT_COUNT;
        const float band = (float)line / (float)(STREAMLINE_COUNT - 1u);
        const float upper_band = powf(band, 0.68f);
        float x = DOMAIN_X_MIN_KM + 78.0f + 54.0f * (float)(line % 8u) +
                  params->streamline_seed_wobble_km *
                      sinf(params->streamline_seed_wobble_rate * time_s + 3.1f * band);
        float y = _mix(DOMAIN_Y_MIN_KM + 132.0f, DOMAIN_Y_MAX_KM - 42.0f, upper_band);
        if (line >= STREAMLINE_COUNT / 2u)
        {
            const uint32_t inner = line - STREAMLINE_COUNT / 2u;
            const float a =
                TAU * (float)inner / (float)(STREAMLINE_COUNT / 2u) +
                params->streamline_inner_rotation_rate * time_s;
            const float radius =
                params->streamline_inner_radius_km +
                params->streamline_inner_radius_jitter_km * (float)(inner % 7u);
            x = params->storm_center_x_km + radius * cosf(a);
            y = params->storm_center_y_km + (params->streamline_inner_y_scale * radius) * sinf(a);
        }

        bool active = true;
        vec3 held_position = {0};
        DvzColor held_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        held_color.a = 0u;
        for (uint32_t point = 0; point < STREAMLINE_POINT_COUNT; point++)
        {
            const uint32_t idx = line * STREAMLINE_POINT_COUNT + point;

            if (!active)
            {
                positions[idx][0] = held_position[0];
                positions[idx][1] = held_position[1];
                positions[idx][2] = held_position[2];
                colors[idx] = held_color;
                widths[idx] = 0.0f;
                continue;
            }

            vec3 data[1] = {{x, y, 0.0f}};
            data[0][2] = 0.02f;
            positions[idx][0] = data[0][0];
            positions[idx][1] = data[0][1];
            positions[idx][2] = data[0][2];
            held_position[0] = data[0][0];
            held_position[1] = data[0][1];
            held_position[2] = data[0][2];

            WindSample sample = _wind_sample(x, y, params, time_s);
            colors[idx] = _wind_streamline_color(
                sample.speed,
                _alpha_u8(
                    line < STREAMLINE_COUNT / 2u ? params->streamline_alpha_outer
                                                  : params->streamline_alpha_inner),
                params);
            held_color = colors[idx];
            held_color.a = 0u;
            widths[idx] = line < STREAMLINE_COUNT / 2u ? params->streamline_width_outer_px
                                                       : params->streamline_width_inner_px;

            const float norm = fmaxf(sample.speed, params->streamline_min_speed_mps);
            const float step = line < STREAMLINE_COUNT / 2u ? params->streamline_step_outer_km
                                                            : params->streamline_step_inner_km;
            x += step * sample.u / norm;
            y += step * sample.v / norm;
            if (x < DOMAIN_X_MIN_KM || x > DOMAIN_X_MAX_KM || y < DOMAIN_Y_MIN_KM ||
                y > DOMAIN_Y_MAX_KM)
            {
                active = false;
            }
        }
    }
    return true;
}



/**
 * Add streamlines as a subdued path overlay.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_visual output path visual
 * @param params procedural model parameters
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool
_add_streamlines(
    DvzScene* scene, DvzPanel* panel, DvzVisual** out_visual,
    const WindShowcaseParams* params, float time_s)
{
    ANN(scene);
    ANN(panel);
    ANN(out_visual);
    ANN(params);

    vec3* positions = (vec3*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*widths));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(STREAMLINE_COUNT, sizeof(*subpaths));
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
        goto error;
    if (!_fill_streamlines(positions, colors, widths, subpaths, params, time_s))
        goto error;

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = STREAMLINE_TOTAL_COUNT},
        {.attr_name = "color", .data = colors, .item_count = STREAMLINE_TOTAL_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = STREAMLINE_TOTAL_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        goto error;
    if (dvz_path_set_subpaths(path, STREAMLINE_COUNT, subpaths) != 0)
        goto error;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        goto error;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        goto error;
    if (dvz_visual_set_depth_test(path, false) != 0)
        goto error;
    DvzVisualAttachDesc attach = _wind_attach(1);
    if (dvz_panel_add_visual(panel, path, &attach) != 0)
        goto error;

    *out_visual = path;
    dvz_free(subpaths);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(positions);
    return true;

error:
    dvz_free(subpaths);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(positions);
    return false;
}



/**
 * Add a fixed probe crosshair and readout card.
 *
 * @param scene scene owning marker visuals
 * @param panel panel receiving overlays
 * @param params procedural model parameters
 * @return true on success
 */
static bool _add_probe(DvzScene* scene, DvzPanel* panel, const WindShowcaseParams* params)
{
    ANN(scene);
    ANN(panel);
    ANN(params);

    vec3 data_starts[PROBE_SEGMENTS] = {{0}};
    vec3 data_ends[PROBE_SEGMENTS] = {{0}};
    vec3 starts[PROBE_SEGMENTS] = {{0}};
    vec3 ends[PROBE_SEGMENTS] = {{0}};
    DvzColor colors[PROBE_SEGMENTS] = {{0}};
    float widths[PROBE_SEGMENTS] = {0};
    const float radius_x = 18.0f;
    const float radius_y = 14.0f;
    const DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);

    for (uint32_t i = 0; i < PROBE_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)PROBE_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)PROBE_SEGMENTS;
        data_starts[i][0] = PROBE_X_KM + radius_x * cosf(a0);
        data_starts[i][1] = PROBE_Y_KM + radius_y * sinf(a0);
        data_ends[i][0] = PROBE_X_KM + radius_x * cosf(a1);
        data_ends[i][1] = PROBE_Y_KM + radius_y * sinf(a1);
        colors[i] = cyan;
        colors[i].a = 235u;
        widths[i] = 2.0f;
    }

    _copy_positions_with_z((const float*)data_starts, (float*)starts, PROBE_SEGMENTS, 0.05f);
    _copy_positions_with_z((const float*)data_ends, (float*)ends, PROBE_SEGMENTS, 0.05f);

    DvzVisual* ring = dvz_segment(scene, 0);
    if (ring == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = PROBE_SEGMENTS},
        {.attr_name = "position_end", .data = ends, .item_count = PROBE_SEGMENTS},
        {.attr_name = "color", .data = colors, .item_count = PROBE_SEGMENTS},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PROBE_SEGMENTS},
    };
    if (dvz_visual_set_data_many(ring, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(ring, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_visual_set_depth_test(ring, false) != 0)
        return false;
    DvzVisualAttachDesc ring_attach = _wind_attach(3);
    if (dvz_panel_add_visual(panel, ring, &ring_attach) != 0)
        return false;

    vec3 data_dot[1] = {{PROBE_X_KM, PROBE_Y_KM, 0.0f}};
    vec3 dot_position[1] = {{0}};
    _copy_positions_with_z((const float*)data_dot, (float*)dot_position, 1, 0.06f);
    DvzVisual* dot = dvz_point(scene, 0);
    if (dot == NULL)
        return false;
    DvzColor dot_color[1] = {cyan};
    dot_color[0].a = 245u;
    float diameter_px[1] = {7.0f};
    DvzVisualDataUpdate dot_updates[] = {
        {.attr_name = "position", .data = dot_position, .item_count = 1},
        {.attr_name = "color", .data = dot_color, .item_count = 1},
        {.attr_name = "diameter_px", .data = diameter_px, .item_count = 1},
    };
    if (dvz_visual_set_data_many(dot, dot_updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(dot, false) != 0)
        return false;
    DvzVisualAttachDesc dot_attach = _wind_attach(4);
    if (dvz_panel_add_visual(panel, dot, &dot_attach) != 0)
        return false;

    WindSample sample = _wind_sample(PROBE_X_KM, PROBE_Y_KM, params, 0.0f);
    char readout[96] = {0};
    snprintf(
        readout, sizeof(readout), "Wind Speed  %.1f m/s    Dir  %.0f deg", sample.speed,
        sample.direction_deg);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return false;
    DvzOverlayCardStyle card_style = dvz_overlay_card_style();
    DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    card_style.background_color = dvz_color_rgba(panel_bg.r, panel_bg.g, panel_bg.b, 226u);
    card_style.text_color = text;
    card_style.padding_px[0] = 12.0f;
    card_style.padding_px[1] = 7.0f;
    card_style.min_width_px = 322.0f;
    card_style.height_px = 32.0f;
    card_style.glyph_advance_px = 7.5f;
    card_style.text_size_px = 14.0f;
    card_style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    card_style.max_text_chars = 96u;

    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = readout,
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT,
            .offset_px = {-112.0f, -46.0f},
        });
    return card != NULL && dvz_overlay_card_set_style(card, &card_style) == 0;
}



/**
 * Create the shared wind-speed color scale.
 *
 * @param scene scene owning scale resources
 * @param params procedural model parameters
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_wind_scale(DvzScene* scene, const WindShowcaseParams* params)
{
    ANN(scene);
    ANN(params);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "wind speed",
                   .unit = "m/s",
               });
    if (scale == NULL)
        return NULL;
    dvz_scale_set_format(
        scale, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                   .precision = 0,
                   .trim_trailing_zeros = true});
    dvz_scale_set_domain(scale, 0.0, params->speed_max_mps);
    dvz_scale_set_view_range(scale, 0.0, params->speed_max_mps);

    DvzColor colors[COLORMAP_LUT_SIZE] = {0};
    _fill_wind_colormap(colors, params);
    DvzColormap* colormap =
        dvz_colormap_custom(scene, "showcase_wind_speed", colors, COLORMAP_LUT_SIZE);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add the wind-speed colorbar.
 *
 * @param panel panel receiving the colorbar
 * @param scale scale bound to the colorbar
 * @return created colorbar, or NULL on failure
 */
static DvzColorbar* _add_wind_colorbar(DvzPanel* panel, DvzScale* scale)
{
    ANN(panel);
    ANN(scale);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_LEFT,
            .title = "m/s",
            .reserve_px = 66.0f,
            .ramp_width_px = 24.0f,
            .plot_gap_px = 10.0f,
            .tick_length_px = 5.0f,
            .label_gap_px = 4.0f,
            .text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        });
    if (colorbar != NULL)
    {
        dvz_colorbar_set_format(
            colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 0, .trim_trailing_zeros = true});
    }
    return colorbar;
}


/**
 * Update the scalar sampled field with the animated wind speed.
 *
 * @param state showcase animation state
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _update_wind_image(WindShowcaseState* state, float time_s)
{
    if (state == NULL || state->field == NULL || state->values == NULL)
        return false;

    _fill_scalar_field(state->values, &state->params, time_s);
    return dvz_sampled_field_update_region(
               state->field,
               (DvzFieldRegion){
                   .x = 0,
                   .y = 0,
                   .z = 0,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               },
               &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = state->values,
                   .bytes_per_row = FIELD_WIDTH * sizeof(float),
                   .rows_per_image = FIELD_HEIGHT,
               }) == DVZ_OK;
}



/**
 * Update the vector overlay from the animated wind field.
 *
 * @param state showcase animation state
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _update_vectors(WindShowcaseState* state, float time_s)
{
    if (state == NULL || state->panel == NULL || state->vectors == NULL)
        return false;

    vec3* positions = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*positions));
    vec3* vectors = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*vectors));
    DvzColor* colors = (DvzColor*)dvz_calloc(VECTOR_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(VECTOR_COUNT, sizeof(*widths));
    bool ok = false;
    if (positions == NULL || vectors == NULL || colors == NULL || widths == NULL)
        goto cleanup;
    if (!_fill_vectors(positions, vectors, colors, widths, &state->params, time_s))
        goto cleanup;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VECTOR_COUNT},
        {.attr_name = "vector", .data = vectors, .item_count = VECTOR_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VECTOR_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = VECTOR_COUNT},
    };
    ok = dvz_visual_set_data_many(state->vectors, updates, 4) == 0;

cleanup:
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(vectors);
    dvz_free(positions);
    return ok;
}



/**
 * Update the streamline overlay from the animated wind field.
 *
 * @param state showcase animation state
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _update_streamlines(WindShowcaseState* state, float time_s)
{
    if (state == NULL || state->panel == NULL || state->streamlines == NULL)
        return false;

    vec3* positions = (vec3*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*widths));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(STREAMLINE_COUNT, sizeof(*subpaths));
    bool ok = false;
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
        goto cleanup;
    if (!_fill_streamlines(positions, colors, widths, subpaths, &state->params, time_s))
        goto cleanup;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = STREAMLINE_TOTAL_COUNT},
        {.attr_name = "color", .data = colors, .item_count = STREAMLINE_TOTAL_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = STREAMLINE_TOTAL_COUNT},
    };
    ok = dvz_visual_set_data_many(state->streamlines, updates, 3) == 0;

cleanup:
    dvz_free(subpaths);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(positions);
    return ok;
}



/**
 * Advance the deterministic weather animation.
 *
 * @param ctx scenario context
 * @param user_data showcase animation state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user_data)
{
    WindShowcaseState* state = (WindShowcaseState*)user_data;
    if (ctx == NULL || state == NULL)
        return;

    const uint32_t frame_index =
        ctx->preview_mode ? (uint32_t)ctx->preview_frame_index + 1u
                          : (uint32_t)ctx->frame_index + 1u;
    const uint32_t update_stride = ctx->preview_mode ? 1u : ANIMATION_STRIDE;
    if (frame_index % update_stride != 0)
        return;

    const float time_s =
        ctx->preview_mode ? (float)dvz_scenario_preview_time(ctx)
                          : (float)frame_index / ANIMATION_FPS;
    state->current_time_s = time_s;
    const float model_time_s = state->current_time_s * state->params.time_scale;
    if (!_update_wind_image(state, model_time_s))
        return;
    if (!_update_streamlines(state, model_time_s))
        return;
    (void)_update_vectors(state, model_time_s);
}


/**
 * Apply current wind parameter edits to retained visuals.
 *
 * @param user wind showcase state
 */
static void _wind_params_apply(void* user)
{
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (state == NULL)
        return;

    if (state->scale != NULL)
    {
        dvz_scale_set_domain(state->scale, 0.0, state->params.speed_max_mps);
        dvz_scale_set_view_range(state->scale, 0.0, state->params.speed_max_mps);
    }
    const float model_time_s = state->current_time_s * state->params.time_scale;
    if (!_update_wind_image(state, model_time_s))
        return;
    if (!_update_streamlines(state, model_time_s))
        return;
    (void)_update_vectors(state, model_time_s);
}


/**
 * Reset editable wind parameters to the showcase defaults.
 *
 * @param user wind showcase state
 */
static void _wind_params_reset(void* user)
{
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (state == NULL)
        return;
    state->params = WIND_PARAMS_SHOWCASE;
    _wind_params_apply(user);
}


/**
 * Draw wind model controls in the example tuner.
 *
 * @param gui GUI
 * @param user wind showcase state
 * @return whether parameters changed
 */
static bool _wind_params_gui(DvzGui* gui, void* user)
{
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (gui == NULL || state == NULL)
        return false;

    bool changed = false;
    WindShowcaseParams* p = &state->params;

    dvz_gui_separator_text(gui, "Time and scale");
    changed |= _wind_gui_slider(gui, "Time scale", &p->time_scale, 0.1f, 8.0f, 0.1f, "%.1f");
    changed |= _wind_gui_slider(gui, "Speed max", &p->speed_max_mps, 30.0f, 140.0f, 1.0f, "%.0f");

    dvz_gui_separator_text(gui, "Storm");
    changed |= _wind_gui_slider(
        gui, "Center X", &p->storm_center_x_km, DOMAIN_X_MIN_KM, DOMAIN_X_MAX_KM, 5.0f,
        "%.0f");
    changed |= _wind_gui_slider(
        gui, "Center Y", &p->storm_center_y_km, DOMAIN_Y_MIN_KM, DOMAIN_Y_MAX_KM, 5.0f,
        "%.0f");
    changed |= _wind_gui_slider(gui, "Drift X", &p->storm_drift_x_km, 0.0f, 140.0f, 5.0f, "%.0f");
    changed |= _wind_gui_slider(gui, "Drift Y", &p->storm_drift_y_km, 0.0f, 100.0f, 5.0f, "%.0f");
    changed |=
        _wind_gui_slider(gui, "Drift rate X", &p->storm_drift_rate_x, 0.0f, 1.0f, 0.05f, "%.2f");
    changed |=
        _wind_gui_slider(gui, "Drift rate Y", &p->storm_drift_rate_y, 0.0f, 1.0f, 0.05f, "%.2f");
    changed |=
        _wind_gui_slider(gui, "Drift phase Y", &p->storm_drift_phase_y, 0.0f, TAU, 0.1f, "%.1f");
    changed |=
        _wind_gui_slider(gui, "Eye radius", &p->eye_radius_km, 30.0f, 260.0f, 5.0f, "%.0f");
    changed |=
        _wind_gui_slider(gui, "Vortex", &p->vortex_strength_mps, 10.0f, 150.0f, 1.0f, "%.0f");
    changed |= _wind_gui_slider(
        gui, "Spiral radius", &p->spiral_radius_km, 80.0f, 700.0f, 10.0f, "%.0f");
    changed |= _wind_gui_slider(gui, "Inflow", &p->inflow_strength_mps, -35.0f, 5.0f, 0.5f, "%.1f");
    changed |=
        _wind_gui_slider(gui, "Breathing", &p->breathing_amplitude, 0.0f, 0.35f, 0.005f, "%.3f");
    changed |= _wind_gui_slider(gui, "Breathing rate", &p->breathing_rate, 0.0f, 1.5f, 0.05f, "%.2f");

    dvz_gui_separator_text(gui, "Background");
    changed |= _wind_gui_slider(gui, "Base U", &p->background_u_mps, -20.0f, 45.0f, 0.5f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "U/Y gradient", &p->background_u_y_gradient, -0.05f, 0.05f, 0.005f,
        "%.3f");
    changed |= _wind_gui_slider(gui, "U wave", &p->background_u_wave_mps, 0.0f, 12.0f, 0.5f, "%.1f");
    changed |=
        _wind_gui_slider(gui, "U wave rate", &p->background_u_wave_rate, 0.0f, 1.5f, 0.05f, "%.2f");
    changed |= _wind_gui_slider(gui, "Base V", &p->background_v_mps, -30.0f, 30.0f, 0.5f, "%.1f");
    changed |= _wind_gui_slider(gui, "V wave", &p->background_v_wave_mps, 0.0f, 16.0f, 0.5f, "%.1f");
    changed |=
        _wind_gui_slider(gui, "V wave rate", &p->background_v_wave_rate, 0.0f, 1.5f, 0.05f, "%.2f");
    changed |= _wind_gui_slider(gui, "Shear", &p->shear_strength_mps, 0.0f, 30.0f, 0.5f, "%.1f");
    changed |= _wind_gui_slider(gui, "Shear rate", &p->shear_rate, 0.0f, 1.5f, 0.05f, "%.2f");
    changed |= _wind_gui_slider(
        gui, "Cross wind", &p->cross_wind_strength_mps, 0.0f, 20.0f, 0.5f, "%.1f");
    changed |=
        _wind_gui_slider(gui, "Cross rate", &p->cross_wind_rate, 0.0f, 1.5f, 0.05f, "%.2f");
    changed |=
        _wind_gui_slider(gui, "Terrain friction", &p->terrain_friction, 0.0f, 0.6f, 0.01f, "%.2f");
    changed |= _wind_gui_slider(
        gui, "Terrain color", &p->scalar_terrain_mix_mps, 0.0f, 20.0f, 0.5f, "%.1f");

    dvz_gui_separator_text(gui, "Vectors");
    changed |= _wind_gui_slider(gui, "Vector scale", &p->vector_scale, 0.2f, 3.0f, 0.05f, "%.2f");
    changed |=
        _wind_gui_slider(gui, "Vector alpha", &p->vector_alpha_base, 20.0f, 255.0f, 5.0f, "%.0f");
    changed |= _wind_gui_slider(
        gui, "Vector alpha range", &p->vector_alpha_range, 0.0f, 160.0f, 5.0f, "%.0f");
    changed |=
        _wind_gui_slider(gui, "Vector width", &p->vector_width_base_px, 0.5f, 8.0f, 0.1f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "Vector width range", &p->vector_width_range_px, 0.0f, 6.0f, 0.1f, "%.1f");

    dvz_gui_separator_text(gui, "Streamlines");
    changed |= _wind_gui_slider(
        gui, "Seed wobble", &p->streamline_seed_wobble_km, 0.0f, 100.0f, 5.0f, "%.0f");
    changed |= _wind_gui_slider(
        gui, "Seed wobble rate", &p->streamline_seed_wobble_rate, 0.0f, 1.5f, 0.05f,
        "%.2f");
    changed |= _wind_gui_slider(
        gui, "Inner rotation", &p->streamline_inner_rotation_rate, 0.0f, 2.0f, 0.05f,
        "%.2f");
    changed |= _wind_gui_slider(
        gui, "Outer alpha", &p->streamline_alpha_outer, 0.0f, 255.0f, 5.0f, "%.0f");
    changed |= _wind_gui_slider(
        gui, "Inner alpha", &p->streamline_alpha_inner, 0.0f, 255.0f, 5.0f, "%.0f");
    changed |= _wind_gui_slider(
        gui, "Outer width", &p->streamline_width_outer_px, 0.3f, 6.0f, 0.1f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "Inner width", &p->streamline_width_inner_px, 0.3f, 6.0f, 0.1f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "Outer step", &p->streamline_step_outer_km, 1.0f, 18.0f, 0.5f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "Inner step", &p->streamline_step_inner_km, 1.0f, 18.0f, 0.5f, "%.1f");
    changed |= _wind_gui_slider(
        gui, "Minimum speed", &p->streamline_min_speed_mps, 0.5f, 30.0f, 0.5f, "%.1f");

    return changed;
}


/**
 * Print pasteable C defaults for the current wind parameters.
 *
 * @param fp output stream
 * @param user wind showcase state
 */
static void _wind_params_print_c(FILE* fp, void* user)
{
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (state == NULL)
        return;
    if (fp == NULL)
        fp = stdout;

    const WindShowcaseParams* p = &state->params;
    fprintf(fp, "static const WindShowcaseParams WIND_PARAMS_SHOWCASE = {\n");
#define PRINT_PARAM_FMT(name, fmt) fprintf(fp, "    ." #name " = " fmt "f,\n", (double)p->name)
#define PRINT_PARAM(name)          PRINT_PARAM_FMT(name, "%.6f")
    PRINT_PARAM_FMT(time_scale, "%.1f");
    PRINT_PARAM_FMT(speed_max_mps, "%.0f");
    PRINT_PARAM_FMT(storm_center_x_km, "%.0f");
    PRINT_PARAM_FMT(storm_center_y_km, "%.0f");
    PRINT_PARAM_FMT(storm_drift_x_km, "%.0f");
    PRINT_PARAM_FMT(storm_drift_y_km, "%.0f");
    PRINT_PARAM_FMT(storm_drift_rate_x, "%.2f");
    PRINT_PARAM_FMT(storm_drift_rate_y, "%.2f");
    PRINT_PARAM_FMT(storm_drift_phase_y, "%.1f");
    PRINT_PARAM_FMT(eye_radius_km, "%.0f");
    PRINT_PARAM_FMT(vortex_strength_mps, "%.0f");
    PRINT_PARAM_FMT(spiral_radius_km, "%.0f");
    PRINT_PARAM_FMT(inflow_strength_mps, "%.1f");
    PRINT_PARAM_FMT(breathing_amplitude, "%.3f");
    PRINT_PARAM_FMT(breathing_rate, "%.2f");
    PRINT_PARAM_FMT(background_u_mps, "%.1f");
    PRINT_PARAM_FMT(background_u_y_gradient, "%.3f");
    PRINT_PARAM_FMT(background_u_wave_mps, "%.1f");
    PRINT_PARAM_FMT(background_u_wave_rate, "%.2f");
    PRINT_PARAM(background_u_wave_phase);
    PRINT_PARAM_FMT(background_v_mps, "%.1f");
    PRINT_PARAM_FMT(background_v_wave_mps, "%.1f");
    PRINT_PARAM(background_v_wave_k);
    PRINT_PARAM_FMT(background_v_wave_rate, "%.2f");
    PRINT_PARAM(background_v_wave_phase);
    PRINT_PARAM_FMT(shear_strength_mps, "%.1f");
    PRINT_PARAM(shear_wave_k);
    PRINT_PARAM_FMT(shear_rate, "%.2f");
    PRINT_PARAM(shear_y_center_km);
    PRINT_PARAM(shear_y_radius_km);
    PRINT_PARAM_FMT(cross_wind_strength_mps, "%.1f");
    PRINT_PARAM(cross_wind_wave_k);
    PRINT_PARAM_FMT(cross_wind_rate, "%.2f");
    PRINT_PARAM(cross_wind_phase);
    PRINT_PARAM(cross_wind_x_center_km);
    PRINT_PARAM(cross_wind_x_radius_km);
    PRINT_PARAM_FMT(terrain_friction, "%.2f");
    PRINT_PARAM_FMT(scalar_terrain_mix_mps, "%.1f");
    PRINT_PARAM_FMT(vector_scale, "%.2f");
    PRINT_PARAM_FMT(vector_alpha_base, "%.0f");
    PRINT_PARAM_FMT(vector_alpha_range, "%.0f");
    PRINT_PARAM_FMT(vector_width_base_px, "%.1f");
    PRINT_PARAM_FMT(vector_width_range_px, "%.1f");
    PRINT_PARAM_FMT(streamline_seed_wobble_km, "%.0f");
    PRINT_PARAM_FMT(streamline_seed_wobble_rate, "%.2f");
    PRINT_PARAM_FMT(streamline_inner_rotation_rate, "%.2f");
    PRINT_PARAM(streamline_inner_radius_km);
    PRINT_PARAM(streamline_inner_radius_jitter_km);
    PRINT_PARAM(streamline_inner_y_scale);
    PRINT_PARAM_FMT(streamline_alpha_outer, "%.0f");
    PRINT_PARAM_FMT(streamline_alpha_inner, "%.0f");
    PRINT_PARAM_FMT(streamline_width_outer_px, "%.1f");
    PRINT_PARAM_FMT(streamline_width_inner_px, "%.1f");
    PRINT_PARAM_FMT(streamline_step_outer_km, "%.1f");
    PRINT_PARAM_FMT(streamline_step_inner_km, "%.1f");
    PRINT_PARAM_FMT(streamline_min_speed_mps, "%.1f");
#undef PRINT_PARAM
#undef PRINT_PARAM_FMT
    fprintf(fp, "};\n");
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the synthetic weather field showcase scenario.
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
    WindShowcaseState* state = (WindShowcaseState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->params = WIND_PARAMS_SHOWCASE;
    state->tuner = example_tuner("Wind field settings");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    example_tuner_figure(&state->tuner, ctx->figure);
    (void)example_tuner_add_component(
        &state->tuner, "Wind model", state, NULL, _wind_params_gui, _wind_params_apply,
        _wind_params_reset, _wind_params_print_c);

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, DOMAIN_X_MIN_KM, DOMAIN_X_MAX_KM);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, DOMAIN_Y_MIN_KM, DOMAIN_Y_MAX_KM);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    DvzScale* scale = _add_wind_scale(ctx->scene, &state->params);
    EXAMPLE_CHECK(scale != NULL, "_add_wind_scale() failed");
    state->scale = scale;
    DvzColorbar* colorbar = _add_wind_colorbar(panel, scale);
    EXAMPLE_CHECK(colorbar != NULL, "_add_wind_colorbar() failed");

    state->values = (float*)dvz_calloc((DvzSize)FIELD_WIDTH * FIELD_HEIGHT, sizeof(float));
    EXAMPLE_CHECK(state->values != NULL, "wind scalar field allocation failed");
    _fill_scalar_field(state->values, &state->params, 0.0f);
    state->panel = panel;

    ok = _add_wind_image(ctx->scene, panel, scale, state->values, &state->field);
    EXAMPLE_CHECK(ok, "_add_wind_image() failed");
    ok = _add_streamlines(ctx->scene, panel, &state->streamlines, &state->params, 0.0f);
    EXAMPLE_CHECK(ok, "_add_streamlines() failed");
    ok = _add_vectors(ctx->scene, panel, &state->vectors, &state->params, 0.0f);
    EXAMPLE_CHECK(ok, "_add_vectors() failed");
    ok = _add_probe(ctx->scene, panel, &state->params);
    EXAMPLE_CHECK(ok, "_add_probe() failed");

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    (void)panzoom;

    ok = true;
cleanup:
    return ok;
}


/**
 * Attach live-only wind model tuning controls to the native view.
 *
 * @param ctx scenario context
 * @param app app
 * @param view native view
 * @param user scenario state
 * @return whether the tuner was attached or intentionally skipped
 */
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



/**
 * Destroy the synthetic weather field showcase scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    WindShowcaseState* state = (WindShowcaseState*)user;
    if (state == NULL)
        return;
    dvz_free(state->values);
    dvz_free(state);
}



/**
 * Return the synthetic weather field showcase scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_wind_field_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_wind_field",
        .title = "Wind Field",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the synthetic weather field showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_wind_field_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
