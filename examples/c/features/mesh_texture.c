/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* mesh_texture - minimal UV textured mesh with a procedural RGBA8 texture.
 *
 * Scenario: feature.mesh_texture
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/mesh_texture
 * Run:    ./build/examples/c/features/mesh_texture
 * Smoke:  ./build/examples/c/features/mesh_texture 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/mesh_texture 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1600u
#define HEIGHT         1200u
#define TEXTURE_WIDTH  128u
#define TEXTURE_HEIGHT 64u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a procedural checker texture.
 *
 * @param pixels output RGBA8 texture
 */
static void _fill_texture(uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4])
{
    for (uint32_t y = 0; y < TEXTURE_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < TEXTURE_WIDTH; x++)
        {
            const uint32_t i = 4u * (y * TEXTURE_WIDTH + x);
            const bool checker = ((x / 16u) + (y / 16u)) % 2u == 0u;
            const bool equator = y >= TEXTURE_HEIGHT / 2u - 1u && y <= TEXTURE_HEIGHT / 2u + 1u;
            if (equator)
            {
                pixels[i + 0u] = 128u;
                pixels[i + 1u] = 255u;
                pixels[i + 2u] = 219u;
            }
            else if (checker)
            {
                pixels[i + 0u] = 76u;
                pixels[i + 1u] = 201u;
                pixels[i + 2u] = 240u;
            }
            else
            {
                pixels[i + 0u] = 18u;
                pixels[i + 1u] = 58u;
                pixels[i + 2u] = 96u;
            }
            pixels[i + 3u] = 255u;
        }
    }
}



/**
 * Create a scene-owned texture field.
 *
 * @param scene scene owning the field
 * @param pixels texture pixels
 * @return sampled field, or NULL on error
 */
static DvzSampledField* _add_texture(
    DvzScene* scene, const uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4])
{
    DvzSampledField* texture = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = TEXTURE_WIDTH,
                   .height = TEXTURE_HEIGHT,
                   .depth = 1});
    if (texture == NULL)
        return NULL;
    if (!dvz_sampled_field_set_data(
            texture, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                         .data = pixels,
                         .bytes_per_row = TEXTURE_WIDTH * 4u,
                         .rows_per_image = TEXTURE_HEIGHT}))
        return NULL;
    return texture;
}



/**
 * Add one textured sphere mesh.
 *
 * @param scene scene owning objects
 * @param panel panel receiving the visual
 * @param texture sampled RGBA8 texture
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_textured_mesh(
    DvzScene* scene, DvzPanel* panel, DvzSampledField* texture, DvzGeometry** out_geometry)
{
    DvzGeometry* sphere = dvz_geom_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = 0.82,
        .sectors = 48,
        .rings = 24,
        .color = {255, 255, 255, 255},
    });
    if (sphere == NULL)
        return false;
    if (out_geometry != NULL)
        *out_geometry = sphere;

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return false;
    if (!example_mesh_geometry(visual, sphere))
        return false;
    dvz_geometry_destroy(sphere);
    if (out_geometry != NULL)
        *out_geometry = NULL;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.20f;
    material.light_direction[1] = -0.35f;
    material.light_direction[2] = +0.70f;
    material.phong.ambient = 0.42f;
    material.phong.diffuse = 0.72f;
    material.phong.specular = 0.08f;
    material.phong.shininess = 18.0f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;
    if (!dvz_visual_set_field(visual, "texture", texture))
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the textured-mesh feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_mesh_texture");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    DvzGeometry* geometry = NULL;
    uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4] = {0};

    _fill_texture(pixels);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.0f;
    camera.eye[2] = 1.20f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.68f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    EXAMPLE_CHECK(dvz_panel_set_camera(panel, &camera), "dvz_panel_set_camera() failed");

    DvzSampledField* texture = _add_texture(scene, pixels);
    EXAMPLE_CHECK(texture != NULL, "texture field setup failed");
    EXAMPLE_CHECK(
        _add_textured_mesh(scene, panel, texture, &geometry), "textured mesh setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "mesh_texture");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(arcball != NULL, "dvz_controller_arcball() failed");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_set(arcball, (vec3){+0.50f, -0.18f, +0.26f});

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
