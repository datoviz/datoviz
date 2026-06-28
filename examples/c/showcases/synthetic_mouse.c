/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* synthetic_mouse - animated textured mouse mesh with trajectory and skeleton trails.
 *
 * Scenario: showcase_synthetic_mouse
 * Style: showcase, graphite_cyan, 1600x1200 capture target
 *
 * Prepared data is loaded from `.cache/datoviz/examples/synthetic_mouse/prepared/`.
 * Generate it with:
 *
 *   python tools/data/prepare_synthetic_mouse.py --force
 *
 * Build:  just example-c showcases/synthetic_mouse
 * Run:    ./build/examples/c/showcases/synthetic_mouse --live
 * Smoke:  ./build/examples/c/showcases/synthetic_mouse --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define MOUSE_CACHE_PATH ".cache/datoviz/examples/synthetic_mouse/prepared/synthetic_mouse.bin"
#define MOUSE_MAGIC      "DVZMSYN"
#define MOUSE_VERSION    1u
#define MOUSE_MAX_VERTS  100000u
#define MOUSE_MAX_FRAMES 600u
#define MOUSE_MAX_EDGES  64u
#define MOUSE_TRAIL      14u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MouseHeader
{
    char magic[8];
    uint32_t version;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t frame_count;
    uint32_t keypoint_count;
    uint32_t edge_count;
    uint32_t texture_width;
    uint32_t texture_height;
} MouseHeader;


typedef struct MouseData
{
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t frame_count;
    uint32_t keypoint_count;
    uint32_t edge_count;
    uint32_t texture_width;
    uint32_t texture_height;
    vec2* texcoords;
    vec3* normals;
    vec3* positions;
    DvzIndex* indices;
    vec3* keypoints;
    uint32_t (*edges)[2];
    DvzColor* texture;
    DvzColor* colors;
} MouseData;


typedef struct MouseState
{
    DvzVisual* mesh;
    DvzVisual* skeleton;
    DvzVisual* skeleton_trail;
    DvzVisual* trajectory;
    MouseData data;
    vec3* current_positions;
    vec3* skeleton_starts;
    vec3* skeleton_ends;
    DvzColor* skeleton_colors;
    float* skeleton_widths;
    vec3* trail_starts;
    vec3* trail_ends;
    DvzColor* trail_colors;
    float* trail_widths;
    vec3* trajectory_positions;
    DvzColor* trajectory_colors;
    float* trajectory_widths;
    uint32_t trajectory_subpath;
    uint32_t current_frame;
} MouseState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Free loaded mouse data.
 *
 * @param data mouse data
 */
static void _free_data(MouseData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->colors);
    dvz_free(data->texture);
    dvz_free(data->edges);
    dvz_free(data->keypoints);
    dvz_free(data->indices);
    dvz_free(data->positions);
    dvz_free(data->normals);
    dvz_free(data->texcoords);
    memset(data, 0, sizeof(*data));
}


/**
 * Free scenario state.
 *
 * @param state mouse state
 */
static void _free_state(MouseState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->trajectory_widths);
    dvz_free(state->trajectory_colors);
    dvz_free(state->trajectory_positions);
    dvz_free(state->trail_widths);
    dvz_free(state->trail_colors);
    dvz_free(state->trail_ends);
    dvz_free(state->trail_starts);
    dvz_free(state->skeleton_widths);
    dvz_free(state->skeleton_colors);
    dvz_free(state->skeleton_ends);
    dvz_free(state->skeleton_starts);
    dvz_free(state->current_positions);
    _free_data(&state->data);
    dvz_free(state);
}


/**
 * Return a pointer to one mesh frame.
 *
 * @param data mouse data
 * @param frame frame index
 * @return positions pointer
 */
static const vec3* _mesh_frame(const MouseData* data, uint32_t frame)
{
    ANN(data);
    const uint64_t offset = (uint64_t)(frame % data->frame_count) * data->vertex_count;
    return (const vec3*)&data->positions[offset];
}


/**
 * Return a pointer to one keypoint frame.
 *
 * @param data mouse data
 * @param frame frame index
 * @return keypoints pointer
 */
static const vec3* _keypoint_frame(const MouseData* data, uint32_t frame)
{
    ANN(data);
    const uint64_t offset = (uint64_t)(frame % data->frame_count) * data->keypoint_count;
    return (const vec3*)&data->keypoints[offset];
}


/**
 * Read the prepared mouse animation bundle.
 *
 * @param data output mouse data
 * @return true on success
 */
static bool _load_mouse(MouseData* data)
{
    ANN(data);
    memset(data, 0, sizeof(*data));

    FILE* fp = fopen(MOUSE_CACHE_PATH, "rb");
    if (fp == NULL)
    {
        dvz_fprintf(
            stderr, "synthetic_mouse: missing prepared cache. Run "
                    "`python tools/data/prepare_synthetic_mouse.py --force`.\n");
        return false;
    }

    bool ok = false;
    MouseHeader header = {0};
    if (fread(&header, sizeof(header), 1, fp) != 1)
        goto cleanup;
    if (
        memcmp(header.magic, MOUSE_MAGIC, strlen(MOUSE_MAGIC)) != 0 ||
        header.version != MOUSE_VERSION || header.vertex_count == 0 ||
        header.vertex_count > MOUSE_MAX_VERTS || header.index_count == 0 ||
        header.frame_count == 0 || header.frame_count > MOUSE_MAX_FRAMES ||
        header.keypoint_count == 0 || header.edge_count == 0 ||
        header.edge_count > MOUSE_MAX_EDGES || header.texture_width == 0 ||
        header.texture_height == 0)
    {
        goto cleanup;
    }

    data->vertex_count = header.vertex_count;
    data->index_count = header.index_count;
    data->frame_count = header.frame_count;
    data->keypoint_count = header.keypoint_count;
    data->edge_count = header.edge_count;
    data->texture_width = header.texture_width;
    data->texture_height = header.texture_height;

    const uint64_t vertex_count = data->vertex_count;
    const uint64_t frame_vertices = (uint64_t)data->frame_count * data->vertex_count;
    const uint64_t keypoints = (uint64_t)data->frame_count * data->keypoint_count;
    const uint64_t texels = (uint64_t)data->texture_width * data->texture_height;
    data->texcoords = (vec2*)dvz_calloc(vertex_count, sizeof(*data->texcoords));
    data->normals = (vec3*)dvz_calloc(vertex_count, sizeof(*data->normals));
    data->positions = (vec3*)dvz_calloc(frame_vertices, sizeof(*data->positions));
    data->indices = (DvzIndex*)dvz_calloc(data->index_count, sizeof(*data->indices));
    data->keypoints = (vec3*)dvz_calloc(keypoints, sizeof(*data->keypoints));
    data->edges = (uint32_t(*)[2])dvz_calloc(data->edge_count, sizeof(*data->edges));
    data->texture = (DvzColor*)dvz_calloc(texels, sizeof(*data->texture));
    data->colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(*data->colors));
    if (
        data->texcoords == NULL || data->normals == NULL || data->positions == NULL ||
        data->indices == NULL || data->keypoints == NULL || data->edges == NULL ||
        data->texture == NULL || data->colors == NULL)
    {
        goto cleanup;
    }

    if (fread(data->texcoords, sizeof(*data->texcoords), vertex_count, fp) != vertex_count)
        goto cleanup;
    if (fread(data->normals, sizeof(*data->normals), vertex_count, fp) != vertex_count)
        goto cleanup;
    if (fread(data->positions, sizeof(*data->positions), frame_vertices, fp) != frame_vertices)
        goto cleanup;
    if (fread(data->indices, sizeof(*data->indices), data->index_count, fp) != data->index_count)
        goto cleanup;
    if (fread(data->keypoints, sizeof(*data->keypoints), keypoints, fp) != keypoints)
        goto cleanup;
    if (fread(data->edges, sizeof(*data->edges), data->edge_count, fp) != data->edge_count)
        goto cleanup;
    if (fread(data->texture, sizeof(*data->texture), texels, fp) != texels)
        goto cleanup;

    for (uint32_t i = 0; i < data->vertex_count; i++)
        data->colors[i] = dvz_color_rgba(255, 255, 255, 255);
    ok = true;

cleanup:
    fclose(fp);
    if (!ok)
        _free_data(data);
    return ok;
}


/**
 * Create the mouse texture field.
 *
 * @param scene scene owning the field
 * @param data mouse data
 * @return sampled field, or NULL
 */
static DvzSampledField* _add_texture(DvzScene* scene, const MouseData* data)
{
    ANN(scene);
    ANN(data);

    DvzSampledField* texture = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = data->texture_width,
                   .height = data->texture_height,
                   .depth = 1});
    if (texture == NULL)
        return NULL;
    if (!dvz_sampled_field_set_data(
            texture, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                         .data = data->texture,
                         .bytes_per_row = data->texture_width * sizeof(DvzColor),
                         .rows_per_image = data->texture_height}))
        return NULL;
    return texture;
}


/**
 * Add the animated textured mesh visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param state mouse state
 * @param texture mesh texture field
 * @return true on success
 */
static bool
_add_mesh(DvzScene* scene, DvzPanel* panel, MouseState* state, DvzSampledField* texture)
{
    ANN(scene);
    ANN(panel);
    ANN(state);
    ANN(texture);

    state->current_positions = (vec3*)dvz_calloc(
        state->data.vertex_count, sizeof(*state->current_positions));
    if (state->current_positions == NULL)
        return false;
    memcpy(
        state->current_positions, _mesh_frame(&state->data, 0),
        (uint64_t)state->data.vertex_count * sizeof(*state->current_positions));

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return false;

    DvzGeometry* geometry = dvz_geom_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = 1.0,
        .sectors = 64,
        .rings = 32,
        .color = {255, 255, 255, 255},
    });
    if (geometry == NULL)
        return false;
    dmat4 body = {
        {0.92, 0.00, 0.00, 0.00},
        {0.00, 0.38, 0.00, 0.00},
        {0.00, 0.00, 0.28, 0.00},
        {0.00, 0.00, 0.36, 1.00},
    };
    if (dvz_geometry_transform(geometry, body) != 0)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    int rc = dvz_mesh_set_geometry(mesh, geometry);
    dvz_geometry_destroy(geometry);
    if (rc != 0)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.30f;
    material.light_direction[1] = +0.45f;
    material.light_direction[2] = +0.80f;
    material.phong.ambient = 0.38f;
    material.phong.diffuse = 0.74f;
    material.phong.specular = 0.12f;
    material.phong.shininess = 24.0f;
    if (dvz_visual_set_material(mesh, &material) != 0)
        return false;
    if (!dvz_visual_set_field(mesh, "texture", texture))
        return false;
    if (dvz_panel_add_visual(panel, mesh, NULL) != 0)
        return false;

    state->mesh = mesh;
    return true;
}


/**
 * Fill skeleton segment arrays for a frame.
 *
 * @param state mouse state
 * @param frame frame index
 */
static void _fill_skeleton(MouseState* state, uint32_t frame)
{
    ANN(state);
    const vec3* points = _keypoint_frame(&state->data, frame);
    DvzColor current = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    for (uint32_t e = 0; e < state->data.edge_count; e++)
    {
        const uint32_t a = state->data.edges[e][0];
        const uint32_t b = state->data.edges[e][1];
        memcpy(state->skeleton_starts[e], points[a], sizeof(vec3));
        memcpy(state->skeleton_ends[e], points[b], sizeof(vec3));
        state->skeleton_colors[e] = current;
        state->skeleton_widths[e] = 3.0f;
    }
}


/**
 * Fill fading skeleton trail arrays.
 *
 * @param state mouse state
 * @param frame current frame index
 */
static void _fill_trail(MouseState* state, uint32_t frame)
{
    ANN(state);
    DvzColor trail = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    uint32_t out = 0;
    for (uint32_t h = 0; h < MOUSE_TRAIL; h++)
    {
        const uint32_t f = (frame + state->data.frame_count - h) % state->data.frame_count;
        const vec3* points = _keypoint_frame(&state->data, f);
        const float t = 1.0f - (float)h / (float)MOUSE_TRAIL;
        for (uint32_t e = 0; e < state->data.edge_count; e++, out++)
        {
            const uint32_t a = state->data.edges[e][0];
            const uint32_t b = state->data.edges[e][1];
            memcpy(state->trail_starts[out], points[a], sizeof(vec3));
            memcpy(state->trail_ends[out], points[b], sizeof(vec3));
            state->trail_colors[out] = trail;
            state->trail_colors[out].a = (uint8_t)(34.0f + 116.0f * t);
            state->trail_widths[out] = 1.2f + 1.2f * t;
        }
    }
}


/**
 * Fill trajectory path arrays from recent spine keypoints.
 *
 * @param state mouse state
 * @param frame current frame index
 */
static void _fill_trajectory(MouseState* state, uint32_t frame)
{
    ANN(state);
    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    for (uint32_t i = 0; i < MOUSE_TRAIL; i++)
    {
        const uint32_t f = (frame + state->data.frame_count - (MOUSE_TRAIL - 1u - i)) %
                           state->data.frame_count;
        const vec3* points = _keypoint_frame(&state->data, f);
        memcpy(state->trajectory_positions[i], points[1], sizeof(vec3));
        state->trajectory_colors[i] = color;
        state->trajectory_colors[i].a = (uint8_t)(60u + 12u * i);
        state->trajectory_widths[i] = 2.0f + 0.12f * (float)i;
    }
}


/**
 * Add skeleton, skeleton trail, and trajectory visuals.
 *
 * @param scene scene owning visuals
 * @param panel panel receiving visuals
 * @param state mouse state
 * @return true on success
 */
static bool _add_overlays(DvzScene* scene, DvzPanel* panel, MouseState* state)
{
    ANN(scene);
    ANN(panel);
    ANN(state);

    const uint32_t edge_count = state->data.edge_count;
    const uint32_t trail_count = edge_count * MOUSE_TRAIL;
    state->skeleton_starts = (vec3*)dvz_calloc(edge_count, sizeof(*state->skeleton_starts));
    state->skeleton_ends = (vec3*)dvz_calloc(edge_count, sizeof(*state->skeleton_ends));
    state->skeleton_colors = (DvzColor*)dvz_calloc(edge_count, sizeof(*state->skeleton_colors));
    state->skeleton_widths = (float*)dvz_calloc(edge_count, sizeof(*state->skeleton_widths));
    state->trail_starts = (vec3*)dvz_calloc(trail_count, sizeof(*state->trail_starts));
    state->trail_ends = (vec3*)dvz_calloc(trail_count, sizeof(*state->trail_ends));
    state->trail_colors = (DvzColor*)dvz_calloc(trail_count, sizeof(*state->trail_colors));
    state->trail_widths = (float*)dvz_calloc(trail_count, sizeof(*state->trail_widths));
    state->trajectory_positions =
        (vec3*)dvz_calloc(MOUSE_TRAIL, sizeof(*state->trajectory_positions));
    state->trajectory_colors =
        (DvzColor*)dvz_calloc(MOUSE_TRAIL, sizeof(*state->trajectory_colors));
    state->trajectory_widths = (float*)dvz_calloc(MOUSE_TRAIL, sizeof(*state->trajectory_widths));
    if (
        state->skeleton_starts == NULL || state->skeleton_ends == NULL ||
        state->skeleton_colors == NULL || state->skeleton_widths == NULL ||
        state->trail_starts == NULL || state->trail_ends == NULL || state->trail_colors == NULL ||
        state->trail_widths == NULL || state->trajectory_positions == NULL ||
        state->trajectory_colors == NULL || state->trajectory_widths == NULL)
    {
        return false;
    }
    _fill_skeleton(state, 0);
    _fill_trail(state, 0);
    _fill_trajectory(state, 0);

    DvzVisual* skeleton = dvz_segment(scene, 0);
    DvzVisual* trail = dvz_segment(scene, 0);
    DvzVisual* trajectory = dvz_path(scene, 0);
    if (skeleton == NULL || trail == NULL || trajectory == NULL)
        return false;

    DvzVisualDataUpdate skeleton_updates[] = {
        {.attr_name = "position_start", .data = state->skeleton_starts, .item_count = edge_count},
        {.attr_name = "position_end", .data = state->skeleton_ends, .item_count = edge_count},
        {.attr_name = "color", .data = state->skeleton_colors, .item_count = edge_count},
        {.attr_name = "stroke_width_px", .data = state->skeleton_widths, .item_count = edge_count},
    };
    if (dvz_visual_set_data_many(skeleton, skeleton_updates, DVZ_ARRAY_COUNT(skeleton_updates)) != 0)
        return false;
    if (dvz_segment_set_caps(skeleton, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;

    DvzVisualDataUpdate trail_updates[] = {
        {.attr_name = "position_start", .data = state->trail_starts, .item_count = trail_count},
        {.attr_name = "position_end", .data = state->trail_ends, .item_count = trail_count},
        {.attr_name = "color", .data = state->trail_colors, .item_count = trail_count},
        {.attr_name = "stroke_width_px", .data = state->trail_widths, .item_count = trail_count},
    };
    if (dvz_visual_set_data_many(trail, trail_updates, DVZ_ARRAY_COUNT(trail_updates)) != 0)
        return false;
    if (dvz_segment_set_caps(trail, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;

    DvzVisualDataUpdate trajectory_updates[] = {
        {.attr_name = "position", .data = state->trajectory_positions, .item_count = MOUSE_TRAIL},
        {.attr_name = "color", .data = state->trajectory_colors, .item_count = MOUSE_TRAIL},
        {.attr_name = "stroke_width_px", .data = state->trajectory_widths, .item_count = MOUSE_TRAIL},
    };
    if (dvz_visual_set_data_many(
            trajectory, trajectory_updates, DVZ_ARRAY_COUNT(trajectory_updates)) != 0)
        return false;
    state->trajectory_subpath = MOUSE_TRAIL;
    if (dvz_path_set_subpaths(trajectory, 1, &state->trajectory_subpath) != 0)
        return false;
    if (dvz_path_set_caps(trajectory, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;

    if (dvz_panel_add_visual(panel, trail, NULL) != 0 ||
        dvz_panel_add_visual(panel, trajectory, NULL) != 0 ||
        dvz_panel_add_visual(panel, skeleton, NULL) != 0)
        return false;

    state->skeleton = skeleton;
    state->skeleton_trail = trail;
    state->trajectory = trajectory;
    return true;
}


/**
 * Add reference grid and orientation gizmo scene aids.
 *
 * @param panel target panel
 * @param controller source arcball controller
 * @return true on success
 */
static bool _add_scene_aids(DvzPanel* panel, DvzController* controller)
{
    ANN(panel);
    ANN(controller);

    DvzReferenceGridDesc grid = dvz_reference_grid_desc();
    grid.plane = DVZ_REFERENCE_GRID_XZ;
    grid.origin[1] = -0.38f;
    grid.size[0] = 7.2f;
    grid.size[1] = 2.6f;
    grid.spacing = 0.25f;
    grid.major_every = 4;
    if (dvz_reference_grid(panel, &grid) == NULL)
        return false;

    (void)controller;
    return true;
}


/**
 * Update all animated visuals for one frame.
 *
 * @param state mouse state
 * @param frame frame index
 * @return true on success
 */
static bool _set_frame(MouseState* state, uint32_t frame)
{
    if (state == NULL || state->mesh == NULL)
        return false;
    frame %= state->data.frame_count;
    if (frame == state->current_frame)
        return true;

    const vec3* points = _keypoint_frame(&state->data, frame);
    mat4 model = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {points[1][0], points[1][1], -0.18f, 1.0f},
    };
    if (dvz_visual_set_transform(state->mesh, model) != 0)
        return false;

    _fill_skeleton(state, frame);
    _fill_trail(state, frame);
    _fill_trajectory(state, frame);
    const uint32_t trail_count = state->data.edge_count * MOUSE_TRAIL;
    DvzVisualDataUpdate skeleton_updates[] = {
        {.attr_name = "position_start", .data = state->skeleton_starts, .item_count = state->data.edge_count},
        {.attr_name = "position_end", .data = state->skeleton_ends, .item_count = state->data.edge_count},
        {.attr_name = "color", .data = state->skeleton_colors, .item_count = state->data.edge_count},
        {.attr_name = "stroke_width_px", .data = state->skeleton_widths, .item_count = state->data.edge_count},
    };
    DvzVisualDataUpdate trail_updates[] = {
        {.attr_name = "position_start", .data = state->trail_starts, .item_count = trail_count},
        {.attr_name = "position_end", .data = state->trail_ends, .item_count = trail_count},
        {.attr_name = "color", .data = state->trail_colors, .item_count = trail_count},
        {.attr_name = "stroke_width_px", .data = state->trail_widths, .item_count = trail_count},
    };
    DvzVisualDataUpdate trajectory_updates[] = {
        {.attr_name = "position", .data = state->trajectory_positions, .item_count = MOUSE_TRAIL},
        {.attr_name = "color", .data = state->trajectory_colors, .item_count = MOUSE_TRAIL},
        {.attr_name = "stroke_width_px", .data = state->trajectory_widths, .item_count = MOUSE_TRAIL},
    };
    if (dvz_visual_set_data_many(state->skeleton, skeleton_updates, DVZ_ARRAY_COUNT(skeleton_updates)) != 0)
        return false;
    if (dvz_visual_set_data_many(state->skeleton_trail, trail_updates, DVZ_ARRAY_COUNT(trail_updates)) != 0)
        return false;
    if (dvz_visual_set_data_many(state->trajectory, trajectory_updates, DVZ_ARRAY_COUNT(trajectory_updates)) != 0)
        return false;

    state->current_frame = frame;
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Animate the mouse and trails.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    MouseState* state = (MouseState*)user;
    if (ctx == NULL || state == NULL || state->data.frame_count == 0)
        return;
    const uint32_t frame = (uint32_t)(ctx->frame_index % state->data.frame_count);
    if (!_set_frame(state, frame))
        fprintf(stderr, "synthetic_mouse: failed to update frame %" PRIu32 "\n", frame);
}


/**
 * Initialize the synthetic mouse showcase.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    MouseState* state = (MouseState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (!_load_mouse(&state->data))
        goto error;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 2.2f;
    camera.view.eye[1] = 2.0f;
    camera.view.eye[2] = 4.4f;
    camera.projection.fov_y = 0.58f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        goto error;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        goto error;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        goto error;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        goto error;
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.18f, +0.25f});
    if (!_add_scene_aids(panel, controller))
        goto error;

    DvzSampledField* texture = _add_texture(ctx->scene, &state->data);
    if (texture == NULL)
        goto error;
    if (!_add_mesh(ctx->scene, panel, state, texture))
        goto error;
    if (!_add_overlays(ctx->scene, panel, state))
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}


/**
 * Destroy the synthetic mouse showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((MouseState*)user);
}


/**
 * Return the synthetic mouse scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _synthetic_mouse_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcase_synthetic_mouse",
        .title = "synthetic_mouse",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the synthetic mouse showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _synthetic_mouse_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
