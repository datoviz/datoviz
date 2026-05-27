/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene sampled-field interpretation profiles                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "sample_profile.h"

#include "_alloc.h"
#include "_assertions.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a field format stores one unsigned integer channel.
 *
 * @param format the sampled-field format
 * @return whether the format is an unsigned scalar integer format
 */
static bool _sample_format_is_unsigned_integer(DvzFieldFormat format)
{
    return format == DVZ_FIELD_FORMAT_R8_UINT || format == DVZ_FIELD_FORMAT_R16_UINT ||
           format == DVZ_FIELD_FORMAT_R32_UINT;
}



/**
 * Return whether a field format stores one signed integer channel.
 *
 * @param format the sampled-field format
 * @return whether the format is a signed scalar integer format
 */
static bool _sample_format_is_signed_integer(DvzFieldFormat format)
{
    return format == DVZ_FIELD_FORMAT_R8_SINT || format == DVZ_FIELD_FORMAT_R16_SINT ||
           format == DVZ_FIELD_FORMAT_R32_SINT;
}



/**
 * Return whether a field format stores one scalar channel supported by current sampled visuals.
 *
 * @param format the sampled-field format
 * @return whether the format is a supported scalar format
 */
static bool _sample_format_is_scalar(DvzFieldFormat format)
{
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
    case DVZ_FIELD_FORMAT_R8_SNORM:
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
    case DVZ_FIELD_FORMAT_R16_UNORM:
    case DVZ_FIELD_FORMAT_R16_SNORM:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R16_FLOAT:
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        return true;
    default:
        return false;
    }
}



/**
 * Return whether a field format stores direct RGBA color supported by current sampled visuals.
 *
 * @param format the sampled-field format
 * @return whether the format is a supported direct RGBA format
 */
static bool _sample_format_is_rgba(DvzFieldFormat format)
{
    return format == DVZ_FIELD_FORMAT_RGBA8_UNORM;
}



/**
 * Fill a sample profile with common query support fields.
 *
 * @param out the profile to fill
 * @param format the sampled-field format
 * @param semantic the sampled-field semantic
 * @param dim the sampled-field dimension
 */
static void _sample_profile_base(
    DvzSceneSampleProfile* out, DvzFieldFormat format, DvzFieldSemantic semantic, DvzFieldDim dim)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneSampleProfile), 0, sizeof(DvzSceneSampleProfile));
    out->format = format;
    out->semantic = semantic;
    out->dim = dim;
    out->query_position_supported = true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve sampled-field storage and semantic metadata into an internal interpretation profile.
 *
 * @param format the sampled-field format
 * @param semantic the sampled-field semantic
 * @param dim the sampled-field dimension
 * @param out resolved profile
 * @return whether the combination is supported by the sampled-field interpretation layer
 */
bool _scene_sample_profile_resolve(
    DvzFieldFormat format, DvzFieldSemantic semantic, DvzFieldDim dim,
    DvzSceneSampleProfile* out)
{
    ANN(out);
    _sample_profile_base(out, format, semantic, dim);

    if (semantic == DVZ_FIELD_SEMANTIC_LABEL)
    {
        if (_sample_format_is_unsigned_integer(format))
        {
            out->value_kind = DVZ_SCENE_SAMPLE_VALUE_LABEL_U32;
            out->sampler_kind = DVZ_SCENE_SAMPLE_SAMPLER_UINT;
            out->colorizer_kind = DVZ_SCENE_COLORIZER_CATEGORICAL;
            out->filter_linear_allowed = false;
            out->query_raw_supported = true;
            return dim == DVZ_FIELD_DIM_2D || dim == DVZ_FIELD_DIM_3D;
        }
        if (_sample_format_is_signed_integer(format))
        {
            out->value_kind = DVZ_SCENE_SAMPLE_VALUE_LABEL_S32;
            out->sampler_kind = DVZ_SCENE_SAMPLE_SAMPLER_SINT;
            out->colorizer_kind = DVZ_SCENE_COLORIZER_CATEGORICAL;
            out->filter_linear_allowed = false;
            out->query_raw_supported = true;
            return dim == DVZ_FIELD_DIM_2D || dim == DVZ_FIELD_DIM_3D;
        }
        return false;
    }

    if (semantic == DVZ_FIELD_SEMANTIC_COLOR && _sample_format_is_rgba(format))
    {
        out->value_kind = DVZ_SCENE_SAMPLE_VALUE_RGBA_U8;
        out->sampler_kind = DVZ_SCENE_SAMPLE_SAMPLER_FLOAT;
        out->colorizer_kind = DVZ_SCENE_COLORIZER_DIRECT_RGBA;
        out->filter_linear_allowed = true;
        out->query_raw_supported = true;
        return dim == DVZ_FIELD_DIM_2D || dim == DVZ_FIELD_DIM_3D;
    }

    if ((semantic == DVZ_FIELD_SEMANTIC_SCALAR || semantic == DVZ_FIELD_SEMANTIC_GENERIC) &&
        _sample_format_is_scalar(format))
    {
        out->value_kind = DVZ_SCENE_SAMPLE_VALUE_SCALAR_F32;
        out->sampler_kind = _sample_format_is_signed_integer(format)
                                ? DVZ_SCENE_SAMPLE_SAMPLER_SINT
                                : (_sample_format_is_unsigned_integer(format)
                                       ? DVZ_SCENE_SAMPLE_SAMPLER_UINT
                                       : DVZ_SCENE_SAMPLE_SAMPLER_FLOAT);
        out->colorizer_kind =
            dim == DVZ_FIELD_DIM_3D ? DVZ_SCENE_COLORIZER_TRANSFER_1D
                                    : DVZ_SCENE_COLORIZER_COLORMAP_1D;
        out->filter_linear_allowed =
            !_sample_format_is_unsigned_integer(format) && !_sample_format_is_signed_integer(format);
        out->query_raw_supported = true;
        return dim == DVZ_FIELD_DIM_2D || dim == DVZ_FIELD_DIM_3D;
    }

    return false;
}



/**
 * Return whether a resolved profile represents signed or unsigned integer labels.
 *
 * @param profile the resolved sample profile
 * @return whether the profile is an integer-label profile
 */
bool _scene_sample_profile_is_integer_label(const DvzSceneSampleProfile* profile)
{
    ANN(profile);
    return profile->value_kind == DVZ_SCENE_SAMPLE_VALUE_LABEL_U32 ||
           profile->value_kind == DVZ_SCENE_SAMPLE_VALUE_LABEL_S32;
}

