/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* surface_grid - generated height-field mesh via geom -> scene mesh upload.
 *
 * Opens a GLFW window showing a lit structured surface generated with `dvz_geom_surface_grid()`
 * and uploaded through `dvz_mesh_geometry()`. This example is the first visible pressure test for
 * the v0.4 CPU geom -> mesh visual path.
 *
 * Build:  just example-c visuals/surface_grid
 * Run:    ./build/examples/c/visuals/surface_grid
 * Smoke:  ./build/examples/c/visuals/surface_grid 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

#define SURFACE_ROWS 256
#define SURFACE_COLS 256



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic surface field and color ramp.
 *
 * @param heights output height buffer
 * @param colors output color buffer
 */
static void _surface_data(double* heights, DvzColor* colors)
{
    ANN(heights);
    ANN(colors);

    for (uint32_t row = 0; row < SURFACE_ROWS; row++)
    {
        const double y = -1.0 + 2.0 * (double)row / (double)(SURFACE_ROWS - 1);
        for (uint32_t col = 0; col < SURFACE_COLS; col++)
        {
            const double x = -1.0 + 2.0 * (double)col / (double)(SURFACE_COLS - 1);
            const double r = sqrt(x * x + y * y);
            const double r2 = r * r;
            const double radial = 0.30 * cos(18.0 * r) * exp(-1.25 * r2);
            const double cross = 0.07 * sin(13.0 * x + 2.0 * y) * cos(12.0 * y - 1.5 * x);
            const double ripple = 0.035 * sin(26.0 * (x + y)) * exp(-0.65 * r2);
            const double wave = radial + cross + ripple;
            const uint32_t idx = row * SURFACE_COLS + col;
            heights[idx] = wave;

            const double t = CLIP((wave + 0.36) / 0.72, 0.0, 1.0);
            colors[idx][0] = (uint8_t)(28.0 + 210.0 * t);
            colors[idx][1] = (uint8_t)(70.0 + 135.0 * (1.0 - fabs(2.0 * t - 1.0)));
            colors[idx][2] = (uint8_t)(150.0 + 85.0 * (1.0 - t));
            colors[idx][3] = 255;
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    const uint32_t vertex_count = SURFACE_ROWS * SURFACE_COLS;

    double* heights = (double*)dvz_calloc(vertex_count, sizeof(double));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    if (heights == NULL || colors == NULL)
    {
        dvz_fprintf(stderr, "surface_grid: allocation failed\n");
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }
    _surface_data(heights, colors);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    if (figure == NULL || panel == NULL)
    {
        dvz_fprintf(stderr, "scene setup failed\n");
        dvz_scene_destroy(scene);
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 1.8f;
    camera_desc.eye[1] = -2.2f;
    camera_desc.eye[2] = 1.5f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }

    DvzGeometrySurfaceGridDesc desc = {
        .rows = SURFACE_ROWS,
        .cols = SURFACE_COLS,
        .heights = heights,
        .colors = colors,
        .origin = {-1.0, -1.0, 0.0},
        .col_basis = {2.0 / (double)(SURFACE_COLS - 1), 0.0, 0.0},
        .row_basis = {0.0, 2.0 / (double)(SURFACE_ROWS - 1), 0.0},
        .height_axis = {0.0, 0.0, 1.0},
        .height_scale = 1.0,
    };
    DvzGeometry* geometry = dvz_geom_surface_grid(&desc);
    if (geometry == NULL)
    {
        dvz_fprintf(stderr, "dvz_geom_surface_grid() failed\n");
        dvz_scene_destroy(scene);
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL || dvz_mesh_geometry(visual, geometry) != 0)
    {
        dvz_fprintf(stderr, "dvz_mesh_geometry() failed\n");
        dvz_geometry_destroy(geometry);
        dvz_scene_destroy(scene);
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = -0.45f;
    material.light_direction[2] = 0.82f;
    material.phong.ambient = 0.18f;
    material.phong.diffuse = 0.78f;
    material.phong.specular = 0.85f;
    material.phong.shininess = 96.0f;
    dvz_visual_set_material(visual, &material);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.04f, 0.045f, 0.05f, 1.0f);

    DvzApp* app = dvz_app(scene);
    DvzAppWindow* win =
        app != NULL ? dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "surface grid") : NULL;
    if (app == NULL || win == NULL)
    {
        dvz_fprintf(stderr, "app/window setup failed\n");
        if (app != NULL)
            dvz_app_destroy(app);
        dvz_geometry_destroy(geometry);
        dvz_scene_destroy(scene);
        dvz_free(heights);
        dvz_free(colors);
        return 1;
    }

    DvzArcball* arcball = dvz_app_window_panel_arcball(win, panel, NULL);
    if (arcball != NULL)
        dvz_arcball_initial(arcball, (vec3){0.55f, 0.0f, -0.25f});

    dvz_app_run(app, frame_count);

    dvz_app_destroy(app);
    dvz_geometry_destroy(geometry);
    dvz_scene_destroy(scene);
    dvz_free(heights);
    dvz_free(colors);
    return 0;
}
