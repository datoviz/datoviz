/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic WASM scene ABI                                                                       */
/*************************************************************************************************/

/* The WASM ABI implementation is split across scene_api_*.c.
 * Shared state and helpers live in scene_api_internal.h. */

#include "scene_api_internal.h"
