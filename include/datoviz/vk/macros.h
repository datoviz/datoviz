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

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BINDINGS        16
#define DVZ_MAX_VERTEX_ATTRS    16
#define DVZ_MAX_VERTEX_BINDINGS 8
#define DVZ_MAX_PARAMS          16

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
    char file_path[1024] = {0};
    strncpy(file_path, __FILE__, sizeof(file_path));

    char path[1024] = {0};
    snprintf(
        path, 1024, "%s%s", dirname(file_path), // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        "/../libs/vulkan/macos/MoltenVK_icd.json:/usr/local/lib/datoviz/MoltenVK_icd.json");
    setenv("VK_DRIVER_FILES", path, 1);
// log_error("Setting VK_DRIVER_FILES to %s", path);
#endif
}
#endif
