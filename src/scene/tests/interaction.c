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

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../../drp2/_stream.h"
#include "frame_plan/frame_plan.h"
#include "_scale_ticks.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "annotation/prepare_internal.h"
#include "datoviz/input.h"
#include "interaction/internal.h"
#include "plot/internal.h"
#include "query/internal.h"
#include "scene_emit/internal.h"
#include "scene_emit/scene_emit.h"
#include "core/units_internal.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "text/internal.h"
#include "text/text_internal.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one generated text atlas from the default scene font.
 *
 * @param scene the scene
 * @param backend atlas backend
 * @param size_px rendered text size
 * @return atlas pointer, or NULL when unavailable
 */
static DvzTextAtlas* text_test_atlas(
    DvzScene* scene, DvzTextAtlasBackend backend, float size_px)
{
    ANN(scene);
    if (scene->font_count == 0)
        return NULL;
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(backend, size_px);
    return _scene_text_atlas_get(&scene->fonts[0], &spec);
}



/**
 * Return one visual attribute by name.
 *
 * @param visual the visual to inspect
 * @param name the retained attribute name
 * @return the attribute, or NULL if missing
 */
static const DvzVisualAttr* _interaction_visual_attr(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    int idx = _attr_index(visual, name);
    return idx >= 0 ? &visual->attrs[idx] : NULL;
}



/**
 * Return whether one stream pipeline with a specific label prefix has a vertex attribute.
 *
 * @param stream the command stream
 * @param label_prefix the pipeline debug label prefix
 * @param format the expected VkFormat
 * @param location the expected shader location
 * @return whether the attribute was found
 */
static bool _interaction_stream_has_pipeline_attr(
    const DvzDrp2CommandStream* stream, const char* label_prefix, uint32_t format,
    uint32_t location)
{
    ANN(stream);
    ANN(label_prefix);
    size_t label_prefix_len = strlen(label_prefix);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* pipeline_label =
            dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (
            pipeline_label == NULL ||
            strncmp(pipeline_label, label_prefix, label_prefix_len) != 0)
            continue;
        for (uint32_t a = 0; a < cmd->u.create_render_pipeline.attr_count; a++)
        {
            if (cmd->u.create_render_pipeline.attr_formats[a] == format &&
                cmd->u.create_render_pipeline.attr_locations[a] == location)
            {
                return true;
            }
        }
    }
    return false;
}



/**
 * Count item-state style bind groups with material and style uniform entries.
 *
 * @param stream the command stream
 * @return number of matching bind groups
 */
static uint32_t
_interaction_stream_item_state_style_bind_group_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            continue;
        if (cmd->u.create_bind_group.entry_count != 2)
            continue;
        const DvzDrp2BindGroupEntry* material = &cmd->u.create_bind_group.entries[0];
        const DvzDrp2BindGroupEntry* style = &cmd->u.create_bind_group.entries[1];
        if (
            material->binding == 0 && material->size == sizeof(DvzSceneMaterialParams) &&
            style->binding == 1 && style->size == sizeof(DvzSceneItemStateStyleParams))
        {
            count++;
        }
    }
    return count;
}



/**
 * Create a pointer event template.
 *
 * @param type event type
 * @param x x coordinate
 * @param y y coordinate
 * @param button pointer button
 * @return pointer event
 */
static DvzPointerEvent
_interaction_pointer_event(DvzPointerEventType type, float x, float y, DvzPointerButton button)
{
    DvzPointerEvent event = {0};
    event.type = type;
    event.pos[0] = x;
    event.pos[1] = y;
    event.button = button;
    return event;
}



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
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_ADDITIVE,
        });
    DvzLinkChannel* channel = dvz_link_channel(scene, "cells");
    ANN(interaction);
    ANN(selection);
    ANN(channel);

    dvz_interaction_bind_panel(interaction, panel);
    dvz_interaction_set_selection(interaction, selection);
    dvz_interaction_set_link_channel(interaction, channel);
    dvz_interaction_set_query_hit_policy(interaction, DVZ_QUERY_HIT_OPAQUE_PREFERRED);
    dvz_interaction_set_auto_pin_readout(interaction, true);

    AT(panel->interaction == interaction);
    AT(interaction->panel == panel);
    AT(interaction->selection == selection);
    AT(interaction->link_channel == channel);
    AT(interaction->query_hit_policy == DVZ_QUERY_HIT_OPAQUE_PREFERRED);
    AT(interaction->auto_pin_readout);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_interaction_descriptor_abi_rejects_invalid_structs(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzSelectionDesc selection_desc = dvz_selection_desc();
    selection_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_selection(scene, &selection_desc) == NULL);

    selection_desc = dvz_selection_desc();
    selection_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_selection(scene, &selection_desc) == NULL);

    DvzHoverDesc hover_desc = dvz_hover_desc();
    hover_desc.struct_size = DVZ_STRUCT_SIZE(DvzHoverDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_hover(scene, &hover_desc) == NULL);

    hover_desc = dvz_hover_desc();
    hover_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_hover(scene, &hover_desc) == NULL);

    DvzHover* hover = dvz_hover(scene, NULL);
    ANN(hover);
    DvzItemStateVisualStyle item_style = dvz_item_state_visual_style();
    item_style.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_hover_set_visual_style(hover, &item_style) < 0);

    item_style = dvz_item_state_visual_style();
    item_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_hover_set_visual_style(hover, &item_style) < 0);

    DvzSelection* selection = dvz_selection(scene, NULL);
    ANN(selection);
    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.struct_size = DVZ_STRUCT_SIZE(DvzSelectionVisualStyle) - 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_selection_set_visual_style(selection, &selection_style) < 0);

    selection_style = dvz_selection_visual_style();
    selection_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_selection_set_visual_style(selection, &selection_style) < 0);

    selection_style = dvz_selection_visual_style();
    selection_style.selected.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_selection_set_visual_style(selection, &selection_style) < 0);

    DvzItemInteractionDesc item_desc = dvz_item_interaction_desc();
    item_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_item_interaction(panel, &item_desc) == NULL);

    item_desc = dvz_item_interaction_desc();
    item_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_item_interaction(panel, &item_desc) == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_text_annotation_descriptor_abi_rejects_invalid_structs(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzFontDesc font_desc = dvz_font_desc();
    font_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_font(scene, &font_desc) == NULL);

    font_desc = dvz_font_desc();
    font_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_font(scene, &font_desc) == NULL);

    DvzFontDefaults font_defaults = dvz_font_defaults();
    font_defaults.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, (dvz_scene_set_font_defaults(scene, &font_defaults), true));

    font_defaults = dvz_font_defaults();
    font_defaults.sans.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, (dvz_scene_set_font_defaults(scene, &font_defaults), true));

    DvzText* text = dvz_text(panel, 0);
    ANN(text);
    DvzTextStyle text_style = dvz_text_style();
    text_style.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_text_set_style(text, &text_style) < 0);

    text_style = dvz_text_style();
    text_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_text_set_style(text, &text_style) < 0);

    DvzTextPlacement text_placement = dvz_text_placement();
    text_placement.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, (dvz_text_set_placement(text, &text_placement), true));

    text_placement = dvz_text_placement();
    text_placement.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, (dvz_text_set_placement(text, &text_placement), true));

    DvzAnnotationDesc annotation_desc = dvz_annotation_desc();
    annotation_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation(panel, &annotation_desc) == NULL);

    annotation_desc = dvz_annotation_desc();
    annotation_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation(panel, &annotation_desc) == NULL);

    DvzLabelDesc label_desc = dvz_label_desc();
    label_desc.struct_size = DVZ_STRUCT_SIZE(DvzLabelDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation_label(panel, &label_desc) == NULL);

    label_desc = dvz_label_desc();
    label_desc.style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation_label(panel, &label_desc) == NULL);

    DvzScaleBarDesc scalebar_desc = dvz_scalebar_desc();
    scalebar_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation_scalebar(panel, &scalebar_desc) == NULL);

    scalebar_desc = dvz_scalebar_desc();
    scalebar_desc.format.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_annotation_scalebar(panel, &scalebar_desc) == NULL);

    dvz_text_destroy(text);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_guide_descriptor_abi_rejects_invalid_structs(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzGuideLineDesc line = dvz_guide_line_desc();
    line.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_line(panel, &line) == NULL);

    line = dvz_guide_line_desc();
    line.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_line(panel, &line) == NULL);

    line = dvz_guide_line_desc();
    line.value = NAN;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_line(panel, &line) == NULL);

    DvzGuideSpanDesc span = dvz_guide_span_desc();
    span.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_span(panel, &span) == NULL);

    span = dvz_guide_span_desc();
    span.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_span(panel, &span) == NULL);

    span = dvz_guide_span_desc();
    span.min_value = 1.0;
    span.max_value = 1.0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_span(panel, &span) == NULL);

    DvzGuideLine* valid_line = dvz_hline(panel, 0.0, NULL);
    ANN(valid_line);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_guide_line_set_value(valid_line, NAN) < 0);

    DvzGuideSpan* valid_span = dvz_vspan(panel, -1.0, 1.0, NULL);
    ANN(valid_span);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_guide_span_set_range(valid_span, 1.0, 1.0) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_bars_descriptor_and_data_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzBarsDesc desc = dvz_bars_desc();
    desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars(panel, &desc) == NULL);

    desc = dvz_bars_desc();
    desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars(panel, &desc) == NULL);

    desc = dvz_bars_desc();
    desc.gap_fraction = 1.0f;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars(panel, &desc) == NULL);

    desc = dvz_bars_desc();
    desc.outline_width_px = NAN;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars(panel, &desc) == NULL);

    DvzBars* bars = dvz_bars(panel, NULL);
    ANN(bars);
    double starts[] = {0.0};
    double ends[] = {1.0};
    double values[] = {2.0};
    AT(dvz_bars_set_intervals(bars, 1, starts, ends, values) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars_set_intervals(bars, 1, NULL, ends, values) < 0);
    ends[0] = starts[0];
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars_set_intervals(bars, 1, starts, ends, values) < 0);
    AT(bars->outline_visual == NULL);
    DvzBarsDesc style = dvz_bars_desc();
    style.outline_width_px = 1.5f;
    AT(dvz_bars_set_style(bars, &style) == 0);
    AT(bars->outline_visual != NULL);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_FILL) == bars->fill_visual);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_OUTLINE) == bars->outline_visual);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_LINE) == NULL);
    style.gap_fraction = NAN;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_bars_set_style(bars, &style) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_band_descriptor_and_data_validation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzBandDesc desc = dvz_band_desc();
    desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band(panel, &desc) == NULL);

    desc = dvz_band_desc();
    desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band(panel, &desc) == NULL);

    desc = dvz_band_desc();
    desc.line_width_px = NAN;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band(panel, &desc) == NULL);

    desc = dvz_band_desc();
    desc.bound_width_px = -1.0f;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band(panel, &desc) == NULL);

    DvzBand* band = dvz_band(panel, NULL);
    ANN(band);
    double x[] = {0.0, 1.0};
    double lower[] = {0.0, 0.5};
    double upper[] = {1.0, 1.5};
    AT(dvz_band_set_bounds(band, 2, x, lower, upper) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band_set_bounds(band, 2, NULL, lower, upper) < 0);
    x[1] = INFINITY;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band_set_bounds(band, 2, x, lower, upper) < 0);
    x[1] = NAN;
    AT(dvz_band_set_bounds(band, 2, x, lower, upper) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band_set_center(band, 2, NULL, lower) < 0);
    AT(band->bounds_visual == NULL);
    DvzBandDesc style = dvz_band_desc();
    style.show_bounds = true;
    AT(dvz_band_set_style(band, &style) == 0);
    AT(band->bounds_visual != NULL);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_FILL) == band->fill_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_LINE) == band->line_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_BOUNDS) == band->bounds_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_OUTLINE) == NULL);
    style.line_width_px = NAN;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_band_set_style(band, &style) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_overlay_descriptor_abi_rejects_invalid_structs(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);
    DvzOverlay* overlay = dvz_overlay(panel, 0);
    ANN(overlay);

    DvzOverlayCardDesc card_desc = dvz_overlay_card_desc();
    card_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card(overlay, &card_desc) == NULL);

    card_desc = dvz_overlay_card_desc();
    card_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card(overlay, &card_desc) == NULL);

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.struct_size = 0;
    card_desc = dvz_overlay_card_desc();
    card_desc.style = &style;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card(overlay, &card_desc) == NULL);

    DvzOverlayCard* card = dvz_overlay_card(overlay, NULL);
    ANN(card);

    style = dvz_overlay_card_style();
    style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card_set_style(card, &style) < 0);

    DvzOverlayRichTextDesc rich_desc = dvz_overlay_rich_text_desc();
    rich_desc.struct_size = 0;
    rich_desc.source = "invalid";
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card_set_rich_text(card, &rich_desc) < 0);

    rich_desc = dvz_overlay_rich_text_desc();
    rich_desc.flags = 1;
    rich_desc.source = "invalid";
    AT_EXPECTED_ERROR_STRICT(suite, dvz_overlay_card_set_rich_text(card, &rich_desc) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_item_interaction_defaults_and_lifetime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(figure);
    ANN(panel);

    DvzItemInteractionDesc desc = dvz_item_interaction_desc();
    AT(desc.hover_enabled);
    AT(desc.selection_enabled);
    AT(desc.select_mode == DVZ_SELECT_TOGGLE);
    AT(desc.target == DVZ_SCENE_TARGET_ITEM);
    AT(desc.hit_policy == DVZ_QUERY_HIT_FRONTMOST);
    AT(desc.clear_hover_on_miss);
    AT(desc.clear_selection_on_miss);

    DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
    ANN(pick);
    AT(pick->active);
    AT(pick->scene == scene);
    AT(pick->panel == panel);
    AT(panel->item_interaction == pick);
    AT(pick->owns_hover);
    AT(pick->owns_selection);
    AT(dvz_item_interaction_hover(pick) != NULL);
    AT(dvz_item_interaction_selection(pick) != NULL);
    AT(dvz_item_interaction_selection(pick)->desc.mode == DVZ_SELECT_TOGGLE);
    AT(dvz_item_interaction_selection(pick)->desc.target == DVZ_SCENE_TARGET_ITEM);

    dvz_item_interaction_destroy(pick);
    AT(!pick->active);
    AT(panel->item_interaction == NULL);
    AT(dvz_item_interaction_hover(pick) == NULL);
    AT(dvz_item_interaction_selection(pick) == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_item_interaction_input_queries(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzInputRouter* router = dvz_input_router();
    ANN(figure);
    ANN(panel);
    ANN(router);

    DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
    ANN(pick);
    AT(dvz_panel_connect_input(panel, router) == 0);

    DvzPointerEvent move =
        _interaction_pointer_event(DVZ_POINTER_EVENT_MOVE, 12.0f, 15.0f, DVZ_POINTER_BUTTON_NONE);
    dvz_input_emit_pointer(router, &move);
    AT(scene->pending_query_count == 1);
    AT(scene->pending_queries[0].panel == panel);
    AT(scene->pending_queries[0].item_interaction == pick);
    AT(scene->pending_queries[0].item_interaction_kind == DVZ_ITEM_INTERACTION_QUERY_HOVER);
    AT(scene->pending_queries[0].request.target == DVZ_SCENE_TARGET_ITEM);
    AT(scene->pending_queries[0].request.hit_policy == DVZ_QUERY_HIT_FRONTMOST);
    AC(scene->pending_queries[0].x, 12.0, 1e-6);
    AC(scene->pending_queries[0].y, 15.0, 1e-6);

    DvzPointerEvent click =
        _interaction_pointer_event(DVZ_POINTER_EVENT_CLICK, 20.0f, 25.0f, DVZ_POINTER_BUTTON_LEFT);
    dvz_input_emit_pointer(router, &click);
    AT(scene->pending_query_count == 2);
    AT(scene->pending_queries[1].item_interaction == pick);
    AT(scene->pending_queries[1].item_interaction_kind == DVZ_ITEM_INTERACTION_QUERY_SELECTION);
    AC(scene->pending_queries[1].x, 20.0, 1e-6);
    AC(scene->pending_queries[1].y, 25.0, 1e-6);

    DvzPointerEvent outside =
        _interaction_pointer_event(DVZ_POINTER_EVENT_MOVE, 400.0f, 400.0f, DVZ_POINTER_BUTTON_NONE);
    dvz_input_emit_pointer(router, &outside);
    AT(scene->pending_query_count == 2);

    dvz_panel_connect_input(panel, NULL);
    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_item_interaction_applies_results(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(figure);
    ANN(panel);

    DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
    DvzHover* hover = dvz_item_interaction_hover(pick);
    DvzSelection* selection = dvz_item_interaction_selection(pick);
    ANN(pick);
    ANN(hover);
    ANN(selection);

    DvzQueryResult hit = {
        .request_id = 1,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = 7,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 3,
        .link_key = 11,
    };
    _scene_item_interaction_apply_query_result(pick, DVZ_ITEM_INTERACTION_QUERY_HOVER, &hit);
    AT(hover->has_item);
    AT(hover->item.visual_id == 7);
    AT(hover->item.target_id == 3);

    _scene_item_interaction_apply_query_result(pick, DVZ_ITEM_INTERACTION_QUERY_SELECTION, &hit);
    AT(dvz_selection_count(selection) == 1);

    DvzQueryResult miss = {.request_id = 2, .status = DVZ_QUERY_STATUS_MISS, .hit = false};
    _scene_item_interaction_apply_query_result(pick, DVZ_ITEM_INTERACTION_QUERY_HOVER, &miss);
    AT(!hover->has_item);
    _scene_item_interaction_apply_query_result(pick, DVZ_ITEM_INTERACTION_QUERY_SELECTION, &miss);
    AT(dvz_selection_count(selection) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_selection_apply_query_and_link_keys(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzLinkChannel* channel = dvz_link_channel(scene, "items");
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    DvzVisual* visual = dvz_point(scene, 0);
    uint64_t keys[] = {10, 11, 12};
    ANN(channel);
    ANN(selection);
    ANN(visual);

    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);
    AT(visual->query_capabilities == DVZ_QUERY_CAPABILITY_ITEM);
    AT(dvz_visual_set_link_keys(visual, channel, keys, 3) == 0);
    AT(visual->link_channel == channel);
    AT(visual->link_key_count == 3);
    AT(visual->link_keys[1] == 11);
    DvzSelectionVisualStyle style = dvz_selection_visual_style();
    AT(style.selected.visual_flags == DVZ_ITEM_STATE_VISUAL_NONE);
    AT(style.unselected.visual_flags == DVZ_ITEM_STATE_VISUAL_ALPHA);
    AC(style.unselected.alpha, 0.25f, 1e-6f);
    AC(style.selected.scale, 1.0f, 1e-6f);
    AC(dvz_item_state_visual_style().scale, 1.0f, 1e-6f);
    style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    style.selected.tint = (DvzColor){255, 183, 3, 255};
    style.selected.tint_mix = 1.0f;
    style.unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_NONE;
    AT(dvz_selection_set_visual_style(selection, &style) == 0);
    AT(selection->visual_style.selected.visual_flags == DVZ_ITEM_STATE_VISUAL_TINT);
    AT(selection->visual_style.unselected.visual_flags == DVZ_ITEM_STATE_VISUAL_NONE);

    DvzQueryResult query = {
        .request_id = 1,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = 7,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 0,
        .link_key = 10,
    };
    AT(dvz_selection_apply_query(selection, &query) == 0);
    AT(dvz_selection_count(selection) == 1);
    AT(dvz_selection_apply_query(selection, &query) == 0);
    AT(dvz_selection_count(selection) == 0);

    query.resolved_id = 43;
    query.link_key = 0;
    selection->desc.mode = DVZ_SELECT_ADDITIVE;
    AT(dvz_selection_apply_query(selection, &query) == 0);
    AT(dvz_selection_count(selection) == 1);

    DvzSelectionItem items[2] = {0};
    dvz_selection_copy(selection, items, 2);
    AT(items[0].target == DVZ_SCENE_TARGET_ITEM);
    AT(items[0].target_id == 43);
    AT(items[0].link_key == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_selection_apply_query_updates_item_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzLinkChannel* channel = dvz_link_channel(scene, "items");
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* pixel = dvz_pixel(scene, 0);
    DvzVisual* marker = dvz_marker(scene, 0);
    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(panel);
    ANN(channel);
    ANN(selection);
    ANN(point);
    ANN(pixel);
    ANN(marker);
    ANN(sphere);

    vec3 point_pos[3] = {{0.0f, 0.0f, 0.0f}, {0.25f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}};
    DvzColor point_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float point_size[3] = {8.0f, 8.0f, 8.0f};
    uint64_t point_keys[3] = {10, 20, 30};
    AT(dvz_visual_set_data(point, "position", point_pos, 3) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 3) == 0);
    AT(dvz_visual_set_data(point, "size", point_size, 3) == 0);
    AT(dvz_visual_set_link_keys(point, channel, point_keys, 3) == 0);
    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    point_style.stroke_width = 2.0f;
    point_style.edge_color = (DvzColor){0, 0, 0, 255};
    AT(dvz_point_set_style(point, &point_style) == 0);
    AT(dvz_visual_set_depth_cue(
           point,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
               .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
               .falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR,
               .near_depth = 0.0f,
               .far_depth = 10.0f,
               .strength = 0.5f,
               .background_color = {1.0f, 1.0f, 1.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    vec3 pixel_pos[3] = {{0.0f, -0.25f, 0.0f}, {0.25f, -0.25f, 0.0f}, {0.5f, -0.25f, 0.0f}};
    DvzColor pixel_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float pixel_size[3] = {8.0f, 8.0f, 8.0f};
    uint64_t pixel_keys[3] = {60, 20, 70};
    AT(dvz_visual_set_data(pixel, "position", pixel_pos, 3) == 0);
    AT(dvz_visual_set_data(pixel, "color", pixel_color, 3) == 0);
    AT(dvz_visual_set_data(pixel, "size", pixel_size, 3) == 0);
    AT(dvz_visual_set_link_keys(pixel, channel, pixel_keys, 3) == 0);
    AT(dvz_visual_set_depth_cue(
           pixel,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
               .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
               .falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR,
               .near_depth = 0.0f,
               .far_depth = 10.0f,
               .strength = 0.5f,
               .background_color = {1.0f, 1.0f, 1.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    vec3 marker_pos[3] = {{0.0f, 0.25f, 0.0f}, {0.25f, 0.25f, 0.0f}, {0.5f, 0.25f, 0.0f}};
    DvzColor marker_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float marker_size[3] = {12.0f, 12.0f, 12.0f};
    float marker_angle[3] = {0.0f, 0.0f, 0.0f};
    uint32_t marker_shape[3] = {
        DVZ_MARKER_SHAPE_DISC,
        DVZ_MARKER_SHAPE_DISC,
        DVZ_MARKER_SHAPE_DISC,
    };
    uint64_t marker_keys[3] = {40, 20, 50};
    AT(dvz_visual_set_data(marker, "position", marker_pos, 3) == 0);
    AT(dvz_visual_set_data(marker, "color", marker_color, 3) == 0);
    AT(dvz_visual_set_data(marker, "size", marker_size, 3) == 0);
    AT(dvz_visual_set_data(marker, "angle", marker_angle, 3) == 0);
    AT(dvz_visual_set_data(marker, "shape", marker_shape, 3) == 0);
    AT(dvz_visual_set_link_keys(marker, channel, marker_keys, 3) == 0);
    AT(dvz_panel_add_visual(panel, marker, NULL) == 0);

    vec3 sphere_pos[3] = {{0.0f, -0.50f, 0.0f}, {0.25f, -0.50f, 0.0f}, {0.5f, -0.50f, 0.0f}};
    DvzColor sphere_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float sphere_radius[3] = {0.05f, 0.05f, 0.05f};
    uint64_t sphere_keys[3] = {80, 20, 90};
    AT(dvz_visual_set_data(sphere, "position", sphere_pos, 3) == 0);
    AT(dvz_visual_set_data(sphere, "color", sphere_color, 3) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sphere_radius, 3) == 0);
    AT(dvz_visual_set_link_keys(sphere, channel, sphere_keys, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzQueryResult query = {
        .request_id = 1,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = _scene_visual_public_id(scene, point),
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 1,
        .link_key = 20,
    };
    AT(dvz_selection_apply_query(selection, &query) == 0);

    int point_state_idx = _attr_index(point, "item_state");
    int pixel_state_idx = _attr_index(pixel, "item_state");
    int marker_state_idx = _attr_index(marker, "item_state");
    int sphere_state_idx = _attr_index(sphere, "item_state");
    AT(point_state_idx >= 0);
    AT(pixel_state_idx >= 0);
    AT(marker_state_idx >= 0);
    AT(sphere_state_idx >= 0);
    const uint32_t* point_state = (const uint32_t*)point->attrs[point_state_idx].data;
    const uint32_t* pixel_state = (const uint32_t*)pixel->attrs[pixel_state_idx].data;
    const uint32_t* marker_state = (const uint32_t*)marker->attrs[marker_state_idx].data;
    const uint32_t* sphere_state = (const uint32_t*)sphere->attrs[sphere_state_idx].data;
    ANN(point_state);
    ANN(pixel_state);
    ANN(marker_state);
    ANN(sphere_state);
    AT(point_state[0] == DVZ_ITEM_STATE_NONE);
    AT(point_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(point_state[2] == DVZ_ITEM_STATE_NONE);
    AT(pixel_state[0] == DVZ_ITEM_STATE_NONE);
    AT(pixel_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(pixel_state[2] == DVZ_ITEM_STATE_NONE);
    AT(marker_state[0] == DVZ_ITEM_STATE_NONE);
    AT(marker_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(marker_state[2] == DVZ_ITEM_STATE_NONE);
    AT(sphere_state[0] == DVZ_ITEM_STATE_NONE);
    AT(sphere_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(sphere_state[2] == DVZ_ITEM_STATE_NONE);
    AT(point->attrs[point_state_idx].dirty_item_count == 3);
    AT(pixel->attrs[pixel_state_idx].dirty_item_count == 3);
    AT(marker->attrs[marker_state_idx].dirty_item_count == 3);
    AT(sphere->attrs[sphere_state_idx].dirty_item_count == 3);
    AT(_visual_family_state(point)->item_state_style_params.unselected[0] ==
       (float)DVZ_ITEM_STATE_VISUAL_ALPHA);
    AC(_visual_family_state(point)->item_state_style_params.unselected[1], 0.25f, 1e-6f);
    AT(_visual_family_state(pixel)->item_state_style_params.unselected[0] ==
       (float)DVZ_ITEM_STATE_VISUAL_ALPHA);
    AC(_visual_family_state(pixel)->item_state_style_params.unselected[1], 0.25f, 1e-6f);
    AT(_visual_family_state(marker)->item_state_style_params.unselected[0] ==
       (float)DVZ_ITEM_STATE_VISUAL_ALPHA);
    AC(_visual_family_state(marker)->item_state_style_params.unselected[1], 0.25f, 1e-6f);
    AT(_visual_family_state(sphere)->item_state_style_params.unselected[0] ==
       (float)DVZ_ITEM_STATE_VISUAL_ALPHA);
    AC(_visual_family_state(sphere)->item_state_style_params.unselected[1], 0.25f, 1e-6f);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_interaction_stream_has_pipeline_attr(
        stream, "_pipe_point_item_stateg", VK_FORMAT_R32_UINT, 5));
    AT(_interaction_stream_has_pipeline_attr(
        stream, "_pipe_pixel_item_stateg", VK_FORMAT_R32_UINT, 5));
    AT(_interaction_stream_has_pipeline_attr(
        stream, "_pipe_marker_item_stateg", VK_FORMAT_R32_UINT, 5));
    AT(_interaction_stream_has_pipeline_attr(
        stream, "_pipe_sphere_item_stateg", VK_FORMAT_R32_UINT, 5));
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneItemStateStyleParams)) == 4);
    AT(_interaction_stream_item_state_style_bind_group_count(stream) == 4);
    _test_scene_stream_destroy(stream);

    AT(point->attrs[point_state_idx].dirty_item_count == 0);
    AT(pixel->attrs[pixel_state_idx].dirty_item_count == 0);
    AT(marker->attrs[marker_state_idx].dirty_item_count == 0);
    AT(sphere->attrs[sphere_state_idx].dirty_item_count == 0);
    AT(!_visual_family_state(point)->item_state_style_params_dirty);
    AT(!_visual_family_state(pixel)->item_state_style_params_dirty);
    AT(!_visual_family_state(marker)->item_state_style_params_dirty);
    AT(!_visual_family_state(sphere)->item_state_style_params_dirty);
    AT(dvz_selection_apply_query(selection, &query) == 0);
    AT(dvz_selection_count(selection) == 0);
    point_state = (const uint32_t*)point->attrs[point_state_idx].data;
    pixel_state = (const uint32_t*)pixel->attrs[pixel_state_idx].data;
    marker_state = (const uint32_t*)marker->attrs[marker_state_idx].data;
    sphere_state = (const uint32_t*)sphere->attrs[sphere_state_idx].data;
    AT(point_state[0] == DVZ_ITEM_STATE_NONE);
    AT(point_state[1] == DVZ_ITEM_STATE_NONE);
    AT(point_state[2] == DVZ_ITEM_STATE_NONE);
    AT(pixel_state[0] == DVZ_ITEM_STATE_NONE);
    AT(pixel_state[1] == DVZ_ITEM_STATE_NONE);
    AT(pixel_state[2] == DVZ_ITEM_STATE_NONE);
    AT(marker_state[0] == DVZ_ITEM_STATE_NONE);
    AT(marker_state[1] == DVZ_ITEM_STATE_NONE);
    AT(marker_state[2] == DVZ_ITEM_STATE_NONE);
    AT(sphere_state[0] == DVZ_ITEM_STATE_NONE);
    AT(sphere_state[1] == DVZ_ITEM_STATE_NONE);
    AT(sphere_state[2] == DVZ_ITEM_STATE_NONE);
    AT(point->attrs[point_state_idx].dirty_item_count == 3);
    AT(pixel->attrs[pixel_state_idx].dirty_item_count == 3);
    AT(marker->attrs[marker_state_idx].dirty_item_count == 3);
    AT(sphere->attrs[sphere_state_idx].dirty_item_count == 3);
    AT(_visual_family_state(point)->item_state_style_params.unselected[0] == 0.0f);
    AT(_visual_family_state(pixel)->item_state_style_params.unselected[0] == 0.0f);
    AT(_visual_family_state(marker)->item_state_style_params.unselected[0] == 0.0f);
    AT(_visual_family_state(sphere)->item_state_style_params.unselected[0] == 0.0f);

    DvzHover* hover = dvz_hover(
        scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    ANN(hover);
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.5f;
    AT(dvz_hover_set_visual_style(hover, &hover_style) == 0);
    AT(dvz_hover_apply_query(hover, &query) == 0);
    point_state = (const uint32_t*)point->attrs[point_state_idx].data;
    pixel_state = (const uint32_t*)pixel->attrs[pixel_state_idx].data;
    marker_state = (const uint32_t*)marker->attrs[marker_state_idx].data;
    sphere_state = (const uint32_t*)sphere->attrs[sphere_state_idx].data;
    AT(point_state[1] == DVZ_ITEM_STATE_HOVERED);
    AT(pixel_state[1] == DVZ_ITEM_STATE_HOVERED);
    AT(marker_state[1] == DVZ_ITEM_STATE_HOVERED);
    AT(sphere_state[1] == DVZ_ITEM_STATE_HOVERED);
    AC(_visual_family_state(point)->item_state_style_params.hovered[3], 1.5f, 1e-6f);
    AT(dvz_selection_apply_query(selection, &query) == 0);
    point_state = (const uint32_t*)point->attrs[point_state_idx].data;
    pixel_state = (const uint32_t*)pixel->attrs[pixel_state_idx].data;
    marker_state = (const uint32_t*)marker->attrs[marker_state_idx].data;
    sphere_state = (const uint32_t*)sphere->attrs[sphere_state_idx].data;
    AT(point_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(pixel_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(marker_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(sphere_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    DvzQueryResult miss = {.request_id = 2, .status = DVZ_QUERY_STATUS_MISS, .hit = false};
    AT(dvz_hover_apply_query(hover, &miss) == 0);
    point_state = (const uint32_t*)point->attrs[point_state_idx].data;
    pixel_state = (const uint32_t*)pixel->attrs[pixel_state_idx].data;
    marker_state = (const uint32_t*)marker->attrs[marker_state_idx].data;
    sphere_state = (const uint32_t*)sphere->attrs[sphere_state_idx].data;
    AT(point_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(pixel_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(marker_state[1] == DVZ_ITEM_STATE_SELECTED);
    AT(sphere_state[1] == DVZ_ITEM_STATE_SELECTED);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pixel_hover_selection_item_state(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzVisual* pixel = dvz_pixel(scene, 0);
    ANN(pixel);
    dvz_visual_set_query_capabilities(pixel, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[4] = {
        {-0.40f, -0.40f, 0.0f},
        {+0.40f, -0.40f, 0.0f},
        {-0.40f, +0.40f, 0.0f},
        {+0.40f, +0.40f, 0.0f},
    };
    DvzColor colors[4] = {
        {80, 160, 220, 255},
        {80, 160, 220, 255},
        {80, 160, 220, 255},
        {80, 160, 220, 255},
    };
    float size[4] = {18.0f, 18.0f, 18.0f, 18.0f};
    AT(dvz_visual_set_data(pixel, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(pixel, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(pixel, "pixel_size", size, 4) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzHover* hover = dvz_hover(
        scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    ANN(hover);
    ANN(selection);

    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.35f;
    AT(dvz_hover_set_visual_style(hover, &hover_style) == 0);

    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = (DvzColor){255, 190, 64, 255};
    selection_style.selected.tint_mix = 1.0f;
    AT(dvz_selection_set_visual_style(selection, &selection_style) == 0);

    DvzQueryResult hit = {
        .request_id = 8,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = _scene_visual_public_id(scene, pixel),
        .visual_family = DVZ_SCENE_VISUAL_FAMILY_PIXEL,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 2,
        .item_id = 2,
    };
    AT(dvz_hover_apply_query(hover, &hit) == 0);

    int state_idx = _attr_index(pixel, "item_state");
    AT(state_idx >= 0);
    const uint32_t* item_state = (const uint32_t*)pixel->attrs[state_idx].data;
    ANN(item_state);
    AT(item_state[0] == DVZ_ITEM_STATE_NONE);
    AT(item_state[1] == DVZ_ITEM_STATE_NONE);
    AT(item_state[2] == DVZ_ITEM_STATE_HOVERED);
    AT(item_state[3] == DVZ_ITEM_STATE_NONE);
    AC(_visual_family_state(pixel)->item_state_style_params.hovered[3], 1.35f, 1e-6f);

    AT(dvz_selection_apply_query(selection, &hit) == 0);
    item_state = (const uint32_t*)pixel->attrs[state_idx].data;
    AT(item_state[2] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(dvz_selection_count(selection) == 1);
    AT(pixel->attrs[state_idx].dirty_item_count == 4);

    DvzQueryResult miss = {
        .request_id = 9,
        .status = DVZ_QUERY_STATUS_MISS,
        .hit = false,
    };
    AT(dvz_hover_apply_query(hover, &miss) == 0);
    item_state = (const uint32_t*)pixel->attrs[state_idx].data;
    AT(item_state[2] == DVZ_ITEM_STATE_SELECTED);

    dvz_selection_clear(selection);
    item_state = (const uint32_t*)pixel->attrs[state_idx].data;
    AT(item_state[2] == DVZ_ITEM_STATE_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sphere_hover_selection_item_state(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    dvz_visual_set_query_capabilities(sphere, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[3] = {
        {-0.45f, 0.0f, 0.0f},
        {+0.00f, 0.0f, 0.0f},
        {+0.45f, 0.0f, 0.0f},
    };
    DvzColor colors[3] = {
        {100, 170, 230, 255},
        {100, 170, 230, 255},
        {100, 170, 230, 255},
    };
    float radii[3] = {0.08f, 0.08f, 0.08f};
    AT(dvz_visual_set_data(sphere, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radii, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzHover* hover = dvz_hover(
        scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    ANN(hover);
    ANN(selection);

    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.4f;
    AT(dvz_hover_set_visual_style(hover, &hover_style) == 0);

    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = (DvzColor){255, 190, 64, 255};
    selection_style.selected.tint_mix = 1.0f;
    AT(dvz_selection_set_visual_style(selection, &selection_style) == 0);

    DvzQueryResult hit = {
        .request_id = 12,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = _scene_visual_public_id(scene, sphere),
        .visual_family = DVZ_SCENE_VISUAL_FAMILY_SPHERE,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 1,
        .item_id = 1,
    };
    AT(dvz_hover_apply_query(hover, &hit) == 0);

    int state_idx = _attr_index(sphere, "item_state");
    AT(state_idx >= 0);
    const uint32_t* item_state = (const uint32_t*)sphere->attrs[state_idx].data;
    ANN(item_state);
    AT(item_state[0] == DVZ_ITEM_STATE_NONE);
    AT(item_state[1] == DVZ_ITEM_STATE_HOVERED);
    AT(item_state[2] == DVZ_ITEM_STATE_NONE);
    AC(_visual_family_state(sphere)->item_state_style_params.hovered[3], 1.4f, 1e-6f);

    AT(dvz_selection_apply_query(selection, &hit) == 0);
    item_state = (const uint32_t*)sphere->attrs[state_idx].data;
    AT(item_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(dvz_selection_count(selection) == 1);
    AT(sphere->attrs[state_idx].dirty_item_count == 3);

    DvzQueryResult miss = {
        .request_id = 13,
        .status = DVZ_QUERY_STATUS_MISS,
        .hit = false,
    };
    AT(dvz_hover_apply_query(hover, &miss) == 0);
    item_state = (const uint32_t*)sphere->attrs[state_idx].data;
    AT(item_state[1] == DVZ_ITEM_STATE_SELECTED);

    dvz_selection_clear(selection);
    item_state = (const uint32_t*)sphere->attrs[state_idx].data;
    AT(item_state[1] == DVZ_ITEM_STATE_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_instance_hover_selection_item_state(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    dvz_visual_set_query_capabilities(mesh, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 positions[4] = {
        {-0.25f, -0.25f, 0.0f},
        {-0.25f, +0.25f, 0.0f},
        {+0.25f, -0.25f, 0.0f},
        {+0.25f, +0.25f, 0.0f},
    };
    DvzColor colors[4] = {
        {100, 170, 230, 255},
        {100, 170, 230, 255},
        {100, 170, 230, 255},
        {100, 170, 230, 255},
    };
    float transforms[2][16] = {
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.45f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.45f, 0, 0, 1},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(mesh, "instance_transform", transforms, 2) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzHover* hover = dvz_hover(
        scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    ANN(hover);
    ANN(selection);

    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.25f;
    AT(dvz_hover_set_visual_style(hover, &hover_style) == 0);

    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = (DvzColor){255, 190, 64, 255};
    selection_style.selected.tint_mix = 1.0f;
    AT(dvz_selection_set_visual_style(selection, &selection_style) == 0);

    DvzQueryResult hit = {
        .request_id = 21,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .visual_id = _scene_visual_public_id(scene, mesh),
        .visual_family = DVZ_SCENE_VISUAL_FAMILY_MESH,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 1,
        .item_id = 1,
    };
    AT(dvz_hover_apply_query(hover, &hit) == 0);

    int state_idx = _attr_index(mesh, "item_state");
    AT(state_idx >= 0);
    const uint32_t* item_state = (const uint32_t*)mesh->attrs[state_idx].data;
    ANN(item_state);
    AT(item_state[0] == DVZ_ITEM_STATE_NONE);
    AT(item_state[1] == DVZ_ITEM_STATE_HOVERED);
    AC(_visual_family_state(mesh)->item_state_style_params.hovered[3], 1.25f, 1e-6f);

    AT(dvz_selection_apply_query(selection, &hit) == 0);
    item_state = (const uint32_t*)mesh->attrs[state_idx].data;
    AT(item_state[1] == (DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED));
    AT(dvz_selection_count(selection) == 1);
    AT(mesh->attrs[state_idx].dirty_item_count == 2);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_interaction_stream_has_pipeline_attr(
        stream, "_pipe_prim_t3_inst_item_stateg", VK_FORMAT_R32_UINT, 7));
    AT(_interaction_stream_item_state_style_bind_group_count(stream) == 1);
    _test_scene_stream_destroy(stream);

    DvzQueryResult miss = {
        .request_id = 22,
        .status = DVZ_QUERY_STATUS_MISS,
        .hit = false,
    };
    AT(dvz_hover_apply_query(hover, &miss) == 0);
    item_state = (const uint32_t*)mesh->attrs[state_idx].data;
    AT(item_state[1] == DVZ_ITEM_STATE_SELECTED);

    dvz_selection_clear(selection);
    item_state = (const uint32_t*)mesh->attrs[state_idx].data;
    AT(item_state[1] == DVZ_ITEM_STATE_NONE);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_selection_card_realizes_query_metadata(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_REPLACE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    ANN(figure);
    ANN(panel);
    ANN(selection);

    DvzQueryResult query = {
        .request_id = 1,
        .status = DVZ_QUERY_STATUS_HIT,
        .hit = true,
        .panel_id = _scene_panel_public_id(figure, panel),
        .visual_id = 7,
        .item_id = 42,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 42,
        .link_key = 123,
        .panel_position = {20.0, 30.0},
    };
    AT(dvz_selection_apply_query(selection, &query) == 0);
    AT(selection->card_panel == panel);
    AT(selection->card.visible);
    AT(selection->card.dirty);
    AT(strcmp(selection->card.text, "visual 7 item 42 key 123") == 0);

    _scene_prepare_text_visuals(figure);
    AT(selection->card.background_visual != NULL);
    AT(selection->card.text_visual != NULL);
    AT(_visual_family_state(selection->card.text_visual)->text.glyph_visual != NULL);
    AT(selection->card.background_visual->visible);
    AT(selection->card.text_visual->visible);
    AT(_visual_family_state(selection->card.text_visual)->text.glyph_visual->visible);
    AT(!selection->card.dirty);

    dvz_selection_clear(selection);
    AT(selection->card_panel == NULL);
    AT(!selection->card.visible);
    AT(!selection->card.background_visual->visible);
    AT(!selection->card.text_visual->visible);
    AT(!_visual_family_state(selection->card.text_visual)->text.glyph_visual->visible);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_overlay_card_public_api(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(figure);
    ANN(panel);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(10, 20, 30, 220);
    style.text_color = dvz_color_rgb(230, 240, 250);
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "overlay",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT,
            .anchor_px = {40.0f, 50.0f},
            .offset_px = {8.0f, 6.0f},
            .style = &style});
    ANN(overlay);
    ANN(card);
    AT(card->active);
    AT(card->panel == panel);
    AT(card->card.visible);
    AT(strcmp(card->card.text, "overlay") == 0);

    _scene_prepare_text_visuals(figure);
    AT(card->card.background_visual != NULL);
    AT(card->card.text_visual != NULL);
    AT(_visual_family_state(card->card.text_visual)->text.glyph_visual != NULL);
    AT(_visual_family_state(card->card.text_visual)->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(card->card.background_visual->visible);
    AT(card->card.text_visual->visible);
    AT(_visual_family_state(card->card.text_visual)->text.glyph_visual->visible);
    const DvzVisualAttr* glyph_position =
        _interaction_visual_attr(_visual_family_state(card->card.text_visual)->text.glyph_visual, "position");
    const DvzVisualAttr* glyph_bounds =
        _interaction_visual_attr(_visual_family_state(card->card.text_visual)->text.glyph_visual, "bounds");
    const DvzVisualAttr* glyph_texcoords =
        _interaction_visual_attr(_visual_family_state(card->card.text_visual)->text.glyph_visual, "texcoords");
    const DvzVisualAttr* glyph_color =
        _interaction_visual_attr(_visual_family_state(card->card.text_visual)->text.glyph_visual, "color");
    const DvzVisualAttr* glyph_angle =
        _interaction_visual_attr(_visual_family_state(card->card.text_visual)->text.glyph_visual, "angle");
    ANN(glyph_position);
    ANN(glyph_bounds);
    ANN(glyph_texcoords);
    ANN(glyph_color);
    ANN(glyph_angle);
    AT(glyph_position->item_count == style.max_text_chars * 6u);
    AT(glyph_bounds->item_count == glyph_position->item_count);
    AT(glyph_texcoords->item_count == glyph_position->item_count);
    AT(glyph_color->item_count == glyph_position->item_count);
    AT(glyph_angle->item_count == glyph_position->item_count);

    const uint32_t live_vertices = (uint32_t)strlen("overlay") * 6u;
    const float* positions = (const float*)glyph_position->data;
    const float* bounds = (const float*)glyph_bounds->data;
    const float* texcoords = (const float*)glyph_texcoords->data;
    const uint8_t* colors = (const uint8_t*)glyph_color->data;
    const float* angles = (const float*)glyph_angle->data;
    bool padding_zero = true;
    for (uint32_t i = live_vertices; i < glyph_position->item_count; i++)
    {
        padding_zero =
            padding_zero && positions[3 * i + 0] == 0.0f && positions[3 * i + 1] == 0.0f &&
            positions[3 * i + 2] == 0.0f && bounds[4 * i + 0] == 0.0f &&
            bounds[4 * i + 1] == 0.0f && bounds[4 * i + 2] == 0.0f &&
            bounds[4 * i + 3] == 0.0f && texcoords[4 * i + 0] == 0.0f &&
            texcoords[4 * i + 1] == 0.0f && texcoords[4 * i + 2] == 0.0f &&
            texcoords[4 * i + 3] == 0.0f && colors[4 * i + 0] == 0u &&
            colors[4 * i + 1] == 0u && colors[4 * i + 2] == 0u &&
            colors[4 * i + 3] == 0u && angles[i] == 0.0f;
    }
    AT(padding_zero);
    AT(card->card.realized_rect_px[0] > 560.0f);
    AC(card->card.realized_rect_px[1], 6.0f, 1e-6f);

    dvz_overlay_card_set_text(card, "updated");
    float anchor[2] = {100.0f, 120.0f};
    float offset[2] = {2.0f, 3.0f};
    dvz_overlay_card_set_layout(card, anchor, offset);
    AT(card->card.dirty);
    AT(strcmp(card->card.text, "updated") == 0);
    AT(card->card.anchor_px[0] == 100.0f);
    AT(card->card.offset_px[1] == 3.0f);
    AT(card->card.placement == DVZ_OVERLAY_CARD_PLACEMENT_PIXEL);

    style.text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    style.height_px = 28.0f;
    AT(dvz_overlay_card_set_style(card, &style) == 0);
    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(card->card.text_visual)->text.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AC(card->card.realized_rect_px[3], 28.0f, 1e-6f);

    float inset[2] = {12.0f, 14.0f};
    dvz_overlay_card_set_placement(card, DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT, inset);
    _scene_prepare_text_visuals(figure);
    AC(card->card.realized_rect_px[0], 12.0f, 1e-6f);
    AC(card->card.realized_rect_px[1] + card->card.realized_rect_px[3], 466.0f, 1e-6f);

    dvz_overlay_card_set_visible(card, false);
    AT(!card->card.visible);
    AT(!card->card.background_visual->visible);
    dvz_overlay_card_set_visible(card, true);
    _scene_prepare_text_visuals(figure);
    AT(card->card.background_visual->visible);

    dvz_overlay_destroy(overlay);
    AT(!overlay->active);
    AT(!card->active);
    AT(!card->card.background_visual->visible);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_overlay_card_rich_text_public_api(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(figure);
    ANN(panel);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    ANN(overlay);
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(8, 12, 20, 235);
    style.padding_px[0] = 10.0f;
    style.padding_px[1] = 8.0f;
    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "fallback",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT,
            .offset_px = {16.0f, 18.0f},
            .style = &style});
    ANN(card);

    AT(dvz_overlay_card_set_rich_text(
           card,
           &(DvzOverlayRichTextDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayRichTextDesc),
               .source = "Rich <b>card</b> <u><color=#2A80E6>blue</color></u>",
               .max_width_px = 126.0f,
               .char_width_px = 7.0f,
               .line_height_px = 12.0f,
               .scale = 2.0f,
               .text_color = {235, 240, 250, 255},
               .background_color = {0, 0, 0, 0},
           }) == 0);
    AT(card->rich_enabled);
    AT(card->rich_dirty);

    _scene_prepare_text_visuals(figure);
    AT(!card->rich_dirty);
    AT(card->card.content == DVZ_SCENE_CARD_CONTENT_IMAGE);
    AT(card->rich_block.rgba != NULL);
    AT(card->rich_block.raster_width > 0);
    AT(card->rich_block.raster_height > 0);
    AC(card->rich_block.raster_scale, 2.0f, 1e-6f);
    ANN(card->card.background_visual);
    ANN(card->rich_block.image_visual);
    AT(card->card.background_visual->visible);
    AT(card->rich_block.image_visual->visible);
    AT(card->card.text_visual == NULL || !card->card.text_visual->visible);
    AT(card->card.realized_rect_px[2] >= card->card.content_size_px[0]);
    AT(card->card.realized_rect_px[3] >= card->card.content_size_px[1]);
    AT(card->rich_block.image_raster_version == card->rich_block.raster_version);

    const DvzVisualAttr* first_position_px =
        _interaction_visual_attr(card->rich_block.image_visual, "position_px");
    const DvzVisualAttr* first_extent_px =
        _interaction_visual_attr(card->rich_block.image_visual, "extent_px");
    const DvzVisualAttr* first_anchor =
        _interaction_visual_attr(card->rich_block.image_visual, "anchor");
    ANN(first_position_px);
    ANN(first_extent_px);
    ANN(first_anchor);
    uint64_t first_position_version = first_position_px->version;
    uint64_t first_extent_version = first_extent_px->version;
    uint64_t first_anchor_version = first_anchor->version;
    uint64_t first_texture_version = _visual_family_state(card->rich_block.image_visual)->texture.version;
    _scene_prepare_text_visuals(figure);
    AT(first_position_px->version == first_position_version);
    AT(first_extent_px->version == first_extent_version);
    AT(first_anchor->version == first_anchor_version);
    AT(_visual_family_state(card->rich_block.image_visual)->texture.version == first_texture_version);
    AT(card->rich_block.image_raster_version == card->rich_block.raster_version);

    DvzVisual* image_visual = card->rich_block.image_visual;
    DvzSampledField* image_field = card->rich_block.image_field;
    uint32_t visual_count = scene->visual_count;
    for (uint32_t i = 0; i < 8; i++)
    {
        char source[128] = {0};
        int n = dvz_snprintf(
            source, sizeof(source), "Updated <b>rich card</b> sample %u", i);
        AT(n > 0 && (size_t)n < sizeof(source));
        AT(dvz_overlay_card_set_rich_text(
               card,
               &(DvzOverlayRichTextDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayRichTextDesc),
                   .source = source,
                   .max_width_px = 126.0f,
                   .char_width_px = 7.0f,
                   .line_height_px = 12.0f,
                   .scale = 2.0f,
                   .text_color = {235, 240, 250, 255},
                   .background_color = {0, 0, 0, 0},
               }) == 0);
        _scene_prepare_text_visuals(figure);
        AT(card->rich_block.image_visual == image_visual);
        AT(card->rich_block.image_field == image_field);
        AT(scene->visual_count == visual_count);
        AT(!card->rich_dirty);
    }

    dvz_overlay_card_set_visible(card, false);
    AT(!card->card.background_visual->visible);
    AT(!card->rich_block.image_visual->visible);

    dvz_overlay_card_set_visible(card, true);
    _scene_prepare_text_visuals(figure);
    AT(card->card.background_visual->visible);
    AT(card->rich_block.image_visual->visible);

    dvz_overlay_card_clear_rich_text(card);
    AT(!card->rich_enabled);
    AT(card->rich_block.image_visual == NULL);
    AT(card->card.content == DVZ_SCENE_CARD_CONTENT_TEXT);
    _scene_prepare_text_visuals(figure);
    AT(card->card.text_visual != NULL);
    AT(card->card.text_visual->visible);

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
        &(DvzFontDesc){DVZ_STRUCT_INIT_FIELDS(DvzFontDesc),
            .path = "Demo.ttf",
            .family = "Demo",
            .style = "Regular",
            .face_index = 2});
    ANN(font);
    AT(strcmp(font->path, "Demo.ttf") == 0);
    AT(strcmp(font->family, "Demo") == 0);
    AT(strcmp(font->style, "Regular") == 0);
    AT(font->face_index == 2);
    AT(font->version == 1);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_visual_family_state(text)->text.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_AUTO) == 0);
    AT(_visual_family_state(text)->text.renderer == DVZ_TEXT_RENDERER_AUTO);
    AT(_visual_family_state(text)->text.renderer_version == 1);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    AT(_visual_family_state(text)->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    AT(_visual_family_state(text)->text.renderer_version == 2);
    const char* strings[2] = {"hello", "world"};
    vec3 positions[2] = {{10.0f, 20.0f, 0.0f}, {80.0f, 24.0f, 0.0f}};
    vec2 text_anchors[2] = {{0.0f, 0.0f}, {0.5f, 0.5f}};
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
    AT(_visual_family_state(text)->text.string_count == 2);
    AT(strcmp(_visual_family_state(text)->text.strings[0], "hello") == 0);
    AT(strcmp(_visual_family_state(text)->text.strings[1], "world") == 0);
    AT(_visual_family_state(text)->text.strings_version == 1);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
            .text = "peak",
            .style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .font = font,
                .size_px = 12.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            },
            .placement = {DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
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
    dvz_annotation_set_format(annotation, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 3, .suffix = " ms"});
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


int test_scene_guide_line_and_span_prepare_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -2.0, 2.0) == 0);

    DvzGuideLineDesc line_desc = dvz_guide_line_desc();
    line_desc.color = dvz_color_rgba(76, 201, 240, 220);
    line_desc.stroke_width_px = 3.0f;
    line_desc.label = "threshold";
    DvzGuideLine* hline = dvz_hline(panel, 0.5, &line_desc);
    ANN(hline);
    AT(scene->guide_line_count == 1);
    AT(hline->line_visual != NULL);
    AT(hline->line_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(dvz_guide_line_visual(hline, DVZ_PLOT_ROLE_LINE) == hline->line_visual);
    AT(dvz_guide_line_visual(hline, DVZ_PLOT_ROLE_FILL) == NULL);

    DvzGuideSpanDesc span_desc = dvz_guide_span_desc();
    span_desc.fill_color = dvz_color_rgba(239, 71, 111, 48);
    span_desc.outline_color = dvz_color_rgba(239, 71, 111, 180);
    span_desc.outline_width_px = 2.0f;
    span_desc.label = "window";
    DvzGuideSpan* vspan = dvz_vspan(panel, 2.0, 4.0, &span_desc);
    ANN(vspan);
    AT(scene->guide_span_count == 1);
    AT(vspan->fill_visual != NULL);
    AT(vspan->fill_visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(vspan->outline_visual != NULL);
    AT(vspan->outline_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(dvz_guide_span_visual(vspan, DVZ_PLOT_ROLE_FILL) == vspan->fill_visual);
    AT(dvz_guide_span_visual(vspan, DVZ_PLOT_ROLE_OUTLINE) == vspan->outline_visual);
    AT(dvz_guide_span_visual(vspan, DVZ_PLOT_ROLE_LINE) == NULL);

    _scene_prepare_guide_visuals(figure);
    ANN(hline->label);
    ANN(vspan->label);
    AT(hline->label->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN);
    AT(hline->label->placement.anchor == DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT);
    AT(vspan->label->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN);
    AC(hline->label->placement.position[0], 400.0, 1e-4);
    AC(hline->label->placement.position[1], 225.0, 1e-4);
    AC(vspan->label->placement.position[0], 240.0, 1e-4);
    AC(vspan->label->placement.position[1], 300.0, 1e-4);

    DvzVisualDataView line_start_view = {0};
    DvzVisualDataView line_end_view = {0};
    DvzVisualDataView line_width_view = {0};
    AT(dvz_visual_data(hline->line_visual, "position_start", &line_start_view) == 0);
    AT(dvz_visual_data(hline->line_visual, "position_end", &line_end_view) == 0);
    AT(dvz_visual_data(hline->line_visual, "stroke_width", &line_width_view) == 0);
    const float* line_start = (const float*)line_start_view.data;
    const float* line_end = (const float*)line_end_view.data;
    const float* line_width = (const float*)line_width_view.data;
    AT(line_start_view.item_count == 1);
    AC(line_start[0], 0.0f, 1e-6f);
    AC(line_start[1], 0.5f, 1e-6f);
    AC(line_end[0], 10.0f, 1e-6f);
    AC(line_end[1], 0.5f, 1e-6f);
    AC(line_width[0], 3.0f, 1e-6f);

    DvzVisualDataView fill_position_view = {0};
    AT(dvz_visual_data(vspan->fill_visual, "position", &fill_position_view) == 0);
    const float* fill_positions = (const float*)fill_position_view.data;
    AT(fill_position_view.item_count == 6);
    AC(fill_positions[0], 2.0f, 1e-6f);
    AC(fill_positions[1], -2.0f, 1e-6f);
    AC(fill_positions[3], 4.0f, 1e-6f);
    AC(fill_positions[4], -2.0f, 1e-6f);
    AC(fill_positions[6], 4.0f, 1e-6f);
    AC(fill_positions[7], 2.0f, 1e-6f);

    DvzVisualDataView outline_start_view = {0};
    DvzVisualDataView outline_end_view = {0};
    AT(dvz_visual_data(vspan->outline_visual, "position_start", &outline_start_view) == 0);
    AT(dvz_visual_data(vspan->outline_visual, "position_end", &outline_end_view) == 0);
    const float* outline_start = (const float*)outline_start_view.data;
    const float* outline_end = (const float*)outline_end_view.data;
    AT(outline_start_view.item_count == 4);
    AC(outline_start[0], 2.0f, 1e-6f);
    AC(outline_start[1], -2.0f, 1e-6f);
    AC(outline_end[0], 4.0f, 1e-6f);
    AC(outline_end[1], -2.0f, 1e-6f);

    AT(dvz_guide_line_set_value(hline, 1.0) == 0);
    AT(dvz_guide_span_set_range(vspan, 5.0, 6.0) == 0);
    _scene_prepare_guide_visuals(figure);

    AT(dvz_visual_data(hline->line_visual, "position_start", &line_start_view) == 0);
    AT(dvz_visual_data(hline->line_visual, "position_end", &line_end_view) == 0);
    line_start = (const float*)line_start_view.data;
    line_end = (const float*)line_end_view.data;
    AC(line_start[1], 1.0f, 1e-6f);
    AC(line_end[1], 1.0f, 1e-6f);

    AT(dvz_visual_data(vspan->fill_visual, "position", &fill_position_view) == 0);
    fill_positions = (const float*)fill_position_view.data;
    AC(fill_positions[0], 5.0f, 1e-6f);
    AC(fill_positions[3], 6.0f, 1e-6f);
    AT(hline->label->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN);
    AC(hline->label->placement.position[0], 400.0, 1e-4);
    AC(hline->label->placement.position[1], 150.0, 1e-4);
    AT(vspan->label->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN);
    AC(vspan->label->placement.position[0], 440.0, 1e-4);
    AC(vspan->label->placement.position[1], 300.0, 1e-4);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_bars_prepare_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzBarsDesc desc = dvz_bars_desc();
    desc.fill_color = dvz_color_rgba(76, 201, 240, 180);
    desc.outline_color = dvz_color_rgba(30, 30, 36, 220);
    desc.outline_width_px = 1.5f;
    DvzBars* bars = dvz_bars(panel, &desc);
    ANN(bars);
    AT(scene->bars_count == 1);
    AT(bars->fill_visual != NULL);
    AT(bars->fill_visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(bars->outline_visual != NULL);
    AT(bars->outline_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_FILL) == bars->fill_visual);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_OUTLINE) == bars->outline_visual);
    AT(dvz_bars_visual(bars, DVZ_PLOT_ROLE_LINE) == NULL);

    const double starts[] = {0.0, 1.0};
    const double ends[] = {1.0, 2.0};
    const double values[] = {2.0, -1.0};
    AT(dvz_bars_set_intervals(bars, 2, starts, ends, values) == 0);

    DvzBarsDesc horizontal_desc = dvz_bars_desc();
    horizontal_desc.orientation = DVZ_BARS_ORIENTATION_HORIZONTAL;
    horizontal_desc.baseline = -1.0;
    horizontal_desc.gap_fraction = 0.2f;
    horizontal_desc.fill_color = dvz_color_rgba(239, 71, 111, 160);
    DvzBars* horizontal = dvz_bars(panel, &horizontal_desc);
    ANN(horizontal);
    const double hstarts[] = {0.0};
    const double hends[] = {10.0};
    const double hvalues[] = {3.0};
    AT(dvz_bars_set_intervals(horizontal, 1, hstarts, hends, hvalues) == 0);

    _scene_prepare_bars_visuals(figure);

    DvzVisualDataView fill_position_view = {0};
    AT(dvz_visual_data(bars->fill_visual, "position", &fill_position_view) == 0);
    const float* fill_positions = (const float*)fill_position_view.data;
    AT(fill_position_view.item_count == 12);
    AC(fill_positions[0], 0.0f, 1e-6f);
    AC(fill_positions[1], 0.0f, 1e-6f);
    AC(fill_positions[3], 1.0f, 1e-6f);
    AC(fill_positions[4], 0.0f, 1e-6f);
    AC(fill_positions[6], 1.0f, 1e-6f);
    AC(fill_positions[7], 2.0f, 1e-6f);
    AC(fill_positions[9], 0.0f, 1e-6f);
    AC(fill_positions[10], 0.0f, 1e-6f);
    AC(fill_positions[12], 1.0f, 1e-6f);
    AC(fill_positions[13], 2.0f, 1e-6f);
    AC(fill_positions[15], 0.0f, 1e-6f);
    AC(fill_positions[16], 2.0f, 1e-6f);
    AC(fill_positions[18], 1.0f, 1e-6f);
    AC(fill_positions[19], 0.0f, 1e-6f);
    AC(fill_positions[24], 2.0f, 1e-6f);
    AC(fill_positions[25], -1.0f, 1e-6f);

    DvzVisualDataView outline_start_view = {0};
    DvzVisualDataView outline_end_view = {0};
    DvzVisualDataView outline_width_view = {0};
    AT(dvz_visual_data(bars->outline_visual, "position_start", &outline_start_view) == 0);
    AT(dvz_visual_data(bars->outline_visual, "position_end", &outline_end_view) == 0);
    AT(dvz_visual_data(bars->outline_visual, "stroke_width", &outline_width_view) == 0);
    const float* outline_start = (const float*)outline_start_view.data;
    const float* outline_end = (const float*)outline_end_view.data;
    const float* outline_width = (const float*)outline_width_view.data;
    AT(outline_start_view.item_count == 8);
    AC(outline_start[0], 0.0f, 1e-6f);
    AC(outline_start[1], 0.0f, 1e-6f);
    AC(outline_end[0], 1.0f, 1e-6f);
    AC(outline_end[1], 0.0f, 1e-6f);
    AC(outline_width[0], 1.5f, 1e-6f);

    DvzVisualDataView horizontal_position_view = {0};
    AT(dvz_visual_data(horizontal->fill_visual, "position", &horizontal_position_view) == 0);
    const float* horizontal_positions = (const float*)horizontal_position_view.data;
    AT(horizontal_position_view.item_count == 6);
    AC(horizontal_positions[0], -1.0f, 1e-6f);
    AC(horizontal_positions[1], 1.0f, 1e-6f);
    AC(horizontal_positions[3], 3.0f, 1e-6f);
    AC(horizontal_positions[4], 1.0f, 1e-6f);
    AC(horizontal_positions[6], 3.0f, 1e-6f);
    AC(horizontal_positions[7], 9.0f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_band_prepare_visuals(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzBandDesc desc = dvz_band_desc();
    desc.fill_color = dvz_color_rgba(128, 255, 219, 64);
    desc.line_color = dvz_color_rgba(76, 201, 240, 255);
    desc.line_width_px = 5.0f;
    desc.show_bounds = true;
    desc.bound_color = dvz_color_rgba(128, 255, 219, 180);
    desc.bound_width_px = 1.25f;
    DvzBand* band = dvz_band(panel, &desc);
    ANN(band);
    AT(scene->band_count == 1);
    AT(band->fill_visual != NULL);
    AT(band->fill_visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(band->line_visual != NULL);
    AT(band->line_visual->type == DVZ_VISUAL_TYPE_PATH);
    AT(band->bounds_visual != NULL);
    AT(band->bounds_visual->type == DVZ_VISUAL_TYPE_PATH);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_FILL) == band->fill_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_LINE) == band->line_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_BOUNDS) == band->bounds_visual);
    AT(dvz_band_visual(band, DVZ_PLOT_ROLE_OUTLINE) == NULL);

    const double x[] = {0.0, 1.0, NAN, 3.0, 4.0};
    const double lower[] = {0.0, 0.5, NAN, 1.0, 1.5};
    const double upper[] = {1.0, 1.5, NAN, 2.0, 2.5};
    const double center_y[] = {0.7, 1.1, NAN, 1.8, 2.0};
    AT(dvz_band_set_bounds(band, 5, x, lower, upper) == 0);
    AT(dvz_band_set_center(band, 5, x, center_y) == 0);

    _scene_prepare_band_visuals(figure);

    DvzVisualDataView fill_position_view = {0};
    AT(dvz_visual_data(band->fill_visual, "position", &fill_position_view) == 0);
    const float* fill_positions = (const float*)fill_position_view.data;
    AT(fill_position_view.item_count == 12);
    AC(fill_positions[0], 0.0f, 1e-6f);
    AC(fill_positions[1], 0.0f, 1e-6f);
    AC(fill_positions[3], 1.0f, 1e-6f);
    AC(fill_positions[4], 0.5f, 1e-6f);
    AC(fill_positions[6], 1.0f, 1e-6f);
    AC(fill_positions[7], 1.5f, 1e-6f);
    AC(fill_positions[18], 3.0f, 1e-6f);
    AC(fill_positions[19], 1.0f, 1e-6f);

    DvzVisualDataView line_position_view = {0};
    DvzVisualDataView line_width_view = {0};
    AT(dvz_visual_data(band->line_visual, "position", &line_position_view) == 0);
    AT(dvz_visual_data(band->line_visual, "stroke_width", &line_width_view) == 0);
    const float* line_positions = (const float*)line_position_view.data;
    const float* line_widths = (const float*)line_width_view.data;
    AT(line_position_view.item_count == 4);
    AC(line_positions[0], 0.0f, 1e-6f);
    AC(line_positions[1], 0.7f, 1e-6f);
    AC(line_positions[3], 1.0f, 1e-6f);
    AC(line_positions[4], 1.1f, 1e-6f);
    AC(line_widths[0], 5.0f, 1e-6f);

    DvzVisualDataView bounds_position_view = {0};
    DvzVisualDataView bounds_width_view = {0};
    AT(dvz_visual_data(band->bounds_visual, "position", &bounds_position_view) == 0);
    AT(dvz_visual_data(band->bounds_visual, "stroke_width", &bounds_width_view) == 0);
    const float* bounds_positions = (const float*)bounds_position_view.data;
    const float* bounds_widths = (const float*)bounds_width_view.data;
    AT(bounds_position_view.item_count == 8);
    AC(bounds_positions[0], 0.0f, 1e-6f);
    AC(bounds_positions[1], 0.0f, 1e-6f);
    AC(bounds_positions[6], 0.0f, 1e-6f);
    AC(bounds_positions[7], 1.0f, 1e-6f);
    AC(bounds_widths[0], 1.25f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check scale-bar nice-length selection and SI unit formatting.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_formatting(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    double units = 0.0;
    float px = 0.0f;
    AT(_scene_scalebar_choose_length(2.5e-5, 120.0f, 70.0f, 180.0f, &units, &px));
    AC(units, 0.002, 1e-12);
    AC(px, 80.0f, 1e-5f);

    char label[64] = {0};
    _scene_format_si_value(0.002, "m", label, sizeof(label));
    AT(strcmp(label, "2 mm") == 0);
    _scene_format_si_value(0.0002, "m", label, sizeof(label));
    AT(strcmp(label, "200 um") == 0);
    _scene_format_si_value(0.02, "m", label, sizeof(label));
    AT(strcmp(label, "2 cm") == 0);
    _scene_format_si_value(2000.0, "m", label, sizeof(label));
    AT(strcmp(label, "2 km") == 0);
    return 0;
}


/**
 * Check unit-ladder formatting core behavior.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_units_formatting_core(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzUnits* duration = dvz_units_builtin(scene, DVZ_UNIT_LADDER_DURATION, 1e-3);
    ANN(duration);
    char label[64] = {0};
    AT(_scene_units_format(duration, 50.0, NULL, label, sizeof(label)));
    AT(strcmp(label, "50 ms") == 0);
    AT(_scene_units_format(duration, 2500.0, NULL, label, sizeof(label)));
    AT(strcmp(label, "2.5 s") == 0);

    DvzUnits* raw = dvz_units_builtin(scene, DVZ_UNIT_LADDER_RAW, 1.0);
    ANN(raw);
    AT(_scene_units_format(raw, 42.0, NULL, label, sizeof(label)));
    AT(strcmp(label, "42") == 0);

    DvzUnitLadder* genome = dvz_unit_ladder_create(scene, "bp");
    ANN(genome);
    AT(dvz_unit_ladder_add(genome, 1e6, "Mb") == 0);
    AT(dvz_unit_ladder_add(genome, 1.0, "bp") == 0);
    AT(dvz_unit_ladder_add(genome, 1e3, "kb") == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_unit_ladder_add(genome, 1e3, "kilobase") < 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_unit_ladder_add(genome, 1e9, "kb") < 0);

    DvzUnits* bp = dvz_units_create(scene);
    ANN(bp);
    AT(dvz_units_ladder(bp, genome) == 0);
    AT(_scene_units_format(bp, 2500000.0, NULL, label, sizeof(label)));
    AT(strcmp(label, "2.5 Mb") == 0);

    DvzUnitFormatContext axis_ctx = {
        .mode = DVZ_UNIT_DISPLAY_AXIS_STABLE,
        .has_axis_range = true,
        .axis_data_min = 0.0,
        .axis_data_max = 2500000.0,
    };
    AT(_scene_units_format(bp, 1000.0, &axis_ctx, label, sizeof(label)));
    AT(strcmp(label, "0.001 Mb") == 0);

    AT(dvz_units_fixed_label(bp, "kb") == 0);
    AT(_scene_units_format(bp, 2500000.0, NULL, label, sizeof(label)));
    AT(strcmp(label, "2500 kb") == 0);

    DvzDateTimeFormat* dt =
        dvz_datetime_format_builtin(scene, DVZ_DATETIME_FORMAT_CONCISE_UTC);
    ANN(dt);
    DvzTimestamp ts = (DvzTimestamp)1714566896123456LL; /* 2024-05-01 12:34:56.123456 UTC */
    AT(_scene_datetime_format(dt, ts, DVZ_TIME_INTERVAL_MICROSECOND, label, sizeof(label)));
    AT(strcmp(label, "12:34:56.123456") == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_datetime_format_timezone(dt, "Europe/Paris") < 0);

    DvzDateTimeFormat* custom_dt = dvz_datetime_format_create(scene);
    ANN(custom_dt);
    AT(dvz_datetime_format_rule(
           custom_dt, DVZ_TIME_INTERVAL_MILLISECOND, "%b %d %H:%M:%S.fff") == 0);
    AT(_scene_datetime_format(
        custom_dt, ts, DVZ_TIME_INTERVAL_MILLISECOND, label, sizeof(label)));
    AT(strcmp(label, "May 01 12:34:56.123") == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check retained 2D scale-bar lowering, panzoom/domain updates, and DPI line width.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_2d_realization(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.01) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .unit = "m",
            .target_length_px = 120.0f,
            .min_length_px = 70.0f,
            .max_length_px = 180.0f,
            .offset_px = {20.0f, 20.0f},
            .line_width_px = 2.0f,
            .line_color = {255, 255, 255, 255},
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 10.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
        });
    ANN(scalebar);
    _scene_prepare_text_visuals(figure);
    ANN(scalebar->scalebar_visual);
    ANN(scalebar->visual);
    AT(scalebar->kind == DVZ_ANNOTATION_SCALEBAR);
    AT(scalebar->scalebar_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(scalebar->visual->type == DVZ_VISUAL_TYPE_TEXT);
    AT(scalebar->scalebar_visual->visible);
    AT(scalebar->visual->visible);
    AT(_visual_family_state(scalebar->visual)->text.glyph_visual != NULL);
    AT(_visual_family_state(scalebar->visual)->text.glyph_visual->visible);
    AT(strcmp(scalebar->text, "2 mm") == 0);
    AC(scalebar->scalebar_units, 0.002, 1e-12);
    AC(scalebar->scalebar_px, 80.0f, 1e-5f);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.001) == 0);
    _scene_prepare_text_visuals(figure);
    AT(strcmp(scalebar->text, "200 um") == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 400;
    cfg.device_scale_x = 2.0f;
    cfg.device_scale_y = 2.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    const DvzVisualAttr* label_size_attr = _interaction_visual_attr(scalebar->visual, "size");
    ANN(label_size_attr);
    AT(label_size_attr->item_count == 1);
    const float* label_size = (const float*)label_size_attr->data;
    ANN(label_size);
    AC(label_size[0], 10.0f, 1e-6f);
    AC(scalebar->scalebar_realization.screen_scale, 2.0f, 1e-6f);
    AC(scalebar->scalebar_realization.label_size, 10.0f, 1e-6f);

    bool saw_scaled_width = false;
    bool saw_glyph_pipeline = false;
    bool saw_glyph_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_glyph") != NULL)
                saw_glyph_pipeline = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            if (cmd->u.draw.vertex_count >= 6 && cmd->u.draw.instance_count == 1)
                saw_glyph_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label == NULL || strstr(label, "line_width") == NULL)
                continue;
            const float* width = (const float*)cmd->u.write_buffer.data_raw;
            ANN(width);
            AC(width[0], 4.0f, 1e-6f);
            saw_scaled_width = true;
        }
    }
    AT(saw_scaled_width);
    AT(saw_glyph_pipeline);
    AT(saw_glyph_draw);

    _test_scene_stream_destroy(stream);
    dvz_annotation_destroy(scalebar);
    AT(!scalebar->scalebar_visual->visible);
    AT(!scalebar->visual->visible);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check retained scale-bar unit objects and duration labels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_duration_units(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 250.0) == 0);

    DvzUnits* duration = dvz_units_builtin(scene, DVZ_UNIT_LADDER_DURATION, 1e-3);
    ANN(duration);
    DvzScaleBar* scalebar = dvz_scalebar(panel);
    ANN(scalebar);
    AT(dvz_scalebar_set_units(scalebar, duration) == 0);

    _scene_prepare_text_visuals(figure);
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    AT(strcmp(annotation->text, "50 ms") == 0);
    AT(strstr(annotation->text, "cs") == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check scale-bar updates dirty only changed retained text/glyph payloads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_update_churn(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.010) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .unit = "m",
            .target_length_px = 120.0f,
            .min_length_px = 70.0f,
            .max_length_px = 180.0f,
            .offset_px = {20.0f, 20.0f},
            .line_width_px = 2.0f,
            .line_color = {255, 255, 255, 255},
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 10.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
        });
    ANN(scalebar);
    _scene_prepare_text_visuals(figure);
    ANN(scalebar->visual);
    AT(_visual_family_state(scalebar->visual)->text.glyph_visual != NULL);
    DvzVisual* glyph = _visual_family_state(scalebar->visual)->text.glyph_visual;

    const DvzVisualAttr* glyph_position = _interaction_visual_attr(glyph, "position");
    const DvzVisualAttr* glyph_bounds = _interaction_visual_attr(glyph, "bounds");
    const DvzVisualAttr* glyph_texcoords = _interaction_visual_attr(glyph, "texcoords");
    const DvzVisualAttr* glyph_color = _interaction_visual_attr(glyph, "color");
    const DvzVisualAttr* glyph_angle = _interaction_visual_attr(glyph, "angle");
    const DvzVisualAttr* segment_start =
        _interaction_visual_attr(scalebar->scalebar_visual, "position_start");
    ANN(glyph_position);
    ANN(glyph_bounds);
    ANN(glyph_texcoords);
    ANN(glyph_color);
    ANN(glyph_angle);
    ANN(segment_start);
    AT(glyph_position->item_count == 72u);
    AT(strcmp(scalebar->text, "2 mm") == 0);

    uint64_t strings_version = _visual_family_state(scalebar->visual)->text.strings_version;
    uint64_t glyph_position_version = glyph_position->version;
    uint64_t glyph_bounds_version = glyph_bounds->version;
    uint64_t glyph_texcoords_version = glyph_texcoords->version;
    uint64_t glyph_color_version = glyph_color->version;
    uint64_t glyph_angle_version = glyph_angle->version;
    uint64_t segment_start_version = segment_start->version;

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.011) == 0);
    _scene_prepare_text_visuals(figure);
    AT(strcmp(scalebar->text, "2 mm") == 0);
    glyph_position = _interaction_visual_attr(glyph, "position");
    glyph_bounds = _interaction_visual_attr(glyph, "bounds");
    glyph_texcoords = _interaction_visual_attr(glyph, "texcoords");
    glyph_color = _interaction_visual_attr(glyph, "color");
    glyph_angle = _interaction_visual_attr(glyph, "angle");
    segment_start = _interaction_visual_attr(scalebar->scalebar_visual, "position_start");
    ANN(glyph_position);
    ANN(glyph_bounds);
    ANN(glyph_texcoords);
    ANN(glyph_color);
    ANN(glyph_angle);
    ANN(segment_start);
    AT(_visual_family_state(scalebar->visual)->text.strings_version == strings_version);
    AT(glyph_position->version > glyph_position_version);
    AT(glyph_bounds->version == glyph_bounds_version);
    AT(glyph_texcoords->version == glyph_texcoords_version);
    AT(glyph_color->version == glyph_color_version);
    AT(glyph_angle->version == glyph_angle_version);
    AT(segment_start->version > segment_start_version);
    AT(glyph_position->item_count == 72u);

    glyph_position_version = glyph_position->version;
    segment_start_version = segment_start->version;
    _scene_prepare_text_visuals(figure);
    glyph_position = _interaction_visual_attr(glyph, "position");
    segment_start = _interaction_visual_attr(scalebar->scalebar_visual, "position_start");
    ANN(glyph_position);
    ANN(segment_start);
    AT(glyph_position->version == glyph_position_version);
    AT(segment_start->version == segment_start_version);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.001) == 0);
    _scene_prepare_text_visuals(figure);
    AT(strcmp(scalebar->text, "200 um") == 0);
    glyph_position = _interaction_visual_attr(glyph, "position");
    glyph_bounds = _interaction_visual_attr(glyph, "bounds");
    glyph_texcoords = _interaction_visual_attr(glyph, "texcoords");
    glyph_color = _interaction_visual_attr(glyph, "color");
    glyph_angle = _interaction_visual_attr(glyph, "angle");
    ANN(glyph_position);
    ANN(glyph_bounds);
    ANN(glyph_texcoords);
    ANN(glyph_color);
    ANN(glyph_angle);
    AT(_visual_family_state(scalebar->visual)->text.strings_version > strings_version);
    AT(glyph_bounds->version > glyph_bounds_version);
    AT(glyph_texcoords->version > glyph_texcoords_version);
    AT(glyph_color->version > glyph_color_version);
    AT(glyph_angle->version > glyph_angle_version);
    AT(glyph_position->item_count == 72u);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check retained 3D scale-bar lowering from an explicit world reference.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_3d_world_reference(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.20f;
    camera_desc.fov_y = 0.74f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .reference_mode = DVZ_SCALEBAR_REFERENCE_WORLD_POINT,
            .reference_position = {0.0, 0.0, 0.0},
            .reference_direction = {1.0, 0.0, 0.0},
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 120.0f,
            .min_length_px = 70.0f,
            .max_length_px = 180.0f,
            .offset_px = {20.0f, 20.0f},
            .line_width_px = 2.0f,
            .line_color = {255, 255, 255, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 10.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
        });
    ANN(scalebar);

    _scene_prepare_text_visuals(figure);
    ANN(scalebar->scalebar_visual);
    ANN(scalebar->visual);
    AT(scalebar->scalebar_visual->visible);
    AT(scalebar->visual->visible);
    AT(strcmp(scalebar->text, "1 m") == 0);
    AC(scalebar->scalebar_units, 1.0, 1e-12);
    AT(scalebar->scalebar_px >= 70.0f);
    AT(scalebar->scalebar_px <= 180.0f);

    dvz_camera_set_view(
        camera, (vec3){0.0f, 0.0f, 6.40f}, (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 1.0f, 0.0f});
    _scene_prepare_text_visuals(figure);
    AT(strcmp(scalebar->text, "2 m") == 0);
    AC(scalebar->scalebar_units, 2.0, 1e-12);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check view-plane 3D scale bars ignore arcball model rotation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_3d_view_plane_rotation_invariant(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.20f;
    camera_desc.fov_y = 0.74f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .reference_mode = DVZ_SCALEBAR_REFERENCE_VIEW_PLANE,
            .reference_position = {0.0, 0.0, 0.0},
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 120.0f,
            .min_length_px = 70.0f,
            .max_length_px = 180.0f,
            .offset_px = {20.0f, 20.0f},
            .line_width_px = 2.0f,
            .line_color = {255, 255, 255, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 10.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
        });
    ANN(scalebar);

    _scene_prepare_text_visuals(figure);
    ANN(scalebar->scalebar_visual);
    ANN(scalebar->visual);
    AT(scalebar->scalebar_visual->visible);
    AT(scalebar->visual->visible);
    double initial_units = scalebar->scalebar_units;
    float initial_px = scalebar->scalebar_px;

    dvz_arcball_set(arcball, (vec3){+0.70f, -0.40f, +0.30f});
    _scene_prepare_text_visuals(figure);
    AC(scalebar->scalebar_units, initial_units, 1e-12);
    AC(scalebar->scalebar_px, initial_px, 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check view-plane 3D scale bars track arcball camera zoom.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_3d_view_plane_zoom_scale(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 400, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.20f;
    camera_desc.fov_y = 0.74f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .reference_mode = DVZ_SCALEBAR_REFERENCE_VIEW_PLANE,
            .reference_position = {0.0, 0.0, 0.0},
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 120.0f,
            .min_length_px = 70.0f,
            .max_length_px = 180.0f,
            .offset_px = {20.0f, 20.0f},
            .line_width_px = 2.0f,
            .line_color = {255, 255, 255, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 10.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
        });
    ANN(scalebar);

    _scene_prepare_text_visuals(figure);
    ANN(scalebar->scalebar_visual);
    ANN(scalebar->visual);
    AT(scalebar->scalebar_visual->visible);
    AT(scalebar->visual->visible);
    AT(scalebar->scalebar_realization.valid);
    double initial_units_per_px = scalebar->scalebar_realization.units_per_px;
    double initial_units = scalebar->scalebar_units;

    dvz_arcball_zoom(arcball, 2.0f);
    _scene_prepare_text_visuals(figure);
    AT(scalebar->scalebar_realization.valid);
    AT(scalebar->scalebar_realization.units_per_px < initial_units_per_px);
    AT(scalebar->scalebar_units <= initial_units);
    AT(scalebar->scalebar_px >= 70.0f);
    AT(scalebar->scalebar_px <= 180.0f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check render emission does not invalidate scale-bar glyph upload sources.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_scalebar_render_emit_keeps_upload_sources(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 720, 420, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.010) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 0.006) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 160.0f,
            .min_length_px = 90.0f,
            .max_length_px = 240.0f,
            .offset_px = {36.0f, 34.0f},
            .tick_length_px = 12.0f,
            .line_width_px = 3.0f,
            .line_color = {245, 248, 252, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 22.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {255, 236, 176, 255},
            },
        });
    ANN(scalebar);

    DvzFramePlan* plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    _scene_emit_visual_uploads(figure, plan, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(scalebar->visual);
    AT(_visual_family_state(scalebar->visual)->text.glyph_visual != NULL);
    DvzVisual* glyph = _visual_family_state(scalebar->visual)->text.glyph_visual;

    const DvzVisualAttr* position_attr = NULL;
    for (uint32_t ai = 0; ai < glyph->attr_count; ai++)
    {
        if (strcmp(glyph->attrs[ai].name, "position") == 0)
        {
            position_attr = &glyph->attrs[ai];
            break;
        }
    }
    ANN(position_attr);
    ANN(position_attr->data);

    uint32_t upload_index = UINT32_MAX;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (
            node->type == DVZ_FRAME_PLAN_NODE_UPLOAD &&
            node->u.upload.metadata.visual_type == (uint32_t)DVZ_VISUAL_TYPE_GLYPH &&
            node->u.upload.metadata.role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
        {
            upload_index = i;
            break;
        }
    }
    AT(upload_index != UINT32_MAX);
    AT(plan->nodes[upload_index].u.upload.data == position_attr->data);

    AT(_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    AT(plan->nodes[upload_index].u.upload.data == position_attr->data);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check the minimal no-data scale-bar stream contains both bar geometry and text work.
 *
 * @param suite the active test suite
 * @param item the active test case
 * @return 0 on success
 */
static int test_scene_scalebar_minimal_stream(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 720, 420, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.010) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 0.006) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 160.0f,
            .min_length_px = 90.0f,
            .max_length_px = 240.0f,
            .offset_px = {36.0f, 34.0f},
            .tick_length_px = 12.0f,
            .line_width_px = 3.0f,
            .line_color = {245, 248, 252, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 22.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {255, 236, 176, 255},
            },
        });
    ANN(scalebar);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 720;
    cfg.target_height = 420;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool saw_segment_pipeline = false;
    bool saw_segment_draw = false;
    bool saw_glyph_pipeline = false;
    bool glyph_pipeline_bound = false;
    bool saw_glyph_bind = false;
    bool saw_glyph_draw = false;
    uint64_t glyph_pass = 0;
    uint32_t segment_draw_index = UINT32_MAX;
    uint32_t glyph_draw_index = UINT32_MAX;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_segment") != NULL)
                saw_segment_pipeline = true;
            if (label != NULL && strstr(label, "_pipe_glyph") != NULL)
                saw_glyph_pipeline = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.set_pipeline.pipeline_id);
            if (label != NULL && strstr(label, "_pipe_glyph") != NULL)
            {
                glyph_pass = cmd->u.set_pipeline.pass_id;
                glyph_pipeline_bound = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            if (
                glyph_pipeline_bound && cmd->u.set_bind_group.pass_id == glyph_pass &&
                cmd->u.set_bind_group.slot == 1)
                saw_glyph_bind = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            if (!saw_segment_draw && cmd->u.draw_indexed.index_count >= 18)
            {
                saw_segment_draw = true;
                segment_draw_index = i;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            if (
                glyph_pipeline_bound && cmd->u.draw.pass_id == glyph_pass &&
                cmd->u.draw.vertex_count >= 6)
            {
                saw_glyph_draw = true;
                glyph_draw_index = i;
            }
        }
    }
    AT(strcmp(scalebar->text, "2 mm") == 0);
    AT(saw_segment_pipeline);
    AT(saw_segment_draw);
    AT(saw_glyph_pipeline);
    AT(saw_glyph_bind);
    AT(saw_glyph_draw);
    AT(segment_draw_index < glyph_draw_index);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check that a graph-backed scale-bar label does not drop a plain neighboring panel.
 *
 * @param suite the active test suite
 * @param item the active test case
 * @return 0 on success
 */
static int test_scene_scalebar_2d_3d_stream_order(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1100, 620, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
    DvzPanel* right = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.5f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
    ANN(left);
    ANN(right);
    dvz_panel_set_background_color(left, dvz_color_from_unit(0.04f, 0.05f, 0.06f, 1.0f));
    dvz_panel_set_background_color(right, dvz_color_from_unit(0.04f, 0.044f, 0.052f, 1.0f));

    DvzVisual* left_points = dvz_point(scene, 0);
    ANN(left_points);
    vec3 left_positions[2] = {{0.0f, 0.0f, 0.0f}, {0.8f, 0.7f, 0.0f}};
    DvzColor left_colors[2] = {{220, 120, 160, 255}, {120, 220, 190, 255}};
    float left_diameters[2] = {10.0f, 12.0f};
    DvzVisualDataUpdate left_updates[] = {
        {.attr_name = "position", .data = left_positions, .item_count = 2},
        {.attr_name = "color", .data = left_colors, .item_count = 2},
        {.attr_name = "diameter", .data = left_diameters, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(left_points, left_updates, 3) == 0);
    AT(dvz_panel_add_visual(left, left_points, NULL) == 0);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        left,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 125.0f,
            .min_length_px = 75.0f,
            .max_length_px = 185.0f,
            .offset_px = {26.0f, 24.0f},
            .tick_length_px = 9.0f,
            .line_width_px = 2.0f,
            .line_color = {245, 248, 252, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 18.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {255, 236, 176, 255},
            },
        });
    ANN(scalebar);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.20f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.74f;
    camera_desc.near_clip = 0.1f;
    camera_desc.far_clip = 100.0f;
    DvzCamera* camera = dvz_panel_set_camera(right, &camera_desc);
    ANN(camera);

    DvzAnnotation* right_scalebar = dvz_annotation_scalebar(
        right,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .reference_mode = DVZ_SCALEBAR_REFERENCE_VIEW_PLANE,
            .reference_position = {0.0, 0.0, 0.0},
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 125.0f,
            .min_length_px = 75.0f,
            .max_length_px = 185.0f,
            .offset_px = {28.0f, 24.0f},
            .tick_length_px = 9.0f,
            .line_width_px = 2.0f,
            .line_color = {235, 246, 255, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 18.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {178, 226, 255, 255},
            },
        });
    ANN(right_scalebar);

    DvzVisual* right_points = dvz_point(scene, 0);
    ANN(right_points);
    vec3 right_positions[3] = {
        {-0.5f, -0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.3f, -0.2f}};
    DvzColor right_colors[3] = {
        {120, 200, 255, 255}, {255, 140, 220, 255}, {120, 255, 180, 255}};
    float right_diameters[3] = {18.0f, 20.0f, 16.0f};
    DvzVisualDataUpdate right_updates[] = {
        {.attr_name = "position", .data = right_positions, .item_count = 3},
        {.attr_name = "color", .data = right_colors, .item_count = 3},
        {.attr_name = "diameter", .data = right_diameters, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(right_points, right_updates, 3) == 0);
    AT(dvz_panel_add_visual(right, right_points, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 1100;
    cfg.target_height = 620;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t right_pass = 0;
    uint64_t glyph_pass = 0;
    bool right_pipeline = false;
    bool glyph_pipeline = false;
    bool saw_right_viewport = false;
    bool saw_right_point_draw = false;
    bool saw_glyph_draw = false;
    uint32_t right_point_draw_index = UINT32_MAX;
    uint32_t glyph_draw_index = UINT32_MAX;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            if (fabsf(cmd->u.set_viewport.viewport[0] - 550.0f) < 1e-5f &&
                fabsf(cmd->u.set_viewport.viewport[2] - 550.0f) < 1e-5f)
            {
                saw_right_viewport = true;
                right_pass = cmd->u.set_viewport.pass_id;
                right_pipeline = false;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.set_pipeline.pipeline_id);
            if (label == NULL)
                continue;
            if (cmd->u.set_pipeline.pass_id == right_pass &&
                strstr(label, "_pipe_point") != NULL)
                right_pipeline = true;
            if (strstr(label, "_pipe_glyph") != NULL)
            {
                glyph_pass = cmd->u.set_pipeline.pass_id;
                glyph_pipeline = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            if (cmd->u.draw.pass_id == right_pass && right_pipeline &&
                cmd->u.draw.vertex_count == 3)
            {
                saw_right_point_draw = true;
                right_point_draw_index = i;
            }
            if (cmd->u.draw.pass_id == glyph_pass && glyph_pipeline &&
                cmd->u.draw.vertex_count >= 6)
            {
                saw_glyph_draw = true;
                glyph_draw_index = i;
            }
        }
    }
    AT(saw_right_viewport);
    AT(saw_right_point_draw);
    AT(saw_glyph_draw);
    AT(right_point_draw_index < glyph_draw_index);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check scene-level font defaults and retained text default sizing.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_font_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFontDefaults defaults = dvz_scene_font_defaults(scene);
    DvzFontDefaults built_in = dvz_font_defaults();
    DvzTextStyle default_style = dvz_text_style();
    AT(default_style.size_px == 0.0f);
    AT(strcmp(defaults.sans.family, built_in.sans.family) == 0);
    AT(defaults.text_size_px == built_in.text_size_px);

    DvzFontDefaults custom = built_in;
    custom.sans.family = "Scene Sans";
    custom.sans.style = "Book";
    custom.text_size_px = 19.0f;
    dvz_scene_set_font_defaults(scene, &custom);
    defaults = dvz_scene_font_defaults(scene);
    AT(strcmp(defaults.sans.family, "Scene Sans") == 0);
    AT(strcmp(defaults.sans.style, "Book") == 0);
    AT(defaults.text_size_px == 19.0f);

    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);
    DvzText* text = dvz_text(panel, 0);
    ANN(text);
    AT(text->style.size_px == 19.0f);
    AT(dvz_text_set_style(text, NULL) == 0);
    AT(text->style.size_px == 19.0f);
    default_style = dvz_text_style();
    AT(dvz_text_set_style(text, &default_style) == 0);
    AT(text->style.size_px == 19.0f);
    default_style.size_px = 21.0f;
    AT(dvz_text_set_style(text, &default_style) == 0);
    AT(text->style.size_px == 21.0f);

    dvz_scene_set_font_defaults(scene, NULL);
    defaults = dvz_scene_font_defaults(scene);
    AT(strcmp(defaults.sans.family, built_in.sans.family) == 0);
    AT(defaults.text_size_px == built_in.text_size_px);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check that SDF text realization uses scene font defaults for null-font styles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_sdf_default_font(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFontDefaults defaults = dvz_scene_font_defaults(scene);
    defaults.sans.family = "SDF Default";
    defaults.sans.style = "Book";
    defaults.text_size_px = 18.0f;
    dvz_scene_set_font_defaults(scene, &defaults);

    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"A"};
    vec3 positions[1] = {{32.0f, 48.0f, 0.0f}};
    DvzVisualDataUpdate updates[1] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 1) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(scene->font_count == 1);
    AT(strcmp(scene->fonts[0].family, "SDF Default") == 0);
    AT(strcmp(scene->fonts[0].style, "Book") == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_text_semantic_object_realization(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});

    DvzText* text = dvz_text(panel, 0);
    ANN(text);
    AT(scene->text_count == 1);
    AT(text->style.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);
    AT(text->dirty_flags == DVZ_TEXT_DIRTY_ALL);

    AT(dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_AUTO) == 0);
    AT(text->style.renderer == DVZ_TEXT_RENDERER_AUTO);
    AT(dvz_text_set_style(
           text,
           &(DvzTextStyle){DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
               .size_px = 16.0f,
               .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
               .color = {180, 220, 255, 255},
           }) == 0);
    dvz_text_set_placement(
        text,
        &(DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
            .mode = DVZ_TEXT_PLACEMENT_SCREEN,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
            .position = {10.0, 20.0, 0.0},
            .text_anchor = {0.0f, 0.0f},
            .has_text_anchor = true,
        });
    dvz_text_set_string(text, "ABC");
    AT(strcmp(text->string, "ABC") == 0);

    _scene_prepare_text_visuals(figure);
    ANN(text->visual);
    AT(text->visual->visible);
    AT(text->dirty_flags == DVZ_TEXT_DIRTY_NONE);
    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(text->visual, "position", &position_view) == 0);
    const float* positions = position_view.data;
    ANN(positions);
    AC(positions[0], -0.96875f, 1e-6f);
    AC(positions[1], 0.9166667f, 1e-6f);

    dvz_text_set_placement(
        text,
        &(DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
            .mode = DVZ_TEXT_PLACEMENT_DATA,
            .position = {0.25, -0.5, 0.1},
            .offset = {12.0f, -6.0f},
            .text_anchor = {0.0f, 0.0f},
            .has_text_anchor = true,
            .depth_test = true,
        });
    _scene_prepare_text_visuals(figure);
    AT(dvz_visual_data(text->visual, "position", &position_view) == 0);
    positions = position_view.data;
    ANN(positions);
    AC(positions[0], 0.25f, 1e-6f);
    AC(positions[1], -0.5f, 1e-6f);
    DvzVisualDataView bounds_view = {0};
    AT(dvz_visual_data(text->visual, "bounds", &bounds_view) == 0);
    const float* bounds = bounds_view.data;
    ANN(bounds);
    AC(bounds[0], 12.0f, 1e-6f);
    AC(bounds[1], -6.0f, 1e-6f);
    AT(text->visual->depth_test_enabled);
    bool found_data_attach = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual == text->visual)
        {
            found_data_attach = true;
            AT(panel->visuals[i].controller_mode == DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL);
            AT(panel->visuals[i].coord_space == DVZ_COORD_DATA);
        }
    }
    AT(found_data_attach);

    dvz_figure_resize(figure, 800, 600);
    _scene_prepare_text_visuals(figure);
    AT(dvz_visual_data(text->visual, "position", &position_view) == 0);
    positions = position_view.data;
    ANN(positions);
    AC(positions[0], 0.25f, 1e-6f);
    AC(positions[1], -0.5f, 1e-6f);
    AT(dvz_visual_data(text->visual, "bounds", &bounds_view) == 0);
    bounds = bounds_view.data;
    ANN(bounds);
    AC(bounds[0], 12.0f, 1e-6f);
    AC(bounds[1], -6.0f, 1e-6f);

    DvzFramePlan* plan = dvz_frame_plan("text.data_clip_rect", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    bool found_glyph_plot_clip = false;
    for (uint32_t node_idx = 0; node_idx < dvz_frame_plan_node_count(plan); node_idx++)
    {
        const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, node_idx);
        ANN(render);
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        for (uint32_t i = 0; i < render->u.render.visual_count; i++)
        {
            if (render->u.render.visual_metadata[i].visual_type == DVZ_VISUAL_TYPE_GLYPH)
            {
                found_glyph_plot_clip = true;
                AT(render->u.render.visual_metadata[i].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PLOT);
            }
        }
    }
    AT(found_glyph_plot_clip);
    dvz_frame_plan_destroy(plan);

    dvz_text_destroy(text);
    AT(text->scene == NULL);
    AT(!text->visual->visible);
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

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    const char* strings[1] = {"Hi"};
    vec3 positions_text[1] = {{10.0f, 20.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    AT(panel->visual_count == 1);

    _scene_prepare_text_visuals(figure);
    DvzVisual* glyph = _visual_family_state(text)->text.glyph_visual;
    ANN(glyph);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].visual == glyph);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->visible);
    AT(glyph->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(!glyph->depth_test_enabled);
    AT(_visual_family_state(glyph)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA);
    AT(_visual_family_state(text)->text.realized_version > 0);
    AT(_visual_family_state(text)->text.span_count == 1);
    AT(_visual_family_state(text)->text.spans[0].glyph_count == 2);

    int pos_idx = _attr_index(glyph, "position");
    int bounds_idx = _attr_index(glyph, "bounds");
    int uv_idx = _attr_index(glyph, "texcoords");
    int color_idx = _attr_index(glyph, "color");
    int angle_idx = _attr_index(glyph, "angle");
    AT(pos_idx >= 0);
    AT(bounds_idx >= 0);
    AT(uv_idx >= 0);
    AT(color_idx >= 0);
    AT(angle_idx >= 0);
    AT(glyph->attrs[pos_idx].item_count == 12);
    AT(glyph->attrs[bounds_idx].item_count == 12);
    AT(glyph->attrs[uv_idx].item_count == 12);
    AT(glyph->attrs[color_idx].item_count == 12);
    AT(glyph->attrs[angle_idx].item_count == 12);
    const vec3* positions = (const vec3*)glyph->attrs[pos_idx].data;
    const vec4* bounds = (const vec4*)glyph->attrs[bounds_idx].data;
    ANN(positions);
    ANN(bounds);
    AC(positions[0][0], -0.96875f, 1e-6f);
    AC(positions[0][1], 0.9166667f, 1e-6f);
    AC(bounds[0][0], 0.0f, 1e-6f);
    AC(bounds[0][1], 0.0f, 1e-6f);
    AC(bounds[11][2], 12.0f, 1e-6f);
    AC(bounds[11][3], 8.0f, 1e-6f);

    text_anchors[0][0] = 0.5f;
    text_anchors[0][1] = 0.5f;
    AT(dvz_visual_set_data(text, "anchor", text_anchors, 1) == 0);
    _scene_prepare_text_visuals(figure);
    positions = (const vec3*)glyph->attrs[pos_idx].data;
    bounds = (const vec4*)glyph->attrs[bounds_idx].data;
    ANN(positions);
    ANN(bounds);
    AC(positions[0][0], -0.96875f, 1e-6f);
    AC(positions[0][1], 0.9166667f, 1e-6f);
    AC(bounds[0][0], -6.0f, 1e-6f);
    AC(bounds[0][1], -4.0f, 1e-6f);

    const uint8_t* colors = (const uint8_t*)glyph->attrs[color_idx].data;
    ANN(colors);
    AT(colors[0] == 64);
    AT(colors[1] == 128);
    AT(colors[2] == 255);
    AT(colors[3] == 255);

    AT(_visual_family_state(glyph)->field != NULL);
    DvzSampledField* atlas = _visual_family_state(glyph)->field;
    AT(atlas->desc.width == 128);
    AT(atlas->desc.height == 60);
    AT(_visual_family_state(glyph)->field->dirty);

    uint64_t old_visual_version = _visual_family_state(text)->text.realized_version;
    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(text)->text.realized_version == old_visual_version);
    AT(panel->visual_count == 2);

    strings[0] = "A" "\xCE" "\xA9" "B";
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    _scene_prepare_text_visuals(figure);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->attrs[pos_idx].item_count == 18);
    AT(glyph->attrs[bounds_idx].item_count == 18);
    AT(glyph->attrs[uv_idx].item_count == 18);
    AT(glyph->attrs[color_idx].item_count == 18);
    AT(glyph->attrs[angle_idx].item_count == 18);
    AT(_visual_family_state(glyph)->field == atlas);
    AT(_visual_family_state(text)->text.spans[0].glyph_count == 3);

    strings[0] = "A";
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    _scene_prepare_text_visuals(figure);
    AT(glyph->attrs[pos_idx].item_count == 6);
    AT(glyph->attrs[bounds_idx].item_count == 6);
    AT(glyph->attrs[uv_idx].item_count == 6);
    AT(glyph->attrs[color_idx].item_count == 6);
    AT(glyph->attrs[angle_idx].item_count == 6);
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
    positions = (const vec3*)glyph->attrs[pos_idx].data;
    ANN(positions);
    AC(positions[0][0], -0.9375f, 1e-6f);
    AC(positions[0][1], 0.8333333f, 1e-6f);
    AT(_visual_family_state(text)->text.visual_figure_width == 320);
    AT(_visual_family_state(text)->text.visual_figure_height == 240);

    dvz_visual_set_visible(text, false);
    _scene_prepare_text_visuals(figure);
    AT(!glyph->visible);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
            .text = "A",
            .style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 8.0f,
                .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
                .color = {255, 255, 255, 255},
            },
            .placement = {DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
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
    AT(_visual_family_state(annotation->visual)->field == atlas);
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

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"S D"};
    vec3 positions[1] = {{32.0f, 48.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    DvzVisual* glyph = _visual_family_state(text)->text.glyph_visual;
    ANN(glyph);
    AT(glyph->type == DVZ_VISUAL_TYPE_GLYPH);
    AT(glyph->visible);
    AT(_visual_family_state(glyph)->field != NULL);
    AT(_visual_family_state(glyph)->field->desc.width > 128);
    AT(_visual_family_state(glyph)->field->desc.height > 60);
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    AT(_visual_family_state(glyph)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#else
    AT(_visual_family_state(glyph)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA ||
       _visual_family_state(glyph)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#endif
    AT(scene->font_count == 1);
    DvzTextAtlas* font_atlas = text_test_atlas(scene, DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    ANN(font_atlas);
    AT(font_atlas->field == _visual_family_state(glyph)->field);
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
    AT(_visual_family_state(text)->text.span_count == 1);
    AT(_visual_family_state(text)->text.spans[0].glyph_count == 2);

    int pos_idx = _attr_index(glyph, "position");
    int bounds_idx = _attr_index(glyph, "bounds");
    int uv_idx = _attr_index(glyph, "texcoords");
    int color_idx = _attr_index(glyph, "color");
    AT(pos_idx >= 0);
    AT(bounds_idx >= 0);
    AT(uv_idx >= 0);
    AT(color_idx >= 0);
    AT(glyph->attrs[pos_idx].item_count == 12);
    AT(glyph->attrs[bounds_idx].item_count == 12);
    AT(glyph->attrs[uv_idx].item_count == 12);
    AT(glyph->attrs[color_idx].item_count == 12);
    const vec4* glyph_bounds = (const vec4*)glyph->attrs[bounds_idx].data;
    ANN(glyph_bounds);
    float first_max_x = glyph_bounds[0][2];
    float second_min_x = glyph_bounds[6][0];
    for (uint32_t k = 1; k < 6; k++)
        if (glyph_bounds[k][2] > first_max_x)
            first_max_x = glyph_bounds[k][2];
    for (uint32_t k = 7; k < 12; k++)
        if (glyph_bounds[k][0] < second_min_x)
            second_min_x = glyph_bounds[k][0];
    AT(second_min_x > first_max_x + 0.5f);

    DvzAnnotation* annotation = dvz_annotation_label(
        panel,
        &(DvzLabelDesc){DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
            .text = "A",
            .style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 14.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            },
            .placement = {DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
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
    AT(_visual_family_state(annotation->visual)->field == _visual_family_state(glyph)->field);
    AT(_visual_family_state(annotation->visual)->glyph_atlas_encoding == _visual_family_state(glyph)->glyph_atlas_encoding);
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

    DvzVisual* small_text = _scene_text_visual(scene, 0);
    ANN(small_text);
    AT(_scene_text_visual_set_renderer(small_text, DVZ_TEXT_RENDERER_AUTO) == 0);
    const char* small_string[1] = {"small"};
    vec3 small_pos[1] = {{24.0f, 24.0f, 0.0f}};
    float small_size[1] = {10.0f};
    DvzVisualDataUpdate small_updates[2] = {
        {.attr_name = "position", .data = small_pos, .item_count = 1},
        {.attr_name = "size", .data = small_size, .item_count = 1},
    };
    AT(dvz_visual_set_strings(small_text, "text", small_string, 1) == 0);
    AT(dvz_visual_set_data_many(small_text, small_updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, small_text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzVisual* large = _scene_text_visual(scene, 0);
    ANN(large);
    AT(_scene_text_visual_set_renderer(large, DVZ_TEXT_RENDERER_AUTO) == 0);
    const char* large_string[1] = {"large"};
    vec3 large_pos[1] = {{24.0f, 64.0f, 0.0f}};
    float large_size[1] = {24.0f};
    DvzVisualDataUpdate large_updates[2] = {
        {.attr_name = "position", .data = large_pos, .item_count = 1},
        {.attr_name = "size", .data = large_size, .item_count = 1},
    };
    AT(dvz_visual_set_strings(large, "text", large_string, 1) == 0);
    AT(dvz_visual_set_data_many(large, large_updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, large,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(small_text)->text.glyph_visual != NULL);
    AT(_visual_family_state(large)->text.glyph_visual != NULL);
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    AT(_visual_family_state(_visual_family_state(small_text)->text.glyph_visual)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA);
    AT(scene->font_count >= 1);
    DvzTextAtlas* small_atlas =
        text_test_atlas(scene, DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP, small_size[0]);
    ANN(small_atlas);
    AT(small_atlas->field == _visual_family_state(_visual_family_state(small_text)->text.glyph_visual)->field);
#else
    AT(_visual_family_state(_visual_family_state(small_text)->text.glyph_visual)->field == scene->text_bitmap_atlas);
#endif
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    AT(_visual_family_state(_visual_family_state(large)->text.glyph_visual)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
#else
    AT(_visual_family_state(_visual_family_state(large)->text.glyph_visual)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA);
#endif

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify MSDF atlas specs preserve the distance-range/em ratio across atlas sizes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_msdf_atlas_spec_scales_range(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzTextAtlasSpec small_spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 32.0f);
    DvzTextAtlasSpec medium = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 72.0f);
    DvzTextAtlasSpec large = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 128.0f);

    AC(small_spec.em_px, 32.0f, 1e-6f);
    AC(small_spec.distance_range_px, 4.0f, 1e-6f);
    AC(medium.em_px, 64.0f, 1e-6f);
    AC(medium.distance_range_px, 8.0f, 1e-6f);
    AC(large.em_px, 128.0f, 1e-6f);
    AC(large.distance_range_px, 16.0f, 1e-6f);
    AC(small_spec.distance_range_px / small_spec.em_px, medium.distance_range_px / medium.em_px, 1e-6f);
    AC(medium.distance_range_px / medium.em_px, large.distance_range_px / large.em_px, 1e-6f);
    return 0;
}



/**
 * Verify font-backed text atlases grow to cover requested UTF-8 glyphs.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_font_atlas_expands_for_utf8(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* ascii = _scene_text_visual(scene, 0);
    DvzVisual* utf8 = _scene_text_visual(scene, 0);
    ANN(ascii);
    ANN(utf8);
    AT(_scene_text_visual_set_renderer(ascii, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    AT(_scene_text_visual_set_renderer(utf8, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);

    const char* ascii_strings[1] = {"ASCII"};
    const char* utf8_strings[1] = {"caf\xC3\xA9"};
    vec3 ascii_pos[1] = {{24.0f, 32.0f, 0.0f}};
    vec3 utf8_pos[1] = {{24.0f, 72.0f, 0.0f}};
    float sizes[1] = {24.0f};
    DvzVisualDataUpdate ascii_updates[2] = {
        {.attr_name = "position", .data = ascii_pos, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
    };
    DvzVisualDataUpdate utf8_updates[2] = {
        {.attr_name = "position", .data = utf8_pos, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
    };
    AT(dvz_visual_set_strings(ascii, "text", ascii_strings, 1) == 0);
    AT(dvz_visual_set_strings(utf8, "text", utf8_strings, 1) == 0);
    AT(dvz_visual_set_data_many(ascii, ascii_updates, 2) == 0);
    AT(dvz_visual_set_data_many(utf8, utf8_updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, ascii,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(scene->font_count == 1);
    DvzTextAtlas* initial_atlas =
        text_test_atlas(scene, DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    ANN(initial_atlas);
    DvzSampledField* initial_field = initial_atlas->field;
    ANN(initial_field);
    uint32_t initial_glyph_count = initial_atlas->glyph_count;
    uint32_t initial_width = initial_atlas->width;
    uint32_t initial_height = initial_atlas->height;
    uint64_t initial_generation = initial_atlas->generation;
    initial_field->dirty = false;
    initial_field->dirty_full = false;
    dvz_memset(
        &initial_field->dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));

    AT(dvz_panel_add_visual(
           panel, utf8,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    DvzTextAtlas* atlas = text_test_atlas(scene, DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    ANN(atlas);
    ANN(atlas->field);
    AT(atlas->glyph_count > initial_glyph_count);
    AT(atlas->generation > initial_generation);
    AT(atlas->field == initial_field);
    AT(atlas->width >= initial_width);
    AT(atlas->height >= initial_height);
    if (initial_field->dirty && atlas->width == initial_width && atlas->height == initial_height)
    {
        AT(initial_field->dirty_region.width > 0);
        AT(initial_field->dirty_region.height > 0);
        AT(initial_field->dirty_region.width <= atlas->width);
        AT(initial_field->dirty_region.height <= atlas->height);
    }
    else if (initial_field->dirty)
    {
        AT(initial_field->dirty_full);
        AT(atlas->width > initial_width || atlas->height > initial_height);
    }
    DvzTextAtlasGlyph* utf8_glyph = _scene_text_atlas_glyph(atlas, 0x00E9u);
    ANN(utf8_glyph);
    AT(_visual_family_state(ascii)->text.glyph_visual != NULL);
    AT(_visual_family_state(utf8)->text.glyph_visual != NULL);
    AT(_visual_family_state(_visual_family_state(ascii)->text.glyph_visual)->field == atlas->field);
    AT(_visual_family_state(_visual_family_state(utf8)->text.glyph_visual)->field == atlas->field);
    AT(_visual_family_state(_visual_family_state(ascii)->text.glyph_visual)->field == _visual_family_state(_visual_family_state(utf8)->text.glyph_visual)->field);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify unavailable glyphs are counted and resolve to the explicit fallback glyph.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_font_atlas_missing_glyph_fallback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"missing \xF4\x8F\xBF\xBF"};
    vec3 positions[1] = {{24.0f, 32.0f, 0.0f}};
    float sizes[1] = {24.0f};
    DvzVisualDataUpdate updates[2] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(scene->font_count == 1);
    DvzTextAtlas* atlas = text_test_atlas(scene, DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    ANN(atlas);
    DvzTextAtlasGlyph* fallback = _scene_text_atlas_glyph(atlas, '?');
    DvzTextAtlasGlyph* missing = _scene_text_atlas_glyph(atlas, 0x10FFFFu);
    ANN(fallback);
    ANN(missing);
    AT(missing == fallback);
    AT(atlas->missing_glyph_count > 0);
    AT(_visual_family_state(text)->text.glyph_visual != NULL);
    AT(_visual_family_state(text)->text.glyph_visual->attrs[0].item_count > 0);

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
    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    char labels[16][16] = {{0}};
    const char* strings[16] = {0};
    vec3 positions[16] = {{0}};
    vec2 text_anchors[16] = {{0}};
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
        colors[i] = dvz_color_rgb(255, 255, 255);
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_APPLY}) == 0);
    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(text)->text.glyph_visual != NULL);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].visual == _visual_family_state(text)->text.glyph_visual);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

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
 * Verify controller-applied text keeps generated glyph anchors in visual coordinates.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_panzoom_glyph_anchor_coordinates(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    const char* strings[1] = {"panzoom"};
    vec3 positions[1] = {{0.25f, -0.125f, 0.25f}};
    float sizes[1] = {24.0f};
    DvzVisualDataUpdate updates[2] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_APPLY}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(text)->text.glyph_visual != NULL);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].visual == _visual_family_state(text)->text.glyph_visual);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(_visual_family_state(text)->text.glyph_visual, "position", &position_view) == 0);
    AT(position_view.item_count > 0);
    const float* glyph_positions = (const float*)position_view.data;
    ANN(glyph_positions);
    AC(glyph_positions[0], positions[0][0], 1e-6f);
    AC(glyph_positions[1], positions[0][1], 1e-6f);
    AC(glyph_positions[2], positions[0][2], 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify text realization regenerates anchors when attachment controller mode changes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_attach_mode_change_regenerates_glyphs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    const char* strings[1] = {"mode"};
    vec3 positions[1] = {{320.0f, 240.0f, 0.0f}};
    float sizes[1] = {24.0f};
    DvzVisualDataUpdate updates[2] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 2) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(text)->text.glyph_visual != NULL);
    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(_visual_family_state(text)->text.glyph_visual, "position", &position_view) == 0);
    const float* glyph_positions = (const float*)position_view.data;
    ANN(glyph_positions);
    AC(glyph_positions[0], 0.0f, 1e-6f);
    AC(glyph_positions[1], 0.0f, 1e-6f);

    panel->visuals[0].controller_mode = DVZ_CONTROLLER_APPLY;
    _scene_prepare_text_visuals(figure);
    AT(panel->visuals[1].visual == _visual_family_state(text)->text.glyph_visual);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);
    AT(dvz_visual_data(_visual_family_state(text)->text.glyph_visual, "position", &position_view) == 0);
    glyph_positions = (const float*)position_view.data;
    ANN(glyph_positions);
    AC(glyph_positions[0], positions[0][0], 1e-6f);
    AC(glyph_positions[1], positions[0][1], 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify private text-block markup parsing and source/text run bookkeeping.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_block_parse_markup(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzTextBlock block = {0};
    _scene_text_block_init(&block, "a <b>bold <i>both</i></b> &lt;x&gt; &amp;");
    AT(_scene_text_block_parse(&block) == 0);
    AT(block.valid);
    AT(block.diagnostic[0] == '\0');
    AT(strcmp(block.text, "a bold both <x> &") == 0);
    AT(block.run_count == 4);
    AT(block.runs[0].style_flags == DVZ_TEXT_BLOCK_STYLE_NONE);
    AT(block.runs[1].style_flags == DVZ_TEXT_BLOCK_STYLE_BOLD);
    AT(block.runs[2].style_flags == (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC));
    AT(block.runs[3].style_flags == DVZ_TEXT_BLOCK_STYLE_NONE);
    AT(block.runs[2].source_start < block.runs[2].source_end);
    AT(block.runs[2].text_start < block.runs[2].text_end);

    _scene_text_block_init(&block, "x </b> y");
    AT(_scene_text_block_parse(&block) == 0);
    AT(block.valid);
    AT(block.diagnostic[0] != '\0');
    AT(strcmp(block.text, "x </b> y") == 0);

    _scene_text_block_init(
        &block, "<u>under</u> <color=#2A80E6><b>blue</b></color>");
    AT(_scene_text_block_parse(&block) == 0);
    AT(block.valid);
    AT(block.diagnostic[0] == '\0');
    AT(strcmp(block.text, "under blue") == 0);
    AT(block.run_count == 3);
    AT((block.runs[0].style_flags & DVZ_TEXT_BLOCK_STYLE_UNDERLINE) != 0);
    AT(block.runs[2].has_color);
    AT(block.runs[2].color.r == 42);
    AT(block.runs[2].color.g == 128);
    AT(block.runs[2].color.b == 230);
    AT((block.runs[2].style_flags & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0);

    return 0;
}


/**
 * Verify private text-block measurement uses fixed-advance wrapping metadata.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_block_measure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzTextBlock block = {0};
    _scene_text_block_init(&block, "abcdefghi");
    AT(_scene_text_block_parse(&block) == 0);
    AT(_scene_text_block_measure(
           &block,
           &(DvzTextBlockLayout){
               .max_width_px = 28.0f,
               .char_width_px = 7.0f,
               .line_height_px = 10.0f,
               .padding_px = {0.0f, 2.0f},
           }) == 0);
    AC(block.metrics.advance[0], 28.0f, 1e-6f);
    AC(block.metrics.advance[1], 34.0f, 1e-6f);
    AC(block.metrics.line_height, 10.0f, 1e-6f);
    AC(block.metrics.baseline, 12.0f, 1e-6f);

#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzTextBlock narrow = {0};
    DvzTextBlock wide = {0};
    _scene_text_block_init(&narrow, "iiii");
    _scene_text_block_init(&wide, "WWWW");
    AT(_scene_text_block_parse(&narrow) == 0);
    AT(_scene_text_block_parse(&wide) == 0);
    DvzTextBlockLayout ft_layout = {
        .scene = scene,
        .font_size_px = 16.0f,
        .line_height_px = 21.0f,
    };
    AT(_scene_text_block_measure(&narrow, &ft_layout) == 0);
    AT(_scene_text_block_measure(&wide, &ft_layout) == 0);
    AT(narrow.layout_glyph_count == 4);
    AT(wide.layout_glyph_count == 4);
    AT(narrow.metrics.advance[0] < wide.metrics.advance[0]);

    DvzTextBlock bold = {0};
    _scene_text_block_init(&bold, "<b>bold</b>");
    AT(_scene_text_block_parse(&bold) == 0);
    AT(_scene_text_block_measure(&bold, &ft_layout) == 0);
    AT(bold.layout_fonts[DVZ_TEXT_BLOCK_FACE_BOLD] != NULL);
    AT((bold.layout_style_flags[0] & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0);

    DvzTextBlock italic = {0};
    _scene_text_block_init(&italic, "<i>italic</i>");
    AT(_scene_text_block_parse(&italic) == 0);
    AT(_scene_text_block_measure(&italic, &ft_layout) == 0);
    if (italic.layout_fonts[DVZ_TEXT_BLOCK_FACE_ITALIC] != NULL)
    {
        AT((italic.layout_style_flags[0] & DVZ_TEXT_BLOCK_STYLE_ITALIC) != 0);
    }
    else
    {
        AT((italic.layout_style_flags[0] & DVZ_TEXT_BLOCK_STYLE_ITALIC) == 0);
        AT(italic.diagnostic[0] != '\0');
    }

    _scene_text_block_destroy(&narrow);
    _scene_text_block_destroy(&wide);
    _scene_text_block_destroy(&bold);
    _scene_text_block_destroy(&italic);
    dvz_scene_destroy(scene);
#endif

    return 0;
}


/**
 * Verify private text-block rasterization produces owned RGBA8 pixels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_block_rasterize(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzTextBlock block = {0};
    _scene_text_block_init(&block, "<b>Hi</b> <i>x</i>");
    AT(_scene_text_block_parse(&block) == 0);
    AT(_scene_text_block_measure(
           &block,
           &(DvzTextBlockLayout){
               .max_width_px = 48.0f,
               .char_width_px = 7.0f,
               .line_height_px = 10.0f,
               .padding_px = {2.0f, 2.0f},
           }) == 0);
    AT(_scene_text_block_rasterize(
           &block,
           &(DvzTextBlockRasterDesc){
               .text_color = {10, 240, 40, 255},
               .background_color = {0, 0, 0, 0},
           }) == 0);
    AT(block.rgba != NULL);
    AT(block.raster_width == 32);
    AT(block.raster_height == 14);
    AT(block.rgba_size == (uint64_t)block.raster_width * block.raster_height * 4u);
    AT(block.raster_version == 1);

    uint32_t opaque = 0;
    uint32_t green = 0;
    for (uint64_t i = 0; i < block.rgba_size / 4u; i++)
    {
        const uint8_t* px = &block.rgba[4u * i];
        if (px[3] > 0)
            opaque++;
        if (px[0] == 10 && px[1] == 240 && px[2] == 40 && px[3] == 255)
            green++;
    }
    AT(opaque > 0);
    AT(green == opaque);

    uint8_t* first_raster = block.rgba;
    AT(_scene_text_block_rasterize(&block, NULL) == 0);
    AT(block.rgba == first_raster);
    AT(block.raster_version == 2);

    AT(_scene_text_block_rasterize(
           &block,
           &(DvzTextBlockRasterDesc){
               .text_color = {255, 255, 255, 255},
               .background_color = {0, 0, 0, 0},
               .scale = 2.0f,
           }) == 0);
    AT(block.raster_width == 64);
    AT(block.raster_height == 28);
    AC(block.raster_scale, 2.0f, 1e-6f);
    AT(block.raster_version == 3);
    _scene_text_block_destroy(&block);
    AT(block.rgba == NULL);
    AT(block.rgba_size == 0);
    return 0;
}


/**
 * Verify private text-block image lowering creates a retained sampled-image visual.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_block_image_lowering(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzTextBlock block = {0};
    _scene_text_block_init(&block, "Rich <b>card</b>");
    AT(_scene_text_block_parse(&block) == 0);
    AT(_scene_text_block_measure(
           &block,
           &(DvzTextBlockLayout){
               .scene = scene,
               .font_size_px = 11.0f,
               .line_height_px = 14.0f,
               .padding_px = {2.0f, 2.0f},
           }) == 0);
    AT(_scene_text_block_rasterize(
           &block,
           &(DvzTextBlockRasterDesc){
               .scene = scene,
               .text_color = {255, 255, 255, 255},
               .background_color = {0, 0, 0, 0},
           }) == 0);
    AT(_scene_text_block_realize_image(
           &block, panel,
           &(DvzTextBlockImageDesc){
               .position_px = {8.0f, 12.0f, 0.0f},
               .extent_px = {48.0f, 20.0f},
               .anchor = {-1.0f, -1.0f},
               .pixel_space = true,
               .z_layer = 3,
               .controller_mode = DVZ_CONTROLLER_FIXED,
           }) == 0);

    ANN(block.image_visual);
    ANN(block.image_field);
    AT(block.image_visual->visible);
    AT(_visual_family_state(block.image_visual)->field == block.image_field);
    AT(block.image_field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM);
    AT(block.image_field->desc.width == block.raster_width);
    AT(block.image_field->desc.height == block.raster_height);
    AT(panel->visual_count == 1);
    AT(panel->visuals[0].visual == block.image_visual);
    AT(panel->visuals[0].z_layer == 3);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    const DvzVisualAttr* position_px = _interaction_visual_attr(block.image_visual, "position_px");
    const DvzVisualAttr* extent_px = _interaction_visual_attr(block.image_visual, "extent_px");
    ANN(position_px);
    ANN(extent_px);
    AT(position_px->item_count == 1);
    AT(extent_px->item_count == 1);

    _scene_text_block_destroy(&block);
    AT(block.image_visual == NULL);
    AT(block.image_field == NULL);
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
    TST_CASE(test_scene_interaction_descriptor_abi_rejects_invalid_structs);
    TST_CASE(test_scene_text_annotation_descriptor_abi_rejects_invalid_structs);
    TST_CASE(test_scene_guide_descriptor_abi_rejects_invalid_structs);
    TST_CASE(test_scene_bars_descriptor_and_data_validation);
    TST_CASE(test_scene_band_descriptor_and_data_validation);
    TST_CASE(test_scene_overlay_descriptor_abi_rejects_invalid_structs);
    TST_CASE(test_scene_item_interaction_defaults_and_lifetime);
    TST_CASE(test_scene_item_interaction_input_queries);
    TST_CASE(test_scene_item_interaction_applies_results);
    TST_CASE(test_scene_selection_apply_query_and_link_keys);
    TST_CASE(test_scene_selection_apply_query_updates_item_state);
    TST_CASE(test_scene_pixel_hover_selection_item_state);
    TST_CASE(test_scene_sphere_hover_selection_item_state);
    TST_CASE(test_scene_mesh_instance_hover_selection_item_state);
    TST_CASE(test_scene_selection_card_realizes_query_metadata);
    TST_CASE(test_scene_overlay_card_public_api);
    TST_CASE(test_scene_overlay_card_rich_text_public_api);
    TST_CASE(test_scene_text_annotation_bookkeeping);
    TST_CASE(test_scene_guide_line_and_span_prepare_visuals);
    TST_CASE(test_scene_bars_prepare_visuals);
    TST_CASE(test_scene_band_prepare_visuals);
    TST_CASE(test_scene_scalebar_formatting);
    TST_CASE(test_scene_units_formatting_core);
    TST_CASE(test_scene_scalebar_2d_realization);
    TST_CASE(test_scene_scalebar_duration_units);
    TST_CASE(test_scene_scalebar_update_churn);
    TST_CASE(test_scene_scalebar_3d_world_reference);
    TST_CASE(test_scene_scalebar_3d_view_plane_rotation_invariant);
    TST_CASE(test_scene_scalebar_3d_view_plane_zoom_scale);
    TST_CASE(test_scene_scalebar_render_emit_keeps_upload_sources);
    TST_CASE(test_scene_scalebar_minimal_stream);
    TST_CASE(test_scene_scalebar_2d_3d_stream_order);
    TST_CASE(test_scene_font_defaults);
    TST_CASE(test_scene_text_sdf_default_font);
    TST_CASE(test_scene_text_semantic_object_realization);
    TST_CASE(test_scene_text_bitmap_visual_realization);
    TST_CASE(test_scene_text_sdf_visual_realization);
    TST_CASE(test_scene_text_auto_renderer_selection);
    TST_CASE(test_scene_text_msdf_atlas_spec_scales_range);
    TST_CASE(test_scene_text_font_atlas_expands_for_utf8);
    TST_CASE(test_scene_text_font_atlas_missing_glyph_fallback);
    TST_CASE(test_scene_text_many_labels_render_plan);
    TST_CASE(test_scene_text_panzoom_glyph_anchor_coordinates);
    TST_CASE(test_scene_text_attach_mode_change_regenerates_glyphs);
    TST_CASE(test_scene_text_block_parse_markup);
    TST_CASE(test_scene_text_block_measure);
    TST_CASE(test_scene_text_block_rasterize);
    TST_CASE(test_scene_text_block_image_lowering);

    return 0;
}
