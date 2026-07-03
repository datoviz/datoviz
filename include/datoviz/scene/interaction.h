/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene interaction                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/drp2/types.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Interaction policy                                                                           */
/*************************************************************************************************/

/**
 * Create an interaction policy object owned by the scene.
 *
 * @param scene the scene
 * @return the interaction policy
 */
DVZ_EXPORT DvzInteractionPolicy* dvz_interaction(DvzScene* scene);


/**
 * Destroy an interaction policy object.
 *
 * @param interaction the interaction policy
 */
DVZ_EXPORT void dvz_interaction_destroy(DvzInteractionPolicy* interaction);


/**
 * Bind an interaction policy to a panel.
 *
 * @param interaction the interaction policy
 * @param panel the panel
 */
DVZ_EXPORT void dvz_interaction_bind_panel(DvzInteractionPolicy* interaction, DvzPanel* panel);


/**
 * Attach a retained selection object to an interaction policy.
 *
 * @param interaction the interaction policy
 * @param selection the selection
 */
DVZ_EXPORT void dvz_interaction_set_selection(
    DvzInteractionPolicy* interaction, DvzSelection* selection);


/**
 * Set the active link channel used by an interaction policy.
 *
 * @param interaction the interaction policy
 * @param channel the link channel
 */
DVZ_EXPORT void dvz_interaction_set_link_channel(
    DvzInteractionPolicy* interaction, DvzLinkChannel* channel);


/**
 * Set the hit-selection policy used for panel queries.
 *
 * @param interaction the interaction policy
 * @param policy the hit-selection policy
 */
DVZ_EXPORT void dvz_interaction_set_query_hit_policy(
    DvzInteractionPolicy* interaction, DvzQueryHitPolicy policy);


/**
 * Enable or disable automatic readout pinning from interaction-driven query results.
 *
 * Invalid interaction handles are ignored.
 *
 * @param interaction the interaction policy
 * @param enabled true to enable automatic pinning
 */
DVZ_EXPORT void dvz_interaction_set_auto_pin_readout(
    DvzInteractionPolicy* interaction, bool enabled);



/*************************************************************************************************/
/*  Visual interaction capabilities                                                              */
/*************************************************************************************************/

/**
 * Declare the query capabilities exposed by a visual.
 *
 * @param visual the visual
 * @param capabilities bitwise OR of DvzQueryCapabilityFlag values
 */
DVZ_EXPORT void dvz_visual_set_query_capabilities(DvzVisual* visual, uint32_t capabilities);


/**
 * Bind per-item link keys for a visual on one link channel.
 *
 * `link_keys` must contain `item_count` entries and must not be NULL unless `item_count` is zero.
 * The keys are copied before return. Passing zero entries clears the binding for this channel.
 *
 * @param visual the visual
 * @param channel the link channel
 * @param link_keys array of link keys
 * @param item_count number of keys
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_visual_set_link_keys(
    DvzVisual* visual, DvzLinkChannel* channel, const uint64_t* link_keys, uint32_t item_count);



/*************************************************************************************************/
/*  Link channels                                                                                */
/*************************************************************************************************/

/**
 * Create a scene-owned link channel.
 *
 * @param scene the scene
 * @param name stable channel name, or NULL
 * @return the link channel
 */
DVZ_EXPORT DvzLinkChannel* dvz_link_channel(DvzScene* scene, const char* name);


/**
 * Destroy a link channel.
 *
 * @param channel the link channel
 */
DVZ_EXPORT void dvz_link_channel_destroy(DvzLinkChannel* channel);



/*************************************************************************************************/
/*  Selection                                                                                   */
/*************************************************************************************************/

/**
 * Return the default selection descriptor.
 *
 * @return the default selection descriptor
 */
DVZ_EXPORT DvzSelectionDesc dvz_selection_desc(void);


/**
 * Return the default item-state visual style.
 *
 * @return the default item-state visual style
 */
DVZ_EXPORT DvzItemStateVisualStyle dvz_item_state_visual_style(void);


/**
 * Create a retained selection object.
 *
 * @param scene the scene
 * @param desc selection descriptor, or NULL for defaults
 * @return the selection
 */
DVZ_EXPORT DvzSelection* dvz_selection(DvzScene* scene, const DvzSelectionDesc* desc);


/**
 * Destroy a selection object.
 *
 * @param selection the selection
 */
DVZ_EXPORT void dvz_selection_destroy(DvzSelection* selection);


/**
 * Clear the contents of a selection object.
 *
 * @param selection the selection
 */
DVZ_EXPORT void dvz_selection_clear(DvzSelection* selection);


/**
 * Return the default selection visual style.
 *
 * The default preserves the first retained-selection behavior: selected items render normally and
 * unselected point-like items are dimmed while a selection is active.
 *
 * @return the default visual style descriptor
 */
DVZ_EXPORT DvzSelectionVisualStyle dvz_selection_visual_style(void);


/**
 * Configure selected/unselected visual styling for retained point-like item states.
 *
 * The initial implementation affects point, pixel, and marker visuals. Pass NULL to restore
 * defaults.
 *
 * @param selection the selection
 * @param style the visual style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_selection_set_visual_style(
    DvzSelection* selection, const DvzSelectionVisualStyle* style);


/**
 * Apply one query result to a selection object.
 *
 * @param selection the selection
 * @param query the query result
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_selection_apply_query(
    DvzSelection* selection, const DvzQueryResult* query);


/**
 * Return the number of resolved targets currently stored in a selection object.
 *
 * @param selection the selection
 * @return the item count
 */
DVZ_EXPORT uint32_t dvz_selection_count(const DvzSelection* selection);


/**
 * Copy resolved selection contents into caller-owned storage.
 *
 * Writes at most `max_items` entries. Use `dvz_selection_count()` first to detect whether the
 * output buffer is large enough; extra entries are not copied.
 *
 * @param selection the selection
 * @param items output item array
 * @param max_items maximum number of items to write
 */
DVZ_EXPORT void dvz_selection_copy(
    const DvzSelection* selection, DvzSelectionItem* items, uint32_t max_items);


/*************************************************************************************************/
/*  Hover                                                                                        */
/*************************************************************************************************/

/**
 * Return the default hover descriptor.
 *
 * @return the default hover descriptor
 */
DVZ_EXPORT DvzHoverDesc dvz_hover_desc(void);


/**
 * Create a retained hover object.
 *
 * @param scene the scene
 * @param desc hover descriptor, or NULL for defaults
 * @return the hover object
 */
DVZ_EXPORT DvzHover* dvz_hover(DvzScene* scene, const DvzHoverDesc* desc);


/**
 * Destroy a hover object.
 *
 * @param hover the hover object
 */
DVZ_EXPORT void dvz_hover_destroy(DvzHover* hover);


/**
 * Clear the hovered item.
 *
 * @param hover the hover object
 */
DVZ_EXPORT void dvz_hover_clear(DvzHover* hover);


/**
 * Configure hover visual styling.
 *
 * The initial implementation affects point, pixel, and marker visuals. Pass NULL to restore
 * defaults.
 *
 * @param hover the hover object
 * @param style the visual style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_hover_set_visual_style(DvzHover* hover, const DvzItemStateVisualStyle* style);


/**
 * Apply one query result to a hover object.
 *
 * @param hover the hover object
 * @param query the query result
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_hover_apply_query(DvzHover* hover, const DvzQueryResult* query);



/*************************************************************************************************/
/*  Item interaction                                                                             */
/*************************************************************************************************/

/**
 * Return the default item interaction descriptor.
 *
 * @return the default item interaction descriptor
 */
DVZ_EXPORT DvzItemInteractionDesc dvz_item_interaction_desc(void);


/**
 * Create a panel-bound item interaction controller.
 *
 * @param panel the panel
 * @param desc interaction descriptor, or NULL for defaults
 * @return the item interaction controller
 */
DVZ_EXPORT DvzItemInteraction* dvz_item_interaction(
    DvzPanel* panel, const DvzItemInteractionDesc* desc);


/**
 * Destroy a panel-bound item interaction controller.
 *
 * @param interaction the item interaction controller
 */
DVZ_EXPORT void dvz_item_interaction_destroy(DvzItemInteraction* interaction);


/**
 * Return the hover object used by an item interaction controller.
 *
 * @param interaction the item interaction controller
 * @return the hover object, or NULL when hover is disabled
 */
DVZ_EXPORT DvzHover* dvz_item_interaction_hover(DvzItemInteraction* interaction);


/**
 * Return the selection object used by an item interaction controller.
 *
 * @param interaction the item interaction controller
 * @return the selection object, or NULL when selection is disabled
 */
DVZ_EXPORT DvzSelection* dvz_item_interaction_selection(DvzItemInteraction* interaction);



/*************************************************************************************************/
/*  Query requests                                                                               */
/*************************************************************************************************/

/**
 * Return the default query request descriptor.
 *
 * @return default query request descriptor
 */
DVZ_EXPORT DvzQueryRequest dvz_query_request(void);


/**
 * Queue an explicit GPU-backed query request on a panel.
 *
 * @param panel the panel
 * @param x the panel-local logical x coordinate, origin at the outer panel rectangle
 * @param y the panel-local logical y coordinate, origin at the outer panel rectangle
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_panel_query(
    DvzPanel* panel, double x, double y, const DvzQueryRequest* request);


/**
 * Poll one resolved query result from the scene.
 *
 * @param scene the scene
 * @param out_result output result
 * @return true when a result was written
 */
DVZ_EXPORT bool dvz_scene_poll_query(DvzScene* scene, DvzQueryResult* out_result);


/**
 * Queue and synchronously resolve a query through a DRP2 runtime.
 *
 * @param panel the panel
 * @param runtime the DRP2 runtime
 * @param x the panel-local logical x coordinate, origin at the outer panel rectangle
 * @param y the panel-local logical y coordinate, origin at the outer panel rectangle
 * @param request the request descriptor, or NULL for defaults
 * @param out_result output result
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_panel_query_now(
    DvzPanel* panel, DvzDrp2Runtime* runtime, double x, double y, const DvzQueryRequest* request,
    DvzQueryResult* out_result);



/**
 * Return the retained hover state for one panel.
 *
 * The returned pointer is borrowed scene state. It remains valid until the next hover/query update
 * for the panel or scene destruction and must not be mutated or retained.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the hover state, or NULL
 */
DVZ_EXPORT const DvzHoverState* dvz_scene_hover(const DvzScene* scene, const DvzPanel* panel);



/*************************************************************************************************/
/*  Pinned readouts                                                                              */
/*************************************************************************************************/

/**
 * Create a pinned readout from a resolved query result.
 *
 * @param panel the panel
 * @param query the query result
 * @return the pinned readout
 */
DVZ_EXPORT DvzPinnedReadout* dvz_pinned_readout_query(
    DvzPanel* panel, const DvzQueryResult* query);


/**
 * Destroy a pinned readout object.
 *
 * @param readout the pinned readout
 */
DVZ_EXPORT void dvz_pinned_readout_destroy(DvzPinnedReadout* readout);


/**
 * Override formatting for a pinned readout.
 *
 * @param readout the pinned readout
 * @param format the format descriptor, or NULL to clear the override
 */
DVZ_EXPORT void dvz_pinned_readout_set_format(
    DvzPinnedReadout* readout, const DvzFormatDesc* format);


EXTERN_C_OFF
