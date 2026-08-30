/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Advanced public API                                                                          */
/*************************************************************************************************/
/* Opt-in umbrella for low-level, backend-facing, integration, and runtime APIs. Ordinary v0.4
 * applications should include <datoviz.h> and stay on the scene/app path unless they need direct
 * command streams, native runtime handles, GUI embedding, or backend integration. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas.h"
#include "controller.h"
#include "drp2.h"
#include "ffi.h"
#include "fileio.h"
#include "gui.h"
#include "app_interop.h"
#include "runner.h"
#include "shader.h"
#include "stream.h"
#include "video.h"
#include "vk.h"
#include "vklite.h"
#include "window.h"
#include "window/backend.h"

/* The raw, version-coupled cimgui declarations remain an explicit <datoviz/imgui.h> opt-in. */
