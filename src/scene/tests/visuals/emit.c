/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual emission tests                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_point_emit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    /* Build a minimal scene: one figure, one full-frame panel, one point visual. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    int rc = dvz_visual_set_data(visual, "position", positions, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "color", colors, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "size", sizes, 3);
    AT(rc == 0);

    rc = dvz_panel_add_visual(panel, visual, NULL);
    AT(rc == 0);

    /* Emit the DRP2 command stream. */
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.max_vertex_buffers = 8;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);

    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_external_unorm_target_encodes_srgb(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{128, 160, 192, 255}};
    float sizes[1] = {16.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.max_vertex_buffers = 8;
    caps.max_bind_groups = 4;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 4242;
    emit_cfg.color_target_format = DVZ_FORMAT_B8G8R8A8_UNORM;
    emit_cfg.target_width = 64;
    emit_cfg.target_height = 64;

    DvzDrp2CommandStream* stream =
        _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool final_target_created = false;
    bool found_intermediate = false;
    bool found_scene_pass = false;
    bool found_encode_pass = false;
    bool found_encode_shader = false;
    uint64_t intermediate_id = 0;
    uint32_t scene_pass_index = UINT32_MAX;
    uint32_t encode_pass_index = UINT32_MAX;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;

        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            if (cmd->u.create_texture.id == emit_cfg.color_target_id)
                final_target_created = true;
            if (cmd->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0)
            {
                found_intermediate = true;
                intermediate_id = cmd->u.create_texture.id;
            }
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
            cmd->u.create_shader_module.code != NULL &&
            (strstr(cmd->u.create_shader_module.code, "linearToSrgb") != NULL ||
             strstr(cmd->u.create_shader_module.code, "linear_to_srgb") != NULL))
        {
            found_encode_shader = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            const DvzDrp2ColorAttachment* attachment =
                &cmd->u.begin_render_pass.color_attachments[0];
            if (cmd->u.begin_render_pass.color_attachment_count > 0 &&
                attachment->texture_id == intermediate_id)
            {
                found_scene_pass = true;
                scene_pass_index = i;
            }
            if (cmd->u.begin_render_pass.color_attachment_count > 0 &&
                attachment->texture_id == emit_cfg.color_target_id)
            {
                found_encode_pass = true;
                encode_pass_index = i;
            }
        }
    }

    AT(!final_target_created);
    AT(found_intermediate);
    AT(found_scene_pass);
    AT(found_encode_pass);
    AT(found_encode_shader);
    AT(scene_pass_index < encode_pass_index);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_external_unorm_target_legacy_srgb_blend(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{128, 160, 192, 128}};
    float sizes[1] = {16.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 8;
    caps.max_bind_groups = 4;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.color_pipeline = DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 4242;
    emit_cfg.color_target_format = DVZ_FORMAT_B8G8R8A8_UNORM;
    emit_cfg.target_width = 64;
    emit_cfg.target_height = 64;

    DvzDrp2CommandStream* stream =
        _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool final_target_created = false;
    bool found_intermediate = false;
    bool found_scene_pass_on_final = false;
    bool found_encode_shader = false;
    bool found_legacy_define = false;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;

        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            if (cmd->u.create_texture.id == emit_cfg.color_target_id)
                final_target_created = true;
            if (cmd->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0)
                found_intermediate = true;
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
            cmd->u.create_shader_module.code != NULL)
        {
            if (strstr(cmd->u.create_shader_module.code, "linearToSrgb") != NULL ||
                strstr(cmd->u.create_shader_module.code, "linear_to_srgb") != NULL)
                found_encode_shader = true;
            if (strstr(cmd->u.create_shader_module.code, "DVZ_LEGACY_SRGB_BLEND") != NULL)
                found_legacy_define = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            const DvzDrp2ColorAttachment* attachment =
                &cmd->u.begin_render_pass.color_attachments[0];
            if (cmd->u.begin_render_pass.color_attachment_count > 0 &&
                attachment->texture_id == emit_cfg.color_target_id)
                found_scene_pass_on_final = true;
        }
    }

    AT(!final_target_created);
    AT(!found_intermediate);
    AT(found_scene_pass_on_final);
    AT(!found_encode_shader);
    AT(found_legacy_define);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_path_emit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.75f, -0.25f, 0.0f},
        {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},
        {0.75f, 0.25f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {255, 255, 0, 255},
        {0, 255, 255, 255},
        {0, 128, 255, 255},
    };

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP);
            AT(cmd->u.create_render_pipeline.binding_count == 2);
            AT(cmd->u.create_render_pipeline.attr_count == 2);
            break;
        }
    }
    AT(found_pipeline);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_multi_item_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    vec3 positions[2] = {{-0.4f, 0.0f, 0.0f}, {+0.4f, 0.0f, 0.0f}};
    vec2 extents[2] = {{0.3f, 0.4f}, {0.2f, 0.5f}};
    vec4 tex_rects[2] = {{0.0f, 0.0f, 0.5f, 1.0f}, {0.5f, 0.0f, 1.0f, 1.0f}};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "extent", extents, 2) == 0);
    AT(dvz_visual_set_data(visual, "tex_rect", tex_rects, 2) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t position_buffer_id = 0;
    uint64_t uv_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_position_upload = false;
    bool found_uv_upload = false;
    bool found_uv_values = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline =
                cmd->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
                cmd->u.create_render_pipeline.bind_group_layout_count == 2;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 1)
                uv_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 12 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);
    AT(uv_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        if (cmd->u.write_buffer.buffer_id == position_buffer_id)
            found_position_upload = cmd->u.write_buffer.size == 12 * 3 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == uv_buffer_id)
        {
            found_uv_upload = cmd->u.write_buffer.size == 12 * 2 * sizeof(float);
            if (found_uv_upload && cmd->u.write_buffer.data_raw != NULL)
            {
                const float* uv = (const float*)cmd->u.write_buffer.data_raw;
                const float expected_uv[24] = {
                    0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f,
                    0.0f, 1.0f, 0.5f, 1.0f, 0.5f, 0.0f, 0.5f, 1.0f,
                    1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f,
                };
                found_uv_values = true;
                for (uint32_t j = 0; j < 24; j++)
                    found_uv_values = found_uv_values && uv[j] == expected_uv[j];
            }
        }
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_position_upload);
    AT(found_uv_upload);
    AT(found_uv_values);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify pixel-anchored image visuals emit fixed-size generated quads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_pixel_anchor_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    vec3 positions[1] = {{10.0f, 20.0f, 0.5f}};
    vec2 extents[1] = {{80.0f, 40.0f}};
    vec2 anchors[1] = {{-1.0f, -1.0f}};
    vec4 tex_rects[1] = {{0.25f, 0.25f, 0.75f, 0.75f}};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position_px", positions, 1) == 0);
    AT(dvz_visual_set_data(visual, "extent_px", extents, 1) == 0);
    AT(dvz_visual_set_data(visual, "anchor", anchors, 1) == 0);
    AT(dvz_visual_set_data(visual, "tex_rect", tex_rects, 1) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, visual,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t position_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_vertex_shader = false;
    bool found_position_values = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_img_pxw") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.vertex_buffer_slots == 2);
                AT(cmd->u.create_render_pipeline.binding_count == 2);
                AT(cmd->u.create_render_pipeline.attr_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_count == 2);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            found_vertex_shader =
                found_vertex_shader ||
                (strcmp(cmd->u.create_shader_module.stage, "VERTEX") == 0 &&
                 strcmp(cmd->u.create_shader_module.format, "wgsl") == 0 &&
                 cmd->u.create_shader_module.code != NULL &&
                 strstr(cmd->u.create_shader_module.code, "image_pixel_anchor") != NULL);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 6 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.buffer_id != position_buffer_id ||
            cmd->u.write_buffer.data_raw == NULL)
            continue;
        const float* pos = (const float*)cmd->u.write_buffer.data_raw;
        const float expected[18] = {
            10.0f, 20.0f, 0.5f, 10.0f, 60.0f, 0.5f, 90.0f, 20.0f, 0.5f,
            90.0f, 20.0f, 0.5f, 10.0f, 60.0f, 0.5f, 90.0f, 60.0f, 0.5f,
        };
        found_position_values = cmd->u.write_buffer.size == sizeof(expected);
        for (uint32_t j = 0; j < 18 && found_position_values; j++)
            found_position_values = pos[j] == expected[j];
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_vertex_shader);
    AT(found_position_values);

    char* json = dvz_drp2_stream_json(stream, "scene_image_pixel_anchor_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "image_pixel_anchor") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [") != NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify glyph visuals emit an MSDF-capable textured triangle pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_glyph_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_glyph(scene, 0);
    ANN(visual);

    vec3 positions[6] = {{0.0f, 0.0f, 0.0f}};
    vec4 bounds[6] = {
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
    };
    vec4 texcoords[6] = {
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
    };
    DvzColor colors[6] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    float angles[6] = {0};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 255, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 6) == 0);
    AT(dvz_visual_set_data(visual, "bounds", bounds, 6) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 6) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 6) == 0);
    AT(dvz_visual_set_data(visual, "angle", angles, 6) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_glyphg"));

    uint64_t position_buffer_id = 0;
    uint64_t bounds_buffer_id = 0;
    uint64_t uv_buffer_id = 0;
    uint64_t color_buffer_id = 0;
    uint64_t angle_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_texture_bind = false;
    bool found_glyph_layout = false;
    bool found_glyph_bind_group = false;
    bool found_glyph_params_write = false;
    uint64_t glyph_params_buffer_id = 0;
    bool found_position_upload = false;
    bool found_bounds_upload = false;
    bool found_uv_upload = false;
    bool found_color_upload = false;
    bool found_angle_upload = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline =
                cmd->u.create_render_pipeline.vertex_buffer_slots == 5 &&
                cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
                cmd->u.create_render_pipeline.binding_count == 5 &&
                cmd->u.create_render_pipeline.attr_count == 5 &&
                cmd->u.create_render_pipeline.bind_group_layout_count == 2;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        {
            const char* label =
                dvz_drp2_stream_label(stream, cmd->u.create_bind_group_layout.id);
            if (label != NULL && strcmp(label, "_bgl_glyph") == 0)
            {
                found_glyph_layout =
                    cmd->u.create_bind_group_layout.entry_count == 3 &&
                    cmd->u.create_bind_group_layout.entries[0].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
                    cmd->u.create_bind_group_layout.entries[1].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_SAMPLER &&
                    cmd->u.create_bind_group_layout.entries[2].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_bind_group.id);
            if (label != NULL && strncmp(label, "_bg_glyph_", 10) == 0)
            {
                found_glyph_bind_group =
                    cmd->u.create_bind_group.entry_count == 3 &&
                    cmd->u.create_bind_group.entries[2].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                    cmd->u.create_bind_group.entries[2].size == 4 * sizeof(float);
                glyph_params_buffer_id = cmd->u.create_bind_group.entries[2].resource_id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 1)
                bounds_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 2)
                uv_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 3)
                color_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 4)
                angle_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_texture_bind = found_texture_bind || cmd->u.set_bind_group.slot == 1;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 6 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);
    AT(bounds_buffer_id != 0);
    AT(uv_buffer_id != 0);
    AT(color_buffer_id != 0);
    AT(angle_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        if (cmd->u.write_buffer.buffer_id == glyph_params_buffer_id)
            found_glyph_params_write = cmd->u.write_buffer.size == 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == position_buffer_id)
            found_position_upload = cmd->u.write_buffer.size == 6 * 3 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == bounds_buffer_id)
            found_bounds_upload = cmd->u.write_buffer.size == 6 * 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == uv_buffer_id)
            found_uv_upload = cmd->u.write_buffer.size == 6 * 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == color_buffer_id)
            found_color_upload = cmd->u.write_buffer.size == 6 * sizeof(DvzColor);
        if (cmd->u.write_buffer.buffer_id == angle_buffer_id)
            found_angle_upload = cmd->u.write_buffer.size == 6 * sizeof(float);
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_texture_bind);
    AT(found_glyph_layout);
    AT(found_glyph_bind_group);
    AT(found_glyph_params_write);
    AT(found_position_upload);
    AT(found_bounds_upload);
    AT(found_uv_upload);
    AT(found_color_upload);
    AT(found_angle_upload);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_image_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"format\": \"glsl\"") == NULL);
    AT(strstr(json, "texture_2d<f32>") != NULL);
    AT(strstr(json, "textureSample") != NULL);
    AT(strstr(json, "sampled_texture_color_to_linear") != NULL);
    AT(strstr(json, "\"color_role\": \"srgb_color\"") != NULL);
    AT(strstr(json, "@group(1) @binding(0)") != NULL);
    AT(strstr(json, "@group(1) @binding(1)") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [") != NULL);
    AT(strstr(json, "\"vertex_buffers\": [") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_image_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_image_wgsl_from_c.json") == 0);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_sampling_nearest_emits_sampler_filters(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);

    AT(dvz_image_set_sampling(image, DVZ_IMAGE_SAMPLING_NEAREST) == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_image_set_sampling(point, DVZ_IMAGE_SAMPLING_NEAREST) == -1);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_image_set_sampling(image, (DvzImageSampling)99) == -1);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_rgba8(image, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_image_nearest_sampler_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"mag_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"min_filter\": \"nearest\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_linear_color_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    DvzColor pixels[4] = {
        {128, 128, 128, 255},
        {128, 128, 128, 255},
        {128, 128, 128, 255},
        {128, 128, 128, 255},
    };

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .color_role = DVZ_COLOR_ROLE_LINEAR_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = pixels,
                   .bytes_per_row = 2 * sizeof(DvzColor),
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_field(visual, "field", field));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_image_linear_color_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"format\": \"glsl\"") == NULL);
    AT(strstr(json, "sampled_texture_color_to_linear") != NULL);
    AT(strstr(json, "\"color_role\": \"linear_color\"") != NULL);
    AT(strstr(json, "\"color_role\": \"srgb_color\"") == NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Return whether a stream creates a texture with the expected format and extent.
 *
 * @param stream the emitted command stream
 * @param format expected texture format token
 * @param width expected texture width
 * @param height expected texture height
 * @return whether a matching texture command was found
 */
static bool _stream_has_texture_format(
    const DvzDrp2CommandStream* stream, DvzFormat format, uint32_t width, uint32_t height)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        if (cmd->u.create_texture.format == format && cmd->u.create_texture.width == width &&
            cmd->u.create_texture.height == height && cmd->u.create_texture.depth == 1)
            return true;
    }
    return false;
}



/**
 * Return whether a stream uploads one texture region with the expected row layout.
 *
 * @param stream the emitted command stream
 * @param width expected upload width
 * @param height expected upload height
 * @param bytes_per_row expected row byte count
 * @return whether a matching upload command was found
 */
static bool _stream_has_texture_upload(
    const DvzDrp2CommandStream* stream, uint32_t width, uint32_t height, uint64_t bytes_per_row)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        if (cmd->u.write_texture.width == width && cmd->u.write_texture.height == height &&
            cmd->u.write_texture.depth == 1 && cmd->u.write_texture.bytes_per_row == bytes_per_row)
            return true;
    }
    return false;
}



/**
 * Log diagnostics before a focused labels test fails.
 *
 * @param report diagnostic report
 */
static void _labels_log_diagnostics(const DvzDiagnosticReport* report)
{
    ANN(report);
    uint32_t count = dvz_diagnostic_report_count(report);
    for (uint32_t i = 0; i < count; i++)
    {
        const char* message = dvz_diagnostic_report_get(report, i);
        if (message != NULL)
            log_error("%s", message);
    }
}



/**
 * Log render pipeline labels in a labels stream test.
 *
 * @param stream the emitted command stream
 */
static void _labels_log_pipeline_labels(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        log_error("pipeline: %s", label != NULL ? label : "(none)");
    }
}


/**
 * Return whether a stream contains the labels texture/sampler/params bind-group layout.
 *
 * @param stream the emitted command stream
 * @return whether the labels layout is present
 */
static bool _labels_stream_has_params_layout(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
            continue;
        if (cmd->u.create_bind_group_layout.entry_count != 3)
            continue;
        const DvzDrp2BindGroupLayoutEntry* entries = cmd->u.create_bind_group_layout.entries;
        if (entries[0].binding == 0 &&
            entries[0].binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
            entries[1].binding == 1 &&
            entries[1].binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER &&
            entries[2].binding == 2 &&
            entries[2].binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER)
        {
            return true;
        }
    }
    return false;
}


/**
 * Return whether a stream writes the expected labels presentation uniform.
 *
 * @param stream the emitted command stream
 * @return whether the labels uniform write is present
 */
static bool _labels_stream_has_params_write(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.size != sizeof(DvzSceneLabelsUniform))
        {
            continue;
        }
        const DvzSceneLabelsUniform* uniform =
            (const DvzSceneLabelsUniform*)cmd->u.write_buffer.data_raw;
        if (uniform == NULL)
            continue;
        if (uniform->ids[0] == 0u && uniform->ids[1] == 17u &&
            uniform->params[0] ==
                (DVZ_SCENE_LABELS_FLAG_SELECTED | DVZ_SCENE_LABELS_FLAG_BOUNDARY) &&
            uniform->params[1] == 123u && uniform->params[2] == 2u &&
            fabsf(uniform->floats[0] - 0.55f) < 1e-6f &&
            fabsf(uniform->floats[1] - 2.0f) < 1e-6f &&
            uniform->hidden_ids[0][0] == (uint32_t)(int32_t)-7 &&
            uniform->hidden_ids[0][1] == 1009u)
        {
            return true;
        }
    }
    return false;
}



/**
 * Build one retained labels visual bound to a categorical scale.
 *
 * @param scene owning scene
 * @param format integer field format
 * @param values field payload
 * @param bytes_per_row field row byte count
 * @param out_figure output configured figure
 * @return 0 on success
 */
static int _labels_emit_figure(
    DvzScene* scene, DvzFieldFormat format, const void* values, uint64_t bytes_per_row,
    DvzFigure** out_figure)
{
    ANN(scene);
    ANN(values);
    ANN(out_figure);

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    DvzCategoryId hidden[2] = {-7, 1009};
    DvzColor boundary = {255, 244, 64, 245};
    AT(dvz_labels_set_opacity(labels, 0.55f) == 0);
    AT(dvz_labels_set_selected(labels, 17) == 0);
    AT(dvz_labels_set_hidden(labels, hidden, 2) == 0);
    AT(dvz_labels_set_boundary(labels, true, 2.0f, boundary) == 0);
    AT(dvz_labels_set_fallback_seed(labels, 123) == 0);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extents[1] = {{2.0f, 2.0f}};
    AT(dvz_visual_set_data(labels, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(labels, "extent", extents, 1) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = format,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = values,
                   .bytes_per_row = bytes_per_row,
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(labels, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[3] = {
        {.category_id = -7, .order = 0, .label = "negative", .color = {200, 40, 40, 180}},
        {.category_id = 17, .order = 1, .label = "cell 17", .color = {40, 180, 80, 180}},
        {.category_id = 4000000000LL, .order = 2, .label = "large", .color = {40, 80, 220, 180}},
    };
    AT(dvz_scale_set_categories(scale, categories, 3));
    AT(dvz_visual_set_scale(labels, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    *out_figure = figure;
    return 0;
}



int test_scene_labels_emit_signed_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintg"));
    AT(!_stream_has_render_pipeline_label(stream, "_pipe_imgg"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, DVZ_FORMAT_R32_SINT, 2, 2));
    AT(_stream_has_texture_upload(stream, 2, 2, 2 * sizeof(int32_t)));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_signed_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"spirv\"") != NULL ||
       strstr(json, "\"format\": \"glsl\"") != NULL);
    AT(strstr(json, "\"mag_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"min_filter\": \"nearest\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_labels_emit_unsigned_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    uint32_t values[4] = {0, 17, 1009, 4000000000u};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_UINT, values, 2 * sizeof(uint32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_uintg"));
    AT(!_stream_has_render_pipeline_label(stream, "_pipe_imgg"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, DVZ_FORMAT_R32_UINT, 2, 2));
    AT(_stream_has_texture_upload(stream, 2, 2, 2 * sizeof(uint32_t)));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_unsigned_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"spirv\"") != NULL ||
       strstr(json, "\"format\": \"glsl\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_labels_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    if (!_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintw"))
        _labels_log_pipeline_labels(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintw"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, DVZ_FORMAT_R32_SINT, 2, 2));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "texture_2d<i32>") != NULL);
    AT(strstr(json, "textureLoad") != NULL);
    AT(strstr(json, "@group(1) @binding(0)") != NULL);
    AT(strstr(json, "@group(1) @binding(2)") != NULL);
    AT(strstr(json, "LabelsParams") != NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_emit_uses_common_and_texture_sets(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_common_layout = false;
    bool found_common_bind = false;
    bool found_viewport_write = false;
    bool found_texture_bind = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.bind_group_layout_count >= 2);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[0] != 0);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        {
            if (cmd->u.create_bind_group_layout.entry_count == 2 &&
                cmd->u.create_bind_group_layout.entries[0].binding == 0 &&
                cmd->u.create_bind_group_layout.entries[1].binding == 1 &&
                cmd->u.create_bind_group_layout.entries[0].binding_type ==
                    DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                cmd->u.create_bind_group_layout.entries[1].binding_type ==
                    DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER)
            {
                found_common_layout = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            if (cmd->u.set_bind_group.slot == 0)
                found_common_bind = true;
            if (cmd->u.set_bind_group.slot == 1)
                found_texture_bind = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
                 cmd->u.write_buffer.size == sizeof(DvzSceneViewportUniform))
        {
            const DvzSceneViewportUniform* viewport =
                (const DvzSceneViewportUniform*)cmd->u.write_buffer.data_raw;
            ANN(viewport);
            if (fabsf(viewport->width - 64.0f) <= 1e-6f &&
                fabsf(viewport->height - 64.0f) <= 1e-6f)
            {
                AC(viewport->x, 0.0f, 1e-6f);
                AC(viewport->y, 0.0f, 1e-6f);
                found_viewport_write = true;
            }
        }
    }
    AT(found_pipeline);
    AT(found_common_layout);
    AT(found_common_bind);
    AT(found_viewport_write);
    AT(found_texture_bind);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure scene visuals keep shared data in set 0 and visual-specific data in set 1.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_visual_common_binding_layout_order(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    vec3 point_pos[3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.7f, -0.8f, 0.0f}, {-0.75f, -0.7f, 0.0f},
    };
    DvzColor point_color[3] = {
        {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255},
    };
    float point_size[3] = {8.0f, 8.0f, 8.0f};
    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    AT(dvz_visual_set_data(point, "position", point_pos, 3) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 3) == 0);
    AT(dvz_visual_set_data(point, "size", point_size, 3) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    vec3 prim_pos[3] = {
        {-0.5f, -0.8f, 0.0f}, {-0.3f, -0.8f, 0.0f}, {-0.4f, -0.6f, 0.0f},
    };
    DvzColor prim_color[3] = {
        {255, 255, 0, 255}, {255, 255, 0, 255}, {255, 255, 0, 255},
    };
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(primitive);
    AT(dvz_visual_set_data(primitive, "position", prim_pos, 3) == 0);
    AT(dvz_visual_set_data(primitive, "color", prim_color, 3) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);

    vec3 path_pos[4] = {
        {-0.2f, -0.8f, 0.0f}, {-0.1f, -0.7f, 0.0f},
        {0.0f, -0.8f, 0.0f},  {0.1f, -0.7f, 0.0f},
    };
    DvzColor path_color[4] = {
        {0, 255, 255, 255}, {0, 255, 255, 255},
        {0, 255, 255, 255}, {0, 255, 255, 255},
    };
    DvzVisual* path = dvz_path(scene, 0);
    ANN(path);
    AT(dvz_visual_set_data(path, "position", path_pos, 4) == 0);
    AT(dvz_visual_set_data(path, "color", path_color, 4) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);

    vec3 image_pos[4] = {
        {0.2f, -0.8f, 0.0f}, {0.2f, -0.6f, 0.0f},
        {0.4f, -0.8f, 0.0f}, {0.4f, -0.6f, 0.0f},
    };
    vec2 image_uv[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 255, sizeof(pixels));
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", image_uv, 4) == 0);
    AT(dvz_visual_set_texture_rgba8(image, (const uint8_t*)pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    vec3 mesh_pos[4] = {
        {0.55f, -0.8f, 0.0f}, {0.55f, -0.6f, 0.0f},
        {0.75f, -0.8f, 0.0f}, {0.75f, -0.6f, 0.0f},
    };
    vec3 mesh_normal[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_index[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_index, sizeof(mesh_index)));
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    AT(dvz_visual_set_data(mesh, "position", mesh_pos, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normal, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t common_layout_id = _stream_scene_common_layout_id(stream);
    AT(common_layout_id != 0);

    bool found_point_pipeline = false;
    bool found_primitive_pipeline = false;
    bool found_path_pipeline = false;
    bool found_image_pipeline = false;
    bool found_lit_mesh_pipeline = false;
    bool found_common_bind = false;
    bool found_visual_bind = false;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const uint32_t topology = cmd->u.create_render_pipeline.topology;
            const uint32_t slots = cmd->u.create_render_pipeline.vertex_buffer_slots;
            const uint32_t layout_count = cmd->u.create_render_pipeline.bind_group_layout_count;
            AT(layout_count >= 1);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[0] == common_layout_id);

            if (slots == 3 && topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST)
            {
                AT(layout_count == 1);
                found_point_pipeline = true;
            }
            else if (slots == 2 && topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            {
                AT(layout_count == 1);
                found_primitive_pipeline = true;
            }
            else if (slots == 2 && topology == DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP)
            {
                AT(layout_count == 1);
                found_path_pipeline = true;
            }
            else if (slots == 2 && topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
            {
                AT(layout_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != common_layout_id);
                found_image_pipeline = true;
            }
            else if (slots == 3 && topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            {
                AT(layout_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != common_layout_id);
                found_lit_mesh_pipeline = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            uint64_t layout_id =
                _stream_bind_group_layout_id(stream, cmd->u.set_bind_group.bind_group_id);
            if (cmd->u.set_bind_group.slot == 0)
            {
                AT(layout_id == common_layout_id);
                found_common_bind = true;
            }
            else if (cmd->u.set_bind_group.slot == 1)
            {
                AT(layout_id != 0);
                AT(layout_id != common_layout_id);
                found_visual_bind = true;
            }
        }
    }

    AT(found_point_pipeline);
    AT(found_primitive_pipeline);
    AT(found_path_pipeline);
    AT(found_image_pipeline);
    AT(found_lit_mesh_pipeline);
    AT(found_common_bind);
    AT(found_visual_bind);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure retained labels parameter mutations keep on-demand scheduling dirty until emitted.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pending_render_work_tracks_labels_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    AT(figure != NULL);
    AT(figure->panel_count == 1);
    DvzPanel* panel = &figure->panels[0];
    AT(panel->visual_count == 1);
    DvzVisual* labels = panel->visuals[0].visual;
    AT(labels != NULL);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;

    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    _test_scene_stream_destroy(stream1);
    AT(!_scene_figure_has_pending_render_work(figure));

    AT(dvz_labels_set_opacity(labels, 0.25f) == 0);
    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    _test_scene_stream_destroy(stream2);
    AT(!_scene_figure_has_pending_render_work(figure));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check that an empty panel emits an explicit clear-only render pass.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_empty_figure_emit_clear_only(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.clear_color[0] = 0.05f;
    emit_cfg.clear_color[1] = 0.06f;
    emit_cfg.clear_color[2] = 0.07f;
    emit_cfg.clear_color[3] = 1.0f;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_begin = false;
    bool found_end = false;
    bool found_draw = false;
    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_begin = true;
            AC(cmd->u.begin_render_pass.clear_color[0], 0.05f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[1], 0.06f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[2], 0.07f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[3], 1.0f, 1e-6f);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_END_RENDER_PASS)
        {
            found_end = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
        {
            found_pipeline = true;
        }
    }
    AT(found_begin);
    AT(found_end);
    AT(!found_draw);
    AT(!found_pipeline);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
