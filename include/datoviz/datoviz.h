/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/
/* Top-level convenience include. The ordinary v0.4 user path is scene/app first. Low-level
 * controller, DRP2, stream, and vklite/Vulkan-facing declarations included transitively here are
 * advanced/unstable unless their module docs say otherwise. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "controller.h"
#include "drp2.h"
#include "dvzmath.h"
#include "app.h"
#include "ffi.h"
#include "font.h"
#include "geom.h"
#include "gui.h"
#include "input.h"
#include "stream.h"
#include "scene.h"
#include "video.h"
#include "window.h"
