/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* polygon - live polygon and polygon-set composites.
 *
 * Build:  just example-c visuals/polygon
 * Run:    ./build/examples/c/visuals/polygon
 * Smoke:  ./build/examples/c/visuals/polygon 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1200u
#define HEIGHT 800u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create and attach one semantic polygon composite.
 *
 * @param scene the scene
 * @param panel the panel
 * @return whether the polygon was created and attached
 */
static bool _add_polygon(DvzScene* scene, DvzPanel* panel)
{
    DvzPolygon* polygon = dvz_polygon(scene, 0);
    if (polygon == NULL)
        return false;

    const dvec2 outer[5] = {
        {-0.92, -0.48},
        {-0.18, -0.56},
        {-0.06, +0.18},
        {-0.62, +0.50},
        {-1.00, +0.02},
    };
    const dvec2 hole[4] = {
        {-0.66, -0.12},
        {-0.39, -0.11},
        {-0.43, +0.16},
        {-0.69, +0.12},
    };
    const DvzPolygonRing holes[1] = {{.xy = hole, .count = 4}};

    int rc = dvz_polygon_set_geometry(
        polygon,
        &(DvzPolygonDesc){
            .outer = {.xy = outer, .count = 5},
            .holes = holes,
            .hole_count = 1,
        });
    if (rc != 0)
        return false;

    rc = dvz_polygon_fill_color(polygon, (DvzColor){62, 142, 188, 220});
    if (rc != 0)
        return false;
    rc = dvz_polygon_stroke_color(polygon, (DvzColor){18, 39, 54, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygon_stroke_width(polygon, 4.0f);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    if (composite == NULL)
        return false;

    return dvz_panel_add_composite(
               panel, composite,
               &(DvzVisualAttachDesc){
                   .z_layer = 0,
                   .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
               }) == 0;
}



/**
 * Create and attach one semantic polygon-set composite.
 *
 * @param scene the scene
 * @param panel the panel
 * @return whether the polygon set was created and attached
 */
static bool _add_polygon_set(DvzScene* scene, DvzPanel* panel)
{
    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    if (set == NULL)
        return false;

    const dvec2 region0[4] = {
        {+0.12, -0.48},
        {+0.52, -0.56},
        {+0.62, -0.10},
        {+0.20, +0.06},
    };
    const dvec2 region1[5] = {
        {+0.34, +0.16},
        {+0.78, +0.06},
        {+0.98, +0.42},
        {+0.66, +0.66},
        {+0.28, +0.50},
    };

    const uint32_t first =
        dvz_polygon_set_add(set, &(DvzPolygonDesc){.outer = {.xy = region0, .count = 4}});
    if (first == UINT32_MAX)
        return false;
    const uint32_t second =
        dvz_polygon_set_add(set, &(DvzPolygonDesc){.outer = {.xy = region1, .count = 5}});
    if (second == UINT32_MAX)
        return false;

    int rc = dvz_polygon_set_region_fill_color(set, first, (DvzColor){226, 91, 74, 230});
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_fill_color(set, second, (DvzColor){238, 190, 76, 230});
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_color(set, first, (DvzColor){63, 32, 28, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_color(set, second, (DvzColor){72, 49, 12, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_width(set, first, 3.0f);
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_width(set, second, 5.0f);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygon_set_composite(set, 0);
    if (composite == NULL)
        return false;

    return dvz_panel_add_composite(
               panel, composite,
               &(DvzVisualAttachDesc){
                   .z_layer = 10,
                   .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
               }) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("polygon");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, 0.96f, 0.97f, 0.96f, 1.0f);

    bool ok = _add_polygon(scene, panel);
    EXAMPLE_CHECK(ok, "failed to create polygon composite");
    ok = _add_polygon_set(scene, panel);
    EXAMPLE_CHECK(ok, "failed to create polygon-set composite");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "polygon");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    int rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");

    dvz_app_run(app, frame_count);

    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
