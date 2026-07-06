/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* volume - This example volume-renders a generated 3D gyroid scalar field.
 *
 * What to look for: the FIELD_SIZE cubed data array is attached as a 3D sampled field, with color
 * and alpha scales controlling which values remain visible. Compare the rotating volume with the
 * boundary box to understand the data bounds, transfer-function opacity, and maximum-intensity
 * projection style used for volumetric microscopy, simulation, or tomography data.
 *
 * Scenario: visual.volume
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/volume
 * Run:    ./build/examples/c/visuals/volume --live
 * Smoke:  ./build/examples/c/visuals/volume --png
 * Video:  ./build/examples/c/visuals/volume --offscreen-record 360
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define FIELD_SIZE   128u
#define BOX_SEGMENTS 12u

#define ROTATION_SPEED_RAD_PER_SEC 0.16f

static const float TAU = 6.28318530718f;
static const double VOLUME_BOUNDS_MIN[3] = {-0.88, -0.74, -1.04};
static const double VOLUME_BOUNDS_MAX[3] = {+0.88, +0.74, +1.04};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VolumeState
{
    DvzTrack* volume_rotation;
    DvzTrack* box_rotation;
    DvzAnimation* volume_animation;
    DvzAnimation* box_animation;
} VolumeState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a scalar to [0, 1].
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
 * Convert a normalized scalar to an 8-bit value.
 *
 * @param value normalized value
 * @return clamped 8-bit value
 */
static uint8_t _u8(float value) { return (uint8_t)(255.0f * _clamp01(value) + 0.5f); }



/**
 * Smooth Hermite interpolation between two edges.
 *
 * @param edge0 lower edge
 * @param edge1 upper edge
 * @param value input value
 * @return smooth transition value
 */
static float _smoothstep(float edge0, float edge1, float value)
{
    if (edge0 == edge1)
        return value < edge0 ? 0.0f : 1.0f;
    const float t = _clamp01((value - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}



/**
 * Return one triply periodic gyroid implicit value.
 *
 * @param x normalized centered x coordinate
 * @param y normalized centered y coordinate
 * @param z normalized centered z coordinate
 * @return gyroid field value
 */
static float _gyroid_value(float x, float y, float z)
{
    const float scale = 1.16f * TAU;
    const float gx = scale * x;
    const float gy = scale * y;
    const float gz = scale * z;
    return sinf(gx) * cosf(gy) + sinf(gy) * cosf(gz) + sinf(gz) * cosf(gx);
}



/**
 * Return a bounded gyroid sheet density.
 *
 * @param x normalized centered x coordinate
 * @param y normalized centered y coordinate
 * @param z normalized centered z coordinate
 * @return normalized scalar sample
 */
static float _sample_volume(float x, float y, float z)
{
    const float g0 = _gyroid_value(x, y, z);
    const float g1 = _gyroid_value(x + 0.035f, y - 0.020f, z + 0.045f);
    const float sheet = expf(-(g0 * g0) / (2.0f * 0.115f * 0.115f));
    const float shoulder =
        expf(-((fabsf(g1) - 0.34f) * (fabsf(g1) - 0.34f)) / (2.0f * 0.135f * 0.135f));

    const float rx = x / 0.92f;
    const float ry = y / 0.86f;
    const float rz = z / 0.96f;
    const float r = sqrtf(rx * rx + ry * ry + rz * rz);
    const float envelope = 1.0f - _smoothstep(0.82f, 1.00f, r);
    const float rim = _smoothstep(0.72f, 0.94f, r) * (1.0f - _smoothstep(0.96f, 1.03f, r));

    const float directional_light = 0.78f + 0.22f * _clamp01(0.5f + 0.5f * (0.45f * x - y + z));
    const float lattice = envelope * directional_light * (0.88f * sheet + 0.16f * shoulder);
    return _clamp01(0.86f * lattice + 0.12f * rim * sheet);
}



/**
 * Fill one deterministic bounded gyroid scalar field.
 *
 * @param data output R8 volume data
 * @param size cubic field edge length
 */
static void _fill_volume(uint8_t* data, uint32_t size)
{
    ANN(data);
    ASSERT(size > 1u);

    for (uint32_t z = 0; z < size; z++)
    {
        const float nz = 2.0f * (float)z / (float)(size - 1u) - 1.0f;
        for (uint32_t y = 0; y < size; y++)
        {
            const float ny = 2.0f * (float)y / (float)(size - 1u) - 1.0f;
            for (uint32_t x = 0; x < size; x++)
            {
                const float nx = 2.0f * (float)x / (float)(size - 1u) - 1.0f;
                const float value = _sample_volume(nx, ny, nz);
                data[(z * size + y) * size + x] = _u8(value);
            }
        }
    }
}



/**
 * Attach a curated graphite/cyan volume transfer function.
 *
 * @param scene scene owning scale resources
 * @param visual volume visual
 * @return true when the transfer resources were attached
 */
static bool _attach_transfer(DvzScene* scene, DvzVisual* visual)
{
    ANN(scene);
    ANN(visual);

    DvzScale* scale = dvz_scale(
        scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return false;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return false;

    DvzColormapStop stops[8] = {
        {.position = 0.00, .rgba = {14, 17, 23, 255}},
        {.position = 0.10, .rgba = {14, 17, 23, 255}},
        {.position = 0.24, .rgba = {18, 58, 96, 255}},
        {.position = 0.40, .rgba = {44, 166, 209, 255}},
        {.position = 0.56, .rgba = {76, 201, 240, 255}},
        {.position = 0.74, .rgba = {128, 255, 219, 255}},
        {.position = 0.92, .rgba = {238, 252, 232, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 8);
    dvz_scale_set_colormap(scale, colormap);

    DvzVolumeAlphaStop alpha[7] = {
        {.position = 0.00, .alpha = 0.00f}, {.position = 0.09, .alpha = 0.00f},
        {.position = 0.18, .alpha = 0.08f}, {.position = 0.32, .alpha = 0.32f},
        {.position = 0.52, .alpha = 0.62f}, {.position = 0.74, .alpha = 0.86f},
        {.position = 1.00, .alpha = 0.98f},
    };
    if (dvz_volume_set_alpha_stops(visual, alpha, 7) != 0)
        return false;
    return dvz_visual_set_scale(visual, "color", scale) == 0;
}



/**
 * Configure visual-family volume rendering defaults.
 *
 * @param visual volume visual
 * @return true when all retained controls were applied
 */
static bool _configure_volume(DvzVisual* visual)
{
    ANN(visual);

    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_volume_set_bounds(visual, VOLUME_BOUNDS_MIN, VOLUME_BOUNDS_MAX) != 0)
        return false;
    if (dvz_volume_set_value_range(visual, 0.0, 0.88) != 0)
        return false;
    if (dvz_volume_set_opacity(visual, 1.0f) != 0)
        return false;
    if (dvz_volume_set_step_count(visual, 128u) != 0)
        return false;
    if (dvz_volume_set_render_mode(visual, DVZ_VOLUME_RENDER_MIP) != 0)
        return false;
    return true;
}



/**
 * Add a subtle bounding box around the rendered volume.
 *
 * @param scene scene owning the segment visual
 * @param panel panel receiving the visual
 * @param out optional created visual output
 * @return true when the boundary was added
 */
static bool _add_boundary_box(DvzScene* scene, DvzPanel* panel, DvzVisual** out)
{
    ANN(scene);
    ANN(panel);

    const float xmin = (float)VOLUME_BOUNDS_MIN[0], xmax = (float)VOLUME_BOUNDS_MAX[0];
    const float ymin = (float)VOLUME_BOUNDS_MIN[1], ymax = (float)VOLUME_BOUNDS_MAX[1];
    const float zmin = (float)VOLUME_BOUNDS_MIN[2], zmax = (float)VOLUME_BOUNDS_MAX[2];
    const vec3 corners[8] = {
        {xmin, ymin, zmin}, {xmax, ymin, zmin}, {xmax, ymax, zmin}, {xmin, ymax, zmin},
        {xmin, ymin, zmax}, {xmax, ymin, zmax}, {xmax, ymax, zmax}, {xmin, ymax, zmax},
    };
    const uint32_t edges[BOX_SEGMENTS][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
        {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    vec3 start[BOX_SEGMENTS] = {{0}};
    vec3 end[BOX_SEGMENTS] = {{0}};
    DvzColor colors[BOX_SEGMENTS] = {{0}};
    float widths[BOX_SEGMENTS] = {0};
    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < BOX_SEGMENTS; i++)
    {
        memcpy(start[i], corners[edges[i][0]], sizeof(vec3));
        memcpy(end[i], corners[edges[i][1]], sizeof(vec3));
        colors[i] = dvz_color_rgba(accent.r, accent.g, accent.b, 46);
        widths[i] = 1.0f;
    }

    DvzVisual* box = dvz_segment(scene, 0);
    if (box == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = start, .item_count = BOX_SEGMENTS},
        {.attr_name = "position_end", .data = end, .item_count = BOX_SEGMENTS},
        {.attr_name = "color", .data = colors, .item_count = BOX_SEGMENTS},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = BOX_SEGMENTS},
    };
    if (dvz_visual_set_data_many(box, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(box, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(box, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_panel_add_visual(panel, box, NULL) != 0)
        return false;
    if (out != NULL)
        *out = box;
    return true;
}



/**
 * Destroy the retained volume visual scenario state.
 *
 * @param state scenario state
 */
static void _volume_state_destroy(VolumeState* state)
{
    if (state == NULL)
        return;
    dvz_track_destroy(state->box_rotation);
    dvz_track_destroy(state->volume_rotation);
    dvz_free(state);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained volume visual scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    uint8_t* data = NULL;
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    VolumeState* state = (VolumeState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(example_set_default_3d_camera(panel, 1.15f), "dvz_panel_set_camera_desc() failed");

    const uint64_t bytes = (uint64_t)FIELD_SIZE * FIELD_SIZE * FIELD_SIZE;
    data = (uint8_t*)dvz_malloc(bytes);
    EXAMPLE_CHECK(data != NULL, "volume allocation failed");
    _fill_volume(data, FIELD_SIZE);

    DvzSampledField* field = dvz_sampled_field(
        ctx->scene, &(DvzSampledFieldDesc){
                        DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc), .dim = DVZ_FIELD_DIM_3D,
                        .format = DVZ_FIELD_FORMAT_R8_UNORM, .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                        .width = FIELD_SIZE, .height = FIELD_SIZE, .depth = FIELD_SIZE});
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");

    ok = dvz_sampled_field_set_data(
             field, &(DvzFieldDataView){
                        DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = data,
                        .bytes_per_row = FIELD_SIZE, .rows_per_image = FIELD_SIZE}) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data() failed");
    dvz_free(data);
    data = NULL;

    DvzVisual* volume = dvz_volume(ctx->scene, 0);
    EXAMPLE_CHECK(volume != NULL, "dvz_volume() failed");
    ok = dvz_visual_set_field(volume, "field", field) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_visual_set_field() failed");
    ok = _configure_volume(volume);
    EXAMPLE_CHECK(ok, "volume configuration failed");
    ok = _attach_transfer(ctx->scene, volume);
    EXAMPLE_CHECK(ok, "volume transfer setup failed");

    int rc = dvz_panel_add_visual(panel, volume, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    DvzVisual* box = NULL;
    ok = _add_boundary_box(ctx->scene, panel, &box);
    EXAMPLE_CHECK(ok, "boundary box setup failed");

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    state->volume_rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(state->volume_rotation != NULL, "dvz_track_rotation(volume) failed");
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = state->volume_rotation;
    state->volume_animation = dvz_anim_visual_transform(ctx->scene, volume, &transform_desc);
    EXAMPLE_CHECK(state->volume_animation != NULL, "dvz_anim_visual_transform(volume) failed");
    dvz_anim_set_interaction_policy(
        state->volume_animation, arcball_controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
    dvz_anim_set_speed(state->volume_animation, ROTATION_SPEED_RAD_PER_SEC);
    dvz_anim_start(state->volume_animation, 0.0);

    state->box_rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(state->box_rotation != NULL, "dvz_track_rotation(box) failed");
    transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = state->box_rotation;
    state->box_animation = dvz_anim_visual_transform(ctx->scene, box, &transform_desc);
    EXAMPLE_CHECK(state->box_animation != NULL, "dvz_anim_visual_transform(box) failed");
    dvz_anim_set_interaction_policy(
        state->box_animation, arcball_controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
    dvz_anim_set_speed(state->box_animation, ROTATION_SPEED_RAD_PER_SEC);
    dvz_anim_start(state->box_animation, 0.0);

    ok = true;
cleanup:
    dvz_free(data);
    return ok;
}



/**
 * Destroy the retained volume visual scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _volume_state_destroy((VolumeState*)user);
}



/**
 * Return the retained volume visual scenario.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _volume_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "volume",
        .title = "Volume",
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
 * Run the retained volume visual through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _volume_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
