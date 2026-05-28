/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Primitive visual internals                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_emit/visual_lowering.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_primitive_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);
