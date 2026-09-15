/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* ibl_atlas_webgpu_spike - Native/WebGPU portability probe for an atlas-like 3D scene.
 *
 * Scenario: lab_ibl_atlas_webgpu_spike
 * Style: lab, graphite_cyan, 1280x720 window target
 *
 * Build:   just example-c lab/ibl_atlas_webgpu_spike
 * Native:  ./build/examples/c/lab/ibl_atlas_webgpu_spike --live
 * Browser: examples/webgpu/examples.html?demo=wasm-ibl-atlas-spike
 * Smoke:   ./build/examples/c/lab/ibl_atlas_webgpu_spike --png
 *
 * Scope: this deliberately small canonical C scenario covers one opaque indexed mesh, four
 * colored probe sites, and arcball input through the native runner and the WebGPU/WASM bridge. It
 * does not cover Python or ImGui integration, prepared dataset transport, picking, or WBOIT.
 *
 * The geometry and signed identity semantics are derived from the renderer-neutral synthetic
 * mesh-pack fixture in ibl-atlas-assets/tests/fixtures/mesh-pack-v1. The fixture is test-only, is
 * not Allen geometry, and must not be used as scientific data or a runtime fallback. The source
 * fixture has 10 vertices, 12 triangles, and signed presentation identities -315 and +315. This
 * scenario expands its indexed triangles only to preserve a flat color per presentation side.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/controller/arcball.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define FIXTURE_VERTEX_COUNT 10u
#define FIXTURE_INDEX_COUNT  36u
#define FIXTURE_FACE_COUNT   12u
#define PROBE_SITE_COUNT     4u

#define SIGNED_ALLEN_LEFT  (-315)
#define SIGNED_ALLEN_RIGHT (+315)



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_lab_ibl_atlas_webgpu_spike_scenario(void);



/*************************************************************************************************/
/*  Synthetic renderer-neutral fixture                                                           */
/*************************************************************************************************/

// Exact decoded positions and normals from mesh pack synthetic-mesh-d2d5c2632f8a.
static const vec3 FIXTURE_POSITIONS[FIXTURE_VERTEX_COUNT] = {
    {-2.0f, +0.0f, +0.0f}, {+2.0f, +0.0f, +0.0f}, {+0.0f, -1.0f, +0.0f}, {+0.0f, +1.0f, +0.0f},
    {+0.0f, +0.0f, -1.0f}, {+0.0f, +0.0f, +1.0f}, {+3.0f, +0.0f, +0.0f}, {+5.0f, +0.0f, +0.0f},
    {+4.0f, -1.0f, +0.0f}, {+4.0f, +0.0f, +1.0f},
};

static const vec3 FIXTURE_NORMALS[FIXTURE_VERTEX_COUNT] = {
    {-1.0000000f, +0.0000000f, +0.0000000f}, {+1.0000000f, +0.0000000f, +0.0000000f},
    {+0.0000000f, -1.0000000f, +0.0000000f}, {+0.0000000f, +1.0000000f, +0.0000000f},
    {+0.0000000f, +0.0000000f, -1.0000000f}, {+0.0000000f, +0.0000000f, +1.0000000f},
    {-0.5773503f, +0.5773503f, -0.5773503f}, {+0.5773503f, +0.5773503f, -0.5773503f},
    {+0.0000000f, -1.0000000f, +0.0000000f}, {+0.0000000f, +0.0000000f, +1.0000000f},
};

static const DvzIndex FIXTURE_INDICES[FIXTURE_INDEX_COUNT] = {
    0, 2, 5, 0, 5, 3, 0, 3, 4, 0, 4, 2, 1, 5, 2, 1, 3, 5,
    1, 4, 3, 1, 2, 4, 6, 8, 9, 6, 9, 7, 6, 7, 8, 7, 9, 8,
};

// The neutral first component is split at original-world ML=0; the second component is right.
static const int32_t FIXTURE_FACE_SIGNED_IDS[FIXTURE_FACE_COUNT] = {
    SIGNED_ALLEN_LEFT,  SIGNED_ALLEN_LEFT,  SIGNED_ALLEN_LEFT,  SIGNED_ALLEN_LEFT,
    SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT,
    SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT, SIGNED_ALLEN_RIGHT,
};

static const vec3 PROBE_SITE_POSITIONS[PROBE_SITE_COUNT] = {
    {-0.88f, -0.18f, +0.22f},
    {-0.46f, +0.08f, -0.18f},
    {+0.42f, -0.12f, +0.30f},
    {+0.86f, +0.16f, -0.06f},
};

static const int32_t PROBE_SITE_SIGNED_IDS[PROBE_SITE_COUNT] = {
    SIGNED_ALLEN_LEFT,
    SIGNED_ALLEN_LEFT,
    SIGNED_ALLEN_RIGHT,
    SIGNED_ALLEN_RIGHT,
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the deterministic color associated with one signed atlas presentation identity.
 *
 * @param signed_id negative for left, positive for right
 * @return opaque left-cyan or right-amber color
 */
static DvzColor _signed_identity_color(int32_t signed_id)
{
    if (signed_id < 0)
        return (DvzColor){80, 210, 195, 255};
    return (DvzColor){255, 190, 90, 255};
}



/**
 * Add the synthetic atlas mesh with one flat signed-identity color per face.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return true on success
 */
static bool _add_atlas_mesh(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    vec3 positions[FIXTURE_INDEX_COUNT] = {{0}};
    vec3 normals[FIXTURE_INDEX_COUNT] = {{0}};
    DvzColor colors[FIXTURE_INDEX_COUNT] = {{0}};
    DvzIndex indices[FIXTURE_INDEX_COUNT] = {0};
    for (uint32_t i = 0; i < FIXTURE_INDEX_COUNT; i++)
    {
        const uint32_t source = FIXTURE_INDICES[i];
        // Center the original fixture bounds around the origin and scale micrometres for display.
        positions[i][0] = 0.34f * (FIXTURE_POSITIONS[source][0] - 1.5f);
        positions[i][1] = 0.34f * FIXTURE_POSITIONS[source][1];
        positions[i][2] = 0.34f * FIXTURE_POSITIONS[source][2];
        normals[i][0] = FIXTURE_NORMALS[source][0];
        normals[i][1] = FIXTURE_NORMALS[source][1];
        normals[i][2] = FIXTURE_NORMALS[source][2];
        colors[i] = _signed_identity_color(FIXTURE_FACE_SIGNED_IDS[i / 3u]);
        indices[i] = i;
    }

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = FIXTURE_INDEX_COUNT},
        {.attr_name = "normal", .data = normals, .item_count = FIXTURE_INDEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = FIXTURE_INDEX_COUNT},
    };
    if (dvz_visual_set_data_many(mesh, updates, 3) != DVZ_OK ||
        dvz_visual_set_index_data(mesh, indices, FIXTURE_INDEX_COUNT) != DVZ_OK)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.phong.ambient = 0.34f;
    material.phong.diffuse = 0.78f;
    if (dvz_visual_set_material(mesh, &material) != DVZ_OK)
        return false;
    return dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK;
}



/**
 * Add four colored probe sites whose colors are derived from their signed identities.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return true on success
 */
static bool _add_probe_sites(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzColor colors[PROBE_SITE_COUNT] = {{0}};
    float radii[PROBE_SITE_COUNT] = {0};
    for (uint32_t i = 0; i < PROBE_SITE_COUNT; i++)
    {
        colors[i] = _signed_identity_color(PROBE_SITE_SIGNED_IDS[i]);
        radii[i] = 0.105f;
    }

    DvzVisual* sites = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (sites == NULL || dvz_sphere_set_mode(sites, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != DVZ_OK)
        return false;
    DvzMaterialDesc material = example_default_standard_material_desc();
    if (dvz_visual_set_material(sites, &material) != DVZ_OK)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = PROBE_SITE_POSITIONS, .item_count = PROBE_SITE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = PROBE_SITE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PROBE_SITE_COUNT},
    };
    if (dvz_visual_set_data_many(sites, updates, 3) != DVZ_OK)
        return false;
    return dvz_panel_add_visual(panel, sites, NULL) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the atlas WebGPU portability scenario.
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

    if (example_set_default_3d_camera(panel, 1.5f) == NULL ||
        !_add_atlas_mesh(ctx->scene, panel) || !_add_probe_sites(ctx->scene, panel))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL ||
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != DVZ_OK)
        return false;
    return dvz_arcball_initial(arcball, (vec3){+0.44f, -0.24f, +0.16f}) == DVZ_OK;
}



/**
 * Return the IBL atlas WebGPU portability scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_lab_ibl_atlas_webgpu_spike_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "lab_ibl_atlas_webgpu_spike",
        .title = "IBL Atlas WebGPU Spike",
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

/**
 * Run the IBL atlas WebGPU portability scenario through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_lab_ibl_atlas_webgpu_spike_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
