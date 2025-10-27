/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing commands */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/types.h"
#include "../types.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  <Title> tests                                                                                */
/*************************************************************************************************/

int test_vklite_commands_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzDevice* device = dvz_bootstrap_device(&bootstrap);
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    DvzCommands cmds = {0};
    dvz_commands(&cmds, device, queue, 3);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_reset(&cmds, 0);
    dvz_cmd_free(&cmds);

    // Cleanup.
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
