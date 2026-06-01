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
#include "_log.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CAPABILITY_SNAPSHOT_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a capability snapshot has a valid ABI prologue.
 *
 * @param snapshot the capability snapshot
 * @return whether the snapshot is accepted
 */
bool dvz_capability_snapshot_valid(const DvzCapabilitySnapshot* snapshot)
{
    if (!DVZ_STRUCT_VALID(
            snapshot, DvzCapabilitySnapshot, DVZ_CAPABILITY_SNAPSHOT_KNOWN_FLAGS))
    {
        log_error("invalid DvzCapabilitySnapshot ABI prologue");
        return false;
    }
    return true;
}



/**
 * Return the default capability snapshot.
 *
 * @return default capability snapshot
 */
DvzCapabilitySnapshot dvz_capability_snapshot(void)
{
    DvzCapabilitySnapshot snapshot = {
        DVZ_STRUCT_INIT_FIELDS(DvzCapabilitySnapshot),
        .max_buffer_size = 256 * 1024 * 1024,
        .max_texture_dimension_2d = 4096,
        .max_bind_groups = 4,
        .max_vertex_buffers = 8,
        .max_color_attachments = 1,
        .max_color_sample_count = 16,
        .max_depth_sample_count = 16,
        .shader_format_wgsl = true,
        .shader_format_glsl = true,
        .supports_readback = true,
        .min_texture_copy_bytes_per_row_alignment = 4,
        .texture_format_r32uint = true,
        .texture_format_rg32uint = true,
        .render_target_format_r32uint = true,
        .render_target_format_rg32uint = true,
        .query_profile_u32_r32 = true,
        .query_profile_u64_rg32 = true,
        .query_profile_u64_2xr32 = true,
    };
    snapshot.max_readback_size = snapshot.max_buffer_size;
    return snapshot;
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
    if (!dvz_capability_snapshot_valid(dst) || !dvz_capability_snapshot_valid(src))
        return;
    dvz_memcpy(dst, sizeof(DvzCapabilitySnapshot), src, sizeof(DvzCapabilitySnapshot));
}
