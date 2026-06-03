/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* point_cloud - RESEPI RGB LiDAR dense point cloud.
 *
 * Scenario: point_cloud
 * Style: showcase, graphite_cyan, 1600x1200 capture target
 *
 * Real data is loaded from `.cache/datoviz/examples/point_cloud/prepared/point_cloud.bin`.
 * Generate it from the upstream LAZ source with:
 *
 *   python tools/data/prepare_point_cloud.py
 *
 * Build:  just example-c showcases/point_cloud
 * Run:    ./build/examples/c/showcases/point_cloud
 * Smoke:  ./build/examples/c/showcases/point_cloud 60
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/showcases/point_cloud 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_debug.h"
#include "example_gui_controls.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define POINT_CLOUD_MAGIC      "DVZPCD1"
#define POINT_CLOUD_MAGIC_SIZE 8u
#define POINT_CLOUD_VERSION    2u
#define POINT_CLOUD_MAX_POINTS 8000000u

#define CACHE_POINT_CLOUD_PATH ".cache/datoviz/examples/point_cloud/prepared/point_cloud.bin"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PointCloudHeader
{
    char magic[POINT_CLOUD_MAGIC_SIZE];
    uint32_t version;
    uint32_t count;
    vec3 bounds_min;
    vec3 bounds_max;
} PointCloudHeader;


typedef struct PointCloudRecord
{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float a;
    float pixel_size;
} PointCloudRecord;


typedef struct PointCloudData
{
    vec3* positions;
    DvzColor* colors;
    float* pixel_sizes;
    uint32_t count;
} PointCloudData;


typedef struct PointCloudGuiState
{
    DvzPanel* panel;
    DvzExampleGuiEdlControls edl;
} PointCloudGuiState;



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
 * Convert a normalized color channel to an 8-bit channel.
 *
 * @param value normalized channel
 * @return 8-bit channel
 */
static uint8_t _u8(float value)
{
    value = _clamp01(value);
    return (uint8_t)(value * 255.0f + 0.5f);
}



/**
 * Try to load a prepared point-cloud binary file.
 *
 * @param path binary path
 * @param data output point-cloud data
 * @return whether loading succeeded
 */
static bool _load_binary(const char* path, PointCloudData* data)
{
    ANN(path);
    ANN(data);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;

    bool ok = false;
    PointCloudHeader header = {0};
    if (fread(&header, sizeof(header), 1, fp) != 1)
        goto cleanup;
    if (memcmp(header.magic, POINT_CLOUD_MAGIC, strlen(POINT_CLOUD_MAGIC)) != 0 ||
        header.version != POINT_CLOUD_VERSION || header.count == 0 ||
        header.count > POINT_CLOUD_MAX_POINTS)
    {
        goto cleanup;
    }

    data->positions = (vec3*)dvz_calloc(header.count, sizeof(*data->positions));
    data->colors = (DvzColor*)dvz_calloc(header.count, sizeof(*data->colors));
    data->pixel_sizes = (float*)dvz_calloc(header.count, sizeof(*data->pixel_sizes));
    if (data->positions == NULL || data->colors == NULL || data->pixel_sizes == NULL)
        goto cleanup;

    for (uint32_t i = 0; i < header.count; i++)
    {
        PointCloudRecord record = {0};
        if (fread(&record, sizeof(record), 1, fp) != 1)
            goto cleanup;
        data->positions[i][0] = record.x;
        data->positions[i][1] = record.z;
        data->positions[i][2] = record.y;
        data->colors[i].r = _u8(record.r);
        data->colors[i].g = _u8(record.g);
        data->colors[i].b = _u8(record.b);
        data->colors[i].a = _u8(record.a);
        data->pixel_sizes[i] = record.pixel_size;
    }
    data->count = header.count;
    ok = true;

cleanup:
    fclose(fp);
    if (!ok)
    {
        dvz_free(data->pixel_sizes);
        dvz_free(data->colors);
        dvz_free(data->positions);
        memset(data, 0, sizeof(*data));
    }
    return ok;
}



/**
 * Load prepared point-cloud data from cache.
 *
 * @param data output point-cloud data
 * @return whether real prepared data is available
 */
static bool _load_data(PointCloudData* data)
{
    ANN(data);
    memset(data, 0, sizeof(*data));

    if (_load_binary(CACHE_POINT_CLOUD_PATH, data))
        return true;

    dvz_fprintf(
        stderr, "point_cloud: missing real v2 prepared data. Run "
                "`python tools/data/prepare_point_cloud.py --force` from the repository root.\n");
    return false;
}



/**
 * Free point-cloud arrays.
 *
 * @param data point-cloud data
 */
static void _free_data(PointCloudData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->pixel_sizes);
    dvz_free(data->colors);
    dvz_free(data->positions);
    memset(data, 0, sizeof(*data));
}



/**
 * Upload point-cloud arrays to a retained pixel visual.
 *
 * @param visual pixel visual
 * @param data point-cloud data
 * @return whether upload succeeded
 */
static bool _upload_pixels(DvzVisual* visual, const PointCloudData* data)
{
    ANN(visual);
    ANN(data);
    ANN(data->positions);
    ANN(data->colors);
    ANN(data->pixel_sizes);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data->positions, .item_count = data->count},
        {.attr_name = "color", .data = data->colors, .item_count = data->count},
        {.attr_name = "pixel_size", .data = data->pixel_sizes, .item_count = data->count},
    };
    return dvz_visual_set_data_many(visual, updates, DVZ_ARRAY_COUNT(updates)) == 0;
}


/**
 * Reset EDL controls to the point-cloud defaults.
 *
 * @param state GUI state
 */
static void _reset_edl(PointCloudGuiState* state)
{
    ANN(state);
    state->edl = (DvzExampleGuiEdlControls){
        .enabled = true,
        .radius = 1.8f,
        .strength = 34.0f,
        .depth_scale = 1.0f,
    };
}


/**
 * Apply current EDL controls to the panel.
 *
 * @param state GUI state
 */
static void _apply_edl(PointCloudGuiState* state)
{
    ANN(state);
    ANN(state->panel);

    DvzEdlDesc edl = {
        DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc),
        .radius = state->edl.enabled ? state->edl.radius : 0.0f,
        .strength = state->edl.enabled ? state->edl.strength : 0.0f,
        .depth_scale = state->edl.depth_scale,
    };
    (void)dvz_panel_set_edl(state->panel, &edl);
}


/**
 * Draw point-cloud GUI controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data GUI state
 */
static void _point_cloud_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    PointCloudGuiState* state = (PointCloudGuiState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool reset = false;
    if (dvz_gui_begin(gui, "Point Cloud", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Depth");
        changed |= dvz_example_gui_edl(gui, &state->edl);
        reset = dvz_gui_button(gui, "Reset EDL");
    }
    dvz_gui_end(gui);

    if (reset)
    {
        _reset_edl(state);
        changed = true;
    }
    if (changed)
        _apply_edl(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the point-cloud showcase example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    PointCloudData data = {0};
    PointCloudGuiState gui_state = {0};
    ExampleDebug debug = {0};
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("showcase_point_cloud");

    EXAMPLE_CHECK(_load_data(&data), "point-cloud data setup failed");
    dvz_fprintf(stderr, "point_cloud: %u points (prepared real data)\n", data.count);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.type = 0;
    camera_desc.eye[0] = +0.791911f;
    camera_desc.eye[1] = +0.472144f;
    camera_desc.eye[2] = -0.891266f;
    camera_desc.target[0] = +0.207716f;
    camera_desc.target[1] = +0.133955f;
    camera_desc.target[2] = -0.153469f;
    camera_desc.up[0] = -0.209938f;
    camera_desc.up[1] = +0.941078f;
    camera_desc.up[2] = +0.265137f;
    camera_desc.fov_y = 0.700000f;
    camera_desc.near = 0.020000f;
    camera_desc.far = 100.000000f;
    camera_desc.ortho_height = 2.000000f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(camera != NULL, "dvz_panel_set_camera() failed");

    gui_state.panel = panel;
    _reset_edl(&gui_state);
    _apply_edl(&gui_state);

    DvzVisual* pixels = dvz_pixel(scene, 0);
    EXAMPLE_CHECK(pixels != NULL, "dvz_pixel() failed");

    EXAMPLE_CHECK(_upload_pixels(pixels, &data), "point-cloud upload failed");

    int rc = dvz_visual_set_depth_test(pixels, true);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, pixels, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "point_cloud");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzFlyDesc fly_desc = dvz_fly_desc();
    fly_desc.mode = DVZ_FLY_MODE_PLANE;
    fly_desc.controller_flags = DVZ_FLY_FLAGS_FIXED_UP | DVZ_FLY_FLAGS_DISABLE_ROLL;
    fly_desc.position[0] = camera_desc.eye[0];
    fly_desc.position[1] = camera_desc.eye[1];
    fly_desc.position[2] = camera_desc.eye[2];
    fly_desc.up[0] = 0.0f;
    fly_desc.up[1] = 1.0f;
    fly_desc.up[2] = 0.0f;
    fly_desc.speed = 0.55f;
    DvzFly* fly = dvz_view_fly(win, panel, &fly_desc);
    EXAMPLE_CHECK(fly != NULL, "failed to create or bind fly controller");

    EXAMPLE_CHECK(
        example_debug_setup(&debug, win, argc, argv, "point_cloud"),
        "example_debug_setup() failed");
    example_debug_camera_ref(&debug, "point_cloud", camera, &camera_desc);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _point_cloud_gui, &gui_state);

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    example_debug_uninstall(&debug);
    if (app != NULL)
        dvz_app_destroy(app);
    _free_data(&data);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
