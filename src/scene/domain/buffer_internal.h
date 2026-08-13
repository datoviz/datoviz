/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene buffer internals                                                                       */
/*************************************************************************************************/

#pragma once

#include "_scene.h"

uint32_t _scene_buffer_index(const DvzScene* scene, const DvzSceneBuffer* buffer);

void _scene_buffer_reset(DvzSceneBuffer* buffer);

DvzResult _scene_buffer_prepare_data(
    const DvzSceneBuffer* buffer, const void* data, uint64_t byte_size, void** out_data,
    uint64_t* out_capacity);

void _scene_buffer_commit_data(
    DvzSceneBuffer* buffer, void* data, uint64_t byte_size, uint64_t capacity);

void _scene_release_visual_buffer(DvzVisual* visual);
