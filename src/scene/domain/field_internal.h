/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field internals                                                                        */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

typedef struct DvzSampledFieldTextureUploadPayload
{
    DvzFieldRegion region;
    const void* data;
    uint64_t byte_size;
    uint32_t texture_format;
    uint32_t bytes_per_texel;
    uint32_t allocation_width;
    uint32_t allocation_height;
    uint32_t allocation_depth;
    DvzColorRole color_role;
    bool texture_3d;
} DvzSampledFieldTextureUploadPayload;


uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field);

void _scene_field_reset(DvzSampledField* field);

bool _field_format_supported(DvzFieldFormat format);

bool _field_format_is_scalar(DvzFieldFormat format);

bool _field_format_is_rgba8(DvzFieldFormat format);

bool _field_format_bytes_per_texel(DvzFieldFormat format, uint32_t* out_bytes);

bool _field_format_texture_format(DvzFieldFormat format, uint32_t* out_format);

bool _field_expected_data_size(const DvzSampledFieldDesc* desc, uint64_t* out_size);

uint64_t _field_default_bytes_per_row(const DvzSampledFieldDesc* desc);

uint64_t _field_default_rows_per_image(const DvzSampledFieldDesc* desc);

DvzFieldRegion _field_full_region(const DvzSampledFieldDesc* desc);

bool _field_regions_union(
    const DvzFieldRegion* a, const DvzFieldRegion* b, DvzFieldRegion* out);

bool _field_region_byte_size(
    DvzFieldFormat format, const DvzFieldRegion* region, uint64_t* out_size);

bool _field_data_view_valid(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view,
    const DvzFieldRegion* region);

void _field_copy_full_data(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view, void* dst);

bool _field_read_scalar(const DvzSampledField* field, uint64_t sample_index, double* out_value);

bool _field_ensure_upload(DvzSampledField* field, uint64_t byte_size);

bool _scene_prepare_field_texture(
    DvzSampledField* field, DvzFieldRegion* out_region, const void** out_data);

bool _scene_sampled_field_texture_upload_payload(
    DvzSampledField* field, DvzSampledFieldTextureUploadPayload* out);

bool _scene_prepare_volume_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data,
    uint32_t* out_format, uint32_t* out_bytes_per_texel);

bool _scene_prepare_image_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data);

void _scene_mark_field_region_dirty(DvzSampledField* field, DvzFieldRegion region, bool full);

void _scene_visual_texture_mark_clean(DvzVisual* visual);

void _scene_visual_texture_mark_dirty(DvzVisual* visual);

void _scene_visual_texture_mark_full_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc);

void _scene_visual_texture_mark_region_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc, DvzFieldRegion region);

void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field);

void _scene_release_visual_field(DvzVisual* visual);

void _scene_release_field_bindings(DvzSampledField* field);

DvzResult _scene_visual_set_texture_rgba8(
    DvzVisual* visual, const uint8_t* rgba, uint32_t width, uint32_t height,
    DvzSize size_bytes);

DvzResult _scene_visual_set_texture_r32f(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height,
    DvzSize size_bytes);
