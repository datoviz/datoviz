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
 * Skip vklite tests when the current runtime cannot create a Vulkan instance.
 *
 * @param suite test context
 * @param item current test case
 * @return NULL when Vulkan is available, otherwise a skip reason
 */
static const char* _vklite_skip_unless_runtime(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        return "requires Vulkan runtime";
    }
    dvz_instance_destroy(instance);
    return NULL;
}



#define TST_VKLITE_CASE(test)                                                                     \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_GPU | TST_RES_VULKAN;                                       \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        _tst_desc.skip = _vklite_skip_unless_runtime;                                             \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)



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
    TST_VKLITE_CASE(test_vklite_commands_1);
    TST_VKLITE_CASE(test_vklite_commands_destroy_without_recording);
    TST_VKLITE_CASE(test_vklite_sampler_1);
    TST_VKLITE_CASE(test_vklite_shader_1);
    TST_VKLITE_CASE(test_vklite_slots_1);
    TST_VKLITE_CASE(test_vklite_compute_1);
    TST_VKLITE_CASE(test_vklite_buffers_1);
    TST_VKLITE_CASE(test_vklite_buffer_views);
    TST_VKLITE_CASE(test_vklite_images_1);
    TST_VKLITE_CASE(test_vklite_descriptors_1);
    TST_VKLITE_CASE(test_vklite_graphics_1);
    TST_VKLITE_CASE(test_technique_triangle);
    TST_VKLITE_CASE(test_technique_render_texture);
    TST_VKLITE_CASE(test_technique_stencil);
    TST_VKLITE_CASE(test_technique_msaa);
    TST_VKLITE_CASE(test_technique_compute_graphics);
    TST_VKLITE_CASE(test_technique_picking);
    TST_VKLITE_CASE(test_technique_wboit);
    TST_VKLITE_CASE(test_technique_ssao);

    int res = tst_suite_run(suite, argc, argv);
    tst_suite_destroy(suite);
    return res;
}
