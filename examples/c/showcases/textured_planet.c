/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* textured_planet - This example combines a textured Earth with real catalogued orbital debris.
 *
 * Scenario: showcases_textured_planet
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * What to look for: `dvz_geometry_sphere()` creates positions, normals, UVs, and indices; the mesh
 * visual receives that geometry plus an RGBA8 sampled field bound to the mesh texture slot.
 * Compare the lit Earth or Mars sphere with the faint star shell and, for Earth, real catalogued
 * debris propagated with SGP4. Object sizes are exaggerated; the trajectories and positions are
 * real.
 *
 * The example uses real texture files from the data submodule when available. Earth has a
 * generated fallback for local development; Mars requires its real texture file and is unavailable
 * when that file is missing.
 *
 * Prepare the debris ephemeris before running:
 *   uv run tools/data/prepare_orbital_debris.py --force
 *
 * Build:  just example-c showcases/textured_planet
 * Run:    ./build/examples/c/showcases/textured_planet --live
 * Smoke:  ./build/examples/c/showcases/textured_planet --png
 * DVZR:   ./build/examples/c/showcases/textured_planet --dvzr 60
 * Video:  ./build/examples/c/showcases/textured_planet --offscreen-record 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "datoviz/fileio.h"
#include "datoviz/geom.h"
#ifndef DVZ_EXAMPLE_NO_MAIN
#include "datoviz/gui.h"
#endif
#include "datoviz/scene.h"
#include "example_common.h"
#include "runner/scenario_runner.h"
#include "textured_planet_orbits.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define TEXTURE_WIDTH  1024
#define TEXTURE_HEIGHT 512
#define EARTH_TEXTURE_PATH          "data/assets/textures/world.200412.3x5400x2700.jpg"
#define EARTH_TEXTURE_FALLBACK_PATH "data/assets/textures/earth.jpg"
#define MARS_TEXTURE_PATH           "data/assets/textures/mars_viking_mdim21.jpg"

#define SPHERE_RADIUS     0.92
#define ATMOSPHERE_RADIUS 0.936
#define SPHERE_SECTORS 96
#define SPHERE_RINGS 48

static const float TAU = 6.28318530718f;

#define STAR_COUNT 900
#define STAR_RADIUS 48.0f

#define ORBIT_DATA_PATH  "data/examples/orbital_debris/prepared/orbital_debris.bin"
#define ORBIT_CACHE_PATH ".cache/datoviz/examples/orbital_debris/prepared/orbital_debris.bin"

#define DEBRIS_TIME_SCALE 60.0f
#define GLOBE_ROTATION_SPEED 0.035f

#define ORBIT_TRACE_COUNT   12
#define ORBIT_TRACE_SAMPLES 121
#define GLOBE_VISUAL_COUNT  4

#define SUN_DIR_X -0.80f
#define SUN_DIR_Y +0.22f
#define SUN_DIR_Z +0.55f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum PlanetKind
{
    PLANET_EARTH,
    PLANET_MARS,
    PLANET_COUNT,
} PlanetKind;



typedef struct PlanetTexture
{
    DvzSampledField* field;
    uint8_t* pixels;
    uint32_t width;
    uint32_t height;
    bool loaded_from_file;
} PlanetTexture;



typedef struct PlanetPreset
{
    const char* label;
    const char* texture_path;
    const char* fallback_path;
    bool require_texture_file;
} PlanetPreset;



typedef struct TexturedPlanetState
{
    DvzVisual* visual;
    DvzVisual* atmosphere_visual;
    DvzVisual* debris_visual;
    DvzVisual* orbit_visual;
    DvzVisual* orbit_glow_visual;
    DvzTrack* globe_rotation;
    DvzAnimation* globe_animations[GLOBE_VISUAL_COUNT];
    PlanetTexture textures[PLANET_COUNT];
    TexturedPlanetOrbitModel orbit_model;
    vec3* debris_positions;
    DvzColor* debris_colors;
    float* debris_sizes;
    int planet_index;
    bool show_debris;
    bool show_orbits;
    bool show_atmosphere;
    bool show_orbit_glow;
    bool animate_debris;
    bool rotate_globe;
    int debris_count;
    float debris_speed;
    float globe_speed;
    double debris_time;
} TexturedPlanetState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_textured_planet_scenario(void);



static const PlanetPreset PLANETS[PLANET_COUNT] = {
    [PLANET_EARTH] =
        {
            .label = "Earth",
            .texture_path = EARTH_TEXTURE_PATH,
            .fallback_path = EARTH_TEXTURE_FALLBACK_PATH,
        },
    [PLANET_MARS] =
        {
            .label = "Mars",
            .texture_path = MARS_TEXTURE_PATH,
            .fallback_path = NULL,
            .require_texture_file = true,
        },
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a scalar to the unit interval.
 *
 * @param value input value
 * @return clamped value
 */
static double _clamp01(double value)
{
    if (value < 0.0)
        return 0.0;
    if (value > 1.0)
        return 1.0;
    return value;
}



/**
 * Convert a normalized scalar to an 8-bit color component.
 *
 * @param value normalized input value
 * @return 8-bit component
 */
static uint8_t _u8(double value)
{
    const double clamped = _clamp01(value);
    return (uint8_t)(255.0 * clamped + 0.5);
}



/**
 * Return whether a texture file exists.
 *
 * @param path file path
 * @return whether the file can be opened for reading
 */
static bool _file_exists(const char* path)
{
    ANN(path);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    fclose(fp);
    return true;
}



/**
 * Return whether an image has equirectangular planet-map dimensions.
 *
 * @param width image width
 * @param height image height
 * @return whether the dimensions are exactly 2:1
 */
static bool _is_equirectangular_texture(uint32_t width, uint32_t height)
{
    return height > 0 && width == 2 * height;
}



/**
 * Remap the generic Z-up UV sphere into the Y-up planet convention.
 *
 * The shared geometry helper uses +Z as the polar axis. This showcase presents planets with +Y
 * as north. The rotation also places the geometry helper's duplicated UV seam on the back side of
 * the default view, so the original equirectangular `u/v` coordinates remain continuous.
 *
 * @param geometry sphere geometry
 */
static void _prepare_planet_geometry(DvzGeometry* geometry)
{
    ANN(geometry);

    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        const double x = geometry->positions[i][0];
        const double y = geometry->positions[i][1];
        const double z = geometry->positions[i][2];
        const double nx = geometry->normals[i][0];
        const double ny = geometry->normals[i][1];
        const double nz = geometry->normals[i][2];

        geometry->positions[i][0] = -y;
        geometry->positions[i][1] = z;
        geometry->positions[i][2] = -x;
        geometry->normals[i][0] = -ny;
        geometry->normals[i][1] = nz;
        geometry->normals[i][2] = -nx;
    }
}



/**
 * Fill one procedural Earth-like equirectangular RGBA texture.
 *
 * @param pixels output RGBA8 texture buffer
 * @param width texture width
 * @param height texture height
 */
static void _make_earth_texture(uint8_t* pixels, uint32_t width, uint32_t height)
{
    ANN(pixels);
    ASSERT(width > 1);
    ASSERT(height > 1);

    for (uint32_t y = 0; y < height; y++)
    {
        const double v = (double)y / (double)(height - 1);
        const double lat = (0.5 - v) * DVZ_PI;
        for (uint32_t x = 0; x < width; x++)
        {
            const double u = (double)x / (double)(width - 1);
            const double lon = (u - 0.5) * 2.0 * DVZ_PI;
            const double bands = sin(12.0 * lon + 1.7 * sin(5.0 * lat)) +
                                 0.72 * sin(9.0 * lat + 2.2 * cos(3.0 * lon)) +
                                 0.38 * sin(21.0 * (lon + lat));
            const double ridge = sin(34.0 * lon) * sin(16.0 * lat);
            const bool land = bands + 0.28 * ridge > 0.38;
            const bool ice = fabs(lat) > 1.22;
            const double grid_u = fabs(u * 24.0 - floor(u * 24.0 + 0.5));
            const double grid_v = fabs(v * 12.0 - floor(v * 12.0 + 0.5));
            const bool grid = grid_u < 0.015 || grid_v < 0.015;

            double r = 0.05;
            double g = 0.13;
            double b = 0.27;
            if (land)
            {
                const double warm = _clamp01(0.5 + 0.5 * sin(5.0 * lon - 7.0 * lat));
                r = 0.20 + 0.38 * warm;
                g = 0.34 + 0.32 * (1.0 - fabs(lat) / (0.5 * DVZ_PI));
                b = 0.16 + 0.12 * (1.0 - warm);
            }
            else
            {
                const double water = 0.5 + 0.5 * sin(8.0 * lon + 5.0 * lat);
                r = 0.03 + 0.05 * water;
                g = 0.20 + 0.18 * water;
                b = 0.44 + 0.22 * water;
            }
            if (ice)
            {
                r = 0.84;
                g = 0.90;
                b = 0.93;
            }
            if (grid)
            {
                r = 0.92;
                g = 0.94;
                b = 0.86;
            }

            const uint32_t offset = 4 * (y * width + x);
            pixels[offset + 0] = _u8(r);
            pixels[offset + 1] = _u8(g);
            pixels[offset + 2] = _u8(b);
            pixels[offset + 3] = 255;
        }
    }
}



/**
 * Try loading a JPEG texture as RGBA8 pixels.
 *
 * @param primary_path preferred file path
 * @param fallback_path fallback file path
 * @param width decoded texture width
 * @param height decoded texture height
 * @return RGBA8 pixel buffer, or NULL when the texture is unavailable or decoding fails
 */
static uint8_t*
_load_texture(const char* primary_path, const char* fallback_path, uint32_t* width, uint32_t* height)
{
    ANN(primary_path);
    ANN(width);
    ANN(height);

    *width = 0;
    *height = 0;
    if (_file_exists(primary_path))
        return dvz_read_jpeg(primary_path, width, height);
    if (fallback_path != NULL && _file_exists(fallback_path))
        return dvz_read_jpeg(fallback_path, width, height);

    return NULL;
}



/**
 * Create and populate one scene-owned planet texture field.
 *
 * @param scene scene
 * @param kind planet kind
 * @param texture texture state to populate
 * @return whether the field was created successfully
 */
static bool _create_planet_texture(
    DvzScene* scene, PlanetKind kind, PlanetTexture* texture)
{
    ANN(scene);
    ANN(texture);
    ASSERT(kind < PLANET_COUNT);
    const PlanetPreset* preset = &PLANETS[kind];

    texture->pixels =
        _load_texture(preset->texture_path, preset->fallback_path, &texture->width, &texture->height);
    texture->loaded_from_file = texture->pixels != NULL;
    if (texture->pixels != NULL && !_is_equirectangular_texture(texture->width, texture->height))
    {
        fprintf(
            stderr,
            "textured_planet: ignoring %s texture with non-equirectangular dimensions %ux%u\n",
            preset->label, texture->width, texture->height);
        dvz_free(texture->pixels);
        texture->pixels = NULL;
        texture->width = 0;
        texture->height = 0;
        texture->loaded_from_file = false;
    }
    if (texture->pixels == NULL)
    {
        if (preset->require_texture_file)
        {
            fprintf(
                stderr, "textured_planet: %s texture unavailable: missing %s\n", preset->label,
                preset->texture_path);
            return true;
        }

        texture->width = TEXTURE_WIDTH;
        texture->height = TEXTURE_HEIGHT;
        texture->pixels =
            (uint8_t*)dvz_calloc((DvzSize)texture->width * texture->height * 4, sizeof(uint8_t));
        if (texture->pixels == NULL)
            return false;
        _make_earth_texture(texture->pixels, texture->width, texture->height);
    }

    texture->field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = texture->width,
                   .height = texture->height,
                   .depth = 1,
               });
    if (texture->field == NULL)
        return false;

    return dvz_sampled_field_set_data(
               texture->field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                                   .data = texture->pixels,
                                   .bytes_per_row = texture->width * 4,
                                   .rows_per_image = texture->height,
                               }) == DVZ_OK;
}



/**
 * Deterministic 32-bit pseudo-random number.
 *
 * @param state generator state
 * @return next pseudo-random value
 */
static uint32_t _rand_u32(uint32_t* state)
{
    ANN(state);
    *state = 1664525u * (*state) + 1013904223u;
    return *state;
}



/**
 * Deterministic pseudo-random float in [0, 1].
 *
 * @param state generator state
 * @return random scalar
 */
static float _rand_f32(uint32_t* state)
{
    return (float)(_rand_u32(state) & 0x00FFFFFFu) / (float)0x01000000u;
}



/**
 * Create a sparse world-space star shell.
 *
 * @param scene scene
 * @return point visual, or NULL on failure
 */
static DvzVisual* _create_star_shell(DvzScene* scene)
{
    ANN(scene);
    vec3* positions = (vec3*)dvz_calloc(STAR_COUNT, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(STAR_COUNT, sizeof(DvzColor));
    float* sizes = (float*)dvz_calloc(STAR_COUNT, sizeof(float));
    if (positions == NULL || colors == NULL || sizes == NULL)
        goto fail;

    uint32_t rng = 0xD42024u;
    for (uint32_t i = 0; i < STAR_COUNT; i++)
    {
        if (i == 0)
        {
            positions[i][0] = STAR_RADIUS * SUN_DIR_X;
            positions[i][1] = STAR_RADIUS * SUN_DIR_Y;
            positions[i][2] = STAR_RADIUS * SUN_DIR_Z;
            colors[i] = (DvzColor){255, 244, 214, 255};
            sizes[i] = 14.0f;
            continue;
        }

        const float z = 2.0f * _rand_f32(&rng) - 1.0f;
        const float phi = 2.0f * (float)DVZ_PI * _rand_f32(&rng);
        const float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
        positions[i][0] = STAR_RADIUS * r * cosf(phi);
        positions[i][1] = STAR_RADIUS * r * sinf(phi);
        positions[i][2] = STAR_RADIUS * z;
        const float brightness = 0.45f + 0.55f * _rand_f32(&rng);
        colors[i].r = _u8(0.82 * brightness);
        colors[i].g = _u8(0.88 * brightness);
        colors[i].b = _u8(1.00 * brightness);
        colors[i].a = 255;
        sizes[i] = 1.0f + 2.2f * _rand_f32(&rng) * _rand_f32(&rng);
    }

    DvzVisual* stars = dvz_point(scene, 0);
    if (stars == NULL)
        goto fail;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = STAR_COUNT},
        {.attr_name = "color", .data = colors, .item_count = STAR_COUNT},
        {.attr_name = "size", .data = sizes, .item_count = STAR_COUNT},
    };
    if (dvz_visual_set_data_many(stars, updates, 3) != 0)
        goto fail;

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(sizes);
    return stars;

fail:
    dvz_free(positions);
    dvz_free(colors);
    dvz_free(sizes);
    return NULL;
}



/**
 * Return the display color assigned to one real debris event.
 *
 * @param event_id prepared event index
 * @param alpha alpha channel
 * @return event color
 */
static DvzColor _debris_event_color(uint8_t event_id, uint8_t alpha)
{
    switch (event_id)
    {
    case 0:
        return (DvzColor){104, 220, 255, alpha};
    case 1:
        return (DvzColor){255, 196, 92, alpha};
    case 2:
        return (DvzColor){255, 112, 96, alpha};
    default:
        return (DvzColor){220, 226, 235, alpha};
    }
}



/**
 * Fill the retained debris colors and point sizes for the active density.
 *
 * Objects outside the active prefix remain allocated but are fully transparent and zero-sized so
 * the GUI can adjust density without changing the retained visual's item count.
 *
 * @param state example state
 */
static void _state_fill_debris_style(TexturedPlanetState* state)
{
    ANN(state);
    ANN(state->orbit_model.event_ids);
    ANN(state->orbit_model.catalog_ids);
    ANN(state->debris_colors);
    ANN(state->debris_sizes);

    for (uint32_t i = 0; i < state->orbit_model.count; i++)
    {
        if (i >= (uint32_t)state->debris_count)
        {
            state->debris_colors[i] = (DvzColor){0, 0, 0, 0};
            state->debris_sizes[i] = 0.0f;
            continue;
        }

        state->debris_colors[i] = _debris_event_color(state->orbit_model.event_ids[i], 255);
        const uint32_t catalog_id = state->orbit_model.catalog_ids[i];
        const float variation = (float)((37u * catalog_id) % 101u) / 100.0f;
        state->debris_sizes[i] = 1.4f + 2.1f * variation;
        if (catalog_id % 79u == 0)
            state->debris_sizes[i] = 5.0f;
    }
}



/**
 * Upload debris colors and sizes after a density change.
 *
 * @param state example state
 * @return whether the retained update succeeded
 */
static bool _state_upload_debris_style(TexturedPlanetState* state)
{
    ANN(state);
    if (state->debris_visual == NULL)
        return false;

    _state_fill_debris_style(state);
    const uint32_t count = state->orbit_model.count;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "color", .data = state->debris_colors, .item_count = count},
        {.attr_name = "size", .data = state->debris_sizes, .item_count = count},
    };
    return dvz_visual_set_data_many(state->debris_visual, updates, 2) == DVZ_OK;
}



/**
 * Select one approximately quantile-spaced object from a prepared debris event.
 *
 * @param model prepared ephemeris
 * @param event_id event index
 * @param ordinal zero-based selection within the event
 * @param selection_count selections requested for the event
 * @return object index
 */
static uint32_t _trace_object_index(
    const TexturedPlanetOrbitModel* model, uint8_t event_id, uint32_t ordinal,
    uint32_t selection_count)
{
    ANN(model);
    uint32_t event_count = 0;
    for (uint32_t i = 0; i < model->count; i++)
        event_count += model->event_ids[i] == event_id ? 1u : 0u;
    if (event_count == 0)
        return 0;

    const uint32_t target = ordinal * event_count / selection_count;
    uint32_t current = 0;
    for (uint32_t i = 0; i < model->count; i++)
    {
        if (model->event_ids[i] != event_id)
            continue;
        if (current++ == target)
            return i;
    }
    return 0;
}



/**
 * Create representative subdued orbit traces.
 *
 * @param scene scene
 * @param panel panel receiving the visual
 * @param model prepared orbit model
 * @param stroke_width path width in pixels
 * @param alpha path alpha
 * @return orbit path visual, or NULL on failure
 */
static DvzVisual* _create_orbit_traces(
    DvzScene* scene, DvzPanel* panel, const TexturedPlanetOrbitModel* model, float stroke_width,
    uint8_t alpha)
{
    ANN(scene);
    ANN(panel);
    ANN(model);
    ASSERT(model->count >= ORBIT_TRACE_COUNT);
    ASSERT(model->event_count > 0);

    const uint32_t sample_count = ORBIT_TRACE_COUNT * ORBIT_TRACE_SAMPLES;
    vec3* positions = (vec3*)dvz_calloc(sample_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(sample_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(sample_count, sizeof(float));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(ORBIT_TRACE_COUNT, sizeof(uint32_t));
    DvzVisual* path = NULL;
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
        goto cleanup;

    for (uint32_t trace_index = 0; trace_index < ORBIT_TRACE_COUNT; trace_index++)
    {
        const uint8_t event_id = (uint8_t)(trace_index % model->event_count);
        const uint32_t ordinal = trace_index / model->event_count;
        const uint32_t selection_count =
            (ORBIT_TRACE_COUNT + model->event_count - 1) / model->event_count;
        const uint32_t orbit_index =
            _trace_object_index(model, event_id, ordinal, selection_count);
        const uint32_t offset = trace_index * ORBIT_TRACE_SAMPLES;
        textured_planet_orbit_model_trace(
            model, orbit_index, ORBIT_TRACE_SAMPLES, &positions[offset]);
        subpaths[trace_index] = ORBIT_TRACE_SAMPLES;
        for (uint32_t j = 0; j < ORBIT_TRACE_SAMPLES; j++)
        {
            colors[offset + j] = _debris_event_color(event_id, alpha);
            widths[offset + j] = stroke_width;
        }
    }

    path = dvz_path(scene, 0);
    if (path == NULL)
        goto cleanup;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = sample_count},
        {.attr_name = "color", .data = colors, .item_count = sample_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = sample_count},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != DVZ_OK)
    {
        path = NULL;
        goto cleanup;
    }
    if (dvz_path_set_subpaths(path, ORBIT_TRACE_COUNT, subpaths) != DVZ_OK)
    {
        path = NULL;
        goto cleanup;
    }
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != DVZ_OK)
    {
        path = NULL;
        goto cleanup;
    }
    if (dvz_visual_set_depth_test(path, true) != DVZ_OK)
    {
        path = NULL;
        goto cleanup;
    }
    if (dvz_visual_set_alpha_mode(path, DVZ_ALPHA_BLENDED) != DVZ_OK)
    {
        path = NULL;
        goto cleanup;
    }
    if (dvz_panel_add_visual(panel, path, NULL) != DVZ_OK)
        path = NULL;

cleanup:
    dvz_free(positions);
    dvz_free(colors);
    dvz_free(widths);
    dvz_free(subpaths);
    return path;
}



/**
 * Create the thin translucent atmosphere shell around Earth.
 *
 * The shell is slightly exaggerated relative to Earth's physical atmosphere so its limb remains
 * legible at gallery scale. Depth testing hides its rear hemisphere behind the opaque planet.
 *
 * @param scene scene
 * @param panel panel receiving the visual
 * @return atmosphere mesh visual, or NULL on failure
 */
static DvzVisual* _create_atmosphere(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);
    DvzGeometry* sphere = dvz_geometry_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = ATMOSPHERE_RADIUS,
        .sectors = SPHERE_SECTORS,
        .rings = SPHERE_RINGS,
        .color = {58, 142, 255, 7},
    });
    if (sphere == NULL)
        return NULL;
    _prepare_planet_geometry(sphere);

    vec3* positions = (vec3*)dvz_calloc(sphere->index_count, sizeof(vec3));
    vec3* normals = (vec3*)dvz_calloc(sphere->index_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(sphere->index_count, sizeof(DvzColor));
    DvzVisual* atmosphere = NULL;
    if (positions == NULL || normals == NULL || colors == NULL)
        goto cleanup;
    for (uint32_t i = 0; i < sphere->index_count; i++)
    {
        const DvzIndex index = sphere->indices[i];
        positions[i][0] = (float)sphere->positions[index][0];
        positions[i][1] = (float)sphere->positions[index][1];
        positions[i][2] = (float)sphere->positions[index][2];
        normals[i][0] = (float)sphere->normals[index][0];
        normals[i][1] = (float)sphere->normals[index][1];
        normals[i][2] = (float)sphere->normals[index][2];
        colors[i] = sphere->colors[index];
    }

    atmosphere = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (atmosphere == NULL)
        goto cleanup;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = sphere->index_count},
        {.attr_name = "color", .data = colors, .item_count = sphere->index_count},
        {.attr_name = "normal", .data = normals, .item_count = sphere->index_count},
    };
    DvzResult rc = dvz_visual_set_data_many(atmosphere, updates, 3);
    if (rc == DVZ_OK)
        rc = dvz_visual_set_alpha_mode(atmosphere, DVZ_ALPHA_BLENDED);
    if (rc == DVZ_OK)
        rc = dvz_visual_set_depth_test(atmosphere, true);
    if (rc == DVZ_OK)
        rc = dvz_panel_add_visual(panel, atmosphere, NULL);
    if (rc != DVZ_OK)
        atmosphere = NULL;

cleanup:
    dvz_free(positions);
    dvz_free(normals);
    dvz_free(colors);
    dvz_geometry_destroy(sphere);
    return atmosphere;
}



/**
 * Create the retained orbital-debris point layer.
 *
 * @param scene scene
 * @param panel panel receiving the visual
 * @param state example state
 * @return debris point visual, or NULL on failure
 */
static DvzVisual*
_create_debris_points(DvzScene* scene, DvzPanel* panel, TexturedPlanetState* state)
{
    ANN(scene);
    ANN(panel);
    ANN(state);

    textured_planet_orbit_model_positions(
        &state->orbit_model, state->debris_time, state->debris_positions);
    _state_fill_debris_style(state);

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return NULL;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position",
         .data = state->debris_positions,
         .item_count = state->orbit_model.count},
        {.attr_name = "color",
         .data = state->debris_colors,
         .item_count = state->orbit_model.count},
        {.attr_name = "size", .data = state->debris_sizes, .item_count = state->orbit_model.count},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != DVZ_OK)
        return NULL;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(points, &style) != DVZ_OK)
        return NULL;
    if (dvz_visual_set_depth_test(points, true) != DVZ_OK)
        return NULL;
    if (dvz_panel_add_visual(panel, points, NULL) != DVZ_OK)
        return NULL;
    return points;
}



/**
 * Reset orbital-debris controls to the showcase defaults.
 *
 * @param state example state
 */
static void _state_reset_debris_controls(TexturedPlanetState* state)
{
    ANN(state);
    state->show_debris = true;
    state->show_orbits = true;
    state->show_atmosphere = true;
    state->show_orbit_glow = true;
    state->animate_debris = true;
    state->rotate_globe = true;
    state->debris_count = (int)state->orbit_model.count;
    state->debris_speed = DEBRIS_TIME_SCALE;
    state->globe_speed = GLOBE_ROTATION_SPEED;
    state->debris_time = 0.0;
}



/**
 * Apply orbital-layer visibility controls.
 *
 * @param state example state
 */
static void _state_apply_debris_visibility(TexturedPlanetState* state)
{
    ANN(state);
    const bool is_earth = state->planet_index == PLANET_EARTH;
    if (state->debris_visual != NULL)
        (void)dvz_visual_set_visible(state->debris_visual, is_earth && state->show_debris);
    if (state->orbit_visual != NULL)
        (void)dvz_visual_set_visible(state->orbit_visual, is_earth && state->show_orbits);
    if (state->orbit_glow_visual != NULL)
        (void)dvz_visual_set_visible(
            state->orbit_glow_visual, is_earth && state->show_orbits && state->show_orbit_glow);
    if (state->atmosphere_visual != NULL)
        (void)dvz_visual_set_visible(state->atmosphere_visual, is_earth && state->show_atmosphere);
}



/**
 * Apply the shared display-rotation speed to Earth, debris, and orbit paths.
 *
 * @param state example state
 */
static void _state_apply_globe_rotation(TexturedPlanetState* state)
{
    ANN(state);
    const float speed = state->rotate_globe ? state->globe_speed : 0.0f;
    for (uint32_t i = 0; i < GLOBE_VISUAL_COUNT; i++)
    {
        if (state->globe_animations[i] != NULL)
            (void)dvz_anim_set_speed(state->globe_animations[i], speed);
    }
}



/**
 * Create one shared display rotation for the planet-relative visual layers.
 *
 * @param scene scene
 * @param state example state
 * @return whether all animations were created
 */
static bool _create_globe_rotation(DvzScene* scene, TexturedPlanetState* state)
{
    ANN(scene);
    ANN(state);
    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[0] = 0.0f;
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.axis[2] = 0.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    state->globe_rotation = dvz_track_rotation(&rotation_desc);
    if (state->globe_rotation == NULL)
        return false;

    DvzVisual* visuals[GLOBE_VISUAL_COUNT] = {
        state->visual, state->orbit_glow_visual, state->orbit_visual, state->debris_visual};
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = state->globe_rotation;
    for (uint32_t i = 0; i < GLOBE_VISUAL_COUNT; i++)
    {
        state->globe_animations[i] = dvz_anim_visual_transform(scene, visuals[i], &transform_desc);
        if (state->globe_animations[i] == NULL)
            return false;
        (void)dvz_anim_set_speed(state->globe_animations[i], state->globe_speed);
        (void)dvz_anim_start(state->globe_animations[i], 0.0);
    }
    return true;
}



/**
 * Bind the selected planet texture to the mesh.
 *
 * @param state example state
 * @param report_error whether to print an error when the selected texture is unavailable
 * @return whether the selected planet texture was bound
 */
static bool _state_apply_planet(TexturedPlanetState* state, bool report_error)
{
    ANN(state);
    if (state->visual == NULL)
        return false;
    if (state->planet_index < 0 || state->planet_index >= PLANET_COUNT)
        return false;

    const PlanetPreset* preset = &PLANETS[state->planet_index];
    PlanetTexture* texture = &state->textures[state->planet_index];
    if (texture->field == NULL)
    {
        if (report_error)
            fprintf(
                stderr,
                "textured_planet: %s texture is unavailable; keeping the current planet\n",
                preset->label);
        return false;
    }

    return dvz_visual_set_field(state->visual, "texture", texture->field) == DVZ_OK;
}



#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Build live GUI controls for the textured planet example.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _textured_planet_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    TexturedPlanetState* state = (TexturedPlanetState*)user_data;
    if (state == NULL)
        return;

    static const char* const planet_items[PLANET_COUNT] = {"Earth", "Mars"};
    const int previous_planet_index = state->planet_index;
    bool planet_changed = false;
    bool debris_visibility_changed = false;
    bool debris_density_changed = false;
    bool rotation_changed = false;
    bool reset = false;

    if (dvz_gui_begin(gui, "Textured Planets", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Planet");
        planet_changed |=
            dvz_gui_combo(gui, "Preset", &state->planet_index, planet_items, PLANET_COUNT);
        rotation_changed |= dvz_gui_checkbox(gui, "Rotate globe", &state->rotate_globe);
        rotation_changed |= dvz_gui_slider_float_format(
            gui, "Globe rotation", &state->globe_speed, 0.0f, 0.20f, "%.3f rad/s");
        debris_visibility_changed |=
            dvz_gui_checkbox(gui, "Show atmosphere", &state->show_atmosphere);

        dvz_gui_separator_text(gui, "Catalogued orbital debris");
        debris_visibility_changed |= dvz_gui_checkbox(gui, "Show debris", &state->show_debris);
        debris_visibility_changed |=
            dvz_gui_checkbox(gui, "Show orbit lines", &state->show_orbits);
        debris_visibility_changed |= dvz_gui_checkbox(gui, "Orbit glow", &state->show_orbit_glow);
        (void)dvz_gui_checkbox(gui, "Animate debris", &state->animate_debris);
        debris_density_changed |= dvz_gui_slider_int(
            gui, "Object count", &state->debris_count, 0, (int)state->orbit_model.count);
        (void)dvz_gui_slider_float_format(
            gui, "Time scale", &state->debris_speed, 0.0f, 600.0f, "%.0fx real time");
        dvz_gui_text(gui, "CelesTrak GP elements propagated with SGP4.");
        dvz_gui_text(gui, state->orbit_model.snapshot_utc);
        dvz_gui_text(gui, "Cyan: FENGYUN 1C | Amber: IRIDIUM 33");
        dvz_gui_text(gui, "Coral: COSMOS 2251 | Point sizes exaggerated");

        reset = dvz_gui_button(gui, "Reset");
    }
    dvz_gui_end(gui);

    if (planet_changed)
    {
        if (_state_apply_planet(state, true))
        {
            debris_visibility_changed = true;
        }
        else
        {
            state->planet_index = previous_planet_index;
        }
    }
    if (reset)
    {
        _state_reset_debris_controls(state);
        debris_visibility_changed = true;
        debris_density_changed = true;
        rotation_changed = true;
    }
    if (debris_visibility_changed)
        _state_apply_debris_visibility(state);
    if (debris_density_changed)
        (void)_state_upload_debris_style(state);
    if (rotation_changed)
        _state_apply_globe_rotation(state);
}
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    DvzGeometry* sphere = NULL;
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    TexturedPlanetState* state = (TexturedPlanetState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->planet_index = PLANET_EARTH;

    ok = textured_planet_orbit_model_load(ORBIT_DATA_PATH, &state->orbit_model);
    if (!ok)
        ok = textured_planet_orbit_model_load(ORBIT_CACHE_PATH, &state->orbit_model);
    if (!ok)
    {
        dvz_fprintf(
            stderr,
            "textured_planet: missing real orbital-debris ephemeris. Run "
            "`uv run tools/data/prepare_orbital_debris.py --force` from the repository root.\n");
    }
    EXAMPLE_CHECK(ok, "real orbital-debris data setup failed");
    _state_reset_debris_controls(state);
    const uint32_t debris_count = state->orbit_model.count;
    state->debris_positions = (vec3*)dvz_calloc(debris_count, sizeof(vec3));
    state->debris_colors = (DvzColor*)dvz_calloc(debris_count, sizeof(DvzColor));
    state->debris_sizes = (float*)dvz_calloc(debris_count, sizeof(float));
    EXAMPLE_CHECK(
        state->debris_positions != NULL && state->debris_colors != NULL &&
            state->debris_sizes != NULL,
        "failed to allocate orbital-debris data");
    dvz_fprintf(
        stderr, "textured_planet: %u real catalogued objects, snapshot %s\n", debris_count,
        state->orbit_model.snapshot_utc);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = 0.0f;
    camera_desc.view.eye[1] = 0.0f;
    camera_desc.view.eye[2] = 3.7f;
    camera_desc.projection.fov_y = 0.72f;
    camera_desc.projection.near_clip = 0.005f;
    camera_desc.projection.far_clip = 100.0f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    DvzCamera* camera = dvz_panel_camera(panel);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");
    EXAMPLE_CHECK(camera != NULL, "dvz_panel_set_camera_desc() failed");

    DvzVisual* stars = _create_star_shell(ctx->scene);
    EXAMPLE_CHECK(stars != NULL, "failed to create star shell");
    int rc = dvz_panel_add_visual(panel, stars, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(stars) failed");

    DvzColor white = {255, 255, 255, 255};
    sphere = dvz_geometry_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = SPHERE_RADIUS,
        .sectors = SPHERE_SECTORS,
        .rings = SPHERE_RINGS,
        .color = white,
    });
    EXAMPLE_CHECK(sphere != NULL, "dvz_geometry_sphere() failed");
    _prepare_planet_geometry(sphere);

    DvzVisual* visual = dvz_mesh(ctx->scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_mesh() failed");
    state->visual = visual;
    EXAMPLE_CHECK(
        dvz_scenario_set_primary_visual(ctx, visual) == 0,
        "dvz_scenario_set_primary_visual(planet) failed");

    ok = dvz_mesh_set_geometry(visual, sphere) == 0;
    EXAMPLE_CHECK(ok, "dvz_mesh_set_geometry() failed");
    dvz_geometry_destroy(sphere);
    sphere = NULL;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = SUN_DIR_X;
    material.light_direction[1] = SUN_DIR_Y;
    material.light_direction[2] = SUN_DIR_Z;
    material.phong.ambient = 0.075f;
    material.phong.diffuse = 1.02f;
    material.phong.specular = 0.025f;
    material.phong.shininess = 22.0f;
    EXAMPLE_CHECK(
        dvz_visual_set_material(visual, &material) == 0, "dvz_visual_set_material() failed");

    for (uint32_t i = 0; i < PLANET_COUNT; i++)
    {
        ok = _create_planet_texture(ctx->scene, (PlanetKind)i, &state->textures[i]);
        EXAMPLE_CHECK(ok, "failed to create planet texture");
    }

    ok = dvz_visual_set_field(visual, "texture", state->textures[state->planet_index].field) ==
         DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_visual_set_field(texture) failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    state->atmosphere_visual = _create_atmosphere(ctx->scene, panel);
    EXAMPLE_CHECK(state->atmosphere_visual != NULL, "failed to create atmosphere shell");

    state->orbit_glow_visual =
        _create_orbit_traces(ctx->scene, panel, &state->orbit_model, 2.8f, 22);
    EXAMPLE_CHECK(state->orbit_glow_visual != NULL, "failed to create orbit glow");
    state->orbit_visual = _create_orbit_traces(ctx->scene, panel, &state->orbit_model, 0.68f, 190);
    EXAMPLE_CHECK(state->orbit_visual != NULL, "failed to create orbit traces");

    state->debris_visual = _create_debris_points(ctx->scene, panel, state);
    EXAMPLE_CHECK(state->debris_visual != NULL, "failed to create orbital-debris points");
    _state_apply_debris_visibility(state);
    ok = _create_globe_rotation(ctx->scene, state);
    EXAMPLE_CHECK(ok, "failed to create shared globe rotation");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.006f, 0.008f, 0.014f, 1.0f));

    DvzTurntableDesc turntable_desc = dvz_turntable_desc();
    turntable_desc.min_distance = 1.02f;
    turntable_desc.max_distance = 20.0f;
    turntable_desc.zoom_speed = 0.018f;
    DvzController* controller = dvz_turntable(ctx->scene, &turntable_desc);
    EXAMPLE_CHECK(controller != NULL, "dvz_turntable() failed");
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    EXAMPLE_CHECK(turntable != NULL, "failed to create or bind turntable controller");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");

    ok = true;
cleanup:
    if (sphere != NULL)
        dvz_geometry_destroy(sphere);
    return ok;
}



/**
 * Advance and upload the prepared orbital-debris positions.
 *
 * @param ctx scenario context
 * @param user example state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    TexturedPlanetState* state = (TexturedPlanetState*)user;
    if (ctx == NULL || state == NULL || state->debris_visual == NULL)
        return;

    if (ctx->preview_mode)
    {
        state->debris_time = dvz_scenario_preview_time(ctx) * state->debris_speed;
    }
    else if (state->animate_debris)
    {
        const double dt = fmin(fmax(ctx->dt, 0.0), 0.1);
        state->debris_time += dt * state->debris_speed;
    }

    textured_planet_orbit_model_positions(
        &state->orbit_model, state->debris_time, state->debris_positions);
    (void)dvz_visual_set_data(
        state->debris_visual, "position", state->debris_positions, state->orbit_model.count);
}



#ifndef DVZ_EXAMPLE_NO_MAIN
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    TexturedPlanetState* state = (TexturedPlanetState*)user;
    if (state == NULL || view == NULL || ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW)
        return true;

    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return true;
    dvz_view_set_gui_callback(view, _textured_planet_gui, state);
    return true;
}
#endif



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    TexturedPlanetState* state = (TexturedPlanetState*)user;
    if (state == NULL)
        return;
    dvz_track_destroy(state->globe_rotation);
    textured_planet_orbit_model_destroy(&state->orbit_model);
    dvz_free(state->debris_positions);
    dvz_free(state->debris_colors);
    dvz_free(state->debris_sizes);
    for (uint32_t i = 0; i < PLANET_COUNT; i++)
        dvz_free(state->textures[i].pixels);
    dvz_free(state);
}



DvzScenarioSpec dvz_showcase_textured_planet_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_textured_planet",
        .title = "Textured Planets and Orbital Debris",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_textured_planet_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
