/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Backend.
typedef enum
{
    DVZ_BACKEND_NONE,
    DVZ_BACKEND_GLFW,
    DVZ_BACKEND_QT,
    DVZ_BACKEND_OFFSCREEN,
    DVZ_BACKEND_WRAP,
} DvzBackend;



// App creation flags.
typedef enum
{
    DVZ_APP_FLAGS_NONE = 0x000000,
    DVZ_APP_FLAGS_OFFSCREEN = 0x008000, // INTERNAL: also passed as CanvasFlags in visual_test.h

    // NOTE: must match DVZ_RENDERER_FLAGS_WHITE_BACKGROUND
    DVZ_APP_FLAGS_EXTERNAL = 0x080000,
    DVZ_APP_FLAGS_WHITE_BACKGROUND = 0x100000,

} DvzAppFlags;
