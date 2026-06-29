/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* wind_field - synthetic weather-like scalar and vector field showcase.
 *
 * Scenario: showcase_wind_field
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
#define WIND_SPEED_MAX_MPS  80.0f
#define DOMAIN_X_MIN_KM     -620.0f
#define DOMAIN_X_MAX_KM     +620.0f
#define DOMAIN_Y_MIN_KM     -390.0f
#define DOMAIN_Y_MAX_KM     +390.0f
#define STORM_CENTER_X_KM   +245.0f
#define STORM_CENTER_Y_KM   -8.0f
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


typedef struct WindShowcaseState
{
    DvzPanel* panel;
    DvzSampledField* field;
    DvzVisual* vectors;
    DvzVisual* streamlines;
    float* values;
} WindShowcaseState;



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
 * @param time_s deterministic animation time in seconds
 * @return wind sample with vector, speed, and direction
 */
static WindSample _wind_sample(float x, float y, float time_s)
{
    const float center_x = STORM_CENTER_X_KM + 26.0f * sinf(0.105f * time_s);
    const float center_y = STORM_CENTER_Y_KM + 16.0f * cosf(0.083f * time_s + 0.7f);
    const float dx = x - center_x;
    const float dy = y - center_y;
    const float r = sqrtf(dx * dx + dy * dy) + 1e-3f;

    const float eye_radius = 112.0f;
    const float rr = r / eye_radius;
    const float breathing = 1.0f + 0.045f * sinf(0.17f * time_s);
    const float vortex = 70.0f * breathing * rr * expf(0.5f * (1.0f - rr * rr));
    const float spiral = expf(-(r * r) / (2.0f * 330.0f * 330.0f));
    const float inflow = -9.5f * spiral;
    const float terrain = _terrain_mask(x, y);

    float u = 17.0f + 0.010f * y + 1.9f * sinf(0.090f * time_s + 0.4f);
    float v = -3.5f + 3.5f * sinf(0.008f * x + 0.7f + 0.12f * time_s);

    u += -vortex * dy / r + inflow * dx / r;
    v += +vortex * dx / r + inflow * dy / r;

    const float shear = 6.5f * sinf(0.0055f * (x - 0.8f * y) + 0.20f * time_s);
    u += shear * expf(-(y + 125.0f) * (y + 125.0f) / (2.0f * 310.0f * 310.0f));
    v += 4.0f * sinf(0.007f * y - 0.3f + 0.16f * time_s) *
         expf(-(x + 220.0f) * (x + 220.0f) / (2.0f * 520.0f * 520.0f));

    const float friction = 1.0f - 0.17f * terrain;
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
 * @return output RGBA8 color
 */
static DvzColor _wind_colormap(float speed_mps)
{
    const float t = _clamp01(speed_mps / WIND_SPEED_MAX_MPS);
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
 * @return output RGBA8 color
 */
static DvzColor
_wind_flow_color(float speed_mps, uint8_t alpha, float midpoint, float gamma)
{
    const float normalized = _clamp01(speed_mps / WIND_SPEED_MAX_MPS);
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
 * @return output RGBA8 color
 */
static DvzColor _wind_arrow_color(float speed_mps, uint8_t alpha)
{
    return _wind_flow_color(speed_mps, alpha, 0.44f, 0.78f);
}



/**
 * Map one wind speed to a streamline color.
 *
 * @param speed_mps wind speed in m/s
 * @param alpha output alpha
 * @return output RGBA8 color
 */
static DvzColor _wind_streamline_color(float speed_mps, uint8_t alpha)
{
    return _wind_flow_color(speed_mps, alpha, 0.36f, 0.64f);
}



/**
 * Fill the custom wind-speed colormap LUT.
 *
 * @param colors output RGBA8 LUT
 */
static void _fill_wind_colormap(DvzColor colors[COLORMAP_LUT_SIZE])
{
    ANN(colors);

    for (uint32_t i = 0; i < COLORMAP_LUT_SIZE; i++)
    {
        const float t =
            COLORMAP_LUT_SIZE > 1u ? (float)i / (float)(COLORMAP_LUT_SIZE - 1u) : 0.0f;
        colors[i] = _wind_colormap(t * WIND_SPEED_MAX_MPS);
    }
}



/**
 * Fill the scalar wind-speed field.
 *
 * @param values output scalar field values in m/s
 * @param time_s deterministic animation time in seconds
 */
static void _fill_scalar_field(float* values, float time_s)
{
    ANN(values);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        const float fy = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
        const float data_y = _mix(DOMAIN_Y_MIN_KM, DOMAIN_Y_MAX_KM, fy);
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float fx = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float data_x = _mix(DOMAIN_X_MIN_KM, DOMAIN_X_MAX_KM, fx);
            WindSample sample = _wind_sample(data_x, data_y, time_s);
            const float terrain = _terrain_mask(data_x, data_y);
            values[y * FIELD_WIDTH + x] =
                _clamp01((sample.speed + 6.0f * terrain) / WIND_SPEED_MAX_MPS) *
                WIND_SPEED_MAX_MPS;
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
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _fill_vectors(vec3* positions, vec3* vectors, DvzColor* colors, float* widths, float time_s)
{
    ANN(positions);
    ANN(vectors);
    ANN(colors);
    ANN(widths);

    uint32_t idx = 0;
    const float step_x = (DOMAIN_X_MAX_KM - DOMAIN_X_MIN_KM) / (float)(VECTOR_COLS - 1u);
    const float step_y = (DOMAIN_Y_MAX_KM - DOMAIN_Y_MIN_KM) / (float)(VECTOR_ROWS - 1u);
    for (uint32_t row = 0; row < VECTOR_ROWS; row++)
    {
        for (uint32_t col = 0; col < VECTOR_COLS; col++)
        {
            const float x = DOMAIN_X_MIN_KM + (float)col * step_x;
            const float y = DOMAIN_Y_MIN_KM + (float)row * step_y;
            WindSample sample = _wind_sample(x, y, time_s);
            const float scale = 1.02f;

            vec3 data_start[1] = {{x, y, 0.0f}};
            vec3 data_end[1] = {{x + scale * sample.u, y + scale * sample.v, 0.0f}};
            data_start[0][2] = 0.03f;
            data_end[0][2] = 0.03f;

            positions[idx][0] = data_start[0][0];
            positions[idx][1] = data_start[0][1];
            positions[idx][2] = data_start[0][2];
            vectors[idx][0] = data_end[0][0] - data_start[0][0];
            vectors[idx][1] = data_end[0][1] - data_start[0][1];
            vectors[idx][2] = 0.0f;

            colors[idx] = _wind_arrow_color(
                sample.speed, (uint8_t)(150u + (uint32_t)(78.0f * _clamp01(sample.speed / 70.0f))));
            widths[idx] = 2.2f + 1.6f * _clamp01(sample.speed / WIND_SPEED_MAX_MPS);
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
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool _add_vectors(DvzScene* scene, DvzPanel* panel, DvzVisual** out_visual, float time_s)
{
    ANN(scene);
    ANN(panel);
    ANN(out_visual);

    vec3* positions = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*positions));
    vec3* vectors = (vec3*)dvz_calloc(VECTOR_COUNT, sizeof(*vectors));
    DvzColor* colors = (DvzColor*)dvz_calloc(VECTOR_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(VECTOR_COUNT, sizeof(*widths));
    if (positions == NULL || vectors == NULL || colors == NULL || widths == NULL)
        goto error;
    if (!_fill_vectors(positions, vectors, colors, widths, time_s))
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
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool
_fill_streamlines(vec3* positions, DvzColor* colors, float* widths, uint32_t* subpaths, float time_s)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);

    for (uint32_t line = 0; line < STREAMLINE_COUNT; line++)
    {
        subpaths[line] = STREAMLINE_POINT_COUNT;
        const float band = (float)line / (float)(STREAMLINE_COUNT - 1u);
        const float upper_band = powf(band, 0.68f);
        float x = DOMAIN_X_MIN_KM + 78.0f + 54.0f * (float)(line % 8u) +
                  22.0f * sinf(0.24f * time_s + 3.1f * band);
        float y = _mix(DOMAIN_Y_MIN_KM + 132.0f, DOMAIN_Y_MAX_KM - 42.0f, upper_band);
        if (line >= STREAMLINE_COUNT / 2u)
        {
            const uint32_t inner = line - STREAMLINE_COUNT / 2u;
            const float a =
                TAU * (float)inner / (float)(STREAMLINE_COUNT / 2u) + 0.10f * time_s;
            const float radius = 106.0f + 4.1f * (float)(inner % 7u);
            x = STORM_CENTER_X_KM + radius * cosf(a);
            y = STORM_CENTER_Y_KM + (0.78f * radius) * sinf(a);
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

            WindSample sample = _wind_sample(x, y, time_s);
            colors[idx] = _wind_streamline_color(
                sample.speed, line < STREAMLINE_COUNT / 2u ? 118u : 166u);
            held_color = colors[idx];
            held_color.a = 0u;
            widths[idx] = line < STREAMLINE_COUNT / 2u ? 1.35f : 1.85f;

            const float norm = fmaxf(sample.speed, 7.5f);
            const float step = line < STREAMLINE_COUNT / 2u ? 6.2f : 4.7f;
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
 * @param time_s deterministic animation time in seconds
 * @return true on success
 */
static bool
_add_streamlines(DvzScene* scene, DvzPanel* panel, DvzVisual** out_visual, float time_s)
{
    ANN(scene);
    ANN(panel);
    ANN(out_visual);

    vec3* positions = (vec3*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*colors));
    float* widths = (float*)dvz_calloc(STREAMLINE_TOTAL_COUNT, sizeof(*widths));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(STREAMLINE_COUNT, sizeof(*subpaths));
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
        goto error;
    if (!_fill_streamlines(positions, colors, widths, subpaths, time_s))
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
 * @return true on success
 */
static bool _add_probe(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

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

    WindSample sample = _wind_sample(PROBE_X_KM, PROBE_Y_KM, 0.0f);
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
            .style = &card_style,
        });
    return card != NULL;
}



/**
 * Create the shared wind-speed color scale.
 *
 * @param scene scene owning scale resources
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_wind_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "wind speed",
                   .unit = "m/s",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 0, .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, 0.0, WIND_SPEED_MAX_MPS);
    dvz_scale_set_view_range(scale, 0.0, WIND_SPEED_MAX_MPS);

    DvzColor colors[COLORMAP_LUT_SIZE] = {0};
    _fill_wind_colormap(colors);
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

    _fill_scalar_field(state->values, time_s);
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
        });
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
    if (!_fill_vectors(positions, vectors, colors, widths, time_s))
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
    if (!_fill_streamlines(positions, colors, widths, subpaths, time_s))
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

    const uint32_t frame_index = (uint32_t)ctx->frame_index + 1u;
    if (frame_index % ANIMATION_STRIDE != 0)
        return;

    const float time_s = (float)frame_index / ANIMATION_FPS;
    if (!_update_wind_image(state, time_s))
        return;
    if (!_update_streamlines(state, time_s))
        return;
    (void)_update_vectors(state, time_s);
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, DOMAIN_X_MIN_KM, DOMAIN_X_MAX_KM);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, DOMAIN_Y_MIN_KM, DOMAIN_Y_MAX_KM);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    DvzScale* scale = _add_wind_scale(ctx->scene);
    EXAMPLE_CHECK(scale != NULL, "_add_wind_scale() failed");
    DvzColorbar* colorbar = _add_wind_colorbar(panel, scale);
    EXAMPLE_CHECK(colorbar != NULL, "_add_wind_colorbar() failed");

    state->values = (float*)dvz_calloc((DvzSize)FIELD_WIDTH * FIELD_HEIGHT, sizeof(float));
    EXAMPLE_CHECK(state->values != NULL, "wind scalar field allocation failed");
    _fill_scalar_field(state->values, 0.0f);
    state->panel = panel;

    ok = _add_wind_image(ctx->scene, panel, scale, state->values, &state->field);
    EXAMPLE_CHECK(ok, "_add_wind_image() failed");
    ok = _add_streamlines(ctx->scene, panel, &state->streamlines, 0.0f);
    EXAMPLE_CHECK(ok, "_add_streamlines() failed");
    ok = _add_vectors(ctx->scene, panel, &state->vectors, 0.0f);
    EXAMPLE_CHECK(ok, "_add_vectors() failed");
    ok = _add_probe(ctx->scene, panel);
    EXAMPLE_CHECK(ok, "_add_probe() failed");

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    (void)panzoom;

    ok = true;
cleanup:
    return ok;
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
        .id = "showcase_wind_field",
        .title = "showcase_wind_field",
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
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
