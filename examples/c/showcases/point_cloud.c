/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* point_cloud - RESEPI RGB LiDAR dense point cloud.
 *
 * Scenario: point_cloud
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Real data is loaded from `.cache/datoviz/examples/point_cloud/prepared/point_cloud.bin`.
 * Generate it from the upstream LAZ source with:
 *
 *   python tools/data/prepare_point_cloud.py --force
 *
 * Build:  just example-c showcases/point_cloud
 * Run:    ./build/examples/c/showcases/point_cloud --live
 * Smoke:  ./build/examples/c/showcases/point_cloud --png
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
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

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
    float pixel_size_px;
} PointCloudRecord;


typedef struct PointCloudData
{
    vec3* positions;
    DvzColor* colors;
    float* pixel_sizes;
    uint32_t count;
} PointCloudData;


typedef struct PointCloudState
{
    PointCloudData data;
} PointCloudState;



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
        data->pixel_sizes[i] = record.pixel_size_px;
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
        {.attr_name = "pixel_size_px", .data = data->pixel_sizes, .item_count = data->count},
    };
    return dvz_visual_set_data_many(visual, updates, DVZ_ARRAY_COUNT(updates)) == 0;
}


/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the point-cloud showcase scenario.
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
    PointCloudState* state = (PointCloudState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    EXAMPLE_CHECK(_load_data(&state->data), "point-cloud data setup failed");
    dvz_fprintf(stderr, "point_cloud: %u points (prepared real data)\n", state->data.count);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.projection.type = DVZ_CAMERA_PERSPECTIVE;
    camera_desc.view.eye[0] = +0.791911f;
    camera_desc.view.eye[1] = +0.472144f;
    camera_desc.view.eye[2] = -0.891266f;
    camera_desc.view.target[0] = +0.207716f;
    camera_desc.view.target[1] = +0.133955f;
    camera_desc.view.target[2] = -0.153469f;
    camera_desc.view.up[0] = -0.209938f;
    camera_desc.view.up[1] = +0.941078f;
    camera_desc.view.up[2] = +0.265137f;
    camera_desc.projection.fov_y = 0.700000f;
    camera_desc.projection.near_clip = 0.020000f;
    camera_desc.projection.far_clip = 100.000000f;
    camera_desc.projection.ortho_height = 2.000000f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    DvzCamera* camera = dvz_panel_camera(panel);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");
    EXAMPLE_CHECK(camera != NULL, "dvz_panel_set_camera_desc() failed");
    (void)camera;

    DvzEdlDesc edl = {
        DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc),
        .radius = 1.8f,
        .strength = 34.0f,
        .depth_scale = 1.0f,
    };
    (void)dvz_panel_set_edl(panel, &edl);

    DvzVisual* pixels = dvz_pixel(ctx->scene, 0);
    EXAMPLE_CHECK(pixels != NULL, "dvz_pixel() failed");

    EXAMPLE_CHECK(_upload_pixels(pixels, &state->data), "point-cloud upload failed");

    int rc = dvz_visual_set_depth_test(pixels, true);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, pixels, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzFlyDesc fly_desc = dvz_fly_desc();
    fly_desc.mode = DVZ_FLY_MODE_PLANE;
    fly_desc.controller_flags = DVZ_FLY_FLAGS_FIXED_UP | DVZ_FLY_FLAGS_DISABLE_ROLL;
    fly_desc.initial_view = camera_desc.view;
    DvzController* controller = dvz_fly(ctx->scene, &fly_desc);
    EXAMPLE_CHECK(controller != NULL, "dvz_fly() failed");
    DvzFly* fly = dvz_controller_fly(controller);
    EXAMPLE_CHECK(fly != NULL, "failed to create or bind fly controller");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");
    (void)fly;

    ok = true;
cleanup:
    return ok;
}



/**
 * Destroy the point-cloud showcase scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    PointCloudState* state = (PointCloudState*)user;
    if (state == NULL)
        return;
    _free_data(&state->data);
    dvz_free(state);
}



/**
 * Return the point-cloud showcase scenario.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _point_cloud_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "point_cloud",
        .title = "Point Cloud",
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
 * Run the point-cloud showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _point_cloud_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
