/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene common bind group helpers                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_common_bindings_resolve_panel_sets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t* out_bgl_id, uint64_t* out_apply_bg_id, uint64_t* out_fixed_bg_id,
    uint64_t* out_isotropic_bg_id);

bool _scene_common_bindings_resolve_visual_set(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    DvzSceneWorkProviderKey provider, uint32_t visual_index, uint64_t common_bgl_id,
    DvzFramePlanViewportRect viewport_rect, uint64_t* out_bg_id);

bool _scene_common_bindings_resolve_single_set(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t* out_bgl_id, uint64_t* out_bg_id);
