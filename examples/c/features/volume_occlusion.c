/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* volume_occlusion compares a volume slice with and without attenuation by the surrounding volume.
 *
 * What to look for: both panels share the same 32x32x32 R8 scalar field, colormap, alpha stops,
 * bounds, opacity, and ray-march step count. Each panel draws a composited volume plus a slice, but
 * only the right panel marks the volume as an occluder and enables slice attenuation. Compare the
 * embedded slice where dense shell and knot structures overlap it; occlusion helps relate slices
 * to their 3D context instead of making them look detached.
 *
 * Scenario: feature_volume_occlusion
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/volume_occlusion
 * Run:    ./build/examples/c/features/volume_occlusion --live
 * Smoke:  ./build/examples/c/features/volume_occlusion --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define FIELD_SIZE 32u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VolumeOcclusionState
{
    uint8_t voxels[FIELD_SIZE * FIELD_SIZE * FIELD_SIZE];
} VolumeOcclusionState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp and convert a normalized scalar to an 8-bit sample.
 *
 * @param value normalized value
 * @return 8-bit scalar value
 */
static uint8_t _u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Fill a deterministic scalar field with a dense shell and central knot.
 *
 * @param voxels output scalar field
 */
static void _fill_voxels(uint8_t voxels[FIELD_SIZE * FIELD_SIZE * FIELD_SIZE])
{
    for (uint32_t z = 0; z < FIELD_SIZE; z++)
    {
        const float nz = 2.0f * (float)z / (float)(FIELD_SIZE - 1u) - 1.0f;
        for (uint32_t y = 0; y < FIELD_SIZE; y++)
        {
            const float ny = 2.0f * (float)y / (float)(FIELD_SIZE - 1u) - 1.0f;
            for (uint32_t x = 0; x < FIELD_SIZE; x++)
            {
                const float nx = 2.0f * (float)x / (float)(FIELD_SIZE - 1u) - 1.0f;
                const float shell =
                    expf(-13.0f * fabsf(nx * nx + 1.3f * ny * ny + 0.7f * nz * nz - 0.40f));
                const float knot = expf(
                    -12.0f *
                    ((nx - 0.18f) * (nx - 0.18f) + ny * ny + (nz + 0.10f) * (nz + 0.10f)));
                voxels[(z * FIELD_SIZE + y) * FIELD_SIZE + x] = _u8(0.70f * shell + 0.45f * knot);
            }
        }
    }
}



/**
 * Create and populate the 3D sampled field.
 *
 * @param scene scene owning the field
 * @param state state owning CPU voxel memory
 * @return sampled field, or NULL
 */
static DvzSampledField* _field(DvzScene* scene, VolumeOcclusionState* state)
{
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc), .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM, .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE, .height = FIELD_SIZE, .depth = FIELD_SIZE});
    if (field == NULL)
        return NULL;
    if (dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = state->voxels,
                       .bytes_per_row = FIELD_SIZE, .rows_per_image = FIELD_SIZE}) != DVZ_OK)
        return NULL;
    return field;
}



/**
 * Attach a shared colormap and alpha ramp to a volume visual.
 *
 * @param scene scene owning the scale
 * @param visual volume visual
 * @return true on success
 */
static bool _attach_transfer(DvzScene* scene, DvzVisual* visual)
{
    DvzScale* scale = dvz_scale(
        scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return false;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return false;
    DvzColormapStop stops[4] = {
        {.position = 0.00, .rgba = {12, 16, 24, 255}},
        {.position = 0.30, .rgba = {30, 86, 130, 255}},
        {.position = 0.66, .rgba = {76, 201, 240, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 4);
    dvz_scale_set_colormap(scale, colormap);

    DvzVolumeAlphaStop alpha[4] = {
        {.position = 0.00, .alpha = 0.00f},
        {.position = 0.26, .alpha = 0.00f},
        {.position = 0.58, .alpha = 0.34f},
        {.position = 1.00, .alpha = 0.90f},
    };
    if (dvz_volume_set_alpha_stops(visual, alpha, 4) != 0)
        return false;
    return dvz_visual_set_scale(visual, "color", scale) == 0;
}



/**
 * Configure a 3D volume visual.
 *
 * @param scene scene owning scales
 * @param visual volume visual
 * @param mode volume render mode
 * @return true on success
 */
static bool _configure_volume(DvzScene* scene, DvzVisual* visual, DvzVolumeRenderMode mode)
{
    const double bounds_min[3] = {-0.90, -0.74, -0.88};
    const double bounds_max[3] = {+0.90, +0.74, +0.88};

    if (!_attach_transfer(scene, visual))
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_volume_set_bounds(visual, bounds_min, bounds_max) != 0)
        return false;
    if (dvz_volume_set_value_range(visual, 0.0, 1.0) != 0)
        return false;
    if (dvz_volume_set_opacity(visual, 0.88f) != 0)
        return false;
    if (dvz_volume_set_step_count(visual, 80u) != 0)
        return false;
    return dvz_volume_set_render_mode(visual, mode) == 0;
}



/**
 * Add the volume/slice pair to one panel, optionally enabling slice attenuation by the volume.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param field sampled scalar field
 * @param occlusion_enabled whether the slice should sample volume occlusion
 * @return true on success
 */
static bool _add_volume_pair(
    DvzScene* scene, DvzPanel* panel, DvzSampledField* field, bool occlusion_enabled)
{
    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    if (volume == NULL || slice == NULL)
        return false;
    if (dvz_visual_set_field(volume, "field", field) != DVZ_OK)
        return false;
    if (dvz_visual_set_field(slice, "field", field) != DVZ_OK)
        return false;
    if (!_configure_volume(scene, volume, DVZ_VOLUME_RENDER_COMPOSITE))
        return false;
    if (!_configure_volume(scene, slice, DVZ_VOLUME_RENDER_SLICE))
        return false;
    if (dvz_visual_set_volume_occluded(slice, occlusion_enabled) != 0)
        return false;

    if (dvz_panel_add_visual(
            panel, volume,
            &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) !=
        0)
        return false;
    if (dvz_panel_add_visual(
            panel, slice,
            &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) !=
        0)
        return false;

    if (occlusion_enabled)
    {
        DvzVolumeOcclusionDesc occlusion = dvz_volume_occlusion_desc();
        occlusion.enabled = true;
        occlusion.alpha_threshold = 0.02f;
        occlusion.fade_distance = 0.05f;
        occlusion.occluded_alpha = 0.24f;
        if (dvz_panel_set_volume_occluder(panel, volume, &occlusion) != 0)
            return false;
    }
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the volume-occlusion feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    VolumeOcclusionState* state =
        (VolumeOcclusionState*)dvz_calloc(1, sizeof(VolumeOcclusionState));
    if (state == NULL)
        return false;
    _fill_voxels(state->voxels);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        goto error;
    if (dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}) != DVZ_OK)
        goto error;
    if (dvz_grid_set_gutter(grid, 30.0f, 0.0f) != DVZ_OK)
        goto error;

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* occluded = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || occluded == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(occluded);
    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(occluded, 1.0f) == NULL)
        goto error;

    DvzSampledField* field = _field(ctx->scene, state);
    if (field == NULL)
        goto error;
    if (!_add_volume_pair(ctx->scene, plain, field, false))
        goto error;
    if (!_add_volume_pair(ctx->scene, occluded, field, true))
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Destroy the volume-occlusion feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the volume-occlusion scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _volume_occlusion_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_volume_occlusion",
        .title = "Volume Occlusion",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the volume-occlusion feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _volume_occlusion_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
