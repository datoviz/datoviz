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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if OS_MACOS
#include <libgen.h>
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
    char file_path[1024] = {0};
    strncpy(file_path, __FILE__, sizeof(file_path));

    char path[1024] = {0};
    const char* bundled = "/../libs/vulkan/macos/MoltenVK_icd.json"; // Relative to this header
                                                                     // inside the source tree.
    const char* installed = "/usr/local/lib/datoviz/MoltenVK_icd.json";

    char root_path[1024] = {0};
    snprintf(
        root_path, sizeof(root_path), "%s",
        dirname(
            file_path)); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

    char bundled_json[1024] = {0};
    snprintf(bundled_json, sizeof(bundled_json), "%s%s", root_path, bundled);

    if (access(bundled_json, F_OK) == 0)
    {
        snprintf(path, sizeof(path), "%s:%s", bundled_json, installed);
    }
    else
    {
        snprintf(path, sizeof(path), "%s", installed);
    }
    setenv("VK_DRIVER_FILES", path, 1);
// log_error("Setting VK_DRIVER_FILES to %s", path);
#endif
}
#endif
