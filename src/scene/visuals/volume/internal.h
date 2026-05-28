/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume visual internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "scene_emit/visual_lowering.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVolumeTextureUploadPayload
{
    DvzFieldRegion region;
    const void* data;
    uint64_t byte_size;
    uint32_t texture_format;
    uint32_t bytes_per_texel;
    uint32_t allocation_width;
    uint32_t allocation_height;
    uint32_t allocation_depth;
} DvzVolumeTextureUploadPayload;



typedef struct DvzVolumeTransferTexturePayload
{
    const void* data;
    uint64_t byte_size;
    uint32_t width;
} DvzVolumeTransferTexturePayload;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _volume_uses_color_texture(const DvzVisual* visual);

bool _volume_uses_label_lookup(const DvzVisual* visual, bool* out_signed);

uint32_t _volume_transfer_texture_width(const DvzVisual* visual);

bool _volume_prepare_transfer_texture(DvzVisual* visual, const void** out_data);

bool _volume_prepare_label_lookup(DvzVisual* visual, const void** out_data, uint64_t* out_size);

bool _volume_source_texture_payload(DvzVisual* visual, DvzVolumeTextureUploadPayload* out);
bool _volume_source_texture_payload_if_dirty(
    DvzVisual* visual, DvzVolumeTextureUploadPayload* out, bool* out_handled);

bool _volume_transfer_texture_payload(DvzVisual* visual, DvzVolumeTransferTexturePayload* out);
bool _volume_transfer_texture_payload_if_needed(
    DvzVisual* visual, DvzVolumeTransferTexturePayload* out, bool* out_handled);

bool _volume_label_lookup_payload(DvzVisual* visual, const void** out_data, uint64_t* out_size);
bool _volume_label_lookup_payload_if_needed(
    DvzVisual* visual, const void** out_data, uint64_t* out_size, bool* out_handled);

bool _volume_bounds_from_state(const DvzVisual* visual, DvzBounds* out);

bool _scene_volume_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);

bool _scene_volume_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d);

bool _scene_volume_visual_fill_metadata(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata);

bool _scene_volume_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_volume_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out);

bool _scene_volume_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

bool _scene_volume_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out);
