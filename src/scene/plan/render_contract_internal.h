/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render contract internals                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "render_contract.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _draw_pass_role_matches(const DvzSceneDrawContract* draw);

void _draw_blend_target_contracts(
    DvzSceneBlendPolicy blend_policy, DvzSceneBlendTargetContract* targets,
    uint32_t* target_count);

void _draw_raster_state_contract(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    bool* out_has_raster_state, uint32_t* out_cull_mode, uint32_t* out_front_face);
