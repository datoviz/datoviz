/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App internals                                                                                */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/common/macros.h"

EXTERN_C_ON

bool _dvz_view_scheduler_should_render(DvzView* view, bool continuous, uint64_t now);
bool _dvz_app_has_continuous_work(DvzApp* app);

EXTERN_C_OFF
