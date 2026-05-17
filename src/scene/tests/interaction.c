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
#include "../_scene.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_interaction_core(TstSuite* suite, TstItem* item)
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


int test_scene_selection_apply_pick_and_link_keys(TstSuite* suite, TstItem* item)
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
        .hit = true,
        .visual_id = 7,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 42,
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


int test_scene_text_annotation_bookkeeping(TstSuite* suite, TstItem* item)
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

    DvzText* text = dvz_text(
        panel,
        &(DvzTextDesc){
            .string = "hello",
            .style = {
                .font = font,
                .size_pts = 14.0f,
                .renderer = DVZ_TEXT_RENDERER_AUTO,
                .color = {255, 255, 255, 255},
            },
            .placement = {
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
                .angle = 0.25f,
            },
        });
    ANN(text);
    AT(text->style.font == font);
    AT(text->style.renderer == DVZ_TEXT_RENDERER_AUTO);
    AT(strcmp(text->string, "hello") == 0);
    AT(text->dirty_flags == DVZ_TEXT_DIRTY_ALL);
    AT(text->version == 1);

    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
    dvz_text_set_string(text, "world");
    AT(text->dirty_flags ==
       (DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER));
    AT(text->version == 2);

    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
    dvz_text_set_style(
        text,
        &(DvzTextStyle){
            .font = font,
            .size_pts = 18.0f,
            .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
            .color = {255, 255, 255, 255},
        });
    AT(text->dirty_flags == (DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER));
    AT(text->version == 3);

    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
    dvz_text_set_placement(
        text,
        &(DvzTextPlacement){
            .mode = DVZ_TEXT_PLACEMENT_DATA,
            .anchor = DVZ_SCENE_ANCHOR_DATA,
            .angle = -0.5f,
            .depth_test = true,
        });
    AT(strcmp(text->string, "world") == 0);
    AT(text->style.size_pts == 18.0f);
    AT(text->style.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AT(text->placement.mode == DVZ_TEXT_PLACEMENT_DATA);
    AT(text->placement.depth_test);
    AT(text->dirty_flags ==
       (DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER));
    AT(text->version == 4);

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
    dvz_text_destroy(text);
    dvz_font_destroy(font);
    AT(annotation->scene == NULL);
    AT(text->scene == NULL);
    AT(font->scene == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_text_bitmap_visual_realization(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzText* text = dvz_text(
        panel,
        &(DvzTextDesc){
            .string = "Hi",
            .style = {
                .size_pts = 8.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {64, 128, 255, 255},
            },
            .placement = {
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
                .offset = {10.0f, 20.0f},
            },
        });
    ANN(text);
    AT(panel->visual_count == 0);

    _scene_prepare_text_visuals(figure);
    ANN(text->visual);
    AT(panel->visual_count == 1);
    AT(panel->visuals[0].visual == text->visual);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(text->visual->type == DVZ_VISUAL_TYPE_IMAGE);
    AT(text->visual->visible);
    AT(text->visual->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(!text->visual->depth_test_enabled);
    AT(text->visual_version == text->version);
    AT(text->dirty_flags == DVZ_TEXT_DIRTY_NONE);
    AC(text->metrics.advance[0], 12.0f, 1e-6f);
    AC(text->metrics.layout_bounds[3], 8.0f, 1e-6f);

    int pos_idx = _attr_index(text->visual, "position");
    int uv_idx = _attr_index(text->visual, "texcoords");
    AT(pos_idx >= 0);
    AT(uv_idx >= 0);
    AT(text->visual->attrs[pos_idx].item_count == 4);
    AT(text->visual->attrs[uv_idx].item_count == 4);
    const float(*positions)[3] = (const float(*)[3])text->visual->attrs[pos_idx].data;
    ANN(positions);
    AC(positions[0][0], -0.96875f, 1e-6f);
    AC(positions[0][1], 0.9166667f, 1e-6f);
    AC(positions[3][0], -0.93125f, 1e-6f);
    AC(positions[3][1], 0.8833333f, 1e-6f);

    ANN(text->visual->field);
    AT(text->visual->field->desc.width == 12);
    AT(text->visual->field->desc.height == 8);
    AT(text->visual->field->dirty);

    uint64_t old_visual_version = text->visual_version;
    _scene_prepare_text_visuals(figure);
    AT(text->visual_version == old_visual_version);
    AT(panel->visual_count == 1);

    dvz_text_set_string(text, "");
    _scene_prepare_text_visuals(figure);
    AT(!text->visual->visible);

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

    TEST_SIMPLE(test_scene_interaction_core);
    TEST_SIMPLE(test_scene_selection_apply_pick_and_link_keys);
    TEST_SIMPLE(test_scene_text_annotation_bookkeeping);
    TEST_SIMPLE(test_scene_text_bitmap_visual_realization);

    return 0;
}
