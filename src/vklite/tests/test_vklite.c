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



static int test_vklite_runtime_unavailable(TstContext* suite, const TstCase* item)
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
    TST_MODULE(suite, tags);
    if (!_vklite_runtime_available())
    {
        TST_CASE(test_vklite_runtime_unavailable);
        return 0;
    }

    TST_CASE(test_vklite_commands_1);
    TST_CASE(test_vklite_commands_repeat_submit);
    TST_CASE(test_vklite_commands_destroy_idempotent);
    TST_CASE(test_vklite_commands_destroy_without_recording);
    TST_CASE(test_vklite_commands_borrowed_recording_rejects_lifecycle);
    TST_CASE(test_vklite_barriers_reset);
    TST_CASE(test_vklite_submit_reset_reuse);
    TST_CASE(test_vklite_sampler_1);
    TST_CASE(test_vklite_shader_1);
    TST_CASE(test_vklite_shader_create_requires_destroy);
    TST_CASE(test_vklite_slots_1);
    TST_CASE(test_vklite_slots_create_failure_unwinds_layouts);
    TST_CASE(test_vklite_compute_1);
    TST_CASE(test_vklite_compute_create_requires_destroy);
    TST_CASE(test_vklite_buffers_1);
    TST_CASE(test_vklite_buffer_views);
    TST_CASE(test_vklite_buffer_create_requires_destroy);
    TST_CASE(test_vklite_images_1);
    TST_CASE(test_vklite_images_create_requires_destroy);
    TST_CASE(test_vklite_descriptors_1);
    TST_CASE(test_vklite_rendering_reset);
    TST_CASE(test_vklite_graphics_1);
    TST_CASE(test_vklite_graphics_create_requires_destroy);
    TST_CASE(test_vklite_fixture_screenshot_repeat);
    TST_CASE(test_vklite_surface_query);
    TST_CASE(test_vklite_swapchain_recreate);
    TST_CASE(test_vklite_surface_swapchain_destroy_idempotent);
    TST_CASE(test_vklite_swapchain_config_present_mode_immediate);
    TST_CASE(test_vklite_swapchain_config_defaults_partial);
    TST_CASE(test_vklite_swapchain_present_invalid_index);
    TST_CASE(test_vklite_swapchain_recreate_resolved_state);
    TST_CASE(test_vklite_swapchain_recreate_repeat_state);
    TST_CASE(test_vklite_swapchain_destroy_clears_cached_state);
    TST_CASE(test_vklite_swapchain_acquire_present_cycle);
    TST_CASE(test_vklite_wrap_backend_external_surface_present);

    TST_CASE(test_technique_triangle);
    TST_CASE(test_technique_render_texture);
    TST_CASE(test_technique_stencil);
    TST_CASE(test_technique_msaa);
    TST_CASE(test_technique_compute_graphics);
    TST_CASE(test_technique_picking);
    TST_CASE(test_technique_wboit);
    TST_CASE(test_technique_ssao);



    return 0;
}
