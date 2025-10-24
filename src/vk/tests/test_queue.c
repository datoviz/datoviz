/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing queues                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  Queue tests                                                                                  */
/*************************************************************************************************/

int test_queues_caps(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    // Get a GPU.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];
    ANN(gpu);

    // Probe the GPU queues.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);
    ANN(qc);

    for (uint32_t qf = 0; qf < qc->family_count; qf++)
    {
        VkQueueFlags flags = qc->flags[qf];
        log_info("Queue family %d (max %d) queues. Flags: 0x%x", qf, qc->queue_count[qf], flags);

        if (flags & VK_QUEUE_GRAPHICS_BIT)
            log_info("  VK_QUEUE_GRAPHICS_BIT");
        if (flags & VK_QUEUE_COMPUTE_BIT)
            log_info("  VK_QUEUE_COMPUTE_BIT");
        if (flags & VK_QUEUE_TRANSFER_BIT)
            log_info("  VK_QUEUE_TRANSFER_BIT");
        if (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
            log_info("  VK_QUEUE_VIDEO_DECODE_BIT_KHR");
        if (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
            log_info("  VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
    }

    dvz_instance_destroy(&instance);
    return 0;
}
