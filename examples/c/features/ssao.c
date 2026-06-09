/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* ssao - screen-space ambient occlusion on normal-producing mesh geometry.
 *
 * Scenario: feature.ssao
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/ssao
 * Run:    ./build/examples/c/features/ssao --live
 * Smoke:  ./build/examples/c/features/ssao --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Bind copied index data to a mesh.
 *
 * @param scene scene owning the buffer
 * @param mesh mesh visual
 * @param indices index array
 * @param index_count number of indices
 * @return true on success
 */
static bool
_set_indices(DvzScene* scene, DvzVisual* mesh, const DvzIndex* indices, uint32_t index_count)
{
    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    desc.stride = sizeof(DvzIndex);
    desc.byte_size = (uint64_t)index_count * sizeof(DvzIndex);

    DvzSceneBuffer* buffer = dvz_scene_buffer(scene, &desc);
    if (buffer == NULL)
        return false;
    if (!dvz_scene_buffer_set_data(buffer, indices, desc.byte_size))
        return false;
    return dvz_visual_set_buffer(mesh, "index", buffer);
}



/**
 * Add a small folded mesh that has readable normals for the SSAO G-buffer.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_occlusion_mesh(DvzScene* scene, DvzPanel* panel)
{
    const vec3 positions[8] = {
        {-0.82f, -0.70f, 0.05f}, {+0.82f, -0.70f, 0.05f}, {-0.82f, -0.04f, 0.34f},
        {+0.82f, -0.04f, 0.34f}, {-0.64f, +0.08f, 0.08f}, {+0.64f, +0.08f, 0.08f},
        {-0.64f, +0.72f, 0.36f}, {+0.64f, +0.72f, 0.36f},
    };
    const vec3 normals[8] = {
        {0.0f, -0.38f, +0.92f}, {0.0f, -0.38f, +0.92f}, {0.0f, -0.38f, +0.92f},
        {0.0f, -0.38f, +0.92f}, {0.0f, +0.36f, +0.93f}, {0.0f, +0.36f, +0.93f},
        {0.0f, +0.36f, +0.93f}, {0.0f, +0.36f, +0.93f},
    };
    DvzColor colors[8] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
    };
    const DvzIndex indices[12] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7};

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 8},
        {.attr_name = "normal", .data = normals, .item_count = 8},
        {.attr_name = "color", .data = colors, .item_count = 8},
    };
    if (dvz_visual_set_data_many(mesh, updates, 3) != 0)
        return false;
    if (!_set_indices(scene, mesh, indices, 12))
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.phong.ambient = 0.30f;
    material.phong.diffuse = 0.78f;
    material.phong.specular = 0.08f;
    material.phong.shininess = 18.0f;
    if (dvz_visual_set_material(mesh, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, mesh, NULL) == 0;
}



/**
 * Add three small opaque spheres close to the mesh.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_spheres(DvzScene* scene, DvzPanel* panel)
{
    const vec3 positions[3] = {
        {-0.42f, -0.12f, 0.50f},
        {+0.08f, -0.18f, 0.58f},
        {+0.44f, +0.18f, 0.46f},
    };
    const float radii[3] = {0.18f, 0.24f, 0.16f};
    DvzColor colors[3] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
    };

    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (spheres == NULL)
        return false;
    if (dvz_sphere_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "radius", .data = radii, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
    };
    if (dvz_visual_set_data_many(spheres, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, spheres, NULL) == 0;
}



/**
 * Set the shared SSAO camera.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.00f;
    camera.eye[2] = 1.40f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.60f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    return dvz_panel_set_camera(panel, &camera) != NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the SSAO feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);
    if (!_set_camera(panel))
        return false;
    if (!_add_occlusion_mesh(ctx->scene, panel) || !_add_spheres(ctx->scene, panel))
        return false;

    DvzSsaoDesc ssao = dvz_ssao_desc();
    ssao.radius = 1.10f;
    ssao.strength = 2.80f;
    ssao.bias = 0.025f;
    ssao.power = 1.30f;
    ssao.min_visibility = 0.36f;
    ssao.sample_count = 16u;
    ssao.blur_enabled = true;
    ssao.blur_radius = 2.0f;
    return dvz_panel_set_ssao(panel, &ssao);
}



/**
 * Return the SSAO scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _ssao_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_ssao",
        .title = "ssao",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the SSAO feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _ssao_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
