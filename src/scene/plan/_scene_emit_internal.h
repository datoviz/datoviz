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

#include "_scene_emit.h"



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
