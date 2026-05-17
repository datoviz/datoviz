/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* ibl_brain - BWM cluster cloud inside a transparent Allen brain shell.
 *
 * Prepare: python examples/c/showcase/prepare_ibl_bwm.py
 * Build:   just example-c showcase/ibl_brain
 * Run:     ./build/examples/c/showcase/ibl_brain
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1000u
#define HEIGHT 760u

#define BWM_DATA_DIR "build/local_data/ibl_bwm"

#define BWM_DEFAULT_POINT_SIZE 5.0f
#define BWM_DEFAULT_MESH_ALPHA (32.0f / 255.0f)
#define BWM_ROTATION_SPEED_RAD_PER_SEC 0.35f



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum BwmTechnique
{
    BWM_TECHNIQUE_DEPTH_PEEL = 0,
    BWM_TECHNIQUE_WBOIT,
    BWM_TECHNIQUE_SOURCE_OVER,
} BwmTechnique;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct BwmDataset
{
    float (*cluster_pos)[3];
    DvzColor* cluster_color;
    float* cluster_size;
    uint32_t cluster_count;

    float (*mesh_pos)[3];
    float (*mesh_normal)[3];
    DvzColor* mesh_color;
    DvzIndex* mesh_idx;
    uint32_t mesh_vertex_count;
    uint32_t mesh_index_count;
} BwmDataset;



typedef struct BwmExampleState
{
    DvzVisual* point;
    DvzVisual* mesh;
    DvzAnimation* spin;
    BwmDataset* dataset;

    BwmTechnique technique;
    bool show_points;
    bool show_shell;
    bool shell_depth_test;
    bool spin_enabled;
    float point_size;
    float mesh_alpha;
    float ambient;
    float diffuse;
    float light_direction[3];
} BwmExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Join a directory and basename into a fixed-size path buffer.
 *
 * @param dir directory path
 * @param basename filename
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit in the output buffer
 */
static bool _join_path(const char* dir, const char* basename, char* out, size_t out_size)
{
    ANN(dir);
    ANN(basename);
    ANN(out);

    int written = dvz_snprintf(out, out_size, "%s/%s", dir, basename);
    return written > 0 && (size_t)written < out_size;
}



/**
 * Read a required NPY payload from the BWM local data directory.
 *
 * @param data_dir directory containing prepared BWM arrays
 * @param basename array filename
 * @param size output payload size in bytes
 * @return payload pointer owned by the caller, or NULL on failure
 */
static void* _read_bwm_npy(const char* data_dir, const char* basename, DvzSize* size)
{
    ANN(data_dir);
    ANN(basename);
    ANN(size);

    char path[1024] = {0};
    if (!_join_path(data_dir, basename, path, sizeof(path)))
    {
        dvz_fprintf(stderr, "BWM data path is too long\n");
        return NULL;
    }
    return dvz_read_npy(path, size);
}



/**
 * Print the BWM local-data preparation instructions.
 *
 * @param data_dir expected data directory
 */
static void _print_prepare_hint(const char* data_dir)
{
    dvz_fprintf(
        stderr,
        "missing local IBL BWM .npy data in %s\n"
        "prepare it with: python examples/c/showcase/prepare_ibl_bwm.py\n",
        data_dir != NULL ? data_dir : BWM_DATA_DIR);
}



/**
 * Load the prepared BWM cluster and mesh arrays.
 *
 * @param data_dir directory containing prepared BWM arrays
 * @param dataset output dataset
 * @return whether loading and validation succeeded
 */
static bool _load_bwm_dataset(const char* data_dir, BwmDataset* dataset)
{
    ANN(data_dir);
    ANN(dataset);

    DvzSize cluster_pos_size = 0;
    DvzSize cluster_color_size = 0;
    DvzSize cluster_size_size = 0;
    DvzSize mesh_pos_size = 0;
    DvzSize mesh_normal_size = 0;
    DvzSize mesh_color_size = 0;
    DvzSize mesh_idx_size = 0;

    float(*cluster_pos)[3] = _read_bwm_npy(data_dir, "bwm_cluster_pos.npy", &cluster_pos_size);
    DvzColor* cluster_color = _read_bwm_npy(data_dir, "bwm_cluster_color.npy", &cluster_color_size);
    float* cluster_size = _read_bwm_npy(data_dir, "bwm_cluster_size.npy", &cluster_size_size);
    float(*mesh_pos)[3] = _read_bwm_npy(data_dir, "bwm_mesh_pos.npy", &mesh_pos_size);
    float(*mesh_normal)[3] = _read_bwm_npy(data_dir, "bwm_mesh_normal.npy", &mesh_normal_size);
    DvzColor* mesh_color = _read_bwm_npy(data_dir, "bwm_mesh_color.npy", &mesh_color_size);
    DvzIndex* mesh_idx = _read_bwm_npy(data_dir, "bwm_mesh_idx.npy", &mesh_idx_size);

    if (cluster_pos == NULL || cluster_color == NULL || cluster_size == NULL || mesh_pos == NULL ||
        mesh_normal == NULL || mesh_color == NULL || mesh_idx == NULL)
    {
        dvz_free(mesh_idx);
        dvz_free(mesh_color);
        dvz_free(mesh_normal);
        dvz_free(mesh_pos);
        dvz_free(cluster_size);
        dvz_free(cluster_color);
        dvz_free(cluster_pos);
        _print_prepare_hint(data_dir);
        return false;
    }

    if (cluster_pos_size == 0 || cluster_pos_size % (3 * sizeof(float)) != 0 ||
        cluster_color_size % sizeof(DvzColor) != 0 || cluster_size_size % sizeof(float) != 0 ||
        mesh_pos_size == 0 || mesh_pos_size % (3 * sizeof(float)) != 0 ||
        mesh_normal_size % (3 * sizeof(float)) != 0 ||
        mesh_color_size % sizeof(DvzColor) != 0 || mesh_idx_size % sizeof(DvzIndex) != 0)
    {
        dvz_fprintf(stderr, "invalid IBL BWM .npy payload sizes\n");
        goto error;
    }

    DvzSize cluster_count = cluster_pos_size / (3 * sizeof(float));
    DvzSize mesh_vertex_count = mesh_pos_size / (3 * sizeof(float));
    DvzSize mesh_index_count = mesh_idx_size / sizeof(DvzIndex);
    if (cluster_count == 0 || cluster_count > UINT32_MAX ||
        cluster_color_size / sizeof(DvzColor) != cluster_count ||
        cluster_size_size / sizeof(float) != cluster_count || mesh_vertex_count == 0 ||
        mesh_vertex_count > UINT32_MAX || mesh_normal_size / (3 * sizeof(float)) != mesh_vertex_count ||
        mesh_color_size / sizeof(DvzColor) != mesh_vertex_count || mesh_index_count == 0 ||
        mesh_index_count > UINT32_MAX || mesh_index_count % 3 != 0)
    {
        dvz_fprintf(stderr, "inconsistent IBL BWM array sizes\n");
        goto error;
    }

    DvzIndex* indices = mesh_idx;
    for (DvzSize i = 0; i < mesh_index_count; i++)
    {
        if (indices[i] >= mesh_vertex_count)
        {
            dvz_fprintf(stderr, "IBL BWM mesh index out of range\n");
            goto error;
        }
    }

    dataset->cluster_pos = cluster_pos;
    dataset->cluster_color = cluster_color;
    dataset->cluster_size = cluster_size;
    dataset->cluster_count = (uint32_t)cluster_count;
    dataset->mesh_pos = mesh_pos;
    dataset->mesh_normal = mesh_normal;
    dataset->mesh_color = mesh_color;
    dataset->mesh_idx = mesh_idx;
    dataset->mesh_vertex_count = (uint32_t)mesh_vertex_count;
    dataset->mesh_index_count = (uint32_t)mesh_index_count;

    dvz_fprintf(
        stderr, "loaded IBL BWM data: %u clusters, %u mesh vertices, %u triangles\n",
        dataset->cluster_count, dataset->mesh_vertex_count, dataset->mesh_index_count / 3);
    return true;

error:
    dvz_free(mesh_idx);
    dvz_free(mesh_color);
    dvz_free(mesh_normal);
    dvz_free(mesh_pos);
    dvz_free(cluster_size);
    dvz_free(cluster_color);
    dvz_free(cluster_pos);
    return false;
}



/**
 * Release CPU-side BWM buffers.
 *
 * @param dataset dataset to release
 */
static void _destroy_bwm_dataset(BwmDataset* dataset)
{
    if (dataset == NULL)
        return;
    dvz_free(dataset->mesh_idx);
    dvz_free(dataset->mesh_color);
    dvz_free(dataset->mesh_normal);
    dvz_free(dataset->mesh_pos);
    dvz_free(dataset->cluster_size);
    dvz_free(dataset->cluster_color);
    dvz_free(dataset->cluster_pos);
    dvz_memset(dataset, sizeof(BwmDataset), 0, sizeof(BwmDataset));
}



/**
 * Return the alpha mode used by a GUI-selected shell transparency technique.
 *
 * @param technique selected technique
 * @return visual alpha mode
 */
static DvzAlphaMode _alpha_mode(BwmTechnique technique)
{
    switch (technique)
    {
    case BWM_TECHNIQUE_WBOIT:
        return DVZ_ALPHA_WBOIT;
    case BWM_TECHNIQUE_SOURCE_OVER:
        return DVZ_ALPHA_BLENDED;
    case BWM_TECHNIQUE_DEPTH_PEEL:
    default:
        return DVZ_ALPHA_DEPTH_PEEL;
    }
}



/**
 * Return a display label for a shell transparency technique.
 *
 * @param technique selected technique
 * @return static display label
 */
static const char* _technique_label(BwmTechnique technique)
{
    switch (technique)
    {
    case BWM_TECHNIQUE_WBOIT:
        return "WBOIT";
    case BWM_TECHNIQUE_SOURCE_OVER:
        return "Source-over";
    case BWM_TECHNIQUE_DEPTH_PEEL:
    default:
        return "Depth peel";
    }
}



/**
 * Return the next shell transparency technique.
 *
 * @param technique current technique
 * @return next technique
 */
static BwmTechnique _next_technique(BwmTechnique technique)
{
    switch (technique)
    {
    case BWM_TECHNIQUE_DEPTH_PEEL:
        return BWM_TECHNIQUE_WBOIT;
    case BWM_TECHNIQUE_WBOIT:
        return BWM_TECHNIQUE_SOURCE_OVER;
    case BWM_TECHNIQUE_SOURCE_OVER:
    default:
        return BWM_TECHNIQUE_DEPTH_PEEL;
    }
}



/**
 * Convert a normalized alpha value to an 8-bit color channel.
 *
 * @param value normalized alpha value
 * @return clamped 8-bit alpha channel
 */
static uint8_t _alpha_u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Upload the GUI-controlled point sizes.
 *
 * @param state example state
 */
static void _apply_point_size(BwmExampleState* state)
{
    ANN(state);
    ANN(state->dataset);
    ANN(state->point);

    for (uint32_t i = 0; i < state->dataset->cluster_count; i++)
        state->dataset->cluster_size[i] = state->point_size;
    if (dvz_visual_set_data(
            state->point, "diameter", state->dataset->cluster_size,
            state->dataset->cluster_count) != 0)
        dvz_fprintf(stderr, "failed to update BWM point size\n");
}



/**
 * Upload the GUI-controlled shell alpha and lighting parameters.
 *
 * @param state example state
 */
static void _apply_shell_material(BwmExampleState* state)
{
    ANN(state);
    ANN(state->dataset);
    ANN(state->mesh);

    uint8_t alpha = _alpha_u8(state->mesh_alpha);
    for (uint32_t i = 0; i < state->dataset->mesh_vertex_count; i++)
        state->dataset->mesh_color[i][3] = alpha;

    if (dvz_visual_set_data(
            state->mesh, "color", state->dataset->mesh_color,
            state->dataset->mesh_vertex_count) != 0)
        dvz_fprintf(stderr, "failed to update BWM shell color\n");

    if (dvz_visual_set_primitive_shading(
            state->mesh,
            &(DvzPrimitiveShadingDesc){
                .light_direction = {
                    state->light_direction[0],
                    state->light_direction[1],
                    state->light_direction[2],
                },
                .ambient = state->ambient,
                .diffuse = state->diffuse,
            }) != 0)
        dvz_fprintf(stderr, "failed to update BWM shell material\n");
}



/**
 * Apply the selected shell transparency technique.
 *
 * @param state example state
 */
static void _apply_technique(BwmExampleState* state)
{
    ANN(state);
    ANN(state->mesh);
    if (dvz_visual_set_alpha_mode(state->mesh, _alpha_mode(state->technique)) != 0)
        dvz_fprintf(stderr, "failed to update BWM shell transparency technique\n");
}



/**
 * Apply the retained visibility controls.
 *
 * @param state example state
 */
static void _apply_visibility(BwmExampleState* state)
{
    ANN(state);
    if (state->point != NULL)
        dvz_visual_set_visible(state->point, state->show_points);
    if (state->mesh != NULL)
        dvz_visual_set_visible(state->mesh, state->show_shell);
}



/**
 * Apply the retained shell depth-test control.
 *
 * @param state example state
 */
static void _apply_shell_depth_test(BwmExampleState* state)
{
    ANN(state);
    if (state->mesh == NULL)
        return;
    if (dvz_visual_set_depth_test(state->mesh, state->shell_depth_test) != 0)
        dvz_fprintf(stderr, "failed to update BWM shell depth test\n");
}



/**
 * Apply the retained spin control.
 *
 * @param state example state
 */
static void _apply_spin(BwmExampleState* state)
{
    ANN(state);
    if (state->spin == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin, 0.0);
    else
        dvz_anim_stop(state->spin);
}



/**
 * Reset the GUI-controlled visual parameters.
 *
 * @param state example state
 */
static void _reset_controls(BwmExampleState* state)
{
    ANN(state);
    state->technique = BWM_TECHNIQUE_WBOIT;
    state->show_points = true;
    state->show_shell = true;
    state->shell_depth_test = true;
    state->spin_enabled = false;
    state->point_size = BWM_DEFAULT_POINT_SIZE;
    state->mesh_alpha = BWM_DEFAULT_MESH_ALPHA;
    state->ambient = 0.20f;
    state->diffuse = 0.95f;
    state->light_direction[0] = 0.20f;
    state->light_direction[1] = 0.70f;
    state->light_direction[2] = 0.45f;
    _apply_point_size(state);
    _apply_shell_material(state);
    _apply_technique(state);
    _apply_visibility(state);
    _apply_shell_depth_test(state);
    _apply_spin(state);
}



/**
 * Build the live BWM shell transparency controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _bwm_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    BwmExampleState* state = (BwmExampleState*)user_data;
    if (state == NULL)
        return;

    bool visibility_changed = false;
    bool depth_test_changed = false;
    bool technique_changed = false;
    bool point_changed = false;
    bool material_changed = false;
    bool spin_changed = false;
    if (dvz_gui_begin(gui, "IBL BWM brain", NULL, 0))
    {
        char technique_label[64] = {0};
        dvz_snprintf(
            technique_label, sizeof(technique_label), "Technique: %s",
            _technique_label(state->technique));
        if (dvz_gui_button(gui, technique_label))
        {
            state->technique = _next_technique(state->technique);
            technique_changed = true;
        }
        visibility_changed |= dvz_gui_checkbox(gui, "Show clusters", &state->show_points);
        visibility_changed |= dvz_gui_checkbox(gui, "Show shell", &state->show_shell);
        depth_test_changed |=
            dvz_gui_checkbox(gui, "Shell depth-tests clusters", &state->shell_depth_test);
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        point_changed |=
            dvz_gui_slider_float(gui, "Cluster size", &state->point_size, 1.0f, 12.0f);
        material_changed |=
            dvz_gui_slider_float(gui, "Shell alpha", &state->mesh_alpha, 0.02f, 0.50f);
        material_changed |= dvz_gui_slider_float(gui, "Ambient", &state->ambient, 0.0f, 1.0f);
        material_changed |= dvz_gui_slider_float(gui, "Diffuse", &state->diffuse, 0.0f, 1.5f);
        material_changed |=
            dvz_gui_slider_float(gui, "Light X", &state->light_direction[0], -1.0f, 1.0f);
        material_changed |=
            dvz_gui_slider_float(gui, "Light Y", &state->light_direction[1], -1.0f, 1.0f);
        material_changed |=
            dvz_gui_slider_float(gui, "Light Z", &state->light_direction[2], -1.0f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
            _reset_controls(state);
    }
    dvz_gui_end(gui);

    if (visibility_changed)
        _apply_visibility(state);
    if (depth_test_changed)
        _apply_shell_depth_test(state);
    if (technique_changed)
        _apply_technique(state);
    if (point_changed)
        _apply_point_size(state);
    if (material_changed)
        _apply_shell_material(state);
    if (spin_changed)
        _apply_spin(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the interactive IBL BWM transparent brain-shell example.
 *
 * @return process status
 */
int main(void)
{
    BwmDataset dataset = {0};
    if (!_load_bwm_dataset(BWM_DATA_DIR, &dataset))
        return 1;

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        _destroy_bwm_dataset(&dataset);
        return 1;
    }
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    dvz_scene_set_capabilities(scene, &caps);

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (figure == NULL || panel == NULL)
    {
        dvz_fprintf(stderr, "scene setup failed\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.97f, 0.97f, 0.95f, 1.0f);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = 0.0f;
    camera_desc.eye[2] = 4.1f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.15f;
    camera_desc.target[2] = 0.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.01f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (point == NULL || mesh == NULL)
    {
        dvz_fprintf(stderr, "visual creation failed\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL ||
        !dvz_scene_buffer_set_data(
            index_buffer, dataset.mesh_idx, dataset.mesh_index_count * sizeof(DvzIndex)))
    {
        dvz_fprintf(stderr, "index buffer setup failed\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    BwmExampleState state = {
        .point = point,
        .mesh = mesh,
        .dataset = &dataset,
        .technique = BWM_TECHNIQUE_WBOIT,
        .show_points = true,
        .show_shell = true,
        .shell_depth_test = true,
        .spin_enabled = false,
        .point_size = BWM_DEFAULT_POINT_SIZE,
        .mesh_alpha = BWM_DEFAULT_MESH_ALPHA,
        .ambient = 0.20f,
        .diffuse = 0.95f,
        .light_direction = {0.20f, 0.70f, 0.45f},
    };

    if (dvz_visual_set_data(point, "position", dataset.cluster_pos, dataset.cluster_count) != 0 ||
        dvz_visual_set_data(point, "color", dataset.cluster_color, dataset.cluster_count) != 0 ||
        dvz_visual_set_data(point, "diameter", dataset.cluster_size, dataset.cluster_count) != 0 ||
        dvz_visual_set_data(mesh, "position", dataset.mesh_pos, dataset.mesh_vertex_count) != 0 ||
        dvz_visual_set_data(mesh, "normal", dataset.mesh_normal, dataset.mesh_vertex_count) != 0 ||
        !dvz_visual_set_buffer(mesh, "index", index_buffer))
    {
        dvz_fprintf(stderr, "visual data upload failed\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    _apply_shell_material(&state);
    _apply_technique(&state);
    _apply_shell_depth_test(&state);

    dvz_panel_add_visual(panel, point, NULL);
    dvz_panel_add_visual(panel, mesh, NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "ibl_brain");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.70f, 0.0f, +0.20f});

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _bwm_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    state.spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, BWM_ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (state.spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_bwm_dataset(&dataset);
        return 1;
    }
    _apply_spin(&state);

    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _destroy_bwm_dataset(&dataset);
    return 0;
}
