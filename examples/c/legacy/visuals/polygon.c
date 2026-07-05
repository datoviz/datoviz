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

#include "_alloc.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1200u
#define HEIGHT 800u

static const dvec2 POLYGON_OUTER[5] = {
    {-0.92, -0.48},
    {-0.18, -0.56},
    {-0.06, +0.18},
    {-0.62, +0.50},
    {-1.00, +0.02},
};

static const dvec2 POLYGON_HOLE[4] = {
    {-0.66, -0.12},
    {-0.39, -0.11},
    {-0.43, +0.16},
    {-0.69, +0.12},
};

static const dvec2 POLYGON_SET_REGION0[4] = {
    {+0.12, -0.48},
    {+0.52, -0.56},
    {+0.62, -0.10},
    {+0.20, +0.06},
};

static const dvec2 POLYGON_SET_REGION1[5] = {
    {+0.34, +0.16},
    {+0.78, +0.06},
    {+0.98, +0.42},
    {+0.66, +0.66},
    {+0.28, +0.50},
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PolygonExampleState
{
    DvzPolygon* polygon;
    DvzPolygons* set;
    DvzComposite* polygon_composite;
    DvzComposite* set_composite;
    DvzVisual* triangulation;
    vec3* triangulation_start;
    vec3* triangulation_end;
    DvzColor* triangulation_color;
    float* triangulation_widths;
    uint32_t triangulation_count;
    uint32_t set_first;
    uint32_t set_second;
    bool show_triangulation;
    float polygon_width;
    float set_first_width;
    float set_second_width;
    float triangulation_width;
    float miter_limit;
    int join;
} PolygonExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create and attach one semantic polygon composite.
 *
 * @param scene the scene
 * @param panel the panel
 * @param state example state
 * @return whether the polygon was created and attached
 */
static bool _add_polygon(DvzScene* scene, DvzPanel* panel, PolygonExampleState* state)
{
    if (state == NULL)
        return false;

    DvzPolygon* polygon = dvz_polygon(scene, 0);
    if (polygon == NULL)
        return false;

    const DvzPolygonRing holes[1] = {{.xy = POLYGON_HOLE, .count = 4}};

    int rc = dvz_polygon_geometry(
        polygon,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_OUTER, .count = 5},
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
    rc = dvz_polygon_stroke_width_px(polygon, state->polygon_width);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    if (composite == NULL)
        return false;

    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    if (stroke == NULL || dvz_path_set_join(stroke, (DvzPathJoin)state->join, state->miter_limit))
        return false;

    const int rc_attach = dvz_panel_add_composite(
        panel, composite,
        &(DvzVisualAttachDesc){
            .z_layer = 0,
            .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
        });
    if (rc_attach != 0)
        return false;

    state->polygon = polygon;
    state->polygon_composite = composite;
    return true;
}



/**
 * Create and attach one semantic polygon-set composite.
 *
 * @param scene the scene
 * @param panel the panel
 * @param state example state
 * @return whether the polygon set was created and attached
 */
static bool _add_polygon_set(DvzScene* scene, DvzPanel* panel, PolygonExampleState* state)
{
    if (state == NULL)
        return false;

    DvzPolygons* set = dvz_polygons(scene, 0);
    if (set == NULL)
        return false;

    const uint32_t first = dvz_polygons_add_region(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_SET_REGION0, .count = 4},
        });
    if (first == UINT32_MAX)
        return false;
    const uint32_t second = dvz_polygons_add_region(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_SET_REGION1, .count = 5},
        });
    if (second == UINT32_MAX)
        return false;

    int rc = dvz_polygons_set_region_fill_color(set, first, (DvzColor){226, 91, 74, 230});
    if (rc != 0)
        return false;
    rc = dvz_polygons_set_region_fill_color(set, second, (DvzColor){238, 190, 76, 230});
    if (rc != 0)
        return false;
    rc = dvz_polygons_set_region_stroke_color(set, first, (DvzColor){63, 32, 28, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygons_set_region_stroke_color(set, second, (DvzColor){72, 49, 12, 255});
    if (rc != 0)
        return false;
    rc = dvz_polygons_set_region_stroke_width_px(set, first, state->set_first_width);
    if (rc != 0)
        return false;
    rc = dvz_polygons_set_region_stroke_width_px(set, second, state->set_second_width);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_polygons_composite(set, 0);
    if (composite == NULL)
        return false;

    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    if (stroke == NULL || dvz_path_set_join(stroke, (DvzPathJoin)state->join, state->miter_limit))
        return false;

    const int rc_attach = dvz_panel_add_composite(
        panel, composite,
        &(DvzVisualAttachDesc){
            .z_layer = 10,
            .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
        });
    if (rc_attach != 0)
        return false;

    state->set = set;
    state->set_composite = composite;
    state->set_first = first;
    state->set_second = second;
    return true;
}


/**
 * Return the number of triangle-edge segments in a triangulated geometry.
 *
 * @param geometry triangulated geometry
 * @return segment count
 */
static uint32_t _triangulation_segment_count(const DvzGeometry* geometry)
{
    if (geometry == NULL || geometry->index_count == 0 || geometry->index_count % 3 != 0)
        return 0;
    return geometry->index_count;
}


/**
 * Append one triangulated geometry as edge segments.
 *
 * @param geometry triangulated geometry
 * @param starts segment start positions
 * @param ends segment end positions
 * @param colors segment colors
 * @param widths segment widths
 * @param offset current write offset
 * @param width segment width in pixels
 * @return updated write offset
 */
static uint32_t _append_triangulation_edges(
    const DvzGeometry* geometry, vec3* starts, vec3* ends, DvzColor* colors, float* widths,
    uint32_t offset, float width)
{
    if (geometry == NULL || starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        return offset;

    static const uint32_t edges[3][2] = {{0, 1}, {1, 2}, {2, 0}};
    for (uint32_t i = 0; i + 2 < geometry->index_count; i += 3)
    {
        const DvzIndex tri[3] = {
            geometry->indices[i + 0],
            geometry->indices[i + 1],
            geometry->indices[i + 2],
        };
        for (uint32_t e = 0; e < 3; e++)
        {
            const double* p0 = geometry->positions[tri[edges[e][0]]];
            const double* p1 = geometry->positions[tri[edges[e][1]]];
            const uint32_t j = offset++;
            starts[j][0] = (float)p0[0];
            starts[j][1] = (float)p0[1];
            starts[j][2] = (float)p0[2];
            ends[j][0] = (float)p1[0];
            ends[j][1] = (float)p1[1];
            ends[j][2] = (float)p1[2];
            colors[j] = dvz_color_rgba(20, 20, 20, 210);
            widths[j] = width;
        }
    }
    return offset;
}


/**
 * Upload the current triangulation line widths.
 *
 * @param state example state
 */
static void _upload_triangulation_widths(PolygonExampleState* state)
{
    if (state == NULL || state->triangulation == NULL || state->triangulation_widths == NULL)
        return;

    for (uint32_t i = 0; i < state->triangulation_count; i++)
        state->triangulation_widths[i] = state->triangulation_width;
    (void)dvz_visual_set_data(
        state->triangulation, "line_width", state->triangulation_widths,
        state->triangulation_count);
}


/**
 * Create and attach a triangle-edge overlay for every polygon in the example.
 *
 * @param scene the scene
 * @param panel the panel
 * @param state example state
 * @return whether the overlay was created
 */
static bool _add_triangulation_overlay(
    DvzScene* scene, DvzPanel* panel, PolygonExampleState* state)
{
    if (scene == NULL || panel == NULL || state == NULL)
        return false;

    const DvzPolygonRing holes[1] = {{.xy = POLYGON_HOLE, .count = 4}};
    DvzGeometry* polygon = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_OUTER, .count = 5},
            .holes = holes,
            .hole_count = 1,
        },
        NULL);
    DvzGeometry* region0 = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_SET_REGION0, .count = 4},
        },
        NULL);
    DvzGeometry* region1 = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = POLYGON_SET_REGION1, .count = 5},
        },
        NULL);
    if (polygon == NULL || region0 == NULL || region1 == NULL)
        goto error;

    const uint32_t segment_count = _triangulation_segment_count(polygon) +
                                   _triangulation_segment_count(region0) +
                                   _triangulation_segment_count(region1);
    if (segment_count == 0)
        goto error;

    state->triangulation_start =
        (vec3*)dvz_calloc(segment_count, sizeof(*state->triangulation_start));
    state->triangulation_end =
        (vec3*)dvz_calloc(segment_count, sizeof(*state->triangulation_end));
    state->triangulation_color = (DvzColor*)dvz_calloc(segment_count, sizeof(DvzColor));
    state->triangulation_widths = (float*)dvz_calloc(segment_count, sizeof(float));
    if (
        state->triangulation_start == NULL || state->triangulation_end == NULL ||
        state->triangulation_color == NULL || state->triangulation_widths == NULL)
    {
        goto error;
    }

    uint32_t offset = 0;
    offset = _append_triangulation_edges(
        polygon, state->triangulation_start, state->triangulation_end,
        state->triangulation_color, state->triangulation_widths, offset,
        state->triangulation_width);
    offset = _append_triangulation_edges(
        region0, state->triangulation_start, state->triangulation_end,
        state->triangulation_color, state->triangulation_widths, offset,
        state->triangulation_width);
    offset = _append_triangulation_edges(
        region1, state->triangulation_start, state->triangulation_end,
        state->triangulation_color, state->triangulation_widths, offset,
        state->triangulation_width);
    if (offset != segment_count)
        goto error;

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        goto error;
    DvzVisualDataUpdate updates[4] = {
        {
            .attr_name = "position_start",
            .data = state->triangulation_start,
            .item_count = segment_count,
        },
        {
            .attr_name = "position_end",
            .data = state->triangulation_end,
            .item_count = segment_count,
        },
        {.attr_name = "color", .data = state->triangulation_color, .item_count = segment_count},
        {
            .attr_name = "line_width",
            .data = state->triangulation_widths,
            .item_count = segment_count,
        },
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        goto error;
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) != 0)
        goto error;
    dvz_visual_set_visible(visual, state->show_triangulation);

    const int rc = dvz_panel_add_visual(
        panel, visual,
        &(DvzVisualAttachDesc){
            .z_layer = 30,
            .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
        });
    if (rc != 0)
        goto error;

    state->triangulation = visual;
    state->triangulation_count = segment_count;
    dvz_geometry_destroy(polygon);
    dvz_geometry_destroy(region0);
    dvz_geometry_destroy(region1);
    return true;

error:
    if (polygon != NULL)
        dvz_geometry_destroy(polygon);
    if (region0 != NULL)
        dvz_geometry_destroy(region0);
    if (region1 != NULL)
        dvz_geometry_destroy(region1);
    return false;
}


/**
 * Apply live polygon stroke controls.
 *
 * @param state example state
 */
static void _apply_polygon_controls(PolygonExampleState* state)
{
    if (state == NULL)
        return;

    const DvzPathJoin join = (DvzPathJoin)state->join;
    if (state->polygon != NULL)
        (void)dvz_polygon_stroke_width_px(state->polygon, state->polygon_width);
    if (state->set != NULL)
    {
        (void)dvz_polygons_set_region_stroke_width_px(
            state->set, state->set_first, state->set_first_width);
        (void)dvz_polygons_set_region_stroke_width_px(
            state->set, state->set_second, state->set_second_width);
    }
    if (state->polygon_composite != NULL)
    {
        DvzVisual* stroke = dvz_composite_visual(state->polygon_composite, "stroke");
        if (stroke != NULL)
            (void)dvz_path_set_join(stroke, join, state->miter_limit);
    }
    if (state->set_composite != NULL)
    {
        DvzVisual* stroke = dvz_composite_visual(state->set_composite, "stroke");
        if (stroke != NULL)
            (void)dvz_path_set_join(stroke, join, state->miter_limit);
    }
    if (state->triangulation != NULL)
    {
        dvz_visual_set_visible(state->triangulation, state->show_triangulation);
        _upload_triangulation_widths(state);
    }
}


/**
 * Build live controls for polygon stroke inspection.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _polygon_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    PolygonExampleState* state = (PolygonExampleState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    if (dvz_gui_begin(gui, "Polygon", NULL, 0))
    {
        changed |= dvz_gui_slider_float_format(
            gui, "Left stroke", &state->polygon_width, 0.5f, 36.0f, "%.1f px");
        changed |= dvz_gui_slider_float_format(
            gui, "Set stroke A", &state->set_first_width, 0.5f, 36.0f, "%.1f px");
        changed |= dvz_gui_slider_float_format(
            gui, "Set stroke B", &state->set_second_width, 0.5f, 36.0f, "%.1f px");

        const char* const joins[] = {"Miter", "Round", "Bevel"};
        changed |= dvz_gui_combo(gui, "Join", &state->join, joins, 3);
        if (state->join == (int)DVZ_PATH_JOIN_MITER)
        {
            changed |=
                dvz_gui_slider_float(gui, "Miter limit", &state->miter_limit, 1.0f, 16.0f);
        }
        changed |= dvz_gui_checkbox(gui, "Triangulation", &state->show_triangulation);
        if (state->show_triangulation)
        {
            changed |= dvz_gui_slider_float_format(
                gui, "Triangle lines", &state->triangulation_width, 0.5f, 8.0f, "%.1f px");
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_polygon_controls(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("polygon");
    PolygonExampleState state = {
        .polygon_width = 12.0f,
        .set_first_width = 10.0f,
        .set_second_width = 14.0f,
        .triangulation_width = 1.5f,
        .miter_limit = 4.0f,
        .join = (int)DVZ_PATH_JOIN_ROUND,
    };

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.96f, 0.97f, 0.96f, 1.0f));

    bool ok = _add_polygon(scene, panel, &state);
    EXAMPLE_CHECK(ok, "failed to create polygon composite");
    ok = _add_polygon_set(scene, panel, &state);
    EXAMPLE_CHECK(ok, "failed to create polygon-set composite");
    ok = _add_triangulation_overlay(scene, panel, &state);
    EXAMPLE_CHECK(ok, "failed to create polygon triangulation overlay");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "polygon");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _polygon_gui, &state);

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
    dvz_free(state.triangulation_start);
    dvz_free(state.triangulation_end);
    dvz_free(state.triangulation_color);
    dvz_free(state.triangulation_widths);
    return ret;
}
