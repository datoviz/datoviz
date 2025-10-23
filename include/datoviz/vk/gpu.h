/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU                                                                                          */
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

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Get the detected physical GPUs.
 *
 * @param instance the instance
 * @param gpus the array of detected GPUs
 * @returns the number of returned GPUs.
 */
DVZ_EXPORT uint32_t dvz_instance_gpus(DvzInstance* instance, DvzGpu* gpus);



DVZ_EXPORT char** dvz_gpu_properties(DvzGpu* gpu, uint32_t* count);



DVZ_EXPORT void* dvz_gpu_property(DvzGpu* gpu, const char* property);



DVZ_EXPORT char** dvz_gpu_supported_extensions(DvzGpu* gpu, uint32_t* count);



DVZ_EXPORT bool dvz_gpu_has_extension(DvzGpu* gpu, const char* extension);



DVZ_EXPORT void dvz_gpu_extensions(DvzGpu* gpu, uint32_t count, const char** extensions);



DVZ_EXPORT void dvz_gpu_extension(DvzGpu* gpu, const char* extension);



DVZ_EXPORT char** dvz_gpu_supported_features(DvzGpu* gpu, uint32_t* count);



DVZ_EXPORT bool dvz_gpu_has_feature(DvzGpu* gpu, const char* feature);



DVZ_EXPORT void dvz_gpu_features(DvzGpu* gpu, uint32_t count, const char** features);



DVZ_EXPORT void dvz_gpu_feature(DvzGpu* gpu, const char* feature);



DVZ_EXPORT int dvz_gpu_device(DvzGpu* gpu, DvzDevice* device);
