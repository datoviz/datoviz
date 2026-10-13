/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares alpha cutout and WBOIT for indexed textured planes around an opaque cube.
 *
 * The left panel applies an effective-alpha cutoff: holes reveal the cube behind each plane.
 * The other panels use WBOIT and reverse the plane attachment order. Rotate the linked cameras
 * to inspect intersecting planes, sampled alpha and opaque depth while the labels stay fixed.
 *
 * Scenario: features_sampled_planes
 * Build: just example-c features/sampled_planes
 * Run: ./build/examples/c/features/sampled_planes --live
 */


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_controller_preview.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants and state                                                                          */
/*************************************************************************************************/

#define TEXTURE_SIZE 64u

typedef struct SampledPlanesState
{
    DvzArcball* arcball;
} SampledPlanesState;

DvzScenarioSpec dvz_example_sampled_planes_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a procedural color field with circular holes and two positive alpha levels.
 *
 * @param scene scene owning the sampled field
 * @return sampled field, or NULL on failure
 */
static DvzSampledField* _texture(DvzScene* scene)
{
    uint8_t pixels[TEXTURE_SIZE * TEXTURE_SIZE * 4] = {0};
    for (uint32_t y = 0; y < TEXTURE_SIZE; y++)
    {
        for (uint32_t x = 0; x < TEXTURE_SIZE; x++)
        {
            int dx = (int)(x % 16) - 8;
            int dy = (int)(y % 16) - 8;
            uint32_t i = 4 * (y * TEXTURE_SIZE + x);
            pixels[i] = 160 + x;
            pixels[i + 1] = 184 + y / 2;
            pixels[i + 2] = 224;
            pixels[i + 3] = dx * dx + dy * dy < 16 ? 0 :
                            ((x / 8 + y / 8) % 2 != 0 ? 224 : 96);
        }
    }
    DvzSampledFieldDesc desc = dvz_sampled_field_desc();
    desc.dim = DVZ_FIELD_DIM_2D;
    desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
    desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    desc.width = TEXTURE_SIZE;
    desc.height = TEXTURE_SIZE;
    DvzSampledField* field = dvz_sampled_field(scene, &desc);
    if (field == NULL)
        return NULL;
    DvzFieldDataView data = dvz_field_data_view();
    data.data = pixels;
    data.bytes_per_row = TEXTURE_SIZE * 4;
    data.rows_per_image = TEXTURE_SIZE;
    return dvz_sampled_field_set_data(field, &data) == DVZ_OK ? field : NULL;
}



/**
 * Add a depth-capable indexed plane in XY or YZ using the shared sampled field.
 *
 * @param scene scene owning the resources
 * @param panel destination panel
 * @param field shared RGBA sampled field
 * @param perpendicular whether to use a YZ plane
 * @param mode masked or WBOIT alpha handling
 * @return true on success
 */
static bool _plane(
    DvzScene* scene, DvzPanel* panel, DvzSampledField* field, bool perpendicular, DvzAlphaMode mode)
{
    vec3 positions[4] = {
        {-0.82f, -0.72f, 0.05f}, {-0.82f, 0.72f, 0.05f},
        {0.82f, -0.72f, 0.05f}, {0.82f, 0.72f, 0.05f},
    };
    vec3 normals[4] = {{0}};
    DvzColor colors[4] = {0};
    vec2 uv[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzColor tint = example_graphite_cyan_color(
        perpendicular ? EXAMPLE_STYLE_COLOR_WARNING : EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < 4; i++)
    {
        if (perpendicular)
        {
            positions[i][2] = positions[i][0];
            positions[i][0] = -0.08f;
        }
        normals[i][perpendicular ? 0 : 2] = 1;
        colors[i] = tint;
    }
    DvzSceneBufferDesc buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    buffer_desc.stride = sizeof(DvzIndex);
    DvzSceneBuffer* buffer = dvz_scene_buffer(scene, &buffer_desc);
    DvzVisual* plane = dvz_mesh(scene, 0);
    if (buffer == NULL || plane == NULL)
        return false;
    if (dvz_scene_buffer_set_data(buffer, indices, sizeof(indices)) != DVZ_OK ||
        dvz_visual_set_data(plane, "position", positions, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "normal", normals, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "color", colors, 4) != DVZ_OK ||
        dvz_visual_set_data(plane, "texcoords", uv, 4) != DVZ_OK ||
        dvz_visual_set_buffer(plane, "index", buffer) != DVZ_OK ||
        dvz_visual_set_field(plane, "texture", field) != DVZ_OK)
        return false;
    DvzFieldSamplingDesc sampling = dvz_field_sampling_desc();
    sampling.min_filter = DVZ_FIELD_FILTER_NEAREST;
    sampling.mag_filter = DVZ_FIELD_FILTER_NEAREST;
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_UNLIT;
    material.alpha_mode = mode;
    material.opacity = mode == DVZ_ALPHA_WBOIT ? 0.62f : 1.0f;
    if (dvz_visual_set_field_sampling(plane, "texture", &sampling) != DVZ_OK ||
        dvz_visual_set_material(plane, &material) != DVZ_OK ||
        dvz_visual_set_alpha_cutoff(plane, 0.5f) != DVZ_OK)
        return false;
    return dvz_panel_add_visual(panel, plane, NULL) == DVZ_OK;
}



/**
 * Add the shared opaque cube and a fixed screen-space label.
 *
 * @param scene scene owning the geometry and annotation
 * @param panel destination panel
 * @param text panel label
 * @return true on success
 */
static bool _cube_and_label(DvzScene* scene, DvzPanel* panel, const char* text)
{
    DvzGeometryCubeDesc cube_desc = dvz_geometry_cube_desc();
    cube_desc.size = 0.62;
    cube_desc.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzGeometry* cube = dvz_geometry_cube(&cube_desc);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (cube == NULL || mesh == NULL)
    {
        if (cube != NULL)
            dvz_geometry_destroy(cube);
        return false;
    }
    DvzResult result = dvz_mesh_set_geometry(mesh, cube);
    dvz_geometry_destroy(cube);
    if (result != DVZ_OK || dvz_panel_add_visual(panel, mesh, NULL) != DVZ_OK)
        return false;
    DvzLabelDesc label = dvz_label_desc();
    label.text = text;
    DvzTextStyle style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 16;
    placement.position[1] = 20;
    placement.has_text_anchor = true;
    DvzAnnotation* annotation = dvz_annotation_label(panel, &label);
    return annotation != NULL && dvz_annotation_set_style(annotation, &style) == DVZ_OK &&
           dvz_annotation_set_placement(annotation, &placement) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario                                                                                     */
/*************************************************************************************************/

/**
 * Build linked cutout and forward/reverse WBOIT plane views.
 *
 * @param ctx scenario context
 * @param out_user output retained preview state
 * @return true on success
 */
static bool _init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    SampledPlanesState* state = dvz_calloc(1, sizeof(*state));
    *out_user = state;
    if (state == NULL)
        return false;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 3);
    DvzSampledField* field = _texture(ctx->scene);
    if (grid == NULL || field == NULL)
        return false;
    DvzPanelReserve margins = {.left_px = 28, .right_px = 28, .top_px = 36, .bottom_px = 36};
    if (dvz_grid_set_margins(grid, &margins) != DVZ_OK ||
        dvz_grid_set_gutter(grid, 20, 0) != DVZ_OK)
        return false;
    DvzCameraDesc camera = example_default_3d_camera_desc(1.15f);
    const char* labels[3] = {"Cutout: alpha >= 0.5", "WBOIT: A then B", "WBOIT: B then A"};
    DvzController* controllers[3] = {0};
    for (uint32_t p = 0; p < 3; p++)
    {
        DvzPanel* panel = dvz_grid_panel(grid, 0, p);
        if (panel == NULL)
            return false;
        example_graphite_cyan_set_panel_background(panel);
        DvzAlphaMode mode = p == 0 ? DVZ_ALPHA_MASK : DVZ_ALPHA_WBOIT;
        if (dvz_panel_set_camera_desc(panel, &camera) != DVZ_OK ||
            !_plane(ctx->scene, panel, field, p == 2, mode) ||
            !_plane(ctx->scene, panel, field, p != 2, mode) ||
            !_cube_and_label(ctx->scene, panel, labels[p]))
            return false;
        controllers[p] = dvz_arcball(ctx->scene, NULL);
        if (controllers[p] == NULL ||
            dvz_scenario_bind_controller(ctx, panel, controllers[p], DVZ_DIM_MASK_XYZ) != DVZ_OK)
            return false;
        DvzArcball* arcball = dvz_controller_arcball(controllers[p]);
        if (arcball == NULL || dvz_arcball_set(arcball, (vec3){0.38f, -0.46f, 0.12f}) != DVZ_OK)
            return false;
        if (p == 0)
            state->arcball = arcball;
    }
    for (uint32_t p = 1; p < 3; p++)
        if (dvz_controller_link(
                ctx->scene, controllers[0], controllers[p],
                DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
                DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
            return false;
    return true;
}



/**
 * Drive deterministic linked-camera rotation for motion previews.
 *
 * @param ctx scenario context
 * @param user retained preview state
 */
static void _frame(DvzScenarioContext* ctx, void* user)
{
    SampledPlanesState* state = user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL)
        return;
    ExamplePreviewArcballDesc motion = {
        .base_angles = {0.38f, -0.46f, 0.12f},
        .amplitude = {0.2f, 0.32f, 0.12f},
        .zoom = 1,
    };
    example_preview_arcball(state->arcball, ctx->preview_frame_index, ctx->preview_frame_count, &motion);
}



/**
 * Release scenario-owned preview state.
 *
 * @param ctx scenario context
 * @param user retained preview state
 */
static void _destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the portable sampled-plane scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_sampled_planes_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_sampled_planes",
        .title = "Sampled Planes",
        .width = EXAMPLE_WINDOW_WIDTH,
        .height = EXAMPLE_WINDOW_HEIGHT,
        .fps = 60,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |
                        DVZ_SCENARIO_REQ_SCENE_BUFFERS | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _init,
        .frame = _frame,
        .destroy = _destroy,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the portable sampled-plane example.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec scenario = dvz_example_sampled_planes_scenario();
    return dvz_scenario_run_native_cli(&scenario, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
