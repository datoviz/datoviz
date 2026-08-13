/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime validation tests                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_runtime.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/


int test_drp2_runtime_validate_render_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_render_stream();
    ANN(stream);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_render_state_inherited_across_passes(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));
    AT(dvz_drp2_stream_create_buffer(stream, 10, 16, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(stream, 11));
    AT(dvz_drp2_stream_create_uniform_bind_group(stream, 12, 11, 10, 0, 16));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
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



int test_drp2_runtime_validate_dynamic_viewport_scissor(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_render_stream();
    ANN(stream);

    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 5));
    AT(dvz_drp2_stream_set_viewport(stream, 21, 25.0f, 0.0f, 50.0f, 100.0f));
    AT(dvz_drp2_stream_set_scissor(stream, 21, 25.0f, 0.0f, 50.0f, 100.0f));
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


int test_drp2_runtime_rejects_draw_past_vertex_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint32_t stride = sizeof(float);
    uint32_t binding = 0;
    uint32_t location = 0;
    DvzFormat format = DVZ_FORMAT_R32_SFLOAT;
    uint32_t offset = 0;

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 2 * sizeof(float), DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = 4;
    pipeline.vertex_shader_module_id = 2;
    pipeline.fragment_shader_module_id = 3;
    pipeline.vertex_buffer_slots = 1;
    pipeline.topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    pipeline.binding_count = 1;
    pipeline.binding_strides = &stride;
    pipeline.attr_count = 1;
    pipeline.attr_bindings = &binding;
    pipeline.attr_locations = &location;
    pipeline.attr_formats = &format;
    pipeline.attr_offsets = &offset;
    AT(dvz_drp2_stream_create_render_pipeline(stream, &pipeline));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 7, 3, 1, 0, 0));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);
    AT(result.command_index == 11);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_rejects_draw_indexed_past_index_buffer(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_create_buffer(stream, 11, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX));
    AT(dvz_drp2_stream_create_buffer(stream, 12, 2 * sizeof(uint16_t), DVZ_DRP2_BUFFER_USAGE_INDEX));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 6));
    AT(dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5));
    AT(dvz_drp2_stream_set_pipeline(stream, 7, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 11, 0));
    AT(dvz_drp2_stream_set_index_buffer(stream, 7, 12, "uint16", 0));
    AT(dvz_drp2_stream_draw_indexed(stream, 7, 3, 1, 0, 0, 0));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);
    AT(result.command_index == 13);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_duplicate_id(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_failed_stream_does_not_commit_state(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_unknown_buffer_write(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 42, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 2);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_draw_without_vertex_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
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



int test_drp2_runtime_rejects_finish_with_open_pass(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_bad_readback_buffer(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_compute_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_compute_stream();
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



int test_drp2_runtime_rejects_dispatch_without_pipeline(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_dispatch_outside_compute_pass(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_wrong_pipeline_type(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 0));
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



int test_drp2_runtime_rejects_finish_with_open_compute_pass(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_indexed_render_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_indexed_render_stream();
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
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
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



int test_drp2_runtime_rejects_wrong_index_buffer_usage(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_indexed_render_stream();
    ANN(stream);

    // Build an otherwise equivalent stream where the index buffer has only VERTEX usage.
    dvz_drp2_stream_destroy(stream);
    stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
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



int test_drp2_runtime_validate_write_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d_base64(stream, 1, 0, 2, 1, 8, 1, "AAAAAAAAAAA="));

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



int test_drp2_runtime_validate_write_texture_3d_formats(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint8_t r8_values[3 * 2 * 2] = {0};
    uint16_t r16_values[2 * 2 * 2] = {0};
    uint32_t rg32_values[2 * 2 * 2 * 2] = {0};

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_3d_format_usage(
        stream, 1, 3, 2, 2, DVZ_FORMAT_R8_UNORM,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_3d_borrowed(
        stream, 1, 0, 0, 0, 0, 3, 2, 2, 3, 2, r8_values));
    AT(dvz_drp2_stream_create_texture_3d_format_usage(
        stream, 2, 2, 2, 2, DVZ_FORMAT_R16_UNORM,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_3d_borrowed(
        stream, 2, 0, 0, 0, 0, 2, 2, 2, 2 * sizeof(uint16_t), 2, r16_values));
    AT(dvz_drp2_stream_create_texture_3d_format_usage(
        stream, 3, 2, 2, 2, DVZ_FORMAT_R32G32_UINT,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_3d_borrowed(
        stream, 3, 0, 0, 0, 0, 2, 2, 2, 2 * 2 * sizeof(uint32_t), 2, rg32_values));

    const DvzDrp2Command* r8_texture = dvz_drp2_stream_get(stream, 2);
    ANN(r8_texture);
    AT(r8_texture->u.create_texture.depth == 2);
    AT(r8_texture->u.create_texture.format == DVZ_FORMAT_R8_UNORM);

    const DvzDrp2Command* r16_texture = dvz_drp2_stream_get(stream, 4);
    ANN(r16_texture);
    AT(r16_texture->u.create_texture.depth == 2);
    AT(r16_texture->u.create_texture.format == DVZ_FORMAT_R16_UNORM);

    const DvzDrp2Command* rg32_texture = dvz_drp2_stream_get(stream, 6);
    ANN(rg32_texture);
    AT(rg32_texture->u.create_texture.depth == 2);
    AT(rg32_texture->u.create_texture.format == DVZ_FORMAT_R32G32_UINT);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_write_texture_format_row_layout(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    uint16_t values[2 * 2 * 2] = {0};

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_3d_format_usage(
        stream, 1, 2, 2, 2, DVZ_FORMAT_R16_UNORM, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_3d_borrowed(
        stream, 1, 0, 0, 0, 0, 2, 2, 2, 3, 2, values));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_copy_buffer_to_texture(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_copy_texture_to_texture(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_texture_sampler_bind_group(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_sampler_filter(
        stream, 200, DVZ_DRP2_FILTER_NEAREST, DVZ_DRP2_FILTER_NEAREST));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 100));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 9000, "VERTEX", "@vertex fn main() -> @builtin(position) vec4f { return vec4f(); }"));
    AT(dvz_drp2_stream_create_shader_module(
        stream, 9001, "FRAGMENT",
        "@group(0) @binding(0) var source: texture_2d<f32>; @group(0) @binding(1) var samp: "
        "sampler; @fragment fn main() -> @location(0) vec4f { return textureSample(source, samp, "
        "vec2f(0.5)); }"));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
        stream, 10, 9000, 9001, 0, 100));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d_base64(stream, 2, 0, 2, 2, 8, 2, "AAAAAAAAAAAAAAAAAAAAAA=="));
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
    AT(strstr(json, "\"mag_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"min_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroup\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [100]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_validate_generic_bind_group_slots(TstContext* suite, const TstCase* item)
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
    AT(drp2_test_create_render_pipeline(stream, 10, 9000, 9001, 0));
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



int test_drp2_runtime_rejects_bind_group_entry_mismatch(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_bind_group_dynamic_offsets(TstContext* suite, const TstCase* item)
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
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
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
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
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



int test_drp2_runtime_validate_bind_group_after_table_growth(TstContext* suite, const TstCase* item)
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



/**
 * Validate stable-id buffer and sampler recreation for an existing bind group.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_validate_recreate_bind_group_resources(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2BindGroupLayoutEntry layout_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    DvzDrp2BindGroupEntry bind_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
            .resource_id = 2,
            .offset = 8,
            .size = 16,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 3,
        },
    };

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(setup, 2, 32, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_sampler(setup, 3));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(setup, 4, 2, layout_entries));
    AT(dvz_drp2_stream_create_bind_group_entries(setup, 5, 4, 2, bind_entries));
    AT(dvz_drp2_stream_create_shader_module(setup, 6, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(setup, 7, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(setup, 8, 6, 7, 0, 4));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 9, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 10));
    AT(dvz_drp2_stream_begin_render_pass(setup, 11, 10, 9));
    AT(dvz_drp2_stream_set_pipeline(setup, 11, 8));
    AT(dvz_drp2_stream_set_bind_group(setup, 11, 0, 5));
    AT(dvz_drp2_stream_draw(setup, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(setup, 11));
    AT(dvz_drp2_stream_finish_command_encoder(setup, 10, 12));
    AT(dvz_drp2_stream_queue_submit(setup, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    DvzDrp2CommandStream* too_small = dvz_drp2_stream();
    ANN(too_small);
    AT(dvz_drp2_stream_create_buffer(too_small, 2, 8, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    result = dvz_drp2_runtime_execute(runtime, too_small);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);

    DvzDrp2CommandStream* recreated = dvz_drp2_stream();
    ANN(recreated);
    AT(dvz_drp2_stream_create_buffer(recreated, 2, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM));
    AT(dvz_drp2_stream_create_sampler(recreated, 3));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        recreated, 19, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(recreated, 20));
    AT(dvz_drp2_stream_begin_render_pass(recreated, 21, 20, 19));
    AT(dvz_drp2_stream_set_pipeline(recreated, 21, 8));
    AT(dvz_drp2_stream_set_bind_group(recreated, 21, 0, 5));
    AT(dvz_drp2_stream_draw(recreated, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(recreated, 21));
    AT(dvz_drp2_stream_finish_command_encoder(recreated, 20, 22));
    AT(dvz_drp2_stream_queue_submit(recreated, 22, 23));

    result = dvz_drp2_runtime_execute(runtime, recreated);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(recreated);
    dvz_drp2_stream_destroy(too_small);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_reuses_submitted_transient_ids(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_create_shader_module(setup, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(setup, 2, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(setup, 3, 1, 2, 0));
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

    DvzDrp2CommandStream* resize = dvz_drp2_stream();
    ANN(resize);
    AT(dvz_drp2_stream_create_texture_2d_usage(
        resize, 4, 4, 3, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    result = dvz_drp2_runtime_execute(runtime, resize);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    DvzDrp2CommandStream* resized_frame = dvz_drp2_stream();
    ANN(resized_frame);
    AT(dvz_drp2_stream_begin_command_encoder(resized_frame, 10));
    AT(dvz_drp2_stream_begin_render_pass(resized_frame, 11, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(resized_frame, 11, 3));
    AT(dvz_drp2_stream_draw(resized_frame, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(resized_frame, 11));
    AT(dvz_drp2_stream_finish_command_encoder(resized_frame, 10, 12));
    AT(dvz_drp2_stream_queue_submit(resized_frame, 12, 13));
    result = dvz_drp2_runtime_execute(runtime, resized_frame);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(resized_frame);
    dvz_drp2_stream_destroy(resize);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}


int test_drp2_runtime_registers_external_buffer_semantic(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_create_shader_module(setup, 1, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(setup, 2, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(setup, 3, 1, 2, 1));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 4, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);

    DvzDrp2ExternalBufferDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = NULL,
        .size = 64,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    DvzDrp2ExternalBufferDesc invalid_abi = desc;
    invalid_abi.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_drp2_runtime_register_external_buffer(runtime, 12, &invalid_abi));
    invalid_abi = desc;
    invalid_abi.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_drp2_runtime_register_external_buffer(runtime, 13, &invalid_abi));
    AT(dvz_drp2_runtime_register_external_buffer(runtime, 11, &desc));
    AT(!dvz_drp2_runtime_register_external_buffer(runtime, 11, &desc));

    DvzDrp2CommandStream* frame = dvz_drp2_stream();
    ANN(frame);
    AT(dvz_drp2_stream_begin_command_encoder(frame, 10));
    AT(dvz_drp2_stream_begin_render_pass(frame, 12, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(frame, 12, 3));
    AT(dvz_drp2_stream_set_vertex_buffer(frame, 12, 0, 11, 0));
    AT(dvz_drp2_stream_draw(frame, 12, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(frame, 12));
    AT(dvz_drp2_stream_finish_command_encoder(frame, 10, 13));
    AT(dvz_drp2_stream_queue_submit(frame, 13, 14));

    result = dvz_drp2_runtime_execute(runtime, frame);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(frame);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_external_buffer_timeline_semantic(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_runtime_execute(runtime, setup).ok);

    DvzDrp2ExternalBufferDesc buffer = dvz_drp2_external_buffer_desc();
    buffer.size = 16;
    buffer.usage = DVZ_DRP2_BUFFER_USAGE_COPY_SRC;
    AT(dvz_drp2_runtime_register_external_buffer(runtime, 1, &buffer));

    DvzDrp2ExternalBufferTimelineDesc timeline =
        dvz_drp2_external_buffer_timeline_desc();
    timeline.wait_value = 4;
    timeline.signal_value = 5;
    DvzDrp2ExternalBufferTimelineDesc invalid = timeline;
    invalid.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &invalid));
    invalid = timeline;
    invalid.signal_value = invalid.wait_value;
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &invalid));
    AT(dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &timeline));
    AT(dvz_drp2_runtime_external_buffer_timeline_pending(runtime, 1));
    AT(!dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &timeline));

    DvzDrp2CommandStream* frame = dvz_drp2_stream();
    ANN(frame);
    AT(dvz_drp2_stream_begin_command_encoder(frame, 3));
    AT(dvz_drp2_stream_copy_buffer_to_texture(frame, 3, 1, 0, 2, 2, 1, 8, 1));
    AT(dvz_drp2_stream_finish_command_encoder(frame, 3, 4));
    AT(dvz_drp2_stream_queue_submit(frame, 4, 5));
    AT(dvz_drp2_runtime_execute(runtime, frame).ok);
    AT(!dvz_drp2_runtime_external_buffer_timeline_pending(runtime, 1));

    AT(!dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &timeline));
    timeline.wait_value = 6;
    timeline.signal_value = 7;
    AT(dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &timeline));

    dvz_drp2_stream_destroy(frame);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



int test_drp2_runtime_validate_compute_storage_bind_group(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_write_buffer_base64(stream, 2, 0, 36, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
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
    AT(strstr(json, "\"binding\": 0, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read\"") != NULL);
    AT(strstr(json, "\"binding\": 1, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read_write\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_validate_destroy_unused_bind_group(TstContext* suite, const TstCase* item)
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
    TstContext* suite, const TstCase* item)
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
    TstContext* suite, const TstCase* item)
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
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
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
    TstContext* suite, const TstCase* item)
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
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
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
    TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_write_texture_out_of_range(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_texture_2d_base64(stream, 1, 0, 3, 1, 12, 1, "AAAAAAAAAAAAAAAA"));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OUT_OF_RANGE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_rejects_write_texture_layout_size_overflow(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_3d(stream, 1, 1, 1, UINT32_MAX));
    AT(dvz_drp2_stream_write_texture_3d_base64(
        stream, 1, 0, 0, 0, 0, 1, 1, UINT32_MAX, UINT32_MAX, UINT32_MAX, ""));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 3);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_copy_buffer_to_texture_usage(TstContext* suite, const TstCase* item)
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
    TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_validate_destroy_unused_buffer(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_use_after_destroy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(stream, 1, 64, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA=="));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_buffer_referenced_by_work(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_allows_destroy_buffer_after_submit(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_queue_submit(stream, 4, 5));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_texture_referenced_by_work(TstContext* suite, const TstCase* item)
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



int test_drp2_runtime_rejects_destroy_submitted_render_pipeline(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = drp2_test_valid_render_stream();
    ANN(stream);
    AT(dvz_drp2_stream_destroy_render_pipeline(stream, 4));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 16);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_rejects_destroy_live_shader_module(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 0));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}
