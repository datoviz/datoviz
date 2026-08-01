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
 * Skip vklite tests when the current runtime cannot create a Vulkan instance.
 *
 * @param suite test context
 * @param item current test case
 * @returns NULL when Vulkan is available, otherwise a skip reason
 */
static const char* _vklite_skip_unless_runtime(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzInstanceConfig cfg = dvz_instance_config();
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
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;                                     \
        _tst_desc.skip = _vklite_skip_unless_runtime;                                             \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)



int test_vklite(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vklite";
    TST_MODULE(suite, tags);

    TST_VKLITE_CASE(test_vklite_commands_1);
    TST_VKLITE_CASE(test_vklite_commands_repeat_submit);
    TST_VKLITE_CASE(test_vklite_commands_destroy_idempotent);
    TST_VKLITE_CASE(test_vklite_timeline_wait_blocks_until_signal);
    TST_VKLITE_CASE(test_vklite_commands_destroy_without_recording);
    TST_VKLITE_CASE(test_vklite_commands_borrowed_recording_rejects_lifecycle);
    TST_VKLITE_CASE(test_vklite_commands_borrowed_recording_unwrap);
    TST_VKLITE_CASE(test_vklite_barriers_reset);
    TST_VKLITE_CASE(test_vklite_submit_reset_reuse);
    TST_VKLITE_CASE(test_vklite_sampler_1);
    TST_VKLITE_CASE(test_vklite_shader_1);
    TST_VKLITE_CASE(test_vklite_shader_create_requires_destroy);
    TST_VKLITE_CASE(test_vklite_slots_1);
    TST_VKLITE_CASE(test_vklite_slots_create_failure_unwinds_layouts);
    TST_VKLITE_CASE(test_vklite_compute_1);
    TST_VKLITE_CASE(test_vklite_compute_create_requires_destroy);
    TST_VKLITE_CASE(test_vklite_buffers_1);
    TST_VKLITE_CASE(test_vklite_buffer_views);
    TST_VKLITE_CASE(test_vklite_buffer_create_requires_destroy);
    TST_VKLITE_CASE(test_vklite_images_1);
    TST_VKLITE_CASE(test_vklite_images_create_requires_destroy);
    TST_VKLITE_CASE(test_vklite_descriptors_1);
    TST_VKLITE_CASE(test_vklite_rendering_reset);
    TST_VKLITE_CASE(test_vklite_graphics_1);
    TST_VKLITE_CASE(test_vklite_graphics_create_requires_destroy);
    TST_VKLITE_CASE(test_vklite_fixture_screenshot_repeat);
    TST_VKLITE_CASE(test_vklite_surface_query);
    TST_VKLITE_CASE(test_vklite_swapchain_recreate);
    TST_VKLITE_CASE(test_vklite_surface_swapchain_destroy_idempotent);
    TST_VKLITE_CASE(test_vklite_swapchain_config_present_mode_immediate);
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    TST_VKLITE_CASE(test_vklite_swapchain_present_mode_fifo_latest_ready);
#endif
    TST_VKLITE_CASE(test_vklite_swapchain_config_defaults_partial);
    TST_VKLITE_CASE(test_vklite_swapchain_present_invalid_index);
    TST_VKLITE_CASE(test_vklite_swapchain_recreate_resolved_state);
    TST_VKLITE_CASE(test_vklite_swapchain_recreate_repeat_state);
    TST_VKLITE_CASE(test_vklite_swapchain_destroy_clears_cached_state);
    TST_VKLITE_CASE(test_vklite_swapchain_acquire_present_cycle);
    TST_VKLITE_CASE(test_vklite_wrap_backend_external_surface_present);

    TST_VKLITE_CASE(test_technique_triangle);
    TST_VKLITE_CASE(test_technique_render_texture);
    TST_VKLITE_CASE(test_technique_stencil);
    TST_VKLITE_CASE(test_technique_msaa);
    TST_VKLITE_CASE(test_technique_compute_graphics);
    TST_VKLITE_CASE(test_technique_picking);
    TST_VKLITE_CASE(test_technique_wboit);
    TST_VKLITE_CASE(test_technique_ssao);



    return 0;
}
