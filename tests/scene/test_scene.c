/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_scene.h"
#include "app.h"
#include "canvas.h"
#include "datoviz.h"
#include "scene/camera.h"
#include "scene/scene.h"
#include "scene/transform.h"
#include "scene/viewset.h"
#include "scene/visuals/mesh.h"
#include "scene/visuals/pixel.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

int test_scene_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panel contains.
    AT(dvz_panel_contains(panel, (vec2){0, 0}));
    AT(!dvz_panel_contains(panel, (vec2){WIDTH, HEIGHT}));
    AT(dvz_panel_at(figure, (vec2){WIDTH / 2, HEIGHT / 2}) == panel);
    AT(dvz_panel_at(figure, (vec2){WIDTH / 2, -1}) == NULL);

    // Panzoom.
    // DvzPanzoom* pz = dvz_panel_panzoom(panel);
    // ANN(pz);

    // Ortho.
    DvzOrtho* ortho = dvz_panel_ortho(panel, 0);
    ANN(ortho);

    // Create a visual.
    DvzVisual* pixel = dvz_pixel(batch, 0);
    const uint32_t n = 100000;
    dvz_pixel_alloc(pixel, n);


    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(192));
    dvz_pixel_color(pixel, 0, n, color, 0);


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, pixel, 0);


    // Run the app.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    // dvz_panel_destroy(panel);
    // dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(pos);
    FREE(color);
    return 0;
}



int test_scene_2(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);

    // Create a visual.
    DvzVisual* pixel = dvz_pixel(batch, 0);
    const uint32_t n = 100000;
    dvz_pixel_alloc(pixel, n);


    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_pixel_color(pixel, 0, n, color, 0);


    // Create two panels.
    DvzPanel* panel_0 = dvz_panel(figure, 0, 0, WIDTH / 2, HEIGHT);
    DvzPanel* panel_1 = dvz_panel(figure, WIDTH / 2, 0, WIDTH / 2, HEIGHT);

    // Transforms.

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(panel_0, 0);
    ANN(pz);

    dvz_panel_transform(panel_1, panel_0->transform);

    // Second visual.
    DvzVisual* pixel_1 = dvz_pixel(batch, 0);
    dvz_pixel_alloc(pixel_1, n / 10);
    dvz_pixel_position(pixel_1, 0, n / 10, pos, 0);
    dvz_pixel_color(pixel_1, 0, n / 10, color, 0);


    // Add the visuals to the panel AFTER setting the visuals' data.
    dvz_panel_visual(panel_0, pixel, 0);
    dvz_panel_visual(panel_1, pixel_1, 0);

    // Run the app.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    // dvz_panel_destroy(panel_0);
    // dvz_panel_destroy(panel_1);
    // dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(pos);
    FREE(color);
    return 0;
}



int test_scene_3(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Arcball.
    DvzArcball* arcball = dvz_panel_arcball(panel, 0);
    ANN(arcball);

    // Create a visual.

    // Disc mesh.
    DvzShape* shape = dvz_shape();
    dvz_shape_cube(
        shape, (DvzColor[]){
                   {RED},
                   {GREEN},
                   {PURPLE},
                   {CYAN},
                   {PURPLE},
                   {YELLOW},
               });
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* mesh = dvz_mesh_shape(batch, shape, flags);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, mesh, 0);

    // Run the app.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    dvz_shape_destroy(shape);
    // dvz_panel_destroy(panel);
    // dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    return 0;
}



int test_scene_offscreen(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app(DVZ_APP_FLAGS_OFFSCREEN);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Create a visual.
    DvzVisual* pixel = dvz_pixel(batch, 0);
    const uint32_t n = 100000;
    dvz_pixel_alloc(pixel, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_pixel_color(pixel, 0, n, color, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, pixel, 0);

    // Run the app.
    // NOTE: with offscreen rendering, N_FRAMES is ignored, the scene is just rendered once.
    dvz_scene_run(scene, app, N_FRAMES);

    // Screenshot.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/offscreen_scene.png", ARTIFACTS_DIR);
    dvz_app_screenshot(app, figure->canvas_id, imgpath);

    // Cleanup.
    // dvz_panel_destroy(panel);
    // dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(pos);
    FREE(color);
    return 0;
}



int test_scene_gui(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app objects.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);
    DvzScene* scene = dvz_scene(batch);
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    DvzPanel* panel1 = dvz_panel(figure, 100, 100, 300, 200);
    dvz_panel_gui(panel1, "GUI panel 1", 0);
    dvz_demo_panel_2D(panel1);

    DvzPanel* panel2 = dvz_panel(figure, 200, 350, 250, 150);
    dvz_panel_gui(panel2, "GUI panel 2", 0);
    dvz_demo_panel_2D(panel2);

    dvz_scene_run(scene, app, N_FRAMES);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}
