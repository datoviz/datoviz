/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene resource key helpers                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_resource_key_visual(uint32_t visual_index, char* out, size_t out_size);

bool _scene_resource_key_buffer(DvzId buffer_id, char* out, size_t out_size);

bool _scene_resource_key_visual_data(
    const char* visual_id, const char* data_tag, char* out, size_t out_size);

bool _scene_resource_key_visual_attr(
    uint32_t visual_index, const char* attr_name, char* out, size_t out_size);

bool _scene_resource_key_visual_texture(uint32_t visual_index, char* out, size_t out_size);

bool _scene_resource_key_visual_indexed(
    uint32_t visual_index, DvzId buffer_id, char* out, size_t out_size);

bool _scene_visual_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size);

bool _scene_visual_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    const char* attr_name, char* out, size_t out_size);

bool _scene_visual_texture_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size);

bool _scene_visual_indexed_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzId buffer_id, char* out, size_t out_size);

bool _scene_resource_key_panel_graph(
    const char* panel_id, const char* suffix, char* out, size_t out_size);

bool _scene_resource_key_panel_lights(const char* panel_id, char* out, size_t out_size);

void _scene_resource_key_split_visual(
    const char* encoded, char* visual_id, size_t visual_id_size, char* index_id,
    size_t index_id_size);

bool _scene_resource_id_has_suffix(const char* resource_id, const char* suffix);

bool _scene_resource_id_has_depth_marker(const char* resource_id);
