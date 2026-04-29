/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_capabilities_diagnostics(TstSuite* suite, TstItem* item);

int test_frame_plan_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_dynamic_update(TstSuite* suite, TstItem* item);

int test_frame_plan_readbacks(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render_glsl(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(
    TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_rejects_small_caps(TstSuite* suite, TstItem* item);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_frame_plan_emit_drp2_static_render_glsl_executes(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_readback_glsl_executes(TstSuite* suite, TstItem* item);
#endif

int test_frame_plan_emit_drp2_readback(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_dynamic_uploads(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_texture_sampling(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_compute_assisted(TstSuite* suite, TstItem* item);



int test_scene(TstSuite* suite);
