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

#include "_assertions.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "testing.h"

#if DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"

bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
#endif



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
    TEST_SIMPLE(test_drp2_runtime_validate_render_stream);
    TEST_SIMPLE(test_drp2_runtime_rejects_duplicate_id);
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
    TEST_SIMPLE(test_drp2_runtime_validate_bind_group_after_table_growth);
    TEST_SIMPLE(test_drp2_runtime_validate_compute_storage_bind_group);
    TEST_SIMPLE(test_drp2_runtime_validate_destroy_unused_bind_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_live_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_layout_used_by_pipeline);
    TEST_SIMPLE(test_drp2_runtime_rejects_destroy_bind_group_referenced_by_work);
    TEST_SIMPLE(test_drp2_runtime_rejects_compute_dispatch_without_bind_group);
    TEST_SIMPLE(test_drp2_runtime_rejects_write_texture_out_of_range);
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
#if DVZ_DRP2_HAS_VKLITE
    TEST_SIMPLE(test_drp2_runtime_vklite_executes_resource_commands);
    TEST_SIMPLE(test_drp2_runtime_vklite_writes_buffer_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_buffer_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_writes_texture_contents);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_buffer_to_texture);
    TEST_SIMPLE(test_drp2_runtime_vklite_copies_texture_to_texture);
    TEST_SIMPLE(test_drp2_runtime_vklite_creates_glsl_shader_modules);
    TEST_SIMPLE(test_drp2_runtime_vklite_creates_render_pipeline);
    TEST_SIMPLE(test_drp2_runtime_vklite_reallocates_object_table_safely);
    TEST_SIMPLE(test_drp2_runtime_vklite_draws_render_pass);
#endif

    return 0;
}
