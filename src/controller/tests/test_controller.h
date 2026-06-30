/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing controller                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_controller_panzoom_create(TstContext* suite, const TstCase* item);

int test_controller_panzoom_keep_aspect_drag(TstContext* suite, const TstCase* item);

int test_controller_arcball_create(TstContext* suite, const TstCase* item);

int test_controller_camera_create(TstContext* suite, const TstCase* item);

int test_controller_camera_orthographic_bounds(TstContext* suite, const TstCase* item);

int test_controller_fly_create(TstContext* suite, const TstCase* item);

int test_controller_fly_z_up_lookat_drag(TstContext* suite, const TstCase* item);

int test_controller_turntable_create(TstContext* suite, const TstCase* item);

int test_controller_desc_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item);



int test_controller(TstSuite* suite);
