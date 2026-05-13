/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual pipeline helpers                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_frame_plan_emit.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _is_point_visual(const ConverterState* state, const uint64_t* ids, uint32_t n);

bool _is_primitive_visual(const ConverterState* state, const uint64_t* ids, uint32_t n);

bool _is_image_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    uint64_t* out_pos, uint64_t* out_uv, uint64_t* out_tex);

bool _emitter_resolve_render_vertex_buffers(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint64_t* out_ids,
    uint32_t* out_count);

bool _scene_render_needs_depth(DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render);
