/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Large sampled-field regression tests                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "visuals/common.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool
_large_field_stream_has_pipeline(const DvzDrp2CommandStream* stream, const char* label_part)
{
    ANN(stream);
    ANN(label_part);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (label != NULL && strstr(label, label_part) != NULL)
            return true;
    }
    return false;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_large_r16_image_padded_upload(TstContext* suite, const TstCase* item);
int test_scene_large_r16_uint_labels_padded_upload(TstContext* suite, const TstCase* item);


/**
 * Exercise the current CPU-colorized scalar-image path with an atlas-scale R16 plane.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_large_r16_image_padded_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    enum
    {
        WIDTH = 1320,
        HEIGHT = 800,
        PADDING_TEXELS = 8,
    };
    const uint64_t tight_row_bytes = WIDTH * sizeof(uint16_t);
    const uint64_t source_row_bytes = (WIDTH + PADDING_TEXELS) * sizeof(uint16_t);
    const uint64_t source_size = source_row_bytes * HEIGHT;
    uint8_t* source = (uint8_t*)dvz_calloc(source_size, 1);
    ANN(source);
    dvz_memset(source, source_size, 0xa5, source_size);
    for (uint32_t y = 0; y < HEIGHT; y++)
    {
        uint16_t* row = (uint16_t*)(source + (uint64_t)y * source_row_bytes);
        for (uint32_t x = 0; x < WIDTH; x++)
            row[x] = (uint16_t)((17u * x + 31u * y) & 0xffffu);
    }
    ((uint16_t*)source)[0] = 0;
    uint16_t* last_source_row = (uint16_t*)(source + (uint64_t)(HEIGHT - 1) * source_row_bytes);
    last_source_row[WIDTH - 1] = UINT16_MAX;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == DVZ_OK);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == DVZ_OK);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = WIDTH,
                   .height = HEIGHT,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){
                      DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                      .data = source,
                      .bytes_per_row = source_row_bytes,
                      .rows_per_image = HEIGHT,
                  }) == DVZ_OK);
    dvz_free(source);

    AT(field->data_size == tight_row_bytes * HEIGHT);
    const uint16_t* owned = (const uint16_t*)field->data;
    ANN(owned);
    AT(owned[0] == 0);
    AT(owned[WIDTH] == (uint16_t)31u);
    AT(owned[(uint64_t)WIDTH * HEIGHT - 1] == UINT16_MAX);
    AT(dvz_visual_set_field(image, "field", field) == DVZ_OK);

    DvzScale* scale = dvz_scale(
        scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    AT(dvz_scale_set_domain(scale, 0.0, 1.0) == DVZ_OK);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    AT(dvz_colormap_set_stops(colormap, stops, 2) == DVZ_OK);
    AT(dvz_scale_set_colormap(scale, colormap) == DVZ_OK);
    AT(dvz_visual_set_scale(image, "color", scale) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, image, NULL) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzVisualTexture* texture = &_visual_family_state(image)->texture;
    ANN(texture->rgba);
    AT(texture->rgba_size == (uint64_t)WIDTH * HEIGHT * 4u);
    const uint8_t* rgba = (const uint8_t*)texture->rgba;
    const uint64_t last_rgba = ((uint64_t)WIDTH * HEIGHT - 1) * 4u;
    AT(rgba[0] == 0 && rgba[1] == 0 && rgba[2] == 0 && rgba[3] == 255);
    AT(rgba[last_rgba + 0] == 255 && rgba[last_rgba + 1] == 255 && rgba[last_rgba + 2] == 255 &&
       rgba[last_rgba + 3] == 255);

    bool created_texture = false;
    bool wrote_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.width == WIDTH &&
            cmd->u.create_texture.height == HEIGHT)
        {
            created_texture = true;
            AT(cmd->u.create_texture.depth == 1);
            /* DRP2 format zero is the default RGBA8 texture used by CPU-colorized images. */
            AT(cmd->u.create_texture.format == DVZ_FORMAT_NONE);
            AT(cmd->u.create_texture.color_role == DVZ_DRP2_COLOR_ROLE_SRGB_COLOR);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == WIDTH &&
            cmd->u.write_texture.height == HEIGHT)
        {
            const uint8_t* upload = (const uint8_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            wrote_texture = true;
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == WIDTH * 4u);
            AT(cmd->u.write_texture.rows_per_image == HEIGHT);
            AT(upload[0] == 0 && upload[1] == 0 && upload[2] == 0 && upload[3] == 255);
            AT(upload[last_rgba + 0] == 255 && upload[last_rgba + 1] == 255 &&
               upload[last_rgba + 2] == 255 && upload[last_rgba + 3] == 255);
        }
    }
    AT(created_texture);
    AT(wrote_texture);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Exercise the WebGPU labels path with an atlas-scale padded R16_UINT plane.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_large_r16_uint_labels_padded_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    enum
    {
        WIDTH = 1140,
        HEIGHT = 1320,
        PADDING_TEXELS = 12,
    };
    const uint64_t tight_row_bytes = WIDTH * sizeof(uint16_t);
    const uint64_t source_row_bytes = (WIDTH + PADDING_TEXELS) * sizeof(uint16_t);
    const uint64_t source_size = source_row_bytes * HEIGHT;
    uint8_t* source = (uint8_t*)dvz_calloc(source_size, 1);
    ANN(source);
    dvz_memset(source, source_size, 0xa5, source_size);
    for (uint32_t y = 0; y < HEIGHT; y++)
    {
        uint16_t* row = (uint16_t*)(source + (uint64_t)y * source_row_bytes);
        for (uint32_t x = 0; x < WIDTH; x++)
            row[x] = (uint16_t)(1u + (x + 3u * y) % 4093u);
    }
    uint16_t* first_source_row = (uint16_t*)source;
    uint16_t* second_source_row = (uint16_t*)(source + source_row_bytes);
    uint16_t* last_source_row = (uint16_t*)(source + (uint64_t)(HEIGHT - 1) * source_row_bytes);
    first_source_row[0] = 0;
    first_source_row[WIDTH - 1] = 17;
    second_source_row[0] = 1009;
    last_source_row[WIDTH - 1] = UINT16_MAX;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extents[1] = {{2.0f, 2.0f}};
    AT(dvz_visual_set_data(labels, "position", positions, 1) == DVZ_OK);
    AT(dvz_visual_set_data(labels, "extent", extents, 1) == DVZ_OK);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = WIDTH,
                   .height = HEIGHT,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){
                      DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                      .data = source,
                      .bytes_per_row = source_row_bytes,
                      .rows_per_image = HEIGHT,
                  }) == DVZ_OK);
    dvz_free(source);

    AT(field->data_size == tight_row_bytes * HEIGHT);
    const uint16_t* owned = (const uint16_t*)field->data;
    ANN(owned);
    AT(owned[0] == 0);
    AT(owned[WIDTH - 1] == 17);
    AT(owned[WIDTH] == 1009);
    AT(owned[(uint64_t)WIDTH * HEIGHT - 1] == UINT16_MAX);
    AT(dvz_visual_set_field(labels, "field", field) == DVZ_OK);

    DvzScale* scale = dvz_scale(
        scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[3] = {
        {.category_id = 17, .order = 0, .label = "region 17", .color = {255, 0, 0, 255}},
        {.category_id = 1009, .order = 1, .label = "region 1009", .color = {0, 255, 0, 255}},
        {.category_id = UINT16_MAX,
         .order = 2,
         .label = "region 65535",
         .color = {0, 0, 255, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 3) == DVZ_OK);
    AT(dvz_visual_set_scale(labels, "labels", scale) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, labels, NULL) == DVZ_OK);

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
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool created_texture = false;
    bool wrote_texture = false;
    bool used_uint_wgsl = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.width == WIDTH &&
            cmd->u.create_texture.height == HEIGHT)
        {
            created_texture = true;
            AT(cmd->u.create_texture.depth == 1);
            AT(cmd->u.create_texture.format == DVZ_FORMAT_R16_UINT);
            AT(cmd->u.create_texture.color_role == DVZ_DRP2_COLOR_ROLE_DATA);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == WIDTH &&
            cmd->u.write_texture.height == HEIGHT)
        {
            const uint16_t* upload = (const uint16_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            wrote_texture = true;
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == tight_row_bytes);
            AT(cmd->u.write_texture.rows_per_image == HEIGHT);
            AT(upload[0] == 0);
            AT(upload[WIDTH - 1] == 17);
            AT(upload[WIDTH] == 1009);
            AT(upload[(uint64_t)WIDTH * HEIGHT - 1] == UINT16_MAX);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
            strcmp(cmd->u.create_shader_module.stage, "FRAGMENT") == 0 &&
            strcmp(cmd->u.create_shader_module.format, "wgsl") == 0 &&
            cmd->u.create_shader_module.code != NULL &&
            strstr(cmd->u.create_shader_module.code, "texture_2d<u32>") != NULL)
        {
            used_uint_wgsl = true;
        }
    }
    AT(created_texture);
    AT(wrote_texture);
    AT(used_uint_wgsl);
    AT(_large_field_stream_has_pipeline(stream, "_pipe_labels_uintw"));

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
