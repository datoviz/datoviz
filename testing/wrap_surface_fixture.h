/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Wrap surface test helpers                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_env.h"
#include "datoviz/window.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a wrap-ready window configuration for tests.
 *
 * @param title window title string
 * @param width logical window width
 * @param height logical window height
 * @return initialized window config
 */
static inline DvzWindowConfig
dvz_test_wrap_window_config(const char* title, uint32_t width, uint32_t height)
{
    DvzWindowConfig cfg = dvz_window_config();
    cfg.title = title;
    cfg.width = width;
    cfg.height = height;
    cfg.visible = checkenv("DVZ_TEST_VISIBLE");
    return cfg;
}



/**
 * Build a DvzWindowExternalSurfaceInfo record for wrap-backend tests.
 *
 * @param instance Vulkan instance handle associated with the surface
 * @param surface Vulkan surface handle
 * @param width surface extent width
 * @param height surface extent height
 * @param scale_x content scale on X
 * @param scale_y content scale on Y
 * @param owned_by_datoviz whether Datoviz owns the surface destruction
 * @return initialized external-surface record
 */
static inline DvzWindowExternalSurfaceInfo dvz_test_wrap_surface_info(
    VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height, float scale_x,
    float scale_y, bool owned_by_datoviz)
{
    DvzWindowExternalSurfaceInfo info = {
        .instance = instance,
        .surface = surface,
        .extent = {.width = width, .height = height},
        .scale_x = scale_x,
        .scale_y = scale_y,
        .owned_by_datoviz = owned_by_datoviz,
    };
    return info;
}
