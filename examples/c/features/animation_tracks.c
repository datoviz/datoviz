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
    DvzTrack* translation;
    DvzTrack* rotation;
    DvzAnimation* animation;
} AnimationTracksState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create one animated cube driven by retained scene tracks.
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
    DvzVisual* cube =
        example_graphite_cyan_cube_mesh(ctx->scene, 0.56, face_roles, &state->geometry);
    if (cube == NULL)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.standard.roughness = 0.38f;
    material.standard.specular = 0.42f;
    material.standard.rim_strength = 0.26f;
    if (dvz_visual_set_material(cube, &material) != 0)
        return false;
    if (dvz_panel_add_visual(panel, cube, NULL) != 0)
        return false;

    static const double times[] = {0.0, 1.0, 2.0, 3.0, 4.0};
    static const vec3 translations[] = {
        {-0.70f, -0.05f, 0.00f}, {-0.24f, +0.28f, 0.10f}, {+0.24f, -0.22f, 0.02f},
        {+0.70f, +0.10f, 0.14f}, {-0.70f, -0.05f, 0.00f},
    };

    DvzTrackKeyframesDesc translation_desc = dvz_track_keyframes_desc();
    translation_desc.type = DVZ_TRACK_VEC3;
    translation_desc.count = DVZ_ARRAY_COUNT(times);
    translation_desc.times = times;
    translation_desc.values = translations;
    translation_desc.repeat = DVZ_TRACK_REPEAT_LOOP;
    translation_desc.interpolation = DVZ_TRACK_INTERP_CATMULL_ROM;
    state->translation = dvz_track_keyframes(&translation_desc);
    if (state->translation == NULL)
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
    transform_desc.translation = state->translation;
    transform_desc.rotation = state->rotation;
    state->animation = dvz_anim_visual_transform(ctx->scene, cube, &transform_desc);
    if (state->animation == NULL)
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.42f, -0.18f, +0.20f});
    dvz_anim_set_interaction_policy(
        state->animation, controller, DVZ_ANIM_INTERACTION_RESUME_AFTER_IDLE, 0.8);
    dvz_anim_set_speed(state->animation, 1.0f);
    dvz_anim_start(state->animation, 0.0);
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
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.30f;
    camera.eye[2] = 1.35f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.66f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;

    return _add_animated_cube(ctx, panel, state);
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

    dvz_track_destroy(state->translation);
    dvz_track_destroy(state->rotation);
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the animation-tracks scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _animation_tracks_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_animation_tracks",
        .title = "animation_tracks",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the animation-tracks feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _animation_tracks_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
