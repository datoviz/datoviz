/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gothic_splat - Gothic Gaussian-splat showcase.
 *
 * This example loads the v0.4 Gothic splat cache arrays, renders them with dvz_splat(), and uses a
 * fly controller like the LIDAR showcase.
 *
 * Prepare: python examples/c/showcase/prepare_gothic_splat.py
 * Build:   just example-c showcase/gothic_splat
 * Run:     ./build/examples/c/showcase/gothic_splat
 * Smoke:   ./build/examples/c/showcase/gothic_splat 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1100u
#define HEIGHT 760u

#define GOTHIC_SPLAT_DATA_DIR ".cache/datoviz/examples/gothic_splat/prepared"
#define GOTHIC_SPLAT_SOURCE   ".cache/datoviz/examples/gothic_splat/source/gothic.ply"

#define GOTHIC_SIGMA_SCALE 900.0f
#define GOTHIC_SIGMA_MIN   0.5f
#define GOTHIC_SIGMA_MAX   9.0f

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GothicSplatDataset GothicSplatDataset;


struct GothicSplatDataset
{
    vec3* positions;
    DvzColor* colors;
    vec2* sigma;
    float* angles;
    uint32_t count;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the configured local splat data directory.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return data directory path
 */
static const char* _data_dir(int argc, char** argv)
{
    const char* value = NULL;
    if (example_arg_value(argc, argv, "--data-dir", &value))
        return value;
    if (example_arg_value_prefix(argc, argv, "--data-dir=", &value))
        return value;
    return GOTHIC_SPLAT_DATA_DIR;
}



/**
 * Return the requested point sampling stride.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return sampling stride, clamped to at least one
 */
static uint32_t _stride(int argc, char** argv)
{
    const char* value = NULL;
    uint32_t stride = 1;
    if (example_arg_value(argc, argv, "--stride", &value) ||
        example_arg_value_prefix(argc, argv, "--stride=", &value))
    {
        if (!example_parse_u32(value, &stride))
            stride = 1;
    }
    return stride == 0 ? 1 : stride;
}



/**
 * Join a directory path and basename.
 *
 * @param dir directory path
 * @param basename file basename
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the joined path fit in the output buffer
 */
static bool _join_path(const char* dir, const char* basename, char* out, size_t out_size)
{
    ANN(dir);
    ANN(basename);
    ANN(out);

    int written = dvz_snprintf(out, out_size, "%s/%s", dir, basename);
    return written > 0 && (size_t)written < out_size;
}



/**
 * Print local splat-cache preparation instructions.
 *
 * @param data_dir expected data directory
 */
static void _print_prepare_hint(const char* data_dir)
{
    dvz_fprintf(
        stderr,
        "missing Gothic splat .npy data in %s\n"
        "put the source PLY at %s, then run:\n"
        "python examples/c/showcase/prepare_gothic_splat.py --normalize\n",
        data_dir != NULL ? data_dir : GOTHIC_SPLAT_DATA_DIR, GOTHIC_SPLAT_SOURCE);
}



/**
 * Clamp a screen-space Gaussian sigma to the configured display range.
 *
 * @param sigma unclamped splat standard deviation in pixels
 * @return splat standard deviation in pixels
 */
static float _clamp_sigma(float sigma)
{
    if (!isfinite(sigma))
        sigma = GOTHIC_SIGMA_MIN;
    if (sigma < GOTHIC_SIGMA_MIN)
        sigma = GOTHIC_SIGMA_MIN;
    if (sigma > GOTHIC_SIGMA_MAX)
        sigma = GOTHIC_SIGMA_MAX;
    return sigma;
}



/**
 * Convert a normalized quaternion to a 3x3 rotation matrix.
 *
 * @param q source quaternion in w, x, y, z order
 * @param out output row-major rotation matrix
 */
static void _quat_to_matrix(const vec4 q, float out[3][3])
{
    ANN(q);
    ANN(out);

    float w = q[0];
    float x = q[1];
    float y = q[2];
    float z = q[3];
    float norm = sqrtf(w * w + x * x + y * y + z * z);
    if (!isfinite(norm) || norm <= 1e-12f)
    {
        w = 1.0f;
        x = y = z = 0.0f;
    }
    else
    {
        float inv_norm = 1.0f / norm;
        w *= inv_norm;
        x *= inv_norm;
        y *= inv_norm;
        z *= inv_norm;
    }

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    out[0][0] = 1.0f - 2.0f * (yy + zz);
    out[0][1] = 2.0f * (xy - wz);
    out[0][2] = 2.0f * (xz + wy);
    out[1][0] = 2.0f * (xy + wz);
    out[1][1] = 1.0f - 2.0f * (xx + zz);
    out[1][2] = 2.0f * (yz - wx);
    out[2][0] = 2.0f * (xz - wy);
    out[2][1] = 2.0f * (yz + wx);
    out[2][2] = 1.0f - 2.0f * (xx + yy);
}



/**
 * Project one source 3D Gaussian shape to a screen-facing ellipse.
 *
 * @param scale source standard deviations along the local 3D axes
 * @param quat source orientation quaternion in w, x, y, z order
 * @param sigma output screen-space ellipse standard deviations in pixels
 * @param angle output screen-space major-axis angle in radians
 */
static void _screen_ellipse_from_splat(const vec3 scale, const vec4 quat, vec2 sigma, float* angle)
{
    ANN(scale);
    ANN(quat);
    ANN(sigma);
    ANN(angle);

    float r[3][3] = {{0}};
    _quat_to_matrix(quat, r);

    float sx2 = scale[0] * scale[0];
    float sy2 = scale[1] * scale[1];
    float sz2 = scale[2] * scale[2];
    if (!isfinite(sx2) || !isfinite(sy2) || !isfinite(sz2) || sx2 <= 0.0f || sy2 <= 0.0f ||
        sz2 <= 0.0f)
    {
        sigma[0] = GOTHIC_SIGMA_MIN;
        sigma[1] = GOTHIC_SIGMA_MIN;
        *angle = 0.0f;
        return;
    }

    float cxx = sx2 * r[0][0] * r[0][0] + sy2 * r[0][1] * r[0][1] +
                sz2 * r[0][2] * r[0][2];
    float cyy = sx2 * r[1][0] * r[1][0] + sy2 * r[1][1] * r[1][1] +
                sz2 * r[1][2] * r[1][2];
    /* The prepared cache mirrors Y positions; mirror the XY covariance block to match. */
    float cxy = -(sx2 * r[0][0] * r[1][0] + sy2 * r[0][1] * r[1][1] +
                  sz2 * r[0][2] * r[1][2]);

    float diff = cxx - cyy;
    float root = sqrtf(fmaxf(diff * diff + 4.0f * cxy * cxy, 0.0f));
    float major_var = 0.5f * (cxx + cyy + root);
    float minor_var = 0.5f * (cxx + cyy - root);
    if (!isfinite(major_var) || !isfinite(minor_var) || major_var <= 0.0f || minor_var <= 0.0f)
    {
        sigma[0] = GOTHIC_SIGMA_MIN;
        sigma[1] = GOTHIC_SIGMA_MIN;
        *angle = 0.0f;
        return;
    }

    sigma[0] = _clamp_sigma(sqrtf(major_var) * GOTHIC_SIGMA_SCALE);
    sigma[1] = _clamp_sigma(sqrtf(minor_var) * GOTHIC_SIGMA_SCALE);
    *angle = 0.5f * atan2f(2.0f * cxy, diff);
}



/**
 * Load the Gothic splat cache arrays and build screen-space splat ellipses.
 *
 * @param data_dir directory containing Gothic splat .npy arrays
 * @param stride sampling stride
 * @param dataset output dataset
 * @return whether loading and validation succeeded
 */
static bool _load_gothic_dataset(
    const char* data_dir, uint32_t stride, GothicSplatDataset* dataset)
{
    ANN(data_dir);
    ANN(dataset);
    if (stride == 0)
        stride = 1;

    char pos_path[1024] = {0};
    char color_path[1024] = {0};
    char scale_path[1024] = {0};
    char quat_path[1024] = {0};
    if (!_join_path(data_dir, "gothic_splat_pos.npy", pos_path, sizeof(pos_path)) ||
        !_join_path(data_dir, "gothic_splat_color.npy", color_path, sizeof(color_path)) ||
        !_join_path(data_dir, "gothic_splat_scale.npy", scale_path, sizeof(scale_path)) ||
        !_join_path(data_dir, "gothic_splat_quat.npy", quat_path, sizeof(quat_path)))
    {
        dvz_fprintf(stderr, "Gothic splat data path is too long\n");
        return false;
    }

    DvzSize pos_size = 0;
    DvzSize color_size = 0;
    DvzSize scale_size = 0;
    DvzSize quat_size = 0;
    vec3* positions = dvz_read_npy(pos_path, &pos_size);
    DvzColor* colors = dvz_read_npy(color_path, &color_size);
    vec3* scales = dvz_read_npy(scale_path, &scale_size);
    vec4* quats = dvz_read_npy(quat_path, &quat_size);
    if (positions == NULL || colors == NULL || scales == NULL || quats == NULL)
    {
        dvz_free(quats);
        dvz_free(scales);
        dvz_free(colors);
        dvz_free(positions);
        _print_prepare_hint(data_dir);
        return false;
    }

    if (pos_size == 0 || color_size == 0 || scale_size == 0 || quat_size == 0 ||
        pos_size % (3 * sizeof(float)) != 0 || color_size % sizeof(DvzColor) != 0 ||
        scale_size % (3 * sizeof(float)) != 0 || quat_size % (4 * sizeof(float)) != 0)
    {
        dvz_fprintf(stderr, "invalid Gothic splat .npy payload sizes\n");
        dvz_free(quats);
        dvz_free(scales);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    DvzSize source_count = pos_size / (3 * sizeof(float));
    DvzSize color_count = color_size / sizeof(DvzColor);
    DvzSize scale_count = scale_size / (3 * sizeof(float));
    DvzSize quat_count = quat_size / (4 * sizeof(float));
    if (source_count != color_count || source_count != scale_count || source_count != quat_count ||
        source_count > UINT32_MAX)
    {
        dvz_fprintf(stderr, "Gothic splat position/color/scale/quaternion counts do not match\n");
        dvz_free(quats);
        dvz_free(scales);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    DvzSize sampled_count = (source_count + stride - 1) / stride;
    if (sampled_count == 0 || sampled_count > UINT32_MAX)
    {
        dvz_fprintf(stderr, "invalid Gothic splat sampled point count\n");
        dvz_free(quats);
        dvz_free(scales);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    dataset->positions = (vec3*)dvz_calloc(sampled_count, sizeof(*dataset->positions));
    dataset->colors = (DvzColor*)dvz_calloc(sampled_count, sizeof(*dataset->colors));
    dataset->sigma = (vec2*)dvz_calloc(sampled_count, sizeof(*dataset->sigma));
    dataset->angles = (float*)dvz_calloc(sampled_count, sizeof(*dataset->angles));
    if (dataset->positions == NULL || dataset->colors == NULL || dataset->sigma == NULL ||
        dataset->angles == NULL)
    {
        dvz_fprintf(stderr, "unable to allocate Gothic splat buffers\n");
        dvz_free(dataset->angles);
        dvz_free(dataset->sigma);
        dvz_free(dataset->colors);
        dvz_free(dataset->positions);
        dataset->angles = NULL;
        dataset->sigma = NULL;
        dataset->colors = NULL;
        dataset->positions = NULL;
        dvz_free(quats);
        dvz_free(scales);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    uint32_t dst = 0;
    for (DvzSize src = 0; src < source_count; src += stride)
    {
        dataset->positions[dst][0] = positions[src][0];
        dataset->positions[dst][1] = positions[src][1];
        dataset->positions[dst][2] = positions[src][2];
        dataset->colors[dst] = colors[src];
        _screen_ellipse_from_splat(
            scales[src], quats[src], dataset->sigma[dst], &dataset->angles[dst]);
        dst++;
    }
    dataset->count = dst;

    dvz_free(quats);
    dvz_free(scales);
    dvz_free(colors);
    dvz_free(positions);

    dvz_fprintf(
        stderr, "loaded Gothic splat dataset with %u splats (stride=%u)\n", dataset->count, stride);
    return true;
}



/**
 * Release CPU-side Gothic splat arrays.
 *
 * @param dataset dataset to destroy
 */
static void _destroy_gothic_dataset(GothicSplatDataset* dataset)
{
    if (dataset == NULL)
        return;
    dvz_free(dataset->angles);
    dvz_free(dataset->sigma);
    dvz_free(dataset->colors);
    dvz_free(dataset->positions);
    *dataset = (GothicSplatDataset){0};
}



/**
 * Compute loose camera bounds for the normalized Gothic cache.
 *
 * @param dataset loaded dataset
 * @param center output center
 * @param radius output radius
 */
static void _dataset_bounds(const GothicSplatDataset* dataset, vec3 center, float* radius)
{
    ANN(dataset);
    ANN(center);
    ANN(radius);

    vec3 min_pos = {+FLT_MAX, +FLT_MAX, +FLT_MAX};
    vec3 max_pos = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (uint32_t i = 0; i < dataset->count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            float v = dataset->positions[i][j];
            if (v < min_pos[j])
                min_pos[j] = v;
            if (v > max_pos[j])
                max_pos[j] = v;
        }
    }

    center[0] = 0.5f * (min_pos[0] + max_pos[0]);
    center[1] = 0.5f * (min_pos[1] + max_pos[1]);
    center[2] = 0.5f * (min_pos[2] + max_pos[2]);

    float extent_x = max_pos[0] - min_pos[0];
    float extent_y = max_pos[1] - min_pos[1];
    float extent_z = max_pos[2] - min_pos[2];
    float extent = fmaxf(extent_x, fmaxf(extent_y, extent_z));
    *radius = extent > 0.0f ? 0.5f * extent : 1.0f;
}



/**
 * Apply a translation that recenters the loaded points around the scene origin.
 *
 * @param dataset loaded dataset
 * @param center center to subtract
 */
static void _center_dataset(GothicSplatDataset* dataset, const vec3 center)
{
    ANN(dataset);
    ANN(center);
    for (uint32_t i = 0; i < dataset->count; i++)
    {
        dataset->positions[i][0] -= center[0];
        dataset->positions[i][1] -= center[1];
        dataset->positions[i][2] -= center[2];
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the Gothic splat showcase.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int status = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    GothicSplatDataset dataset = {0};

    const char* data_dir = _data_dir(argc, argv);
    uint32_t stride = _stride(argc, argv);
    bool ok = _load_gothic_dataset(data_dir, stride, &dataset);
    EXAMPLE_CHECK(ok, "failed to load Gothic splat data");

    vec3 center = {0};
    float radius = 1.0f;
    _dataset_bounds(&dataset, center, &radius);
    _center_dataset(&dataset, center);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");

    vec3 eye = {0.0f, 0.0f, fmaxf(2.8f, 2.8f * radius)};
    vec3 target = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = eye[0];
    camera_desc.eye[1] = eye[1];
    camera_desc.eye[2] = eye[2];
    camera_desc.near = 0.01f;
    camera_desc.far = 100.0f;
    ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");
    dvz_panel_set_background_color(panel, 0.018f, 0.020f, 0.024f, 1.0f);

    DvzEdlDesc edl = {
        .radius = 2.0f,
        .strength = 0.42f,
        .depth_scale = 0.70f,
    };
    ok = dvz_panel_set_edl(panel, &edl);
    EXAMPLE_CHECK(ok, "dvz_panel_set_edl() failed");

    DvzVisual* visual = dvz_splat(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_splat() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = dataset.positions, .item_count = dataset.count},
        {.attr_name = "color", .data = dataset.colors, .item_count = dataset.count},
        {.attr_name = "sigma", .data = dataset.sigma, .item_count = dataset.count},
        {.attr_name = "angle", .data = dataset.angles, .item_count = dataset.count},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");

    rc = dvz_visual_set_depth_test(visual, true);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "gothic_splat");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzFlyDesc fly_desc = dvz_fly_desc();
    fly_desc.mode = DVZ_FLY_MODE_PLANE;
    dvz_memcpy(fly_desc.position, sizeof(fly_desc.position), eye, sizeof(eye));
    dvz_memcpy(fly_desc.target, sizeof(fly_desc.target), target, sizeof(target));
    dvz_memcpy(fly_desc.up, sizeof(fly_desc.up), up, sizeof(up));
    fly_desc.speed = fmaxf(0.12f, 0.35f * radius);
    DvzFly* fly = dvz_view_fly(win, panel, &fly_desc);
    EXAMPLE_CHECK(fly != NULL, "failed to create or bind fly controller");

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);
    dvz_app_run(app, example_frame_count(argc, argv));

    status = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    _destroy_gothic_dataset(&dataset);
    return status;
}
