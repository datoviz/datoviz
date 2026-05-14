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
    DvzFont* font = dvz_font(scene, &(DvzFontDesc){.path = "Demo.ttf", .size_pts = 14.0f});
    ANN(font);
    AT(strcmp(font->path, "Demo.ttf") == 0);

    DvzText* text = dvz_text(
        panel,
        &(DvzTextDesc){
            .string = "hello",
            .style = {.font = font, .size_pts = 14.0f},
            .placement = {.mode = DVZ_TEXT_PLACEMENT_SCREEN, .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT},
        });
    ANN(text);
    AT(text->style.font == font);
    AT(strcmp(text->string, "hello") == 0);

    dvz_text_set_string(text, "world");
    dvz_text_set_style(text, &(DvzTextStyle){.font = font, .size_pts = 18.0f});
    dvz_text_set_placement(
        text, &(DvzTextPlacement){.mode = DVZ_TEXT_PLACEMENT_DATA, .anchor = DVZ_SCENE_ANCHOR_DATA});
    AT(strcmp(text->string, "world") == 0);
    AT(text->style.size_pts == 18.0f);
    AT(text->placement.mode == DVZ_TEXT_PLACEMENT_DATA);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){
            .text = "peak",
            .style = {.font = font, .size_pts = 12.0f},
            .placement = {.mode = DVZ_TEXT_PLACEMENT_SCREEN, .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT},
        });
    ANN(annotation);
    AT(annotation->kind == DVZ_ANNOTATION_LABEL);
    AT(strcmp(annotation->text, "peak") == 0);

    dvz_annotation_set_format(annotation, &(DvzFormatDesc){.precision = 3, .suffix = " ms"});
    AT(annotation->has_format);
    AT(strcmp(annotation->format.suffix, " ms") == 0);

    dvz_annotation_destroy(annotation);
    dvz_text_destroy(text);
    dvz_font_destroy(font);
    AT(annotation->scene == NULL);
    AT(text->scene == NULL);
    AT(font->scene == NULL);

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

    return 0;
}
