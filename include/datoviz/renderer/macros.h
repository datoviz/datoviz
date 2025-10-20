/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Renderer macros                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define IF_VERBOSE if (getenv("DVZ_VERBOSE") && (strncmp(getenv("DVZ_VERBOSE"), "req", 3) == 0))
