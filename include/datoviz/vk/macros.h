/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vulkan macros                                                                                */
/*************************************************************************************************/

#pragma once

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdlib.h>

#if OS_MACOS
#include <unistd.h>
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef SPIRV_DIR
#define SPIRV_DIR ""
#endif



/*************************************************************************************************/
/*  VK_DRIVER_FILES env variable for macOS MoltenVK                                              */
/*************************************************************************************************/

// macOS NOTE: if INCLUDE_VK_DRIVER_FILES is #defined, set the vulkan driver files to the path
// to the MoltenVK_icd.json file.
#ifdef INCLUDE_VK_DRIVER_FILES
__attribute__((constructor)) static void set_vk_driver_files(void)
{
#if OS_MACOS
    const char* installed = "/usr/local/lib/datoviz/MoltenVK_icd.json";
    if (access(installed, F_OK) == 0)
        setenv("VK_DRIVER_FILES", installed, 1);
#endif
}
#endif
