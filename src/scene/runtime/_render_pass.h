/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render-pass helpers                                                                    */
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
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _render_pass_resolve_color_target(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_id);

bool _render_pass_resolve_readback_buffer(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* copy,
    uint64_t* out_id);

void _render_pass_next_ids(
    DvzFramePlanEmitter* emitter, uint64_t* encoder_id, uint64_t* render_pass_id,
    uint64_t* command_buffer_id, uint64_t* submission_id);

bool _render_pass_copy_finish_submit(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t command_buffer_id,
    uint64_t submission_id, uint64_t color_id, uint64_t readback_buffer_id,
    const DvzFramePlanNode* copy);
