/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* wboit — transparent mesh via scene WBOIT + app/GLFW.
 *
 * Opens a GLFW window showing one transparent WBOIT cube between opaque reference cards. A GUI
 * overlay exposes live color, alpha, ambient/diffuse, and light-direction sliders. The visual
 * alpha-mode opt-in exercises scene planning, DRP2 WBOIT accumulation/resolve, and the vklite
 * runtime while retaining the normal app/canvas presentation path.
 *
 * Build:  just example-c techniques/wboit
 * Run:    ./build/examples/c/techniques/wboit
 * Smoke:  ./build/examples/c/techniques/wboit 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_gui_controls.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600

#define ROTATION_SPEED_RAD_PER_SEC 0.65f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshWboitState MeshWboitState;

struct MeshWboitState
{
    DvzPanel* panel;
    DvzVisual* cube;
    uint32_t cube_vertex_count;
    DvzColor cube_colors[24];
    float cube_rgb[3];
    float cube_alpha;
    float cube_light_direction[3];
    float cube_ambient;
    float cube_diffuse;
    bool light_background;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an indexed cube with duplicated vertices, per-face normals, and one color.
 *
 * @param scale cube half-extent
 * @param color cube vertex color
 * @param positions output vertex positions
 * @param colors output vertex colors
 * @param normals output vertex normals
 * @param indices output triangle-list indices
 */
static void _build_cube(
    float scale, DvzColor color, vec3 positions[24], DvzColor colors[24],
    vec3 normals[24], DvzIndex indices[36])
{
    const float s = scale;
    const vec3 face_positions[6][4] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const vec3 face_normals[6] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            colors[vertex] = color;
            normals[vertex][0] = face_normals[face][0];
            normals[vertex][1] = face_normals[face][1];
            normals[vertex][2] = face_normals[face][2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}



/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8_from_unit(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Apply the GUI-controlled diagnostic background.
 *
 * @param state mesh WBOIT example state
 */
static void _mesh_wboit_update_background(MeshWboitState* state)
{
    ANN(state);
    if (state->panel == NULL)
        return;

    if (state->light_background)
        dvz_panel_set_background_color(state->panel, dvz_color_from_unit(0.96f, 0.97f, 0.98f, 1.0f));
    else
        dvz_panel_set_background_color(state->panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));
}



/**
 * Upload the GUI-controlled WBOIT cube material.
 *
 * @param state mesh WBOIT example state
 */
static void _mesh_wboit_update_cube(MeshWboitState* state)
{
    ANN(state);
    if (state->cube == NULL || state->cube_vertex_count == 0)
        return;

    DvzColor color = dvz_color_rgba(
        _u8_from_unit(state->cube_rgb[0]), _u8_from_unit(state->cube_rgb[1]),
        _u8_from_unit(state->cube_rgb[2]), _u8_from_unit(state->cube_alpha));
    for (uint32_t i = 0; i < state->cube_vertex_count; i++)
        state->cube_colors[i] = color;

    dvz_visual_set_data(state->cube, "color", state->cube_colors, state->cube_vertex_count);
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.alpha_mode = dvz_visual_alpha_mode(state->cube);
    material.light_direction[0] = state->cube_light_direction[0];
    material.light_direction[1] = state->cube_light_direction[1];
    material.light_direction[2] = state->cube_light_direction[2];
    material.phong.ambient = state->cube_ambient;
    material.phong.diffuse = state->cube_diffuse;
    dvz_visual_set_material(state->cube, &material);
}



/**
 * Build the live WBOIT material controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data mesh WBOIT example state
 */
static void _mesh_wboit_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    MeshWboitState* state = (MeshWboitState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool background_changed = false;
    if (dvz_gui_begin(gui, "WBOIT cube", NULL, 0))
    {
        background_changed |=
            dvz_gui_checkbox(gui, "Light background", &state->light_background);

        float cube_rgba[4] = {
            state->cube_rgb[0],
            state->cube_rgb[1],
            state->cube_rgb[2],
            state->cube_alpha,
        };
        changed |= dvz_gui_color_edit4(gui, "Cube color", cube_rgba, 0);
        state->cube_rgb[0] = cube_rgba[0];
        state->cube_rgb[1] = cube_rgba[1];
        state->cube_rgb[2] = cube_rgba[2];
        state->cube_alpha = cube_rgba[3];

        dvz_gui_separator_text(gui, "Lighting");
        changed |= dvz_gui_slider_float(gui, "Ambient", &state->cube_ambient, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Diffuse", &state->cube_diffuse, 0.0f, 1.5f);
        changed |= example_gui_vec3(
            gui, "Light direction", state->cube_light_direction, -1.0f, 1.0f, "%.2f");
        if (dvz_gui_button(gui, "Reset"))
        {
            state->light_background = false;
            state->cube_rgb[0] = 36.0f / 255.0f;
            state->cube_rgb[1] = 150.0f / 255.0f;
            state->cube_rgb[2] = 185.0f / 255.0f;
            state->cube_alpha = 82.0f / 255.0f;
            state->cube_light_direction[0] = 0.25f;
            state->cube_light_direction[1] = 0.70f;
            state->cube_light_direction[2] = 0.45f;
            state->cube_ambient = 0.18f;
            state->cube_diffuse = 0.95f;
            changed = true;
            background_changed = true;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _mesh_wboit_update_cube(state);
    if (background_changed)
        _mesh_wboit_update_background(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count(argc, argv);
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzExampleVisualSpin spin = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 3.2f;
    camera_desc.view.up[1] = 1.0f;
    camera_desc.projection.fov_y = 0.78539816339f;
    camera_desc.projection.near_clip = 0.1f;
    camera_desc.projection.far_clip = 100.0f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");

    DvzVisual* reference = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* cube = dvz_mesh(scene, 0);
    EXAMPLE_CHECK(reference != NULL && cube != NULL, "visual creation failed");

    vec3 reference_positions[12] = {
        {-0.95f, -0.95f, -1.05f},
        {+0.95f, -0.95f, -1.05f},
        {+0.95f, +0.95f, -1.05f},
        {-0.95f, -0.95f, -1.05f},
        {+0.95f, +0.95f, -1.05f},
        {-0.95f, +0.95f, -1.05f},
        {-0.16f, -0.86f, +1.05f},
        {+0.16f, -0.86f, +1.05f},
        {+0.16f, +0.86f, +1.05f},
        {-0.16f, -0.86f, +1.05f},
        {+0.16f, +0.86f, +1.05f},
        {-0.16f, +0.86f, +1.05f},
    };
    DvzColor reference_colors[12] = {
        {255, 230, 80, 255},
        {255, 230, 80, 255},
        {255, 80, 180, 255},
        {255, 230, 80, 255},
        {255, 80, 180, 255},
        {80, 200, 255, 255},
        {32, 32, 32, 255},
        {32, 32, 32, 255},
        {120, 255, 150, 255},
        {32, 32, 32, 255},
        {120, 255, 150, 255},
        {32, 32, 32, 255},
    };

    vec3 positions[24] = {0};
    DvzColor colors[24] = {0};
    vec3 normals[24] = {0};
    DvzIndex indices[36] = {0};
    DvzColor cube_color = {36, 150, 185, 82};
    _build_cube(0.72f, cube_color, positions, colors, normals, indices);

    MeshWboitState state = {
        .panel = panel,
        .cube = cube,
        .cube_vertex_count = 24,
        .cube_rgb = {36.0f / 255.0f, 150.0f / 255.0f, 185.0f / 255.0f},
        .cube_alpha = 82.0f / 255.0f,
        .cube_light_direction = {0.25f, 0.70f, 0.45f},
        .cube_ambient = 0.18f,
        .cube_diffuse = 0.95f,
        .light_background = false,
    };
    dvz_memcpy(state.cube_colors, sizeof(state.cube_colors), colors, sizeof(colors));

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    EXAMPLE_CHECK(index_buffer != NULL, "dvz_scene_buffer() failed");

    ok = dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_scene_buffer_set_data() failed");

    DvzVisualDataUpdate reference_updates[] = {
        {.attr_name = "position", .data = reference_positions, .item_count = 12},
        {.attr_name = "color", .data = reference_colors, .item_count = 12},
    };
    int rc = dvz_visual_set_data_many(reference, reference_updates, 2);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for reference");

    DvzVisualDataUpdate cube_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 24},
        {.attr_name = "normal", .data = normals, .item_count = 24},
    };
    rc = dvz_visual_set_data_many(cube, cube_updates, 2);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for cube");
    ok = dvz_visual_set_buffer(cube, "index", index_buffer) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_visual_set_buffer() failed");
    _mesh_wboit_update_cube(&state);
    dvz_visual_set_alpha_mode(cube, DVZ_ALPHA_WBOIT);

    rc = dvz_panel_add_visual(panel, reference, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for reference");
    rc = dvz_panel_add_visual(panel, cube, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for cube");
    _mesh_wboit_update_background(&state);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "wboit");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.65f, 0.0f, +0.35f});

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _mesh_wboit_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    spin.rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(spin.rotation != NULL, "dvz_track_rotation() failed");
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = spin.rotation;
    spin.animation = dvz_anim_visual_transform(scene, cube, &transform_desc);
    EXAMPLE_CHECK(spin.animation != NULL, "dvz_anim_visual_transform() failed");
    dvz_anim_set_speed(spin.animation, ROTATION_SPEED_RAD_PER_SEC);
    dvz_anim_start(spin.animation, 0.0);

    dvz_app_run(app, frame_count);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_track_destroy(spin.rotation);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
