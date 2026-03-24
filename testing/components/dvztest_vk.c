/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz vk test runner                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "datoviz/vk/instance.h"
#include "../../src/vk/tests/test_vk.h"
#include "../../src/vklite/tests/test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Probe whether the current runtime can create a Vulkan instance for vklite tests.
 *
 * @return true when the runtime can create a Vulkan instance, false otherwise
 */
static bool _vklite_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("vklite tests skipped because Vulkan instance creation failed");
        return false;
    }
    dvz_instance_destroy(instance);
    return true;
}



/**
 * Fallback placeholder when vklite runtime is unavailable.
 *
 * @param suite test suite
 * @param item current test item
 * @return 0
 */
static int test_vklite_runtime_unavailable(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    log_warn("vklite tests skipped because Vulkan runtime is unavailable");
    return 0;
}



/**
 * Run vk/vklite module tests.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite _suite = tst_suite();
    TstSuite* suite = &_suite;

    test_vk(suite);

    const char* tags = "vklite";
    if (!_vklite_runtime_available())
    {
        TEST_SIMPLE(test_vklite_runtime_unavailable);
    }
    else
    {
        TEST_SIMPLE(test_vklite_commands_1);
        TEST_SIMPLE(test_vklite_commands_destroy_without_recording);
        TEST_SIMPLE(test_vklite_sampler_1);
        TEST_SIMPLE(test_vklite_shader_1);
        TEST_SIMPLE(test_vklite_slots_1);
        TEST_SIMPLE(test_vklite_compute_1);
        TEST_SIMPLE(test_vklite_buffers_1);
        TEST_SIMPLE(test_vklite_buffer_views);
        TEST_SIMPLE(test_vklite_images_1);
        TEST_SIMPLE(test_vklite_descriptors_1);
        TEST_SIMPLE(test_vklite_graphics_1);
        TEST_SIMPLE(test_technique_triangle);
        TEST_SIMPLE(test_technique_render_texture);
        TEST_SIMPLE(test_technique_stencil);
        TEST_SIMPLE(test_technique_msaa);
        TEST_SIMPLE(test_technique_compute_graphics);
        TEST_SIMPLE(test_technique_picking);
        TEST_SIMPLE(test_technique_wboit);
        TEST_SIMPLE(test_technique_ssao);
    }

    tst_suite_run(suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(suite);
    return 0;
}
