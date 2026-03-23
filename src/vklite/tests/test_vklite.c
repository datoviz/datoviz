/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/instance.h"

#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Probe whether the current runtime can create a Vulkan instance for vklite tests.
 *
 * @returns true when the runtime can create a Vulkan instance, false otherwise
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



static int test_vklite_runtime_unavailable(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    log_warn("vklite tests skipped because Vulkan runtime is unavailable");
    return 0;
}



int test_vklite(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vklite";
    if (!_vklite_runtime_available())
    {
        TEST_SIMPLE(test_vklite_runtime_unavailable);
        return 0;
    }

    TEST_SIMPLE(test_vklite_commands_1);
    TEST_SIMPLE(test_vklite_barriers_reset);
    TEST_SIMPLE(test_vklite_sampler_1);
    TEST_SIMPLE(test_vklite_shader_1);
    TEST_SIMPLE(test_vklite_slots_1);
    TEST_SIMPLE(test_vklite_compute_1);
    TEST_SIMPLE(test_vklite_buffers_1);
    TEST_SIMPLE(test_vklite_buffer_views);
    TEST_SIMPLE(test_vklite_images_1);
    TEST_SIMPLE(test_vklite_descriptors_1);
    TEST_SIMPLE(test_vklite_graphics_1);
    TEST_SIMPLE(test_vklite_fixture_screenshot_repeat);
    TEST_SIMPLE(test_vklite_surface_query);
    TEST_SIMPLE(test_vklite_swapchain_recreate);
    TEST_SIMPLE(test_vklite_swapchain_config_present_mode_immediate);
    TEST_SIMPLE(test_vklite_swapchain_config_defaults_partial);
    TEST_SIMPLE(test_vklite_swapchain_present_invalid_index);
    TEST_SIMPLE(test_vklite_swapchain_recreate_resolved_state);
    TEST_SIMPLE(test_vklite_wrap_backend_external_surface_present);

    TEST_SIMPLE(test_technique_triangle);
    TEST_SIMPLE(test_technique_render_texture);
    TEST_SIMPLE(test_technique_stencil);
    TEST_SIMPLE(test_technique_msaa);
    TEST_SIMPLE(test_technique_compute_graphics);
    TEST_SIMPLE(test_technique_picking);
    TEST_SIMPLE(test_technique_wboit);
    TEST_SIMPLE(test_technique_ssao);



    return 0;
}
