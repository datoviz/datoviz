/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing DRP2                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "testing.h"

#if DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#include "../_runtime.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"

bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _captured_log_contains(const TstSuite* suite, const char* needle)
{
    ANN(suite);
    ANN(needle);
    for (uint32_t i = 0; i < tst_log_capture_count(suite); i++)
    {
        const TstLogRecord* rec = tst_log_capture_get(suite, i);
        if (rec != NULL && strstr(rec->message, needle) != NULL)
            return true;
    }
    return false;
}



#if DVZ_DRP2_HAS_VKLITE
/**
 * Probe whether the current runtime can create a Vulkan instance for DRP2 vklite execution tests.
 *
 * @return true when the runtime can create a Vulkan instance, false otherwise
 */
static bool _drp2_vklite_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("DRP2 vklite execution test skipped because Vulkan instance creation failed");
        return false;
    }
    dvz_instance_destroy(instance);
    return true;
}
#endif

static DvzDrp2CommandStream* _valid_render_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA==");
    dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}");
    dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}");
    dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1);
    dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4);
    dvz_drp2_stream_begin_command_encoder(stream, 6);
    dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5);
    dvz_drp2_stream_set_pipeline(stream, 7, 4);
    dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 1, 0);
    dvz_drp2_stream_draw(stream, 7, 3, 1, 0, 0);
    dvz_drp2_stream_end_render_pass(stream, 7);
    dvz_drp2_stream_finish_command_encoder(stream, 6, 8);
    dvz_drp2_stream_queue_submit(stream, 8, 9);
    return stream;
}



static DvzDrp2CommandStream* _valid_indexed_render_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}");
    dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}");
    dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1);
    dvz_drp2_stream_create_buffer(stream, 11, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_drp2_stream_create_buffer(stream, 12, 64, DVZ_DRP2_BUFFER_USAGE_INDEX);
    dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4);
    dvz_drp2_stream_begin_command_encoder(stream, 6);
    dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5);
    dvz_drp2_stream_set_pipeline(stream, 7, 4);
    dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 11, 0);
    dvz_drp2_stream_set_index_buffer(stream, 7, 12, "uint16", 0);
    dvz_drp2_stream_draw_indexed(stream, 7, 3, 1, 0, 0, 0);
    dvz_drp2_stream_end_render_pass(stream, 7);
    dvz_drp2_stream_finish_command_encoder(stream, 6, 8);
    return stream;
}



static DvzDrp2CommandStream* _valid_compute_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}");
    dvz_drp2_stream_create_compute_pipeline(stream, 20, 9000);
    dvz_drp2_stream_begin_command_encoder(stream, 1);
    dvz_drp2_stream_begin_compute_pass(stream, 2, 1);
    dvz_drp2_stream_set_pipeline(stream, 2, 20);
    dvz_drp2_stream_dispatch_workgroups(stream, 2, 1, 1, 1);
    dvz_drp2_stream_end_compute_pass(stream, 2);
    dvz_drp2_stream_finish_command_encoder(stream, 1, 3);
    return stream;
}


/**
 * Return a non-owning stream frame descriptor with stable fake handles.
 *
 * @param seed seed used to make the fake handles distinct
 * @param width frame width
 * @param height frame height
 * @return stream frame descriptor
 */
static DvzStreamFrame _test_stream_frame(uintptr_t seed, uint32_t width, uint32_t height)
{
    DvzStreamFrame frame = {0};
    frame.image = (VkImage)(seed + 1);
    frame.image_view = (VkImageView)(seed + 2);
    frame.command_buffer = (VkCommandBuffer)(seed + 3);
    frame.extent = (VkExtent2D){width, height};
    frame.color_format = VK_FORMAT_R8G8B8A8_UNORM;
    frame.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    frame.usage = DVZ_STREAM_FRAME_USAGE_RENDER_TARGET | DVZ_STREAM_FRAME_USAGE_COPY_DST;
    frame.command_buffer_recording = true;
    frame.image_borrowed = true;
    frame.image_view_borrowed = true;
    frame.command_buffer_borrowed = true;
    return frame;
}


/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/


int test_drp2_stream_empty(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_count(stream) == 0);
    AT(dvz_drp2_stream_get(stream, 0) == NULL);
    AT(dvz_drp2_command_type(NULL) == DVZ_DRP2_COMMAND_NONE);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_stream_destroy(NULL);
    return 0;
}



int test_drp2_stream_append(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 2, 3));
    AT(dvz_drp2_stream_queue_submit_readback(stream, 3, 4, 1, 0, 16));

    AT(dvz_drp2_stream_count(stream) == 7);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);
    AT(dvz_drp2_stream_get(stream, 7) == NULL);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_json(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "fixture-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "fixture-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 4, 4, "AQIDBA=="));

    char* json = dvz_drp2_stream_json(stream, "write_buffer_basic_from_c");
    ANN(json);

    AT(strstr(json, "\"name\": \"write_buffer_basic_from_c\"") != NULL);
    AT(strstr(json, "\"cmd\": \"HelloRenderer\"") != NULL);
    AT(strstr(json, "\"cmd\": \"RendererHelloReply\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\"]") != NULL);
    AT(strstr(json, "\"cmd\": \"WriteBuffer\"") != NULL);
    AT(strstr(json, "\"data\": \"AQIDBA==\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_growth_json(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    for (uint32_t i = 0; i < 160; i++)
        AT(dvz_drp2_stream_create_buffer(stream, i + 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    AT(dvz_drp2_stream_count(stream) == 160);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 159)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_stream_get(stream, 160) == NULL);

    char* json = dvz_drp2_stream_json(stream, "stream_growth");
    ANN(json);
    AT(strstr(json, "\"name\": \"stream_growth\"") != NULL);
    AT(strstr(json, "\"id\": 160") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_render_stream(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_render_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_render_state_inherited_across_passes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));
    AT(dvz_drp2_stream_create_buffer(stream, 10, 16, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(stream, 11));
    AT(dvz_drp2_stream_create_uniform_bind_group(stream, 12, 11, 10, 0, 16));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_pipeline_set_bind_group_layout(stream, 11));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));

    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_set_bind_group(stream, 7, 0, 12));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 7, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 7));

    AT(dvz_drp2_stream_begin_render_pass(stream, 8, 6, 5));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 8, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 8, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 8));

    AT(dvz_drp2_stream_finish_command_encoder(stream, 6, 9));
    AT(dvz_drp2_stream_queue_submit(stream, 9, 13));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_dynamic_viewport_scissor(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_render_stream();
    ANN(stream);

    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 5));
    AT(dvz_drp2_stream_set_viewport(stream, 21, 0.25f, 0.0f, 0.5f, 1.0f));
    AT(dvz_drp2_stream_set_scissor(stream, 21, 0.25f, 0.0f, 0.5f, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 21, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 21));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 20, 22));
    AT(dvz_drp2_stream_queue_submit(stream, 22, 23));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_duplicate_id(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_failed_stream_does_not_commit_state(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);

    DvzDrp2CommandStream* bad = dvz_drp2_stream();
    ANN(bad);
    AT(dvz_drp2_stream_create_buffer(bad, 2, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_buffer(bad, 2, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    result = dvz_drp2_runtime_execute(runtime, bad);
    AT(!result.ok);

    DvzDrp2CommandStream* retry = dvz_drp2_stream();
    ANN(retry);
    AT(dvz_drp2_stream_create_buffer(retry, 2, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    result = dvz_drp2_runtime_execute(runtime, retry);
    AT(result.ok);

    dvz_drp2_stream_destroy(retry);
    dvz_drp2_stream_destroy(bad);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_rejects_unknown_buffer_write(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_write_buffer(stream, 42, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 2);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_draw_without_vertex_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_draw(stream, 7, 3, 1, 0, 0));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 9);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_finish_with_open_pass(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 6, 8));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_bad_readback_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 2, 3));
    AT(dvz_drp2_stream_queue_submit_readback(stream, 3, 4, 1, 0, 16));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_compute_stream(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_compute_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(stream, "compute_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateComputePipeline\"") != NULL);
    AT(strstr(json, "\"cmd\": \"BeginComputePass\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);
    AT(strstr(json, "\"cmd\": \"EndComputePass\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_dispatch_without_pipeline(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}"));
    AT(dvz_drp2_stream_create_compute_pipeline(stream, 20, 9000));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 1));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 2, 1));
    AT(dvz_drp2_stream_dispatch_workgroups(stream, 2, 1, 1, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 6);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_dispatch_outside_compute_pass(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}"));
    AT(dvz_drp2_stream_create_compute_pipeline(stream, 20, 9000));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 1));
    AT(dvz_drp2_stream_dispatch_workgroups(stream, 2, 1, 1, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_wrong_pipeline_type(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 0));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 1));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 5, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 5, 4));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 7);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_finish_with_open_compute_pass(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 1));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 2, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 1, 3));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_indexed_render_stream(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_indexed_render_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(stream, "indexed_render_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"SetIndexBuffer\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DrawIndexed\"") != NULL);
    AT(strstr(json, "\"index_format\": \"uint16\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_draw_indexed_without_index_buffer(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_create_buffer(stream, 11, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 11, 0));
    AT(dvz_drp2_stream_draw_indexed(stream, 7, 3, 1, 0, 0, 0));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 11);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_wrong_index_buffer_usage(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_indexed_render_stream();
    ANN(stream);

    // Build an otherwise equivalent stream where the index buffer has only VERTEX usage.
    dvz_drp2_stream_destroy(stream);
    stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_create_buffer(stream, 11, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_create_buffer(stream, 12, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 11, 0));
    AT(dvz_drp2_stream_set_index_buffer(stream, 7, 12, "uint16", 0));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 12);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_write_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d(stream, 1, 0, 2, 1, 8, 1, "AAAAAAAAAAA="));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "write_texture_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"WriteTexture\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\"]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_copy_buffer_to_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 3, 1, 0, 2, 2, 1, 8, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "copy_buffer_to_texture_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyBufferToTexture\"") != NULL);
    AT(strstr(json, "\"dst_texture_id\": 2") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_copy_texture_to_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_texture_to_texture(stream, 10, 1, 2, 2, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "copy_texture_to_texture_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyTextureToTexture\"") != NULL);
    AT(strstr(json, "\"src_texture_id\": 1") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_texture_sampler_bind_group(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_sampler(stream, 200));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 9000, "VERTEX", "@vertex fn main() -> @builtin(position) vec4f { return vec4f(); }"));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 9001, "FRAGMENT",
        "@group(0) @binding(0) var source: texture_2d<f32>; @group(0) @binding(1) var samp: "
        "sampler; @fragment fn main() -> @location(0) vec4f { return textureSample(source, samp, "
        "vec2f(0.5)); }"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d(stream, 2, 0, 2, 2, 8, 2, "AAAAAAAAAAAAAAAAAAAAAA=="));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 13, 100, 2, 200));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 4, 4, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group(stream, 21, 0, 13));
    AT(dvz_drp2_stream_draw(stream, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 21));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 20, 22));
    AT(dvz_drp2_stream_queue_submit(stream, 22, 30));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "texture_sampler_bind_group_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroup\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [100]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_generic_bind_group_slots(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry layout0 = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    DvzDrp2BindGroupLayoutEntry layout1 = {
        .binding = 3,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    DvzDrp2BindGroupEntry group0 = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 50,
        .size = 16,
    };
    DvzDrp2BindGroupEntry group1 = {
        .binding = 3,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 51,
        .offset = 16,
        .size = 16,
    };
    uint64_t layouts[2] = {100, 101};

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 50, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_buffer(stream, 51, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 100, 1, &layout0));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 101, 1, &layout1));
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 110, 100, 1, &group0));
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 111, 101, 1, &group1));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 9001, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 10, 9000, 9001, 0));
    AT(dvz_drp2_stream_pipeline_set_bind_group_layouts(stream, 2, layouts));
    AT(dvz_drp2_stream_create_texture_2d(stream, 1, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group(stream, 21, 0, 110));
    AT(dvz_drp2_stream_set_bind_group(stream, 21, 1, 111));
    AT(dvz_drp2_stream_draw(stream, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 21));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 20, 22));
    AT(dvz_drp2_stream_queue_submit(stream, 22, 30));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(stream, "generic_bind_group_slots_from_c");
    ANN(json);
    AT(strstr(json, "\"bind_group_layout_ids\": [100, 101]") != NULL);
    AT(strstr(json, "\"binding\": 3") != NULL);
    AT(strstr(json, "\"visibility\": [\"FRAGMENT\"]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_bind_group_entry_mismatch(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry layout = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    DvzDrp2BindGroupEntry entry = {
        .binding = 1,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 2,
        .size = 16,
    };

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 2, 16, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 100, 1, &layout));
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 101, 100, 1, &entry));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_bind_group_dynamic_offsets(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2BindGroupLayoutEntry layout = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
        .has_dynamic_offset = true,
    };
    DvzDrp2BindGroupEntry entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 2,
        .offset = 8,
        .size = 16,
    };

    DvzDrp2CommandStream* ok_stream = dvz_drp2_stream();
    ANN(ok_stream);
    uint64_t ok_offset = 40;

    AT(dvz_drp2_stream_hello_renderer(ok_stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(ok_stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(ok_stream, 2, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(ok_stream, 100, 1, &layout));
    AT(dvz_drp2_stream_create_bind_group_entries(ok_stream, 101, 100, 1, &entry));
    AT(dvz_drp2_stream_create_shader_module(ok_stream, 9000, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(
        ok_stream, 9001, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        ok_stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_create_texture_2d(ok_stream, 1, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(ok_stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(ok_stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(ok_stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group_dynamic(ok_stream, 21, 0, 101, 1, &ok_offset));
    AT(dvz_drp2_stream_draw(ok_stream, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(ok_stream, 21));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(ok_stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(ok_stream, "dynamic_bind_group_from_c");
    ANN(json);
    AT(strstr(json, "\"has_dynamic_offset\": true") != NULL);
    AT(strstr(json, "\"dynamic_offsets\": [40]") != NULL);
    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(ok_stream);

    DvzDrp2CommandStream* bad_stream = dvz_drp2_stream();
    ANN(bad_stream);
    uint64_t bad_offset = 41;

    AT(dvz_drp2_stream_hello_renderer(bad_stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(bad_stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(bad_stream, 2, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(bad_stream, 100, 1, &layout));
    AT(dvz_drp2_stream_create_bind_group_entries(bad_stream, 101, 100, 1, &entry));
    AT(dvz_drp2_stream_create_shader_module(bad_stream, 9000, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(
        bad_stream, 9001, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        bad_stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_create_texture_2d(bad_stream, 1, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(bad_stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(bad_stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(bad_stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group_dynamic(bad_stream, 21, 0, 101, 1, &bad_offset));

    result = dvz_drp2_validate_stream(bad_stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);

    dvz_drp2_stream_destroy(bad_stream);
    return 0;
}



int test_drp2_runtime_validate_bind_group_after_table_growth(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 4, 4, DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_sampler(stream, 2));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 3));

    for (uint64_t id = 100; id < 161; id++)
        AT(dvz_drp2_stream_create_buffer(stream, id, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));

    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 4, 3, 1, 2));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_reuses_submitted_transient_ids(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = {0};
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(setup, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(setup, 2, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(setup, 3, 1, 2, 0));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 4, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    for (uint32_t i = 0; i < 3; i++)
    {
        DvzDrp2CommandStream* frame = dvz_drp2_stream();
        ANN(frame);
        AT(dvz_drp2_stream_begin_command_encoder(frame, 10));
        AT(dvz_drp2_stream_begin_render_pass(frame, 11, 10, 4));
        AT(dvz_drp2_stream_set_pipeline(frame, 11, 3));
        AT(dvz_drp2_stream_draw(frame, 11, 3, 1, 0, 0));
        AT(dvz_drp2_stream_end_render_pass(frame, 11));
        AT(dvz_drp2_stream_finish_command_encoder(frame, 10, 12));
        AT(dvz_drp2_stream_queue_submit(frame, 12, 13));

        result = dvz_drp2_runtime_execute(runtime, frame);
        AT(result.ok);
        AT(result.code == DVZ_DRP2_VALIDATION_OK);
        dvz_drp2_stream_destroy(frame);
    }

    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_validate_compute_storage_bind_group(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_storage_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}"));
    AT(dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(stream, 10, 9000, 100));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 36, DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_buffer(
        stream, 3, 36, DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_write_buffer(stream, 2, 0, 36, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
    AT(dvz_drp2_stream_create_storage_bind_group(stream, 101, 100, 2, 3, 36));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 21, 20));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group(stream, 21, 0, 101));
    AT(dvz_drp2_stream_dispatch_workgroups(stream, 21, 1, 1, 1));
    AT(dvz_drp2_stream_end_compute_pass(stream, 21));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "compute_storage_bind_group_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"binding_type\": \"storage_buffer\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_destroy_unused_bind_group(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_sampler(stream, 200));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 13, 100, 2, 200));
    AT(dvz_drp2_stream_destroy_bind_group(stream, 13));
    AT(dvz_drp2_stream_destroy_bind_group_layout(stream, 100));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "destroy_bind_group_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"DestroyBindGroup\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DestroyBindGroupLayout\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_sampler(stream, 200));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 13, 100, 2, 200));
    AT(dvz_drp2_stream_destroy_bind_group_layout(stream, 100));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 6);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "VERTEX", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 9001, "FRAGMENT", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_destroy_bind_group_layout(stream, 100));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 6);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_sampler(stream, 200));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "VERTEX", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 9001, "FRAGMENT", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
        stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 13, 100, 2, 200));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 4, 4, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 10));
    AT(dvz_drp2_stream_set_bind_group(stream, 21, 0, 13));
    AT(dvz_drp2_stream_draw(stream, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 21));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 20, 22));
    AT(dvz_drp2_stream_destroy_bind_group(stream, 13));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 17);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_compute_dispatch_without_bind_group(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_storage_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}"));
    AT(dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(stream, 10, 9000, 100));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 21, 20));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 10));
    AT(dvz_drp2_stream_dispatch_workgroups(stream, 21, 1, 1, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 8);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_write_texture_out_of_range(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d(stream, 1, 0, 3, 1, 12, 1, "AAAAAAAAAAAAAAAA"));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_rejects_write_texture_layout_size_overflow(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_3d(stream, 1, 1, 1, UINT32_MAX));
    AT(dvz_drp2_stream_write_texture_3d(
        stream, 1, 0, 0, 0, 0, 1, 1, UINT32_MAX, UINT32_MAX, UINT32_MAX, ""));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_copy_buffer_to_texture_usage(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 3, 1, 0, 2, 2, 1, 8, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_copy_texture_to_texture_inside_pass(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_compute_pass(stream, 11, 10));
    AT(dvz_drp2_stream_copy_texture_to_texture(stream, 10, 1, 2, 2, 2));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 6);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_destroy_unused_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 64, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    char* json = dvz_drp2_stream_json(stream, "destroy_buffer_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"DestroyBuffer\"") != NULL);
    AT(strstr(json, "\"buffer_id\": 1") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_use_after_destroy(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 64, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_buffer_referenced_by_work(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 64, DVZ_DRP2_BUFFER_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(stream, 2, 64, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 3, 1, 0, 2, 0, 16));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 3, 4));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 7);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_texture_referenced_by_work(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 3, 1, 0, 2, 2, 1, 8, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 3, 4));
    AT(dvz_drp2_stream_destroy_texture(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 7);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_submitted_render_pipeline(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_render_stream();
    ANN(stream);
    AT(dvz_drp2_stream_destroy_render_pipeline(stream, 4));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 16);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_live_shader_module(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 4, 2, 3, 0));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_skeleton_create_destroy(TstSuite* suite, TstItem* item)
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



int test_drp2_runtime_vklite_skeleton_execute_valid_stream(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = _valid_render_stream();
    ANN(stream);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_vklite_skeleton_execute_invalid_stream(TstSuite* suite, TstItem* item)
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



int test_drp2_runtime_vklite_skeleton_rejects_null_runtime(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = _valid_render_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(NULL, stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_frame_target_validation(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzStreamFrame frame = _test_stream_frame(0x100, 4, 4);
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

    AT(dvz_drp2_runtime_attach_frame_target(runtime, 7, &frame));
    frame = _test_stream_frame(0x200, 8, 4);
    AT(dvz_drp2_runtime_attach_frame_target(runtime, 7, &frame));

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
int test_drp2_runtime_vklite_deferred_destroy_flush(TstSuite* suite, TstItem* item)
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


int test_drp2_runtime_vklite_trims_destroyed_tail_slots(TstSuite* suite, TstItem* item)
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



int test_drp2_runtime_frame_lifecycle_edge_cases(TstSuite* suite, TstItem* item)
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

    DvzStreamFrame frame = _test_stream_frame(0x100, 4, 4);
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



#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_executes_resource_commands(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite execution test skipped because GPU context creation failed");
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
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));
    AT(dvz_drp2_stream_destroy_texture(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_writes_buffer_contents(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite buffer content test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_write_buffer(stream, 1, 8, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 1, 8, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_copies_buffer_contents(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite buffer copy test skipped because GPU context creation failed");
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
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 32, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 4, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 10, 1, 4, 2, 12, 16));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 2, 12, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_download_buffer_rejects_out_of_range(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

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
    AT(_captured_log_contains(suite, "runtime buffer download [24, 36) exceeds buffer 1 size 32"));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
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



int test_drp2_runtime_vklite_writes_texture_contents(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite texture write test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_write_texture_2d(
        stream, 1, 0, 2, 2, 8, 2, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 1, 2, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 2, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_copies_buffer_to_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite texture copy test skipped because GPU context creation failed");
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
        stream, 1, 16,
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer(stream, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 3, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 10, 1, 0, 2, 2, 2, 8, 2));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 2, 3, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 3, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_copies_texture_to_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite texture-to-texture test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_write_texture_2d(
        stream, 1, 0, 2, 2, 8, 2, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_buffer(
        stream, 3, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_texture_to_texture(stream, 10, 1, 2, 2, 2));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 2, 3, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 3, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_creates_glsl_shader_modules(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite GLSL shader test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 1));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_creates_render_pipeline(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite render-pipeline test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_destroy_render_pipeline(stream, 3));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 1));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_rejects_invalid_glsl_shader(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_drp2_runtime_vklite_rejects_invalid_glsl_shader skipped: no GPU");
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
    /* Intentionally broken GLSL — missing version directive and garbled syntax. */
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));

    tst_log_capture_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);
    AT(_captured_log_contains(suite, "GLSL compilation failed"));

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader skipped: no GPU");
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
    /* Valid fragment, broken vertex — pipeline creation must fail cleanly. */
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 c;void main(){c=vec4(1.0);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));

    tst_log_capture_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(!result.ok);
    /* Runtime must not crash or leave a NULL pipeline object registered. */
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_destroy_after_partial_failure(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_drp2_runtime_vklite_destroy_after_partial_failure skipped: no GPU");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    /* Execute a stream that fails mid-way (bad GLSL). */
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));

    tst_log_capture_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(!result.ok);
    dvz_drp2_stream_destroy(stream);

    /* Destroy the runtime after the failed execution — must not crash or leak. */
    dvz_drp2_runtime_destroy(runtime);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_reallocates_object_table_safely(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite object-table realloc test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));

    for (uint64_t id = 100; id < 161; id++)
    {
        AT(dvz_drp2_stream_create_sampler(stream, id));
    }

    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));

    for (uint64_t id = 200; id < 262; id++)
    {
        AT(dvz_drp2_stream_create_sampler(stream, id));
    }

    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_draws_render_pass(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite render-pass test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_samples_then_copies_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 vklite texture layout test skipped because GPU context creation failed");
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
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform sampler2D tex;"
        "layout(location=0)out vec4 color;"
        "void main(){color=texture(tex,vec2(0.5));}"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 3));
    AT(dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(stream, 4, 1, 2, 0, 3));
    AT(dvz_drp2_stream_create_sampler(stream, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 6, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC |
            DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d(
        stream, 6, 0, 2, 2, 8, 2, "/wAA//8AAP//AAD//wAA/w=="));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 7, 3, 6, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 8, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 9, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 8));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 4));
    AT(dvz_drp2_stream_set_bind_group(stream, 11, 0, 7));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 6, 9, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 9, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i += 4)
    {
        AT(downloaded[i + 0] == 255);
        AT(downloaded[i + 1] == 0);
        AT(downloaded[i + 2] == 0);
        AT(downloaded[i + 3] == 255);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
#endif



/* ---- New regression tests ---- */

int test_drp2_write_buffer_bytes_uses_data_raw(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    static const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));
    AT(dvz_drp2_stream_count(stream) == 1);

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    /* The in-process path must store the raw pointer, NOT encode to base64. */
    AT(cmd->u.write_buffer.data_raw == (const void*)payload);
    AT(cmd->u.write_buffer.data_base64 == NULL);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_write_buffer_bytes_json_encodes_data_raw(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    /* {1,2,3,4} → base64 "AQIDBA==" */
    static const uint8_t payload[4] = {1, 2, 3, 4};
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, sizeof(payload), payload));

    char* json = dvz_drp2_stream_json(stream, "write_buffer_bytes_json_test");
    ANN(json);
    /* Verify the JSON serializer correctly encodes data_raw on the fly. */
    AT(strstr(json, "\"data\": \"AQIDBA==\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



#if DVZ_DRP2_HAS_VKLITE
int test_drp2_write_buffer_bytes_large_payload_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_drp2_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("large payload test skipped because GPU context creation failed");
        return 0;
    }

    /* 3000 floats = 12000 bytes — well above the old 4096-byte dvz_strdup cap. */
    const uint32_t N    = 3000;
    const uint64_t SIZE = N * sizeof(float);
    float* upload = (float*)dvz_malloc(SIZE);
    ANN(upload);
    for (uint32_t i = 0; i < N; i++)
        upload[i] = (float)i;

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, SIZE,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, SIZE, upload));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    float* downloaded = (float*)dvz_malloc(SIZE);
    ANN(downloaded);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 1, 0, SIZE, downloaded));
    for (uint32_t i = 0; i < N; i++)
    {
        AT(downloaded[i] == upload[i]);
    }

    dvz_free(downloaded);
    dvz_free(upload);
    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
#endif



int test_drp2_begin_render_pass_clear_color_stored(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 1, 2, 3, 0.2f, 0.4f, 0.6f, 1.0f));
    AT(dvz_drp2_stream_count(stream) == 1);

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AC(cmd->u.begin_render_pass.clear_color[0], 0.2f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[1], 0.4f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[2], 0.6f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[3], 1.0f, 1e-6f);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_stream_json_preserves_clear_color(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    /* Use a distinctive non-zero clear color so the hardcoded-zeros bug is visible. */
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 1, 2, 3, 0.5f, 0.25f, 0.125f, 1.0f));

    char* json = dvz_drp2_stream_json(stream, "clear_color_json_test");
    ANN(json);
    /* JSON must contain the actual red component, not the hardcoded 0. */
    AT(strstr(json, "0.5") != NULL);
    AT(strstr(json, "0.25") != NULL);
    AT(strstr(json, "0.125") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_write_buffer_bytes_large_json_roundtrip(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    /* 3000 floats = 12000 bytes — verifies JSON serializer encodes the full data_raw payload. */
    const uint32_t N    = 3000;
    const uint64_t SIZE = N * sizeof(float);
    float* data = (float*)dvz_malloc(SIZE);
    ANN(data);
    for (uint32_t i = 0; i < N; i++)
        data[i] = (float)i * 0.5f;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, SIZE, data));

    char* json = dvz_drp2_stream_json(stream, "large_json_roundtrip");
    ANN(json);
    /* JSON must contain a "data" field — non-empty base64 encoding of the full payload. */
    AT(strstr(json, "\"data\": \"") != NULL);
    /* The base64 of 12000 bytes is 16000 chars — verify the JSON is large enough. */
    AT(strlen(json) > 16000);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_free(data);
    return 0;
}


int test_drp2_render_pipeline_step_modes_json(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint32_t strides[2] = {3 * sizeof(float), 4 * sizeof(uint8_t)};
    uint32_t step_modes[2] = {
        DVZ_DRP2_VERTEX_STEP_MODE_VERTEX,
        DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE,
    };
    uint32_t bindings[2] = {0, 1};
    uint32_t locations[2] = {0, 1};
    uint32_t formats[2] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
    uint32_t offsets[2] = {0, 0};

    AT(dvz_drp2_stream_create_render_pipeline_ex2(
        stream, 10, 9000, 9001, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 2, strides,
        step_modes, 2, bindings, locations, formats, offsets));

    char* json = dvz_drp2_stream_json(stream, "pipeline_step_modes");
    ANN(json);
    AT(strstr(json, "\"step_mode\": \"vertex\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_drp2(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "drp2";

    TEST_SIMPLE(test_drp2_stream_empty);
    TEST_SIMPLE(test_drp2_stream_append);
    TEST_SIMPLE(test_drp2_stream_json);
    TEST_SIMPLE(test_drp2_stream_growth_json);
    TEST_SIMPLE(test_drp2_write_buffer_bytes_uses_data_raw);
    TEST_SIMPLE(test_drp2_write_buffer_bytes_json_encodes_data_raw);
    TEST_SIMPLE(test_drp2_write_buffer_bytes_large_json_roundtrip);
    TEST_SIMPLE(test_drp2_render_pipeline_step_modes_json);
    TEST_SIMPLE(test_drp2_begin_render_pass_clear_color_stored);
    TEST_SIMPLE(test_drp2_stream_json_preserves_clear_color);
    TEST_SIMPLE(test_drp2_runtime_validate_render_stream);
    TEST_SIMPLE(test_drp2_runtime_validate_render_state_inherited_across_passes);
    TEST_SIMPLE(test_drp2_runtime_validate_dynamic_viewport_scissor);
    TEST_SIMPLE(test_drp2_runtime_rejects_duplicate_id);
    TEST_SIMPLE(test_drp2_runtime_failed_stream_does_not_commit_state);
    TEST_SIMPLE(test_drp2_runtime_rejects_unknown_buffer_write);
    TEST_SIMPLE(test_drp2_runtime_rejects_draw_without_vertex_buffer);
    TEST_SIMPLE(test_drp2_runtime_rejects_finish_with_open_pass);
    TEST_SIMPLE(test_drp2_runtime_rejects_bad_readback_buffer);
    TEST_SIMPLE(test_drp2_runtime_validate_compute_stream);
    TEST_SIMPLE(test_drp2_runtime_rejects_dispatch_without_pipeline);
    TEST_SIMPLE(test_drp2_runtime_rejects_dispatch_outside_compute_pass);
    TEST_SIMPLE(test_drp2_runtime_rejects_wrong_pipeline_type);
    TEST_SIMPLE(test_drp2_runtime_rejects_finish_with_open_compute_pass);
    TEST_SIMPLE(test_drp2_runtime_validate_indexed_render_stream);
    TEST_SIMPLE(test_drp2_runtime_rejects_draw_indexed_without_index_buffer);
    TEST_SIMPLE(test_drp2_runtime_rejects_wrong_index_buffer_usage);
    TEST_SIMPLE(test_drp2_runtime_validate_write_texture);
    TEST_SIMPLE(test_drp2_runtime_validate_copy_buffer_to_texture);
    TEST_SIMPLE(test_drp2_runtime_validate_copy_texture_to_texture);
    TEST_SIMPLE(test_drp2_runtime_validate_texture_sampler_bind_group);
    TEST_SIMPLE(test_drp2_runtime_validate_generic_bind_group_slots);
    TEST_SIMPLE(test_drp2_runtime_rejects_bind_group_entry_mismatch);
    TEST_SIMPLE(test_drp2_runtime_validate_bind_group_dynamic_offsets);
    TEST_SIMPLE(test_drp2_runtime_validate_bind_group_after_table_growth);
    TEST_SIMPLE(test_drp2_runtime_reuses_submitted_transient_ids);
    TEST_SIMPLE(test_drp2_runtime_validate_compute_storage_bind_group);
    TEST_SIMPLE(test_drp2_runtime_validate_destroy_unused_bind_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work);
    TEST_SIMPLE(test_drp2_runtime_rejects_compute_dispatch_without_bind_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_write_texture_out_of_range);
    TEST_SIMPLE(test_drp2_runtime_rejects_write_texture_layout_size_overflow);
    TEST_SIMPLE(test_drp2_runtime_rejects_copy_buffer_to_texture_usage);
    TEST_SIMPLE(test_drp2_runtime_rejects_copy_texture_to_texture_inside_pass);
    TEST_SIMPLE(test_drp2_runtime_validate_destroy_unused_buffer);
    TEST_SIMPLE(test_drp2_runtime_rejects_use_after_destroy);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_buffer_referenced_by_work);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_texture_referenced_by_work);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_submitted_render_pipeline);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_live_shader_module);
    TEST_SIMPLE(test_drp2_runtime_vklite_skeleton_create_destroy);
    TEST_SIMPLE(test_drp2_runtime_vklite_skeleton_execute_valid_stream);
    TEST_SIMPLE(test_drp2_runtime_vklite_skeleton_execute_invalid_stream);
    TEST_SIMPLE(test_drp2_runtime_vklite_skeleton_rejects_null_runtime);
    TEST_SIMPLE(test_drp2_runtime_frame_target_validation);
    TEST_SIMPLE(test_drp2_runtime_frame_lifecycle_edge_cases);
#if DVZ_DRP2_HAS_VKLITE
    TEST_SIMPLE(test_drp2_runtime_vklite_deferred_destroy_flush);
    TEST_SIMPLE(test_drp2_runtime_vklite_trims_destroyed_tail_slots);
#endif
    TEST_SIMPLE(test_drp2_runtime_download_buffer_rejects_out_of_range);
#if DVZ_DRP2_HAS_VKLITE
    TEST_SIMPLE(test_drp2_write_buffer_bytes_large_payload_executes);
    TEST_SIMPLE(test_drp2_runtime_vklite_executes_resource_commands);
    TEST_SIMPLE(test_drp2_runtime_vklite_writes_buffer_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_buffer_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_writes_texture_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_buffer_to_texture);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_texture_to_texture);
    TEST_SIMPLE(test_drp2_runtime_vklite_creates_glsl_shader_modules);
    TEST_SIMPLE(test_drp2_runtime_vklite_rejects_invalid_glsl_shader);
    TEST_SIMPLE(test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader);
    TEST_SIMPLE(test_drp2_runtime_vklite_destroy_after_partial_failure);
    TEST_SIMPLE(test_drp2_runtime_vklite_creates_render_pipeline);
    TEST_SIMPLE(test_drp2_runtime_vklite_reallocates_object_table_safely);
    TEST_SIMPLE(test_drp2_runtime_vklite_draws_render_pass);
    TEST_SIMPLE(test_drp2_runtime_vklite_samples_then_copies_texture);
#endif

    return 0;
}
