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
 * Set the hit-selection policy used for picking.
 *
 * @param interaction the interaction policy
 * @param policy the hit-selection policy
 */
DVZ_EXPORT void dvz_interaction_set_pick_hit_policy(
    DvzInteractionPolicy* interaction, DvzPickHitPolicy policy);


/**
 * Enable or disable automatic probe pinning from interaction-driven probe results.
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
 * Declare the picking capabilities exposed by a visual.
 *
 * @param visual the visual
 * @param capabilities bitwise OR of DvzPickCapabilityFlag values
 */
DVZ_EXPORT void dvz_visual_set_pick_capabilities(DvzVisual* visual, uint32_t capabilities);


/**
 * Bind per-item link keys for a visual on one link channel.
 *
 * @param visual the visual
 * @param channel the link channel
 * @param link_keys array of link keys
 * @param item_count number of keys
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_link_keys(
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
 * Apply one pick result to a selection object.
 *
 * @param selection the selection
 * @param pick the pick result
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_selection_apply_pick(
    DvzSelection* selection, const DvzPickResult* pick);


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
 * @param selection the selection
 * @param items output item array
 * @param max_items maximum number of items to write
 */
DVZ_EXPORT void dvz_selection_copy(
    const DvzSelection* selection, DvzSelectionItem* items, uint32_t max_items);



/*************************************************************************************************/
/*  Query requests                                                                               */
/*************************************************************************************************/

/**
 * Queue an explicit GPU-backed query request on a panel.
 *
 * @param panel the panel
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_query(
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
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @param out_result output result
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_query_now(
    DvzPanel* panel, DvzDrp2Runtime* runtime, double x, double y, const DvzQueryRequest* request,
    DvzQueryResult* out_result);



/*************************************************************************************************/
/*  Transitional pick and probe requests                                                         */
/*************************************************************************************************/

/**
 * Queue an explicit pick request on a panel.
 *
 * Freshness rules for the current v0.4 slice:
 * non-zero `request_id` values supersede older pick work on the same panel with the same id,
 * while zero-id requests use one latest-request-wins scope within the panel pick stream. Once a
 * newer request claims that scope, late older results are discarded even if the newer result was
 * already polled.
 *
 * @param panel the panel
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_pick(
    DvzPanel* panel, double x, double y, const DvzPickRequest* request);


/**
 * Queue an explicit probe request on a panel.
 *
 * Freshness rules for the current v0.4 slice:
 * non-zero `request_id` values supersede older probe work on the same panel with the same id,
 * while zero-id requests use one latest-request-wins scope within the panel probe stream. Once a
 * newer request claims that scope, late older results are discarded even if the newer result was
 * already polled.
 *
 * @param panel the panel
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_probe(
    DvzPanel* panel, double x, double y, const DvzProbeRequest* request);


/**
 * Poll one resolved pick result from the scene.
 *
 * @param scene the scene
 * @param out_result output result
 * @return true when a result was written
 */
DVZ_EXPORT bool dvz_scene_poll_pick(DvzScene* scene, DvzPickResult* out_result);


/**
 * Poll one resolved probe result from the scene.
 *
 * @param scene the scene
 * @param out_result output result
 * @return true when a result was written
 */
DVZ_EXPORT bool dvz_scene_poll_probe(DvzScene* scene, DvzProbeResult* out_result);


/**
 * Return the retained hover state for one panel.
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
 * Create a pinned readout from a resolved probe result.
 *
 * @param panel the panel
 * @param probe the probe result
 * @return the pinned readout
 */
DVZ_EXPORT DvzPinnedReadout* dvz_pinned_readout(DvzPanel* panel, const DvzProbeResult* probe);


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
