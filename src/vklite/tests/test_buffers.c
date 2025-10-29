/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing buffers                                                                              */
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
#include "datoviz/vk/enums.h"
#include "datoviz/vklite/buffers.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Shader tests                                                                                */
/*************************************************************************************************/

int test_vklite_buffers_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzBuffer buffer = {0};
    DvzSize size = 65536;

    dvz_buffer(&bootstrap.device, &bootstrap.allocator, &buffer);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_create(&buffer);
    dvz_buffer_destroy(&buffer);

    // Cleanup.
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
