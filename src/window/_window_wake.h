/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "datoviz/window.h"

EXTERN_C_ON

void _dvz_window_host_wake(DvzWindowHost* host);
void _dvz_window_glfw_wake(void);

EXTERN_C_OFF
