/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal Vulkan loader                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <volk.h>



/*************************************************************************************************/
/*  Internal loader API                                                                          */
/*************************************************************************************************/

/**
 * Initialize Datoviz's Vulkan loader and return its instance-procedure entry point.
 *
 * @return the active Vulkan loader entry point, or NULL when initialization failed
 */
PFN_vkGetInstanceProcAddr _dvz_vulkan_loader(void);
