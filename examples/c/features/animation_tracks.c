/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* animation_tracks - retained track-backed visual transform animation.
 *
 * Scenario: feature.animation_tracks
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/animation_tracks
 * Run:    ./build/examples/c/features/animation_tracks --live
 * Smoke:  ./build/examples/c/features/animation_tracks --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_animation_tracks_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AnimationTracksState
{
    DvzGeometry* geometry;
    DvzTrack* rotation;
    DvzTrack* camera_eye;
    DvzTrack* camera_target;
    DvzAnimation* visual_animation;
    DvzAnimation* camera_animation;
} AnimationTracksState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a mesh visual backed by one graphite-cyan colored cube geometry.
 *
 * @param scene scene owning the visual
 * @param size cube edge length
 * @param face_roles six graphite-cyan face color roles
 * @param out_geometry geometry handle for cleanup
 * @return uploaded mesh visual, or NULL on error
 */
static DvzVisual* _graphite_cyan_cube_mesh(
    DvzScene* scene, double size, const ExampleStyleColorRole face_roles[6],
    DvzGeometry** out_geometry)
{
    if (out_geometry != NULL)
        *out_geometry = NULL;
    if (scene == NULL || face_roles == NULL)
        return NULL;

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return NULL;

    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {0};
    for (uint32_t i = 0; i < DVZ_GEOM_CUBE_FACE_COUNT; i++)
        face_colors[i] = example_graphite_cyan_color(face_roles[i]);

    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = size,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return NULL;
    if (out_geometry != NULL)
        *out_geometry = cube;

    if (dvz_mesh_set_geometry(visual, cube) != 0)
        return NULL;
    return visual;
}



/**
 * Create one cube whose local transform is driven by retained scene tracks.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @param state scenario state
 * @return true on success
 */
static bool
_add_animated_cube(DvzScenarioContext* ctx, DvzPanel* panel, AnimationTracksState* state)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY, EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,           EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* cube = _graphite_cyan_cube_mesh(ctx->scene, 0.56, face_roles, &state->geometry);
    if (cube == NULL)
        return false;

    if (!example_apply_default_standard_material(cube))
        return false;
    if (dvz_panel_add_visual(panel, cube, NULL) != 0)
        return false;

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[0] = 0.35f;
    rotation_desc.axis[1] = 0.85f;
    rotation_desc.axis[2] = 0.25f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    state->rotation = dvz_track_rotation(&rotation_desc);
    if (state->rotation == NULL)
        return false;

    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = state->rotation;
    state->visual_animation = dvz_anim_visual_transform(ctx->scene, cube, &transform_desc);
    if (state->visual_animation == NULL)
        return false;

    dvz_anim_set_speed(state->visual_animation, 1.0f);
    dvz_anim_start(state->visual_animation, 0.0);
    return true;
}



/**
 * Create a keyframed camera flyover that keeps looking at the cube.
 *
 * @param ctx scenario context
 * @param camera panel-owned camera
 * @param state scenario state
 * @return true on success
 */
static bool
_add_camera_hover(DvzScenarioContext* ctx, DvzCamera* camera, AnimationTracksState* state)
{
    if (ctx == NULL || camera == NULL || state == NULL)
        return false;

    static const double times[] = {0.0, 1.2, 2.4, 3.6, 4.8};
    static const vec3 eyes[] = {
        {-2, 2, +2}, //
        {+2, 2, +2}, //
        {+2, 2, -2}, //
        {-2, 2, -2}, //
        {-2, 2, +2},
    };

    DvzTrackKeyframesDesc eye_desc = dvz_track_keyframes_desc();
    eye_desc.type = DVZ_TRACK_VEC3;
    eye_desc.count = DVZ_ARRAY_COUNT(times);
    eye_desc.times = times;
    eye_desc.values = eyes;
    eye_desc.topology = DVZ_TRACK_TOPOLOGY_CLOSED;
    eye_desc.repeat = DVZ_TRACK_REPEAT_LOOP;
    eye_desc.interpolation = DVZ_TRACK_INTERP_CATMULL_ROM;
    state->camera_eye = dvz_track_keyframes(&eye_desc);
    if (state->camera_eye == NULL)
        return false;

    DvzTrackConstantDesc target_desc = dvz_track_constant_desc();
    target_desc.type = DVZ_TRACK_VEC3;
    target_desc.value = (float[3]){0.0f, 0.0f, 0.0f};
    state->camera_target = dvz_track_constant(&target_desc);
    if (state->camera_target == NULL)
        return false;

    DvzCameraMotionDesc camera_motion = dvz_camera_motion_desc();
    camera_motion.eye = state->camera_eye;
    camera_motion.target = state->camera_target;
    camera_motion.up_mode = DVZ_CAMERA_UP_WORLD;
    camera_motion.up[0] = 0.0f;
    camera_motion.up[1] = 1.0f;
    camera_motion.up[2] = 0.0f;
    state->camera_animation = dvz_anim_camera_motion(ctx->scene, camera, &camera_motion);
    if (state->camera_animation == NULL)
        return false;

    dvz_anim_set_speed(state->camera_animation, 0.5f);
    dvz_anim_start(state->camera_animation, 0.0);
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the animation-tracks feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    *out_user = NULL;

    AnimationTracksState* state = (AnimationTracksState*)calloc(1, sizeof(AnimationTracksState));
    if (state == NULL)
        return false;
    *out_user = state;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = -2;
    camera.eye[1] = 2;
    camera.eye[2] = +2;
    camera.up[1] = 1.0f;
    camera.up[2] = 0.0f;
    camera.fov_y = 0.66f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    DvzCamera* panel_camera = dvz_panel_set_camera(panel, &camera);
    if (panel_camera == NULL)
        return false;

    if (!example_add_default_xz_reference_grid(panel, -0.32f))
        return false;
    if (!_add_animated_cube(ctx, panel, state))
        return false;
    if (!_add_camera_hover(ctx, panel_camera, state))
        return false;

    DvzController* controller = dvz_orbit_camera(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_anim_set_interaction_policy(
        state->camera_animation, controller, DVZ_ANIM_INTERACTION_PAUSE, 0);
    return true;
}



/**
 * Destroy the animation-tracks feature state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    AnimationTracksState* state = (AnimationTracksState*)user;
    if (state == NULL)
        return;

    dvz_track_destroy(state->rotation);
    dvz_track_destroy(state->camera_eye);
    dvz_track_destroy(state->camera_target);
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the animation-tracks scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_animation_tracks_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_animation_tracks",
        .title = "animation_tracks",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTROLLER,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the animation-tracks feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_animation_tracks_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
