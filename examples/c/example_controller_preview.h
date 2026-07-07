/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Deterministic controller motions for generated example preview media. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArcball DvzArcball;
typedef struct DvzFly DvzFly;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzTurntable DvzTurntable;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ExamplePreviewArcballDesc
{
    vec3 base_angles;
    vec3 amplitude;
    float zoom;
} ExamplePreviewArcballDesc;


typedef struct ExamplePreviewTurntableDesc
{
    float yaw_amplitude;
    float pitch_amplitude;
    float distance_delta;
} ExamplePreviewTurntableDesc;


typedef struct ExamplePreviewPanzoomDesc
{
    vec2 base_pan;
    vec2 pan_amplitude;
    vec2 base_zoom;
    vec2 zoom_amplitude;
} ExamplePreviewPanzoomDesc;


typedef struct ExamplePreviewFlyDesc
{
    float forward_amplitude;
    float right_amplitude;
    float up_amplitude;
    float yaw_amplitude;
    float pitch_amplitude;
} ExamplePreviewFlyDesc;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void example_preview_arcball(
    DvzArcball* arcball,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewArcballDesc* desc);

void example_preview_arcball_angles(
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewArcballDesc* desc,
    vec3 out_angles);

ExamplePreviewArcballDesc example_preview_arcball_cube_desc(void);

void example_preview_turntable(
    DvzTurntable* turntable,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewTurntableDesc* desc);

ExamplePreviewTurntableDesc example_preview_turntable_cube_desc(void);

void example_preview_panzoom(
    DvzPanzoom* panzoom,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewPanzoomDesc* desc);

void example_preview_fly(
    DvzFly* fly,
    uint64_t frame_index,
    uint64_t frame_count,
    const ExamplePreviewFlyDesc* desc);

ExamplePreviewFlyDesc example_preview_fly_cube_desc(void);
