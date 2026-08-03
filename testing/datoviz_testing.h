/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz testing adapters                                                                     */
/*************************************************************************************************/

#ifndef DVZ_DATOVIZ_TESTING_HEADER
#define DVZ_DATOVIZ_TESTING_HEADER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"
#if defined(DVZ_HAS_TEST_GPU_ADAPTER) && DVZ_HAS_TEST_GPU_ADAPTER
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/gpu_ctx.h"
#endif



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_testing_install_log_adapter(TstSuite* suite);

#if defined(DVZ_HAS_TEST_GPU_ADAPTER) && DVZ_HAS_TEST_GPU_ADAPTER
void dvz_testing_install_gpu_adapter(TstSuite* suite);

uint32_t dvz_testing_gpu_index(const TstContext* ctx);

DvzGpuCtxConfig dvz_testing_gpu_ctx_config(const TstContext* ctx);

uint32_t dvz_testing_suite_gpu_index(const TstSuite* suite);

DvzGpuCtxConfig dvz_testing_suite_gpu_ctx_config(const TstSuite* suite);

bool dvz_testing_gpu_info(const TstContext* ctx, DvzGpuInfo* out_info);

bool dvz_testing_suite_gpu_info(const TstSuite* suite, DvzGpuInfo* out_info);
#endif



EXTERN_C_OFF

#endif
