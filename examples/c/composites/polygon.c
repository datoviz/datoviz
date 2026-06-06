/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* polygon - semantic polygon and polygon-set composites.
 *
 * Scenario: composite_polygon
 * Style: feature composite, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c composites/polygon
 * Run:    ./build/examples/c/composites/polygon --live
 * Smoke:  ./build/examples/c/composites/polygon --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
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
 * Configure the panel used by the polygon composite example.
 *
 * @param panel target panel
 * @return whether setup succeeded
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);
    example_graphite_cyan_set_panel_background(panel);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.06f, .right = 0.06f, .bottom = 0.06f,
                                        .top = 0.06f});
    if (!ok)
        return false;
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -2.4, 3.6);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.4, 1.7);
    return rc == 0;
}



/**
 * Add one semantic polygon with a visible hole.
 *
 * @param scene scene owning the polygon
 * @param panel panel receiving the composite
 * @return whether the polygon was created and attached
 */
static bool _add_holed_polygon(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    static const dvec2 outer[6] = {
        {-2.00, -0.86}, {-1.22, -1.04}, {-0.55, -0.42},
        {-0.70, +0.88}, {-1.58, +1.18}, {-2.18, +0.42},
    };
    static const dvec2 hole[4] = {
        {-1.62, -0.20},
        {-1.06, -0.10},
        {-1.16, +0.48},
        {-1.72, +0.35},
    };

    DvzPolygon* polygon = dvz_polygon(scene, 0);
    if (polygon == NULL)
        return false;
    int rc = dvz_polygon_geometry(
        polygon, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                                   .outer = {.xy = outer, .count = 6}});
    if (rc != 0)
        return false;
    rc = dvz_polygon_hole(polygon, 0, 4, hole);
    if (rc != 0)
        return false;
    rc = dvz_polygon_id(polygon, 10);
    if (rc != 0)
        return false;
    rc = dvz_polygon_fill_color(polygon, (DvzColor){36, 151, 178, 210});
    if (rc != 0)
        return false;
    rc = dvz_polygon_stroke_color(polygon, (DvzColor){214, 240, 255, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygon_stroke_width(polygon, 8.0f);
    if (rc != 0)
        return false;
    rc = dvz_polygon_stroke_join(polygon, DVZ_PATH_JOIN_ROUND, 4.0f);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(
        panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
                                                .z_layer = 0});
    return rc == 0;
}



/**
 * Add a small semantic polygon set with per-region styling.
 *
 * @param scene scene owning the polygon set
 * @param panel panel receiving the composite
 * @return whether the polygon set was created and attached
 */
static bool _add_polygon_set(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    static const dvec2 left[5] = {
        {+0.10, -0.95}, {+0.95, -1.10}, {+1.22, -0.22}, {+0.62, +0.42}, {-0.10, +0.10},
    };
    static const dvec2 middle[5] = {
        {+0.84, -0.05}, {+1.56, -0.36}, {+2.05, +0.20}, {+1.74, +1.02}, {+0.88, +0.76},
    };
    static const dvec2 right[6] = {
        {+2.00, -0.88}, {+2.88, -0.82}, {+3.18, -0.15},
        {+2.92, +0.68}, {+2.20, +0.92}, {+1.88, +0.18},
    };

    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    if (set == NULL)
        return false;

    const uint32_t a = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = left, .count = 5}});
    const uint32_t b = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = middle, .count = 5}});
    const uint32_t c = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = right, .count = 6}});
    if (a == UINT32_MAX || b == UINT32_MAX || c == UINT32_MAX)
        return false;

    const uint64_t ids[3] = {21, 22, 23};
    int rc = dvz_polygon_set_region_ids(set, 0, 3, ids);
    if (rc != 0)
        return false;

    const DvzColor fill[3] = {
        {231, 98, 82, 220},
        {240, 189, 72, 220},
        {92, 189, 132, 220},
    };
    const DvzColor stroke[3] = {
        {85, 42, 38, 255},
        {88, 68, 26, 255},
        {26, 74, 54, 255},
    };
    const float widths[3] = {5.0f, 7.0f, 5.0f};
    rc = dvz_polygon_set_region_fill_colors(set, 0, 3, fill);
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_colors(set, 0, 3, stroke);
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_region_stroke_widths(set, 0, 3, widths);
    if (rc != 0)
        return false;
    rc = dvz_polygon_set_stroke_join(set, DVZ_PATH_JOIN_BEVEL, 4.0f);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygon_set_composite(set, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(
        panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
                                                .z_layer = 2});
    return rc == 0;
}



static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(ctx->figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel), "panel configuration failed");
    EXAMPLE_CHECK(_add_holed_polygon(ctx->scene, panel), "holed polygon setup failed");
    EXAMPLE_CHECK(_add_polygon_set(ctx->scene, panel), "polygon set setup failed");

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    (void)panzoom;

    ok = true;
cleanup:
    return ok;
}



static DvzScenarioSpec _polygon_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "composite_polygon",
        .title = "composite_polygon",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _polygon_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
