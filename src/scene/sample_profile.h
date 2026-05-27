/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene sampled-field interpretation profiles                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene/field.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_SAMPLE_VALUE_NONE = 0,
    DVZ_SCENE_SAMPLE_VALUE_SCALAR_F32,
    DVZ_SCENE_SAMPLE_VALUE_LABEL_U32,
    DVZ_SCENE_SAMPLE_VALUE_LABEL_S32,
    DVZ_SCENE_SAMPLE_VALUE_RGBA_U8,
    DVZ_SCENE_SAMPLE_VALUE_RGBA_F32,
    DVZ_SCENE_SAMPLE_VALUE_MASK_U8,
} DvzSceneSampleValueKind;



typedef enum
{
    DVZ_SCENE_SAMPLE_SAMPLER_NONE = 0,
    DVZ_SCENE_SAMPLE_SAMPLER_FLOAT,
    DVZ_SCENE_SAMPLE_SAMPLER_UINT,
    DVZ_SCENE_SAMPLE_SAMPLER_SINT,
} DvzSceneSampleSamplerKind;



typedef enum
{
    DVZ_SCENE_COLORIZER_NONE = 0,
    DVZ_SCENE_COLORIZER_DIRECT_RGBA,
    DVZ_SCENE_COLORIZER_COLORMAP_1D,
    DVZ_SCENE_COLORIZER_TRANSFER_1D,
    DVZ_SCENE_COLORIZER_CATEGORICAL,
    DVZ_SCENE_COLORIZER_MASK,
} DvzSceneColorizerKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzSceneSampleProfile
{
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    DvzFieldDim dim;
    DvzSceneSampleValueKind value_kind;
    DvzSceneSampleSamplerKind sampler_kind;
    DvzSceneColorizerKind colorizer_kind;
    bool filter_linear_allowed;
    bool query_raw_supported;
    bool query_position_supported;
} DvzSceneSampleProfile;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_sample_profile_resolve(
    DvzFieldFormat format, DvzFieldSemantic semantic, DvzFieldDim dim,
    DvzSceneSampleProfile* out);

bool _scene_sample_profile_is_integer_label(const DvzSceneSampleProfile* profile);

bool _scene_sample_profile_is_unsigned_label(const DvzSceneSampleProfile* profile);

bool _scene_sample_profile_is_signed_label(const DvzSceneSampleProfile* profile);
