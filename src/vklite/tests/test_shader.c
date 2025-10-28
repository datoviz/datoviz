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
#include "datoviz/vklite/shader.h"
#include "test_shader.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Shader tests                                                                                */
/*************************************************************************************************/

int test_vklite_shader_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzShader shader = {0};
    dvz_shader(&bootstrap.device, sizeof(comp_shader), (uint32_t*)comp_shader, &shader);

    dvz_shader_destroy(&shader);
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
