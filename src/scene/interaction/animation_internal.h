/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene animation internals                                                                    */
/*************************************************************************************************/

#pragma once

#include <stdint.h>

#include "_scene.h"

void _dvz_scene_animations_step(DvzScene* scene, uint64_t wall_time_ns);
void _dvz_scene_animations_step_external(DvzScene* scene, double t, double dt);
