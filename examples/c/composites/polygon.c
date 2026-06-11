/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* polygon - clean semantic polygon and polygon-set composites.
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
#include "datoviz/controller/panzoom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_composite_polygon_scenario(void);



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
    return example_configure_equal_aspect_panel(
        panel, (DvzDataDomain){.min = -2.18, .max = +3.06},
        (DvzDataDomain){.min = -0.88, .max = +0.88}, 0.05,
        &(DvzPanelLayoutReserve){
            .left = 0.06f, .right = 0.06f, .bottom = 0.06f, .top = 0.06f});
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

    static const dvec2 outer[8] = {
        {-2.12, +0.00}, {-1.88, -0.58}, {-1.30, -0.82}, {-0.72, -0.58},
        {-0.48, +0.00}, {-0.72, +0.58}, {-1.30, +0.82}, {-1.88, +0.58},
    };
    static const dvec2 hole[6] = {
        {-1.66, +0.00}, {-1.52, -0.26}, {-1.22, -0.26},
        {-1.08, +0.00}, {-1.22, +0.26}, {-1.52, +0.26},
    };

    DvzPolygon* polygon = dvz_polygon(scene, 0);
    if (polygon == NULL)
        return false;
    int rc = dvz_polygon_geometry(
        polygon, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                                   .outer = {.xy = outer, .count = DVZ_ARRAY_COUNT(outer)}});
    if (rc != 0)
        return false;
    rc = dvz_polygon_hole(polygon, 0, DVZ_ARRAY_COUNT(hole), hole);
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

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_DATA;
    rc = dvz_panel_add_composite(panel, composite, &attach);
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

    static const dvec2 left[10] = {
        {+0.05, +0.74}, {+0.24, +0.24}, {+0.78, +0.24}, {+0.35, -0.06}, {+0.52, -0.58},
        {+0.05, -0.26}, {-0.42, -0.58}, {-0.25, -0.06}, {-0.68, +0.24}, {-0.14, +0.24},
    };
    static const dvec2 middle[4] = {
        {+1.12, -0.82},
        {+1.92, -0.82},
        {+1.92, -0.10},
        {+1.12, -0.10},
    };
    static const dvec2 right[4] = {
        {+2.20, +0.10},
        {+3.00, +0.10},
        {+3.00, +0.82},
        {+2.20, +0.82},
    };

    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    if (set == NULL)
        return false;

    const uint32_t a = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = left, .count = DVZ_ARRAY_COUNT(left)}});
    const uint32_t b = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = middle, .count = DVZ_ARRAY_COUNT(middle)}});
    const uint32_t c = dvz_polygon_set_add(
        set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                               .outer = {.xy = right, .count = DVZ_ARRAY_COUNT(right)}});
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

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = 2;
    attach.coord_space = DVZ_COORD_DATA;
    rc = dvz_panel_add_composite(panel, composite, &attach);
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

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, &panzoom_desc, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    (void)panzoom;

    ok = true;
cleanup:
    return ok;
}



DvzScenarioSpec dvz_composite_polygon_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "composite_polygon",
        .title = "composite_polygon",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_composite_polygon_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
