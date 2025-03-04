/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing host                                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_host.h"
#include "glfw_utils.h"
#include "host.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Host tests                                                                                   */
/*************************************************************************************************/

int test_host(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    ANN(host);
    dvz_host_create(host);

    AT(host->obj.status == DVZ_OBJECT_STATUS_CREATED);
    AT(host->gpus.count >= 1);
    AT(((DvzGpu*)(host->gpus.items[0]))->name != NULL);
    AT(((DvzGpu*)(host->gpus.items[0]))->obj.status == DVZ_OBJECT_STATUS_INIT);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_TRANSFER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_GRAPHICS | DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, 2, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    gpu = dvz_gpu_best(host);
    ANN(gpu);
    log_info("Best GPU is %s with %s VRAM", gpu->name, pretty_size(gpu->vram));
    ANN(gpu->name);

    dvz_host_destroy(host);
    return 0;
}
