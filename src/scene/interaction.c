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

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"



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
        _selection_clear_items(selection);
        selection->items[0] = item;
        selection->item_count = 1;
        return _selection_sync_masks(selection);
    case DVZ_SELECT_ADDITIVE:
        if (present)
            return 0;
        break;
    case DVZ_SELECT_SUBTRACT:
        if (present)
            _scene_remove_selection_item(selection, &item);
        return _selection_sync_masks(selection);
    case DVZ_SELECT_TOGGLE:
        if (present)
        {
            _scene_remove_selection_item(selection, &item);
            return _selection_sync_masks(selection);
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
    return _selection_sync_masks(selection);
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
    panel->pinned_readouts[panel->pinned_readout_count++] = readout;
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
}
