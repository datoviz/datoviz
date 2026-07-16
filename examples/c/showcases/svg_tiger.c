/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* svg_tiger - This example renders the classic colored tiger from prepared SVG paths.
 *
 * What to look for: solid path fills, black outlines, thin whiskers, and document paint order are
 * reproduced with one merged mesh plus one retained path visual. Cubic curves are flattened by the
 * preparation script; Datoviz performs the final polygon triangulation at runtime.
 *
 * The source artwork remains cache-local while its redistribution terms are unresolved. Prepare it
 * from a pinned Glumpy source revision with:
 *
 *   python3 tools/data/prepare_svg_tiger.py --download
 *
 * Scenario: showcases_svg_tiger
 * Style: showcase, source artwork colors, 1000x1000 window target
 *
 * Build:  just example-c showcases/svg_tiger
 * Run:    ./build/examples/c/showcases/svg_tiger --live
 * Smoke:  ./build/examples/c/showcases/svg_tiger --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "runner/scenario_runner.h"
#include "svg_tiger_model.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1000u
#define HEIGHT 1000u

#define SVG_TIGER_CACHE_PATH ".cache/datoviz/examples/svg_tiger/prepared/tiger_paths.bin"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_svg_tiger_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Print the exact cache preparation command.
 */
static void _print_prepare_hint(void)
{
    dvz_fprintf(
        stderr, "svg_tiger: missing prepared data at %s\n"
                "Run `python3 tools/data/prepare_svg_tiger.py --download` from the repository "
                "root.\n",
        SVG_TIGER_CACHE_PATH);
}



/**
 * Configure a white equal-aspect SVG viewport and MSAA.
 *
 * @param panel target panel
 * @param data loaded SVG document metadata
 * @return whether panel configuration succeeded
 */
static bool _configure_panel(DvzPanel* panel, const SvgTigerData* data)
{
    ANN(panel);
    ANN(data);
    if (!example_configure_equal_aspect_panel(
            panel, (DvzDataDomain){.min = 0.0, .max = data->width},
            (DvzDataDomain){.min = 0.0, .max = data->height}, 0.0))
    {
        return false;
    }
    if (dvz_panel_set_background_color(panel, dvz_color_rgba(255, 255, 255, 255)) != 0)
        return false;
    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.enabled = true;
    return dvz_panel_set_msaa(panel, &msaa) == 0;
}



/**
 * Add the merged, flat-colored SVG fill mesh.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param data loaded SVG paths
 * @return whether the fill mesh was added
 */
static bool _add_fills(DvzScene* scene, DvzPanel* panel, const SvgTigerData* data)
{
    ANN(scene);
    ANN(panel);
    ANN(data);

    DvzGeometry* geometry = svg_tiger_fill_geometry(data);
    if (geometry == NULL)
        return false;
    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.phong.ambient = 1.0f;
    material.phong.diffuse = 0.0f;
    material.phong.specular = 0.0f;
    int rc = dvz_visual_set_material(mesh, &material);
    if (rc == 0)
        rc = dvz_mesh_set_geometry(mesh, geometry);
    if (rc == 0)
        rc = dvz_visual_set_depth_test(mesh, true);
    if (rc == 0)
        rc = dvz_panel_add_visual(panel, mesh, NULL);
    dvz_geometry_destroy(geometry);
    return rc == 0;
}



/**
 * Add all SVG outlines and open whisker paths as one retained path visual.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param data loaded SVG paths
 * @return whether the stroke visual was added
 */
static bool _add_strokes(DvzScene* scene, DvzPanel* panel, const SvgTigerData* data)
{
    ANN(scene);
    ANN(panel);
    ANN(data);

    SvgTigerStrokeData stroke = {0};
    if (!svg_tiger_stroke_data(data, &stroke))
        return false;
    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
    {
        svg_tiger_stroke_destroy(&stroke);
        return false;
    }

    const DvzVisualDataUpdate updates[3] = {
        {.attr_name = "position", .data = stroke.positions, .item_count = stroke.point_count},
        {.attr_name = "color", .data = stroke.colors, .item_count = stroke.point_count},
        {.attr_name = "stroke_width_px", .data = stroke.widths, .item_count = stroke.point_count},
    };
    int rc = dvz_visual_set_data_many(path, updates, 3);
    if (rc == 0)
        rc = dvz_path_set_subpaths(path, stroke.subpath_count, stroke.lengths);
    if (rc == 0)
        rc = dvz_path_set_caps(path, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT);
    if (rc == 0)
        rc = dvz_path_set_join(path, DVZ_PATH_JOIN_MITER, 4.0f);
    if (rc == 0)
        rc = dvz_visual_set_depth_test(path, true);
    if (rc == 0)
        rc = dvz_panel_add_visual(panel, path, NULL);
    svg_tiger_stroke_destroy(&stroke);
    return rc == 0;
}



/**
 * Initialize the prepared SVG tiger scenario.
 *
 * @param ctx scenario context
 * @param out_user unused user-state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    SvgTigerData data = {0};
    if (!svg_tiger_load(SVG_TIGER_CACHE_PATH, &data))
    {
        _print_prepare_hint();
        return false;
    }

    bool ok = false;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    DvzPanel* panel = dvz_panel(ctx->figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel, &data), "SVG tiger panel configuration failed");
    EXAMPLE_CHECK(_add_fills(ctx->scene, panel, &data), "SVG tiger fill setup failed");
    EXAMPLE_CHECK(_add_strokes(ctx->scene, panel, &data), "SVG tiger stroke setup failed");

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, &panzoom_desc, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "SVG tiger panzoom setup failed");
    (void)panzoom;

    dvz_fprintf(
        stderr, "svg_tiger: %u paths, %u flattened points\n", data.path_count, data.point_count);
    ok = true;

cleanup:
    svg_tiger_destroy(&data);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_svg_tiger_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_svg_tiger",
        .title = "SVG Tiger",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_svg_tiger_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
