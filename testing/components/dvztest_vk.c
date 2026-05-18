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
#include "datoviz_testing.h"
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
static int test_vklite_runtime_unavailable(TstContext* suite, const TstCase* item)
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
    dvz_testing_install_log_adapter(&_suite);
    TstSuite* suite = &_suite;

    test_vk(suite);

    const char* tags = "vklite";
    TST_MODULE(suite, tags);
    if (!_vklite_runtime_available())
    {
        TST_CASE(test_vklite_runtime_unavailable);
    }
    else
    {
        TST_CASE(test_vklite_commands_1);
        TST_CASE(test_vklite_commands_destroy_without_recording);
        TST_CASE(test_vklite_sampler_1);
        TST_CASE(test_vklite_shader_1);
        TST_CASE(test_vklite_slots_1);
        TST_CASE(test_vklite_compute_1);
        TST_CASE(test_vklite_buffers_1);
        TST_CASE(test_vklite_buffer_views);
        TST_CASE(test_vklite_images_1);
        TST_CASE(test_vklite_descriptors_1);
        TST_CASE(test_vklite_graphics_1);
        TST_CASE(test_technique_triangle);
        TST_CASE(test_technique_render_texture);
        TST_CASE(test_technique_stencil);
        TST_CASE(test_technique_msaa);
        TST_CASE(test_technique_compute_graphics);
        TST_CASE(test_technique_picking);
        TST_CASE(test_technique_wboit);
        TST_CASE(test_technique_ssao);
    }

    int res = tst_suite_run(suite, argc, argv);
    tst_suite_destroy(suite);
    return res;
}
