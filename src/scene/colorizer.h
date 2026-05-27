/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene colorizers                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/types.h"
#include "datoviz/scene/types.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzSceneColorizer
{
    DvzSceneColorizerKind kind;
    const DvzScale* scale;
} DvzSceneColorizer;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_colorizer_from_scale(
    const DvzScale* scale, DvzSceneColorizerKind kind, DvzSceneColorizer* out);

bool _scene_colorizer_category_color(
    const DvzSceneColorizer* colorizer, DvzCategoryId id, DvzColor* out_color);

bool _scene_colorizer_category_label(
    const DvzSceneColorizer* colorizer, DvzCategoryId id, char* out_label, uint64_t label_size);

bool _scene_colorizer_dense_palette_extent(
    const DvzSceneColorizer* colorizer, uint32_t* out_count);

bool _scene_colorizer_build_dense_palette(
    const DvzSceneColorizer* colorizer, DvzColor fallback, DvzColor* out_palette,
    uint32_t palette_count);

