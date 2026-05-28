/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume visual internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _volume_uses_color_texture(const DvzVisual* visual);

bool _volume_uses_label_lookup(const DvzVisual* visual, bool* out_signed);

uint32_t _volume_transfer_texture_width(const DvzVisual* visual);

bool _volume_prepare_transfer_texture(DvzVisual* visual, const void** out_data);

bool _volume_prepare_label_lookup(DvzVisual* visual, const void** out_data, uint64_t* out_size);
