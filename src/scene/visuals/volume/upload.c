/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume visual upload helpers                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "volume/internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "colorizer.h"
#include "domain/field_internal.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Interpolate retained volume alpha stops at one normalized value.
 *
 * @param state volume state
 * @param t normalized transfer coordinate
 * @return alpha in [0, 1]
 */
static float _volume_alpha_at(const DvzVolumeState* state, double t)
{
    ANN(state);
    if (state->alpha_stop_count == 0)
        return (float)t;
    if (t <= state->alpha_stops[0].position)
        return state->alpha_stops[0].alpha;
    uint32_t last = state->alpha_stop_count - 1;
    if (t >= state->alpha_stops[last].position)
        return state->alpha_stops[last].alpha;
    for (uint32_t i = 1; i < state->alpha_stop_count; i++)
    {
        const DvzVolumeAlphaStop* lo = &state->alpha_stops[i - 1];
        const DvzVolumeAlphaStop* hi = &state->alpha_stops[i];
        if (t <= hi->position)
        {
            double denom = hi->position - lo->position;
            double u = denom > 0.0 ? (t - lo->position) / denom : 0.0;
            return (float)((1.0 - u) * lo->alpha + u * hi->alpha);
        }
    }
    return state->alpha_stops[last].alpha;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a volume visual needs a scalar transfer texture.
 *
 * @param visual the volume visual
 * @return whether the volume resolves to a transfer-texture profile
 */
bool _volume_uses_color_texture(const DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL)
        return false;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile))
    {
        return false;
    }
    return _scene_sample_profile_uses_transfer(&profile) ||
           _scene_sample_profile_is_integer_label(&profile);
}



/**
 * Return whether a visual needs a sparse label lookup storage buffer.
 *
 * @param visual the volume visual
 * @param out_signed output signed-key flag
 * @return whether the visual uses an integer label profile
 */
bool _volume_uses_label_lookup(const DvzVisual* visual, bool* out_signed)
{
    ANN(visual);
    if (out_signed != NULL)
        *out_signed = false;
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL)
        return false;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile))
    {
        return false;
    }
    if (!_scene_sample_profile_is_integer_label(&profile))
        return false;
    if (out_signed != NULL)
        *out_signed = _scene_sample_profile_is_signed_label(&profile);
    return true;
}



/**
 * Return the transfer texture width required by one volume visual.
 *
 * @param visual the volume visual
 * @return transfer texture width
 */
uint32_t _volume_transfer_texture_width(const DvzVisual* visual)
{
    ANN(visual);
    uint32_t width = 256;
    if (visual->field != NULL && visual->field->desc.semantic == DVZ_FIELD_SEMANTIC_LABEL)
    {
        width = 1;
        DvzSceneColorizer colorizer = {0};
        uint32_t palette_width = 0;
        if (
            _scene_colorizer_from_scale(
                visual->scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer) &&
            _scene_colorizer_dense_palette_extent(&colorizer, &palette_width) &&
            palette_width > 0)
            width = palette_width;
    }
    return width;
}



/**
 * Build the RGBA transfer texture for a scalar volume.
 *
 * @param visual the volume visual
 * @param out_data transfer texture bytes
 * @return whether transfer bytes are available
 */
bool _volume_prepare_transfer_texture(DvzVisual* visual, const void** out_data)
{
    ANN(visual);
    ANN(out_data);
    *out_data = NULL;
    if (!_volume_uses_color_texture(visual))
        return false;

    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile))
    {
        return false;
    }

    if (_scene_sample_profile_is_integer_label(&profile))
    {
        DvzSceneColorizer colorizer = {0};
        bool has_colorizer = _scene_colorizer_from_scale(
            visual->scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer);
        uint32_t palette_count = 1;
        bool has_dense_palette =
            has_colorizer && _scene_colorizer_dense_palette_extent(&colorizer, &palette_count);
        if (palette_count == 0)
            palette_count = 1;

        uint64_t size = 0;
        if (_dvz_mul_u64_overflows(palette_count, sizeof(DvzColor), &size))
        {
            log_error("label volume palette size overflow");
            return false;
        }
        if (visual->texture.rgba == NULL || visual->texture.rgba_size != size)
        {
            if (visual->texture.rgba != NULL)
                dvz_free(visual->texture.rgba);
            visual->texture.rgba = dvz_calloc(size, 1);
            if (visual->texture.rgba == NULL)
            {
                visual->texture.rgba_size = 0;
                log_error("label volume palette allocation failed");
                return false;
            }
            visual->texture.rgba_size = size;
        }

        DvzColor fallback = {48, 48, 48, 180};
        if (has_dense_palette)
        {
            if (!_scene_colorizer_build_dense_palette(
                    &colorizer, fallback, (DvzColor*)visual->texture.rgba, palette_count))
            {
                log_error("label volume categorical palette build failed");
                return false;
            }
        }
        else
        {
            ((DvzColor*)visual->texture.rgba)[0] = (DvzColor){0, 0, 0, 0};
        }
        *out_data = visual->texture.rgba;
        return true;
    }

    const uint64_t size = 256ull * 4ull;
    if (visual->texture.rgba == NULL || visual->texture.rgba_size != size)
    {
        if (visual->texture.rgba != NULL)
            dvz_free(visual->texture.rgba);
        visual->texture.rgba = dvz_calloc(size, 1);
        if (visual->texture.rgba == NULL)
        {
            visual->texture.rgba_size = 0;
            log_error("volume transfer texture allocation failed");
            return false;
        }
        visual->texture.rgba_size = size;
    }

    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    const DvzColormap* colormap =
        visual->scale != NULL && visual->scale->colormap != NULL ? visual->scale->colormap : NULL;
    for (uint32_t i = 0; i < 256; i++)
    {
        double t = (double)i / 255.0;
        if (colormap != NULL)
            _scene_color_from_colormap(colormap, t, &rgba[4 * i]);
        else
        {
            uint8_t v = (uint8_t)i;
            rgba[4 * i + 0] = v;
            rgba[4 * i + 1] = v;
            rgba[4 * i + 2] = v;
            rgba[4 * i + 3] = 255;
        }
        float alpha = _volume_alpha_at(&visual->volume, t);
        rgba[4 * i + 3] = (uint8_t)((float)rgba[4 * i + 3] * alpha + 0.5f);
    }
    *out_data = visual->texture.rgba;
    return true;
}



/**
 * Build the sparse label lookup table for a label volume.
 *
 * @param visual the volume visual
 * @param out_data output lookup bytes
 * @param out_size output byte size
 * @return whether lookup bytes are available
 */
bool _volume_prepare_label_lookup(DvzVisual* visual, const void** out_data, uint64_t* out_size)
{
    ANN(visual);
    ANN(out_data);
    ANN(out_size);
    *out_data = NULL;
    *out_size = 0;
    bool signed_keys = false;
    if (!_volume_uses_label_lookup(visual, &signed_keys))
        return false;

    DvzSceneColorizer colorizer = {0};
    (void)_scene_colorizer_from_scale(
        visual->scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer);
    uint32_t entry_count = 1;
    if (!_scene_colorizer_label_lookup_extent(&colorizer, &entry_count))
        return false;
    if (entry_count == 0)
        entry_count = 1;

    uint64_t size = 0;
    if (_dvz_mul_u64_overflows(entry_count, sizeof(DvzSceneLabelLookupEntry), &size))
    {
        log_error("label volume lookup size overflow");
        return false;
    }
    if (visual->texture.label_lookup == NULL || visual->texture.label_lookup_size != size)
    {
        if (visual->texture.label_lookup != NULL)
            dvz_free(visual->texture.label_lookup);
        visual->texture.label_lookup = dvz_calloc(size, 1);
        if (visual->texture.label_lookup == NULL)
        {
            visual->texture.label_lookup_size = 0;
            log_error("label volume lookup allocation failed");
            return false;
        }
        visual->texture.label_lookup_size = size;
    }
    if (!_scene_colorizer_build_label_lookup(
            &colorizer, signed_keys, (DvzSceneLabelLookupEntry*)visual->texture.label_lookup,
            entry_count))
    {
        log_error("label volume lookup build failed");
        return false;
    }

    *out_data = visual->texture.label_lookup;
    *out_size = size;
    return true;
}



/**
 * Prepare the source 3D texture upload payload for a volume visual.
 *
 * @param visual the volume visual
 * @param out output texture upload payload
 * @return whether the payload is available
 */
bool _volume_source_texture_payload(DvzVisual* visual, DvzVolumeTextureUploadPayload* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(
        out, sizeof(DvzVolumeTextureUploadPayload), 0, sizeof(DvzVolumeTextureUploadPayload));
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL)
        return false;

    if (!_scene_prepare_volume_texture(
            visual, &out->region, &out->data, &out->texture_format, &out->bytes_per_texel))
    {
        return false;
    }

    DvzFieldFormat byte_format = out->texture_format == VK_FORMAT_R8G8B8A8_UNORM
                                     ? DVZ_FIELD_FORMAT_RGBA8_UNORM
                                     : visual->field->desc.format;
    if (!_field_region_byte_size(byte_format, &out->region, &out->byte_size))
        return false;
    out->allocation_width = visual->field->desc.width;
    out->allocation_height = visual->field->desc.height;
    out->allocation_depth = visual->field->desc.depth;
    return true;
}



/**
 * Prepare a dirty source 3D texture upload payload for a volume visual.
 *
 * @param visual the volume visual
 * @param out output texture upload payload
 * @param out_handled whether the visual is handled by volume source texture logic
 * @return whether the payload decision succeeded
 */
bool _volume_source_texture_payload_if_dirty(
    DvzVisual* visual, DvzVolumeTextureUploadPayload* out, bool* out_handled)
{
    ANN(visual);
    ANN(out);
    ANN(out_handled);
    dvz_memset(
        out, sizeof(DvzVolumeTextureUploadPayload), 0, sizeof(DvzVolumeTextureUploadPayload));
    *out_handled = visual->type == DVZ_VISUAL_TYPE_VOLUME;
    if (!*out_handled)
        return true;
    if (visual->field == NULL || (!visual->texture.dirty && !visual->field->dirty))
        return true;
    return _volume_source_texture_payload(visual, out);
}



/**
 * Prepare the scalar transfer texture upload payload for a volume visual.
 *
 * @param visual the volume visual
 * @param out output transfer texture upload payload
 * @return whether the payload is available
 */
bool _volume_transfer_texture_payload(DvzVisual* visual, DvzVolumeTransferTexturePayload* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(
        out, sizeof(DvzVolumeTransferTexturePayload), 0,
        sizeof(DvzVolumeTransferTexturePayload));
    if (!_volume_uses_color_texture(visual))
        return false;
    out->width = _volume_transfer_texture_width(visual);
    out->byte_size = (uint64_t)out->width * 4ull;
    return _volume_prepare_transfer_texture(visual, &out->data);
}



/**
 * Prepare the scalar transfer texture upload payload when one is needed.
 *
 * @param visual the volume visual
 * @param out output transfer texture upload payload
 * @param out_handled whether the visual needs a transfer texture
 * @return whether the payload decision succeeded
 */
bool _volume_transfer_texture_payload_if_needed(
    DvzVisual* visual, DvzVolumeTransferTexturePayload* out, bool* out_handled)
{
    ANN(visual);
    ANN(out);
    ANN(out_handled);
    dvz_memset(
        out, sizeof(DvzVolumeTransferTexturePayload), 0,
        sizeof(DvzVolumeTransferTexturePayload));
    *out_handled = _volume_uses_color_texture(visual);
    if (!*out_handled)
        return true;
    return _volume_transfer_texture_payload(visual, out);
}



/**
 * Prepare the sparse label lookup upload payload for a volume visual.
 *
 * @param visual the volume visual
 * @param out_data output lookup bytes
 * @param out_size output byte size
 * @return whether lookup bytes are available
 */
bool _volume_label_lookup_payload(DvzVisual* visual, const void** out_data, uint64_t* out_size)
{
    ANN(visual);
    ANN(out_data);
    ANN(out_size);
    if (!_volume_uses_label_lookup(visual, NULL))
    {
        *out_data = NULL;
        *out_size = 0;
        return false;
    }
    return _volume_prepare_label_lookup(visual, out_data, out_size);
}



/**
 * Prepare the sparse label lookup upload payload when one is needed.
 *
 * @param visual the volume visual
 * @param out_data output lookup bytes
 * @param out_size output byte size
 * @param out_handled whether the visual needs a sparse label lookup buffer
 * @return whether the payload decision succeeded
 */
bool _volume_label_lookup_payload_if_needed(
    DvzVisual* visual, const void** out_data, uint64_t* out_size, bool* out_handled)
{
    ANN(visual);
    ANN(out_data);
    ANN(out_size);
    ANN(out_handled);
    *out_data = NULL;
    *out_size = 0;
    *out_handled = _volume_uses_label_lookup(visual, NULL);
    if (!*out_handled)
        return true;
    return _volume_label_lookup_payload(visual, out_data, out_size);
}
