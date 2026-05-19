/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene interaction tests                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "../_frame_plan.h"
#include "../_scene.h"
#include "../_scene_emit.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_interaction_core(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzInteractionPolicy* interaction = dvz_interaction(scene);
    DvzSelection* selection =
        dvz_selection(scene, &(DvzSelectionDesc){.mode = DVZ_SELECT_ADDITIVE});
    DvzLinkChannel* channel = dvz_link_channel(scene, "cells");
    ANN(interaction);
    ANN(selection);
    ANN(channel);

    dvz_interaction_bind_panel(interaction, panel);
    dvz_interaction_set_selection(interaction, selection);
    dvz_interaction_set_link_channel(interaction, channel);
    dvz_interaction_set_pick_hit_policy(interaction, DVZ_PICK_HIT_OPAQUE_PREFERRED);
    dvz_interaction_set_auto_pin_readout(interaction, true);

    AT(panel->interaction == interaction);
    AT(interaction->panel == panel);
    AT(interaction->selection == selection);
    AT(interaction->link_channel == channel);
    AT(interaction->pick_hit_policy == DVZ_PICK_HIT_OPAQUE_PREFERRED);
    AT(interaction->auto_pin_readout);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_selection_apply_pick_and_link_keys(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzLinkChannel* channel = dvz_link_channel(scene, "items");
    DvzSelection* selection = dvz_selection(
        scene, &(DvzSelectionDesc){.mode = DVZ_SELECT_TOGGLE, .target = DVZ_SCENE_TARGET_ITEM});
    DvzVisual* visual = dvz_point(scene, 0);
    uint64_t keys[] = {10, 11, 12};
    ANN(channel);
    ANN(selection);
    ANN(visual);

    dvz_visual_set_pick_capabilities(visual, DVZ_PICK_CAPABILITY_ITEM);
    AT(visual->pick_capabilities == DVZ_PICK_CAPABILITY_ITEM);
    AT(dvz_visual_set_link_keys(visual, channel, keys, 3) == 0);
    AT(visual->link_channel == channel);
    AT(visual->link_key_count == 3);
    AT(visual->link_keys[1] == 11);

    DvzPickResult pick = {
        .request_id = 1,
        .status = DVZ_PICK_STATUS_HIT,
        .hit = true,
        .visual_id = 7,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 0,
    };
    AT(dvz_selection_apply_pick(selection, &pick) == 0);
    AT(dvz_selection_count(selection) == 1);
    AT(dvz_selection_apply_pick(selection, &pick) == 0);
    AT(dvz_selection_count(selection) == 0);

    pick.resolved_id = 43;
    selection->desc.mode = DVZ_SELECT_ADDITIVE;
    AT(dvz_selection_apply_pick(selection, &pick) == 0);
    AT(dvz_selection_count(selection) == 1);

    DvzSelectionItem items[2] = {0};
    dvz_selection_copy(selection, items, 2);
    AT(items[0].target == DVZ_SCENE_TARGET_ITEM);
    AT(items[0].target_id == 43);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_text_annotation_bookkeeping(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzFont* font = dvz_font(
        scene,
        &(DvzFontDesc){
            .path = "Demo.ttf",
            .family = "Demo",
            .style = "Regular",
            .size_pts = 14.0f,
            .face_index = 2});
    ANN(font);
    AT(strcmp(font->path, "Demo.ttf") == 0);
    AT(strcmp(font->family, "Demo") == 0);
    AT(strcmp(font->style, "Regular") == 0);
    AT(font->face_index == 2);
    AT(font->version == 1);

    DvzVisual* text = dvz_text(scene, 0);
    ANN(text);
    AT(text->text.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AT(dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_AUTO) == 0);
    AT(text->text.renderer == DVZ_TEXT_RENDERER_AUTO);
    AT(text->text.renderer_version == 1);
    AT(dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    AT(text->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(text->text.renderer_version == 2);
    const char* strings[2] = {"hello", "world"};
    float positions[2][3] = {{10.0f, 20.0f, 0.0f}, {80.0f, 24.0f, 0.0f}};
    float text_anchors[2][2] = {{0.0f, 0.0f}, {0.5f, 0.5f}};
    float sizes[2] = {14.0f, 18.0f};
    float angles[2] = {0.25f, -0.5f};
    DvzColor colors[2] = {{255, 255, 255, 255}, {128, 200, 255, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 2},
        {.attr_name = "size", .data = sizes, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "angle", .data = angles, .item_count = 2},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 2) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(text->type == DVZ_VISUAL_TYPE_TEXT);
    AT(text->text.string_count == 2);
    AT(strcmp(text->text.strings[0], "hello") == 0);
    AT(strcmp(text->text.strings[1], "world") == 0);
    AT(text->text.strings_version == 1);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){
            .text = "peak",
            .style = {
                .font = font,
                .size_pts = 12.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            },
            .placement = {
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            },
        });
    ANN(annotation);
    AT(annotation->kind == DVZ_ANNOTATION_LABEL);
    AT(strcmp(annotation->text, "peak") == 0);
    AT(annotation->style.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(annotation->dirty_flags == DVZ_TEXT_DIRTY_ALL);
    AT(annotation->version == 1);

    annotation->dirty_flags = DVZ_TEXT_DIRTY_NONE;
    dvz_annotation_set_format(annotation, &(DvzFormatDesc){.precision = 3, .suffix = " ms"});
    AT(annotation->has_format);
    AT(strcmp(annotation->format.suffix, " ms") == 0);
    AT(annotation->dirty_flags ==
       (DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER));
    AT(annotation->version == 2);

    dvz_annotation_destroy(annotation);
    dvz_font_destroy(font);
    AT(annotation->scene == NULL);
    AT(font->scene == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_text_bitmap_visual_realization(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = dvz_text(scene, 0);
    ANN(text);
    const char* strings[1] = {"Hi"};
    float positions_text[1][3] = {{10.0f, 20.0f, 0.0f}};
    float text_anchors[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {8.0f};
    float angles[1] = {0.0f};
    DvzColor colors_text[1] = {{64, 128, 255, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions_text, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors_text, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    AT(panel->visual_count == 1);

    _scene_prepare_text_visuals(figure);
    DvzVisual* glyph = text->text.glyph_visual;
    ANN(glyph);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].visual == glyph);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->visible);
    AT(glyph->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(!glyph->depth_test_enabled);
    AT(glyph->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA);
    AT(text->text.realized_version > 0);
    AT(text->text.span_count == 1);
    AT(text->text.spans[0].glyph_count == 2);

    int pos_idx = _attr_index(glyph, "position");
    int uv_idx = _attr_index(glyph, "texcoords");
    int color_idx = _attr_index(glyph, "color");
    AT(pos_idx >= 0);
    AT(uv_idx >= 0);
    AT(color_idx >= 0);
    AT(glyph->attrs[pos_idx].item_count == 12);
    AT(glyph->attrs[uv_idx].item_count == 12);
    AT(glyph->attrs[color_idx].item_count == 12);
    const float(*positions)[3] = (const float(*)[3])glyph->attrs[pos_idx].data;
    ANN(positions);
    AC(positions[0][0], -0.96875f, 1e-6f);
    AC(positions[0][1], 0.9166667f, 1e-6f);
    AC(positions[11][0], -0.93125f, 1e-6f);
    AC(positions[11][1], 0.8833333f, 1e-6f);

    text_anchors[0][0] = 0.5f;
    text_anchors[0][1] = 0.5f;
    AT(dvz_visual_set_data(text, "anchor", text_anchors, 1) == 0);
    _scene_prepare_text_visuals(figure);
    positions = (const float(*)[3])glyph->attrs[pos_idx].data;
    ANN(positions);
    AC(positions[0][0], -0.9875f, 1e-6f);
    AC(positions[0][1], 0.9333333f, 1e-6f);

    const uint8_t* colors = (const uint8_t*)glyph->attrs[color_idx].data;
    ANN(colors);
    AT(colors[0] == 64);
    AT(colors[1] == 128);
    AT(colors[2] == 255);
    AT(colors[3] == 255);

    ANN(glyph->field);
    DvzSampledField* atlas = glyph->field;
    AT(atlas->desc.width == 128);
    AT(atlas->desc.height == 60);
    AT(glyph->field->dirty);

    uint64_t old_visual_version = text->text.realized_version;
    _scene_prepare_text_visuals(figure);
    AT(text->text.realized_version == old_visual_version);
    AT(panel->visual_count == 2);

    strings[0] = "A" "\xCE" "\xA9" "B";
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    _scene_prepare_text_visuals(figure);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->attrs[pos_idx].item_count == 18);
    AT(glyph->attrs[uv_idx].item_count == 18);
    AT(glyph->attrs[color_idx].item_count == 18);
    AT(glyph->field == atlas);
    AT(text->text.spans[0].glyph_count == 3);

    strings[0] = "A";
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    _scene_prepare_text_visuals(figure);
    AT(glyph->attrs[pos_idx].item_count == 6);
    AT(glyph->attrs[uv_idx].item_count == 6);
    AT(glyph->attrs[color_idx].item_count == 6);
    DvzFramePlanVisualMeta metadata = {0};
    uint32_t glyph_index = 0;
    AT(_figure_visual_index(figure, glyph, &glyph_index));
    AT(_scene_visual_frame_plan_metadata(figure, glyph, glyph_index, &metadata));
    AT(metadata.vertex_count == 6);
    AT(metadata.glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA);

    text_anchors[0][0] = 0.0f;
    text_anchors[0][1] = 0.0f;
    AT(dvz_visual_set_data(text, "anchor", text_anchors, 1) == 0);
    _scene_prepare_text_visuals(figure);
    dvz_figure_resize(figure, 320, 240);
    _scene_prepare_text_visuals(figure);
    positions = (const float(*)[3])glyph->attrs[pos_idx].data;
    ANN(positions);
    AC(positions[0][0], -0.9375f, 1e-6f);
    AC(positions[0][1], 0.8333333f, 1e-6f);
    AT(text->text.visual_figure_width == 320);
    AT(text->text.visual_figure_height == 240);

    dvz_visual_set_visible(text, false);
    _scene_prepare_text_visuals(figure);
    AT(!glyph->visible);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){
            .text = "A",
            .style = {
                .size_pts = 8.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
            .placement = {
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT,
                .offset = {-4.0f, -4.0f},
            },
        });
    ANN(annotation);
    _scene_prepare_text_visuals(figure);
    ANN(annotation->visual);
    AT(annotation->visual->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(annotation->visual->visible);
    AT(annotation->visual->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(annotation->visual->field == atlas);
    AT(annotation->dirty_flags == DVZ_TEXT_DIRTY_NONE);
    AT(panel->visual_count == 3);

    dvz_annotation_destroy(annotation);
    AT(!annotation->visual->visible);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify the SDF text renderer realizes through a font-backed glyph atlas.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_sdf_visual_realization(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = dvz_text(scene, 0);
    ANN(text);
    AT(dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"S D"};
    float positions[1][3] = {{32.0f, 48.0f, 0.0f}};
    float text_anchors[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {18.0f};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    DvzVisualDataUpdate updates[4] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    DvzVisual* glyph = text->text.glyph_visual;
    ANN(glyph);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->visible);
    ANN(glyph->field);
    AT(glyph->field->desc.width > 128);
    AT(glyph->field->desc.height > 60);
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    AT(glyph->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#else
    AT(glyph->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA ||
       glyph->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#endif
    AT(scene->font_count == 1);
    DvzTextAtlas* font_atlas =
        scene->fonts[0].msdf_atlas != NULL ? scene->fonts[0].msdf_atlas : scene->fonts[0].sdf_atlas;
    ANN(font_atlas);
    AT(font_atlas->field == glyph->field);
    DvzTextAtlasGlyph* space_glyph = _scene_text_atlas_glyph(font_atlas, ' ');
    ANN(space_glyph);
    AT(space_glyph->advance > 0.0f);
    AT(space_glyph->valid);
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    if (font_atlas->encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB)
    {
        DvzTextAtlasGlyph* atlas_glyph = _scene_text_atlas_glyph(font_atlas, 'S');
        ANN(atlas_glyph);
        float x0 = atlas_glyph->uv[0] * (float)font_atlas->width;
        float y0 = atlas_glyph->uv[1] * (float)font_atlas->height;
        float x1 = atlas_glyph->uv[2] * (float)font_atlas->width;
        float y1 = atlas_glyph->uv[3] * (float)font_atlas->height;
        AT(x0 > atlas_glyph->atlas_bounds[0]);
        AT(y0 > atlas_glyph->atlas_bounds[1]);
        AT(x1 < atlas_glyph->atlas_bounds[2]);
        AT(y1 < atlas_glyph->atlas_bounds[3]);
        const uint8_t* atlas_rgba = (const uint8_t*)font_atlas->field->data;
        ANN(atlas_rgba);
        uint8_t min_alpha = 255;
        uint8_t max_alpha = 0;
        for (uint64_t px = 0; px < (uint64_t)font_atlas->width * font_atlas->height; px++)
        {
            uint8_t alpha = atlas_rgba[4 * px + 3];
            if (alpha < min_alpha)
                min_alpha = alpha;
            if (alpha > max_alpha)
                max_alpha = alpha;
        }
        AT(min_alpha < 250);
        AT(max_alpha > 5);
    }
#endif
    AT(text->text.span_count == 1);
    AT(text->text.spans[0].glyph_count == 2);

    int pos_idx = _attr_index(glyph, "position");
    int uv_idx = _attr_index(glyph, "texcoords");
    int color_idx = _attr_index(glyph, "color");
    AT(pos_idx >= 0);
    AT(uv_idx >= 0);
    AT(color_idx >= 0);
    AT(glyph->attrs[pos_idx].item_count == 12);
    AT(glyph->attrs[uv_idx].item_count == 12);
    AT(glyph->attrs[color_idx].item_count == 12);
    const float(*glyph_positions)[3] = (const float(*)[3])glyph->attrs[pos_idx].data;
    ANN(glyph_positions);
    float first_max_x = glyph_positions[0][0];
    float second_min_x = glyph_positions[6][0];
    for (uint32_t k = 1; k < 6; k++)
        if (glyph_positions[k][0] > first_max_x)
            first_max_x = glyph_positions[k][0];
    for (uint32_t k = 7; k < 12; k++)
        if (glyph_positions[k][0] < second_min_x)
            second_min_x = glyph_positions[k][0];
    AT(second_min_x > first_max_x + 0.005f);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){
            .text = "A",
            .style = {
                .size_pts = 14.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            },
            .placement = {
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
                .offset = {4.0f, 4.0f},
            },
        });
    ANN(annotation);
    _scene_prepare_text_visuals(figure);
    ANN(annotation->visual);
    AT(annotation->visual->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(annotation->visual->visible);
    AT(annotation->visual->field == glyph->field);
    AT(annotation->visual->glyph_atlas_encoding == glyph->glyph_atlas_encoding);
    AT(annotation->dirty_flags == DVZ_TEXT_DIRTY_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify automatic renderer selection uses hinted bitmap for small text when available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_auto_renderer_selection(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* small = dvz_text(scene, 0);
    ANN(small);
    AT(dvz_text_set_renderer(small, DVZ_TEXT_RENDERER_AUTO) == 0);
    const char* small_string[1] = {"small"};
    float small_pos[1][3] = {{24.0f, 24.0f, 0.0f}};
    float small_size[1] = {10.0f};
    DvzVisualDataUpdate small_updates[2] = {
        {.attr_name = "position", .data = small_pos, .item_count = 1},
        {.attr_name = "size", .data = small_size, .item_count = 1},
    };
    AT(dvz_visual_set_strings(small, "text", small_string, 1) == 0);
    AT(dvz_visual_set_data_many(small, small_updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, small,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzVisual* large = dvz_text(scene, 0);
    ANN(large);
    AT(dvz_text_set_renderer(large, DVZ_TEXT_RENDERER_AUTO) == 0);
    const char* large_string[1] = {"large"};
    float large_pos[1][3] = {{24.0f, 64.0f, 0.0f}};
    float large_size[1] = {24.0f};
    DvzVisualDataUpdate large_updates[2] = {
        {.attr_name = "position", .data = large_pos, .item_count = 1},
        {.attr_name = "size", .data = large_size, .item_count = 1},
    };
    AT(dvz_visual_set_strings(large, "text", large_string, 1) == 0);
    AT(dvz_visual_set_data_many(large, large_updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, large,
           &(DvzVisualAttachDesc){.z_layer = 2, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    ANN(small->text.glyph_visual);
    ANN(large->text.glyph_visual);
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    AT(small->text.glyph_visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA);
    AT(scene->font_count >= 1);
    ANN(scene->fonts[0].bitmap_atlas);
    AT(scene->fonts[0].bitmap_atlas->field == small->text.glyph_visual->field);
#else
    AT(small->text.glyph_visual->field == scene->text_bitmap_atlas);
#endif
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    AT(large->text.glyph_visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#else
    AT(large->text.glyph_visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA);
#endif

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify batched text emits many labels through one glyph visual.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_many_labels_render_plan(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    const uint32_t label_count = 16;
    DvzVisual* text = dvz_text(scene, 0);
    ANN(text);
    char labels[16][16] = {{0}};
    const char* strings[16] = {0};
    float positions[16][3] = {{0}};
    float text_anchors[16][2] = {{0}};
    float sizes[16] = {0};
    float angles[16] = {0};
    DvzColor colors[16] = {{0}};
    for (uint32_t i = 0; i < label_count; i++)
    {
        dvz_snprintf(labels[i], sizeof(labels[i]), "%u", i);
        strings[i] = labels[i];
        positions[i][0] = 320.0f + (float)i * 8.0f;
        positions[i][1] = 240.0f;
        text_anchors[i][0] = 0.5f;
        text_anchors[i][1] = 0.5f;
        sizes[i] = 8.0f;
        colors[i][0] = 255;
        colors[i][1] = 255;
        colors[i][2] = 255;
        colors[i][3] = 255;
    }
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = label_count},
        {.attr_name = "anchor", .data = text_anchors, .item_count = label_count},
        {.attr_name = "size", .data = sizes, .item_count = label_count},
        {.attr_name = "color", .data = colors, .item_count = label_count},
        {.attr_name = "angle", .data = angles, .item_count = label_count},
    };
    AT(dvz_visual_set_strings(text, "text", strings, label_count) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    _scene_prepare_text_visuals(figure);
    ANN(text->text.glyph_visual);

    DvzFramePlan* plan = dvz_frame_plan("figure.text.labels", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    uint32_t visual_count = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_RENDER)
            visual_count += plan->nodes[i].u.render.visual_count;
    }
    AT(visual_count == 1);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Register scene interaction tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_interaction(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("interaction");

    TST_CASE(test_scene_interaction_core);
    TST_CASE(test_scene_selection_apply_pick_and_link_keys);
    TST_CASE(test_scene_text_annotation_bookkeeping);
    TST_CASE(test_scene_text_bitmap_visual_realization);
    TST_CASE(test_scene_text_sdf_visual_realization);
    TST_CASE(test_scene_text_auto_renderer_selection);
    TST_CASE(test_scene_text_many_labels_render_plan);

    return 0;
}
