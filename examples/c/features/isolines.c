/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* isolines - CPU contour extraction rendered as retained segment overlays.
 *
 * Scenario: feature_isolines
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/isolines
 * Run:    ./build/examples/c/features/isolines --live
 * Smoke:  ./build/examples/c/features/isolines --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_isolines_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define GRID_ROWS   42u
#define GRID_COLS   50u
#define LEVEL_COUNT 9u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Assign scalar-field heights and vertex colors to a surface grid geometry.
 *
 * @param geometry surface-grid geometry
 * @param values output scalar values, one per vertex
 * @return true when all values were assigned
 */
static bool _shape_surface(DvzGeometry* geometry, double* values)
{
    ANN(geometry);
    ANN(values);

    if (geometry->vertex_count != GRID_ROWS * GRID_COLS || geometry->positions == NULL ||
        geometry->colors == NULL)
    {
        return false;
    }

    for (uint32_t row = 0; row < GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < GRID_COLS; col++)
        {
            const uint32_t idx = row * GRID_COLS + col;
            const double x = geometry->positions[idx][0];
            const double y = geometry->positions[idx][1];
            const double r2 = x * x + y * y;
            const double z = 0.24 * exp(-1.65 * r2) + 0.055 * sin(8.0 * x) * cos(6.0 * y);
            geometry->positions[idx][2] = z;
            values[idx] = z;

            const double t = fmin(1.0, fmax(0.0, (z + 0.09) / 0.38));
            const DvzColor lo = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
            const DvzColor hi = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
            geometry->colors[idx] = dvz_color_rgba(
                (uint8_t)((1.0 - t) * lo.r + t * hi.r),
                (uint8_t)((1.0 - t) * lo.g + t * hi.g),
                (uint8_t)((1.0 - t) * lo.b + t * hi.b), 230);
        }
    }
    return dvz_geometry_compute_normals(geometry) == 0;
}



/**
 * Add the colored surface mesh.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry surface geometry
 * @return true when the mesh was added
 */
static bool _add_surface(DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return false;
    DvzMaterialDesc material = example_default_phong_material_desc();
    if (dvz_visual_set_material(mesh, &material) != 0)
        return false;
    if (dvz_mesh_set_geometry(mesh, geometry) != 0)
        return false;
    return dvz_panel_add_visual(panel, mesh, NULL) == 0;
}



/**
 * Add extracted contour segments.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param contours extracted contour segments
 * @return true when the visual was added
 */
static bool _add_contours(DvzScene* scene, DvzPanel* panel, const DvzGeometryContours* contours)
{
    ANN(scene);
    ANN(panel);
    ANN(contours);

    if (contours->segment_count == 0 || contours->segments == NULL)
        return false;

    vec3* starts = (vec3*)dvz_calloc(contours->segment_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(contours->segment_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(contours->segment_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(contours->segment_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto error;

    for (uint32_t i = 0; i < contours->segment_count; i++)
    {
        const DvzGeometryContourSegment* segment = &contours->segments[i];
        starts[i][0] = (float)segment->p0[0];
        starts[i][1] = (float)segment->p0[1];
        starts[i][2] = (float)(segment->p0[2] + 0.010);
        ends[i][0] = (float)segment->p1[0];
        ends[i][1] = (float)segment->p1[1];
        ends[i][2] = (float)(segment->p1[2] + 0.010);
        const bool major = segment->level_index == LEVEL_COUNT / 2u;
        colors[i] = major ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT)
                          : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
        widths[i] = major ? 4.0f : 2.0f;
    }

    DvzVisual* segment = dvz_segment(scene, 0);
    if (segment == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = contours->segment_count},
        {.attr_name = "position_end", .data = ends, .item_count = contours->segment_count},
        {.attr_name = "color", .data = colors, .item_count = contours->segment_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = contours->segment_count},
    };
    if (dvz_visual_set_data_many(segment, updates, 4) != 0)
        goto error;
    if (dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, segment, NULL) != 0)
        goto error;

    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return true;

error:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return false;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the isolines feature scenario.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 1.55f;
    camera.view.eye[1] = -2.00f;
    camera.view.eye[2] = 1.45f;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = 0.08f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 0.0f;
    camera.view.up[2] = 1.0f;
    camera.projection.fov_y = 0.66f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    if (dvz_panel_set_camera(panel, &camera) == NULL)
        return false;

    DvzGeometrySurfaceGridDesc desc = dvz_geometry_surface_grid_desc();
    desc.rows = GRID_ROWS;
    desc.cols = GRID_COLS;
    desc.origin[0] = -1.05;
    desc.origin[1] = -0.775;
    desc.col_basis[0] = 2.10 / (double)(GRID_COLS - 1u);
    desc.row_basis[1] = 1.55 / (double)(GRID_ROWS - 1u);
    DvzGeometry* geometry = dvz_geom_surface_grid(&desc);
    double* values = (double*)dvz_calloc(GRID_ROWS * GRID_COLS, sizeof(double));
    if (geometry == NULL || values == NULL)
        goto error;
    if (!_shape_surface(geometry, values))
        goto error;

    double levels[LEVEL_COUNT] = {0};
    for (uint32_t i = 0; i < LEVEL_COUNT; i++)
        levels[i] = -0.030 + 0.035 * (double)i;
    DvzGeometryContours* contours =
        dvz_geometry_contours(geometry, values, geometry->vertex_count, levels, LEVEL_COUNT);
    if (contours == NULL)
        goto error;

    const bool ok = _add_surface(ctx->scene, panel, geometry) &&
                    _add_contours(ctx->scene, panel, contours);
    dvz_geometry_contours_destroy(contours);
    dvz_free(values);
    dvz_geometry_destroy(geometry);
    if (!ok)
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.55f, 0.0f, -0.25f});
    return true;

error:
    dvz_free(values);
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    return false;
}



/**
 * Return the isolines scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_isolines_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_isolines",
        .title = "isolines",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the isolines feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_isolines_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
