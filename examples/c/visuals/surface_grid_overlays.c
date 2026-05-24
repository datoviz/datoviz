/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* surface_grid_overlays - generated mesh with segment-based wireframe and contour overlays.
 *
 * Opens a GLFW window showing a lit structured surface generated with `dvz_geom_surface_grid()`.
 * The wireframe and isoline overlays are derived with CPU-side geom helpers, then rendered as
 * ordinary segment visuals. This keeps mesh geometry, overlay derivation, and stroke rendering
 * separated in the v0.4 scene path.
 *
 * Build:  just example-c visuals/surface_grid_overlays
 * Run:    ./build/examples/c/visuals/surface_grid_overlays
 * Smoke:  ./build/examples/c/visuals/surface_grid_overlays 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

#define SURFACE_ROWS 96
#define SURFACE_COLS 96
#define CONTOUR_LEVEL_COUNT 13



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic surface field and color ramp.
 *
 * @param heights output height buffer
 * @param colors output color buffer
 * @param out_min output minimum height
 * @param out_max output maximum height
 */
static void _surface_data(double* heights, DvzColor* colors, double* out_min, double* out_max)
{
    ANN(heights);
    ANN(colors);
    ANN(out_min);
    ANN(out_max);

    double zmin = +INFINITY;
    double zmax = -INFINITY;
    for (uint32_t row = 0; row < SURFACE_ROWS; row++)
    {
        const double y = -1.0 + 2.0 * (double)row / (double)(SURFACE_ROWS - 1);
        for (uint32_t col = 0; col < SURFACE_COLS; col++)
        {
            const double x = -1.0 + 2.0 * (double)col / (double)(SURFACE_COLS - 1);
            const double r2 = x * x + y * y;
            const double ridge = 0.33 * cos(15.5 * sqrt(r2)) * exp(-1.20 * r2);
            const double saddle = 0.16 * (x * x - 0.65 * y * y) * exp(-0.85 * r2);
            const double wave = 0.055 * sin(15.0 * x + 3.0 * y) * cos(10.0 * y - 2.0 * x);
            const double z = ridge + saddle + wave;
            const uint32_t idx = row * SURFACE_COLS + col;
            heights[idx] = z;
            zmin = z < zmin ? z : zmin;
            zmax = z > zmax ? z : zmax;
        }
    }

    const double span = zmax > zmin ? zmax - zmin : 1.0;
    for (uint32_t i = 0; i < SURFACE_ROWS * SURFACE_COLS; i++)
    {
        const double t = CLIP((heights[i] - zmin) / span, 0.0, 1.0);
        colors[i][0] = (uint8_t)(34.0 + 206.0 * t);
        colors[i][1] = (uint8_t)(75.0 + 150.0 * (1.0 - fabs(2.0 * t - 1.0)));
        colors[i][2] = (uint8_t)(165.0 + 60.0 * (1.0 - t));
        colors[i][3] = 255;
    }

    *out_min = zmin;
    *out_max = zmax;
}



/**
 * Upload derived geometry edges to a segment visual.
 *
 * @param visual target segment visual
 * @param geometry source geometry
 * @param edges derived edge list
 * @param color overlay color
 * @param width_px stroke width in pixels
 * @param z_offset visual-space offset along +Z
 * @return true on success, false on error
 */
static bool _upload_edges(
    DvzVisual* visual, const DvzGeometry* geometry, const DvzGeometryEdges* edges,
    const DvzColor color, float width_px, double z_offset)
{
    if (visual == NULL || geometry == NULL || edges == NULL || color == NULL ||
        edges->edge_count == 0)
    {
        return false;
    }

    bool ok = false;
    vec3* starts = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(edges->edge_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(edges->edge_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto cleanup;

    for (uint32_t i = 0; i < edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &edges->edges[i];
        const dvec3* p0 = &geometry->positions[edge->v0];
        const dvec3* p1 = &geometry->positions[edge->v1];
        starts[i][0] = (float)(*p0)[0];
        starts[i][1] = (float)(*p0)[1];
        starts[i][2] = (float)((*p0)[2] + z_offset);
        ends[i][0] = (float)(*p1)[0];
        ends[i][1] = (float)(*p1)[1];
        ends[i][2] = (float)((*p1)[2] + z_offset);
        colors[i][0] = color[0];
        colors[i][1] = color[1];
        colors[i][2] = color[2];
        colors[i][3] = color[3];
        widths[i] = width_px;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = edges->edge_count},
        {.attr_name = "position_end", .data = ends, .item_count = edges->edge_count},
        {.attr_name = "color", .data = colors, .item_count = edges->edge_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = edges->edge_count},
    };
    ok = dvz_visual_set_data_many(visual, updates, 4) == 0 &&
         dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0;

cleanup:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return ok;
}



/**
 * Upload extracted contour segments to a segment visual.
 *
 * @param visual target segment visual
 * @param contours extracted contour segments
 * @param width_px stroke width in pixels
 * @param z_offset visual-space offset along +Z
 * @return true on success, false on error
 */
static bool _upload_contours(
    DvzVisual* visual, const DvzGeometryContours* contours, float width_px, double z_offset)
{
    if (visual == NULL || contours == NULL || contours->segment_count == 0)
        return false;

    bool ok = false;
    vec3* starts = (vec3*)dvz_calloc(contours->segment_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(contours->segment_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(contours->segment_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(contours->segment_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto cleanup;

    for (uint32_t i = 0; i < contours->segment_count; i++)
    {
        const DvzGeometryContourSegment* segment = &contours->segments[i];
        starts[i][0] = (float)segment->p0[0];
        starts[i][1] = (float)segment->p0[1];
        starts[i][2] = (float)(segment->p0[2] + z_offset);
        ends[i][0] = (float)segment->p1[0];
        ends[i][1] = (float)segment->p1[1];
        ends[i][2] = (float)(segment->p1[2] + z_offset);

        const bool major = segment->level_index % 4 == 0;
        colors[i][0] = major ? 255 : 245;
        colors[i][1] = major ? 255 : 245;
        colors[i][2] = major ? 255 : 245;
        colors[i][3] = major ? 255 : 225;
        widths[i] = major ? width_px * 1.65f : width_px;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = contours->segment_count},
        {.attr_name = "position_end", .data = ends, .item_count = contours->segment_count},
        {.attr_name = "color", .data = colors, .item_count = contours->segment_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = contours->segment_count},
    };
    ok = dvz_visual_set_data_many(visual, updates, 4) == 0 &&
         dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0;

cleanup:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    const uint32_t vertex_count = SURFACE_ROWS * SURFACE_COLS;
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGeometry* geometry = NULL;
    DvzGeometryEdges* edges = NULL;
    DvzGeometryContours* contours = NULL;

    double* heights = (double*)dvz_calloc(vertex_count, sizeof(double));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    double levels[CONTOUR_LEVEL_COUNT] = {0};
    EXAMPLE_CHECK(heights != NULL && colors != NULL, "surface_grid_overlays: allocation failed");

    double zmin = 0.0;
    double zmax = 0.0;
    _surface_data(heights, colors, &zmin, &zmax);
    for (uint32_t i = 0; i < CONTOUR_LEVEL_COUNT; i++)
    {
        const double t = (double)(i + 1) / (double)(CONTOUR_LEVEL_COUNT + 1);
        levels[i] = zmin + t * (zmax - zmin);
    }

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "scene setup failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 1.8f;
    camera_desc.eye[1] = -2.25f;
    camera_desc.eye[2] = 1.55f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

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
    geometry = dvz_geom_surface_grid(&desc);
    EXAMPLE_CHECK(geometry != NULL, "dvz_geom_surface_grid() failed");

    DvzVisual* mesh = dvz_mesh(scene, 0);
    EXAMPLE_CHECK(mesh != NULL, "dvz_mesh() failed");
    bool uploaded = example_mesh_geometry(mesh, geometry);
    EXAMPLE_CHECK(uploaded, "example_mesh_geometry() failed");

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = -0.45f;
    material.light_direction[2] = 0.82f;
    material.phong.ambient = 0.20f;
    material.phong.diffuse = 0.78f;
    material.phong.specular = 0.55f;
    material.phong.shininess = 80.0f;
    ok = dvz_visual_set_material(mesh, &material) == 0;
    EXAMPLE_CHECK(ok, "dvz_visual_set_material() failed");
    ok = dvz_panel_add_visual(panel, mesh, NULL) == 0;
    EXAMPLE_CHECK(ok, "dvz_panel_add_visual(mesh) failed");

    edges = dvz_geometry_edges(geometry);
    EXAMPLE_CHECK(edges != NULL, "dvz_geometry_edges() failed");
    DvzVisual* wire = dvz_segment(scene, 0);
    EXAMPLE_CHECK(wire != NULL, "dvz_segment(wire) failed");
    const DvzColor wire_color = {20, 24, 30, 205};
    uploaded = _upload_edges(wire, geometry, edges, wire_color, 0.75f, 0.002);
    EXAMPLE_CHECK(uploaded, "wireframe upload failed");
    ok = dvz_visual_set_alpha_mode(wire, DVZ_ALPHA_BLENDED) == 0;
    EXAMPLE_CHECK(ok, "wireframe alpha mode failed");
    ok = dvz_panel_add_visual(panel, wire, NULL) == 0;
    EXAMPLE_CHECK(ok, "dvz_panel_add_visual(wire) failed");

    contours = dvz_geometry_contours(geometry, heights, vertex_count, levels, CONTOUR_LEVEL_COUNT);
    EXAMPLE_CHECK(contours != NULL, "dvz_geometry_contours() failed");
    DvzVisual* contour_visual = dvz_segment(scene, 0);
    EXAMPLE_CHECK(contour_visual != NULL, "dvz_segment(contours) failed");
    uploaded = _upload_contours(contour_visual, contours, 2.0f, 0.006);
    EXAMPLE_CHECK(uploaded, "contour upload failed");
    ok = dvz_visual_set_alpha_mode(contour_visual, DVZ_ALPHA_BLENDED) == 0;
    EXAMPLE_CHECK(ok, "contour alpha mode failed");
    ok = dvz_panel_add_visual(panel, contour_visual, NULL) == 0;
    EXAMPLE_CHECK(ok, "dvz_panel_add_visual(contours) failed");

    dvz_panel_set_background_color(panel, 0.04f, 0.045f, 0.05f, 1.0f);

    app = dvz_app(scene);
    DvzView* win =
        app != NULL ? dvz_view_glfw(app, figure, WIDTH, HEIGHT, "surface grid overlays") : NULL;
    EXAMPLE_CHECK(app != NULL && win != NULL, "app/window setup failed");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    if (arcball != NULL)
        dvz_arcball_initial(arcball, (vec3){0.55f, 0.0f, -0.25f});

    dvz_app_run(app, frame_count);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (contours != NULL)
        dvz_geometry_contours_destroy(contours);
    if (edges != NULL)
        dvz_geometry_edges_destroy(edges);
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(heights);
    dvz_free(colors);
    return ret;
}
