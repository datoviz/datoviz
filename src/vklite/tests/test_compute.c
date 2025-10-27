/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing sampler                                                                              */
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
#include "datoviz/vklite/compute.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Compute tests                                                                                */
/*************************************************************************************************/

int test_vklite_compute_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzCompute compute = {0};
    dvz_compute(&bootstrap.device, &compute);
    // dvz_compute_layout(compute,  layout);
    // dvz_compute_shader(compute, shader);
    AT(dvz_compute_create(&compute) == 0);

    // Cleanup.
    dvz_compute_destroy(&compute);
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
