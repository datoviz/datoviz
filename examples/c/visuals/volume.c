/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* volume - deterministic gyroid scalar field rendered with the retained volume visual.
 *
 * Scenario: visual.volume
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just volume
 * Run:    ./build/examples/c/visuals/volume
 * Smoke:  ./build/examples/c/visuals/volume 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/volume 90
 * Video:  DVZ_CAPTURE=mp4 ./build/examples/c/visuals/volume 360
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
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define FIELD_SIZE   128u
#define BOX_SEGMENTS  12u

#define ROTATION_SPEED_RAD_PER_SEC 0.16f

static const float TAU = 6.28318530718f;



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
static uint8_t _u8(float value)
{
    return (uint8_t)(255.0f * _clamp01(value) + 0.5f);
}



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
    const float shoulder = expf(-((fabsf(g1) - 0.34f) * (fabsf(g1) - 0.34f)) /
                                (2.0f * 0.135f * 0.135f));

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
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS});
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
        {.position = 0.00, .alpha = 0.00f},
        {.position = 0.09, .alpha = 0.00f},
        {.position = 0.18, .alpha = 0.08f},
        {.position = 0.32, .alpha = 0.32f},
        {.position = 0.52, .alpha = 0.62f},
        {.position = 0.74, .alpha = 0.86f},
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

    const double bounds_min[3] = {-0.88, -0.74, -1.04};
    const double bounds_max[3] = {+0.88, +0.74, +1.04};

    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_volume_set_bounds(visual, bounds_min, bounds_max) != 0)
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

    const float xmin = -0.88f, xmax = +0.88f;
    const float ymin = -0.74f, ymax = +0.74f;
    const float zmin = -1.04f, zmax = +1.04f;
    const vec3 corners[8] = {
        {xmin, ymin, zmin},
        {xmax, ymin, zmin},
        {xmax, ymax, zmin},
        {xmin, ymax, zmin},
        {xmin, ymin, zmax},
        {xmax, ymin, zmax},
        {xmax, ymax, zmax},
        {xmin, ymax, zmax},
    };
    const uint32_t edges[BOX_SEGMENTS][2] = {
        {0, 1},
        {1, 2},
        {2, 3},
        {3, 0},
        {4, 5},
        {5, 6},
        {6, 7},
        {7, 4},
        {0, 4},
        {1, 5},
        {2, 6},
        {3, 7},
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
        {.attr_name = "stroke_width", .data = widths, .item_count = BOX_SEGMENTS},
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained volume visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_volume");
    const bool video_enabled = (capture.flags & DVZ_APP_CAPTURE_VIDEO) != 0;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    uint8_t* data = NULL;
    DvzView* win = NULL;
    bool capture_started = false;
    DvzExampleVisualSpin volume_spin = {0};
    DvzExampleVisualSpin box_spin = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -3.85f;
    camera_desc.eye[2] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.66f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    const uint64_t bytes = (uint64_t)FIELD_SIZE * FIELD_SIZE * FIELD_SIZE;
    data = (uint8_t*)dvz_malloc(bytes);
    EXAMPLE_CHECK(data != NULL, "volume allocation failed");
    _fill_volume(data, FIELD_SIZE);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE,
                   .height = FIELD_SIZE,
                   .depth = FIELD_SIZE});
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");

    ok = dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = data,
                   .bytes_per_row = FIELD_SIZE,
                   .rows_per_image = FIELD_SIZE});
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data() failed");
    dvz_free(data);
    data = NULL;

    DvzVisual* volume = dvz_volume(scene, 0);
    EXAMPLE_CHECK(volume != NULL, "dvz_volume() failed");
    ok = dvz_visual_set_field(volume, "field", field);
    EXAMPLE_CHECK(ok, "dvz_visual_set_field() failed");
    ok = _configure_volume(volume);
    EXAMPLE_CHECK(ok, "volume configuration failed");
    ok = _attach_transfer(scene, volume);
    EXAMPLE_CHECK(ok, "volume transfer setup failed");

    int rc = dvz_panel_add_visual(panel, volume, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    DvzVisual* box = NULL;
    ok = _add_boundary_box(scene, panel, &box);
    EXAMPLE_CHECK(ok, "boundary box setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_volume");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_set(arcball, (vec3){+0.50f, -0.10f, +0.24f});

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    EXAMPLE_CHECK(
        example_visual_spin(
            scene, volume, (vec3){0.0f, 0.0f, 1.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &volume_spin),
        "example_visual_spin(volume) failed");
    EXAMPLE_CHECK(
        example_visual_spin(
            scene, box, (vec3){0.0f, 0.0f, 1.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &box_spin),
        "example_visual_spin(box) failed");
    example_visual_spin_start(&volume_spin, 0.0);
    example_visual_spin_start(&box_spin, 0.0);

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    capture_started = false;
    ret = 0;

cleanup:
    if (capture_started && win != NULL)
        (void)dvz_view_capture_stop(win);
    if (app != NULL)
        dvz_app_destroy(app);
    example_visual_spin_destroy(&box_spin);
    example_visual_spin_destroy(&volume_spin);
    dvz_free(data);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
