/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* terrain_relief - This example drapes aligned NAIP orthoimagery over USGS 3DEP elevation.
 *
 * What to look for: the real bare-earth DEM drives a lit surface-grid mesh while the matching
 * natural-color aerial image is sampled through the grid UVs. Rotate the arcball view to inspect
 * McHenrys Peak, Glacier Gorge, alpine lakes, and the Continental Divide.
 *
 * Prepare the cache before running:
 *
 *   uv run tools/data/prepare_terrain_relief.py
 *
 * Scenario: showcases_terrain_relief
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c showcases/terrain_relief
 * Run:    ./build/examples/c/showcases/terrain_relief --live
 * Smoke:  ./build/examples/c/showcases/terrain_relief --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/fileio.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_terrain_relief_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define TERRAIN_DATA_DIR  "data/examples/terrain_relief/prepared"
#define TERRAIN_CACHE_DIR ".cache/datoviz/examples/terrain_relief/prepared"
#define TERRAIN_BIN_NAME  "terrain.bin"
#define TERRAIN_JPEG_NAME "terrain.jpg"

#define TERRAIN_MAGIC       "DVZTRN1"
#define TERRAIN_VERSION     1u
#define TERRAIN_HEADER_SIZE 36u
#define TERRAIN_MAX_DIM     2048u

#define TERRAIN_DISPLAY_WIDTH 5.0
#define TERRAIN_EXAGGERATION  1.35
#define TERRAIN_BASE_HEIGHT   0.18f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TerrainData
{
    uint32_t rows;
    uint32_t cols;
    float width_m;
    float depth_m;
    float elevation_min_m;
    float elevation_max_m;
    double* heights_m;
} TerrainData;


typedef struct TerrainReliefState
{
    ExampleTuner tuner;
    DvzArcball* arcball;
    DvzVisual* visual;
    DvzMaterialDesc material;
    DvzExampleGuiMsaaControls msaa;
} TerrainReliefState;



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
    dvz_memcpy(&out, sizeof(out), &value, sizeof(value));
    return out;
}



/**
 * Join a prepared bundle directory and filename.
 *
 * @param directory bundle directory
 * @param filename prepared filename
 * @param out output path
 * @param out_size output buffer size
 * @return whether the path fit in the output buffer
 */
static bool _prepared_path(const char* directory, const char* filename, char* out, size_t out_size)
{
    ANN(directory);
    ANN(filename);
    ANN(out);
    if (out_size == 0)
        return false;
    const int count = snprintf(out, out_size, "%s/%s", directory, filename);
    return count > 0 && (size_t)count < out_size;
}



/**
 * Return whether a prepared file is readable without logging a failed candidate path.
 *
 * @param path file path
 * @return whether the file can be opened
 */
static bool _file_exists(const char* path)
{
    ANN(path);
    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return false;
    fclose(file);
    return true;
}



/**
 * Resolve a complete prepared terrain bundle without mixing data and cache files.
 *
 * @param terrain_path output terrain binary path
 * @param texture_path output texture path
 * @param path_size size of both output buffers
 * @return whether a complete bundle was found
 */
static bool _resolve_bundle(char* terrain_path, char* texture_path, size_t path_size)
{
    ANN(terrain_path);
    ANN(texture_path);

    const char* directories[] = {TERRAIN_DATA_DIR, TERRAIN_CACHE_DIR};
    for (uint32_t i = 0; i < 2; i++)
    {
        if (!_prepared_path(directories[i], TERRAIN_BIN_NAME, terrain_path, path_size) ||
            !_prepared_path(directories[i], TERRAIN_JPEG_NAME, texture_path, path_size))
        {
            return false;
        }
        if (_file_exists(terrain_path) && _file_exists(texture_path))
            return true;
    }
    return false;
}



/**
 * Load and validate a prepared terrain binary.
 *
 * @param path terrain binary path
 * @param out decoded terrain data
 * @return whether the file was valid
 */
static bool _terrain_load(const char* path, TerrainData* out)
{
    ANN(path);
    ANN(out);

    DvzSize size = 0;
    uint8_t* bytes = (uint8_t*)dvz_read_file(path, &size);
    if (bytes == NULL || size < TERRAIN_HEADER_SIZE)
        goto error;

    if (memcmp(bytes, TERRAIN_MAGIC, strlen(TERRAIN_MAGIC)) != 0)
        goto error;
    const uint32_t version = _u32_le(&bytes[8]);
    out->rows = _u32_le(&bytes[12]);
    out->cols = _u32_le(&bytes[16]);
    out->width_m = _f32_le(&bytes[20]);
    out->depth_m = _f32_le(&bytes[24]);
    out->elevation_min_m = _f32_le(&bytes[28]);
    out->elevation_max_m = _f32_le(&bytes[32]);

    if (version != TERRAIN_VERSION || out->rows < 2 || out->cols < 2 ||
        out->rows > TERRAIN_MAX_DIM || out->cols > TERRAIN_MAX_DIM || !isfinite(out->width_m) ||
        !isfinite(out->depth_m) || out->width_m <= 0.0f || out->depth_m <= 0.0f ||
        !isfinite(out->elevation_min_m) || !isfinite(out->elevation_max_m) ||
        out->elevation_max_m <= out->elevation_min_m)
    {
        goto error;
    }

    const uint64_t count = (uint64_t)out->rows * out->cols;
    if (count > (UINT64_MAX - TERRAIN_HEADER_SIZE) / sizeof(float))
        goto error;
    const uint64_t expected_size = TERRAIN_HEADER_SIZE + count * sizeof(float);
    if (size != expected_size || count > SIZE_MAX / sizeof(double))
        goto error;

    out->heights_m = (double*)dvz_calloc((size_t)count, sizeof(double));
    if (out->heights_m == NULL)
        goto error;
    for (uint64_t i = 0; i < count; i++)
    {
        const float elevation = _f32_le(&bytes[TERRAIN_HEADER_SIZE + i * sizeof(float)]);
        if (!isfinite(elevation))
            goto error;
        out->heights_m[i] = (double)elevation - out->elevation_min_m;
    }

    dvz_free(bytes);
    return true;

error:
    dvz_free(bytes);
    dvz_free(out->heights_m);
    out->heights_m = NULL;
    return false;
}



/**
 * Release decoded terrain data.
 *
 * @param terrain terrain data
 */
static void _terrain_destroy(TerrainData* terrain)
{
    if (terrain == NULL)
        return;
    dvz_free(terrain->heights_m);
    terrain->heights_m = NULL;
}



/*************************************************************************************************/
/*  Scene construction                                                                           */
/*************************************************************************************************/

/**
 * Create the scene-owned sampled field for the prepared JPEG texture.
 *
 * @param scene owning scene
 * @param path texture path
 * @return sampled texture field, or NULL on failure
 */
static DvzSampledField* _create_texture(DvzScene* scene, const char* path)
{
    ANN(scene);
    ANN(path);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* pixels = dvz_read_jpeg(path, &width, &height);
    if (pixels == NULL || width == 0 || height == 0)
    {
        dvz_free(pixels);
        return NULL;
    }

    DvzSampledField* texture = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc), .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM, .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = width, .height = height, .depth = 1});
    DvzResult result = DVZ_ERROR;
    if (texture != NULL)
    {
        result = dvz_sampled_field_set_data(
            texture, &(DvzFieldDataView){
                         DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = pixels,
                         .bytes_per_row = width * 4u, .rows_per_image = height});
    }
    dvz_free(pixels);
    return result == DVZ_OK ? texture : NULL;
}



/**
 * Create the physically proportioned surface-grid geometry.
 *
 * @param terrain decoded terrain data
 * @param out_depth display depth output
 * @return surface geometry, or NULL on failure
 */
static DvzGeometry* _create_terrain_geometry(const TerrainData* terrain, float* out_depth)
{
    ANN(terrain);
    ANN(out_depth);

    const double horizontal_scale = TERRAIN_DISPLAY_WIDTH / terrain->width_m;
    const double display_depth = terrain->depth_m * horizontal_scale;
    *out_depth = (float)display_depth;

    DvzGeometrySurfaceGridDesc desc = dvz_geometry_surface_grid_desc();
    desc.rows = terrain->rows;
    desc.cols = terrain->cols;
    desc.heights = terrain->heights_m;
    desc.color = (DvzColor){255, 255, 255, 255};
    desc.origin[0] = -0.5 * TERRAIN_DISPLAY_WIDTH;
    desc.origin[2] = +0.5 * display_depth;
    desc.col_basis[0] = TERRAIN_DISPLAY_WIDTH / (double)(terrain->cols - 1u);
    desc.col_basis[1] = 0.0;
    desc.col_basis[2] = 0.0;
    desc.row_basis[0] = 0.0;
    desc.row_basis[1] = 0.0;
    desc.row_basis[2] = -display_depth / (double)(terrain->rows - 1u);
    desc.height_axis[0] = 0.0;
    desc.height_axis[1] = 1.0;
    desc.height_axis[2] = 0.0;
    desc.height_scale = horizontal_scale * TERRAIN_EXAGGERATION;
    return dvz_geometry_surface_grid(&desc);
}



/**
 * Append one outward-facing vertical terrain-skirt quad.
 *
 * @param skirt destination geometry
 * @param quad quad index
 * @param top0 first top edge position
 * @param top1 second top edge position
 */
static void _set_skirt_quad(DvzGeometry* skirt, uint32_t quad, const dvec3 top0, const dvec3 top1)
{
    ANN(skirt);
    ANN(top0);
    ANN(top1);

    const uint32_t vertex = 4u * quad;
    const uint32_t index = 6u * quad;
    const DvzColor color = {24, 32, 35, 255};
    for (uint32_t i = 0; i < 4; i++)
        skirt->colors[vertex + i] = color;

    for (uint32_t axis = 0; axis < 3; axis++)
    {
        skirt->positions[vertex + 0u][axis] = top0[axis];
        skirt->positions[vertex + 1u][axis] = top1[axis];
        skirt->positions[vertex + 2u][axis] = top1[axis];
        skirt->positions[vertex + 3u][axis] = top0[axis];
    }
    skirt->positions[vertex + 2u][1] = 0.0;
    skirt->positions[vertex + 3u][1] = 0.0;

    skirt->indices[index + 0u] = vertex + 0u;
    skirt->indices[index + 1u] = vertex + 1u;
    skirt->indices[index + 2u] = vertex + 2u;
    skirt->indices[index + 3u] = vertex + 0u;
    skirt->indices[index + 4u] = vertex + 2u;
    skirt->indices[index + 5u] = vertex + 3u;
}



/**
 * Create vertical walls around the structured terrain boundary.
 *
 * The four edges are traversed clockwise from above so their triangle winding faces outward.
 *
 * @param surface source surface-grid geometry
 * @return skirt geometry, or NULL on failure
 */
static DvzGeometry* _create_skirt_geometry(const DvzGeometry* surface)
{
    ANN(surface);
    if (surface->grid_rows < 2 || surface->grid_cols < 2 || surface->positions == NULL)
        return NULL;

    const uint32_t rows = surface->grid_rows;
    const uint32_t cols = surface->grid_cols;
    const uint32_t quad_count = 2u * (rows - 1u) + 2u * (cols - 1u);
    DvzGeometry* skirt = dvz_geometry(4u * quad_count, 6u * quad_count);
    if (skirt == NULL)
        return NULL;

    uint32_t quad = 0;
    for (uint32_t col = 0; col < cols - 1u; col++)
    {
        const uint32_t row = rows - 1u;
        _set_skirt_quad(
            skirt, quad++, surface->positions[row * cols + col],
            surface->positions[row * cols + col + 1u]);
    }
    for (uint32_t row = rows - 1u; row > 0; row--)
    {
        _set_skirt_quad(
            skirt, quad++, surface->positions[row * cols + cols - 1u],
            surface->positions[(row - 1u) * cols + cols - 1u]);
    }
    for (uint32_t col = cols - 1u; col > 0; col--)
    {
        _set_skirt_quad(skirt, quad++, surface->positions[col], surface->positions[col - 1u]);
    }
    for (uint32_t row = 0; row < rows - 1u; row++)
    {
        _set_skirt_quad(
            skirt, quad++, surface->positions[row * cols], surface->positions[(row + 1u) * cols]);
    }

    if (quad != quad_count || dvz_geometry_compute_normals(skirt) != DVZ_OK)
    {
        dvz_geometry_destroy(skirt);
        return NULL;
    }
    return skirt;
}



/**
 * Add the vertical relief-map skirt around the terrain.
 *
 * @param scene owning scene
 * @param panel target panel
 * @param surface source surface geometry
 * @return whether the skirt was added
 */
static bool _add_skirt(DvzScene* scene, DvzPanel* panel, const DvzGeometry* surface)
{
    ANN(scene);
    ANN(panel);
    ANN(surface);

    DvzGeometry* skirt = _create_skirt_geometry(surface);
    if (skirt == NULL)
        return false;
    DvzVisual* visual = dvz_mesh(scene, 0);
    DvzResult result = visual != NULL ? dvz_mesh_set_geometry(visual, skirt) : DVZ_ERROR;
    dvz_geometry_destroy(skirt);
    if (result != DVZ_OK)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.42f;
    material.light_direction[1] = +0.72f;
    material.light_direction[2] = +0.55f;
    material.phong.ambient = 0.36f;
    material.phong.diffuse = 0.58f;
    material.phong.specular = 0.04f;
    material.phong.shininess = 16.0f;
    result = dvz_visual_set_material(visual, &material);
    if (result == DVZ_OK)
        result = dvz_panel_add_visual(panel, visual, NULL);
    return result == DVZ_OK;
}



/**
 * Add the dark relief-map base beneath the terrain.
 *
 * @param scene owning scene
 * @param panel target panel
 * @param display_depth terrain depth in display units
 * @return whether the base was added
 */
static bool _add_base(DvzScene* scene, DvzPanel* panel, float display_depth)
{
    ANN(scene);
    ANN(panel);

    DvzGeometryCubeDesc cube_desc = dvz_geometry_cube_desc();
    cube_desc.color = (DvzColor){20, 28, 32, 255};
    DvzGeometry* cube = dvz_geometry_cube(&cube_desc);
    if (cube == NULL)
        return false;

    DvzVisual* base = dvz_mesh(scene, 0);
    DvzResult result = base != NULL ? dvz_mesh_set_geometry(base, cube) : DVZ_ERROR;
    dvz_geometry_destroy(cube);
    if (result != DVZ_OK)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.42f;
    material.light_direction[1] = +0.72f;
    material.light_direction[2] = +0.55f;
    material.phong.ambient = 0.32f;
    material.phong.diffuse = 0.55f;
    material.phong.specular = 0.08f;
    material.phong.shininess = 18.0f;
    result = dvz_visual_set_material(base, &material);

    mat4 transform = {
        {TERRAIN_DISPLAY_WIDTH + 0.12f, 0.0f, 0.0f, 0.0f},
        {0.0f, TERRAIN_BASE_HEIGHT, 0.0f, 0.0f},
        {0.0f, 0.0f, display_depth + 0.12f, 0.0f},
        {0.0f, -0.5f * TERRAIN_BASE_HEIGHT, 0.0f, 1.0f},
    };
    if (result == DVZ_OK)
        result = dvz_visual_set_transform(base, transform);
    if (result == DVZ_OK)
        result = dvz_panel_add_visual(panel, base, NULL);
    return result == DVZ_OK;
}



/**
 * Add the textured terrain mesh.
 *
 * @param scene owning scene
 * @param panel target panel
 * @param geometry terrain geometry
 * @param texture aligned texture field
 * @return terrain visual, or NULL on failure
 */
static DvzVisual* _add_terrain(
    DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry, DvzSampledField* texture,
    DvzMaterialDesc* material)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);
    ANN(texture);
    ANN(material);

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return NULL;
    DvzResult result = dvz_mesh_set_geometry(visual, geometry);

    if (result == DVZ_OK)
        result = dvz_visual_set_material(visual, material);
    if (result == DVZ_OK)
        result = dvz_visual_set_field(visual, "texture", texture);
    if (result == DVZ_OK)
        result = dvz_panel_add_visual(panel, visual, NULL);
    return result == DVZ_OK ? visual : NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the real-terrain showcase.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    TerrainReliefState* state = (TerrainReliefState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->tuner = example_tuner("Terrain relief settings");
    if (out_user != NULL)
        *out_user = state;

    bool ok = false;
    char terrain_path[1024] = {0};
    char texture_path[1024] = {0};
    TerrainData terrain = {0};
    DvzGeometry* geometry = NULL;

    if (!_resolve_bundle(terrain_path, texture_path, sizeof(terrain_path)))
    {
        dvz_fprintf(
            stderr, "terrain_relief: missing prepared USGS terrain. Run "
                    "`uv run tools/data/prepare_terrain_relief.py` from the repository root.\n");
        goto cleanup;
    }
    EXAMPLE_CHECK(_terrain_load(terrain_path, &terrain), "invalid prepared terrain binary");

    DvzSampledField* texture = _create_texture(ctx->scene, texture_path);
    EXAMPLE_CHECK(texture != NULL, "unable to load prepared terrain texture");

    float display_depth = 0.0f;
    geometry = _create_terrain_geometry(&terrain, &display_depth);
    EXAMPLE_CHECK(geometry != NULL, "dvz_geometry_surface_grid() failed");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    example_tuner_figure(&state->tuner, ctx->figure);
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.025f, 0.034f, 0.040f, 1.0f));

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = +4.95f;
    camera.view.eye[1] = +3.95f;
    camera.view.eye[2] = +5.70f;
    camera.view.target[1] = +0.38f;
    camera.view.up[1] = 1.0f;
    camera.projection.fov_y = 0.56f;
    camera.projection.near_clip = 0.03f;
    camera.projection.far_clip = 100.0f;
    EXAMPLE_CHECK(
        dvz_panel_set_camera_desc(panel, &camera) == DVZ_OK, "dvz_panel_set_camera_desc() failed");

    DvzCamera* camera_ref = dvz_panel_camera(panel);
    EXAMPLE_CHECK(camera_ref != NULL, "dvz_panel_camera() failed");

    EXAMPLE_CHECK(_add_base(ctx->scene, panel, display_depth), "failed to add terrain base");
    EXAMPLE_CHECK(_add_skirt(ctx->scene, panel, geometry), "failed to add terrain skirt");
    state->material = dvz_phong_material_desc();
    state->material.light_direction[0] = -0.38f;
    state->material.light_direction[1] = +0.76f;
    state->material.light_direction[2] = +0.52f;
    state->material.phong.ambient = 0.48f;
    state->material.phong.diffuse = 0.62f;
    state->material.phong.specular = 0.025f;
    state->material.phong.shininess = 24.0f;
    state->visual = _add_terrain(ctx->scene, panel, geometry, texture, &state->material);
    EXAMPLE_CHECK(state->visual != NULL, "failed to add textured terrain");
    EXAMPLE_CHECK(
        dvz_scenario_set_primary_visual(ctx, state->visual) == DVZ_OK,
        "dvz_scenario_set_primary_visual() failed");

#ifndef DVZ_EXAMPLE_NO_APP
    state->msaa = (DvzExampleGuiMsaaControls){
        .enabled = true,
        .alpha_to_coverage = false,
        .samples = 8.0f,
        .min_samples = 2.0f,
        .max_samples = 16.0f,
    };
    DvzMsaaDesc msaa_desc = dvz_msaa_desc();
    msaa_desc.sample_count = 8u;
    EXAMPLE_CHECK(dvz_panel_set_msaa(panel, &msaa_desc) == DVZ_OK, "dvz_panel_set_msaa() failed");
#endif

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    state->arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(state->arcball != NULL, "dvz_controller_arcball() failed");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK,
        "dvz_scenario_bind_controller() failed");

    vec3 arcball_angles = {0.0f, 0.0f, 0.0f};
    vec2 arcball_pan = {0.0f, 0.0f};
    example_tuner_camera_ref(&state->tuner, "Camera", panel, camera_ref, &camera);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, arcball_angles, 1.0f, arcball_pan);
    example_tuner_material(&state->tuner, "Terrain material", state->visual, &state->material);
#ifndef DVZ_EXAMPLE_NO_APP
    example_tuner_msaa(&state->tuner, "MSAA", panel, &state->msaa);
#endif
    ok = true;

cleanup:
    dvz_geometry_destroy(geometry);
    _terrain_destroy(&terrain);
    return ok;
}



/**
 * Attach the live terrain tuner after the native view exists.
 *
 * @param ctx scenario context
 * @param app native app
 * @param view native view
 * @param user scenario state
 * @return whether the tuner was attached or intentionally skipped
 */
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    TerrainReliefState* state = (TerrainReliefState*)user;
    if (ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
    {
        return true;
    }
    return example_tuner_attach(&state->tuner, view);
}



/**
 * Destroy the terrain-relief showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    TerrainReliefState* state = (TerrainReliefState*)user;
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    dvz_free(state);
}



/**
 * Return the terrain-relief showcase scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_terrain_relief_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_terrain_relief",
        .title = "McHenrys Peak Terrain Relief",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .native_view = _scenario_native_view,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the terrain-relief showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_terrain_relief_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
