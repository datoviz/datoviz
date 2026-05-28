/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan capabilities                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "_alloc.h"
#include "_assertions.h"
#include "frame_plan/frame_plan.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a capability snapshot.
 *
 * @param snapshot the capability snapshot
 */
void dvz_capability_snapshot_default(DvzCapabilitySnapshot* snapshot)
{
    ANN(snapshot);
    dvz_memset(snapshot, sizeof(DvzCapabilitySnapshot), 0, sizeof(DvzCapabilitySnapshot));
    snapshot->max_buffer_size = 256 * 1024 * 1024;
    snapshot->max_texture_dimension_2d = 4096;
    snapshot->max_bind_groups = 4;
    snapshot->max_vertex_buffers = 8;
    snapshot->max_color_attachments = 1;
    snapshot->max_color_sample_count = 16;
    snapshot->max_depth_sample_count = 16;
    snapshot->shader_format_wgsl = true;
    snapshot->shader_format_glsl = true;
    snapshot->render_target_format_rgba16float = false;
    snapshot->render_target_format_r16float = false;
    snapshot->supports_render_target_sampling = false;
    snapshot->supports_color_blending = false;
    snapshot->supports_readback = true;
    snapshot->min_texture_copy_bytes_per_row_alignment = 4;
    snapshot->max_readback_size = snapshot->max_buffer_size;
    snapshot->texture_format_r32uint = true;
    snapshot->texture_format_rg32uint = true;
    snapshot->render_target_format_r32uint = true;
    snapshot->render_target_format_rg32uint = true;
    snapshot->query_profile_u32_r32 = true;
    snapshot->query_profile_u64_rg32 = true;
    snapshot->query_profile_u64_2xr32 = true;
}



/**
 * Copy a capability snapshot.
 *
 * @param dst the destination snapshot
 * @param src the source snapshot
 */
void dvz_capability_snapshot_copy(DvzCapabilitySnapshot* dst, const DvzCapabilitySnapshot* src)
{
    ANN(dst);
    ANN(src);
    dvz_memcpy(dst, sizeof(DvzCapabilitySnapshot), src, sizeof(DvzCapabilitySnapshot));
}
