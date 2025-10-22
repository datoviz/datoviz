/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing device                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "testing.h"



/*************************************************************************************************/
/*  <Title> tests                                                                                */
/*************************************************************************************************/

int test_device_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    log_info("Testing dvz_instance_supported_layers()...");

    // Call the function under test.
    uint32_t count = 0;
    char** layers = dvz_instance_supported_layers(&count);
    log_info("Found %u supported Vulkan layers:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, layers[i]);
    }

    // Free.
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_free(layers[i]);
    }
    dvz_free(layers);

    log_info("Test completed successfully.");

    return 0;
}
