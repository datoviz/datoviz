/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene figure emission internals                                                              */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>

#include "_scene.h"

bool _scene_figure_has_pending_render_work(const DvzFigure* figure);
