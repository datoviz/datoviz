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

void _scene_release_visual_buffer(DvzVisual* visual);
