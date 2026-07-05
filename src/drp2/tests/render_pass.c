/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 render-pass tests                                                                       */
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
#include "test_drp2_helpers.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_begin_render_pass_clear_color_stored(TstContext* suite, const TstCase* item)
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
    AT(cmd->u.begin_render_pass.color_attachment_count == 1);
    AT(cmd->u.begin_render_pass.color_attachments[0].texture_id == 3);
    AC(cmd->u.begin_render_pass.clear_color[0], 0.2f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[1], 0.4f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[2], 0.6f, 1e-6f);
    AC(cmd->u.begin_render_pass.clear_color[3], 1.0f, 1e-6f);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_begin_render_pass_multi_color_attachments(TstContext* suite, const TstCase* item)
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
    AT(dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 4, 2, 3, 1));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, DVZ_FORMAT_R8G8B8A8_UNORM));
    AT(dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4));
    AT(dvz_drp2_stream_create_texture_2d(stream, 6, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 7));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 8, 7, 5, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 6, 1.0f, 1.0f, 1.0f, 1.0f, true));
    AT(dvz_drp2_stream_set_pipeline(stream, 8, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 8, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 8, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 8));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 7, 9));
    AT(dvz_drp2_stream_queue_submit(stream, 9, 10));

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 10);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AT(cmd->u.begin_render_pass.color_attachment_count == 2);
    AT(cmd->u.begin_render_pass.color_attachments[0].texture_id == 5);
    AT(cmd->u.begin_render_pass.color_attachments[1].texture_id == 6);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    char* json = dvz_drp2_stream_json(stream, "multi_attachment_json_test");
    ANN(json);
    AT(strstr(json, "\"texture_id\": 5") != NULL);
    AT(strstr(json, "\"texture_id\": 6") != NULL);
    AT(strstr(json, "\"r\": 1") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_begin_render_pass_attachment_ops(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 1, 2, 3, 0.2f, 0.4f, 0.6f, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        stream, 0, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
        stream, 0, DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth(stream, 0.5f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE, DVZ_DRP2_ATTACHMENT_STORE_STORE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));

    const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, 0);
    ANN(cmd);
    AT(cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS);
    AT(cmd->u.begin_render_pass.color_attachments[0].load_op == DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
    AT(
        cmd->u.begin_render_pass.color_attachments[0].store_op ==
        DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE);
    AT(
        cmd->u.begin_render_pass.color_attachments[0].access ==
        DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE);
    AT(!cmd->u.begin_render_pass.color_attachments[0].clear);
    AT(cmd->u.begin_render_pass.depth_load_op == DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE);
    AT(cmd->u.begin_render_pass.depth_store_op == DVZ_DRP2_ATTACHMENT_STORE_STORE);
    AT(cmd->u.begin_render_pass.depth_access == DVZ_DRP2_ATTACHMENT_ACCESS_READ);
    AT(cmd->u.begin_render_pass.depth_ops_explicit);

    char* json = dvz_drp2_stream_json(stream, "attachment_ops");
    ANN(json);
    AT(strstr(json, "\"load_op\": \"load\"") != NULL);
    AT(strstr(json, "\"store_op\": \"dont_care\"") != NULL);
    AT(strstr(json, "\"access\": \"read_write\"") != NULL);
    AT(strstr(json, "\"depth_stencil_attachment\"") != NULL);
    AT(strstr(json, "\"depth_load_op\": \"dont_care\"") != NULL);
    AT(strstr(json, "\"depth_store_op\": \"store\"") != NULL);
    AT(strstr(json, "\"access\": \"read\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_begin_render_pass_attachment_ops_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d(stream, 1, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_begin_render_pass(stream, 3, 2, 1));
    uint32_t invalid_load_op = 99;
    AT(
        sizeof(stream->commands[4].u.begin_render_pass.color_attachments[0].load_op) ==
        sizeof(invalid_load_op));
    dvz_memcpy(
        &stream->commands[4].u.begin_render_pass.color_attachments[0].load_op,
        sizeof(stream->commands[4].u.begin_render_pass.color_attachments[0].load_op),
        &invalid_load_op, sizeof(invalid_load_op));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_begin_render_pass_named_depth_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(!dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(!dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass(stream, 4, 3, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 0.5f));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d(stream, 1, 4, 4));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_begin_render_pass(stream, 4, 3, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 99, 1.0f));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d(stream, 1, 4, 4));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_begin_render_pass(stream, 4, 3, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d(stream, 1, 4, 4));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 8, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_begin_render_pass(stream, 4, 3, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_render_pipeline_rejects_depth_color_target(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_D32_SFLOAT));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R64_UINT));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_render_pass_rejects_attachment_format_classes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 2));
    AT(dvz_drp2_stream_begin_render_pass(stream, 3, 2, 1));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 4);

    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_begin_render_pass(stream, 4, 3, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    AT(result.command_index == 5);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_render_pipeline_attachment_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_R16_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 2, 0, 0, 0, 0, false));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 2));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R8G8B8A8_UNORM));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, DVZ_FORMAT_R16_SFLOAT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_R16G16B16A16_SFLOAT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 2));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R8G8B8A8_UNORM));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, DVZ_FORMAT_R16_SFLOAT));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 2, 0, 0, 0, 0, false));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_R16_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 1, 4, 4, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 2, 4, 4, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_shader_module(stream, 10, "vertex", "@vertex fn main() {}"));
    AT(dvz_drp2_stream_create_shader_module(stream, 11, "fragment", "@fragment fn main() {}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 20));
    AT(dvz_drp2_stream_begin_render_pass(stream, 21, 20, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 2, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 21, 12));
    result = dvz_drp2_validate_stream(stream);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_USAGE);
    dvz_drp2_stream_destroy(stream);

    return 0;
}



int test_drp2_stream_json_preserves_clear_color(TstContext* suite, const TstCase* item)
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
