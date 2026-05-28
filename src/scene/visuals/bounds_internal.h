/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual bounds internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const DvzVisualAttr*
_bounds_attr(const DvzVisual* visual, const char* attr_name, uint32_t item_size);
void _bounds_reset(DvzBounds* out);
void _bounds_include_point(DvzBounds* out, double x, double y, double z);
void _bounds_include_vec3f(DvzBounds* out, const float* data, uint64_t item_count);

bool _scene_visual_default_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d);
bool _sphere_bounds_from_radius(const DvzVisual* visual, DvzBounds* out);
void _sphere_bounds_expand_wire_overlay(const DvzVisual* visual, DvzBounds* bounds);
bool _image_bounds_from_extent(const DvzVisual* visual, DvzBounds* out);
bool _glyph_bounds_from_rect(const DvzVisual* visual, DvzBounds* out);
