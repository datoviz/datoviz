/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual binding internals                                                               */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>

#include "_scene.h"

const DvzVisualBinding* _visual_binding_const(
    const DvzVisual* visual, DvzVisualBindingKind kind);

void _visual_binding_assign(
    DvzVisual* visual, DvzVisualBindingKind kind, const char* slot_name, void* resource,
    bool owned);

void _visual_binding_clear(DvzVisual* visual, DvzVisualBindingKind kind);
