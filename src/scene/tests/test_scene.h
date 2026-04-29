/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_capabilities_diagnostics(TstSuite* suite, TstItem* item);

int test_frame_plan_static_render(TstSuite* suite, TstItem* item);

int test_frame_plan_dynamic_update(TstSuite* suite, TstItem* item);

int test_frame_plan_readbacks(TstSuite* suite, TstItem* item);

int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item);



int test_scene(TstSuite* suite);
