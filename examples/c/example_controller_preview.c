/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Deterministic controller motions for generated example preview media. */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_controller_preview.h"

#include "datoviz/controller/arcball.h"
#include "datoviz/controller/fly.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/controller/turntable.h"

#include <math.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EXAMPLE_PREVIEW_TAU 6.28318530718f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static float _preview_phase(uint64_t frame_index, uint64_t frame_count)
{
    if (frame_count <= 1)
        return 0.0f;
    return (float)(frame_index % frame_count) / (float)frame_count;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void example_preview_arcball(
    DvzArcball* arcball,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewArcballDesc* desc)
{
    if (arcball == NULL || desc == NULL)
        return;

    vec3 angles = {0};
    example_preview_arcball_angles(frame_index, frame_count, desc, angles);
    (void)dvz_arcball_set(arcball, angles);
    if (desc->zoom > 0.0f)
        (void)dvz_arcball_zoom(arcball, desc->zoom);
}


void example_preview_arcball_angles(
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewArcballDesc* desc,
    vec3 out_angles)
{
    if (desc == NULL || out_angles == NULL)
        return;

    const float theta = EXAMPLE_PREVIEW_TAU * _preview_phase(frame_index, frame_count);
    out_angles[0] = desc->base_angles[0] + desc->amplitude[0] * sinf(theta);
    out_angles[1] = desc->base_angles[1] + desc->amplitude[1] * cosf(theta);
    out_angles[2] = desc->base_angles[2] + desc->amplitude[2] * sinf(theta + 0.72f);
}


void example_preview_turntable(
    DvzTurntable* turntable,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewTurntableDesc* desc)
{
    if (turntable == NULL || desc == NULL)
        return;

    const float phase = _preview_phase(frame_index, frame_count);
    const float theta = EXAMPLE_PREVIEW_TAU * phase;
    (void)dvz_turntable_reset(turntable);
    (void)dvz_turntable_orbit(
        turntable, desc->yaw_amplitude * sinf(theta), desc->pitch_amplitude * cosf(theta));
    if (desc->distance_delta != 0.0f)
        (void)dvz_turntable_dolly(turntable, desc->distance_delta * (0.5f - 0.5f * cosf(theta)));
    (void)dvz_turntable_apply_camera(turntable);
}


void example_preview_panzoom(
    DvzPanzoom* panzoom,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewPanzoomDesc* desc)
{
    if (panzoom == NULL || desc == NULL)
        return;

    const float theta = EXAMPLE_PREVIEW_TAU * _preview_phase(frame_index, frame_count);
    const float zoom_phase = 0.5f - 0.5f * cosf(theta);
    vec2 pan = {
        desc->base_pan[0] + desc->pan_amplitude[0] * sinf(theta),
        desc->base_pan[1] + desc->pan_amplitude[1] * cosf(theta),
    };
    vec2 zoom = {
        desc->base_zoom[0] + desc->zoom_amplitude[0] * zoom_phase,
        desc->base_zoom[1] + desc->zoom_amplitude[1] * zoom_phase,
    };
    (void)dvz_panzoom_pan(panzoom, pan);
    (void)dvz_panzoom_zoom(panzoom, zoom);
}


void example_preview_fly(
    DvzFly* fly,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewFlyDesc* desc)
{
    if (fly == NULL || desc == NULL)
        return;

    const float theta = EXAMPLE_PREVIEW_TAU * _preview_phase(frame_index, frame_count);
    (void)dvz_fly_reset(fly);
    (void)dvz_fly_rotate(
        fly, desc->yaw_amplitude * sinf(theta + 0.45f), desc->pitch_amplitude * cosf(theta));
    (void)dvz_fly_move_forward(fly, desc->forward_amplitude * sinf(theta));
    (void)dvz_fly_move_right(fly, desc->right_amplitude * cosf(theta));
    (void)dvz_fly_move_up(fly, desc->up_amplitude * sinf(theta + 1.20f));
    (void)dvz_fly_apply_camera(fly);
}
