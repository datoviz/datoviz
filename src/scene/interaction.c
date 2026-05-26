/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene interaction / selection / readout                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "datoviz/scene/overlay.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static bool _selection_matches_pick(
    const DvzSelection* selection, const DvzPickResult* pick, DvzSelectionItem* out_item);

static bool _selection_item_equals(const DvzSelectionItem* a, const DvzSelectionItem* b);

static bool _scene_remove_selection_item(DvzSelection* selection, const DvzSelectionItem* item);

static void _selection_clear_items(DvzSelection* selection);

static bool _selection_visual_supports_mask(const DvzVisual* visual);

static DvzVisual* _selection_visual_from_public_id(DvzScene* scene, uint64_t visual_id);

static uint32_t _selection_visual_item_count(const DvzVisual* visual);

static bool _selection_matches_visual_item(
    const DvzSelection* selection, const DvzVisual* visual, uint64_t visual_id,
    uint32_t item_index);

static int _selection_sync_visual_mask(DvzSelection* selection, DvzVisual* visual);

static int _selection_sync_masks(DvzSelection* selection);

static DvzPanel* _selection_card_panel_from_pick(DvzSelection* selection, const DvzPickResult* pick);

static void _selection_card_refresh_text(DvzSelection* selection, const DvzPickResult* pick);

static void _selection_card_hide(DvzSelection* selection);

static void _selection_card_update_from_pick(DvzSelection* selection, const DvzPickResult* pick);

static bool _selection_card_realize(DvzFigure* figure, DvzSelection* selection);

static void _readout_format_value(
    const DvzSceneFormatState* format, double value, char* out, uint32_t out_size);

static void _readout_refresh_text(DvzPinnedReadout* readout);

static void _scene_card_init(DvzSceneCard* card, DvzPanel* panel);

static void _scene_card_panel_size_px(const DvzFigure* figure, const DvzPanel* panel, float out[2]);

static void _scene_card_hide(DvzSceneCard* card);

static bool _scene_card_realize(DvzFigure* figure, DvzSceneCard* card);

static bool _scene_card_renderer_supported(DvzTextRenderer renderer);

static int _scene_card_apply_style(DvzSceneCard* card, const DvzOverlayCardStyle* style);

static void _scene_card_origin_px(
    const DvzSceneCard* card, const float panel_size[2], float card_w, float card_h,
    float out_xy[2]);

static bool _readout_card_realize(DvzFigure* figure, DvzPinnedReadout* readout);

static bool _overlay_card_realize(DvzFigure* figure, DvzOverlayCard* card);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _selection_matches_pick(
    const DvzSelection* selection, const DvzPickResult* pick, DvzSelectionItem* out_item)
{
    ANN(selection);
    ANN(pick);
    ANN(out_item);
    if (!pick->hit)
        return false;
    if (pick->resolved_target == DVZ_SCENE_TARGET_NONE)
        return false;
    if (selection->desc.target != DVZ_SCENE_TARGET_NONE &&
        selection->desc.target != pick->resolved_target)
    {
        return false;
    }
    out_item->visual_id = pick->visual_id;
    out_item->target = pick->resolved_target;
    out_item->target_id = pick->resolved_id;
    out_item->link_key = pick->link_key;
    return true;
}



static bool _selection_item_equals(const DvzSelectionItem* a, const DvzSelectionItem* b)
{
    ANN(a);
    ANN(b);
    return a->visual_id == b->visual_id && a->target == b->target && a->target_id == b->target_id &&
           a->link_key == b->link_key;
}



static bool _scene_remove_selection_item(DvzSelection* selection, const DvzSelectionItem* item)
{
    ANN(selection);
    ANN(item);
    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        if (!_selection_item_equals(&selection->items[i], item))
            continue;
        for (uint32_t j = i + 1; j < selection->item_count; j++)
            selection->items[j - 1] = selection->items[j];
        selection->item_count--;
        dvz_memset(
            &selection->items[selection->item_count], sizeof(DvzSelectionItem), 0,
            sizeof(DvzSelectionItem));
        return true;
    }
    return false;
}


/**
 * Clear stored selection items without synchronizing visual masks.
 *
 * @param selection the selection
 */
static void _selection_clear_items(DvzSelection* selection)
{
    ANN(selection);
    selection->item_count = 0;
    dvz_memset(selection->items, sizeof(selection->items), 0, sizeof(selection->items));
}



/**
 * Return whether one visual supports the first selection-mask rendering path.
 *
 * @param visual the visual
 * @return whether the visual supports a per-item selection mask
 */
static bool _selection_visual_supports_mask(const DvzVisual* visual)
{
    return visual != NULL &&
           (visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_MARKER);
}



/**
 * Resolve a public visual id to a retained visual slot.
 *
 * @param scene the scene
 * @param visual_id one-based public visual id
 * @return the visual, or NULL when the id is invalid
 */
static DvzVisual* _selection_visual_from_public_id(DvzScene* scene, uint64_t visual_id)
{
    if (scene == NULL || visual_id == 0 || visual_id > scene->visual_count)
        return NULL;
    DvzVisual* visual = &scene->visuals[visual_id - 1];
    if (_scene_visual_public_id(scene, visual) != visual_id)
        return NULL;
    return visual;
}



/**
 * Return the selectable item count for one point-like visual.
 *
 * @param visual the visual
 * @return the item count, or zero when unavailable
 */
static uint32_t _selection_visual_item_count(const DvzVisual* visual)
{
    if (!_selection_visual_supports_mask(visual))
        return 0;
    int pos_idx = _attr_index(visual, "position");
    if (pos_idx < 0 || visual->attrs[pos_idx].item_count > UINT32_MAX)
        return 0;
    return (uint32_t)visual->attrs[pos_idx].item_count;
}



/**
 * Return whether the retained selection contains one visual item.
 *
 * @param selection the selection
 * @param visual the visual
 * @param visual_id one-based public visual id
 * @param item_index zero-based item index
 * @return whether the item is selected directly or through a link key
 */
static bool _selection_matches_visual_item(
    const DvzSelection* selection, const DvzVisual* visual, uint64_t visual_id,
    uint32_t item_index)
{
    ANN(selection);
    ANN(visual);

    uint64_t link_key = 0;
    if (visual->link_keys != NULL && item_index < visual->link_key_count)
        link_key = visual->link_keys[item_index];

    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        const DvzSelectionItem* item = &selection->items[i];
        if (item->target != DVZ_SCENE_TARGET_ITEM)
            continue;
        if (item->visual_id == visual_id && item->target_id == item_index)
            return true;
        if (item->link_key != 0 && link_key != 0 && item->link_key == link_key)
            return true;
    }
    return false;
}



/**
 * Rebuild the selection mask attribute for one visual.
 *
 * @param selection the selection
 * @param visual the visual
 * @return 0 on success, -1 on error
 */
static int _selection_sync_visual_mask(DvzSelection* selection, DvzVisual* visual)
{
    ANN(selection);
    ANN(visual);
    if (!_selection_visual_supports_mask(visual))
        return 0;

    uint32_t item_count = _selection_visual_item_count(visual);
    int selection_idx = _attr_index(visual, "selection");
    if (item_count == 0 && selection_idx < 0)
        return 0;
    if (item_count == 0)
        return 0;

    uint8_t* mask = (uint8_t*)dvz_calloc(item_count, sizeof(uint8_t));
    if (mask == NULL)
    {
        log_error("selection mask allocation failed for %u items", item_count);
        return -1;
    }

    uint64_t visual_id = _scene_visual_public_id(selection->scene, visual);
    bool any_selected = false;
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (_selection_matches_visual_item(selection, visual, visual_id, i))
        {
            mask[i] = 1;
            any_selected = true;
        }
    }

    if (selection_idx < 0 && !any_selected)
    {
        dvz_free(mask);
        return 0;
    }

    int res = dvz_visual_set_data(visual, "selection", mask, item_count);
    dvz_free(mask);
    return res;
}



/**
 * Synchronize all point-like visual masks affected by a selection change.
 *
 * @param selection the selection
 * @return 0 on success, -1 on error
 */
static int _selection_sync_masks(DvzSelection* selection)
{
    ANN(selection);
    if (selection->scene == NULL)
        return 0;
    if (!_scene_visual_mutation_allowed(selection->scene, "update selection masks"))
        return -1;

    int res = 0;
    DvzScene* scene = selection->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        DvzVisual* picked_visual = _selection_visual_from_public_id(scene, (uint64_t)i + 1);
        if (picked_visual != visual)
            continue;
        if (_selection_sync_visual_mask(selection, visual) != 0)
            res = -1;
    }
    return res;
}


/**
 * Resolve the panel that owns one selected-pick card.
 *
 * @param selection the selection
 * @param pick the pick result
 * @return the panel, or NULL when the pick cannot be resolved to this scene
 */
static DvzPanel* _selection_card_panel_from_pick(DvzSelection* selection, const DvzPickResult* pick)
{
    ANN(selection);
    ANN(pick);
    DvzScene* scene = selection->scene;
    if (scene == NULL)
        return NULL;

    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            if (_scene_panel_public_id(figure, panel) == pick->panel_id)
                return panel;
        }
    }
    return NULL;
}


/**
 * Format the selected-item metadata card text.
 *
 * @param selection the selection
 * @param pick the pick result
 */
static void _selection_card_refresh_text(DvzSelection* selection, const DvzPickResult* pick)
{
    ANN(selection);
    ANN(pick);
    uint64_t item_id = pick->resolved_id != 0 ? pick->resolved_id : pick->item_id;
    if (pick->link_key != 0)
    {
        dvz_snprintf(
            selection->card.text, sizeof(selection->card.text),
            "visual %" PRIu64 " item %" PRIu64 " key %" PRIu64, pick->visual_id, item_id,
            pick->link_key);
    }
    else
    {
        dvz_snprintf(
            selection->card.text, sizeof(selection->card.text),
            "visual %" PRIu64 " item %" PRIu64, pick->visual_id, item_id);
    }
}


/**
 * Hide the selected-item metadata card.
 *
 * @param selection the selection
 */
static void _selection_card_hide(DvzSelection* selection)
{
    if (selection == NULL)
        return;
    DvzFigure* figure = selection->card_panel != NULL ? selection->card_panel->figure : NULL;
    _scene_card_hide(&selection->card);
    selection->card_panel = NULL;
    selection->card_pick = (DvzPickResult){0};
    selection->card.text[0] = '\0';
    selection->card.dirty = true;
    _scene_notify_request_frame(figure);
}


/**
 * Update the selected-item metadata card from one applied pick.
 *
 * @param selection the selection
 * @param pick the pick result
 */
static void _selection_card_update_from_pick(DvzSelection* selection, const DvzPickResult* pick)
{
    ANN(selection);
    ANN(pick);
    if (!selection->card_enabled)
        return;
    DvzPanel* panel = _selection_card_panel_from_pick(selection, pick);
    if (panel == NULL)
    {
        _selection_card_hide(selection);
        return;
    }
    if (selection->card.panel == NULL)
        _scene_card_init(&selection->card, panel);
    selection->card.panel = panel;
    selection->card_panel = panel;
    selection->card_pick = *pick;
    selection->card.visible = true;
    selection->card.anchor_px[0] = (float)pick->panel_position[0];
    selection->card.anchor_px[1] = (float)pick->panel_position[1];
    _selection_card_refresh_text(selection, pick);
    selection->card.dirty = true;
    _scene_notify_request_frame(panel->figure);
}


/**
 * Realize or update the selected-item metadata card for one selection.
 *
 * @param figure the figure being prepared
 * @param selection the selection
 * @return whether realization succeeded
 */
static bool _selection_card_realize(DvzFigure* figure, DvzSelection* selection)
{
    ANN(figure);
    ANN(selection);
    if (selection->scene == NULL || selection->card_panel == NULL ||
        selection->card_panel->figure != figure)
    {
        return true;
    }
    return _scene_card_realize(figure, &selection->card);
}



/**
 * Format one numeric value for a readout label.
 *
 * @param format retained formatting state
 * @param value numeric value
 * @param out output text buffer
 * @param out_size output text buffer size
 */
static void _readout_format_value(
    const DvzSceneFormatState* format, double value, char* out, uint32_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;

    DvzSceneFormatState default_format = {
        .precision = 3,
        .trim_trailing_zeros = true,
    };
    if (format == NULL)
        format = &default_format;

    int32_t precision = format->precision;
    if (precision < 0)
        precision = 0;
    if (precision > 12)
        precision = 12;

    char value_str[64] = {0};
    if (format->scientific)
        dvz_snprintf(value_str, sizeof(value_str), "%.*e", precision, value);
    else
        dvz_snprintf(value_str, sizeof(value_str), "%.*f", precision, value);

    if (format->trim_trailing_zeros && !format->scientific)
    {
        char* dot = strchr(value_str, '.');
        if (dot != NULL)
        {
            char* end = value_str + strlen(value_str);
            while (end > dot + 1 && end[-1] == '0')
                *(--end) = '\0';
            if (end > dot && end[-1] == '.')
                *(--end) = '\0';
        }
    }

    if (format->show_unit && format->unit[0] != '\0')
    {
        dvz_snprintf(
            out, out_size, "%s%s %s%s", format->prefix, value_str, format->unit,
            format->suffix);
    }
    else
    {
        dvz_snprintf(out, out_size, "%s%s%s", format->prefix, value_str, format->suffix);
    }
}



/**
 * Refresh the cached display text for one pinned readout.
 *
 * @param readout the pinned readout
 */
static void _readout_refresh_text(DvzPinnedReadout* readout)
{
    ANN(readout);
    const DvzProbeResult* probe = &readout->probe;
    const char* label = probe->label[0] != '\0' ? probe->label : "value";

    DvzSceneFormatState probe_format = {
        .precision = 3,
        .trim_trailing_zeros = true,
    };
    const DvzSceneFormatState* format = readout->has_format ? &readout->format : &probe_format;
    if (!readout->has_format && probe->unit[0] != '\0')
    {
        probe_format.show_unit = true;
        dvz_strlcpy(probe_format.unit, probe->unit, sizeof(probe_format.unit));
    }

    char value[4][64] = {{0}};
    switch (probe->value_kind)
    {
    case DVZ_PROBE_VALUE_SCALAR:
        _readout_format_value(format, probe->scalar, value[0], sizeof(value[0]));
        dvz_snprintf(readout->text, sizeof(readout->text), "%s: %s", label, value[0]);
        break;

    case DVZ_PROBE_VALUE_VEC2:
        for (uint32_t i = 0; i < 2; i++)
            _readout_format_value(format, probe->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s", label, value[0], value[1]);
        break;

    case DVZ_PROBE_VALUE_VEC3:
        for (uint32_t i = 0; i < 3; i++)
            _readout_format_value(format, probe->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s %s", label, value[0], value[1],
            value[2]);
        break;

    case DVZ_PROBE_VALUE_VEC4:
        for (uint32_t i = 0; i < 4; i++)
            _readout_format_value(format, probe->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s %s %s", label, value[0], value[1],
            value[2], value[3]);
        break;

    case DVZ_PROBE_VALUE_LABEL:
        dvz_snprintf(readout->text, sizeof(readout->text), "%s", probe->label);
        break;

    default:
        dvz_snprintf(readout->text, sizeof(readout->text), "%s: n/a", label);
        break;
    }
}



/**
 * Initialize a reusable internal overlay card shell.
 *
 * @param card the card
 * @param panel the owning panel
 */
static void _scene_card_init(DvzSceneCard* card, DvzPanel* panel)
{
    ANN(card);
    dvz_memset(card, sizeof(DvzSceneCard), 0, sizeof(DvzSceneCard));
    card->panel = panel;
    card->offset_px[0] = 12.0f;
    card->offset_px[1] = 12.0f;
    card->padding_px[0] = 8.0f;
    card->padding_px[1] = 6.0f;
    card->min_width_px = 32.0f;
    card->height_px = 24.0f;
    card->glyph_advance_px = 7.0f;
    card->text_size_px = 12.0f;
    card->text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    card->max_text_chars = 96;
    card->background_color = dvz_color_rgba(16, 22, 32, 225);
    card->text_color = dvz_color_rgb(245, 248, 255);
    card->dirty = true;
    card->visible = true;
}


/**
 * Return whether a card text renderer is supported by the internal glyph path.
 *
 * @param renderer renderer selection
 * @return whether the renderer can be used
 */
static bool _scene_card_renderer_supported(DvzTextRenderer renderer)
{
    return renderer == DVZ_TEXT_RENDERER_AUTO ||
           renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS;
}


/**
 * Apply an optional public card style to an internal card shell.
 *
 * @param card the card
 * @param style the public style, or NULL for defaults
 * @return 0 on success, -1 on error
 */
static int _scene_card_apply_style(DvzSceneCard* card, const DvzOverlayCardStyle* style)
{
    ANN(card);
    if (style == NULL)
        return 0;
    if (!_scene_card_renderer_supported(style->text_renderer))
    {
        log_error("overlay card text renderer %d is not implemented", style->text_renderer);
        return -1;
    }

    card->background_color = style->background_color;
    card->text_color = style->text_color;
    card->padding_px[0] = style->padding_px[0];
    card->padding_px[1] = style->padding_px[1];
    card->min_width_px = style->min_width_px;
    card->height_px = style->height_px;
    card->glyph_advance_px = style->glyph_advance_px;
    card->text_size_px = style->text_size_px;
    card->text_renderer = style->text_renderer;
    card->max_text_chars = style->max_text_chars;
    card->dirty = true;
    return 0;
}



/**
 * Return the panel size in logical pixels for overlay placement.
 *
 * @param figure the figure
 * @param panel the panel
 * @param out output width and height
 */
static void _scene_card_panel_size_px(const DvzFigure* figure, const DvzPanel* panel, float out[2])
{
    ANN(figure);
    ANN(panel);
    ANN(out);
    out[0] = (float)figure->width * panel->desc.width;
    out[1] = (float)figure->height * panel->desc.height;
}


/**
 * Resolve the top-left origin of an overlay card in panel-local pixels.
 *
 * @param card the card
 * @param panel_size panel width and height in logical pixels
 * @param card_w card width in logical pixels
 * @param card_h card height in logical pixels
 * @param out_xy output top-left origin
 */
static void _scene_card_origin_px(
    const DvzSceneCard* card, const float panel_size[2], float card_w, float card_h,
    float out_xy[2])
{
    ANN(card);
    ANN(panel_size);
    ANN(out_xy);
    switch (card->placement)
    {
    case DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT:
        out_xy[0] = card->offset_px[0];
        out_xy[1] = card->offset_px[1];
        break;
    case DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT:
        out_xy[0] = panel_size[0] - card_w - card->offset_px[0];
        out_xy[1] = card->offset_px[1];
        break;
    case DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT:
        out_xy[0] = card->offset_px[0];
        out_xy[1] = panel_size[1] - card_h - card->offset_px[1];
        break;
    case DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT:
        out_xy[0] = panel_size[0] - card_w - card->offset_px[0];
        out_xy[1] = panel_size[1] - card_h - card->offset_px[1];
        break;
    case DVZ_OVERLAY_CARD_PLACEMENT_CENTER:
        out_xy[0] = 0.5f * (panel_size[0] - card_w) + card->offset_px[0];
        out_xy[1] = 0.5f * (panel_size[1] - card_h) + card->offset_px[1];
        break;
    case DVZ_OVERLAY_CARD_PLACEMENT_PIXEL:
    default:
        out_xy[0] = card->anchor_px[0] + card->offset_px[0];
        out_xy[1] = card->anchor_px[1] + card->offset_px[1];
        break;
    }
}



/**
 * Hide generated visuals for one reusable internal overlay card.
 *
 * @param card the card
 */
static void _scene_card_hide(DvzSceneCard* card)
{
    if (card == NULL)
        return;
    card->visible = false;
    if (card->background_visual != NULL)
        dvz_visual_set_visible(card->background_visual, false);
    if (card->text_visual != NULL)
    {
        dvz_visual_set_visible(card->text_visual, false);
        if (card->text_visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(card->text_visual->text.glyph_visual, false);
    }
}



/**
 * Realize or update a reusable internal overlay card.
 *
 * @param figure the figure being prepared
 * @param card the card
 * @return whether realization succeeded
 */
static bool _scene_card_realize(DvzFigure* figure, DvzSceneCard* card)
{
    ANN(figure);
    ANN(card);
    DvzPanel* panel = card->panel;
    if (panel == NULL || panel->figure != figure)
        return true;
    if (!card->visible || card->text[0] == '\0')
    {
        _scene_card_hide(card);
        return true;
    }

    DvzScene* scene = figure->scene;
    ANN(scene);
    if (card->background_visual == NULL)
    {
        card->background_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
        if (card->background_visual == NULL)
            return false;
        card->background_visual->visible = false;
        if (dvz_visual_set_alpha_mode(card->background_visual, DVZ_ALPHA_BLENDED) != 0)
            return false;
        if (dvz_visual_set_depth_test(card->background_visual, false) != 0)
            return false;
        if (dvz_panel_add_visual(
                panel, card->background_visual,
                &(DvzVisualAttachDesc){
                    .z_layer = INT32_MAX / 4 - 2, .controller_mode = DVZ_CONTROLLER_FIXED}) != 0)
            return false;
    }
    if (card->text_visual == NULL)
    {
        card->text_visual = _scene_text_visual(scene, 0);
        if (card->text_visual == NULL)
            return false;
        card->text_visual->visible = false;
        card->text_visual->text.reserved_glyph_vertices = card->max_text_chars * 6u;
        if (_scene_adornment_text_visual_set_renderer(card->text_visual, card->text_renderer) != 0)
            return false;
        if (dvz_panel_add_visual(
                panel, card->text_visual,
                &(DvzVisualAttachDesc){
                    .z_layer = INT32_MAX / 4 - 1, .controller_mode = DVZ_CONTROLLER_FIXED}) != 0)
            return false;
    }
    if (card->text_visual->text.renderer != card->text_renderer)
    {
        if (_scene_adornment_text_visual_set_renderer(card->text_visual, card->text_renderer) != 0)
            return false;
        card->dirty = true;
    }

    bool size_changed =
        card->figure_width != figure->width || card->figure_height != figure->height;
    bool text_changed = strcmp(card->realized_text, card->text) != 0;
    if (!card->dirty && !size_changed && !text_changed)
    {
        dvz_visual_set_visible(card->background_visual, true);
        dvz_visual_set_visible(card->text_visual, true);
        return true;
    }

    float panel_size[2] = {0};
    _scene_card_panel_size_px(figure, panel, panel_size);
    if (panel_size[0] <= 0.0f || panel_size[1] <= 0.0f)
    {
        _scene_card_hide(card);
        return true;
    }

    size_t text_len = strlen(card->text);
    if (text_len > card->max_text_chars)
        text_len = card->max_text_chars;
    float card_w = 2.0f * card->padding_px[0] + card->glyph_advance_px * (float)text_len;
    float card_h = card->height_px;
    if (card_w > panel_size[0] - 8.0f)
        card_w = panel_size[0] - 8.0f;
    if (card_w < card->min_width_px)
        card_w = card->min_width_px;

    float origin[2] = {0};
    _scene_card_origin_px(card, panel_size, card_w, card_h, origin);
    float x = origin[0];
    float y = origin[1];
    if (x + card_w > panel_size[0] - 4.0f)
        x = panel_size[0] - card_w - 4.0f;
    if (y + card_h > panel_size[1] - 4.0f)
        y = panel_size[1] - card_h - 4.0f;
    if (x < 4.0f)
        x = 4.0f;
    if (y < 4.0f)
        y = 4.0f;
    card->realized_rect_px[0] = x;
    card->realized_rect_px[1] = y;
    card->realized_rect_px[2] = card_w;
    card->realized_rect_px[3] = card_h;

    float x0 = -1.0f + 2.0f * x / panel_size[0];
    float x1 = -1.0f + 2.0f * (x + card_w) / panel_size[0];
    float y0 = +1.0f - 2.0f * y / panel_size[1];
    float y1 = +1.0f - 2.0f * (y + card_h) / panel_size[1];
    vec3 positions[4] = {
        {x0, y0, 0.0f},
        {x0, y1, 0.0f},
        {x1, y0, 0.0f},
        {x1, y1, 0.0f},
    };
    DvzColor colors[4] = {
        card->background_color,
        card->background_color,
        card->background_color,
        card->background_color,
    };
    DvzVisualDataUpdate background_updates[2] = {
        {.attr_name = "position", .data = positions, .item_count = 4},
        {.attr_name = "color", .data = colors, .item_count = 4},
    };
    if (dvz_visual_set_data_many(card->background_visual, background_updates, 2) != 0)
        return false;
    dvz_visual_set_visible(card->background_visual, true);

    const char* labels[1] = {card->text};
    vec3 text_pos[1] = {{x + card->padding_px[0], y + card->padding_px[1], 0.0f}};
    vec2 text_anchor[1] = {{0.0f, 0.0f}};
    float text_size[1] = {card->text_size_px};
    DvzColor text_color[1] = {card->text_color};
    float text_angle[1] = {0.0f};
    DvzVisualDataUpdate text_updates[5] = {
        {.attr_name = "position", .data = text_pos, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchor, .item_count = 1},
        {.attr_name = "size", .data = text_size, .item_count = 1},
        {.attr_name = "color", .data = text_color, .item_count = 1},
        {.attr_name = "angle", .data = text_angle, .item_count = 1},
    };
    if (dvz_visual_set_strings(card->text_visual, "text", labels, 1) != 0 ||
        dvz_visual_set_data_many(card->text_visual, text_updates, 5) != 0)
        return false;
    dvz_visual_set_visible(card->text_visual, true);

    card->figure_width = figure->width;
    card->figure_height = figure->height;
    dvz_strlcpy(card->realized_text, card->text, sizeof(card->realized_text));
    card->dirty = false;
    return true;
}



/**
 * Realize or update the private overlay card for one pinned readout.
 *
 * @param figure the figure being prepared
 * @param readout the pinned readout
 * @return whether realization succeeded
 */
static bool _readout_card_realize(DvzFigure* figure, DvzPinnedReadout* readout)
{
    ANN(figure);
    ANN(readout);
    if (readout->scene == NULL || readout->panel == NULL || readout->panel->figure != figure)
        return true;

    DvzSceneCard* card = &readout->card;
    if (readout->text[0] == '\0')
    {
        _scene_card_hide(card);
        return true;
    }
    if (card->panel == NULL)
        _scene_card_init(card, readout->panel);
    card->visible = true;
    if (strcmp(card->text, readout->text) != 0)
    {
        dvz_strlcpy(card->text, readout->text, sizeof(card->text));
        card->dirty = true;
    }
    card->anchor_px[0] = (float)readout->probe.panel_position[0];
    card->anchor_px[1] = (float)readout->probe.panel_position[1];
    return _scene_card_realize(figure, card);
}


/**
 * Realize or update one public overlay card.
 *
 * @param figure the figure being prepared
 * @param card the overlay card
 * @return whether realization succeeded
 */
static bool _overlay_card_realize(DvzFigure* figure, DvzOverlayCard* card)
{
    ANN(figure);
    ANN(card);
    if (!card->active || card->scene == NULL || card->panel == NULL || card->panel->figure != figure)
        return true;
    return _scene_card_realize(figure, &card->card);
}



/*************************************************************************************************/
/*  Interaction policy                                                                           */
/*************************************************************************************************/

/**
 * Create a scene-owned interaction policy object.
 *
 * @param scene the scene
 * @return the interaction policy, or NULL on allocation failure
 */
DvzInteractionPolicy* dvz_interaction(DvzScene* scene)
{
    ANN(scene);
    if (scene->interaction_count >= DVZ_SCENE_MAX_INTERACTIONS)
    {
        log_error("maximum interaction policy count reached");
        return NULL;
    }
    DvzInteractionPolicy* interaction = &scene->interactions[scene->interaction_count++];
    dvz_memset(interaction, sizeof(DvzInteractionPolicy), 0, sizeof(DvzInteractionPolicy));
    interaction->scene = scene;
    interaction->pick_hit_policy = DVZ_PICK_HIT_FRONTMOST;
    return interaction;
}



/**
 * Destroy a scene-owned interaction policy object.
 *
 * @param interaction the interaction policy
 */
void dvz_interaction_destroy(DvzInteractionPolicy* interaction)
{
    if (interaction == NULL)
        return;
    if (interaction->panel != NULL && interaction->panel->interaction == interaction)
        interaction->panel->interaction = NULL;
    interaction->scene = NULL;
    interaction->panel = NULL;
    interaction->selection = NULL;
    interaction->link_channel = NULL;
    interaction->auto_pin_readout = false;
}



/**
 * Bind an interaction policy to a panel.
 *
 * @param interaction the interaction policy
 * @param panel the panel
 */
void dvz_interaction_bind_panel(DvzInteractionPolicy* interaction, DvzPanel* panel)
{
    ANN(interaction);
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL ||
        interaction->scene != panel->figure->scene)
    {
        log_error("cannot bind an interaction policy across scenes");
        return;
    }
    if (interaction->panel != NULL && interaction->panel->interaction == interaction)
        interaction->panel->interaction = NULL;
    panel->interaction = interaction;
    interaction->panel = panel;
}



/**
 * Attach a retained selection object to an interaction policy.
 *
 * @param interaction the interaction policy
 * @param selection the selection
 */
void dvz_interaction_set_selection(DvzInteractionPolicy* interaction, DvzSelection* selection)
{
    ANN(interaction);
    if (selection != NULL && selection->scene != interaction->scene)
    {
        log_error("cannot bind a selection from a different scene");
        return;
    }
    interaction->selection = selection;
}



/**
 * Set the active link channel used by an interaction policy.
 *
 * @param interaction the interaction policy
 * @param channel the link channel
 */
void dvz_interaction_set_link_channel(
    DvzInteractionPolicy* interaction, DvzLinkChannel* channel)
{
    ANN(interaction);
    if (channel != NULL && channel->scene != interaction->scene)
    {
        log_error("cannot bind a link channel from a different scene");
        return;
    }
    interaction->link_channel = channel;
}



/**
 * Set the hit-selection policy used for picking.
 *
 * @param interaction the interaction policy
 * @param policy the hit-selection policy
 */
void dvz_interaction_set_pick_hit_policy(
    DvzInteractionPolicy* interaction, DvzPickHitPolicy policy)
{
    ANN(interaction);
    interaction->pick_hit_policy = policy;
}



/**
 * Enable or disable automatic probe pinning.
 *
 * @param interaction the interaction policy
 * @param enabled whether auto pinning is enabled
 */
void dvz_interaction_set_auto_pin_readout(DvzInteractionPolicy* interaction, bool enabled)
{
    ANN(interaction);
    interaction->auto_pin_readout = enabled;
}



/*************************************************************************************************/
/*  Link channels                                                                                */
/*************************************************************************************************/

/**
 * Create a scene-owned link channel.
 *
 * @param scene the scene
 * @param name the stable channel name, or NULL
 * @return the link channel, or NULL on allocation failure
 */
DvzLinkChannel* dvz_link_channel(DvzScene* scene, const char* name)
{
    ANN(scene);
    if (scene->link_channel_count >= DVZ_SCENE_MAX_LINK_CHANNELS)
    {
        log_error("maximum link channel count reached");
        return NULL;
    }
    DvzLinkChannel* channel = &scene->link_channels[scene->link_channel_count++];
    dvz_memset(channel, sizeof(DvzLinkChannel), 0, sizeof(DvzLinkChannel));
    channel->scene = scene;
    if (name != NULL)
        dvz_strlcpy(channel->name, name, sizeof(channel->name));
    return channel;
}



/**
 * Destroy a scene-owned link channel.
 *
 * @param channel the link channel
 */
void dvz_link_channel_destroy(DvzLinkChannel* channel)
{
    if (channel == NULL)
        return;
    if (channel->scene != NULL)
    {
        DvzScene* scene = channel->scene;
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            if (scene->visuals[i].link_channel == channel)
            {
                scene->visuals[i].link_channel = NULL;
                if (scene->visuals[i].link_keys != NULL)
                {
                    dvz_free(scene->visuals[i].link_keys);
                    scene->visuals[i].link_keys = NULL;
                }
                scene->visuals[i].link_key_count = 0;
            }
        }
        for (uint32_t i = 0; i < scene->interaction_count; i++)
        {
            if (scene->interactions[i].link_channel == channel)
                scene->interactions[i].link_channel = NULL;
        }
        for (uint32_t i = 0; i < scene->figure_count; i++)
        {
            DvzFigure* figure = &scene->figures[i];
            for (uint32_t j = 0; j < figure->panel_count; j++)
            {
                DvzPanel* panel = &figure->panels[j];
                if (panel->hover.link_channel == channel)
                    panel->hover.link_channel = NULL;
            }
        }
    }
    channel->scene = NULL;
}



/*************************************************************************************************/
/*  Selections                                                                                   */
/*************************************************************************************************/

/**
 * Create a retained scene-owned selection object.
 *
 * @param scene the scene
 * @param desc the selection descriptor, or NULL for defaults
 * @return the selection, or NULL on allocation failure
 */
DvzSelection* dvz_selection(DvzScene* scene, const DvzSelectionDesc* desc)
{
    ANN(scene);
    if (scene->selection_count >= DVZ_SCENE_MAX_SELECTIONS)
    {
        log_error("maximum selection count reached");
        return NULL;
    }
    DvzSelection* selection = &scene->selections[scene->selection_count++];
    dvz_memset(selection, sizeof(DvzSelection), 0, sizeof(DvzSelection));
    selection->scene = scene;
    if (desc != NULL)
        selection->desc = *desc;
    else
        selection->desc.mode = DVZ_SELECT_REPLACE;
    selection->card_enabled = true;
    return selection;
}



/**
 * Destroy a retained selection object.
 *
 * @param selection the selection
 */
void dvz_selection_destroy(DvzSelection* selection)
{
    if (selection == NULL)
        return;
    _selection_card_hide(selection);
    if (selection->scene != NULL)
    {
        DvzScene* scene = selection->scene;
        for (uint32_t i = 0; i < scene->interaction_count; i++)
        {
            if (scene->interactions[i].selection == selection)
                scene->interactions[i].selection = NULL;
        }
    }
    selection->scene = NULL;
    selection->item_count = 0;
    selection->card_enabled = false;
}



/**
 * Clear the contents of a selection object.
 *
 * @param selection the selection
 */
void dvz_selection_clear(DvzSelection* selection)
{
    ANN(selection);
    _selection_clear_items(selection);
    (void)_selection_sync_masks(selection);
    _selection_card_hide(selection);
}



/**
 * Apply one resolved pick result to a selection.
 *
 * @param selection the selection
 * @param pick the pick result
 * @return 0 on success, -1 on error
 */
int dvz_selection_apply_pick(DvzSelection* selection, const DvzPickResult* pick)
{
    ANN(selection);
    ANN(pick);
    DvzSelectionItem item = {0};
    if (!_selection_matches_pick(selection, pick, &item))
        return -1;
    bool present = false;
    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        if (_selection_item_equals(&selection->items[i], &item))
        {
            present = true;
            break;
        }
    }
    switch (selection->desc.mode)
    {
    case DVZ_SELECT_REPLACE:
    {
        _selection_clear_items(selection);
        selection->items[0] = item;
        selection->item_count = 1;
        int replace_res = _selection_sync_masks(selection);
        if (replace_res == 0)
            _selection_card_update_from_pick(selection, pick);
        return replace_res;
    }
    case DVZ_SELECT_ADDITIVE:
        if (present)
        {
            _selection_card_update_from_pick(selection, pick);
            return 0;
        }
        break;
    case DVZ_SELECT_SUBTRACT:
    {
        if (present)
            _scene_remove_selection_item(selection, &item);
        int subtract_res = _selection_sync_masks(selection);
        if (subtract_res == 0 && selection->item_count == 0)
            _selection_card_hide(selection);
        return subtract_res;
    }
    case DVZ_SELECT_TOGGLE:
        if (present)
        {
            _scene_remove_selection_item(selection, &item);
            int toggle_remove_res = _selection_sync_masks(selection);
            if (toggle_remove_res == 0)
                _selection_card_hide(selection);
            return toggle_remove_res;
        }
        break;
    default:
        break;
    }
    if (selection->item_count >= DVZ_SCENE_MAX_SELECTION_ITEMS)
    {
        log_error("selection item capacity reached");
        return -1;
    }
    selection->items[selection->item_count++] = item;
    int add_res = _selection_sync_masks(selection);
    if (add_res == 0)
        _selection_card_update_from_pick(selection, pick);
    return add_res;
}



/**
 * Return the number of stored selection items.
 *
 * @param selection the selection
 * @return the item count
 */
uint32_t dvz_selection_count(const DvzSelection* selection)
{
    ANN(selection);
    return selection->item_count;
}



/**
 * Copy selection contents into caller-owned storage.
 *
 * @param selection the selection
 * @param items the destination item array
 * @param max_items the maximum number of items to write
 */
void dvz_selection_copy(
    const DvzSelection* selection, DvzSelectionItem* items, uint32_t max_items)
{
    ANN(selection);
    if (items == NULL || max_items == 0)
        return;
    uint32_t count = selection->item_count < max_items ? selection->item_count : max_items;
    dvz_memcpy(
        items, max_items * sizeof(DvzSelectionItem), selection->items,
        count * sizeof(DvzSelectionItem));
}



/*************************************************************************************************/
/*  Hover                                                                                        */
/*************************************************************************************************/

/**
 * Return the retained hover state for one panel.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the hover state, or NULL when the panel is foreign
 */
const DvzHoverState* dvz_scene_hover(const DvzScene* scene, const DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene != scene)
        return NULL;
    return &panel->hover;
}



/**
 * Prepare private pinned-readout card visuals attached to one figure.
 *
 * @param figure the figure being emitted
 */
void _scene_prepare_pinned_readout_cards(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->pinned_readout_count; i++)
    {
        DvzPinnedReadout* readout = &scene->pinned_readouts[i];
        if (!_readout_card_realize(figure, readout))
            log_error("failed to prepare pinned readout card %u", i);
    }
}


/**
 * Prepare private selected-item card visuals attached to one figure.
 *
 * @param figure the figure being emitted
 */
void _scene_prepare_selection_cards(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->selection_count; i++)
    {
        DvzSelection* selection = &scene->selections[i];
        if (!_selection_card_realize(figure, selection))
            log_error("failed to prepare selection card %u", i);
    }
}


/**
 * Prepare public overlay-card visuals attached to one figure.
 *
 * @param figure the figure being emitted
 */
void _scene_prepare_overlay_cards(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->overlay_card_count; i++)
    {
        DvzOverlayCard* card = &scene->overlay_cards[i];
        if (!_overlay_card_realize(figure, card))
            log_error("failed to prepare overlay card %u", i);
    }
}



/*************************************************************************************************/
/*  Public overlays                                                                              */
/*************************************************************************************************/

/**
 * Create a panel overlay object.
 *
 * @param panel the panel
 * @param flags overlay flags
 * @return the overlay, or NULL on error
 */
DvzOverlay* dvz_overlay(DvzPanel* panel, uint32_t flags)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->overlay_count >= DVZ_SCENE_MAX_OVERLAYS)
    {
        log_error("maximum overlay count reached");
        return NULL;
    }

    DvzOverlay* overlay = &scene->overlays[scene->overlay_count++];
    dvz_memset(overlay, sizeof(DvzOverlay), 0, sizeof(DvzOverlay));
    overlay->scene = scene;
    overlay->panel = panel;
    overlay->flags = flags;
    overlay->active = true;
    return overlay;
}


/**
 * Destroy a panel overlay object.
 *
 * @param overlay the overlay
 */
void dvz_overlay_destroy(DvzOverlay* overlay)
{
    if (overlay == NULL)
        return;
    DvzScene* scene = overlay->scene;
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->overlay_card_count; i++)
        {
            DvzOverlayCard* card = &scene->overlay_cards[i];
            if (card->overlay == overlay)
                dvz_overlay_card_destroy(card);
        }
    }
    overlay->scene = NULL;
    overlay->panel = NULL;
    overlay->flags = 0;
    overlay->active = false;
}


/**
 * Return the default overlay-card style.
 *
 * @return the default style descriptor
 */
DvzOverlayCardStyle dvz_overlay_card_style(void)
{
    DvzSceneCard card = {0};
    _scene_card_init(&card, NULL);
    return (DvzOverlayCardStyle){
        .background_color = card.background_color,
        .text_color = card.text_color,
        .padding_px = {card.padding_px[0], card.padding_px[1]},
        .min_width_px = card.min_width_px,
        .height_px = card.height_px,
        .glyph_advance_px = card.glyph_advance_px,
        .text_size_px = card.text_size_px,
        .text_renderer = card.text_renderer,
        .max_text_chars = card.max_text_chars,
    };
}


/**
 * Create a card attached to a panel overlay.
 *
 * @param overlay the overlay
 * @param desc card descriptor, or NULL for defaults
 * @return the card, or NULL on error
 */
DvzOverlayCard* dvz_overlay_card(DvzOverlay* overlay, const DvzOverlayCardDesc* desc)
{
    ANN(overlay);
    if (!overlay->active || overlay->scene == NULL || overlay->panel == NULL)
        return NULL;
    DvzScene* scene = overlay->scene;
    if (scene->overlay_card_count >= DVZ_SCENE_MAX_OVERLAY_CARDS)
    {
        log_error("maximum overlay card count reached");
        return NULL;
    }

    DvzOverlayCard* card = &scene->overlay_cards[scene->overlay_card_count++];
    dvz_memset(card, sizeof(DvzOverlayCard), 0, sizeof(DvzOverlayCard));
    card->scene = scene;
    card->overlay = overlay;
    card->panel = overlay->panel;
    card->active = true;
    card->flags = desc != NULL ? desc->flags : 0;
    _scene_card_init(&card->card, overlay->panel);
    if (desc != NULL)
    {
        if (_scene_card_apply_style(&card->card, desc->style) != 0)
            return NULL;
        if (desc->text != NULL)
            dvz_strlcpy(card->card.text, desc->text, sizeof(card->card.text));
        card->card.placement = desc->placement;
        card->card.anchor_px[0] = desc->anchor_px[0];
        card->card.anchor_px[1] = desc->anchor_px[1];
        card->card.offset_px[0] = desc->offset_px[0];
        card->card.offset_px[1] = desc->offset_px[1];
    }
    card->card.visible = (card->flags & DVZ_OVERLAY_CARD_HIDDEN) == 0;
    card->card.dirty = true;
    _scene_notify_request_frame(overlay->panel->figure);
    return card;
}


/**
 * Destroy an overlay card.
 *
 * @param card the card
 */
void dvz_overlay_card_destroy(DvzOverlayCard* card)
{
    if (card == NULL)
        return;
    DvzFigure* figure = card->panel != NULL ? card->panel->figure : NULL;
    _scene_card_hide(&card->card);
    card->scene = NULL;
    card->overlay = NULL;
    card->panel = NULL;
    card->flags = 0;
    card->active = false;
    card->card.panel = NULL;
    card->card.text[0] = '\0';
    card->card.realized_text[0] = '\0';
    card->card.dirty = false;
    _scene_notify_request_frame(figure);
}


/**
 * Set an overlay card style.
 *
 * @param card the card
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_overlay_card_set_style(DvzOverlayCard* card, const DvzOverlayCardStyle* style)
{
    ANN(card);
    if (!card->active)
        return -1;
    DvzOverlayCardStyle resolved = style != NULL ? *style : dvz_overlay_card_style();
    if (_scene_card_apply_style(&card->card, &resolved) != 0)
        return -1;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
    return 0;
}


/**
 * Set the text displayed in an overlay card.
 *
 * @param card the card
 * @param text the text, or NULL to clear it
 */
void dvz_overlay_card_set_text(DvzOverlayCard* card, const char* text)
{
    ANN(card);
    if (!card->active)
        return;
    if (text != NULL)
        dvz_strlcpy(card->card.text, text, sizeof(card->card.text));
    else
        card->card.text[0] = '\0';
    card->card.dirty = true;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}


/**
 * Set the panel-local pixel layout of an overlay card.
 *
 * @param card the card
 * @param anchor_px panel-local anchor in logical pixels, or NULL to keep it unchanged
 * @param offset_px offset from the anchor in logical pixels, or NULL to keep it unchanged
 */
void dvz_overlay_card_set_layout(
    DvzOverlayCard* card, const float anchor_px[2], const float offset_px[2])
{
    ANN(card);
    if (!card->active)
        return;
    card->card.placement = DVZ_OVERLAY_CARD_PLACEMENT_PIXEL;
    if (anchor_px != NULL)
    {
        card->card.anchor_px[0] = anchor_px[0];
        card->card.anchor_px[1] = anchor_px[1];
    }
    if (offset_px != NULL)
    {
        card->card.offset_px[0] = offset_px[0];
        card->card.offset_px[1] = offset_px[1];
    }
    card->card.dirty = true;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}


/**
 * Set semantic placement for an overlay card.
 *
 * @param card the card
 * @param placement semantic placement mode
 * @param offset_px inward/relative offset in logical pixels, or NULL to keep it unchanged
 */
void dvz_overlay_card_set_placement(
    DvzOverlayCard* card, DvzOverlayCardPlacement placement, const float offset_px[2])
{
    ANN(card);
    if (!card->active)
        return;
    card->card.placement = placement;
    if (offset_px != NULL)
    {
        card->card.offset_px[0] = offset_px[0];
        card->card.offset_px[1] = offset_px[1];
    }
    card->card.dirty = true;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}


/**
 * Show or hide an overlay card.
 *
 * @param card the card
 * @param visible whether the card should be visible
 */
void dvz_overlay_card_set_visible(DvzOverlayCard* card, bool visible)
{
    ANN(card);
    if (!card->active)
        return;
    card->card.visible = visible;
    card->card.dirty = true;
    if (!visible)
        _scene_card_hide(&card->card);
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}



/*************************************************************************************************/
/*  Pinned readouts                                                                              */
/*************************************************************************************************/

/**
 * Create a pinned readout from a resolved probe result.
 *
 * @param panel the panel
 * @param probe the probe result
 * @return the pinned readout, or NULL on allocation failure
 */
DvzPinnedReadout* dvz_pinned_readout(DvzPanel* panel, const DvzProbeResult* probe)
{
    ANN(panel);
    ANN(probe);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->pinned_readout_count >= DVZ_SCENE_MAX_PINNED_READOUTS)
    {
        log_error("maximum pinned readout count reached");
        return NULL;
    }
    if (panel->pinned_readout_count >= DVZ_SCENE_MAX_PINNED_READOUTS)
    {
        log_error("maximum panel pinned readout count reached");
        return NULL;
    }
    DvzPinnedReadout* readout = &scene->pinned_readouts[scene->pinned_readout_count++];
    dvz_memset(readout, sizeof(DvzPinnedReadout), 0, sizeof(DvzPinnedReadout));
    readout->scene = scene;
    readout->panel = panel;
    readout->probe = *probe;
    _readout_refresh_text(readout);
    _scene_card_init(&readout->card, panel);
    dvz_strlcpy(readout->card.text, readout->text, sizeof(readout->card.text));
    readout->card.anchor_px[0] = (float)probe->panel_position[0];
    readout->card.anchor_px[1] = (float)probe->panel_position[1];
    panel->pinned_readouts[panel->pinned_readout_count++] = readout;
    _scene_notify_request_frame(panel->figure);
    return readout;
}



/**
 * Destroy a pinned readout object.
 *
 * @param readout the pinned readout
 */
void dvz_pinned_readout_destroy(DvzPinnedReadout* readout)
{
    if (readout == NULL)
        return;
    _scene_card_hide(&readout->card);
    if (readout->panel != NULL)
    {
        DvzPanel* panel = readout->panel;
        for (uint32_t i = 0; i < panel->pinned_readout_count; i++)
        {
            if (panel->pinned_readouts[i] != readout)
                continue;
            for (uint32_t j = i + 1; j < panel->pinned_readout_count; j++)
                panel->pinned_readouts[j - 1] = panel->pinned_readouts[j];
            panel->pinned_readouts[panel->pinned_readout_count - 1] = NULL;
            panel->pinned_readout_count--;
            break;
        }
    }
    readout->scene = NULL;
    readout->panel = NULL;
    readout->has_format = false;
    readout->text[0] = '\0';
    readout->card.panel = NULL;
    readout->card.background_visual = NULL;
    readout->card.text_visual = NULL;
    readout->card.figure_width = 0;
    readout->card.figure_height = 0;
    readout->card.text[0] = '\0';
    readout->card.realized_text[0] = '\0';
    readout->card.dirty = false;
}



/**
 * Override formatting on a pinned readout.
 *
 * @param readout the pinned readout
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_pinned_readout_set_format(DvzPinnedReadout* readout, const DvzFormatDesc* format)
{
    ANN(readout);
    readout->has_format = format != NULL;
    _scene_format_state_copy(&readout->format, format);
    _readout_refresh_text(readout);
    dvz_strlcpy(readout->card.text, readout->text, sizeof(readout->card.text));
    readout->card.dirty = true;
    _scene_notify_request_frame(readout->panel != NULL ? readout->panel->figure : NULL);
}
