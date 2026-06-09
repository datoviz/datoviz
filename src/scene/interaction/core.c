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
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_visual_internal.h"
#include "annotation/prepare_internal.h"
#include "core/format_state_internal.h"
#include "datoviz/font.h"
#include "datoviz/scene/interaction.h"
#include "datoviz/scene/overlay.h"
#include "interaction/internal.h"
#include "query/internal.h"
#include "annotation/text_visual_bridge.h"
#include "text/text_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ITEM_INTERACTION_HOVER_REQUEST_ID UINT64_C(0xD171000000000001)
#define DVZ_ITEM_INTERACTION_SELECTION_REQUEST_ID UINT64_C(0xD171000000000002)
#define DVZ_SELECTION_DESC_KNOWN_FLAGS 0u
#define DVZ_HOVER_DESC_KNOWN_FLAGS 0u
#define DVZ_ITEM_STATE_VISUAL_STYLE_KNOWN_FLAGS 0u
#define DVZ_SELECTION_VISUAL_STYLE_KNOWN_FLAGS 0u
#define DVZ_ITEM_INTERACTION_DESC_KNOWN_FLAGS 0u
#define DVZ_OVERLAY_CARD_STYLE_KNOWN_FLAGS 0u
#define DVZ_OVERLAY_CARD_DESC_KNOWN_FLAGS 0u
#define DVZ_OVERLAY_RICH_TEXT_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static DvzItemInteractionDesc _item_interaction_resolve_desc(
    const DvzItemInteractionDesc* desc);

static int _item_interaction_queue_query(
    DvzItemInteraction* interaction, double x, double y, uint32_t query_kind);

static bool _selection_matches_query(
    const DvzSelection* selection, const DvzQueryResult* query, DvzSelectionItem* out_item);

static bool _selection_item_equals(const DvzSelectionItem* a, const DvzSelectionItem* b);

static bool _scene_remove_selection_item(DvzSelection* selection, const DvzSelectionItem* item);

static void _selection_clear_items(DvzSelection* selection);

static bool _item_state_visual_supports(const DvzVisual* visual);

static DvzVisual* _item_state_visual_from_public_id(DvzScene* scene, uint64_t visual_id);

static uint32_t _item_state_visual_item_count(const DvzVisual* visual);

static bool _selection_matches_visual_item(
    const DvzSelection* selection, const DvzVisual* visual, uint64_t visual_id,
    uint32_t item_index);

static bool _hover_matches_visual_item(
    const DvzHover* hover, const DvzVisual* visual, uint64_t visual_id, uint32_t item_index);

static void _item_state_sync_visual_style(DvzScene* scene, DvzVisual* visual);

static int _item_state_sync_visual(DvzScene* scene, DvzVisual* visual);

static int _item_state_sync_scene(DvzScene* scene, const char* reason);

static int _selection_sync_item_state(DvzSelection* selection);

static DvzPanel* _selection_card_panel_from_query(
    DvzSelection* selection, const DvzQueryResult* query);

static void _selection_card_refresh_text(DvzSelection* selection, const DvzQueryResult* query);

static void _selection_card_hide(DvzSelection* selection);

static void _selection_card_update_from_query(DvzSelection* selection, const DvzQueryResult* query);

static bool _selection_card_realize(DvzFigure* figure, DvzSelection* selection);

static void _readout_format_value(
    const DvzSceneFormatState* format, double value, char* out, uint32_t out_size);

static void _readout_refresh_text(DvzPinnedReadout* readout);

static void _scene_card_init(DvzSceneCard* card, DvzPanel* panel);

static void _scene_card_panel_size_px(const DvzFigure* figure, const DvzPanel* panel, float out[2]);

static void _scene_card_hide(DvzSceneCard* card);

static bool _scene_card_realize(DvzFigure* figure, DvzSceneCard* card);

static bool _scene_card_renderer_supported(DvzTextRenderer renderer);

static bool _overlay_card_style_validate(const DvzOverlayCardStyle* style);

static bool _overlay_card_desc_validate(const DvzOverlayCardDesc* desc);

static bool _overlay_rich_text_desc_validate(const DvzOverlayRichTextDesc* desc);

static int _scene_card_apply_style(DvzSceneCard* card, const DvzOverlayCardStyle* style);

static void _scene_card_origin_px(
    const DvzSceneCard* card, const float panel_size[2], float card_w, float card_h,
    float out_xy[2]);

static bool _readout_card_realize(DvzFigure* figure, DvzPinnedReadout* readout);

static bool _overlay_card_realize(DvzFigure* figure, DvzOverlayCard* card);

static void _overlay_card_hide_rich(DvzOverlayCard* card);

static bool _overlay_card_realize_rich(DvzFigure* figure, DvzOverlayCard* card);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _selection_desc_validate(const DvzSelectionDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzSelectionDesc, DVZ_SELECTION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzSelectionDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _hover_desc_validate(const DvzHoverDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzHoverDesc, DVZ_HOVER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzHoverDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _item_state_visual_style_abi_validate(const DvzItemStateVisualStyle* style)
{
    if (style == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            style, DvzItemStateVisualStyle, DVZ_ITEM_STATE_VISUAL_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzItemStateVisualStyle ABI prologue");
        return false;
    }
    return true;
}



static bool _selection_visual_style_abi_validate(const DvzSelectionVisualStyle* style)
{
    if (style == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            style, DvzSelectionVisualStyle, DVZ_SELECTION_VISUAL_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzSelectionVisualStyle ABI prologue");
        return false;
    }
    if (
        !_item_state_visual_style_abi_validate(&style->selected) ||
        !_item_state_visual_style_abi_validate(&style->unselected))
    {
        return false;
    }
    return true;
}



static bool _item_interaction_desc_validate(const DvzItemInteractionDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzItemInteractionDesc, DVZ_ITEM_INTERACTION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzItemInteractionDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _overlay_card_style_validate(const DvzOverlayCardStyle* style)
{
    if (style == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(style, DvzOverlayCardStyle, DVZ_OVERLAY_CARD_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzOverlayCardStyle ABI prologue");
        return false;
    }
    if (!_scene_card_renderer_supported(style->text_renderer))
    {
        log_error("overlay card text renderer %d is not implemented", style->text_renderer);
        return false;
    }
    return true;
}



static bool _overlay_card_desc_validate(const DvzOverlayCardDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzOverlayCardDesc, DVZ_OVERLAY_CARD_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzOverlayCardDesc ABI prologue");
        return false;
    }
    if (!_overlay_card_style_validate(desc->style))
        return false;
    return true;
}



static bool _overlay_rich_text_desc_validate(const DvzOverlayRichTextDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(
            desc, DvzOverlayRichTextDesc, DVZ_OVERLAY_RICH_TEXT_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzOverlayRichTextDesc ABI prologue");
        return false;
    }
    return true;
}



static DvzItemInteractionDesc _item_interaction_resolve_desc(
    const DvzItemInteractionDesc* desc)
{
    DvzItemInteractionDesc defaults = dvz_item_interaction_desc();
    if (desc == NULL)
        return defaults;

    DvzItemInteractionDesc resolved = *desc;
    if (!resolved.hover_enabled && !resolved.selection_enabled)
    {
        resolved.hover_enabled = true;
        resolved.selection_enabled = true;
    }
    if (resolved.target == DVZ_SCENE_TARGET_NONE)
        resolved.target = DVZ_SCENE_TARGET_ITEM;
    if (resolved.hit_policy == 0)
        resolved.hit_policy = DVZ_QUERY_HIT_FRONTMOST;
    if (!resolved.clear_hover_on_miss && !resolved.clear_selection_on_miss)
    {
        resolved.clear_hover_on_miss = defaults.clear_hover_on_miss;
        resolved.clear_selection_on_miss = defaults.clear_selection_on_miss;
    }
    return resolved;
}



static int _item_interaction_queue_query(
    DvzItemInteraction* interaction, double x, double y, uint32_t query_kind)
{
    ANN(interaction);
    if (!interaction->active || interaction->panel == NULL || interaction->scene == NULL)
        return -1;

    uint64_t request_id = query_kind == DVZ_ITEM_INTERACTION_QUERY_SELECTION
                              ? DVZ_ITEM_INTERACTION_SELECTION_REQUEST_ID
                              : DVZ_ITEM_INTERACTION_HOVER_REQUEST_ID;
    DvzQueryRequest request = dvz_query_request();
    request.request_id = request_id;
    request.target = interaction->desc.target;
    request.hit_policy = interaction->desc.hit_policy;
    if (dvz_panel_query(interaction->panel, x, y, &request) != 0)
        return -1;

    if (interaction->scene->pending_query_count == 0)
        return -1;
    DvzPendingQueryRequest* pending =
        &interaction->scene->pending_queries[interaction->scene->pending_query_count - 1];
    pending->item_interaction = interaction;
    pending->item_interaction_kind = query_kind;
    return 0;
}



static bool _selection_matches_query(
    const DvzSelection* selection, const DvzQueryResult* query, DvzSelectionItem* out_item)
{
    ANN(selection);
    ANN(query);
    ANN(out_item);
    if (!query->hit)
        return false;
    if (query->resolved_target == DVZ_SCENE_TARGET_NONE)
        return false;
    if (selection->desc.target != DVZ_SCENE_TARGET_NONE &&
        selection->desc.target != query->resolved_target)
    {
        return false;
    }
    out_item->visual_id = query->visual_id;
    out_item->target = query->resolved_target;
    out_item->target_id = query->resolved_id;
    out_item->link_key = query->link_key;
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
 * Clear stored selection items without synchronizing visual item state.
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
 * Return whether one visual supports the retained item-state rendering path.
 *
 * @param visual the visual
 * @return whether the visual supports a per-item state bitfield
 */
static bool _item_state_visual_supports(const DvzVisual* visual)
{
    if (visual == NULL)
        return false;
    if (visual->type == DVZ_VISUAL_TYPE_MESH)
        return _visual_family_state(visual)->field == NULL;
    return visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
           visual->type == DVZ_VISUAL_TYPE_MARKER || visual->type == DVZ_VISUAL_TYPE_SPHERE;
}



/**
 * Resolve a public visual id to a retained visual slot.
 *
 * @param scene the scene
 * @param visual_id one-based public visual id
 * @return the visual, or NULL when the id is invalid
 */
static DvzVisual* _item_state_visual_from_public_id(DvzScene* scene, uint64_t visual_id)
{
    if (scene == NULL || visual_id == 0 || visual_id > scene->visual_count)
        return NULL;
    DvzVisual* visual = &scene->visuals[visual_id - 1];
    if (_scene_visual_public_id(scene, visual) != visual_id)
        return NULL;
    return visual;
}



/**
 * Return the selectable item count for one item-state visual.
 *
 * @param visual the visual
 * @return the item count, or zero when unavailable
 */
static uint32_t _item_state_visual_item_count(const DvzVisual* visual)
{
    if (!_item_state_visual_supports(visual))
        return 0;
    if (visual->type == DVZ_VISUAL_TYPE_MESH)
    {
        int transform_idx = _attr_index(visual, "instance_transform");
        if (
            transform_idx >= 0 && visual->attrs[transform_idx].data != NULL &&
            visual->attrs[transform_idx].item_count > 0 &&
            visual->attrs[transform_idx].item_count <= UINT32_MAX)
        {
            return (uint32_t)visual->attrs[transform_idx].item_count;
        }
        return 1;
    }
    int pos_idx = _attr_index(visual, "position");
    if (pos_idx < 0 || visual->attrs[pos_idx].item_count > UINT32_MAX)
        return 0;
    return (uint32_t)visual->attrs[pos_idx].item_count;
}



/**
 * Return the link key for one visual item.
 *
 * @param visual the visual
 * @param item_index zero-based item index
 * @return link key, or 0 when absent
 */
static uint64_t _item_state_visual_link_key(const DvzVisual* visual, uint32_t item_index)
{
    ANN(visual);
    if (visual->link_keys != NULL && item_index < visual->link_key_count)
        return visual->link_keys[item_index];
    return 0;
}



/**
 * Return whether one resolved target matches a visual item directly or through a link key.
 *
 * @param item resolved interaction item
 * @param visual the visual
 * @param visual_id one-based public visual id
 * @param item_index zero-based item index
 * @return whether the item matches
 */
static bool _item_state_matches_visual_item(
    const DvzSelectionItem* item, const DvzVisual* visual, uint64_t visual_id, uint32_t item_index)
{
    ANN(item);
    ANN(visual);
    if (item->target != DVZ_SCENE_TARGET_ITEM)
        return false;
    if (item->visual_id == visual_id && item->target_id == item_index)
        return true;

    uint64_t link_key = _item_state_visual_link_key(visual, item_index);
    return item->link_key != 0 && link_key != 0 && item->link_key == link_key;
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
    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        if (_item_state_matches_visual_item(&selection->items[i], visual, visual_id, item_index))
            return true;
    }
    return false;
}



/**
 * Return whether the retained hover contains one visual item.
 *
 * @param hover the hover
 * @param visual the visual
 * @param visual_id one-based public visual id
 * @param item_index zero-based item index
 * @return whether the item is hovered directly or through a link key
 */
static bool _hover_matches_visual_item(
    const DvzHover* hover, const DvzVisual* visual, uint64_t visual_id, uint32_t item_index)
{
    ANN(hover);
    ANN(visual);
    if (!hover->has_item)
        return false;
    return _item_state_matches_visual_item(&hover->item, visual, visual_id, item_index);
}



/**
 * Store one item-state style in a dedicated item-state style payload slot.
 *
 * @param dst item-state style payload vec4
 * @param tint_dst item-state style tint payload vec4
 * @param style item-state style
 */
static void _item_state_store_style_params(
    float dst[4], float tint_dst[4], const DvzItemStateVisualStyle* style)
{
    ANN(dst);
    ANN(tint_dst);
    ANN(style);
    dst[0] = (float)style->visual_flags;
    dst[1] = isfinite(style->alpha) ? style->alpha : 1.0f;
    dst[2] = isfinite(style->tint_mix) ? style->tint_mix : 0.0f;
    dst[3] = isfinite(style->scale) && style->scale > 0.0f ? style->scale : 1.0f;
    tint_dst[0] = (float)style->tint.r / 255.0f;
    tint_dst[1] = (float)style->tint.g / 255.0f;
    tint_dst[2] = (float)style->tint.b / 255.0f;
    tint_dst[3] = (float)style->tint.a / 255.0f;
}



/**
 * Return the first active selection style in the scene.
 *
 * @param scene the scene
 * @param out selected/unselected style output
 * @return whether an active selection style was found
 */
static bool _item_state_active_selection_style(
    DvzScene* scene, DvzSelectionVisualStyle* out)
{
    ANN(scene);
    ANN(out);
    for (uint32_t i = 0; i < scene->selection_count; i++)
    {
        DvzSelection* selection = &scene->selections[i];
        if (selection->scene != scene || selection->item_count == 0)
            continue;
        *out = selection->visual_style;
        return true;
    }
    return false;
}



/**
 * Return the first active hover style in the scene.
 *
 * @param scene the scene
 * @param out hover style output
 * @return whether an active hover style was found
 */
static bool _item_state_active_hover_style(DvzScene* scene, DvzItemStateVisualStyle* out)
{
    ANN(scene);
    ANN(out);
    for (uint32_t i = 0; i < scene->hover_count; i++)
    {
        DvzHover* hover = &scene->hovers[i];
        if (hover->scene != scene || !hover->has_item)
            continue;
        *out = hover->visual_style;
        return true;
    }
    return false;
}



/**
 * Synchronize item-state visual style material parameters.
 *
 * @param scene the scene
 * @param visual the visual
 */
static void _item_state_sync_visual_style(DvzScene* scene, DvzVisual* visual)
{
    ANN(scene);
    ANN(visual);
    DvzItemStateVisualStyle normal = dvz_item_state_visual_style();
    DvzSelectionVisualStyle selection_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzSelectionVisualStyle),
        .selected = normal,
        .unselected = normal,
    };
    DvzItemStateVisualStyle hover_style = normal;
    (void)_item_state_active_selection_style(scene, &selection_style);
    (void)_item_state_active_hover_style(scene, &hover_style);

    DvzSceneItemStateStyleParams* item_params =
        &_visual_family_state(visual)->item_state_style_params;
    _item_state_store_style_params(
        item_params->selected, item_params->selected_tint, &selection_style.selected);
    _item_state_store_style_params(
        item_params->unselected, item_params->unselected_tint, &selection_style.unselected);
    _item_state_store_style_params(item_params->hovered, item_params->hovered_tint, &hover_style);
    _visual_family_state(visual)->item_state_style_params_dirty = true;

    _visual_family_state(visual)->material_params_dirty = true;
}



/**
 * Rebuild the item-state attribute for one visual.
 *
 * @param scene the scene
 * @param visual the visual
 * @return 0 on success, -1 on error
 */
static int _item_state_sync_visual(DvzScene* scene, DvzVisual* visual)
{
    ANN(scene);
    ANN(visual);
    if (!_item_state_visual_supports(visual))
        return 0;

    uint32_t item_count = _item_state_visual_item_count(visual);
    int item_state_idx = _attr_index(visual, "item_state");
    if (item_count == 0)
        return 0;

    uint32_t* states = (uint32_t*)dvz_calloc(item_count, sizeof(uint32_t));
    if (states == NULL)
    {
        log_error("item_state allocation failed for %u items", item_count);
        return -1;
    }

    uint64_t visual_id = _scene_visual_public_id(scene, visual);
    bool any_state = false;
    for (uint32_t item_index = 0; item_index < item_count; item_index++)
    {
        for (uint32_t i = 0; i < scene->selection_count; i++)
        {
            DvzSelection* selection = &scene->selections[i];
            if (selection->scene != scene)
                continue;
            if (_selection_matches_visual_item(selection, visual, visual_id, item_index))
                states[item_index] |= DVZ_ITEM_STATE_SELECTED;
        }
        for (uint32_t i = 0; i < scene->hover_count; i++)
        {
            DvzHover* hover = &scene->hovers[i];
            if (hover->scene != scene)
                continue;
            if (_hover_matches_visual_item(hover, visual, visual_id, item_index))
                states[item_index] |= DVZ_ITEM_STATE_HOVERED;
        }
        any_state = any_state || states[item_index] != 0;
    }

    if (item_state_idx < 0 && !any_state)
    {
        dvz_free(states);
        return 0;
    }

    _item_state_sync_visual_style(scene, visual);
    int res = dvz_visual_set_data(visual, "item_state", states, item_count);
    dvz_free(states);
    return res;
}



/**
 * Synchronize all retained item-state attributes in a scene.
 *
 * @param scene the scene
 * @param reason mutation reason for diagnostics
 * @return 0 on success, -1 on error
 */
static int _item_state_sync_scene(DvzScene* scene, const char* reason)
{
    ANN(scene);
    if (!_scene_visual_mutation_allowed(scene, reason != NULL ? reason : "update item_state"))
        return -1;

    int res = 0;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        DvzVisual* picked_visual = _item_state_visual_from_public_id(scene, (uint64_t)i + 1);
        if (picked_visual != visual)
            continue;
        if (_item_state_sync_visual(scene, visual) != 0)
            res = -1;
    }
    return res;
}



/**
 * Synchronize all point-like visual states affected by a selection change.
 *
 * @param selection the selection
 * @return 0 on success, -1 on error
 */
static int _selection_sync_item_state(DvzSelection* selection)
{
    ANN(selection);
    if (selection->scene == NULL)
        return 0;
    return _item_state_sync_scene(selection->scene, "update item_state");
}



/**
 * Resolve the panel that owns one selected-query card.
 *
 * @param selection the selection
 * @param query the query result
 * @return the panel, or NULL when the query cannot be resolved to this scene
 */
static DvzPanel* _selection_card_panel_from_query(
    DvzSelection* selection, const DvzQueryResult* query)
{
    ANN(selection);
    ANN(query);
    DvzScene* scene = selection->scene;
    if (scene == NULL)
        return NULL;

    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            if (_scene_panel_public_id(figure, panel) == query->panel_id)
                return panel;
        }
    }
    return NULL;
}


/**
 * Format the selected-item metadata card text.
 *
 * @param selection the selection
 * @param query the query result
 */
static void _selection_card_refresh_text(DvzSelection* selection, const DvzQueryResult* query)
{
    ANN(selection);
    ANN(query);
    uint64_t item_id = query->resolved_id != 0 ? query->resolved_id : query->item_id;
    if (query->link_key != 0)
    {
        dvz_snprintf(
            selection->card.text, sizeof(selection->card.text),
            "visual %" PRIu64 " item %" PRIu64 " key %" PRIu64, query->visual_id, item_id,
            query->link_key);
    }
    else
    {
        dvz_snprintf(
            selection->card.text, sizeof(selection->card.text),
            "visual %" PRIu64 " item %" PRIu64, query->visual_id, item_id);
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
    selection->card_query = (DvzQueryResult){0};
    selection->card.text[0] = '\0';
    selection->card.dirty = true;
    _scene_notify_request_frame(figure);
}


/**
 * Update the selected-item metadata card from one applied query.
 *
 * @param selection the selection
 * @param query the query result
 */
static void _selection_card_update_from_query(DvzSelection* selection, const DvzQueryResult* query)
{
    ANN(selection);
    ANN(query);
    if (!selection->card_enabled)
        return;
    DvzPanel* panel = _selection_card_panel_from_query(selection, query);
    if (panel == NULL)
    {
        _selection_card_hide(selection);
        return;
    }
    if (selection->card.panel == NULL)
        _scene_card_init(&selection->card, panel);
    selection->card.panel = panel;
    selection->card_panel = panel;
    selection->card_query = *query;
    selection->card.visible = true;
    selection->card.anchor_px[0] = (float)query->panel_position[0];
    selection->card.anchor_px[1] = (float)query->panel_position[1];
    _selection_card_refresh_text(selection, query);
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
    const DvzQueryResult* query = &readout->query;
    const char* label = query->label[0] != '\0' ? query->label : "value";

    DvzSceneFormatState query_format = {
        .precision = 3,
        .trim_trailing_zeros = true,
    };
    const DvzSceneFormatState* format = readout->has_format ? &readout->format : &query_format;
    if (!readout->has_format && query->unit[0] != '\0')
    {
        query_format.show_unit = true;
        dvz_strlcpy(query_format.unit, query->unit, sizeof(query_format.unit));
    }

    char value[4][64] = {{0}};
    switch (query->value_kind)
    {
    case DVZ_QUERY_VALUE_SCALAR:
        _readout_format_value(format, query->scalar, value[0], sizeof(value[0]));
        dvz_snprintf(readout->text, sizeof(readout->text), "%s: %s", label, value[0]);
        break;

    case DVZ_QUERY_VALUE_VEC2:
        for (uint32_t i = 0; i < 2; i++)
            _readout_format_value(format, query->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s", label, value[0], value[1]);
        break;

    case DVZ_QUERY_VALUE_VEC3:
        for (uint32_t i = 0; i < 3; i++)
            _readout_format_value(format, query->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s %s", label, value[0], value[1],
            value[2]);
        break;

    case DVZ_QUERY_VALUE_VEC4:
        for (uint32_t i = 0; i < 4; i++)
            _readout_format_value(format, query->vector[i], value[i], sizeof(value[i]));
        dvz_snprintf(
            readout->text, sizeof(readout->text), "%s: %s %s %s %s", label, value[0], value[1],
            value[2], value[3]);
        break;

    case DVZ_QUERY_VALUE_CATEGORY:
    case DVZ_QUERY_VALUE_TEXT:
        if (query->label[0] != '\0')
            dvz_snprintf(readout->text, sizeof(readout->text), "%s", query->label);
        else
            dvz_snprintf(readout->text, sizeof(readout->text), "%s: n/a", label);
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
    card->content = DVZ_SCENE_CARD_CONTENT_TEXT;
    card->glyph_advance_px = 7.5f;
    card->text_size_px = dvz_font_defaults().text_size_px;
    card->text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
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
    if (!_overlay_card_style_validate(style))
        return -1;

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
        if (_visual_family_state(card->text_visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(card->text_visual)->text.glyph_visual, false);
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
    if (!card->visible ||
        (card->content == DVZ_SCENE_CARD_CONTENT_TEXT && card->text[0] == '\0') ||
        (card->content == DVZ_SCENE_CARD_CONTENT_IMAGE &&
         (card->content_size_px[0] <= 0.0f || card->content_size_px[1] <= 0.0f)))
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
        DvzVisualAttachDesc attach = dvz_visual_attach_desc();
        attach.z_layer = INT32_MAX / 4 - 2;
        attach.controller_mode = DVZ_CONTROLLER_FIXED;
        if (dvz_panel_add_visual(panel, card->background_visual, &attach) != 0)
            return false;
    }
    if (card->content == DVZ_SCENE_CARD_CONTENT_TEXT && card->text_visual == NULL)
    {
        card->text_visual = _scene_text_visual(scene, 0);
        if (card->text_visual == NULL)
            return false;
        card->text_visual->visible = false;
        _visual_family_state(card->text_visual)->text.reserved_glyph_vertices = card->max_text_chars * 6u;
        if (_scene_adornment_text_visual_set_renderer(card->text_visual, card->text_renderer) != 0)
            return false;
        DvzVisualAttachDesc attach = dvz_visual_attach_desc();
        attach.z_layer = INT32_MAX / 4 - 1;
        attach.controller_mode = DVZ_CONTROLLER_FIXED;
        if (dvz_panel_add_visual(panel, card->text_visual, &attach) != 0)
            return false;
    }
    if (card->content == DVZ_SCENE_CARD_CONTENT_TEXT &&
        _visual_family_state(card->text_visual)->text.renderer != card->text_renderer)
    {
        if (_scene_adornment_text_visual_set_renderer(card->text_visual, card->text_renderer) != 0)
            return false;
        card->dirty = true;
    }
    if (card->content == DVZ_SCENE_CARD_CONTENT_IMAGE && card->text_visual != NULL)
    {
        dvz_visual_set_visible(card->text_visual, false);
        if (_visual_family_state(card->text_visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(card->text_visual)->text.glyph_visual, false);
    }

    bool size_changed =
        card->figure_width != figure->width || card->figure_height != figure->height;
    bool text_changed = card->content == DVZ_SCENE_CARD_CONTENT_TEXT &&
                        strcmp(card->realized_text, card->text) != 0;
    if (!card->dirty && !size_changed && !text_changed)
    {
        dvz_visual_set_visible(card->background_visual, true);
        if (card->content == DVZ_SCENE_CARD_CONTENT_TEXT && card->text_visual != NULL)
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

    float card_w = 0.0f;
    float card_h = 0.0f;
    if (card->content == DVZ_SCENE_CARD_CONTENT_IMAGE)
    {
        card_w = 2.0f * card->padding_px[0] + card->content_size_px[0];
        card_h = 2.0f * card->padding_px[1] + card->content_size_px[1];
    }
    else
    {
        size_t text_len = strlen(card->text);
        if (text_len > card->max_text_chars)
            text_len = card->max_text_chars;
        card_w = 2.0f * card->padding_px[0] + card->glyph_advance_px * (float)text_len;
        card_h = card->height_px;
    }
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

    if (card->content == DVZ_SCENE_CARD_CONTENT_TEXT)
    {
        ANN(card->text_visual);
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
    }

    card->figure_width = figure->width;
    card->figure_height = figure->height;
    if (card->content == DVZ_SCENE_CARD_CONTENT_TEXT)
        dvz_strlcpy(card->realized_text, card->text, sizeof(card->realized_text));
    else
        card->realized_text[0] = '\0';
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
    card->anchor_px[0] = (float)readout->query.panel_position[0];
    card->anchor_px[1] = (float)readout->query.panel_position[1];
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
    if (!card->rich_enabled)
    {
        _overlay_card_hide_rich(card);
        return _scene_card_realize(figure, &card->card);
    }
    return _overlay_card_realize_rich(figure, card);
}


/**
 * Hide the rich-image visual associated with one public overlay card.
 *
 * @param card the overlay card
 */
static void _overlay_card_hide_rich(DvzOverlayCard* card)
{
    if (card == NULL)
        return;
    if (card->rich_block.image_visual != NULL)
        dvz_visual_set_visible(card->rich_block.image_visual, false);
}


/**
 * Realize or update one public overlay card with rich text content.
 *
 * @param figure the figure being prepared
 * @param card the overlay card
 * @return whether realization succeeded
 */
static bool _overlay_card_realize_rich(DvzFigure* figure, DvzOverlayCard* card)
{
    ANN(figure);
    ANN(card);
    DvzPanel* panel = card->panel;
    if (panel == NULL || panel->figure != figure)
        return true;
    if (!card->card.visible)
    {
        _scene_card_hide(&card->card);
        _overlay_card_hide_rich(card);
        return true;
    }

    if (card->rich_dirty || !card->rich_block.valid || card->rich_block.rgba == NULL)
    {
        if (_scene_text_block_parse(&card->rich_block) != 0)
            return false;
        if (_scene_text_block_measure(&card->rich_block, &card->rich_layout) != 0)
            return false;
        if (_scene_text_block_rasterize(&card->rich_block, &card->rich_raster) != 0)
            return false;
        card->rich_dirty = false;
    }

    float old_content_w = card->card.content_size_px[0];
    float old_content_h = card->card.content_size_px[1];
    card->card.content = DVZ_SCENE_CARD_CONTENT_IMAGE;
    card->card.content_size_px[0] =
        card->rich_block.raster_scale > 0.0f
            ? (float)card->rich_block.raster_width / card->rich_block.raster_scale
            : (float)card->rich_block.raster_width;
    card->card.content_size_px[1] =
        card->rich_block.raster_scale > 0.0f
            ? (float)card->rich_block.raster_height / card->rich_block.raster_scale
            : (float)card->rich_block.raster_height;
    if (old_content_w != card->card.content_size_px[0] ||
        old_content_h != card->card.content_size_px[1])
    {
        card->card.dirty = true;
    }
    if (!_scene_card_realize(figure, &card->card))
        return false;

    float content_x = card->card.realized_rect_px[0] + card->card.padding_px[0];
    float content_y = card->card.realized_rect_px[1] + card->card.padding_px[1];
    float content_w = card->card.content_size_px[0];
    float content_h = card->card.content_size_px[1];
    return _scene_text_block_realize_image(
               &card->rich_block, panel,
               &(DvzTextBlockImageDesc){
                   .position_px = {content_x, content_y, 0.0f},
                   .extent_px = {content_w, content_h},
                   .anchor = {-1.0f, -1.0f},
                   .pixel_space = true,
                   .z_layer = INT32_MAX / 4 - 1,
                   .controller_mode = DVZ_CONTROLLER_FIXED,
               }) == 0;
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
    interaction->query_hit_policy = DVZ_QUERY_HIT_FRONTMOST;
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
 * Set the hit-selection policy used for panel queries.
 *
 * @param interaction the interaction policy
 * @param policy the hit-selection policy
 */
void dvz_interaction_set_query_hit_policy(
    DvzInteractionPolicy* interaction, DvzQueryHitPolicy policy)
{
    ANN(interaction);
    interaction->query_hit_policy = policy;
}



/**
 * Enable or disable automatic query readout pinning.
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

DvzSelectionDesc dvz_selection_desc(void)
{
    return (DvzSelectionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
        .mode = DVZ_SELECT_REPLACE,
        .target = DVZ_SCENE_TARGET_NONE,
        .selection_flags = 0,
    };
}



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
    if (!_selection_desc_validate(desc))
        return NULL;
    if (scene->selection_count >= DVZ_SCENE_MAX_SELECTIONS)
    {
        log_error("maximum selection count reached");
        return NULL;
    }
    DvzSelection* selection = &scene->selections[scene->selection_count++];
    dvz_memset(selection, sizeof(DvzSelection), 0, sizeof(DvzSelection));
    selection->scene = scene;
    selection->desc = desc != NULL ? *desc : dvz_selection_desc();
    selection->visual_style = dvz_selection_visual_style();
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
    DvzScene* scene = selection->scene;
    if (scene != NULL && selection->item_count > 0)
    {
        _selection_clear_items(selection);
        (void)_item_state_sync_scene(scene, "destroy selection item_state");
    }
    _selection_card_hide(selection);
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->interaction_count; i++)
        {
            if (scene->interactions[i].selection == selection)
                scene->interactions[i].selection = NULL;
        }
        for (uint32_t i = 0; i < scene->item_interaction_count; i++)
        {
            if (scene->item_interactions[i].selection == selection)
                scene->item_interactions[i].selection = NULL;
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
    (void)_selection_sync_item_state(selection);
    _selection_card_hide(selection);
}



/**
 * Return the default item-state visual style.
 *
 * @return the default item-state style descriptor
 */
DvzItemStateVisualStyle dvz_item_state_visual_style(void)
{
    return (DvzItemStateVisualStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzItemStateVisualStyle),
        .visual_flags = DVZ_ITEM_STATE_VISUAL_NONE,
        .alpha = 1.0f,
        .tint = {255, 255, 255, 255},
        .tint_mix = 0.0f,
        .scale = 1.0f,
    };
}



/**
 * Return the default selection visual style.
 *
 * @return the default style descriptor
 */
DvzSelectionVisualStyle dvz_selection_visual_style(void)
{
    DvzItemStateVisualStyle normal = dvz_item_state_visual_style();
    DvzItemStateVisualStyle unselected = normal;
    unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_ALPHA;
    unselected.alpha = 0.25f;
    return (DvzSelectionVisualStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzSelectionVisualStyle),
        .selected = normal,
        .unselected = unselected,
    };
}



/**
 * Configure selected/unselected visual styling for a retained selection.
 *
 * @param selection the selection
 * @param style style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_selection_set_visual_style(DvzSelection* selection, const DvzSelectionVisualStyle* style)
{
    ANN(selection);
    if (!_selection_visual_style_abi_validate(style))
        return -1;
    DvzSelectionVisualStyle resolved = style != NULL ? *style : dvz_selection_visual_style();
    const uint32_t supported_flags =
        DVZ_ITEM_STATE_VISUAL_ALPHA | DVZ_ITEM_STATE_VISUAL_TINT | DVZ_ITEM_STATE_VISUAL_SCALE;
    if ((resolved.selected.visual_flags & ~supported_flags) != 0 ||
        (resolved.unselected.visual_flags & ~supported_flags) != 0)
    {
        log_error("unsupported selection visual style flags");
        return -1;
    }
    if (
        !isfinite(resolved.selected.alpha) || !isfinite(resolved.selected.tint_mix) ||
        !isfinite(resolved.selected.scale) || !isfinite(resolved.unselected.alpha) ||
        !isfinite(resolved.unselected.tint_mix) || !isfinite(resolved.unselected.scale))
    {
        log_error("selection visual alpha and tint_mix values must be finite");
        return -1;
    }

    selection->visual_style = resolved;
    return _selection_sync_item_state(selection);
}



/**
 * Apply one resolved query result to a selection.
 *
 * @param selection the selection
 * @param query the query result
 * @return 0 on success, -1 on error
 */
int dvz_selection_apply_query(DvzSelection* selection, const DvzQueryResult* query)
{
    ANN(selection);
    ANN(query);
    DvzSelectionItem item = {0};
    if (!_selection_matches_query(selection, query, &item))
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
        int replace_res = _selection_sync_item_state(selection);
        if (replace_res == 0)
            _selection_card_update_from_query(selection, query);
        return replace_res;
    }
    case DVZ_SELECT_ADDITIVE:
        if (present)
        {
            _selection_card_update_from_query(selection, query);
            return 0;
        }
        break;
    case DVZ_SELECT_SUBTRACT:
    {
        if (present)
            _scene_remove_selection_item(selection, &item);
        int subtract_res = _selection_sync_item_state(selection);
        if (subtract_res == 0 && selection->item_count == 0)
            _selection_card_hide(selection);
        return subtract_res;
    }
    case DVZ_SELECT_TOGGLE:
        if (present)
        {
            _scene_remove_selection_item(selection, &item);
            int toggle_remove_res = _selection_sync_item_state(selection);
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
    int add_res = _selection_sync_item_state(selection);
    if (add_res == 0)
        _selection_card_update_from_query(selection, query);
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

DvzHoverDesc dvz_hover_desc(void)
{
    return (DvzHoverDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
        .target = DVZ_SCENE_TARGET_ITEM,
        .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        .hover_flags = 0,
    };
}



/**
 * Create a retained scene-owned hover object.
 *
 * @param scene the scene
 * @param desc hover descriptor, or NULL for defaults
 * @return the hover, or NULL on allocation failure
 */
DvzHover* dvz_hover(DvzScene* scene, const DvzHoverDesc* desc)
{
    ANN(scene);
    if (!_hover_desc_validate(desc))
        return NULL;
    if (scene->hover_count >= DVZ_SCENE_MAX_HOVERS)
    {
        log_error("maximum hover count reached");
        return NULL;
    }
    DvzHover* hover = &scene->hovers[scene->hover_count++];
    dvz_memset(hover, sizeof(DvzHover), 0, sizeof(DvzHover));
    hover->scene = scene;
    hover->desc = desc != NULL ? *desc : dvz_hover_desc();
    if (hover->desc.hit_policy == 0)
        hover->desc.hit_policy = DVZ_QUERY_HIT_FRONTMOST;
    hover->visual_style = dvz_item_state_visual_style();
    return hover;
}



/**
 * Destroy a retained hover object.
 *
 * @param hover the hover
 */
void dvz_hover_destroy(DvzHover* hover)
{
    if (hover == NULL)
        return;
    DvzScene* scene = hover->scene;
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->item_interaction_count; i++)
        {
            if (scene->item_interactions[i].hover == hover)
                scene->item_interactions[i].hover = NULL;
        }
    }
    hover->scene = NULL;
    hover->has_item = false;
    hover->item = (DvzSelectionItem){0};
    if (scene != NULL)
        (void)_item_state_sync_scene(scene, "clear hover item_state");
}



/**
 * Clear the hovered item.
 *
 * @param hover the hover
 */
void dvz_hover_clear(DvzHover* hover)
{
    ANN(hover);
    DvzScene* scene = hover->scene;
    hover->has_item = false;
    hover->item = (DvzSelectionItem){0};
    if (scene != NULL)
        (void)_item_state_sync_scene(scene, "clear hover item_state");
}



/**
 * Configure visual styling for a retained hover object.
 *
 * @param hover the hover
 * @param style style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_hover_set_visual_style(DvzHover* hover, const DvzItemStateVisualStyle* style)
{
    ANN(hover);
    if (!_item_state_visual_style_abi_validate(style))
        return -1;
    DvzItemStateVisualStyle resolved = style != NULL ? *style : dvz_item_state_visual_style();
    const uint32_t supported_flags =
        DVZ_ITEM_STATE_VISUAL_ALPHA | DVZ_ITEM_STATE_VISUAL_TINT | DVZ_ITEM_STATE_VISUAL_SCALE;
    if ((resolved.visual_flags & ~supported_flags) != 0)
    {
        log_error("unsupported hover visual style flags");
        return -1;
    }
    if (!isfinite(resolved.alpha) || !isfinite(resolved.tint_mix) || !isfinite(resolved.scale))
    {
        log_error("hover visual alpha, tint_mix, and scale values must be finite");
        return -1;
    }

    hover->visual_style = resolved;
    return hover->scene != NULL ? _item_state_sync_scene(hover->scene, "update hover style") : 0;
}



/**
 * Apply one resolved query to a retained hover object.
 *
 * @param hover the hover
 * @param query query result
 * @return 0 on success, -1 on error
 */
int dvz_hover_apply_query(DvzHover* hover, const DvzQueryResult* query)
{
    ANN(hover);
    ANN(query);
    if (hover->scene == NULL)
        return -1;
    if (!query->hit || query->resolved_target == DVZ_SCENE_TARGET_NONE)
    {
        hover->has_item = false;
        hover->item = (DvzSelectionItem){0};
        return _item_state_sync_scene(hover->scene, "clear hover item_state");
    }
    DvzSceneTargetKind target = hover->desc.target;
    if (target != DVZ_SCENE_TARGET_NONE && target != query->resolved_target)
        return -1;

    hover->item = (DvzSelectionItem){
        .visual_id = query->visual_id,
        .target = query->resolved_target,
        .target_id = query->resolved_id,
        .link_key = query->link_key,
    };
    hover->has_item = true;
    return _item_state_sync_scene(hover->scene, "update hover item_state");
}



/*************************************************************************************************/
/*  Item interaction                                                                             */
/*************************************************************************************************/

/**
 * Return the default item interaction descriptor.
 *
 * @return the default descriptor
 */
DvzItemInteractionDesc dvz_item_interaction_desc(void)
{
    return (DvzItemInteractionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzItemInteractionDesc),
        .hover_enabled = true,
        .selection_enabled = true,
        .select_mode = DVZ_SELECT_TOGGLE,
        .target = DVZ_SCENE_TARGET_ITEM,
        .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        .clear_hover_on_miss = true,
        .clear_selection_on_miss = true,
    };
}



/**
 * Create a panel-bound item interaction controller.
 *
 * @param panel the panel
 * @param desc descriptor, or NULL for defaults
 * @return the item interaction, or NULL on error
 */
DvzItemInteraction* dvz_item_interaction(DvzPanel* panel, const DvzItemInteractionDesc* desc)
{
    ANN(panel);
    if (!_item_interaction_desc_validate(desc))
        return NULL;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (panel->item_interaction != NULL && panel->item_interaction->active)
    {
        log_error("panel already has an item interaction controller");
        return NULL;
    }

    DvzScene* scene = panel->figure->scene;
    if (scene->item_interaction_count >= DVZ_SCENE_MAX_ITEM_INTERACTIONS)
    {
        log_error("maximum item interaction count reached");
        return NULL;
    }

    DvzItemInteractionDesc resolved = _item_interaction_resolve_desc(desc);
    DvzItemInteraction* interaction = &scene->item_interactions[scene->item_interaction_count++];
    dvz_memset(interaction, sizeof(DvzItemInteraction), 0, sizeof(DvzItemInteraction));
    interaction->scene = scene;
    interaction->panel = panel;
    interaction->desc = resolved;
    interaction->active = true;

    if (resolved.hover_enabled)
    {
        if (resolved.hover != NULL)
        {
            if (resolved.hover->scene != scene)
            {
                log_error("cannot bind a hover object from a different scene");
                interaction->active = false;
                return NULL;
            }
            interaction->hover = resolved.hover;
        }
        else
        {
            DvzHoverDesc hover_desc = dvz_hover_desc();
            hover_desc.target = resolved.target;
            hover_desc.hit_policy = resolved.hit_policy;
            interaction->hover = dvz_hover(scene, &hover_desc);
            interaction->owns_hover = interaction->hover != NULL;
        }
        if (interaction->hover == NULL)
        {
            interaction->active = false;
            return NULL;
        }
    }

    if (resolved.selection_enabled)
    {
        if (resolved.selection != NULL)
        {
            if (resolved.selection->scene != scene)
            {
                log_error("cannot bind a selection object from a different scene");
                if (interaction->owns_hover)
                    dvz_hover_destroy(interaction->hover);
                interaction->active = false;
                return NULL;
            }
            interaction->selection = resolved.selection;
        }
        else
        {
            DvzSelectionDesc selection_desc = dvz_selection_desc();
            selection_desc.mode = resolved.select_mode;
            selection_desc.target = resolved.target;
            interaction->selection = dvz_selection(scene, &selection_desc);
            interaction->owns_selection = interaction->selection != NULL;
        }
        if (interaction->selection == NULL)
        {
            if (interaction->owns_hover)
                dvz_hover_destroy(interaction->hover);
            interaction->active = false;
            return NULL;
        }
    }

    panel->item_interaction = interaction;
    return interaction;
}



/**
 * Destroy a panel-bound item interaction controller.
 *
 * @param interaction the item interaction
 */
void dvz_item_interaction_destroy(DvzItemInteraction* interaction)
{
    if (interaction == NULL || !interaction->active)
        return;
    if (interaction->panel != NULL && interaction->panel->item_interaction == interaction)
        interaction->panel->item_interaction = NULL;
    if (interaction->owns_hover)
        dvz_hover_destroy(interaction->hover);
    if (interaction->owns_selection)
        dvz_selection_destroy(interaction->selection);
    interaction->scene = NULL;
    interaction->panel = NULL;
    interaction->desc = (DvzItemInteractionDesc){0};
    interaction->hover = NULL;
    interaction->selection = NULL;
    interaction->owns_hover = false;
    interaction->owns_selection = false;
    interaction->active = false;
}



/**
 * Return the hover object used by an item interaction controller.
 *
 * @param interaction the item interaction
 * @return the hover, or NULL
 */
DvzHover* dvz_item_interaction_hover(DvzItemInteraction* interaction)
{
    return interaction != NULL && interaction->active ? interaction->hover : NULL;
}



/**
 * Return the selection object used by an item interaction controller.
 *
 * @param interaction the item interaction
 * @return the selection, or NULL
 */
DvzSelection* dvz_item_interaction_selection(DvzItemInteraction* interaction)
{
    return interaction != NULL && interaction->active ? interaction->selection : NULL;
}



/**
 * Apply a panel-local pointer event to an item interaction controller.
 *
 * @param interaction the item interaction
 * @param ev panel-local pointer event
 * @return true when a query was queued
 */
bool _scene_item_interaction_pointer(DvzItemInteraction* interaction, const DvzPointerEvent* ev)
{
    if (interaction == NULL || ev == NULL || !interaction->active)
        return false;
    if (ev->type == DVZ_POINTER_EVENT_MOVE && interaction->desc.hover_enabled &&
        interaction->hover != NULL)
    {
        return _item_interaction_queue_query(
                   interaction, ev->pos[0], ev->pos[1], DVZ_ITEM_INTERACTION_QUERY_HOVER) == 0;
    }
    if (ev->type == DVZ_POINTER_EVENT_CLICK && ev->button == DVZ_POINTER_BUTTON_LEFT &&
        interaction->desc.selection_enabled && interaction->selection != NULL)
    {
        return _item_interaction_queue_query(
                   interaction, ev->pos[0], ev->pos[1],
                   DVZ_ITEM_INTERACTION_QUERY_SELECTION) == 0;
    }
    return false;
}



/**
 * Clear transient hover state when the pointer leaves the panel.
 *
 * @param interaction the item interaction
 */
void _scene_item_interaction_pointer_leave(DvzItemInteraction* interaction)
{
    if (interaction == NULL || !interaction->active || !interaction->desc.clear_hover_on_miss ||
        interaction->hover == NULL)
    {
        return;
    }
    dvz_hover_clear(interaction->hover);
}



/**
 * Apply one resolved interaction-owned query result.
 *
 * @param interaction the item interaction
 * @param query_kind internal query kind
 * @param query the query result
 */
void _scene_item_interaction_apply_query_result(
    DvzItemInteraction* interaction, uint32_t query_kind, const DvzQueryResult* query)
{
    if (interaction == NULL || !interaction->active || query == NULL)
        return;

    bool hit = query->hit && query->resolved_target != DVZ_SCENE_TARGET_NONE;
    if (query_kind == DVZ_ITEM_INTERACTION_QUERY_HOVER)
    {
        if (interaction->hover == NULL || !interaction->desc.hover_enabled)
            return;
        if (hit || interaction->desc.clear_hover_on_miss)
            (void)dvz_hover_apply_query(interaction->hover, query);
    }
    else if (query_kind == DVZ_ITEM_INTERACTION_QUERY_SELECTION)
    {
        if (interaction->selection == NULL || !interaction->desc.selection_enabled)
            return;
        if (hit)
            (void)dvz_selection_apply_query(interaction->selection, query);
        else if (interaction->desc.clear_selection_on_miss)
            dvz_selection_clear(interaction->selection);
    }
}



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
        DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardStyle),
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


DvzOverlayCardDesc dvz_overlay_card_desc(void)
{
    return (DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc)};
}



DvzOverlayRichTextDesc dvz_overlay_rich_text_desc(void)
{
    return (DvzOverlayRichTextDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayRichTextDesc)};
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
    if (!_overlay_card_desc_validate(desc))
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
    card->flags = desc != NULL ? desc->card_flags : 0;
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
    _scene_text_block_destroy(&card->rich_block);
    card->scene = NULL;
    card->overlay = NULL;
    card->panel = NULL;
    card->flags = 0;
    card->active = false;
    card->card.panel = NULL;
    card->card.text[0] = '\0';
    card->card.realized_text[0] = '\0';
    card->card.dirty = false;
    card->rich_enabled = false;
    card->rich_dirty = false;
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
    if (!_overlay_card_style_validate(style))
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
    if (card->rich_enabled)
    {
        _scene_text_block_destroy(&card->rich_block);
        card->rich_enabled = false;
        card->rich_dirty = false;
    }
    card->card.content = DVZ_SCENE_CARD_CONTENT_TEXT;
    if (text != NULL)
        dvz_strlcpy(card->card.text, text, sizeof(card->card.text));
    else
        card->card.text[0] = '\0';
    card->card.dirty = true;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}


/**
 * Set rich text displayed in an overlay card.
 *
 * @param card the card
 * @param desc rich text descriptor
 * @return 0 on success, -1 on error
 */
int dvz_overlay_card_set_rich_text(DvzOverlayCard* card, const DvzOverlayRichTextDesc* desc)
{
    ANN(card);
    if (!card->active || desc == NULL || desc->source == NULL)
        return -1;
    if (!_overlay_rich_text_desc_validate(desc))
        return -1;

    if (card->rich_enabled)
        _scene_text_block_set_source(&card->rich_block, desc->source);
    else
        _scene_text_block_init(&card->rich_block, desc->source);

    card->rich_layout = (DvzTextBlockLayout){
        .scene = card->scene,
        .font_size_px = desc->line_height_px > 0.0f ? 0.78f * desc->line_height_px : 11.0f,
        .max_width_px = desc->max_width_px > 0.0f ? desc->max_width_px : 220.0f,
        .char_width_px = desc->char_width_px > 0.0f ? desc->char_width_px : 7.0f,
        .line_height_px = desc->line_height_px > 0.0f ? desc->line_height_px : 14.0f,
        .padding_px = {0.0f, 0.0f},
    };

    DvzColor text_color = desc->text_color;
    if (text_color.a == 0)
        text_color = card->card.text_color;
    card->rich_raster = (DvzTextBlockRasterDesc){
        .scene = card->scene,
        .text_color = text_color,
        .background_color = desc->background_color,
        .font_size_px = card->rich_layout.font_size_px,
        .scale = desc->scale > 0.0f ? desc->scale : 1.0f,
    };

    card->rich_enabled = true;
    card->rich_dirty = true;
    card->card.content = DVZ_SCENE_CARD_CONTENT_IMAGE;
    card->card.dirty = true;
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
    return 0;
}


/**
 * Clear rich text content and return the card to the plain GPU text path.
 *
 * @param card the card
 */
void dvz_overlay_card_clear_rich_text(DvzOverlayCard* card)
{
    ANN(card);
    if (!card->active)
        return;
    _scene_text_block_destroy(&card->rich_block);
    card->rich_enabled = false;
    card->rich_dirty = false;
    card->card.content = DVZ_SCENE_CARD_CONTENT_TEXT;
    card->card.content_size_px[0] = 0.0f;
    card->card.content_size_px[1] = 0.0f;
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
    {
        _scene_card_hide(&card->card);
        _overlay_card_hide_rich(card);
    }
    _scene_notify_request_frame(card->panel != NULL ? card->panel->figure : NULL);
}



/*************************************************************************************************/
/*  Pinned readouts                                                                              */
/*************************************************************************************************/

/**
 * Create a pinned readout from a resolved query result.
 *
 * @param panel the panel
 * @param query the query result
 * @return the pinned readout, or NULL on allocation failure
 */
DvzPinnedReadout* dvz_pinned_readout_query(DvzPanel* panel, const DvzQueryResult* query)
{
    ANN(panel);
    ANN(query);
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
    readout->query = *query;
    _readout_refresh_text(readout);
    _scene_card_init(&readout->card, panel);
    dvz_strlcpy(readout->card.text, readout->text, sizeof(readout->card.text));
    readout->card.anchor_px[0] = (float)query->panel_position[0];
    readout->card.anchor_px[1] = (float)query->panel_position[1];
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
    if (!_scene_format_desc_validate(format))
        return;
    readout->has_format = format != NULL;
    _scene_format_state_copy(&readout->format, format);
    _readout_refresh_text(readout);
    dvz_strlcpy(readout->card.text, readout->text, sizeof(readout->card.text));
    readout->card.dirty = true;
    _scene_notify_request_frame(readout->panel != NULL ? readout->panel->figure : NULL);
}
