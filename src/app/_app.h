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

bool _dvz_view_scheduler_should_render(DvzView* view, uint64_t now);
bool _dvz_view_has_continuous_work(DvzView* view);
bool _dvz_app_has_continuous_work(DvzApp* app);

void _dvz_view_test_force_draw_failure(DvzView* view, bool force);

EXTERN_C_OFF
