/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2ValidationResult DvzDrp2ValidationResult;
typedef struct DvzDrp2RuntimeConfig DvzDrp2RuntimeConfig;

struct DvzDrp2ValidationResult
{
    bool ok;
    DvzDrp2ValidationCode code;
    uint32_t command_index;
};


struct DvzDrp2RuntimeConfig
{
    DvzDevice* device;
    DvzVma* allocator;
    bool semantic_only;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a DRP2 runtime configuration for a vklite-backed runtime.
 *
 * @param device the borrowed Vulkan device wrapper
 * @param allocator the borrowed Vulkan allocator wrapper
 * @return the runtime configuration
 */
DVZ_EXPORT DvzDrp2RuntimeConfig
dvz_drp2_runtime_vklite_config(DvzDevice* device, DvzVma* allocator);



/**
 * Create a DRP2 runtime using the vklite backend boundary.
 *
 * @param cfg the runtime configuration
 * @return the runtime, or NULL on invalid configuration
 */
DVZ_EXPORT DvzDrp2Runtime* dvz_drp2_runtime_vklite(const DvzDrp2RuntimeConfig* cfg);



/**
 * Destroy a DRP2 runtime.
 *
 * @param runtime the runtime
 */
DVZ_EXPORT void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime);



/**
 * Validate a DRP2 command stream against the backend-agnostic semantic rules.
 *
 * @param stream the command stream
 * @return the validation result
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_validate_stream(const DvzDrp2CommandStream* stream);



/**
 * Execute a command stream through a DRP2 runtime skeleton.
 *
 * @param runtime the runtime
 * @param stream the command stream
 * @return the validation result before backend execution
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_runtime_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream);

EXTERN_C_OFF
