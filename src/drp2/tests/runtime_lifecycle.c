/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime lifecycle tests                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "../_runtime.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"
#include "vulkan_core.h"

#if DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#endif



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_runtime_vklite_skeleton_create_destroy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    AT(!cfg.semantic_only);
    AT(dvz_drp2_runtime_vklite(NULL) == NULL);
    AT(dvz_drp2_runtime_vklite(&cfg) == NULL);

    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_runtime_destroy(NULL);
    return 0;
}



int test_drp2_runtime_vklite_skeleton_execute_valid_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = drp2_test_valid_render_stream();
    ANN(stream);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_vklite_skeleton_execute_invalid_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_write_buffer(stream, 42, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_vklite_skeleton_rejects_null_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_render_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(NULL, stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_frame_target_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzStreamFrame frame = drp2_test_stream_frame(0x100, 4, 4);
    AT(!dvz_drp2_runtime_attach_frame_target(NULL, 7, &frame));
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 0, &frame));
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, NULL));

    DvzStreamFrame invalid = frame;
    invalid.image = VK_NULL_HANDLE;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.image_view = VK_NULL_HANDLE;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.command_buffer = VK_NULL_HANDLE;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.extent.width = 0;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.color_format = VK_FORMAT_UNDEFINED;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.usage = DVZ_STREAM_FRAME_USAGE_COPY_DST;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.command_buffer_recording = false;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    invalid = frame;
    invalid.image_borrowed = false;
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 7, &invalid));

    frame.color_format = VK_FORMAT_B8G8R8A8_UNORM;
    AT(dvz_drp2_runtime_attach_frame_target(runtime, 7, &frame));
    Drp2Object* frame_target = _drp2_find_any_object(runtime->semantic_state, 7);
    ANN(frame_target);
    AT(frame_target->format == VK_FORMAT_B8G8R8A8_UNORM);
    AT(frame_target->sample_count == 1);
    frame = drp2_test_stream_frame(0x200, 8, 4);
    AT(dvz_drp2_runtime_attach_frame_target(runtime, 7, &frame));
    frame_target = _drp2_find_any_object(runtime->semantic_state, 7);
    ANN(frame_target);
    AT(frame_target->format == VK_FORMAT_R8G8B8A8_UNORM);
    AT(frame_target->sample_count == 1);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 4));
    AT(dvz_drp2_stream_begin_render_pass(stream, 5, 4, 7));
    AT(dvz_drp2_stream_set_pipeline(stream, 5, 3));
    AT(dvz_drp2_stream_draw(stream, 5, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 5));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 4, 6));
    AT(dvz_drp2_stream_queue_submit(stream, 6, 8));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_deferred_destroy_flush(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    Drp2VkliteState state = {0};
    Drp2VkliteObject object = {
        .id = 77,
        .kind = DRP2_OBJECT_TEXTURE,
        .image_view = (VkImageView)(uintptr_t)0x456,
        .borrowed_frame_target = true,
    };
    VkCommandBuffer command_buffer = (VkCommandBuffer)(uintptr_t)0x123;

    AT(!_vklite_defer_destroy_object(&state, &object, VK_NULL_HANDLE));
    AT(!object.destroyed);
    AT(state.deferred_count == 0);

    AT(_vklite_defer_destroy_object(&state, &object, command_buffer));
    AT(object.destroyed);
    AT(state.deferred_count == 1);
    AT(state.deferred[0].command_buffer == command_buffer);
    AT(state.deferred[0].object.id == 77);

    _vklite_flush_deferred_for_command_buffer(&state, (VkCommandBuffer)(uintptr_t)0x999);
    AT(state.deferred_count == 1);

    _vklite_flush_deferred_for_command_buffer(&state, command_buffer);
    AT(state.deferred_count == 0);

    _vklite_state_cleanup(&state);
    return 0;
}



int test_drp2_runtime_vklite_trims_destroyed_tail_slots(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    Drp2VkliteState state = {0};
    Drp2VkliteObject* persistent = _vklite_add(&state, 1, DRP2_OBJECT_BUFFER);
    ANN(persistent);
    Drp2VkliteObject* transient = _vklite_add(&state, 2, DRP2_OBJECT_RENDER_PASS);
    ANN(transient);
    AT(state.count == 2);

    _vklite_destroy_object_slot(&state, transient);
    AT(state.count == 1);
    AT(state.objects[0].id == 1);
    AT(state.objects[1].id == 0);

    Drp2VkliteObject* inner = _vklite_add(&state, 3, DRP2_OBJECT_TEXTURE);
    ANN(inner);
    Drp2VkliteObject* tail = _vklite_add(&state, 4, DRP2_OBJECT_SHADER_VERTEX);
    ANN(tail);
    AT(state.count == 3);
    _vklite_destroy_object_slot(&state, inner);
    AT(state.count == 3);
    AT(state.objects[1].destroyed);
    _vklite_destroy_object_slot(&state, tail);
    AT(state.count == 1);

    Drp2VkliteObject* deferred = _vklite_add(&state, 5, DRP2_OBJECT_TEXTURE);
    ANN(deferred);
    VkCommandBuffer command_buffer = (VkCommandBuffer)(uintptr_t)0x123;
    AT(state.count == 2);
    AT(_vklite_defer_destroy_object(&state, deferred, command_buffer));
    AT(state.count == 1);
    AT(state.deferred_count == 1);
    AT(state.deferred[0].object.id == 5);

    _vklite_flush_deferred_for_command_buffer(&state, command_buffer);
    AT(state.deferred_count == 0);

    _vklite_state_cleanup(&state);
    return 0;
}
#endif



int test_drp2_runtime_download_buffer_rejects_out_of_range(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "DRP2 vklite out-of-range download test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 32,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t downloaded[16] = {0};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_drp2_runtime_download_buffer(runtime, 1, 24, 12, downloaded));
    AT(drp2_test_captured_log_contains(
        suite, "runtime buffer download [24, 36) exceeds buffer 1 size 32"));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
    };
    AT(dvz_drp2_runtime_download_buffer(runtime, 1, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_frame_lifecycle_edge_cases(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    /* Execute a stream that creates a texture, then attach a frame target to that texture id.
       The attach should succeed because the semantic object is already a DRP2_OBJECT_TEXTURE. */
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 9, 4, 4,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);

    DvzStreamFrame frame = drp2_test_stream_frame(0x100, 4, 4);
    AT(dvz_drp2_runtime_attach_frame_target(runtime, 9, &frame));

    /* Attach fails for a non-texture runtime object. */
    DvzDrp2CommandStream* stream2 = dvz_drp2_stream();
    ANN(stream2);
    AT(dvz_drp2_stream_create_buffer(
        stream2, 11, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    result = dvz_drp2_runtime_execute(runtime, stream2);
    AT(result.ok);
    AT(!dvz_drp2_runtime_attach_frame_target(runtime, 11, &frame));

    /* After attach failure, valid streams still execute. */
    DvzDrp2CommandStream* stream3 = dvz_drp2_stream();
    ANN(stream3);
    AT(dvz_drp2_stream_destroy_buffer(stream3, 11));
    result = dvz_drp2_runtime_execute(runtime, stream3);
    AT(result.ok);

    /* copy_texture_to_frame rejects NULL in semantic-only mode. */
    AT(!dvz_drp2_runtime_copy_texture_to_frame(NULL, 9, &frame));
    AT(!dvz_drp2_runtime_copy_texture_to_frame(runtime, 9, NULL));

    dvz_drp2_stream_destroy(stream3);
    dvz_drp2_stream_destroy(stream2);
    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}
