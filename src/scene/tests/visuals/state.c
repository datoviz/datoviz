/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene retained visual state tests                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_json(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene*  scene  = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel*  panel  = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzVisual* visual = dvz_point(scene, 0);

    vec3 positions[2] = {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}};
    dvz_visual_set_data(visual, "position", positions, 2);
    dvz_panel_add_visual(panel, visual, NULL);

    char* json = dvz_scene_json(scene);
    AT(json != NULL);
    AT(strstr(json, "\"figures\"") != NULL);
    AT(strstr(json, "\"fig0\"") != NULL);
    AT(strstr(json, "\"point\"") != NULL);
    AT(strstr(json, "\"position\"") != NULL);
    AT(strstr(json, "\"item_count\":2") != NULL);
    AT(strstr(json, "\"data\":\"") != NULL); /* base64 data present */

    dvz_scene_json_destroy(json);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json_includes_field_dirty_metadata(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    uint8_t base[4 * 4 * 4] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = base, .bytes_per_row = 4 * 4, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    ANN(stream);
    _test_scene_stream_destroy(stream);

    uint8_t patch[2 * 4] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = patch, .bytes_per_row = 2 * 4, .rows_per_image = 1}));

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"dirty\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    AT(strstr(json, "\"field_state\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json_includes_buffer_binding_metadata(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    DvzIndex indices[3] = {0, 1, 2};

    DvzSceneBuffer* buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc), .usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    ANN(buffer);
    AT(dvz_scene_buffer_set_data(buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_buffer(visual, "index", buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"buffers\":[") != NULL);
    AT(strstr(json, "\"id\":\"b0\"") != NULL);
    AT(strstr(json, "\"usage\":2") != NULL);
    AT(strstr(json, "\"stride\":4") != NULL);
    AT(strstr(json, "\"byte_size\":12") != NULL);
    AT(strstr(json, "\"dirty\":{\"pending\":true}") != NULL);
    AT(strstr(json, "\"buffer\":{\"id\":\"b0\",\"slot\":\"index\"}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}





int test_scene_rejects_cross_scene_visual(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene_a = dvz_scene();
    DvzScene* scene_b = dvz_scene();
    ANN(scene_a);
    ANN(scene_b);

    DvzFigure* figure = dvz_figure(scene_a, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* foreign = dvz_point(scene_b, 0);
    ANN(foreign);

    AT(dvz_panel_add_visual(panel, foreign, NULL) == -1);

    dvz_scene_destroy(scene_b);
    dvz_scene_destroy(scene_a);
    return 0;
}


int test_scene_rejects_unsupported_point_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    float opacity[2] = {0.25f, 0.75f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "opacity", opacity, 2) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'opacity'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_attr_source_and_mutability_metadata(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    AT(dvz_visual_attr_source(visual, "position") == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM);
    AT(dvz_visual_attr_mutability(visual, "position") == DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC);

    AT(dvz_visual_set_attr_mutability(
           visual, "position", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING) == 0);
    AT(dvz_visual_attr_mutability(visual, "position") ==
       DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);

    AT(dvz_visual_set_attr_source(visual, "color", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == 0);
    AT(dvz_visual_attr_source(visual, "color") == DVZ_VISUAL_ATTR_SOURCE_CONSTANT);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_source(visual, "position", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == -1);
    AT(_captured_log_contains(suite, "does not accept source"));

    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "color", colors, 2) == -1);
    AT(_captured_log_contains(suite, "dense data requires PER_ITEM source"));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify public read-only views over retained dense visual data.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_data_view(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    float sizes[2] = {6.0f, 12.0f};

    AT(dvz_visual_set_attr_mutability(
           visual, "position", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING) == 0);
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "diameter", sizes, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(visual, "position", &view) == 0);
    AT(view.data != NULL);
    AT(view.item_count == 2);
    AT(view.item_size == 3 * sizeof(float));
    AT(view.source == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM);
    AT(view.mutability == DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    AT(view.version > 0);
    const float* view_positions = view.data;
    AT(view_positions[3] == 1.0f);

    DvzVisualDataView alias_view = {0};
    AT(dvz_visual_data(visual, "diameter", &alias_view) == 0);
    AT(alias_view.data != NULL);
    AT(alias_view.item_count == 2);
    AT(alias_view.item_size == sizeof(float));
    const float* view_sizes = alias_view.data;
    AT(view_sizes[0] == 6.0f);
    AT(view_sizes[1] == 12.0f);

    AT(dvz_visual_set_attr_source(visual, "color", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == 0);
    DvzVisualDataView missing_view = {0};
    AT(dvz_visual_data(visual, "color", &missing_view) == -1);
    AT(missing_view.data == NULL);
    AT(dvz_visual_data(visual, "texcoords", &missing_view) == -1);
    AT(dvz_visual_data(NULL, "position", &missing_view) == -1);
    AT(dvz_visual_data(visual, NULL, &missing_view) == -1);
    AT(dvz_visual_data(visual, "position", NULL) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify scalar color attribute format metadata and retained dense views.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_scalar_color_attr_format(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    AT(dvz_visual_attr_format(point, "color") == DVZ_VISUAL_ATTR_FORMAT_RGBA_U8);
    AT(dvz_visual_attr_format(point, "position") == DVZ_VISUAL_ATTR_FORMAT_DEFAULT);
    AT(dvz_visual_set_attr_format(
           point, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == 0);
    AT(dvz_visual_attr_format(point, "color") == DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    float scalars[2] = {0.25f, 0.75f};
    float sizes[2] = {6.0f, 12.0f};
    AT(dvz_visual_set_data(point, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(point, "color", scalars, 2) == 0);
    AT(dvz_visual_set_data(point, "diameter", sizes, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(point, "color", &view) == 0);
    AT(view.item_count == 2);
    AT(view.item_size == sizeof(float));
    const float* retained = view.data;
    AT(retained[0] == 0.25f);
    AT(retained[1] == 0.75f);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_format(point, "color", DVZ_VISUAL_ATTR_FORMAT_RGBA_U8) == -1);
    AT(_captured_log_contains(suite, "format cannot change after payload attachment"));

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_format(mesh, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == -1);
    AT(_captured_log_contains(suite, "does not support format"));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify scalar point/pixel color data emits the RGBA buffer expected by current pipelines.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_scalar_color_emits_rgba_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* pixel = dvz_pixel(scene, 0);
    ANN(pixel);

    const uint32_t N = 4;
    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    float values[4] = {0.0f, 0.25f, 0.75f, 1.0f};
    float sizes[4] = {4.0f, 4.0f, 4.0f, 4.0f};

    AT(dvz_visual_set_attr_format(pixel, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == 0);
    AT(dvz_visual_set_data(pixel, "position", positions, N) == 0);
    AT(dvz_visual_set_data(pixel, "color", values, N) == 0);
    AT(dvz_visual_set_data(pixel, "pixel_size", sizes, N) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(pixel, "color", &view) == 0);
    AT(view.item_size == sizeof(float));

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_GRAY);
    ANN(colormap);
    DvzScale* scale =
        dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                            .kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_colormap(scale, colormap);
    AT(dvz_visual_set_scale(pixel, "color", scale) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_color_upload = false;
    DvzColor expected_first = {0};
    DvzColor expected_last = {0};
    AT(dvz_colormap_sample(colormap, 0.0, &expected_first));
    AT(dvz_colormap_sample(colormap, 1.0, &expected_last));
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
        if (label == NULL || strstr(label, "color") == NULL)
            continue;
        found_color_upload = true;
        AT(cmd->u.write_buffer.offset == 0);
        AT(cmd->u.write_buffer.size == N * sizeof(DvzColor));
        ANN(cmd->u.write_buffer.data_raw);
        const DvzColor* uploaded = (const DvzColor*)cmd->u.write_buffer.data_raw;
        AT(uploaded[0].r == expected_first.r);
        AT(uploaded[0].g == expected_first.g);
        AT(uploaded[0].b == expected_first.b);
        AT(uploaded[0].a == expected_first.a);
        AT(uploaded[N - 1].r == expected_last.r);
        AT(uploaded[N - 1].g == expected_last.g);
        AT(uploaded[N - 1].b == expected_last.b);
        AT(uploaded[N - 1].a == expected_last.a);
        break;
    }
    AT(found_color_upload);
    _test_scene_stream_destroy(stream);

    int attr_idx = _attr_index(pixel, "color");
    AT(attr_idx >= 0);
    AT(pixel->attrs[attr_idx].dirty_item_count == 0);
    dvz_scale_set_domain(scale, 0.0, 2.0);
    AT(pixel->attrs[attr_idx].dirty_first_item == 0);
    AT(pixel->attrs[attr_idx].dirty_item_count == N);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify retained visual-space bounds for point data and range mutations.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_point_and_range_update(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == -1);
    AT(!bounds.valid);

    vec3 positions[3] = {
        {-2.0f, +1.0f, 0.0f},
        {+4.0f, -3.0f, 2.0f},
        {+1.0f, +5.0f, -1.0f},
    };
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -2.0, -3.0, -1.0, +4.0, +5.0, +2.0) == 0);

    vec3 update[1] = {{+8.0f, +2.0f, +4.0f}};
    AT(dvz_visual_set_data_range(visual, "position", update, 1, 1) == 0);
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -2.0, +1.0, -1.0, +8.0, +5.0, +4.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify family-specific retained visual-space bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_family_reducers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzBounds bounds = {0};

    DvzVisual* segment = dvz_segment(scene, 0);
    ANN(segment);
    vec3 starts[2] = {{-1.0f, -2.0f, 0.0f}, {+2.0f, +1.0f, +3.0f}};
    vec3 ends[2] = {{+4.0f, -1.0f, 1.0f}, {-3.0f, +5.0f, -2.0f}};
    AT(dvz_visual_set_data(segment, "position_start", starts, 2) == 0);
    AT(dvz_visual_set_data(segment, "position_end", ends, 2) == 0);
    AT(dvz_visual_bounds(segment, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -3.0, -2.0, -2.0, +4.0, +5.0, +3.0) == 0);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    vec3 sphere_pos[2] = {{0.0f, 0.0f, 0.0f}, {3.0f, -1.0f, 2.0f}};
    float radius[2] = {0.5f, 2.0f};
    AT(dvz_visual_set_data(sphere, "position", sphere_pos, 2) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radius, 2) == 0);
    AT(dvz_visual_bounds(sphere, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -0.5, -3.0, -0.5, +5.0, +1.0, +4.0) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[2] = {{0.0f, 0.0f, 0.0f}, {4.0f, 2.0f, 1.0f}};
    vec2 image_extent[2] = {{2.0f, 4.0f}, {6.0f, 2.0f}};
    vec2 image_anchor[2] = {{0.0f, 0.0f}, {-1.0f, +1.0f}};
    AT(dvz_visual_set_data(image, "position", image_pos, 2) == 0);
    AT(dvz_visual_set_data(image, "extent", image_extent, 2) == 0);
    AT(dvz_visual_set_data(image, "anchor", image_anchor, 2) == 0);
    AT(dvz_visual_bounds(image, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -1.0, -2.0, 0.0, +10.0, +2.0, +1.0) == 0);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    double volume_min[3] = {-2.0, -3.0, -4.0};
    double volume_max[3] = {+4.0, +5.0, +6.0};
    AT(dvz_volume_set_bounds(volume, volume_min, volume_max) == 0);
    AT(dvz_visual_bounds(volume, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -2.0, -3.0, -4.0, +4.0, +5.0, +6.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify mesh bounds include per-instance transforms.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_mesh_instance_transform(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}};
    float transforms[2][16] = {
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        },
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            10.0f, -1.0f, 2.0f, 1.0f,
        },
    };
    AT(dvz_visual_set_data(mesh, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(mesh, "instance_transform", transforms, 2) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(mesh, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, 0.0, -1.0, 0.0, 11.0, 2.0, 5.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify panel-level visual and screen bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_visual_bounds_and_union(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* left = dvz_point(scene, 0);
    DvzVisual* right = dvz_point(scene, 0);
    ANN(left);
    ANN(right);
    vec3 left_pos[2] = {{-1.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    vec3 right_pos[2] = {{+0.5f, -0.5f, 0.0f}, {+1.0f, +0.5f, 0.0f}};
    AT(dvz_visual_set_data(left, "position", left_pos, 2) == 0);
    AT(dvz_visual_set_data(right, "position", right_pos, 2) == 0);
    AT(dvz_panel_add_visual(panel, left, NULL) == 0);
    AT(dvz_panel_add_visual(panel, right, NULL) == 0);

    DvzBounds bounds = {0};
    AT(dvz_panel_visual_bounds(panel, left, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, 0.0, +1.0, 0.0) == 0);

    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +1.0, 0.0) == 0);

    dvz_visual_set_visible(right, false);
    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, 0.0, +1.0, 0.0) == 0);

    AT(dvz_panel_visual_bounds(panel, left, DVZ_BOUNDS_SPACE_SCREEN, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, 0.0, 0.0, 0.0, 100.0, 100.0, 0.0) == 0);

    DvzVisual* unattached = dvz_point(scene, 0);
    ANN(unattached);
    AT(dvz_panel_visual_bounds(panel, unattached, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify the panel-owned bounds overlay generates front and occluded wireframe visuals.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_visual(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    vec3 positions[2] = {{-1.0f, -1.0f, 0.0f}, {+1.0f, +1.0f, 0.0f}};
    float diameters[2] = {20.0f, 20.0f};
    AT(dvz_visual_set_data(points, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(points, "diameter", diameters, 2) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    AT(!dvz_panel_bounds_visible(panel));
    AT(dvz_panel_set_bounds_visible(panel, true) == 0);
    AT(dvz_panel_bounds_visible(panel));
    _scene_prepare_bounds_visuals(figure);

    DvzVisual* overlay = panel->bounds_visual;
    DvzVisual* occluded_overlay = panel->bounds_occluded_visual;
    ANN(overlay);
    ANN(occluded_overlay);
    AT(overlay != occluded_overlay);
    AT(overlay->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(occluded_overlay->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(overlay->visible);
    AT(occluded_overlay->visible);
    AT(overlay->depth_test_enabled);
    AT(occluded_overlay->depth_test_enabled);
    AT(overlay->depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL);
    AT(occluded_overlay->depth_compare_op == VK_COMPARE_OP_GREATER);
    int start_idx = _attr_index(overlay, "position_start");
    int end_idx = _attr_index(overlay, "position_end");
    int color_idx = _attr_index(overlay, "color");
    int width_idx = _attr_index(overlay, "line_width");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    AT(color_idx >= 0);
    AT(width_idx >= 0);
    AT(overlay->attrs[start_idx].item_count == 4);
    AT(overlay->attrs[end_idx].item_count == 4);
    AT(overlay->attrs[color_idx].item_count == 4);
    AT(overlay->attrs[width_idx].item_count == 4);
    const float* starts = (const float*)overlay->attrs[start_idx].data;
    const float* ends = (const float*)overlay->attrs[end_idx].data;
    ANN(starts);
    ANN(ends);
    float min_x = +FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = +FLT_MAX;
    float max_y = -FLT_MAX;
    for (uint32_t i = 0; i < overlay->attrs[start_idx].item_count; i++)
    {
        min_x = fminf(min_x, starts[3 * i + 0]);
        min_x = fminf(min_x, ends[3 * i + 0]);
        max_x = fmaxf(max_x, starts[3 * i + 0]);
        max_x = fmaxf(max_x, ends[3 * i + 0]);
        min_y = fminf(min_y, starts[3 * i + 1]);
        min_y = fminf(min_y, ends[3 * i + 1]);
        max_y = fmaxf(max_y, starts[3 * i + 1]);
        max_y = fmaxf(max_y, ends[3 * i + 1]);
    }
    AT(min_x < -1.09f);
    AT(max_x > +1.09f);
    AT(min_y < -1.19f);
    AT(max_y > +1.19f);

    start_idx = _attr_index(occluded_overlay, "position_start");
    end_idx = _attr_index(occluded_overlay, "position_end");
    color_idx = _attr_index(occluded_overlay, "color");
    width_idx = _attr_index(occluded_overlay, "line_width");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    AT(color_idx >= 0);
    AT(width_idx >= 0);
    AT(occluded_overlay->attrs[start_idx].item_count == 4);
    AT(occluded_overlay->attrs[end_idx].item_count == 4);
    AT(occluded_overlay->attrs[color_idx].item_count == 4);
    AT(occluded_overlay->attrs[width_idx].item_count == 4);
    DvzColor* hidden_colors = (DvzColor*)occluded_overlay->attrs[color_idx].data;
    ANN(hidden_colors);
    AT(hidden_colors[0].a == 120);

    DvzBounds bounds = {0};
    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +1.0, 0.0) == 0);

    AT(dvz_panel_set_bounds_visible(panel, false) == 0);
    _scene_prepare_bounds_visuals(figure);
    AT(!overlay->visible);
    AT(!occluded_overlay->visible);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify point-like overlay padding follows the active panzoom extent.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_visual_panzoom_padding(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    dvz_panzoom_zoom(panzoom, (vec2){0.5f, 0.5f});

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    vec3 positions[2] = {{-1.0f, -1.0f, 0.0f}, {+1.0f, +1.0f, 0.0f}};
    float diameters[2] = {20.0f, 20.0f};
    AT(dvz_visual_set_data(points, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(points, "diameter", diameters, 2) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    AT(dvz_panel_set_bounds_visible(panel, true) == 0);
    _scene_prepare_bounds_visuals(figure);

    DvzVisual* overlay = panel->bounds_visual;
    ANN(overlay);
    int start_idx = _attr_index(overlay, "position_start");
    int end_idx = _attr_index(overlay, "position_end");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    AT(overlay->attrs[start_idx].item_count == 4);
    const float* starts = (const float*)overlay->attrs[start_idx].data;
    const float* ends = (const float*)overlay->attrs[end_idx].data;
    ANN(starts);
    ANN(ends);

    float min_x = +FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = +FLT_MAX;
    float max_y = -FLT_MAX;
    for (uint32_t i = 0; i < overlay->attrs[start_idx].item_count; i++)
    {
        min_x = fminf(min_x, starts[3 * i + 0]);
        min_x = fminf(min_x, ends[3 * i + 0]);
        max_x = fmaxf(max_x, starts[3 * i + 0]);
        max_x = fmaxf(max_x, ends[3 * i + 0]);
        min_y = fminf(min_y, starts[3 * i + 1]);
        min_y = fminf(min_y, ends[3 * i + 1]);
        max_y = fmaxf(max_y, starts[3 * i + 1]);
        max_y = fmaxf(max_y, ends[3 * i + 1]);
    }

    AC(min_x, -1.2f, 1e-6);
    AC(max_x, +1.2f, 1e-6);
    AC(min_y, -1.4f, 1e-6);
    AC(max_y, +1.4f, 1e-6);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify sphere overlays use conservative wire bounds while public bounds remain exact.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_sphere_wire_padding(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float radius[1] = {0.25f};
    AT(dvz_visual_set_data(sphere, "position", position, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", color, 1) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radius, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(sphere, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 3, -0.25, -0.25, -0.25, +0.25, +0.25, +0.25) == 0);

    AT(dvz_panel_set_bounds_visible(panel, true) == 0);
    _scene_prepare_bounds_visuals(figure);
    ANN(panel->bounds_visual);
    int start_idx = _attr_index(panel->bounds_visual, "position_start");
    int end_idx = _attr_index(panel->bounds_visual, "position_end");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    const float* starts = (const float*)panel->bounds_visual->attrs[start_idx].data;
    const float* ends = (const float*)panel->bounds_visual->attrs[end_idx].data;
    ANN(starts);
    ANN(ends);

    float min_x = +FLT_MAX;
    float max_x = -FLT_MAX;
    for (uint32_t i = 0; i < panel->bounds_visual->attrs[start_idx].item_count; i++)
    {
        min_x = fminf(min_x, starts[3 * i + 0]);
        min_x = fminf(min_x, ends[3 * i + 0]);
        max_x = fmaxf(max_x, starts[3 * i + 0]);
        max_x = fmaxf(max_x, ends[3 * i + 0]);
    }
    AT(min_x < -0.40f);
    AT(max_x > +0.40f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify bounds overlay pipeline keys survive runtime suffix composition.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_emit_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.sample_count = 4;
    AT(dvz_panel_set_msaa(panel, &msaa));

    DvzVisual* spheres = dvz_sphere(scene, 0);
    ANN(spheres);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{220, 120, 80, 255}};
    float radii[1] = {0.25f};
    AT(dvz_visual_set_data(spheres, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(spheres, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(spheres, "radius", radii, 1) == 0);
    AT(dvz_panel_add_visual(panel, spheres, NULL) == 0);
    AT(dvz_panel_set_bounds_visible(panel, true) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(
        stream, "_pipe_segmentg_coverage_blend_depth_msaa4"));
    AT(_stream_has_render_pipeline_label_part(
        stream, "_pipe_segmentg_coverage_blend_depth_gt_depth_msaa4"));

    bool found_front_pipeline = false;
    bool found_occluded_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
        if (label == NULL)
            continue;
        if (strstr(label, "_pipe_segmentg_coverage_blend_depth_gt_depth_msaa4") != NULL)
        {
            found_occluded_pipeline =
                command->u.create_render_pipeline.has_depth_attachment &&
                !command->u.create_render_pipeline.depth_write_enabled &&
                command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_GREATER;
        }
        else if (strstr(label, "_pipe_segmentg_coverage_blend_depth_msaa4") != NULL)
        {
            found_front_pipeline =
                command->u.create_render_pipeline.has_depth_attachment &&
                command->u.create_render_pipeline.depth_write_enabled &&
                command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL;
        }
    }
    AT(found_front_pipeline);
    AT(found_occluded_pipeline);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_typed_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float diameters[2] = {4.0f, 8.0f};
    uint32_t item_state[2] = {DVZ_ITEM_STATE_SELECTED, DVZ_ITEM_STATE_NONE};

    DvzVisualDataUpdate point_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "diameter", .data = diameters, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(visual, point_updates, 3) == 0);
    AT(dvz_visual_set_data(visual, "item_state", item_state, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(visual, "diameter", &view) == 0);
    const float* stored_diameters = view.data;
    AT(stored_diameters[1] == 8.0f);

    DvzVisualDataView state_view = {0};
    AT(dvz_visual_data(visual, "item_state", &state_view) == 0);
    const uint32_t* stored_state = state_view.data;
    AT(stored_state[0] == DVZ_ITEM_STATE_SELECTED);
    AT(stored_state[1] == DVZ_ITEM_STATE_NONE);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_typed_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    vec2 texcoords[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    mat4 transforms[1] = {{{1.0f, 0.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f}}};

    DvzVisualDataUpdate mesh_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "normal", .data = normals, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(visual, mesh_updates, 3) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 3) == 0);
    AT(dvz_visual_set_data(visual, "instance_transform", transforms, 1) == 0);

    DvzVisualDataView normal_view = {0};
    AT(dvz_visual_data(visual, "normal", &normal_view) == 0);
    const float* stored_normals = normal_view.data;
    AT(stored_normals[2] == 1.0f);

    DvzVisualDataView transform_view = {0};
    AT(dvz_visual_data(visual, "instance_transform", &transform_view) == 0);
    AT(transform_view.item_count == 1);

    DvzVisualDataView texcoord_view = {0};
    AT(dvz_visual_data(visual, "texcoords", &texcoord_view) == 0);
    AT(texcoord_view.item_count == 3);
    AT(texcoord_view.item_size == 2 * sizeof(float));
    const float* stored_texcoords = texcoord_view.data;
    AT(stored_texcoords[3] == 0.0f);
    AT(stored_texcoords[5] == 1.0f);

    DvzVisual* default_color_mesh = dvz_mesh(scene, 0);
    ANN(default_color_mesh);
    AT(dvz_visual_set_data(default_color_mesh, "position", positions, 3) == 0);
    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(default_color_mesh, "color", &color_view) == 0);
    AT(color_view.item_count == 3);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check copied index data convenience upload and helper-owned buffer replacement.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_index_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* point = dvz_point(scene, 0);
    ANN(mesh);
    ANN(primitive);
    ANN(point);

    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    AT(dvz_visual_set_index_data(mesh, indices, 6) == 0);
    AT(_visual_family_state(mesh)->buffer != NULL);
    AT(_visual_family_state(mesh)->buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX);
    AT(_visual_family_state(mesh)->buffer->desc.stride == sizeof(DvzIndex));
    AT(_visual_family_state(mesh)->buffer->desc.byte_size == sizeof(indices));
    const DvzIndex* stored = _visual_family_state(mesh)->buffer->data;
    AT(stored[5] == 3);

    DvzSceneBuffer* old_buffer = _visual_family_state(mesh)->buffer;
    DvzIndex updated[3] = {0, 2, 1};
    AT(dvz_visual_set_index_data(mesh, updated, 3) == 0);
    AT(_visual_family_state(mesh)->buffer != old_buffer);
    AT(old_buffer->scene == NULL);
    AT(_visual_family_state(mesh)->buffer->desc.byte_size == sizeof(updated));
    stored = _visual_family_state(mesh)->buffer->data;
    AT(stored[1] == 2);

    AT(dvz_visual_set_index_data(primitive, indices, 3) == 0);
    AT(_visual_family_state(primitive)->buffer != NULL);
    AT(_visual_family_state(primitive)->buffer->desc.byte_size == 3 * sizeof(DvzIndex));

    AT(dvz_visual_set_index_data(point, indices, 3) == -1);
    AT(dvz_visual_set_index_data(mesh, NULL, 3) == -1);
    AT(dvz_visual_set_index_data(mesh, indices, 0) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_geometry_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };
    DvzGeometrySurfaceGridDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = 2,
        .cols = 2,
        .colors = colors,
    };
    DvzGeometry* geometry = dvz_geom_surface_grid(&desc);
    ANN(geometry);

    AT(dvz_mesh_set_geometry(visual, geometry) == 0);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(visual, "position", &position_view) == 0);
    AT(position_view.item_count == 4);
    AT(position_view.item_size == 3 * sizeof(float));
    const float* positions = position_view.data;
    AT(positions[0] == 0.0f);
    AT(positions[3] == 1.0f);

    DvzVisualDataView normal_view = {0};
    AT(dvz_visual_data(visual, "normal", &normal_view) == 0);
    const float* normals = normal_view.data;
    AT(normals[2] == 1.0f);

    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(visual, "color", &color_view) == 0);
    const uint8_t* stored_colors = color_view.data;
    AT(stored_colors[0] == 255);
    AT(stored_colors[5] == 255);

    DvzVisualDataView texcoord_view = {0};
    AT(dvz_visual_data(visual, "texcoords", &texcoord_view) == 0);
    AT(texcoord_view.item_count == 4);
    AT(texcoord_view.item_size == 2 * sizeof(float));
    const float* texcoords = texcoord_view.data;
    AT(texcoords[0] == 0.0f);
    AT(texcoords[2] == 1.0f);
    AT(texcoords[7] == 1.0f);

    AT(_visual_family_state(visual)->buffer != NULL);
    AT(_visual_family_state(visual)->buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX);
    AT(_visual_family_state(visual)->buffer->desc.stride == sizeof(DvzIndex));
    AT(_visual_family_state(visual)->buffer->desc.byte_size == 6 * sizeof(DvzIndex));

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    AT(dvz_mesh_set_geometry(point, geometry) == -1);

    dvz_geometry_destroy(geometry);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_polygon_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzPolygon* polygon = dvz_polygon(scene, 0);
    ANN(polygon);
    const dvec2 outer[4] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    };
    AT(dvz_polygon_geometry(
           polygon,
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
               .outer = {.xy = outer, .count = 4},
           }) == 0);

    const DvzColor fill_color = {20, 40, 200, 255};
    const DvzColor stroke_color = {240, 220, 40, 255};
    AT(dvz_polygon_fill_color(polygon, fill_color) == 0);
    AT(dvz_polygon_stroke_color(polygon, stroke_color) == 0);
    AT(dvz_polygon_stroke_width(polygon, 3.0f) == 0);
    AT(dvz_polygon_id(polygon, 42) == 0);
    AT(polygon->user_id == 42);
    AT(dvz_polygon_stroke_caps(polygon, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_TRIANGLE_OUT) == 0);
    AT(dvz_polygon_stroke_join(polygon, DVZ_PATH_JOIN_BEVEL, 3.0f) == 0);
    DvzPolygonStyle style = dvz_polygon_style();
    style.fill_color = fill_color;
    style.stroke_color = stroke_color;
    style.stroke_width = 3.0f;
    style.stroke_start_cap = DVZ_SEGMENT_CAP_BUTT;
    style.stroke_end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    style.stroke_join = DVZ_PATH_JOIN_BEVEL;
    style.stroke_miter_limit = 3.0f;
    AT(dvz_polygon_set_style(polygon, &style) == 0);

    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    ANN(composite);
    AT(dvz_composite_visual_count(composite) == 2);
    DvzVisual* fill = dvz_composite_visual(composite, "fill");
    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    ANN(fill);
    ANN(stroke);
    AT(dvz_composite_visual_at(composite, 0) == fill);

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = 5;
    attach.coord_space = DVZ_COORD_DATA;
    AT(dvz_panel_add_composite(panel, composite, &attach) == 0);
    AT(panel->visual_count == 2);
    AT(panel->visuals[0].visual == fill);
    AT(panel->visuals[0].z_layer == 5);
    AT(panel->visuals[0].coord_space == DVZ_COORD_DATA);
    AT(panel->visuals[1].visual == stroke);
    AT(panel->visuals[1].z_layer == 6);
    AT(panel->visuals[1].coord_space == DVZ_COORD_DATA);
    AT(dvz_panel_add_composite(panel, composite, NULL) == 0);
    AT(panel->visual_count == 2);

    DvzVisualDataView fill_position_view = {0};
    AT(dvz_visual_data(fill, "position", &fill_position_view) == 0);
    AT(fill_position_view.item_count == 4);
    AT(_visual_family_state(fill)->buffer != NULL);
    AT(_visual_family_state(fill)->buffer->desc.byte_size == 6 * sizeof(DvzIndex));

    DvzVisualDataView fill_color_view = {0};
    AT(dvz_visual_data(fill, "color", &fill_color_view) == 0);
    const uint8_t* fill_colors = fill_color_view.data;
    AT(fill_colors[0] == fill_color.r);
    AT(fill_colors[1] == fill_color.g);
    AT(fill_colors[2] == fill_color.b);
    AT(fill_colors[3] == fill_color.a);

    DvzVisualDataView stroke_position_view = {0};
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 5);
    AT(_visual_family_state(stroke)->path.subpath_count == 1);
    AT(_visual_family_state(stroke)->path.subpath_lengths[0] == 5);
    DvzVisualDataView stroke_width_view = {0};
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    const float* widths = stroke_width_view.data;
    AC(widths[0], 3.0f, EPS);
    AT(_visual_family_state(stroke)->path.cap_start == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(stroke)->path.cap_end == DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(stroke)->path.join == DVZ_PATH_JOIN_BEVEL);
    AC(_visual_family_state(stroke)->path.miter_limit, 3.0f, EPS);

    uint64_t fill_position_version = 0;
    uint64_t fill_color_version = 0;
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
            fill_position_version = fill->attrs[ai].version;
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
            fill_color_version = fill->attrs[ai].version;
    }
    AT(fill_position_version > 0);
    AT(fill_color_version > 0);
    AT(dvz_polygon_stroke_width(polygon, 7.0f) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], 7.0f, EPS);
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
        {
            AT(fill->attrs[ai].version == fill_position_version);
        }
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
        {
            AT(fill->attrs[ai].version == fill_color_version);
        }
    }

    const DvzColor fill_update = {200, 30, 40, 255};
    AT(dvz_polygon_fill_color(polygon, fill_update) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &fill_color_view) == 0);
    fill_colors = fill_color_view.data;
    AT(fill_colors[0] == fill_update.r);
    AT(fill_colors[1] == fill_update.g);
    AT(fill_colors[2] == fill_update.b);

    const dvec2 hole[4] = {
        {0.25, 0.25},
        {0.75, 0.25},
        {0.75, 0.75},
        {0.25, 0.75},
    };
    AT(dvz_polygon_hole(polygon, 0, 4, hole) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "position", &fill_position_view) == 0);
    AT(fill_position_view.item_count == 8);
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 10);
    AT(_visual_family_state(stroke)->path.subpath_count == 2);

    AT(dvz_polygon_visible(polygon, false) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(!fill->visible);
    AT(!stroke->visible);
    AT(dvz_polygon_visible(polygon, true) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(fill->visible);
    AT(stroke->visible);

    dvz_composite_destroy(composite);
    AT(dvz_composite_visual_count(composite) == 0);
    AT(!fill->visible);
    AT(!stroke->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_polygon_set_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    ANN(set);
    const dvec2 left[4] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    };
    const dvec2 right[4] = {
        {2.0, 0.0},
        {3.0, 0.0},
        {3.0, 1.0},
        {2.0, 1.0},
    };
    const uint32_t left_index = dvz_polygon_set_add(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = left, .count = 4},
        });
    const uint32_t right_index = dvz_polygon_set_add(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = right, .count = 4},
        });
    AT(left_index == 0);
    AT(right_index == 1);

    const DvzColor red = {255, 0, 0, 255};
    const DvzColor green = {0, 255, 0, 255};
    AT(dvz_polygon_set_region_fill_color(set, left_index, red) == 0);
    AT(dvz_polygon_set_region_fill_color(set, right_index, green) == 0);
    AT(dvz_polygon_set_region_stroke_width(set, left_index, 2.0f) == 0);
    AT(dvz_polygon_set_region_stroke_width(set, right_index, 4.0f) == 0);
    const uint64_t ids[2] = {101, 102};
    AT(dvz_polygon_set_region_ids(set, 0, 2, ids) == 0);
    AT(set->polygons[left_index].user_id == 101);
    AT(set->polygons[right_index].user_id == 102);
    AT(dvz_polygon_set_stroke_caps(set, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_TRIANGLE_OUT) == 0);
    AT(dvz_polygon_set_stroke_join(set, DVZ_PATH_JOIN_MITER, 5.0f) == 0);

    DvzComposite* composite = dvz_polygon_set_composite(set, 0);
    ANN(composite);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = 3;
    attach.coord_space = DVZ_COORD_DATA;
    AT(dvz_panel_add_composite(panel, composite, &attach) == 0);
    AT(panel->visual_count == 2);

    DvzVisual* fill = dvz_composite_visual(composite, "fill");
    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    ANN(fill);
    ANN(stroke);
    AT(panel->visuals[0].visual == fill);
    AT(panel->visuals[0].z_layer == 3);
    AT(panel->visuals[0].coord_space == DVZ_COORD_DATA);
    AT(panel->visuals[1].visual == stroke);
    AT(panel->visuals[1].z_layer == 4);
    AT(panel->visuals[1].coord_space == DVZ_COORD_DATA);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(fill, "position", &position_view) == 0);
    AT(position_view.item_count == 8);
    AT(_visual_family_state(fill)->buffer != NULL);
    AT(_visual_family_state(fill)->buffer->desc.byte_size == 12 * sizeof(DvzIndex));

    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    const uint8_t* colors = color_view.data;
    AT(colors[0] == red.r);
    AT(colors[1] == red.g);
    AT(colors[16] == green.r);
    AT(colors[17] == green.g);

    DvzVisualDataView stroke_position_view = {0};
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 10);
    AT(_visual_family_state(stroke)->path.subpath_count == 2);
    DvzVisualDataView stroke_width_view = {0};
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    const float* widths = stroke_width_view.data;
    AC(widths[0], 2.0f, EPS);
    AC(widths[5], 4.0f, EPS);
    AT(_visual_family_state(stroke)->path.cap_start == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(stroke)->path.cap_end == DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(stroke)->path.join == DVZ_PATH_JOIN_MITER);
    AC(_visual_family_state(stroke)->path.miter_limit, 5.0f, EPS);

    uint64_t fill_position_version = 0;
    uint64_t fill_color_version = 0;
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
            fill_position_version = fill->attrs[ai].version;
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
            fill_color_version = fill->attrs[ai].version;
    }
    AT(fill_position_version > 0);
    AT(fill_color_version > 0);
    AT(dvz_polygon_set_region_stroke_width(set, right_index, 7.0f) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], 2.0f, EPS);
    AC(widths[5], 7.0f, EPS);
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
        {
            AT(fill->attrs[ai].version == fill_position_version);
        }
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
        {
            AT(fill->attrs[ai].version == fill_color_version);
        }
    }

    const DvzColor blue = {0, 0, 255, 255};
    AT(dvz_polygon_set_region_fill_color(set, right_index, blue) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    colors = color_view.data;
    AT(colors[16] == blue.r);
    AT(colors[17] == blue.g);
    AT(colors[18] == blue.b);

    const DvzColor bulk_fill[2] = {{11, 22, 33, 255}, {44, 55, 66, 255}};
    const DvzColor bulk_stroke[2] = {{77, 88, 99, 255}, {111, 122, 133, 255}};
    const float bulk_widths[2] = {1.5f, 2.5f};
    AT(dvz_polygon_set_region_fill_colors(set, 0, 2, bulk_fill) == 0);
    AT(dvz_polygon_set_region_stroke_colors(set, 0, 2, bulk_stroke) == 0);
    AT(dvz_polygon_set_region_stroke_widths(set, 0, 2, bulk_widths) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    colors = color_view.data;
    AT(colors[0] == bulk_fill[0].r);
    AT(colors[16] == bulk_fill[1].r);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], bulk_widths[0], EPS);
    AC(widths[5], bulk_widths[1], EPS);

    const bool one_visible[2] = {true, false};
    AT(dvz_polygon_set_region_visibilities(set, 0, 2, one_visible) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "position", &position_view) == 0);
    AT(position_view.item_count == 4);
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 5);
    AT(dvz_polygon_set_region_visible(set, left_index, false) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(!fill->visible);
    AT(!stroke->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_graph_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzGraph* graph = dvz_graph(scene, 0);
    ANN(graph);
    const dvec3 positions[3] = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.5, 1.0, 0.0},
    };
    AT(dvz_graph_node_count(graph, 3) == 0);
    AT(dvz_graph_node_positions(graph, 0, 3, positions) == 0);
    const uint32_t edges[4] = {0, 1, 1, 2};
    AT(dvz_graph_edge_count(graph, 2) == 0);
    AT(dvz_graph_edges(graph, 0, 2, edges) == 0);
    const uint64_t node_ids[3] = {101, 102, 103};
    const uint64_t edge_ids[2] = {201, 202};
    AT(dvz_graph_node_ids(graph, 0, 3, node_ids) == 0);
    AT(dvz_graph_edge_ids(graph, 0, 2, edge_ids) == 0);
    AT(graph->nodes[0].user_id == 101);
    AT(graph->nodes[2].user_id == 103);
    AT(graph->edges[0].user_id == 201);
    AT(graph->edges[1].user_id == 202);
    const DvzColor node_colors[3] = {
        {255, 80, 80, 255},
        {80, 255, 80, 255},
        {80, 80, 255, 255},
    };
    const float node_sizes[3] = {10.0f, 20.0f, 30.0f};
    const DvzColor edge_colors[2] = {{220, 220, 220, 255}, {255, 180, 80, 255}};
    const float edge_widths[2] = {2.0f, 4.0f};
    AT(dvz_graph_node_colors(graph, 0, 3, node_colors) == 0);
    AT(dvz_graph_node_sizes(graph, 0, 3, node_sizes) == 0);
    AT(dvz_graph_edge_colors(graph, 0, 2, edge_colors) == 0);
    AT(dvz_graph_edge_widths(graph, 0, 2, edge_widths) == 0);

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    ANN(composite);
    AT(dvz_composite_visual_count(composite) == 3);
    AT(dvz_panel_add_composite(
           panel, composite,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 7}) == 0);
    AT(panel->visual_count == 3);
    AT(dvz_panel_add_composite(panel, composite, NULL) == 0);
    AT(panel->visual_count == 3);

    DvzVisual* edge_visual = dvz_composite_visual(composite, "edges");
    DvzVisual* node_visual = dvz_composite_visual(composite, "nodes");
    ANN(edge_visual);
    ANN(node_visual);
    AT(edge_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(node_visual->type == DVZ_VISUAL_TYPE_MARKER);
    AT(panel->visuals[0].z_layer == 7);
    AT(panel->visuals[2].z_layer == 8);

    DvzVisualDataView node_position_view = {0};
    AT(dvz_visual_data(node_visual, "position", &node_position_view) == 0);
    AT(node_position_view.item_count == 3);
    const float* node_positions = node_position_view.data;
    AC(node_positions[0], 0.0f, EPS);
    AC(node_positions[3], 1.0f, EPS);

    DvzVisualDataView node_size_view = {0};
    AT(dvz_visual_data(node_visual, "diameter", &node_size_view) == 0);
    AT(node_size_view.item_count == 3);
    const float* sizes = node_size_view.data;
    AC(sizes[0], 10.0f, EPS);
    AC(sizes[2], 30.0f, EPS);

    DvzVisualDataView edge_start_view = {0};
    DvzVisualDataView edge_end_view = {0};
    DvzVisualDataView edge_width_view = {0};
    AT(dvz_visual_data(edge_visual, "position_start", &edge_start_view) == 0);
    AT(dvz_visual_data(edge_visual, "position_end", &edge_end_view) == 0);
    AT(dvz_visual_data(edge_visual, "stroke_width", &edge_width_view) == 0);
    AT(edge_start_view.item_count == 2);
    const float* starts = edge_start_view.data;
    const float* ends = edge_end_view.data;
    const float* widths = edge_width_view.data;
    AC(starts[0], 0.0f, EPS);
    AC(ends[0], 1.0f, EPS);
    AC(widths[0], 2.0f, EPS);
    AC(widths[1], 4.0f, EPS);

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = DVZ_GRAPH_EDGE_MODE_BEZIER;
    edge_style.tessellation.segment_count = 4;
    AT(dvz_graph_set_edge_style(graph, &edge_style) == 0);
    _scene_prepare_composite_visuals(figure);
    DvzVisual* path_edges = dvz_composite_visual(composite, "edges");
    ANN(path_edges);
    AT(path_edges->type == DVZ_VISUAL_TYPE_PATH);
    AT(path_edges->visible);
    AT(!edge_visual->visible);
    DvzVisualDataView path_position_view = {0};
    AT(dvz_visual_data(path_edges, "position", &path_position_view) == 0);
    AT(path_position_view.item_count == 10);
    AT(_visual_family_state(path_edges)->path.subpath_count == 2);
    AT(_visual_family_state(path_edges)->path.subpath_lengths[0] == 5);
    AT(_visual_family_state(path_edges)->path.subpath_lengths[1] == 5);

    const dvec3 moved[1] = {{2.0, 0.0, 0.0}};
    AT(dvz_graph_node_positions(graph, 1, 1, moved) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(node_visual, "position", &node_position_view) == 0);
    node_positions = node_position_view.data;
    AC(node_positions[3], 2.0f, EPS);
    AT(dvz_visual_data(path_edges, "position", &path_position_view) == 0);
    const float* path_positions = path_position_view.data;
    AC(path_positions[12], 2.0f, EPS);

    dvz_graph_destroy(graph);
    AT(dvz_composite_visual_count(composite) == 0);
    AT(!node_visual->visible);
    AT(!path_edges->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_additional_typed_data_uploads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    vec3 positions[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    float sizes[3] = {4.0f, 8.0f, 12.0f};

    DvzVisual* pixel = dvz_pixel(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(pixel);
    ANN(primitive);
    ANN(sphere);

    DvzVisualDataUpdate pixel_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "pixel_size", .data = sizes, .item_count = 3},
    };
    DvzVisualDataUpdate primitive_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "normal", .data = normals, .item_count = 3},
    };
    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "radius", .data = sizes, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(pixel, pixel_updates, 3) == 0);
    AT(dvz_visual_set_data_many(primitive, primitive_updates, 3) == 0);
    AT(dvz_visual_set_data_many(sphere, sphere_updates, 3) == 0);

    DvzVisualDataView pixel_size_view = {0};
    AT(dvz_visual_data(pixel, "pixel_size", &pixel_size_view) == 0);
    const float* stored_pixel_sizes = pixel_size_view.data;
    AT(stored_pixel_sizes[2] == 12.0f);

    DvzVisualDataView primitive_normal_view = {0};
    AT(dvz_visual_data(primitive, "normal", &primitive_normal_view) == 0);
    const float* stored_normals = primitive_normal_view.data;
    AT(stored_normals[2] == 1.0f);

    DvzVisualDataView sphere_radius_view = {0};
    AT(dvz_visual_data(sphere, "radius", &sphere_radius_view) == 0);
    const float* stored_radii = sphere_radius_view.data;
    AT(stored_radii[1] == 8.0f);

    DvzVisual* flat_primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(flat_primitive);
    DvzVisualDataUpdate flat_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(flat_primitive, flat_updates, 2) == 0);
    DvzVisualDataView flat_normal_view = {0};
    AT(dvz_visual_data(flat_primitive, "normal", &flat_normal_view) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_typed_upload_rejects_wrong_family(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(point);
    ANN(mesh);
    ANN(primitive);
    ANN(sphere);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    float diameters[1] = {4.0f};
    uint32_t item_state[1] = {DVZ_ITEM_STATE_SELECTED};

    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(mesh, "diameter", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(point, "normal", positions, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(primitive, "radius", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(sphere, "pixel_size", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(point, "item_state", item_state, 0) == -1);

    DvzVisualDataUpdate mismatch[] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 2},
    };
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_many(point, mismatch, 2) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_external_position_buffer_emits_no_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX,
        .stride = sizeof(vec3),
        .byte_size = 3 * sizeof(vec3),
    };
    DvzSceneBuffer* position = dvz_scene_buffer(scene, &desc);
    ANN(position);
    AT(dvz_visual_set_attr_buffer(visual, "position", position, 0, 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char position_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(position, position_key, sizeof(position_key)));
    uint64_t position_buffer_id = dvz_drp2_stream_label_id(stream, position_key);
    AT(position_buffer_id != 0);

    uint32_t create_count = 0;
    uint32_t write_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER &&
            cmd->u.create_buffer.id == position_buffer_id)
        {
            create_count++;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.buffer_id == position_buffer_id)
        {
            write_count++;
        }
    }
    AT(create_count == 0);
    AT(write_count == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_storage_position_buffer_emits_usage(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.75f, -0.75f, 0.0f},
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
        .stride = sizeof(vec3),
    };
    DvzSceneBuffer* position = dvz_scene_buffer(scene, &desc);
    ANN(position);
    AT(dvz_scene_buffer_set_data(position, positions, sizeof(positions)));
    AT(dvz_visual_set_attr_buffer(visual, "position", position, sizeof(vec3), 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
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
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
            cmd->u.set_vertex_buffer.slot == 0)
        {
            position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            AT(cmd->u.set_vertex_buffer.offset == 0);
        }
    }
    AT(position_buffer_id != 0);

    bool found_create = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER &&
            cmd->u.create_buffer.id == position_buffer_id)
        {
            found_create = true;
            AT((cmd->u.create_buffer.usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) != 0);
            AT((cmd->u.create_buffer.usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) != 0);
        }
    }
    AT(found_create);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_descriptor_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    DvzSceneBufferDesc buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.struct_size = 0;
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_VERTEX;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_buffer(scene, &buffer_desc) == NULL);

    buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.flags = 1;
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_VERTEX;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_buffer(scene, &buffer_desc) == NULL);

    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.struct_size = DVZ_STRUCT_SIZE(DvzSampledFieldDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_sampled_field(scene, &field_desc) == NULL);

    field_desc = dvz_sampled_field_desc();
    field_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_sampled_field(scene, &field_desc) == NULL);

    DvzSceneComputeDesc compute_desc = dvz_scene_compute_desc();
    compute_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_compute(scene, &compute_desc) == NULL);

    compute_desc = dvz_scene_compute_desc();
    compute_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_compute(scene, &compute_desc) == NULL);

    DvzVisualAttachDesc attach_desc = dvz_visual_attach_desc();
    attach_desc.struct_size = DVZ_STRUCT_SIZE(DvzVisualAttachDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    attach_desc = dvz_visual_attach_desc();
    attach_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    attach_desc = dvz_visual_attach_desc();
    attach_desc.coord_space = (DvzVisualCoordSpace)999;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    attach_desc = dvz_visual_attach_desc();
    attach_desc.controller_mode = (DvzControllerMode)999;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    DvzPanelBackgroundDesc background_desc = dvz_panel_background_desc();
    background_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_background(panel, &background_desc));

    background_desc = dvz_panel_background_desc();
    background_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_background(panel, &background_desc));

    DvzPanelBorderDesc border_desc = dvz_panel_border_desc();
    border_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_border(panel, &border_desc));

    border_desc = dvz_panel_border_desc();
    border_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_border(panel, &border_desc));

    DvzQueryRequest request = dvz_query_request();
    request.struct_size = DVZ_STRUCT_SIZE(DvzQueryRequest) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_query(panel, 0.0, 0.0, &request) < 0);

    request = dvz_query_request();
    request.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_query(panel, 0.0, 0.0, &request) < 0);

    DvzEdlDesc edl = dvz_edl_desc();
    edl.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_edl(panel, &edl));

    edl = dvz_edl_desc();
    edl.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_edl(panel, &edl));

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.struct_size = DVZ_STRUCT_SIZE(DvzMsaaDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_msaa(panel, &msaa));

    msaa = dvz_msaa_desc();
    msaa.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_msaa(panel, &msaa));

    DvzSsaoDesc ssao = dvz_ssao_desc();
    ssao.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_ssao(panel, &ssao));

    ssao = dvz_ssao_desc();
    ssao.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_ssao(panel, &ssao));

    DvzVolumeOcclusionDesc volume_occlusion = dvz_volume_occlusion_desc();
    volume_occlusion.struct_size = DVZ_STRUCT_SIZE(DvzVolumeOcclusionDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_panel_set_volume_occluder(panel, NULL, &volume_occlusion) < 0);

    volume_occlusion = dvz_volume_occlusion_desc();
    volume_occlusion.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_panel_set_volume_occluder(panel, NULL, &volume_occlusion) < 0);

    DvzSceneOcclusionDesc scene_occlusion = dvz_scene_occlusion_desc();
    scene_occlusion.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_set_scene_occlusion(panel, &scene_occlusion) < 0);

    scene_occlusion = dvz_scene_occlusion_desc();
    scene_occlusion.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_set_scene_occlusion(panel, &scene_occlusion) < 0);

    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.struct_size = DVZ_STRUCT_SIZE(DvzPointStyleDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_point_set_style(visual, &point_style) < 0);

    point_style = dvz_point_style_desc();
    point_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_point_set_style(visual, &point_style) < 0);

    DvzMarkerStyle marker_style = dvz_marker_style();
    marker_style.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_marker_set_style(marker, &marker_style) < 0);

    marker_style = dvz_marker_style();
    marker_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_marker_set_style(marker, &marker_style) < 0);

    DvzVectorStyle vector_style = dvz_vector_style();
    vector_style.struct_size = DVZ_STRUCT_SIZE(DvzVectorStyle) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_vector_set_style(vector, &vector_style) < 0);

    vector_style = dvz_vector_style();
    vector_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_vector_set_style(vector, &vector_style) < 0);

    DvzMaterialDesc material = dvz_material_desc();
    material.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &material) < 0);

    material = dvz_material_desc();
    material.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &material) < 0);

    DvzDepthCueDesc depth_cue = dvz_depth_cue_desc();
    depth_cue.struct_size = DVZ_STRUCT_SIZE(DvzDepthCueDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_depth_cue(visual, &depth_cue) < 0);

    depth_cue = dvz_depth_cue_desc();
    depth_cue.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_depth_cue(visual, &depth_cue) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_compute_point_position_buffer_emits_drp2(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    DvzSceneBuffer* position = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = sizeof(positions),
               });
    ANN(position);
    AT(dvz_scene_buffer_set_data(position, positions, sizeof(positions)));
    AT(dvz_visual_set_attr_buffer(visual, "position", position, 0, 3));

    vec4 params = {0.0f, 0.0f, 3.0f, 0.0f};
    DvzSceneBuffer* param = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec4),
                   .byte_size = sizeof(params),
               });
    ANN(param);
    AT(dvz_scene_buffer_set_data(param, &params, sizeof(params)));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    const char* shader =
        "#version 450\n"
        "layout(local_size_x = 1) in;\n"
        "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 p; } params;\n"
        "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
        "void main() {\n"
        "    uint i = gl_GlobalInvocationID.x;\n"
        "    if (i >= uint(params.p.z)) return;\n"
        "    positions.x[3u * i + 0u] += 0.0;\n"
        "}\n";
    DvzSceneCompute* compute = dvz_scene_compute(
        scene, &(DvzSceneComputeDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneComputeDesc),
                   .label = "test_compute_points",
                   .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
                   .shader_source = shader,
                   .dispatch = {3, 1, 1},
               });
    ANN(compute);
    AT(dvz_scene_compute_set_buffer(
        compute, 0, param, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(params)));
    AT(dvz_scene_compute_set_buffer(
        compute, 1, position, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0, sizeof(positions)));
    AT(dvz_figure_add_compute(figure, compute));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    uint64_t position_buffer_id = 0;
    bool found_compute_pipeline = false;
    bool found_dispatch = false;
    bool found_barrier = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
            cmd->u.set_vertex_buffer.slot == 0)
            position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        found_compute_pipeline =
            found_compute_pipeline || cmd->type == DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE;
        found_dispatch = found_dispatch || cmd->type == DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS;
    }
    AT(position_buffer_id != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_RESOURCE_BARRIER &&
            cmd->u.resource_barrier.buffer_id == position_buffer_id)
            found_barrier = true;
    }
    AT(found_compute_pipeline);
    AT(found_dispatch);
    AT(found_barrier);

    _test_scene_stream_destroy(stream);

    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream2);

    uint32_t create_bind_group_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            create_bind_group_count++;
    }
    AT(create_bind_group_count == 0);
    _test_scene_stream_destroy(stream2);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_external_position_buffer_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "test_scene_point_external_position_buffer_executes skipped: GPU context creation "
            "failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    uint64_t position_bytes = sizeof(positions);

    DvzBuffer* runtime_position = dvz_buffer_create_wrapper();
    ANN(runtime_position);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), runtime_position);
    dvz_buffer_size(runtime_position, position_bytes);
    dvz_buffer_usage(runtime_position, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(runtime_position, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT(dvz_buffer_create(runtime_position) == 0);
    dvz_buffer_upload(runtime_position, 0, position_bytes, positions);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX,
        .stride = sizeof(vec3),
        .byte_size = position_bytes,
    };
    DvzSceneBuffer* scene_position = dvz_scene_buffer(scene, &desc);
    ANN(scene_position);
    AT(dvz_visual_set_attr_buffer(visual, "position", scene_position, 0, 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {8.0f, 8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char position_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(scene_position, position_key, sizeof(position_key)));
    uint64_t position_buffer_id = dvz_drp2_stream_label_id(stream, position_key);
    AT(position_buffer_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ExternalBufferDesc external = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = runtime_position,
        .size = position_bytes,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    AT(dvz_drp2_runtime_register_external_buffer(runtime, position_buffer_id, &external));

    DvzSemaphore* ready = dvz_semaphore_create_wrapper();
    ANN(ready);
    dvz_semaphore_timeline(dvz_gpu_ctx_device(ctx), 0, ready, 0);
    dvz_semaphore_signal(ready, 1);
    AT(dvz_interop_buffer_wait_timeline(
        dvz_gpu_ctx_device(ctx), runtime_position, position_bytes, ready, 1));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_semaphore_destroy(ready);
    dvz_semaphore_free(ready);
    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
    dvz_buffer_destroy(runtime_position);
    dvz_buffer_free(runtime_position);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_point_rejects_texcoords_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float uv[2] = {0.0f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "texcoords", uv, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'texcoords'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_rejects_size_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported primitive visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_path_rejects_size_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_path(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported path visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_rejects_size_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported image visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_emit_warns_visual_with_no_position(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Emit with no position set — should warn but not crash. */
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    tst_log_capture_begin(suite);
    DvzDrp2CommandStream* stream = NULL;
    AT_EXPECTED_LOG_STRICT(
        suite, LOG_WARN, (stream = _test_scene_emit_stream(figure, &caps, &report)) != NULL);
    AT(stream != NULL);
    AT(_captured_log_contains(suite, "has no 'position' data"));

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_mismatched_point_attribute_counts(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {
        -0.25f, 0.00f, 0.0f,
         0.25f, 0.00f, 0.0f,
    };
    DvzColor color = {255, 0, 0, 255};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "color", &color, 1) == -1);
    AT(_captured_log_contains(suite, "item_count 1 does not match existing attribute 'position'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_visual_resizes_existing_attributes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions3[3 * 3] = {
        -0.50f, 0.00f, 0.0f,
         0.00f, 0.00f, 0.0f,
         0.50f, 0.00f, 0.0f,
    };
    DvzColor colors3[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    float sizes3[3] = {3.0f, 4.0f, 5.0f};

    float positions2[2 * 3] = {
        -0.25f, 0.00f, 0.0f,
         0.25f, 0.00f, 0.0f,
    };
    DvzColor colors2[2] = {
        {255, 255, 0, 255},
        {0, 255, 255, 255},
    };
    float sizes2[2] = {6.0f, 7.0f};

    AT(dvz_visual_set_data(visual, "position", positions3, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors3, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes3, 3) == 0);

    DvzVisualDataUpdate partial[] = {
        {.attr_name = "position", .data = positions2, .item_count = 2},
    };
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_many(visual, partial, 1) == -1);
    AT(_captured_log_contains(suite, "omits existing attribute 'color'"));

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions2, .item_count = 2},
        {.attr_name = "color", .data = colors2, .item_count = 2},
        {.attr_name = "size", .data = sizes2, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(visual, updates, 3) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_range_update_without_full_allocation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float update[3] = {0.5f, 0.0f, 0.0f};

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data_range(visual, "position", update, 0, 1) == -1);
    AT(_captured_log_contains(suite, "range update requires prior full allocation"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_stream_allows_mutation_after_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);

    float update[2 * 3] = {-0.5f, 0.1f, 0.0f, 0.5f, 0.1f, 0.0f};
    AT(dvz_visual_set_data(visual, "position", update, 2) == 0);
    AT(dvz_drp2_stream_count(stream) > 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_stream_snapshot_freezes_upload_payloads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(stream != NULL);

    float update[2] = {10.0f, 12.0f};
    AT(dvz_visual_set_data_range(visual, "size", update, 0, 2) == 0);

    bool found_size_upload = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.size != 2 * sizeof(float) || cmd->u.write_buffer.data_raw == NULL)
        {
            continue;
        }
        const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
        if (uploaded[0] == 8.0f && uploaded[1] == 8.0f)
            found_size_upload = true;
    }
    AT(found_size_upload);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_stream_survives_scene_destroy_after_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 12.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream) > 0);

    dvz_scene_destroy(scene);
    AT(dvz_drp2_stream_count(stream) > 0);
    _test_scene_stream_destroy(stream);
    return 0;
}


int test_scene_artifact_allows_mutation_after_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(visual->scene == scene);
    AT(visual->attr_count == 3);
    for (uint32_t i = 0; i < visual->attr_count; i++)
        AT(visual->attrs[i].data != NULL);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzSceneFrameArtifact* artifact = dvz_figure_emit_frame(figure, &caps, &report, NULL);
    AT(artifact != NULL);
    AT(dvz_scene_frame_artifact_status(artifact) == DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK);
    AT(dvz_scene_frame_artifact_resource_version(artifact) == 1);
    AT(dvz_scene_frame_artifact_frame_index(artifact) == 1);

    const DvzDrp2CommandStream* artifact_stream = dvz_scene_frame_artifact_stream(artifact);
    AT(artifact_stream != NULL);
    AT(dvz_drp2_stream_count(artifact_stream) > 0);

    char* json = dvz_scene_frame_artifact_json(artifact, "scene_artifact_test");
    AT(json != NULL);
    AT(strstr(json, "\"commands\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    const void* packet = NULL;
    uint64_t packet_size = 0;
    const void* arena = NULL;
    uint64_t arena_size = 0;
    AT(dvz_scene_frame_artifact_get_packet(
        artifact, DVZ_DRP2_PACKET_FRAME, &packet, &packet_size, &arena, &arena_size));
    AT(packet != NULL);
    AT(packet_size > 0);
    (void)arena;
    (void)arena_size;

    float update[2] = {10.0f, 12.0f};
    AT(dvz_visual_set_data_range(visual, "size", update, 0, 2) == 0);
    AT(dvz_drp2_stream_count(artifact_stream) > 0);

    dvz_scene_frame_artifact_destroy(artifact);
    dvz_scene_destroy(scene);
    return 0;
}
