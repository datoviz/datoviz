/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* sphere - deterministic 3D impostor-sphere cluster with depth, radius, and color variation.
 *
 * Scenario: visual.sphere
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/sphere
 * Run:    ./build/examples/c/visuals/sphere
 * Smoke:  ./build/examples/c/visuals/sphere 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/sphere 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define SPHERE_COUNT 42u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic compact sphere cluster.
 *
 * @param positions output sphere centers
 * @param radii output sphere radii
 * @param colors output sphere colors
 */
static void _fill_spheres(
    vec3 positions[SPHERE_COUNT], float radii[SPHERE_COUNT], DvzColor colors[SPHERE_COUNT])
{
    ANN(positions);
    ANN(radii);
    ANN(colors);

    const ExampleStyleColorRole palette[] = {
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < SPHERE_COUNT; i++)
    {
        const float t = (float)i / (float)(SPHERE_COUNT - 1u);
        const float angle = TAU * (0.145f * (float)i + 0.07f * sinf(11.0f * t));
        const float layer = 2.0f * t - 1.0f;
        const float ring = 0.36f + 0.50f * sqrtf(1.0f - 0.58f * layer * layer);
        const float wobble = 0.5f + 0.5f * sinf(17.0f * t + 0.4f);

        positions[i][0] = ring * cosf(angle);
        positions[i][1] = 0.70f * layer + 0.10f * sinf(3.0f * angle);
        positions[i][2] = ring * sinf(angle) + 0.26f * layer;
        radii[i] = 0.075f + 0.070f * (1.0f - fabsf(layer)) + 0.028f * wobble;

        const ExampleStyleColorRole role = palette[i % DVZ_ARRAY_COUNT(palette)];
        colors[i] = example_graphite_cyan_color(role);
    }
}



/**
 * Add one retained sphere visual to the panel.
 *
 * @param scene scene owning visual
 * @param panel panel receiving visual
 * @return true when the visual was added
 */
static bool _add_spheres(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[SPHERE_COUNT] = {{0}};
    float radii[SPHERE_COUNT] = {0};
    DvzColor colors[SPHERE_COUNT] = {{0}};
    _fill_spheres(positions, radii, colors);

    DvzVisual* visual = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
        return false;
    if (dvz_sphere_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_STANDARD;
    material.light_direction[0] = 0.30f;
    material.light_direction[1] = 0.62f;
    material.light_direction[2] = 0.72f;
    material.standard.roughness = 0.42f;
    material.standard.specular = 0.58f;
    material.standard.rim_strength = 0.14f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained sphere visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_sphere");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    bool capture_started = false;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.12f;
    camera_desc.eye[1] = -3.20f;
    camera_desc.eye[2] = 0.78f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.56f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    EXAMPLE_CHECK(dvz_panel_set_camera(panel, &camera_desc), "dvz_panel_set_camera() failed");

    EXAMPLE_CHECK(_add_spheres(scene, panel), "sphere visual setup failed");

    DvzMsaaDesc msaa_desc = dvz_msaa_desc();
    msaa_desc.sample_count = 8;
    msaa_desc.alpha_to_coverage = true;
    (void)dvz_panel_set_msaa(panel, &msaa_desc);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_sphere");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_set(arcball, (vec3){+0.55f, -0.18f, +0.22f});

    int rc = dvz_view_capture_start(win, &capture);
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
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
