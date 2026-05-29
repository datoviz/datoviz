/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene notification internals                                                                 */
/*************************************************************************************************/

#pragma once

#include "_scene.h"

bool _scene_add_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data);

void _scene_remove_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data);

void _scene_notify_request_frame(DvzFigure* figure);

void _scene_notify_visual_changed(DvzVisual* visual);

void _scene_notify_buffer_changed(DvzSceneBuffer* buffer);
