/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field and scale tests                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_overflow.h"
#include "_scene.h"
#include "scene_emit/internal.h"
#include "scene_emit/scene_emit.h"
#include "colorizer.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_mesh_visual_binds_texture_field(TstContext* suite, const TstCase* item);


/**
 * Return whether a stream contains a render pipeline label fragment.
 *
 * @param stream the emitted command stream
 * @param label the label fragment
 * @return whether a matching pipeline was found
 */
static bool _colorbar_stream_has_pipeline_label(
    const DvzDrp2CommandStream* stream, const char* label)
{
    ANN(stream);
    ANN(label);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* pipeline_label =
            dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (pipeline_label != NULL && strstr(pipeline_label, label) != NULL)
            return true;
    }
    return false;
}


/**
 * Return whether a stream contains a sparse volume-label lookup buffer.
 *
 * @param stream the emitted command stream
 * @param out_buffer_id optional output buffer id
 * @param out_entries optional output lookup entries
 * @param out_entry_count optional output entry count
 * @return whether a lookup payload was found
 */
static bool _stream_find_volume_label_lookup(
    const DvzDrp2CommandStream* stream, uint64_t* out_buffer_id,
    const DvzSceneLabelLookupEntry** out_entries, uint32_t* out_entry_count)
{
    ANN(stream);
    if (out_buffer_id != NULL)
        *out_buffer_id = 0;
    if (out_entries != NULL)
        *out_entries = NULL;
    if (out_entry_count != NULL)
        *out_entry_count = 0;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
        if (label == NULL || strstr(label, "volume_label_lookup") == NULL)
            continue;
        if (cmd->u.write_buffer.data_raw == NULL ||
            cmd->u.write_buffer.size < sizeof(DvzSceneLabelLookupEntry))
            continue;
        if (out_buffer_id != NULL)
            *out_buffer_id = cmd->u.write_buffer.buffer_id;
        if (out_entries != NULL)
            *out_entries = (const DvzSceneLabelLookupEntry*)cmd->u.write_buffer.data_raw;
        if (out_entry_count != NULL)
            *out_entry_count = (uint32_t)(cmd->u.write_buffer.size /
                                          sizeof(DvzSceneLabelLookupEntry));
        return true;
    }
    return false;
}


/**
 * Return whether a stream binds the lookup buffer at volume binding 5.
 *
 * @param stream the emitted command stream
 * @param buffer_id expected lookup buffer id
 * @return whether the binding was found
 */
static bool _stream_binds_volume_label_lookup(const DvzDrp2CommandStream* stream, uint64_t buffer_id)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            continue;
        for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
        {
            const DvzDrp2BindGroupEntry* entry = &cmd->u.create_bind_group.entries[j];
            if (entry->binding == 5 &&
                entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER &&
                entry->resource_id == buffer_id)
            {
                return true;
            }
        }
    }
    return false;
}



int test_scene_scale_colormap_colorbar_core(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 100, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Depth",
                   .unit = "um",
                   .format =
                       (DvzFormatDesc){
                           .precision = 2,
                           .show_unit = true,
                           .unit = "um",
                           .suffix = " depth",
                       },
               });
    ANN(scale);
    AT(scene->scale_count == 1);
    AT(scale->scene == scene);
    AT(scale->kind == DVZ_SCALE_CONTINUOUS);
    AT(strcmp(scale->label, "Depth") == 0);
    AT(strcmp(scale->unit, "um") == 0);
    AT(scale->format.precision == 2);
    AT(strcmp(scale->format.unit, "um") == 0);
    AT(strcmp(scale->format.suffix, " depth") == 0);

    dvz_scale_set_domain(scale, -600.0, 0.0);
    dvz_scale_set_view_range(scale, -300.0, -50.0);
    AT(scale->has_domain);
    AT(scale->domain_min == -600.0);
    AT(scale->domain_max == 0.0);
    AT(scale->has_view_range);
    AT(scale->view_min == -300.0);
    AT(scale->view_max == -50.0);

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_MAGMA);
    ANN(colormap);
    AT(scene->colormap_count == 1);
    AT(colormap->builtin == DVZ_BUILTIN_COLORMAP_MAGMA);
    uint8_t builtin_rgba[4] = {0};
    AT(_scene_color_from_colormap(colormap, 0.5, builtin_rgba));
    AT(builtin_rgba[0] != builtin_rgba[1]);

    DvzColor public_rgba = {0};
    AT(dvz_colormap_sample(colormap, 0.5, &public_rgba));
    AT(public_rgba.r == builtin_rgba[0]);
    AT(public_rgba.g == builtin_rgba[1]);
    AT(public_rgba.b == builtin_rgba[2]);
    AT(public_rgba.a == builtin_rgba[3]);

    DvzColor direct_rgba = {0};
    AT(dvz_colormap_builtin_sample(DVZ_BUILTIN_COLORMAP_MAGMA, 0.5, &direct_rgba));
    AT(direct_rgba.r == builtin_rgba[0]);
    AT(direct_rgba.g == builtin_rgba[1]);
    AT(direct_rgba.b == builtin_rgba[2]);
    AT(direct_rgba.a == builtin_rgba[3]);

    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    dvz_colormap_set_center(colormap, 0.5);
    dvz_colormap_set_stops(colormap, stops, 2);
    AT(colormap->has_center);
    AT(colormap->center == 0.5);
    AT(colormap->stop_count == 2);
    AT(colormap->stops[1].rgba[0] == 255);

    dvz_scale_set_colormap(scale, colormap);
    AT(scale->colormap == colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale, &(DvzColorbarDesc){
                          .orientation = DVZ_COLORBAR_ORIENTATION_HORIZONTAL,
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_BOTTOM,
                          .title = "Depth map",
                      });
    ANN(colorbar);
    AT(scene->colorbar_count == 1);
    AT(panel->colorbar_count == 1);
    AT(panel->colorbars[0] == colorbar);
    AT(colorbar->scene == scene);
    AT(colorbar->panel == panel);
    AT(colorbar->scale == scale);
    AT(colorbar->orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL);
    AT(colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM);
    AT(colorbar->text_renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(strcmp(colorbar->title, "Depth map") == 0);

    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){
                       .precision = 0,
                       .unit = "um",
                   });
    AT(colorbar->has_format);
    AT(colorbar->format.precision == 0);
    AT(strcmp(colorbar->format.unit, "um") == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_categorical_scale_entries(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzScale* categorical = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(categorical);
    DvzScaleCategory categories[3] = {
        {.category_id = 7, .order = 2, .label = "Beta", .color = {220, 40, 40, 255}},
        {.category_id = -3, .order = 1, .label = "Alpha", .color = {40, 220, 40, 255}},
        {.category_id = 4000000000LL, .order = 3, .label = NULL, .color = {40, 40, 220, 255}},
    };
    AT(dvz_scale_set_categories(categorical, categories, 3));
    AT(categorical->category_count == 3);
    AT(categorical->categories[0].category_id == 7);
    AT(categorical->categories[0].order == 2);
    AT(categorical->categories[0].has_label);
    AT(strcmp(categorical->categories[0].label, "Beta") == 0);
    AT(categorical->categories[0].color.r == 220);
    AT(categorical->categories[1].category_id == -3);
    AT(categorical->categories[2].category_id == 4000000000LL);
    AT(!categorical->categories[2].has_label);

    DvzScaleCategory duplicate[2] = {
        {.category_id = -4, .order = 0, .label = "First", .color = {1, 2, 3, 255}},
        {.category_id = -4, .order = 1, .label = "Second", .color = {4, 5, 6, 255}},
    };
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_scale_set_categories(categorical, duplicate, 2));
    AT(categorical->category_count == 3);
    AT(categorical->categories[0].category_id == 7);

    DvzScaleCategory patch[2] = {
        {.category_id = -3, .order = 4, .label = "Updated", .color = {10, 20, 30, 255}},
        {.category_id = -100, .order = 0, .label = NULL, .color = {80, 90, 100, 255}},
    };
    AT(dvz_scale_update_categories(categorical, patch, 2));
    AT(categorical->category_count == 4);
    AT(categorical->categories[1].category_id == -3);
    AT(categorical->categories[1].order == 4);
    AT(strcmp(categorical->categories[1].label, "Updated") == 0);
    AT(categorical->categories[3].category_id == -100);

    DvzCategoryId remove_ids[2] = {-100, 12345};
    AT(dvz_scale_remove_categories(categorical, remove_ids, 2));
    AT(categorical->category_count == 3);
    AT(categorical->categories[2].category_id == 4000000000LL);

    DvzScaleCategory many_categories[96] = {0};
    for (uint32_t i = 0; i < 96; i++)
    {
        many_categories[i] = (DvzScaleCategory){
            .category_id = (DvzCategoryId)(1000 + i),
            .order = i,
            .label = NULL,
            .color = {(uint8_t)i, 0, 255, 255},
        };
    }
    AT(dvz_scale_set_categories(categorical, many_categories, 96));
    AT(categorical->category_count == 96);
    AT(categorical->category_capacity >= 96);
    AT(categorical->categories[95].category_id == 1095);

    DvzScale* continuous = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(continuous);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_scale_set_categories(continuous, categories, 3));
    AT(continuous->category_count == 0);

    AT(dvz_scale_set_categories(categorical, NULL, 0));
    AT(categorical->category_count == 0);
    AT(categorical->category_capacity == 0);
    AT(categorical->categories == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_legend_lifecycle_and_reserve(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 300, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[2] = {
        {.category_id = 1, .order = 0, .label = "One", .color = {255, 0, 0, 255}},
        {.category_id = 2, .order = 1, .label = "Two", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));

    DvzLegend* legend = dvz_legend(
        panel, scale, &(DvzLegendDesc){
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                          .title = "Classes",
                          .reserve_px = 120.0f,
                          .mark_size_px = 10.0f,
                      });
    ANN(legend);
    AT(scene->legend_count == 1);
    AT(panel->legend_count == 1);
    AT(panel->legends[0] == legend);
    AT(legend->scene == scene);
    AT(legend->panel == panel);
    AT(legend->scale == scale);
    AT(legend->placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED);
    AT(legend->anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT);
    AT(strcmp(legend->title, "Classes") == 0);
    AT(fabsf(panel->legend_reserve.right_px - 120.0f) < 1e-6f);
    AT(fabsf(panel->reserve.right_px - 120.0f) < 1e-6f);

    uint64_t version = legend->version;
    dvz_legend_set_title(legend, "Updated");
    AT(strcmp(legend->title, "Updated") == 0);
    AT(legend->dirty);
    AT(legend->version > version);

    AT(dvz_legend_set_layout(
        legend, &(DvzLegendDesc){
                    .placement_mode = DVZ_LEGEND_PLACEMENT_DETACHED,
                    .title = "Detached",
                    .text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                    .placement = {
                        .space = DVZ_PLACEMENT_SPACE_FIGURE,
                        .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
                        .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
                        .offset_x_px = -12.0f,
                        .offset_y_px = 16.0f,
                        .width_px = 80.0f,
                        .height_px = 120.0f,
                    },
                }));
    AT(legend->placement_mode == DVZ_LEGEND_PLACEMENT_DETACHED);
    AT(legend->text_renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AT(fabsf(panel->legend_reserve.right_px) < 1e-6f);
    AT(fabsf(panel->reserve.right_px) < 1e-6f);

    dvz_legend_destroy(legend);
    AT(panel->legend_count == 0);
    AT(legend->scene == NULL);
    AT(legend->panel == NULL);
    AT(legend->scale == NULL);

    DvzScale* continuous = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(continuous);
    DvzLegend* invalid = NULL;
    AT_EXPECTED_ERROR_STRICT(suite, (invalid = dvz_legend(panel, continuous, NULL)) == NULL);
    AT(invalid == NULL);

    DvzScene* other_scene = dvz_scene();
    ANN(other_scene);
    DvzScale* foreign = dvz_scale(other_scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(foreign);
    AT_EXPECTED_ERROR_STRICT(suite, (invalid = dvz_legend(panel, foreign, NULL)) == NULL);
    AT(invalid == NULL);
    dvz_scene_destroy(other_scene);

    AT_EXPECTED_ERROR_STRICT(
        suite,
        (invalid = dvz_legend(
             panel, scale, &(DvzLegendDesc){.anchor = DVZ_SCENE_ANCHOR_PANEL_CENTER})) == NULL);
    AT(invalid == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_legend_prepare_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 500, 320, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[4] = {
        {.category_id = 7, .order = 2, .label = "Beta", .color = {220, 40, 40, 255}},
        {.category_id = 3, .order = 1, .label = "Alpha", .color = {40, 220, 40, 255}},
        {.category_id = -7, .order = 3, .label = NULL, .color = {40, 40, 220, 255}},
        {.category_id = 4000000000LL, .order = 4, .label = NULL, .color = {220, 220, 40, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 4));

    DvzLegend* legend = dvz_legend(
        panel, scale, &(DvzLegendDesc){
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                          .title = "Classes",
                          .reserve_px = 150.0f,
                          .mark_size_px = 12.0f,
                      });
    ANN(legend);

    _scene_prepare_legend_visuals(figure, NULL);
    AT(legend->mark_visual != NULL);
    AT(legend->text_visual != NULL);
    AT(legend->text_visual->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(legend->mark_visual->visible);
    AT(legend->text_visual->visible);
    AT(legend->entry_count == 4);
    AT(legend->text_count == 5);
    AT(strcmp(legend->text_labels[0], "Classes") == 0);
    AT(strcmp(legend->text_labels[1], "Alpha") == 0);
    AT(strcmp(legend->text_labels[2], "Beta") == 0);
    AT(strcmp(legend->text_labels[3], "-7") == 0);
    AT(strcmp(legend->text_labels[4], "4000000000") == 0);

    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(legend->mark_visual, "color", &color_view) == 0);
    AT(color_view.item_count == 4);
    const uint8_t* colors = color_view.data;
    ANN(colors);
    AT(colors[1] == 220);
    AT(colors[4] == 220);
    AT(colors[10] == 220);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(legend->mark_visual, "position", &position_view) == 0);
    AT(position_view.item_count == 4);
    const float* positions = (const float*)position_view.data;
    ANN(positions);
    AT(positions[0] > 0.0f);

    DvzVisualDataView text_position_view = {0};
    AT(dvz_visual_data(legend->text_visual, "position", &text_position_view) == 0);
    AT(text_position_view.item_count == 5);

    DvzVisualDataView first_position = {0};
    AT(dvz_visual_data(legend->mark_visual, "position", &first_position) == 0);
    const void* first_ptr = first_position.data;
    _scene_prepare_legend_visuals(figure, NULL);
    AT(dvz_visual_data(legend->mark_visual, "position", &first_position) == 0);
    AT(first_position.data == first_ptr);

    uint64_t version = legend->version;
    AT(dvz_legend_set_highlight(legend, -7));
    AT(legend->highlight_count == 1);
    AT(legend->highlighted_ids[0] == -7);
    AT(legend->dirty);
    AT(legend->version > version);
    _scene_prepare_legend_visuals(figure, NULL);
    DvzVisualDataView size_view = {0};
    AT(dvz_visual_data(legend->mark_visual, "diameter", &size_view) == 0);
    AT(size_view.item_count == 4);
    const float* sizes = (const float*)size_view.data;
    ANN(sizes);
    AT(fabsf(sizes[0] - 12.0f) < 1e-6f);
    AT(fabsf(sizes[1] - 12.0f) < 1e-6f);
    AT(sizes[2] > 12.0f);
    AT(fabsf(sizes[3] - 12.0f) < 1e-6f);

    DvzCategoryId highlights[2] = {-7, 4000000000LL};
    AT(dvz_legend_set_highlights(legend, highlights, 2));
    _scene_prepare_legend_visuals(figure, NULL);
    AT(dvz_visual_data(legend->mark_visual, "diameter", &size_view) == 0);
    sizes = (const float*)size_view.data;
    ANN(sizes);
    AT(sizes[2] > 12.0f);
    AT(sizes[3] > 12.0f);

    AT(dvz_legend_clear_highlight(legend));
    _scene_prepare_legend_visuals(figure, NULL);
    AT(dvz_visual_data(legend->mark_visual, "diameter", &size_view) == 0);
    sizes = (const float*)size_view.data;
    ANN(sizes);
    AT(fabsf(sizes[0] - 12.0f) < 1e-6f);
    AT(fabsf(sizes[1] - 12.0f) < 1e-6f);
    AT(fabsf(sizes[2] - 12.0f) < 1e-6f);
    AT(fabsf(sizes[3] - 12.0f) < 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_legend_emit_stream_contains_derived_visuals(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[2] = {
        {.category_id = 1, .order = 0, .label = "A", .color = {255, 80, 80, 255}},
        {.category_id = 2, .order = 1, .label = "B", .color = {80, 255, 80, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    DvzLegend* legend = dvz_legend(
        panel, scale, &(DvzLegendDesc){
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                          .title = "Group",
                      });
    ANN(legend);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(legend->mark_visual != NULL);
    AT(legend->text_visual != NULL);
    AT(legend->text_visual->text.glyph_visual != NULL);

    bool found_mark_position_label = false;
    bool found_mark_color_label = false;
    bool found_mark_shape_label = false;
    bool found_glyph_position_label = false;
    bool found_glyph_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label == NULL)
                continue;
            found_mark_position_label =
                found_mark_position_label || strcmp(label, "legend.0.marks.position") == 0;
            found_mark_color_label =
                found_mark_color_label || strcmp(label, "legend.0.marks.color") == 0;
            found_mark_shape_label =
                found_mark_shape_label || strcmp(label, "legend.0.marks.shape") == 0;
            found_glyph_position_label =
                found_glyph_position_label ||
                strcmp(label, "legend.0.labels.glyph.position") == 0;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW && cmd->u.draw.vertex_count > 0 &&
                 cmd->u.draw.vertex_count % 6 == 0)
        {
            found_glyph_draw = true;
        }
    }
    AT(found_mark_position_label);
    AT(found_mark_color_label);
    AT(found_mark_shape_label);
    AT(found_glyph_position_label);
    AT(found_glyph_draw);
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_glyphg"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_colorbar_auto_reserve_and_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .unit = "u",
                   .format = (DvzFormatDesc){.precision = 1, .show_unit = true},
               });
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale, &(DvzColorbarDesc){
                          .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                          .title = "Intensity",
                      });
    ANN(colorbar);
    AT(colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED);
    AT(fabsf(colorbar->reserve_px - 140.0f) < 1e-6f);
    AT(fabsf(colorbar->ramp_width_px - 36.0f) < 1e-6f);
    AT(fabsf(colorbar->edge_offset_px) < 1e-6f);
    AT(fabsf(colorbar->plot_gap_px - 12.0f) < 1e-6f);
    DvzPanelReserve reserve = {0};
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);
    DvzRect plot_rect = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.width - 660.0f) < 1e-6f);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(x_axis);
    DvzAxisStyle x_style = x_axis->style;
    x_style.reserve_px = 35.0f;
    AT(dvz_axis_set_style(x_axis, &x_style));
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);
    AT(fabsf(reserve.bottom_px - 35.0f) < 1e-6f);

    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(colorbar->ramp_visual != NULL);
    AT(colorbar->tick_visual != NULL);
    AT(colorbar->text_visual != NULL);
    AT(colorbar->text_visual->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(colorbar->ramp_visual->topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    AT(colorbar->ramp_visual->visible);
    AT(colorbar->tick_visual->visible);
    AT(colorbar->text_visual->visible);
    AT(colorbar->tick_count >= 2);
    AT(colorbar->text_count == colorbar->tick_count + 1);

    DvzVisualDataView pos_view = {0};
    DvzVisualDataView ramp_color_view = {0};
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &pos_view) == 0);
    AT(dvz_visual_data(colorbar->ramp_visual, "color", &ramp_color_view) == 0);
    AT(pos_view.item_count == 6 * 64);
    AT(ramp_color_view.item_count == pos_view.item_count);
    const float* positions = (const float*)pos_view.data;
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    float expected_bottom_y = 1.0f - 2.0f * (plot_rect.y + plot_rect.height) / 600.0f;
    AT(fabsf(positions[1] - expected_bottom_y) < 1e-5f);
    AT(positions[3 * (pos_view.item_count - 1) + 1] > 0.95f);

    DvzFramePlan* plan = dvz_frame_plan("figure.colorbar.clip", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    uint32_t ramp_index = UINT32_MAX;
    uint32_t tick_index = UINT32_MAX;
    AT(_figure_visual_index(figure, colorbar->ramp_visual, &ramp_index));
    AT(_figure_visual_index(figure, colorbar->tick_visual, &tick_index));
    bool found_ramp_clip = false;
    bool found_tick_clip = false;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        if (node->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        for (uint32_t j = 0; j < node->u.render.visual_count; j++)
        {
            const DvzFramePlanVisualMeta* meta = &node->u.render.visual_metadata[j];
            if (!meta->has_metadata)
                continue;
            if (meta->visual_index == ramp_index)
            {
                AT(meta->clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PANEL);
                found_ramp_clip = true;
            }
            if (meta->visual_index == tick_index)
            {
                AT(meta->clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PANEL);
                found_tick_clip = true;
            }
        }
    }
    AT(found_ramp_clip);
    AT(found_tick_clip);
    dvz_frame_plan_destroy(plan);

    dvz_colorbar_set_orientation(colorbar, DVZ_COLORBAR_ORIENTATION_HORIZONTAL);
    AT(colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM);
    AT(dvz_colorbar_set_anchor(colorbar, DVZ_SCENE_ANCHOR_PANEL_BOTTOM));
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px) < 1e-6f);
    AT(fabsf(reserve.bottom_px - 131.0f) < 1e-6f);
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &pos_view) == 0);
    positions = (const float*)pos_view.data;
    AT(positions[0] < -0.95f);
    AT(positions[3 * (pos_view.item_count - 1) + 0] > 0.95f);

    AT(dvz_colorbar_set_layout(
        colorbar, &(DvzColorbarDesc){
                      .placement_mode = DVZ_COLORBAR_PLACEMENT_DETACHED,
                      .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                      .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                      .reserve_px = 80.0f,
                      .ramp_width_px = 24.0f,
                      .placement = {
                          .space = DVZ_PLACEMENT_SPACE_PANEL,
                          .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
                          .vertical_anchor = DVZ_VERTICAL_ANCHOR_CENTER,
                          .offset_x_px = -24.0f,
                          .width_px = 48.0f,
                          .height_px = 240.0f,
                      },
                  }));
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_DETACHED);
    AT(fabsf(colorbar->ramp_width_px - 24.0f) < 1e-6f);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px) < 1e-6f);
    AT(fabsf(reserve.bottom_px - 35.0f) < 1e-6f);

    AT(dvz_colorbar_set_layout(
        colorbar, &(DvzColorbarDesc){
                      .placement_mode = DVZ_COLORBAR_PLACEMENT_ATTACHED,
                      .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                      .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                      .reserve_px = 80.0f,
                      .ramp_width_px = 20.0f,
                      .title = "Live layout",
                  }));
    AT(strcmp(colorbar->title, "Live layout") == 0);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 80.0f) < 1e-6f);
    AT(fabsf(reserve.bottom_px - 35.0f) < 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify repeated colorbar preparation does not replace clean retained buffers.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_prepare_is_idempotent(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    ANN(colorbar);

    _scene_prepare_colorbar_visuals(figure, NULL);
    DvzVisualDataView ramp_pos = {0};
    DvzVisualDataView ramp_color = {0};
    DvzVisualDataView tick_pos = {0};
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &ramp_pos) == 0);
    AT(dvz_visual_data(colorbar->ramp_visual, "color", &ramp_color) == 0);
    AT(dvz_visual_data(colorbar->tick_visual, "position_start", &tick_pos) == 0);
    const void* ramp_pos_data = ramp_pos.data;
    const void* ramp_color_data = ramp_color.data;
    const void* tick_pos_data = tick_pos.data;
    uint64_t ramp_pos_version = ramp_pos.version;
    uint64_t ramp_color_version = ramp_color.version;
    uint64_t tick_pos_version = tick_pos.version;

    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &ramp_pos) == 0);
    AT(dvz_visual_data(colorbar->ramp_visual, "color", &ramp_color) == 0);
    AT(dvz_visual_data(colorbar->tick_visual, "position_start", &tick_pos) == 0);
    AT(ramp_pos.data == ramp_pos_data);
    AT(ramp_color.data == ramp_color_data);
    AT(tick_pos.data == tick_pos_data);
    AT(ramp_pos.version == ramp_pos_version);
    AT(ramp_color.version == ramp_color_version);
    AT(tick_pos.version == tick_pos_version);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify automatic colorbar reserves keep a fixed pixel size across figure resizes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_auto_reserve_tracks_resize(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    ANN(colorbar);

    DvzPanelReserve reserve = {0};
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);

    dvz_figure_resize(figure, 1200, 600);
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);

    dvz_figure_resize(figure, 700, 600);
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify attached colorbars align to plot rects after panel-wide padding.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_attached_respects_panel_padding(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_padding(
        panel, &(DvzPanelReserve){
                   .left_px = 32.0f,
                   .right_px = 24.0f,
                   .top_px = 20.0f,
                   .bottom_px = 16.0f,
               }));
    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){
                   .left_px = 40.0f,
                   .top_px = 20.0f,
                   .bottom_px = 30.0f,
               }));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
        });
    ANN(colorbar);

    DvzPanelReserve reserve = {0};
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.left_px - 40.0f) < 1e-6f);
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);
    AT(fabsf(reserve.top_px - 20.0f) < 1e-6f);
    AT(fabsf(reserve.bottom_px - 30.0f) < 1e-6f);

    DvzRect inner_rect = {0};
    AT(dvz_panel_inner_rect_px(panel, &inner_rect));
    AT(fabsf(inner_rect.x - 32.0f) < 1e-4f);
    AT(fabsf(inner_rect.y - 20.0f) < 1e-4f);
    AT(fabsf(inner_rect.width - 744.0f) < 1e-4f);
    AT(fabsf(inner_rect.height - 564.0f) < 1e-4f);

    DvzRect plot_rect = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 72.0f) < 1e-4f);
    AT(fabsf(plot_rect.y - 40.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 564.0f) < 1e-4f);
    AT(fabsf(plot_rect.height - 514.0f) < 1e-4f);

    _scene_prepare_colorbar_visuals(figure, NULL);
    ANN(colorbar->ramp_visual);
    DvzVisualDataView pos_view = {0};
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &pos_view) == 0);
    AT(pos_view.item_count > 0);
    const float* positions = (const float*)pos_view.data;
    float min_x = +FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = +FLT_MAX;
    float max_y = -FLT_MAX;
    for (uint32_t i = 0; i < pos_view.item_count; i++)
    {
        const float x = positions[3 * i + 0];
        const float y = positions[3 * i + 1];
        min_x = fminf(min_x, x);
        max_x = fmaxf(max_x, x);
        min_y = fminf(min_y, y);
        max_y = fmaxf(max_y, y);
    }

    const float expected_x0 =
        -1.0f + 2.0f * (plot_rect.x + plot_rect.width + colorbar->plot_gap_px) / 800.0f;
    const float expected_x1 = -1.0f + 2.0f *
                                           (plot_rect.x + plot_rect.width +
                                            colorbar->plot_gap_px + colorbar->ramp_width_px) /
                                           800.0f;
    const float expected_y0 = 1.0f - 2.0f * plot_rect.y / 600.0f;
    const float expected_y1 = 1.0f - 2.0f * (plot_rect.y + plot_rect.height) / 600.0f;
    AT(fabsf(min_x - expected_x0) < 1e-5f);
    AT(fabsf(max_x - expected_x1) < 1e-5f);
    AT(fabsf(max_y - expected_y0) < 1e-5f);
    AT(fabsf(min_y - expected_y1) < 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify detached colorbar placement leaves the panel plot rectangle unchanged.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_detached_placement(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){
                   .left_px = 40.0f,
                   .right_px = 20.0f,
                   .top_px = 10.0f,
                   .bottom_px = 30.0f,
               }));
    DvzRect before = {0};
    AT(dvz_panel_plot_rect_px(panel, &before));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .placement_mode = DVZ_COLORBAR_PLACEMENT_DETACHED,
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .title = "Detached",
            .ramp_width_px = 24.0f,
            .text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
            .placement = {
                .space = DVZ_PLACEMENT_SPACE_FIGURE,
                .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
                .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
                .offset_x_px = -32.0f,
                .offset_y_px = 48.0f,
                .width_px = 64.0f,
                .height_px = 320.0f,
            },
        });
    ANN(colorbar);
    AT(colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_DETACHED);
    AT(colorbar->text_renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);

    DvzRect after = {0};
    AT(dvz_panel_plot_rect_px(panel, &after));
    AT(fabsf(after.x - before.x) < 1e-6f);
    AT(fabsf(after.y - before.y) < 1e-6f);
    AT(fabsf(after.width - before.width) < 1e-6f);
    AT(fabsf(after.height - before.height) < 1e-6f);

    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(colorbar->ramp_visual != NULL);
    AT(colorbar->text_visual != NULL);
    AT(colorbar->text_visual->text.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    DvzVisualDataView pos_view = {0};
    AT(dvz_visual_data(colorbar->ramp_visual, "position", &pos_view) == 0);
    const float* positions = (const float*)pos_view.data;
    AT(fabsf(positions[0] - 0.76f) < 1e-5f);
    AT(fabsf(positions[1] + 0.2266667f) < 1e-5f);
    AT(fabsf(positions[3] - 0.82f) < 1e-5f);
    AT(fabsf(positions[3 * (pos_view.item_count - 1) + 0] - 0.82f) < 1e-5f);
    AT(fabsf(positions[3 * (pos_view.item_count - 1) + 1] - 0.84f) < 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify retained colorbar visuals respond to scale and colormap updates.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_updates_retained_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_set_reserve(panel, &(DvzPanelReserve){.left_px = 24.0f}));

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS, .format = {.precision = 0}});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormapStop stops0[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    dvz_colormap_set_stops(colormap, stops0, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Initial",
        });
    ANN(colorbar);
    DvzPanelReserve reserve = {0};
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.left_px - 24.0f) < 1e-6f);
    AT(fabsf(reserve.right_px - 140.0f) < 1e-6f);
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(colorbar->tick_count >= 2);
    AT(colorbar->text_count == colorbar->tick_count + 1);
    AT(strcmp(colorbar->text_labels[0], "0") == 0);
    AT(strcmp(colorbar->text_labels[colorbar->text_count - 1], "Initial") == 0);

    DvzVisualDataView ramp_color = {0};
    AT(dvz_visual_data(colorbar->ramp_visual, "color", &ramp_color) == 0);
    const uint8_t* colors = ramp_color.data;
    ANN(colors);
    AT(colors[2] == 255);
    AT(colors[4 * (ramp_color.item_count - 1)] == 255);

    dvz_scale_set_domain(scale, -10.0, 10.0);
    dvz_colorbar_set_title(colorbar, "Updated");
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(colorbar->tick_count >= 2);
    AT(strcmp(colorbar->text_labels[0], "-10") == 0);
    AT(strcmp(colorbar->text_labels[colorbar->text_count - 1], "Updated") == 0);
    AT(!colorbar->dirty);

    DvzColormapStop stops1[2] = {
        {.position = 0.0, .rgba = {0, 255, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops1, 2);
    _scene_prepare_colorbar_visuals(figure, NULL);
    AT(dvz_visual_data(colorbar->ramp_visual, "color", &ramp_color) == 0);
    colors = ramp_color.data;
    ANN(colors);
    AT(colors[1] == 255);
    AT(colors[2] == 0);
    AT(colors[4 * (ramp_color.item_count - 1)] == 255);
    AT(colors[4 * (ramp_color.item_count - 1) + 1] == 255);

    DvzVisual* ramp = colorbar->ramp_visual;
    DvzVisual* ticks = colorbar->tick_visual;
    DvzVisual* text = colorbar->text_visual;
    ANN(ramp);
    ANN(ticks);
    ANN(text);
    dvz_colorbar_destroy(colorbar);
    AT(!ramp->visible);
    AT(!ticks->visible);
    AT(!text->visible);
    AT(panel->colorbar_count == 0);
    AT(dvz_panel_get_reserve(panel, &reserve));
    AT(fabsf(reserve.left_px - 24.0f) < 1e-6f);
    AT(fabsf(reserve.right_px) < 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify colorbars emit ramp, tick, and glyph-derived DRP2 work.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_emit_stream_contains_derived_visuals(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS, .format = {.precision = 1}});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);
    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    ANN(colorbar);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(colorbar->tick_count >= 2);
    AT(colorbar->text_visual != NULL);
    AT(colorbar->text_visual->text.glyph_visual != NULL);

    const uint64_t ramp_vertex_count = 6u * 64u;
    const uint64_t ramp_position_size = ramp_vertex_count * 3u * sizeof(float);
    const uint64_t ramp_color_size = ramp_vertex_count * sizeof(DvzColor);
    bool found_ramp_position_upload = false;
    bool found_ramp_color_upload = false;
    bool found_ramp_draw = false;
    bool found_tick_draw = false;
    bool found_glyph_bind = false;
    bool found_glyph_draw = false;
    bool found_ramp_position_label = false;
    bool found_ramp_color_label = false;
    bool found_tick_start_label = false;
    bool found_tick_index_label = false;
    bool found_glyph_position_label = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            found_ramp_position_upload =
                found_ramp_position_upload || cmd->u.write_buffer.size == ramp_position_size;
            found_ramp_color_upload =
                found_ramp_color_upload || cmd->u.write_buffer.size == ramp_color_size;
            if (label != NULL)
            {
                found_ramp_position_label =
                    found_ramp_position_label ||
                    strcmp(label, "colorbar.0.ramp.position") == 0;
                found_ramp_color_label =
                    found_ramp_color_label || strcmp(label, "colorbar.0.ramp.color") == 0;
                found_tick_start_label =
                    found_tick_start_label ||
                    strcmp(label, "colorbar.0.ticks.position_start") == 0;
                found_tick_index_label =
                    found_tick_index_label || strcmp(label, "colorbar.0.ticks.index") == 0;
                found_glyph_position_label =
                    found_glyph_position_label ||
                    strcmp(label, "colorbar.0.labels.glyph.position") == 0;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            if (cmd->u.draw.vertex_count == ramp_vertex_count)
                found_ramp_draw = true;
            else if (cmd->u.draw.vertex_count > 0 && cmd->u.draw.vertex_count % 6 == 0)
                found_glyph_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_tick_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_glyph_bind = found_glyph_bind || cmd->u.set_bind_group.slot == 1;
        }
    }
    AT(found_ramp_position_upload);
    AT(found_ramp_color_upload);
    AT(found_ramp_draw);
    AT(found_tick_draw);
    AT(found_glyph_bind);
    AT(found_glyph_draw);
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_prim_t"));
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_glyphg"));
    AT(found_ramp_position_label);
    AT(found_ramp_color_label);
    AT(found_tick_start_label);
    AT(found_tick_index_label);
    AT(found_glyph_position_label);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify invalid colorbar realization writes an emit diagnostic.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_colorbar_invalid_domain_reports_diagnostic(
    TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 1.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
    ANN(colormap);
    dvz_scale_set_colormap(scale, colormap);
    DvzColorbar* colorbar = dvz_colorbar(panel, scale, NULL);
    ANN(colorbar);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = NULL;
    AT_EXPECTED_ERROR_STRICT(
        suite, (stream = dvz_figure_emit(figure, &caps, &report)) != NULL);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "colorbar scale domain") != NULL);
    AT(colorbar->ramp_visual == NULL || !colorbar->ramp_visual->visible);
    AT(colorbar->tick_visual == NULL || !colorbar->tick_visual->visible);
    AT(colorbar->text_visual == NULL || !colorbar->text_visual->visible);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_colorbar_rejects_unsupported_requests(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 256, 256, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    DvzColorbar* colorbar = NULL;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        (colorbar = dvz_colorbar(
             panel, scale,
             &(DvzColorbarDesc){.anchor = DVZ_SCENE_ANCHOR_PANEL_CENTER})) == NULL);
    AT(colorbar == NULL);

    DvzScale* categorical = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(categorical);
    AT_EXPECTED_ERROR_STRICT(suite, (colorbar = dvz_colorbar(panel, categorical, NULL)) == NULL);
    AT(colorbar == NULL);

    colorbar = dvz_colorbar(panel, scale, NULL);
    ANN(colorbar);
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_colorbar_set_anchor(colorbar, DVZ_SCENE_ANCHOR_SCREEN));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_colorbar_rejects_cross_scene_scale(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzFigure* figure = dvz_figure(scene0, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);

    DvzColorbar* colorbar = NULL;
    AT_EXPECTED_ERROR_STRICT(suite, (colorbar = dvz_colorbar(panel, foreign_scale, NULL)) == NULL);
    AT(colorbar == NULL);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_image_visual_binds_colormap_scale(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Intensity",
                   .unit = "a.u.",
               });
    ANN(scale);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);
    AT(image->scale == scale);
    AT(strcmp(image->scale_slot, "colormap") == 0);

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,
        +1.0f, -1.0f, 0.0f,
    };
    float texcoords[4 * 2] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    static const uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"scale\":{\"id\":\"s0\",\"slot\":\"colormap\"}") != NULL);
    AT(strstr(json, "\"field\":{\"id\":\"f0\",\"slot\":\"field\"}") != NULL);
    AT(strstr(json, "\"fields\":[{\"id\":\"f0\"") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_labels_visual_binds_categorical_scale(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    AT(labels->type == DVZ_VISUAL_TYPE_LABELS);
    AT(labels->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(!labels->depth_test_enabled);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[2] = {
        {.category_id = -1, .order = 0, .label = "unassigned", .color = {128, 128, 128, 120}},
        {.category_id = 42, .order = 1, .label = "cell 42", .color = {0, 255, 0, 180}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(labels, "labels", scale) == 0);
    AT(labels->scale == scale);
    AT(strcmp(labels->scale_slot, "labels") == 0);

    int32_t values[4] = {0, -1, 42, -1};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 2 * sizeof(int32_t),
                                   .rows_per_image = 2}));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(labels->field == field);

    DvzScale* continuous = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(continuous);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_scale(labels, "labels", continuous) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_scale(labels, "colormap", scale) != 0);

    DvzSampledField* scalar = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(scalar);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(labels, "field", scalar));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_labels_state_setters(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);

    const DvzLabelsState* state = dvz_labels_state(labels);
    ANN(state);
    AT(state->opacity == 1.0f);
    AT(state->background_id == 0);
    AT(!state->selected_enabled);
    AT(state->boundary_width_px == 1.0f);
    AT(state->slice_axis == DVZ_VOLUME_AXIS_Z);
    AC(state->slice_position, 0.5, 1e-6);
    uint64_t version0 = state->version;

    AT(dvz_labels_set_opacity(labels, 0.35f) == 0);
    AT(dvz_labels_set_background(labels, -1) == 0);
    AT(dvz_labels_set_selected(labels, 42) == 0);
    DvzCategoryId hidden[2] = {-7, 1009};
    AT(dvz_labels_set_hidden(labels, hidden, 2) == 0);
    DvzColor boundary = {12, 34, 56, 200};
    AT(dvz_labels_set_boundary(labels, true, 2.5f, boundary) == 0);
    AT(dvz_labels_set_fallback_seed(labels, 12345) == 0);
    AT(dvz_labels_set_slice_axis(labels, DVZ_VOLUME_AXIS_X) == 0);
    AT(dvz_labels_set_slice_position(labels, 0.25) == 0);

    state = dvz_labels_state(labels);
    ANN(state);
    AC(state->opacity, 0.35f, 1e-6f);
    AT(state->background_id == -1);
    AT(state->selected_enabled);
    AT(state->selected_id == 42);
    AT(state->hidden_count == 2);
    AT(state->hidden_ids[0] == -7);
    AT(state->hidden_ids[1] == 1009);
    AT(state->boundary_enabled);
    AC(state->boundary_width_px, 2.5f, 1e-6f);
    AT(state->boundary_color.r == 12);
    AT(state->boundary_color.g == 34);
    AT(state->boundary_color.b == 56);
    AT(state->boundary_color.a == 200);
    AT(state->fallback_seed == 12345);
    AT(state->slice_axis == DVZ_VOLUME_AXIS_X);
    AC(state->slice_position, 0.25, 1e-6);
    AT(state->version > version0);

    AT(dvz_labels_clear_selected(labels) == 0);
    AT(!state->selected_enabled);
    AT(dvz_labels_set_hidden(labels, NULL, 0) == 0);
    AT(state->hidden_count == 0);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_labels_set_opacity(labels, -0.1f) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_labels_set_hidden(labels, NULL, 1) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_labels_set_boundary(labels, true, -1.0f, boundary) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_labels_set_slice_position(labels, 1.5) != 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    AT(dvz_labels_state(image) == NULL);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_labels_set_opacity(image, 0.5f) != 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_scale_rejects_cross_scene_scale(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);
    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_scale(image, "colormap", foreign_scale) != 0);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_visual_buffer_rejects_cross_scene_buffer(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzSceneBuffer* foreign_buffer = dvz_scene_buffer(
        scene1, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    ANN(foreign_buffer);
    DvzVisual* visual = dvz_primitive(scene0, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_buffer(visual, "index", foreign_buffer));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_image_scalar_texture_uses_bound_scale(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);
    AT(dvz_visual_set_scale(visual, "colormap", scale) == 0);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float pixels[4 * 4];
    for (uint32_t i = 0; i < 16; i++)
        pixels[i] = 1.0f;

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_f32(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(visual->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_r16_float_field_uses_bound_scale(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

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
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    uint16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 0x3c00u; /* half-float 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(uint16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_r16_snorm_field_uses_bound_scale(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, -1.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

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
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    int16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 32767; /* SNORM 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_SNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(int16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_field_rejects_cross_scene_field(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene1, &(DvzSampledFieldDesc){
                    .dim = DVZ_FIELD_DIM_2D,
                    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                    .width = 2,
                    .height = 2,
                    .depth = 1,
                });
    ANN(field);

    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_sampled_field_update_region(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));

    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 2, .height = 2, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));

    uint8_t* data = (uint8_t*)field->data;
    AT(data[5] == 1);
    AT(data[6] == 2);
    AT(data[9] == 3);
    AT(data[10] == 4);
    AT(field->dirty);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_rejects_unsupported_format(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = NULL;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        (field = dvz_sampled_field(
             scene, &(DvzSampledFieldDesc){
                        .dim = DVZ_FIELD_DIM_2D,
                        .format = DVZ_FIELD_FORMAT_RG32_FLOAT,
                        .semantic = DVZ_FIELD_SEMANTIC_VECTOR_2,
                        .width = 4,
                        .height = 4,
                        .depth = 1,
                    })) == NULL);
    AT(field == NULL);
    AT(_captured_log_contains(suite, "unsupported sampled field format"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_visual_rejects_3d_field(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 4,
               });
    ANN(field);

    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "require a 2D sampled field"));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify textured meshes accept only 2D RGBA sampled fields.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_mesh_visual_binds_texture_field(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };
    DvzSampledField* rgba = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(rgba);
    AT(dvz_sampled_field_set_data(
        rgba, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 2 * 4, .rows_per_image = 2}));
    AT(dvz_visual_set_field(mesh, "texture", rgba));
    AT(mesh->field == rgba);
    AT(strcmp(mesh->field_slot, "texture") == 0);

    DvzSampledField* scalar = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(scalar);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(mesh, "texture", scalar));
    AT(_captured_log_contains(suite, "mesh texture fields require RGBA8_UNORM"));

    DvzSampledField* volume = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(volume);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(mesh, "texture", volume));
    AT(_captured_log_contains(suite, "require a 2D sampled field"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_visual_binds_3d_field(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    AT(volume->type == DVZ_VISUAL_TYPE_VOLUME);
    AT(volume->topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    DvzVisualDataView position_view = {0};
    DvzVisualDataView texcoord_view = {0};
    AT(dvz_visual_data(volume, "position", &position_view) == 0);
    AT(dvz_visual_data(volume, "texcoords", &texcoord_view) == 0);
    AT(position_view.item_count == 36);
    AT(texcoord_view.item_count == 36);
    const float* positions = position_view.data;
    const float* texcoords = texcoord_view.data;
    ANN(positions);
    ANN(texcoords);
    AT(positions[2] == -1.0f);
    AT(positions[11 * 3 + 2] == +1.0f);
    AT(texcoords[2] == 0.0f);
    AT(texcoords[11 * 3 + 2] == 1.0f);

    DvzSampledField* field3d = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 4,
               });
    ANN(field3d);
    AT(dvz_visual_set_field(volume, "field", field3d));
    AT(volume->field == field3d);
    AT(strcmp(volume->field_slot, "field") == 0);
    AT(volume->texture.dirty);

    DvzSampledField* field2d = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field2d);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(volume, "field", field2d));
    AT(_captured_log_contains(suite, "require a 3D sampled field"));

    AT(dvz_visual_set_field(volume, "field", NULL));
    AT(volume->field == NULL);
    AT(volume->field_slot[0] == '\0');

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_field_emit_realizes_3d_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);

    uint16_t base[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = base,
                   .bytes_per_row = 2 * sizeof(uint16_t),
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t texture_id = 0;
    bool created_texture = false;
    bool wrote_full_texture = false;
    bool created_triangle_list_pipeline = false;
    bool created_entry_face_pipeline = false;
    bool drew_box_proxy = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == 2 &&
            cmd->u.write_texture.height == 2 && cmd->u.write_texture.depth == 2)
            texture_id = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
        {
            created_texture = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
            AT(cmd->u.create_texture.depth == 2);
            AT(cmd->u.create_texture.format == VK_FORMAT_R16_UNORM);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            wrote_full_texture = true;
            AT(cmd->u.write_texture.origin_x == 0);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 0);
            AT(cmd->u.write_texture.width == 2);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 2);
            AT(cmd->u.write_texture.bytes_per_row == 2 * sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        {
            created_triangle_list_pipeline = true;
            if (cmd->u.create_render_pipeline.has_raster_state &&
                cmd->u.create_render_pipeline.cull_mode == VK_CULL_MODE_BACK_BIT &&
                cmd->u.create_render_pipeline.front_face == VK_FRONT_FACE_CLOCKWISE)
            {
                created_entry_face_pipeline = true;
            }
        }
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW && cmd->u.draw.vertex_count == 36)
            drew_box_proxy = true;
    }
    AT(texture_id != 0);
    AT(created_texture);
    AT(wrote_full_texture);
    AT(created_triangle_list_pipeline);
    AT(created_entry_face_pipeline);
    AT(drew_box_proxy);
    AT(!field->dirty);
    AT(!volume->texture.dirty);
    dvz_drp2_stream_destroy(stream0);

    uint16_t patch[2] = {101, 102};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 0, .z = 1, .width = 1, .height = 2, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = sizeof(uint16_t),
            .rows_per_image = 2,
        }));

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_texture = false;
    bool wrote_partial_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
            recreated_texture = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            const uint16_t* uploaded = (const uint16_t*)cmd->u.write_texture.data_raw;
            wrote_partial_texture = true;
            AT(cmd->u.write_texture.origin_x == 1);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 1);
            AT(cmd->u.write_texture.width == 1);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
            ANN(uploaded);
            AT(uploaded[0] == 101);
            AT(uploaded[1] == 102);
        }
    }
    AT(!recreated_texture);
    AT(wrote_partial_texture);
    AT(!field->dirty);
    AT(!volume->texture.dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_retained_controls(TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);

    const DvzVolumeState* state = dvz_volume_state(volume);
    ANN(state);
    AT(state->opacity == 1.0f);
    AT(state->sampling == DVZ_VOLUME_SAMPLING_LINEAR);
    AT(state->render_mode == DVZ_VOLUME_RENDER_COMPOSITE);
    AT(state->slice_axis == DVZ_VOLUME_AXIS_Z);
    AT(state->slice_position == 0.5);
    AT(state->step_count == 64);
    AT(!state->clipping_enabled);
    AT(!state->clip_plane_enabled);
    AT(state->clip_min[0] == 0.0);
    AT(state->clip_max[2] == 1.0);
    AT(state->clip_plane_point[0] == 0.5);
    AT(state->clip_plane_normal[0] == 1.0);
    AT(state->bounds_min[0] == -1.0);
    AT(state->bounds_max[2] == +1.0);
    AT(state->axis_order[0] == 0);
    AT(state->axis_order[1] == 1);
    AT(state->axis_order[2] == 2);
    AT(!state->axis_flip[0]);
    AT(state->value_min == 0.0);
    AT(state->value_max == 1.0);
    AT(state->alpha_stop_count == 2);
    AT(state->alpha_stops[0].position == 0.0);
    AT(state->alpha_stops[0].alpha == 0.0f);
    AT(state->alpha_stops[1].position == 1.0);
    AT(state->alpha_stops[1].alpha == 1.0f);
    uint64_t version0 = state->version;

    AT(dvz_volume_set_opacity(volume, 0.35f) == 0);
    AT(dvz_volume_state(volume)->opacity == 0.35f);
    AT(dvz_volume_state(volume)->version != version0);
    uint64_t version1 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);
    AT(dvz_volume_state(volume)->sampling == DVZ_VOLUME_SAMPLING_NEAREST);
    AT(dvz_volume_state(volume)->version != version1);
    uint64_t version2 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_state(volume)->render_mode == DVZ_VOLUME_RENDER_MIP);
    AT(dvz_volume_state(volume)->version != version2);
    uint64_t version3 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_state(volume)->render_mode == DVZ_VOLUME_RENDER_COMPOSITE);
    AT(dvz_volume_state(volume)->version != version3);
    uint64_t version4 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_step_count(volume, 32) == 0);
    AT(dvz_volume_state(volume)->step_count == 32);
    AT(dvz_volume_state(volume)->version != version4);
    uint64_t version5 = dvz_volume_state(volume)->version;

    double bounds_min[3] = {-0.5, -0.75, -1.0};
    double bounds_max[3] = {+0.5, +0.75, +1.0};
    AT(dvz_volume_set_bounds(volume, bounds_min, bounds_max) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->bounds_min[1] == -0.75);
    AT(state->bounds_max[0] == +0.5);
    AT(state->version != version5);

    double clip_min[3] = {0.1, 0.2, 0.3};
    double clip_max[3] = {0.9, 0.8, 0.7};
    AT(dvz_volume_set_clipping_box(volume, clip_min, clip_max) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->clipping_enabled);
    AT(state->clip_min[1] == 0.2);
    AT(state->clip_max[2] == 0.7);
    uint64_t version6 = state->version;

    uint32_t axis_order[3] = {2, 0, 1};
    bool axis_flip[3] = {true, false, true};
    AT(dvz_volume_set_axis_mapping(volume, axis_order, axis_flip) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->axis_order[0] == 2);
    AT(state->axis_order[1] == 0);
    AT(state->axis_order[2] == 1);
    AT(state->axis_flip[0]);
    AT(!state->axis_flip[1]);
    AT(state->axis_flip[2]);
    AT(state->version != version6);
    uint64_t version7 = state->version;

    AT(dvz_volume_set_value_range(volume, -1000.0, 3000.0) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->value_min == -1000.0);
    AT(state->value_max == 3000.0);
    AT(state->version != version7);
    uint64_t version8 = state->version;

    DvzVolumeAlphaStop alpha_stops[3] = {
        {.position = 1.0, .alpha = 0.2f},
        {.position = 0.0, .alpha = 0.0f},
        {.position = 0.5, .alpha = 1.0f},
    };
    AT(dvz_volume_set_alpha_stops(volume, alpha_stops, 3) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->alpha_stop_count == 3);
    AT(state->alpha_stops[0].position == 0.0);
    AT(state->alpha_stops[1].position == 0.5);
    AT(state->alpha_stops[2].position == 1.0);
    AT(state->alpha_stops[1].alpha == 1.0f);
    AT(state->version != version8);
    uint64_t version9 = state->version;

    double plane_point[3] = {0.25, 0.5, 0.75};
    double plane_normal[3] = {0.0, 0.0, 2.0};
    AT(dvz_volume_set_clipping_plane(volume, plane_point, plane_normal, true) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->clip_plane_enabled);
    AT(state->clip_plane_keep_positive);
    AT(state->clip_plane_point[0] == 0.25);
    AT(state->clip_plane_point[2] == 0.75);
    AT(state->clip_plane_normal[0] == 0.0);
    AT(state->clip_plane_normal[2] == 1.0);
    AT(state->version != version9);
    AT(dvz_volume_clear_clipping_plane(volume) == 0);
    AT(!dvz_volume_state(volume)->clip_plane_enabled);
    AT(dvz_volume_set_clipping_plane(volume, plane_point, plane_normal, true) == 0);

    AT(dvz_volume_set_slice_axis(volume, DVZ_VOLUME_AXIS_X) == 0);
    AT(dvz_volume_state(volume)->slice_axis == DVZ_VOLUME_AXIS_X);
    AT(dvz_volume_set_slice_position(volume, 0.42) == 0);
    AT(dvz_volume_state(volume)->slice_position == 0.42);

    AT(dvz_volume_clear_clipping(volume) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(!state->clipping_enabled);
    AT(!state->clip_plane_enabled);
    AT(state->clip_min[0] == 0.0);
    AT(state->clip_max[2] == 1.0);
    AT(state->clip_plane_point[0] == 0.5);
    AT(state->clip_plane_normal[0] == 1.0);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_opacity(volume, -0.1f) != 0);
    AT(_captured_log_contains(suite, "volume opacity must be finite"));
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_slice_axis(volume, (DvzVolumeAxis)999) != 0);
    AT(_captured_log_contains(suite, "unsupported volume slice axis"));
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_slice_position(volume, -0.5) != 0);
    AT(_captured_log_contains(suite, "volume slice position must be finite"));
    double invalid_min[3] = {0.0, 0.8, 0.0};
    double invalid_max[3] = {1.0, 0.7, 1.0};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_volume_set_clipping_box(volume, invalid_min, invalid_max) != 0);
    AT(_captured_log_contains(suite, "volume clipping box coordinates"));
    double invalid_bounds_min[3] = {0.0, -1.0, -1.0};
    double invalid_bounds_max[3] = {0.0, +1.0, +1.0};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_volume_set_bounds(volume, invalid_bounds_min, invalid_bounds_max) != 0);
    AT(_captured_log_contains(suite, "volume bounds must be finite"));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 0}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);
    AT(dvz_visual_set_scale(volume, "colormap", scale) == 0);
    AT(volume->scale == scale);
    AT(strcmp(volume->scale_slot, "colormap") == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    AT(dvz_volume_state(image) == NULL);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_sampling(image, DVZ_VOLUME_SAMPLING_LINEAR) != 0);
    AT(_captured_log_contains(suite, "requires a volume visual"));
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_render_mode(image, DVZ_VOLUME_RENDER_SLICE) != 0);
    AT(_captured_log_contains(suite, "requires a volume visual"));
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_step_count(volume, 0) != 0);
    AT(_captured_log_contains(suite, "volume step count must be in"));
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_volume_set_render_mode(volume, (DvzVolumeRenderMode)999) != 0);
    AT(_captured_log_contains(suite, "unsupported volume render mode"));
    uint32_t invalid_order[3] = {0, 0, 2};
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_axis_mapping(volume, invalid_order, NULL) != 0);
    AT(_captured_log_contains(suite, "volume axis order must be a permutation"));
    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_value_range(volume, 1.0, 1.0) != 0);
    AT(_captured_log_contains(suite, "volume value range must be finite"));
    DvzVolumeAlphaStop invalid_alpha_stops[1] = {{.position = 1.5, .alpha = 0.5f}};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_volume_set_alpha_stops(volume, invalid_alpha_stops, 1) != 0);
    AT(_captured_log_contains(suite, "volume alpha stops require finite"));
    double invalid_plane_point[3] = {0.5, 0.5, 1.2};
    double valid_plane_normal[3] = {1.0, 0.0, 0.0};
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_volume_set_clipping_plane(volume, invalid_plane_point, valid_plane_normal, true) != 0);
    AT(_captured_log_contains(suite, "volume clipping plane point"));
    double valid_plane_point[3] = {0.5, 0.5, 0.5};
    double invalid_plane_normal[3] = {0.0, 0.0, 0.0};
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_volume_set_clipping_plane(volume, valid_plane_point, invalid_plane_normal, true) != 0);
    AT(_captured_log_contains(suite, "volume clipping plane normal"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_visual_metadata_lowering(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 8,
                   .height = 6,
                   .depth = 4,
               });
    ANN(field);
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    DvzColormap* volume_colormap = dvz_colormap(scene, NULL);
    ANN(volume_colormap);
    DvzColormapStop volume_stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 0}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    dvz_colormap_set_stops(volume_colormap, volume_stops, 2);
    dvz_scale_set_colormap(scale, volume_colormap);
    AT(dvz_visual_set_scale(volume, "colormap", scale) == 0);
    AT(dvz_volume_set_opacity(volume, 0.5f) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);
    double clip_min[3] = {0.1, 0.2, 0.3};
    double clip_max[3] = {0.9, 0.8, 0.7};
    double clip_plane_point[3] = {0.5, 0.5, 0.25};
    double clip_plane_normal[3] = {0.0, 1.0, 0.0};
    AT(dvz_volume_set_slice_axis(volume, DVZ_VOLUME_AXIS_Y) == 0);
    AT(dvz_volume_set_slice_position(volume, 0.35) == 0);
    AT(dvz_volume_set_clipping_box(volume, clip_min, clip_max) == 0);
    AT(dvz_volume_set_clipping_plane(volume, clip_plane_point, clip_plane_normal, false) == 0);

    DvzFramePlanVisualMeta metadata = {0};
    AT(_scene_visual_frame_plan_metadata(figure, volume, 0, &metadata));
    AT(metadata.has_metadata);
    AT(metadata.visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(metadata.visual_index == 0);
    AT(metadata.has_volume);
    AT(metadata.field_format == DVZ_FIELD_FORMAT_R8_UNORM);
    AT(metadata.field_width == 8);
    AT(metadata.field_height == 6);
    AT(metadata.field_depth == 4);
    AT(metadata.scale_index == 0);
    AT(!metadata.volume_transfer_rgba);
    AT(metadata.volume_state.opacity == 0.5f);
    AT(metadata.volume_state.sampling == DVZ_VOLUME_SAMPLING_NEAREST);
    AT(metadata.volume_state.slice_axis == DVZ_VOLUME_AXIS_Y);
    AT(metadata.volume_state.slice_position == 0.35);
    AT(metadata.volume_state.clipping_enabled);
    AT(metadata.volume_state.clip_plane_enabled);
    AT(!metadata.volume_state.clip_plane_keep_positive);
    AT(metadata.volume_state.clip_min[2] == 0.3);
    AT(metadata.volume_state.clip_max[1] == 0.8);
    AT(metadata.volume_state.clip_plane_point[2] == 0.25);
    AT(metadata.volume_state.clip_plane_normal[1] == 1.0);
    AT(metadata.volume_state.axis_order[0] == 0);
    AT(metadata.texture_id[0] != '\0');
    AT(strcmp(metadata.volume_texture_id, metadata.texture_id) == 0);
    AT(metadata.volume_transfer_texture_id[0] != '\0');

    DvzFramePlan* plan = dvz_frame_plan("figure.volume.metadata", 0);
    ANN(plan);
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "volume.debug"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, 0);
    ANN(node);
    AT(node->u.render.visual_metadata[0].has_metadata);
    AT(node->u.render.visual_metadata[0].has_volume);
    AT(node->u.render.visual_metadata[0].volume_state.opacity == 0.5f);
    AT(node->u.render.visual_metadata[0].volume_state.slice_axis == DVZ_VOLUME_AXIS_Y);
    AT(node->u.render.visual_metadata[0].volume_state.slice_position == 0.35);
    AT(strcmp(node->u.render.visual_metadata[0].volume_texture_id, metadata.texture_id) == 0);
    AT(node->u.render.visual_metadata[0].volume_transfer_texture_id[0] != '\0');

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_rgba_field_no_transfer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);

    const uint32_t width = 4;
    const uint32_t height = 4;
    const uint32_t depth = 2;
    uint64_t data_size = 0;
    if (_dvz_mul_u64_overflows(width, height, &data_size) ||
        _dvz_mul_u64_overflows(data_size, depth, &data_size) ||
        _dvz_mul_u64_overflows(data_size, 4, &data_size))
    {
        dvz_scene_destroy(scene);
        return 1;
    }

    uint8_t* data = (uint8_t*)dvz_calloc(data_size, 1);
    ANN(data);
    for (uint32_t z = 0; z < depth; z++)
    {
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint64_t i = ((uint64_t)z * height + y) * width + x;
                data[4 * i + 0] = (uint8_t)(x + 16);
                data[4 * i + 1] = (uint8_t)(y + 32);
                data[4 * i + 2] = (uint8_t)(z + 48);
                data[4 * i + 3] = 200;
            }
        }
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = width,
                   .height = height,
                   .depth = depth,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = data,
            .bytes_per_row = width * 4,
            .rows_per_image = height,
        }));
    dvz_free(data);
    data = NULL;

    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(volume->texture.rgba == NULL);
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzFramePlanVisualMeta metadata = {0};
    AT(_scene_visual_frame_plan_metadata(figure, volume, 0, &metadata));
    AT(metadata.has_volume);
    AT(metadata.field_format == DVZ_FIELD_FORMAT_RGBA8_UNORM);
    AT(metadata.volume_transfer_rgba);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool wrote_rgba_texture = false;
    bool matched_start_value = false;
    uint64_t texture_id = 0;
    uint64_t transfer_binding_id = 0;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == width &&
            cmd->u.write_texture.height == height && cmd->u.write_texture.depth == depth)
        {
            texture_id = cmd->u.write_texture.texture_id;
            wrote_rgba_texture = true;
            AT(cmd->u.write_texture.bytes_per_row == width * 4);
            AT(cmd->u.write_texture.rows_per_image == height);
            const uint8_t* upload = (const uint8_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            AT(upload[0] == 16);
            AT(upload[1] == 32);
            AT(upload[2] == 48);
            AT(upload[3] == 200);
            matched_start_value = true;
        }
    }
    AT(wrote_rgba_texture);
    AT(texture_id != 0);

    bool created_rgba_texture = false;
    bool created_dummy_transfer_texture = false;
    bool wrote_dummy_transfer_texture = false;
    bool found_volume_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
            cmd->u.create_texture.id == texture_id)
        {
            created_rgba_texture = true;
            AT(cmd->u.create_texture.format == VK_FORMAT_R8G8B8A8_UNORM);
            AT(cmd->u.create_texture.width == width);
            AT(cmd->u.create_texture.height == height);
            AT(cmd->u.create_texture.depth == depth);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            cmd->u.create_bind_group.entry_count == 6)
        {
            bool binds_volume_texture = false;
            uint64_t binding4_id = 0;
            for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
            {
                const DvzDrp2BindGroupEntry* entry = &cmd->u.create_bind_group.entries[j];
                if (entry->binding == 0 && entry->resource_id == texture_id)
                    binds_volume_texture = true;
                if (entry->binding == 4)
                    binding4_id = entry->resource_id;
            }
            if (binds_volume_texture && binding4_id != 0)
            {
                found_volume_bind_group = true;
                transfer_binding_id = binding4_id;
                AT(transfer_binding_id != texture_id);
            }
        }
    }
    AT(found_volume_bind_group);
    AT(transfer_binding_id != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
            cmd->u.create_texture.id == transfer_binding_id)
        {
            created_dummy_transfer_texture = true;
            AT(cmd->u.create_texture.format == VK_FORMAT_R8G8B8A8_UNORM);
            AT(cmd->u.create_texture.width == 1);
            AT(cmd->u.create_texture.height == 1);
            AT(cmd->u.create_texture.depth == 1);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == transfer_binding_id)
        {
            const uint8_t* upload = (const uint8_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            wrote_dummy_transfer_texture = true;
            AT(cmd->u.write_texture.width == 1);
            AT(cmd->u.write_texture.height == 1);
            AT(cmd->u.write_texture.depth == 1);
            AT(upload[0] == 255);
            AT(upload[1] == 255);
            AT(upload[2] == 255);
            AT(upload[3] == 255);
        }
    }
    AT(created_rgba_texture);
    AT(created_dummy_transfer_texture);
    AT(wrote_dummy_transfer_texture);
    AT(matched_start_value);
    AT(volume->texture.rgba == NULL);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_label_slice_uses_categorical_scale(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    uint16_t labels[8] = {0, 1, 2, 0, 1, 2, 0, 2};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = 0, .order = 0, .label = "background", .color = {0, 0, 0, 0}},
        {.category_id = 1, .order = 1, .label = "one", .color = {255, 0, 0, 255}},
        {.category_id = 2, .order = 2, .label = "two", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 3));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzFramePlanVisualMeta metadata = {0};
    AT(_scene_visual_frame_plan_metadata(figure, volume, 0, &metadata));
    AT(metadata.has_volume);
    AT(metadata.field_format == DVZ_FIELD_FORMAT_R16_UINT);
    AT(metadata.field_semantic == DVZ_FIELD_SEMANTIC_LABEL);
    AT(!metadata.volume_transfer_rgba);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_vol_labels_uint"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure unsigned label volumes use the first-hit composite shader.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_label_composite_uses_first_hit_shader(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    uint16_t labels[8] = {0, 1, 2, 0, 1, 2, 0, 2};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = 0, .order = 0, .label = "background", .color = {0, 0, 0, 0}},
        {.category_id = 1, .order = 1, .label = "one", .color = {255, 0, 0, 255}},
        {.category_id = 2, .order = 2, .label = "two", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 3));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_vol_labels_uint_composite"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure signed label volumes use the first-hit composite shader.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_signed_label_composite_uses_first_hit_shader(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    int16_t labels[8] = {0, -1, 2, 0, -1, 2, 0, 2};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(int16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = -1, .order = 0, .label = "minus one", .color = {255, 0, 0, 255}},
        {.category_id = 2, .order = 1, .label = "two", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_colorbar_stream_has_pipeline_label(stream, "_pipe_vol_labels_sint_composite"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure sparse label volumes emit and bind an SSBO lookup table.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_label_sparse_lookup_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    uint32_t labels[8] = {0, 23, 4000000000u, 0, 23, 4000000000u, 0, 23};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint32_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = 23, .order = 0, .label = "low", .color = {1, 2, 3, 4}},
        {.category_id = 4000000000LL, .order = 1, .label = "high", .color = {5, 6, 7, 8}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t lookup_buffer_id = 0;
    const DvzSceneLabelLookupEntry* entries = NULL;
    uint32_t entry_count = 0;
    AT(_stream_find_volume_label_lookup(stream, &lookup_buffer_id, &entries, &entry_count));
    AT(lookup_buffer_id != 0);
    ANN(entries);
    AT(entry_count == 3);
    AT(entries[0].key == 2);
    AT(entries[1].key == 23);
    AT(entries[1].rgba == 0x04030201u);
    AT(entries[2].key == 4000000000u);
    AT(entries[2].rgba == 0x08070605u);
    AT(_stream_binds_volume_label_lookup(stream, lookup_buffer_id));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure signed sparse label volumes preserve negative keys in the SSBO lookup table.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_signed_label_sparse_lookup_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    int32_t labels[8] = {0, -7, 23, 0, -7, 23, 0, -7};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(int32_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = -7, .order = 0, .label = "negative", .color = {9, 10, 11, 12}},
        {.category_id = 23, .order = 1, .label = "positive", .color = {1, 2, 3, 4}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t lookup_buffer_id = 0;
    const DvzSceneLabelLookupEntry* entries = NULL;
    uint32_t entry_count = 0;
    AT(_stream_find_volume_label_lookup(stream, &lookup_buffer_id, &entries, &entry_count));
    AT(lookup_buffer_id != 0);
    ANN(entries);
    AT(entry_count == 3);
    AT(entries[0].key == 2);
    AT(entries[1].key == 23);
    AT(entries[1].rgba == 0x04030201u);
    AT(entries[2].key == (uint32_t)(int32_t)-7);
    AT(entries[2].rgba == 0x0c0b0a09u);
    AT(_stream_binds_volume_label_lookup(stream, lookup_buffer_id));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure label volumes reject categorical MIP emission explicitly.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_label_mip_reports_unsupported(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    uint16_t labels[8] = {0, 1, 2, 0, 1, 2, 0, 2};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory category = {
        .category_id = 1,
        .order = 0,
        .label = "one",
        .color = {255, 0, 0, 255},
    };
    AT(dvz_scale_set_categories(scale, &category, 1));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) != 0);

    DvzVisual* mip_first = dvz_volume(scene, 0);
    ANN(mip_first);
    AT(dvz_volume_set_render_mode(mip_first, DVZ_VOLUME_RENDER_MIP) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_visual_set_field(mip_first, "field", field));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_volume_scalar_transfer_function_uploads_rgba(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    uint8_t values[8] = {0, 64, 128, 255, 32, 96, 160, 224};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);
    AT(dvz_visual_set_scale(volume, "colormap", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = {.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool created_scalar_texture = false;
    bool created_transfer_texture = false;
    bool wrote_scalar_texture = false;
    bool wrote_transfer_texture = false;
    bool wrote_transfer_alpha = false;
    uint64_t texture_id = 0;
    uint64_t transfer_texture_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == 2 &&
            cmd->u.write_texture.height == 2 && cmd->u.write_texture.depth == 2)
            texture_id = cmd->u.write_texture.texture_id;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE && cmd->u.write_texture.width == 256 &&
            cmd->u.write_texture.height == 1 && cmd->u.write_texture.depth == 1)
            transfer_texture_id = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
        {
            created_scalar_texture = true;
            AT(cmd->u.create_texture.format == VK_FORMAT_R8_UNORM);
            AT(cmd->u.create_texture.depth == 2);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
            cmd->u.create_texture.id == transfer_texture_id)
        {
            created_transfer_texture = true;
            AT(cmd->u.create_texture.format == VK_FORMAT_R8G8B8A8_UNORM);
            AT(cmd->u.create_texture.width == 256);
            AT(cmd->u.create_texture.height == 1);
            AT(cmd->u.create_texture.depth == 1);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            const uint8_t* upload = (const uint8_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            wrote_scalar_texture = true;
            AT(cmd->u.write_texture.bytes_per_row == 2);
            AT(upload[0] == 0);
            AT(upload[3] == 255);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == transfer_texture_id)
        {
            const uint8_t* upload = (const uint8_t*)cmd->u.write_texture.data_raw;
            ANN(upload);
            wrote_transfer_texture = true;
            AT(cmd->u.write_texture.bytes_per_row == 256 * 4);
            AT(cmd->u.write_texture.rows_per_image == 1);
            AT(upload[0] == 0);
            AT(upload[2] == 255);
            AT(upload[3] == 0);
            AT(upload[4 * 128 + 0] > 0);
            AT(upload[4 * 128 + 0] < 255);
            AT(upload[4 * 128 + 3] > 0);
            wrote_transfer_alpha = upload[4 * 255 + 3] == 255;
        }
    }
    AT(created_scalar_texture);
    AT(created_transfer_texture);
    AT(wrote_scalar_texture);
    AT(wrote_transfer_texture);
    AT(wrote_transfer_alpha);
    AT(volume->texture.rgba != NULL);
    AT(!field->dirty);
    AT(!volume->texture.dirty);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_3d_emits_runtime_texture_upload(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);

    uint16_t base[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = base,
                   .bytes_per_row = 2 * sizeof(uint16_t),
                   .rows_per_image = 2,
               }));

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzFramePlan* frame0 = dvz_frame_plan("figure.field3d.runtime", 0);
    ANN(frame0);
    AT(_scene_emit_sampled_field_texture_upload(frame0, "tex.field.volume", field));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame0, "visual.texture.volume"));
    AT(dvz_frame_plan_render_allow_untyped_visuals(frame0));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t texture_id = 0;
    bool created_texture = false;
    bool wrote_full_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            texture_id = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
        {
            created_texture = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
            AT(cmd->u.create_texture.depth == 2);
            AT(cmd->u.create_texture.format == VK_FORMAT_R16_UNORM);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            wrote_full_texture = true;
            AT(cmd->u.write_texture.origin_z == 0);
            AT(cmd->u.write_texture.width == 2);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 2);
            AT(cmd->u.write_texture.bytes_per_row == 2 * sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
        }
    }
    AT(texture_id != 0);
    AT(created_texture);
    AT(wrote_full_texture);

    field->dirty = false;
    field->dirty_full = false;
    field->dirty_region = (DvzFieldRegion){0};
    uint16_t patch[2] = {101, 102};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 0, .z = 1, .width = 1, .height = 2, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = sizeof(uint16_t),
            .rows_per_image = 2,
        }));

    DvzFramePlan* frame1 = dvz_frame_plan("figure.field3d.runtime", 1);
    ANN(frame1);
    AT(_scene_emit_sampled_field_texture_upload(frame1, "tex.field.volume", field));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame1, "visual.texture.volume"));
    AT(dvz_frame_plan_render_allow_untyped_visuals(frame1));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_texture = false;
    bool wrote_partial_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
            recreated_texture = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            const uint16_t* uploaded = (const uint16_t*)cmd->u.write_texture.data_raw;
            wrote_partial_texture = true;
            AT(cmd->u.write_texture.origin_x == 1);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 1);
            AT(cmd->u.write_texture.width == 1);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
            ANN(uploaded);
            AT(uploaded[0] == 101);
            AT(uploaded[1] == 102);
        }
    }
    AT(!recreated_texture);
    AT(wrote_partial_texture);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstContext* suite, const TstCase* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));
    AT_EXPECTED_ERROR_STRICT(
        suite,
        !dvz_sampled_field_update_region(
            field, (DvzFieldRegion){.x = 3, .y = 3, .z = 0, .width = 2, .height = 2, .depth = 1},
            &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(_captured_log_contains(suite, "update region exceeds field dimensions"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_destroy_clears_visual_binding(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);

    uint8_t rgba[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = rgba, .bytes_per_row = 8, .rows_per_image = 2}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(image->field == field);
    AT(strcmp(image->field_slot, "field") == 0);

    AT(dvz_sampled_field_destroy(field));
    AT(image->field == NULL);
    AT(image->field_slot[0] == '\0');

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_field_update_marks_two_visuals_dirty(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 2 * sizeof(float), .rows_per_image = 2}));
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_visual_set_field(image1, "field", field));

    image0->texture.dirty = false;
    image1->texture.dirty = false;
    field->dirty = false;

    float patch[1] = {1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 1, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch, .bytes_per_row = sizeof(float), .rows_per_image = 1}));
    AT(field->dirty);
    AT(image0->texture.dirty);
    AT(image1->texture.dirty);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_field_partial_update_emits_texture_subregion(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

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
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_region = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_region = true;
        }
    }
    AT(write_texture_count == 1);
    AT(found_region);
    AT(!field->dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure a retained image field resize emits a new texture allocation extent.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_field_resize_emits_texture_reallocation(TstContext* suite, const TstCase* item)
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
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    uint8_t pixels[2 * 2 * 4] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 2 * 4, .rows_per_image = 2}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex0 = 0;
    bool created_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex0 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
        {
            created_tex0 = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
        }
    }
    AT(tex0 != 0);
    AT(created_tex0);
    dvz_drp2_stream_destroy(stream0);

    uint8_t resized[4 * 3 * 4] = {0};
    AT(dvz_sampled_field_resize(
        field, 4, 3, 1,
        &(DvzFieldDataView){.data = resized, .bytes_per_row = 4 * 4, .rows_per_image = 3}));
    const DvzSampledFieldDesc* desc = dvz_sampled_field_desc(field);
    ANN(desc);
    AT(desc->width == 4);
    AT(desc->height == 3);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex1 = 0;
    bool created_tex1 = false;
    bool wrote_resized = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            tex1 = cmd->u.write_texture.texture_id;
            if (cmd->u.write_texture.origin_x == 0 && cmd->u.write_texture.origin_y == 0 &&
                cmd->u.write_texture.width == 4 && cmd->u.write_texture.height == 3)
            {
                wrote_resized = true;
            }
        }
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex1)
        {
            created_tex1 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 3);
        }
    }
    AT(tex1 != 0);
    AT(tex1 != tex0);
    AT(created_tex1);
    AT(wrote_resized);
    AT(!field->dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_field_mixed_full_and_partial_uploads(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale0, colormap);
    dvz_scale_set_colormap(scale1, colormap);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));

    vec3 positions0[4] = {
        {-1.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 0.0f},
    };
    vec3 positions1[4] = {
        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));
    dvz_scale_set_view_range(scale0, 0.0, 1.0);

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_full = false;
    bool found_partial = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 0 && cmd->u.write_texture.origin_y == 0 &&
            cmd->u.write_texture.width == 4 && cmd->u.write_texture.height == 4)
        {
            found_full = true;
        }
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_partial = true;
        }
    }
    AT(write_texture_count == 2);
    AT(found_full);
    AT(found_partial);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Register scene field and scale tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_fields(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("fields");

    TST_CASE(test_scene_scale_colormap_colorbar_core);
    TST_CASE(test_scene_categorical_scale_entries);
    TST_CASE(test_scene_legend_lifecycle_and_reserve);
    TST_CASE(test_scene_legend_prepare_visuals);
    TST_CASE(test_scene_legend_emit_stream_contains_derived_visuals);
    TST_CASE(test_scene_colorbar_auto_reserve_and_visuals);
    TST_CASE(test_scene_colorbar_prepare_is_idempotent);
    TST_CASE(test_scene_colorbar_auto_reserve_tracks_resize);
    TST_CASE(test_scene_colorbar_attached_respects_panel_padding);
    TST_CASE(test_scene_colorbar_detached_placement);
    TST_CASE(test_scene_colorbar_updates_retained_visuals);
    TST_CASE(test_scene_colorbar_emit_stream_contains_derived_visuals);
    TST_CASE(test_scene_colorbar_invalid_domain_reports_diagnostic);
    TST_CASE(test_scene_colorbar_rejects_unsupported_requests);
    TST_CASE(test_scene_colorbar_rejects_cross_scene_scale);
    TST_CASE(test_scene_image_visual_binds_colormap_scale);
    TST_CASE(test_scene_labels_visual_binds_categorical_scale);
    TST_CASE(test_scene_labels_state_setters);
    TST_CASE(test_scene_visual_scale_rejects_cross_scene_scale);
    TST_CASE(test_scene_visual_buffer_rejects_cross_scene_buffer);
    TST_CASE(test_scene_image_scalar_texture_uses_bound_scale);
    TST_CASE(test_scene_image_r16_float_field_uses_bound_scale);
    TST_CASE(test_scene_image_r16_snorm_field_uses_bound_scale);
    TST_CASE(test_scene_visual_field_rejects_cross_scene_field);
    TST_CASE(test_scene_sampled_field_update_region);
    TST_CASE(test_scene_sampled_field_rejects_unsupported_format);
    TST_CASE(test_scene_image_visual_rejects_3d_field);
    TST_CASE(test_scene_mesh_visual_binds_texture_field);
    TST_CASE(test_scene_volume_visual_binds_3d_field);
    TST_CASE(test_scene_volume_field_emit_realizes_3d_texture);
    TST_CASE(test_scene_volume_retained_controls);
    TST_CASE(test_scene_volume_rgba_field_no_transfer);
    TST_CASE(test_scene_volume_label_slice_uses_categorical_scale);
    TST_CASE(test_scene_volume_label_composite_uses_first_hit_shader);
    TST_CASE(test_scene_volume_signed_label_composite_uses_first_hit_shader);
    TST_CASE(test_scene_volume_label_sparse_lookup_buffer);
    TST_CASE(test_scene_volume_signed_label_sparse_lookup_buffer);
    TST_CASE(test_scene_volume_label_mip_reports_unsupported);
    TST_CASE(test_scene_volume_visual_metadata_lowering);
    TST_CASE(test_scene_volume_scalar_transfer_function_uploads_rgba);
    TST_CASE(test_scene_sampled_field_3d_emits_runtime_texture_upload);
    TST_CASE(test_scene_sampled_field_update_region_rejects_out_of_bounds);
    TST_CASE(test_scene_sampled_field_destroy_clears_visual_binding);
    TST_CASE(test_scene_shared_field_update_marks_two_visuals_dirty);
    TST_CASE(test_scene_image_field_partial_update_emits_texture_subregion);
    TST_CASE(test_scene_image_field_resize_emits_texture_reallocation);
    TST_CASE(test_scene_shared_field_mixed_full_and_partial_uploads);

    return 0;
}
