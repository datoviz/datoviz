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
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

#define SURFACE_ROWS 96
#define SURFACE_COLS 96

#define CONTOUR_LEVEL_MAX 32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SurfaceOverlayState
{
    DvzPanel* panel;
    DvzArcball* arcball;

    DvzVisual* mesh;
    DvzVisual* wire;
    DvzVisual* contours_visual;

    DvzGeometry* geometry;
    DvzGeometryEdges* edges;
    DvzGeometryContours* contours;

    double* heights;
    DvzColor* colors;
    double zmin;
    double zmax;

    bool show_surface;
    bool show_wire;
    bool show_contours;
    bool boundary_only;
    bool show_diagnostics;

    float height_scale;
    float surface_opacity;
    float light_direction[3];
    float ambient;
    float diffuse;
    float specular;
    float shininess;

    DvzColor wire_color;
    float wire_width;
    float wire_z_offset;

    float contour_count;
    float contour_range_min;
    float contour_range_max;
    float contour_major_step;
    DvzColor contour_minor_color;
    DvzColor contour_major_color;
    float contour_width;
    float contour_major_width;
    float contour_z_offset;
} SurfaceOverlayState;



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
        colors[i] = dvz_color_rgb(
            (uint8_t)(34.0 + 206.0 * t),
            (uint8_t)(75.0 + 150.0 * (1.0 - fabs(2.0 * t - 1.0))),
            (uint8_t)(165.0 + 60.0 * (1.0 - t)));
    }

    *out_min = zmin;
    *out_max = zmax;
}



/**
 * Initialize the live example state controls.
 *
 * @param state example state
 */
static void _state_defaults(SurfaceOverlayState* state)
{
    ANN(state);

    state->show_surface = true;
    state->show_wire = true;
    state->show_contours = true;
    state->boundary_only = false;
    state->show_diagnostics = true;

    state->height_scale = 0.938f;
    state->surface_opacity = 1.0f;
    state->light_direction[0] = 0.35f;
    state->light_direction[1] = -0.45f;
    state->light_direction[2] = 0.82f;
    state->ambient = 0.279f;
    state->diffuse = 0.642f;
    state->specular = 0.55f;
    state->shininess = 55.395f;

    state->wire_color = dvz_color_rgba(20, 24, 30, 205);
    state->wire_width = 2.39f;
    state->wire_z_offset = 0.0011f;

    state->contour_count = 15.0f;
    state->contour_major_step = 1.0f;
    state->contour_minor_color = dvz_color_rgba(245, 245, 245, 225);
    state->contour_major_color = dvz_color_rgb(255, 255, 255);
    state->contour_width = 2.1f;
    state->contour_major_width = 1.65f;
    state->contour_z_offset = 0.0045f;
}



/**
 * Return the integer contour count from GUI state.
 *
 * @param state example state
 * @return clamped contour count
 */
static uint32_t _state_contour_count(const SurfaceOverlayState* state)
{
    ANN(state);
    const float rounded = roundf(state->contour_count);
    if (rounded < 1.0f)
        return 1;
    if (rounded > (float)CONTOUR_LEVEL_MAX)
        return CONTOUR_LEVEL_MAX;
    return (uint32_t)rounded;
}



/**
 * Return the integer major-contour cadence from GUI state.
 *
 * @param state example state
 * @return clamped major contour step
 */
static uint32_t _state_major_step(const SurfaceOverlayState* state)
{
    ANN(state);
    const float rounded = roundf(state->contour_major_step);
    if (rounded < 1.0f)
        return 1;
    if (rounded > 16.0f)
        return 16;
    return (uint32_t)rounded;
}



/**
 * Apply visual visibility controls.
 *
 * @param state example state
 */
static void _state_apply_visibility(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->mesh != NULL)
        dvz_visual_set_visible(state->mesh, state->show_surface);
    if (state->wire != NULL)
        dvz_visual_set_visible(state->wire, state->show_wire);
    if (state->contours_visual != NULL)
        dvz_visual_set_visible(
            state->contours_visual,
            state->show_contours && state->contours != NULL && state->contours->segment_count > 0);
}



/**
 * Apply material controls to the surface mesh.
 *
 * @param state example state
 * @return whether the material was applied
 */
static bool _state_apply_material(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->mesh == NULL)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.alpha_mode = state->surface_opacity < 0.999f ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE;
    material.opacity = state->surface_opacity;
    material.light_direction[0] = state->light_direction[0];
    material.light_direction[1] = state->light_direction[1];
    material.light_direction[2] = state->light_direction[2];
    material.phong.ambient = state->ambient;
    material.phong.diffuse = state->diffuse;
    material.phong.specular = state->specular;
    material.phong.shininess = state->shininess;
    return dvz_visual_set_material(state->mesh, &material) == 0;
}



/**
 * Upload derived geometry edges to a segment visual.
 *
 * @param state example state
 * @return true on success, false on error
 */
static bool _state_upload_edges(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->wire == NULL || state->geometry == NULL || state->edges == NULL ||
        state->edges->edge_count == 0)
    {
        return false;
    }

    uint32_t edge_count = 0;
    for (uint32_t i = 0; i < state->edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &state->edges->edges[i];
        if (!state->boundary_only || (edge->flags & DVZ_GEOMETRY_EDGE_BOUNDARY))
            edge_count++;
    }
    if (edge_count == 0)
        return false;

    bool ok = false;
    vec3* starts = (vec3*)dvz_calloc(edge_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(edge_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(edge_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(edge_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto cleanup;

    uint32_t dst = 0;
    for (uint32_t i = 0; i < state->edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &state->edges->edges[i];
        if (state->boundary_only && !(edge->flags & DVZ_GEOMETRY_EDGE_BOUNDARY))
            continue;

        const double* p0 = state->geometry->positions[edge->v0];
        const double* p1 = state->geometry->positions[edge->v1];
        starts[dst][0] = (float)p0[0];
        starts[dst][1] = (float)p0[1];
        starts[dst][2] = (float)(p0[2] + state->wire_z_offset);
        ends[dst][0] = (float)p1[0];
        ends[dst][1] = (float)p1[1];
        ends[dst][2] = (float)(p1[2] + state->wire_z_offset);
        colors[dst] = state->wire_color;
        widths[dst] = state->wire_width;
        dst++;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = edge_count},
        {.attr_name = "position_end", .data = ends, .item_count = edge_count},
        {.attr_name = "color", .data = colors, .item_count = edge_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = edge_count},
    };
    ok = dvz_visual_set_data_many(state->wire, updates, 4) == 0 &&
         dvz_segment_set_caps(state->wire, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0;

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
 * @param state example state
 * @return true on success, false on error
 */
static bool _state_upload_contours(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->contours_visual == NULL || state->contours == NULL)
        return false;
    if (state->contours->segment_count == 0)
    {
        dvz_visual_set_visible(state->contours_visual, false);
        return true;
    }

    bool ok = false;
    vec3* starts = (vec3*)dvz_calloc(state->contours->segment_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(state->contours->segment_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(state->contours->segment_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(state->contours->segment_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto cleanup;

    const uint32_t major_step = _state_major_step(state);
    for (uint32_t i = 0; i < state->contours->segment_count; i++)
    {
        const DvzGeometryContourSegment* segment = &state->contours->segments[i];
        const bool major = segment->level_index % major_step == 0;
        const DvzColor* color = major ? &state->contour_major_color : &state->contour_minor_color;
        starts[i][0] = (float)segment->p0[0];
        starts[i][1] = (float)segment->p0[1];
        starts[i][2] = (float)(segment->p0[2] + state->contour_z_offset);
        ends[i][0] = (float)segment->p1[0];
        ends[i][1] = (float)segment->p1[1];
        ends[i][2] = (float)(segment->p1[2] + state->contour_z_offset);
        colors[i] = *color;
        widths[i] = major ? state->contour_width * state->contour_major_width :
                            state->contour_width;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = state->contours->segment_count},
        {.attr_name = "position_end", .data = ends, .item_count = state->contours->segment_count},
        {.attr_name = "color", .data = colors, .item_count = state->contours->segment_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = state->contours->segment_count},
    };
    ok = dvz_visual_set_data_many(state->contours_visual, updates, 4) == 0 &&
         dvz_segment_set_caps(
             state->contours_visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0;
    if (ok)
        _state_apply_visibility(state);

cleanup:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return ok;
}



/**
 * Rebuild contour segments after level controls change.
 *
 * @param state example state
 * @return true on success, false on error
 */
static bool _state_rebuild_contours(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->geometry == NULL || state->heights == NULL)
        return false;

    const uint32_t count = _state_contour_count(state);
    double levels[CONTOUR_LEVEL_MAX] = {0};
    double lo = state->contour_range_min;
    double hi = state->contour_range_max;
    if (hi <= lo)
        hi = lo + 1e-6;
    for (uint32_t i = 0; i < count; i++)
    {
        const double t = (double)(i + 1) / (double)(count + 1);
        levels[i] = lo + t * (hi - lo);
    }

    DvzGeometryContours* contours =
        dvz_geometry_contours(state->geometry, state->heights, SURFACE_ROWS * SURFACE_COLS, levels,
                              count);
    if (contours == NULL)
        return false;

    if (state->contours != NULL)
        dvz_geometry_contours_destroy(state->contours);
    state->contours = contours;
    return _state_upload_contours(state);
}



/**
 * Rebuild surface geometry and all derived overlays.
 *
 * @param state example state
 * @return true on success, false on error
 */
static bool _state_rebuild_geometry(SurfaceOverlayState* state)
{
    ANN(state);
    ANN(state->heights);
    ANN(state->colors);

    DvzGeometrySurfaceGridDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = SURFACE_ROWS,
        .cols = SURFACE_COLS,
        .heights = state->heights,
        .colors = state->colors,
        .origin = {-1.0, -1.0, 0.0},
        .col_basis = {2.0 / (double)(SURFACE_COLS - 1), 0.0, 0.0},
        .row_basis = {0.0, 2.0 / (double)(SURFACE_ROWS - 1), 0.0},
        .height_axis = {0.0, 0.0, 1.0},
        .height_scale = state->height_scale,
    };
    DvzGeometry* geometry = dvz_geom_surface_grid(&desc);
    if (geometry == NULL)
        return false;

    DvzGeometryEdges* edges = dvz_geometry_edges(geometry);
    if (edges == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }

    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    if (state->edges != NULL)
        dvz_geometry_edges_destroy(state->edges);
    if (state->contours != NULL)
    {
        dvz_geometry_contours_destroy(state->contours);
        state->contours = NULL;
    }
    state->geometry = geometry;
    state->edges = edges;

    if (state->mesh != NULL && !example_mesh_geometry(state->mesh, state->geometry))
        return false;
    if (state->wire != NULL && !_state_upload_edges(state))
        return false;
    if (!_state_rebuild_contours(state))
        return false;
    _state_apply_visibility(state);
    return true;
}



/**
 * Release CPU geometry objects owned by the example state.
 *
 * @param state example state
 */
static void _state_destroy_geometry(SurfaceOverlayState* state)
{
    ANN(state);
    if (state->contours != NULL)
        dvz_geometry_contours_destroy(state->contours);
    if (state->edges != NULL)
        dvz_geometry_edges_destroy(state->edges);
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    state->contours = NULL;
    state->edges = NULL;
    state->geometry = NULL;
}



/**
 * Draw and apply the live example GUI.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _overlays_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    SurfaceOverlayState* state = (SurfaceOverlayState*)user_data;
    ANN(state);

    bool visibility_changed = false;
    bool material_changed = false;
    bool geometry_changed = false;
    bool wire_changed = false;
    bool contours_changed = false;
    bool contour_style_changed = false;

    if (dvz_gui_begin(gui, "Surface overlays", NULL, 0))
    {
        dvz_gui_separator_text(gui, "View");
        visibility_changed |= dvz_gui_checkbox(gui, "Surface", &state->show_surface);
        visibility_changed |= dvz_gui_checkbox(gui, "Wireframe", &state->show_wire);
        visibility_changed |= dvz_gui_checkbox(gui, "Contours", &state->show_contours);
        if (dvz_gui_button(gui, "Reset view") && state->arcball != NULL)
            dvz_arcball_initial(state->arcball, (vec3){0.55f, 0.0f, -0.25f});

        dvz_gui_separator_text(gui, "Surface");
        geometry_changed |=
            dvz_gui_slider_float(gui, "Height scale", &state->height_scale, 0.15f, 2.5f);
        material_changed |=
            dvz_gui_slider_float(gui, "Opacity", &state->surface_opacity, 0.15f, 1.0f);
        material_changed |=
            dvz_gui_slider_float3(gui, "Light direction", state->light_direction, -1.0f, 1.0f);
        material_changed |= dvz_gui_slider_float(gui, "Ambient", &state->ambient, 0.0f, 1.0f);
        material_changed |= dvz_gui_slider_float(gui, "Diffuse", &state->diffuse, 0.0f, 1.5f);
        material_changed |= dvz_gui_slider_float(gui, "Specular", &state->specular, 0.0f, 1.5f);
        material_changed |=
            dvz_gui_slider_float(gui, "Shininess", &state->shininess, 1.0f, 160.0f);

        dvz_gui_separator_text(gui, "Wireframe");
        wire_changed |= dvz_gui_checkbox(gui, "Boundary only", &state->boundary_only);
        wire_changed |= dvz_gui_color_edit_dvz(gui, "Wire color", &state->wire_color, 0);
        wire_changed |=
            dvz_gui_slider_float_format(gui, "Wire width", &state->wire_width, 0.25f, 5.0f,
                                        "%.2f px");
        wire_changed |=
            dvz_gui_slider_float_format(gui, "Wire z offset", &state->wire_z_offset, 0.0f, 0.025f,
                                        "%.4f");

        dvz_gui_separator_text(gui, "Contours");
        contours_changed |=
            dvz_gui_slider_float_format(gui, "Level count", &state->contour_count, 1.0f,
                                        (float)CONTOUR_LEVEL_MAX, "%.0f");
        contours_changed |= dvz_gui_range_float(
            gui, "Range", &state->contour_range_min, &state->contour_range_max, 0.001f,
            state->zmin, state->zmax, "%.3f");
        contour_style_changed |=
            dvz_gui_slider_float_format(gui, "Major every", &state->contour_major_step, 1.0f,
                                        16.0f, "%.0f");
        contour_style_changed |=
            dvz_gui_color_edit_dvz(gui, "Minor color", &state->contour_minor_color, 0);
        contour_style_changed |=
            dvz_gui_color_edit_dvz(gui, "Major color", &state->contour_major_color, 0);
        contour_style_changed |=
            dvz_gui_slider_float_format(gui, "Contour width", &state->contour_width, 0.5f, 8.0f,
                                        "%.1f px");
        contour_style_changed |= dvz_gui_slider_float(
            gui, "Major width x", &state->contour_major_width, 1.0f, 4.0f);
        contour_style_changed |=
            dvz_gui_slider_float_format(gui, "Contour z offset", &state->contour_z_offset, 0.0f,
                                        0.035f, "%.4f");

        dvz_gui_separator_text(gui, "Diagnostics");
        (void)dvz_gui_checkbox(gui, "Show counts", &state->show_diagnostics);
        if (state->show_diagnostics)
        {
            char line[128] = {0};
            dvz_snprintf(
                line, sizeof(line), "vertices: %u", state->geometry != NULL ?
                                                       state->geometry->vertex_count : 0);
            dvz_gui_text(gui, line);
            dvz_snprintf(
                line, sizeof(line), "triangles: %u",
                state->geometry != NULL ? state->geometry->index_count / 3 : 0);
            dvz_gui_text(gui, line);
            dvz_snprintf(
                line, sizeof(line), "edges: %u",
                state->edges != NULL ? state->edges->edge_count : 0);
            dvz_gui_text(gui, line);
            dvz_snprintf(
                line, sizeof(line), "contour segments: %u",
                state->contours != NULL ? state->contours->segment_count : 0);
            dvz_gui_text(gui, line);
        }
    }
    dvz_gui_end(gui);

    if (geometry_changed)
        (void)_state_rebuild_geometry(state);
    else
    {
        if (contours_changed)
            (void)_state_rebuild_contours(state);
        else if (contour_style_changed)
            (void)_state_upload_contours(state);
        if (wire_changed)
            (void)_state_upload_edges(state);
    }
    if (material_changed)
        (void)_state_apply_material(state);
    if (visibility_changed)
        _state_apply_visibility(state);
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
    SurfaceOverlayState state = {0};
    _state_defaults(&state);

    state.heights = (double*)dvz_calloc(vertex_count, sizeof(double));
    state.colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    EXAMPLE_CHECK(
        state.heights != NULL && state.colors != NULL, "surface_grid_overlays: allocation failed");

    _surface_data(state.heights, state.colors, &state.zmin, &state.zmax);
    state.contour_range_min = (float)state.zmin;
    state.contour_range_max = (float)state.zmax;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    state.panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && state.panel != NULL, "scene setup failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 1.8f;
    camera_desc.eye[1] = -2.25f;
    camera_desc.eye[2] = 1.55f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near_clip = 0.05f;
    camera_desc.far_clip = 100.0f;
    bool ok = dvz_panel_set_camera(state.panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    state.mesh = dvz_mesh(scene, 0);
    state.wire = dvz_segment(scene, 0);
    state.contours_visual = dvz_segment(scene, 0);
    EXAMPLE_CHECK(
        state.mesh != NULL && state.wire != NULL && state.contours_visual != NULL,
        "visual creation failed");

    (void)dvz_visual_set_attr_mutability(
        state.wire, "position_start", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.wire, "position_end", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.wire, "color", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.wire, "stroke_width_px", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.contours_visual, "position_start", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.contours_visual, "position_end", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.contours_visual, "color", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.contours_visual, "stroke_width_px", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);

    ok = _state_rebuild_geometry(&state);
    EXAMPLE_CHECK(ok, "surface and overlay upload failed");
    ok = _state_apply_material(&state);
    EXAMPLE_CHECK(ok, "dvz_visual_set_material() failed");

    int rc = dvz_panel_add_visual(state.panel, state.mesh, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(mesh) failed");
    rc = dvz_panel_add_visual(state.panel, state.wire, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(wire) failed");
    rc = dvz_panel_add_visual(state.panel, state.contours_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(contours) failed");
    _state_apply_visibility(&state);
    dvz_panel_set_background_color(state.panel, dvz_color_from_unit(0.04f, 0.045f, 0.05f, 1.0f));

    app = dvz_app(scene);
    DvzView* win =
        app != NULL ? dvz_view_glfw(app, figure, WIDTH, HEIGHT, "surface grid overlays") : NULL;
    EXAMPLE_CHECK(app != NULL && win != NULL, "app/window setup failed");

    state.arcball = dvz_view_arcball(win, state.panel, NULL);
    if (state.arcball != NULL)
        dvz_arcball_initial(state.arcball, (vec3){0.55f, 0.0f, -0.25f});

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _overlays_gui, &state);

    dvz_app_run(app, frame_count);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    _state_destroy_geometry(&state);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(state.heights);
    dvz_free(state.colors);
    return ret;
}
