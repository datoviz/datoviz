/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene format state internals                                                                 */
/*************************************************************************************************/

#pragma once

#include "_scene.h"

void _scene_format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src);
