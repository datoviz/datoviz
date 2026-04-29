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

    return 0;
}
