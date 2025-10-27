/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**
 * This module is mostly used to simplify test implementations, it is not meant for production
 * code.
 *
 */

/*************************************************************************************************/
/*  Bootstrap                                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBootstrap DvzBootstrap;
typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE = 0x01,
    DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE = 0x02,
    DVZ_BOOTSTRAP_MANUAL_CREATE_ALLOCATOR = 0x05,
} DvzBootstrapFlags;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Create a bootstrap, wrapping instance, GPU, device, allocator.
 *
 * @param bootstrap the bootstrap object
 * @param flags the creation flags
 */
DVZ_EXPORT void dvz_bootstrap(DvzBootstrap* bootstrap, int flags);



/**
 * Return the bootstrap's instance.
 *
 * @param bootstrap the bootstrap
 * @returns the instance
 */
DVZ_EXPORT DvzInstance* dvz_bootstrap_instance(DvzBootstrap* bootstrap);



/**
 * Return the bootstrap's gpu.
 *
 * @param bootstrap the bootstrap
 * @returns the gpu
 */
DVZ_EXPORT DvzGpu* dvz_bootstrap_gpu(DvzBootstrap* bootstrap);



/**
 * Return the bootstrap's device.
 *
 * @param bootstrap the bootstrap
 * @returns the device
 */
DVZ_EXPORT DvzDevice* dvz_bootstrap_device(DvzBootstrap* bootstrap);



/**
 * Return the bootstrap's allocator.
 *
 * @param bootstrap the bootstrap
 * @returns the allocator
 */
DVZ_EXPORT DvzVma* dvz_bootstrap_allocator(DvzBootstrap* bootstrap);



/**
 * Destroy the instance, device, allocator.
 *
 * @param bootstrap the bootstrap
 */
DVZ_EXPORT void dvz_bootstrap_destroy(DvzBootstrap* bootstrap);



EXTERN_C_OFF
