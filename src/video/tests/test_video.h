/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing video                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/fileio.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/math/types.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests video                                                                                  */
/*************************************************************************************************/

int test_video_1(TstSuite* suite, TstItem* tstitem);



int test_video(TstSuite* suite);
