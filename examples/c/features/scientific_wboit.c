/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares meshes, RGBA points and a sampled plane with reversed transparent order.
 *
 * Two translucent surface regions, a moving RGBA point collection and a physical sampled plane
 * share weighted OIT. The plane's RGBA texture has transparent holes and two positive alpha levels.
 * Opaque recording-site spheres and round trajectories keep their depth and silhouettes as the
 * scene rotates. Both panels use identical data and linked cameras; their transparent attachment
 * orders are opposite. Weighted OIT approximates layering, so overlapping colors need not match
 * exact source-over compositing. Point positions, colors, sizes and alpha are retained updates.
 *
 * Build: just example-c features/scientific_wboit
 * Run:   ./build/examples/c/features/scientific_wboit --live
 * Smoke: ./build/examples/c/features/scientific_wboit --png
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_controller_preview.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_scientific_wboit_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define POINT_COUNT 768u
#define PATH_COUNT  48u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ScientificWboitState
{
    DvzVisual* points[2];
    DvzArcball* arcball;
    vec3 positions[POINT_COUNT];
    DvzColor colors[POINT_COUNT];
    float sizes[POINT_COUNT];
} ScientificWboitState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill identical deterministic scientific points for both panels.
 *
 * @param state retained point payloads
 * @param time elapsed scenario time
 */
static void _fill_points(ScientificWboitState* state, double time)
{
    const float phase = TAU * (float)time / 3.0f;
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float u = (float)i / (float)(POINT_COUNT - 1);
        const float angle = 2.39996323f * (float)i;
        const float y = 1.0f - 2.0f * u;
        const float radius = sqrtf(fmaxf(0.0f, 1.0f - y * y));
        const float shell = .50f + .36f * (float)(i % 11u) / 10.0f;
        state->positions[i][0] = shell * radius * cosf(angle);
        state->positions[i][1] = .72f * shell * y;
        state->positions[i][2] = shell * radius * sinf(angle) + .06f * sinf(phase + 8.0f * u);
        state->colors[i] = (DvzColor){
            (uint8_t)(250.0f - 154.0f * u), (uint8_t)(204.0f - 39.0f * u),
            (uint8_t)(21.0f + 229.0f * u),
            i % 17u == 0 ? 0 : (uint8_t)(64.0f + 96.0f * (.5f + .5f * sinf(phase + TAU * u)))};
        state->sizes[i] = 3.0f + 5.0f * (.5f + .5f * cosf(phase + TAU * u));
    }
}



/**
 * Replace all authored point attributes together.
 *
 * @param state retained scenario state
 * @return whether both point updates succeeded
 */
static bool _upload_points(ScientificWboitState* state)
{
    const DvzVisualDataUpdate updates[] = {
        {"position", state->positions, POINT_COUNT},
        {"color", state->colors, POINT_COUNT},
        {"diameter_px", state->sizes, POINT_COUNT},
    };
    for (uint32_t panel = 0; panel < 2; panel++)
        if (dvz_visual_set_data_many(state->points[panel], updates, 3) != DVZ_OK)
            return false;
    return true;
}



/**
 * Create one lit translucent regional surface.
 *
 * @param scene scene owning the surface
 * @param center_x region translation along X
 * @param color authored surface RGBA
 * @return retained mesh, or NULL on failure
 */
static DvzVisual* _surface(DvzScene* scene, float center_x, DvzColor color)
{
    DvzGeometry* geometry = dvz_geometry_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = .62,
        .sectors = 48,
        .rings = 24,
        .color = color,
    });
    if (geometry == NULL)
        return NULL;
    DvzVisual* mesh = dvz_mesh(scene, 0);
    const bool uploaded = mesh != NULL && dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK;
    dvz_geometry_destroy(geometry);
    if (!uploaded)
        return NULL;
    DvzMaterialDesc material = example_default_phong_material_desc();
    material.alpha_mode = DVZ_ALPHA_WBOIT;
    if (dvz_visual_set_material(mesh, &material) != DVZ_OK)
        return NULL;
    mat4 transform = {{0}};
    for (uint32_t i = 0; i < 4; i++)
        transform[i][i] = 1;
    transform[3][0] = center_x;
    if (dvz_visual_set_transform(mesh, transform) != DVZ_OK)
        return NULL;
    return mesh;
}


/**
 * Create a physical sampled plane crossing the regional surfaces.
 *
 * @param scene scene owning the field, index buffer and plane
 * @return retained plane, or NULL on failure
 */
static DvzVisual* _sampled_plane(DvzScene* scene)
{
    enum { SIZE = 32 };
    uint8_t pixels[SIZE * SIZE * 4] = {0};
    for (uint32_t y = 0; y < SIZE; y++)
    {
        for (uint32_t x = 0; x < SIZE; x++)
        {
            int dx = (int)(x % 8) - 4;
            int dy = (int)(y % 8) - 4;
            uint32_t i = 4 * (y * SIZE + x);
            pixels[i] = 244;
            pixels[i + 1] = 114;
            pixels[i + 2] = 182;
            pixels[i + 3] = dx * dx + dy * dy < 5 ? 0 :
                            ((x / 8 + y / 8) % 2 != 0 ? 144 : 208);
        }
    }
    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.dim = DVZ_FIELD_DIM_2D;
    field_desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
    field_desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    field_desc.width = field_desc.height = SIZE;
    DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
    DvzFieldDataView data = dvz_field_data_view();
    data.data = pixels;
    data.bytes_per_row = SIZE * 4;
    data.rows_per_image = SIZE;
    DvzSceneBufferDesc index_desc = dvz_scene_buffer_desc();
    index_desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    index_desc.stride = sizeof(DvzIndex);
    DvzSceneBuffer* index = dvz_scene_buffer(scene, &index_desc);
    DvzVisual* plane = dvz_mesh(scene, 0);
    if (field == NULL || index == NULL || plane == NULL)
        return NULL;
    vec3 positions[4] = {
        {-.68f, -.60f, -.0704f}, {-.68f, .60f, -.0704f},
        {.68f, -.60f, .3104f}, {.68f, .60f, .3104f},
    };
    vec3 normals[4] = {
        {-.26963f, 0, .96296f}, {-.26963f, 0, .96296f},
        {-.26963f, 0, .96296f}, {-.26963f, 0, .96296f},
    };
    vec2 uv[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    DvzColor colors[4] = {
        {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    DvzIndex indices[6] = {0, 2, 1, 2, 3, 1};
    DvzFieldSamplingDesc sampling = dvz_field_sampling_desc();
    sampling.min_filter = sampling.mag_filter = DVZ_FIELD_FILTER_NEAREST;
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_UNLIT;
    material.alpha_mode = DVZ_ALPHA_WBOIT;
    material.opacity = .50f;
    if (dvz_sampled_field_set_data(field, &data) != DVZ_OK ||
        dvz_scene_buffer_set_data(index, indices, sizeof(indices)) != DVZ_OK ||
        dvz_visual_set_data(plane, "position", positions, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "normal", normals, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "color", colors, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "texcoords", uv, 4) != DVZ_OK ||
        dvz_visual_set_buffer(plane, "index", index) != DVZ_OK ||
        dvz_visual_set_field(plane, "texture", field) != DVZ_OK ||
        dvz_visual_set_field_sampling(plane, "texture", &sampling) != DVZ_OK ||
        dvz_visual_set_material(plane, &material) != DVZ_OK)
        return NULL;
    return plane;
}



/**
 * Add opaque analytic sites and independent round trajectories.
 *
 * @param scene scene owning visuals
 * @param panel panel receiving visuals
 * @return whether all visuals were added
 */
static bool _opaque_context(DvzScene* scene, DvzPanel* panel)
{
    DvzVisual* spheres = dvz_sphere(scene, 0);
    vec3 centers[2] = {{-.52f, -.14f, .45f}, {.52f, .14f, -.45f}};
    DvzColor colors[2] = {{250, 204, 21, 255}, {248, 113, 113, 255}};
    float radii[2] = {.12f, .12f};
    const DvzVisualDataUpdate sphere_data[] = {
        {"position", centers, 2}, {"color", colors, 2}, {"radius", radii, 2}};
    if (spheres == NULL || dvz_visual_set_data_many(spheres, sphere_data, 3) != DVZ_OK ||
        dvz_panel_add_visual(panel, spheres, NULL) != DVZ_OK)
        return false;
    for (uint32_t trajectory = 0; trajectory < 2; trajectory++)
    {
        vec3 positions[PATH_COUNT] = {{0}};
        DvzColor path_colors[PATH_COUNT] = {{0}};
        float widths[PATH_COUNT] = {0};
        for (uint32_t i = 0; i < PATH_COUNT; i++)
        {
            const float u = (float)i / (float)(PATH_COUNT - 1);
            positions[i][0] = (trajectory == 0 ? -.22f : .22f) + .10f * sinf(TAU * u);
            positions[i][1] = -.86f + 1.72f * u;
            positions[i][2] = .72f * cosf(TAU * u + (float)trajectory * 1.8f);
            path_colors[i] =
                trajectory == 0 ? (DvzColor){217, 226, 236, 255} : (DvzColor){74, 222, 128, 255};
            widths[i] = trajectory == 0 ? 4 : 6;
        }
        DvzVisual* path = dvz_path(scene, 0);
        const DvzVisualDataUpdate path_data[] = {
            {"position", positions, PATH_COUNT},
            {"color", path_colors, PATH_COUNT},
            {"stroke_width_px", widths, PATH_COUNT}};
        if (path == NULL || dvz_visual_set_data_many(path, path_data, 3) != DVZ_OK ||
            dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != DVZ_OK ||
            dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4) != DVZ_OK ||
            dvz_panel_add_visual(panel, path, NULL) != DVZ_OK)
            return false;
    }
    return true;
}



/**
 * Label one panel's authored transparent order.
 *
 * @param panel receiving panel
 * @param text label text
 * @return whether label creation succeeded
 */
static bool _label(DvzPanel* panel, const char* text)
{
    DvzLabelDesc label = dvz_label_desc();
    label.text = text;
    DvzAnnotation* annotation = dvz_annotation_label(panel, &label);
    DvzTextStyle style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 18;
    placement.position[1] = 18;
    placement.text_anchor[0] = placement.text_anchor[1] = 0;
    placement.has_text_anchor = true;
    return annotation != NULL && dvz_annotation_set_style(annotation, &style) == DVZ_OK &&
           dvz_annotation_set_placement(annotation, &placement) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize two views with identical data and opposite transparent attachment orders.
 *
 * @param ctx scenario context
 * @param out_user retained scenario state
 * @return whether construction succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    ScientificWboitState* state = dvz_calloc(1, sizeof(ScientificWboitState));
    if (state == NULL)
        return false;
    *out_user = state;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL || dvz_grid_set_gutter(grid, 24, 0) != DVZ_OK ||
        dvz_grid_set_margins(grid, &(DvzPanelReserve){32, 32, 32, 32}) != DVZ_OK)
        return false;
    DvzController* controllers[2] = {0};
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzPanel* panel = dvz_grid_panel(grid, 0, i);
        if (panel == NULL)
            return false;
        example_graphite_cyan_set_panel_background(panel);
        if (example_set_default_3d_camera(panel, .82f) == NULL ||
            !_label(panel, i == 0 ? "Forward order" : "Reverse order"))
            return false;
        controllers[i] = dvz_arcball(ctx->scene, NULL);
        if (controllers[i] == NULL ||
            dvz_scenario_bind_controller(ctx, panel, controllers[i], DVZ_DIM_MASK_XYZ) != DVZ_OK)
            return false;
        DvzVisual* surfaces[2] = {
            _surface(ctx->scene, -.23f, (DvzColor){34, 211, 238, 68}),
            _surface(ctx->scene, .23f, (DvzColor){96, 165, 250, 92})};
        DvzVisual* plane = _sampled_plane(ctx->scene);
        state->points[i] = dvz_point(ctx->scene, 0);
        if (surfaces[0] == NULL || surfaces[1] == NULL || plane == NULL ||
            state->points[i] == NULL ||
            dvz_visual_set_alpha_mode(state->points[i], DVZ_ALPHA_WBOIT) != DVZ_OK ||
            dvz_panel_add_visual(panel, surfaces[i], NULL) != DVZ_OK ||
            (i == 1 && dvz_panel_add_visual(panel, plane, NULL) != DVZ_OK) ||
            dvz_panel_add_visual(panel, state->points[i], NULL) != DVZ_OK ||
            (i == 0 && dvz_panel_add_visual(panel, plane, NULL) != DVZ_OK) ||
            dvz_panel_add_visual(panel, surfaces[1 - i], NULL) != DVZ_OK ||
            !_opaque_context(ctx->scene, panel))
            return false;
    }
    if (dvz_controller_link(
            ctx->scene, controllers[0], controllers[1],
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    state->arcball = dvz_controller_arcball(controllers[0]);
    _fill_points(state, 0);
    return _upload_points(state);
}



/**
 * Update point attributes and the deterministic preview camera.
 *
 * @param ctx scenario context
 * @param user retained state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    ScientificWboitState* state = user;
    if (ctx == NULL || state == NULL)
        return;
    _fill_points(state, ctx->preview_mode ? dvz_scenario_preview_time(ctx) : ctx->time);
    if (!_upload_points(state))
        return;
    if (ctx->preview_mode)
    {
        ExamplePreviewArcballDesc preview = example_preview_arcball_cube_desc();
        example_preview_arcball(
            state->arcball, ctx->preview_frame_index, ctx->preview_frame_count, &preview);
    }
}



/**
 * Release retained example payloads.
 *
 * @param ctx scenario context
 * @param user retained state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the portable scientific WBOIT scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_scientific_wboit_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_scientific_wboit",
        .title = "Scientific 3D Transparency",
        .width = EXAMPLE_WINDOW_WIDTH,
        .height = EXAMPLE_WINDOW_HEIGHT,
        .fps = 60,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_MESH_VISUAL |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/**
 * Run the scientific WBOIT scenario.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_scientific_wboit_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
