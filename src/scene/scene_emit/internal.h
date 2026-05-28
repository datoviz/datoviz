/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan lowering shared internals                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_emit/scene_emit.h"
#include "upload.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_visual_has_attr_data(const DvzVisual* visual, const char* attr_name);

bool _scene_visual_needs_material_params(const DvzVisual* visual);

bool _scene_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, const char* attr_name,
    char* out_key, size_t out_size);

bool _scene_edl_params_resource_key(const char* panel_id, char* out_key, size_t out_size);

bool _scene_ssao_params_resource_key(const char* panel_id, char* out_key, size_t out_size);

bool _scene_image_uses_generated_quads(const DvzVisual* visual);

bool _scene_resource_key_volume_transfer(uint32_t visual_index, char* out, size_t out_size);
bool _scene_resource_key_volume_label_lookup(uint32_t visual_index, char* out, size_t out_size);

DvzFramePlanResourceRole _scene_attr_frame_plan_role(const char* attr_name);

uint32_t _scene_buffer_drp2_usage(uint32_t usage);

bool _scene_attach_upload_metadata(
    DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanResourceRole role, DvzFramePlanResourceKind kind, uint32_t buffer_index,
    uint64_t logical_item_count);

bool _scene_frame_plan_upload_style_bytes(
    const DvzFigure* figure, DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset,
    uint64_t byte_size, const char* data_tag, const void* data, DvzFramePlanResourceRole role);

bool _scene_visual_attrs_dirty(const DvzVisual* visual);

bool _scene_emit_visual_material_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index);

bool _scene_emit_visual_material_upload_if_needed(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    bool upload_material_params);

void _scene_emit_visual_dense_attr_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    bool upload_position_topology, bool* emitted_buffers);

void _scene_emit_visual_index_buffer_upload(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    bool* emitted_buffers);

bool _scene_emit_visual_family_derived_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    bool* out_skip_dense_attrs, bool* out_finished_visual);

void _scene_emit_visual_family_texture_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index);

void _scene_emit_visual_buffer_payloads(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    const DvzVisualUploadPayload* payloads, uint32_t payload_count, uint32_t position_topology);
