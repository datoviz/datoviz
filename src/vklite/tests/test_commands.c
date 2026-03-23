/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing commands                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/sync.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  vklite tests                                                                                 */
/*************************************************************************************************/

int test_vklite_commands_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    DvzCommands cmds = {0};
    dvz_commands(device, queue, 3, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_end(&cmds);
    dvz_cmd_reset(&cmds);
    dvz_cmd_free(&cmds);

    // Cleanup.
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_barriers_reset(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);

    // Fill to capacity.
    for (uint32_t i = 0; i < DVZ_MAX_BARRIERS; i++)
    {
        DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)(i + 1));
        AT(bimg != NULL);
    }
    AT(barriers.info.imageMemoryBarrierCount == DVZ_MAX_BARRIERS);
    AT_EXPECTED_ERROR_STRICT(
        suite, (dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0xFFFF) == NULL));

    // Reset and ensure counters restart from zero.
    dvz_barriers(&barriers);
    AT(barriers.info.imageMemoryBarrierCount == 0);
    AT(barriers.info.bufferMemoryBarrierCount == 0);
    AT(barriers.info.memoryBarrierCount == 0);

    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0x1234);
    AT(bimg != NULL);
    AT(barriers.info.imageMemoryBarrierCount == 1);

    return 0;
}
