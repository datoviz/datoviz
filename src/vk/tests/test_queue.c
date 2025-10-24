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
#include "_assertions.h"
#include "_log.h"
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



int test_queues_basic(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueueCaps qc = {0};
    qc.family_count = 1;
    qc.flags[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    qc.queue_count[0] = 1;

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    ASSERT(queues.queue_count == 1);
    ASSERT(queues.queues[DVZ_QUEUE_MAIN].is_main);
    ASSERT(dvz_queue_supports(&queues.queues[DVZ_QUEUE_MAIN], DVZ_QUEUE_TRANSFER));

    return 0;
}



int test_queues_multiple(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueueCaps qc = {0};
    qc.family_count = 3;
    qc.flags[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; // main
    qc.flags[1] = VK_QUEUE_COMPUTE_BIT;                         // async compute
    qc.flags[2] = VK_QUEUE_TRANSFER_BIT;                        // async transfer

    for (uint32_t i = 0; i < 3; i++)
        qc.queue_count[i] = 1;

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    ASSERT(queues.queue_count == 3);

    ASSERT(queues.queues[DVZ_QUEUE_MAIN].family_idx == 0);
    ASSERT(queues.queues[DVZ_QUEUE_COMPUTE].family_idx == 1);
    ASSERT(queues.queues[DVZ_QUEUE_TRANSFER].family_idx == 2);

    dvz_queues_show(&queues);

    return 0;
}



int test_queues_tie_break(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueueCaps qc = {0};
    qc.family_count = 3;
    qc.flags[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; // main
    qc.flags[1] = VK_QUEUE_COMPUTE_BIT;                         // specialized
    qc.flags[2] = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT; // more generic

    for (uint32_t i = 0; i < 3; i++)
        qc.queue_count[i] = 1;

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    ASSERT(queues.queues[DVZ_QUEUE_COMPUTE].family_idx == 1);

    return 0;
}



int test_queues_no_optional(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueueCaps qc = {0};
    qc.family_count = 1;
    qc.flags[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    qc.queue_count[0] = 1;

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    ASSERT(queues.queue_count == 1);
    ASSERT(queues.queues[DVZ_QUEUE_MAIN].is_main);
    ASSERT(!queues.queues[DVZ_QUEUE_COMPUTE].is_set);
    ASSERT(!queues.queues[DVZ_QUEUE_TRANSFER].is_set);

    return 0;
}



int test_queues_video_roles(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueueCaps qc = {0};
    qc.family_count = 4;
    qc.flags[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; // main
    qc.flags[1] = VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    qc.flags[2] = VK_QUEUE_VIDEO_DECODE_BIT_KHR;
    qc.flags[3] = VK_QUEUE_TRANSFER_BIT;

    for (uint32_t i = 0; i < qc.family_count; i++)
        qc.queue_count[i] = 1;

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    ASSERT(queues.queues[DVZ_QUEUE_VIDEO_ENCODE].is_set);
    ASSERT(queues.queues[DVZ_QUEUE_VIDEO_DECODE].is_set);
    ASSERT(queues.queues[DVZ_QUEUE_TRANSFER].is_set);
    ASSERT(queues.queue_count == 4);

    return 0;
}



int test_queue_from_role(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueues queues = {0};

    // Main queue: graphics+compute.
    queues.queues[DVZ_QUEUE_MAIN] = (DvzQueue){
        .family_idx = 0,
        .flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
        .is_main = true,
        .is_set = true,
    };
    queues.queue_count = 1;

    // Add an async compute queue.
    queues.queues[DVZ_QUEUE_COMPUTE] = (DvzQueue){
        .family_idx = 1,
        .flags = VK_QUEUE_COMPUTE_BIT,
        .is_set = true,
    };
    queues.queue_count++;

    // Test lookups.
    DvzQueue* q = NULL;

    q = dvz_queue_from_role(&queues, DVZ_QUEUE_MAIN);
    ASSERT(q != NULL && q->is_main);

    q = dvz_queue_from_role(&queues, DVZ_QUEUE_COMPUTE);
    ASSERT(q != NULL && q->family_idx == 1);

    q = dvz_queue_from_role(&queues, DVZ_QUEUE_TRANSFER);
    ASSERT(q != NULL && q->family_idx == 1);

    // Should fail for encode/decode
    q = dvz_queue_from_role(&queues, DVZ_QUEUE_VIDEO_ENCODE);
    ASSERT(q == NULL || !q->is_set);

    return 0;
}



int test_queue_supports(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzQueue q = {0};

    // ------------------------------------------------------------------
    // Graphics + Compute queue (main-like)
    // ------------------------------------------------------------------
    q.flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_MAIN));
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_COMPUTE));
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_TRANSFER)); // implicit per spec
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_ENCODE));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_DECODE));

    // ------------------------------------------------------------------
    // Transfer-only queue
    // ------------------------------------------------------------------
    q.flags = VK_QUEUE_TRANSFER_BIT;
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_MAIN));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_COMPUTE));
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_TRANSFER));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_ENCODE));

    // ------------------------------------------------------------------
    // Compute-only queue
    // ------------------------------------------------------------------
    q.flags = VK_QUEUE_COMPUTE_BIT;
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_MAIN));
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_COMPUTE));
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_TRANSFER)); // implicitly supports transfer
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_DECODE));

    // ------------------------------------------------------------------
    // Video encode
    // ------------------------------------------------------------------
    q.flags = VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_ENCODE));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_DECODE));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_MAIN));

    // ------------------------------------------------------------------
    // Video decode
    // ------------------------------------------------------------------
    q.flags = VK_QUEUE_VIDEO_DECODE_BIT_KHR;
    ASSERT(dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_DECODE));
    ASSERT(!dvz_queue_supports(&q, DVZ_QUEUE_VIDEO_ENCODE));

    return 0;
}
