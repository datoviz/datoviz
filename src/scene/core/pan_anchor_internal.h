/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "_scene.h"

void _scene_arcball_pan_anchor_begin(
    DvzPanel* panel, DvzController* controller, const DvzPointerEvent* pointer);

bool _scene_arcball_pan_anchor_valid(const DvzPendingQueryRequest* pending);

void _scene_arcball_pan_anchor_finish(
    const DvzPendingQueryRequest* pending, const DvzQueryResult* result);

void _scene_controller_navigation_pan_changed(DvzController* controller);
