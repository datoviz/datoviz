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

#include <string.h>

#include "_assertions.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

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

    return 0;
}
