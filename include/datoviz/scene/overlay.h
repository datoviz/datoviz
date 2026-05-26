/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene overlays                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Overlay cards                                                                                */
/*************************************************************************************************/

typedef enum
{
    DVZ_OVERLAY_CARD_HIDDEN = 0x01u,
} DvzOverlayCardFlag;


typedef enum
{
    DVZ_OVERLAY_CARD_PLACEMENT_PIXEL = 0,
    DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT,
    DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT,
    DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT,
    DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT,
    DVZ_OVERLAY_CARD_PLACEMENT_CENTER,
} DvzOverlayCardPlacement;


typedef struct DvzOverlayCardStyle
{
    DvzColor background_color;
    DvzColor text_color;
    float padding_px[2];
    float min_width_px;
    float height_px;
    float glyph_advance_px;
    float text_size_px;
    DvzTextRenderer text_renderer;
    uint32_t max_text_chars;
} DvzOverlayCardStyle;


typedef struct DvzOverlayCardDesc
{
    const char* text;
    DvzOverlayCardPlacement placement;
    float anchor_px[2];
    float offset_px[2];
    const DvzOverlayCardStyle* style;
    uint32_t flags;
} DvzOverlayCardDesc;


typedef struct DvzOverlayRichTextDesc
{
    const char* source;
    float max_width_px;
    float char_width_px;
    float line_height_px;
    float scale;
    DvzColor text_color;
    DvzColor background_color;
} DvzOverlayRichTextDesc;


/**
 * Create a panel overlay object.
 *
 * @param panel the panel
 * @param flags overlay flags, currently zero
 * @return the overlay, or NULL on error
 */
DVZ_EXPORT DvzOverlay* dvz_overlay(DvzPanel* panel, uint32_t flags);


/**
 * Destroy a panel overlay object and hide its cards.
 *
 * @param overlay the overlay
 */
DVZ_EXPORT void dvz_overlay_destroy(DvzOverlay* overlay);


/**
 * Return the default overlay-card style.
 *
 * @return the default style descriptor
 */
DVZ_EXPORT DvzOverlayCardStyle dvz_overlay_card_style(void);


/**
 * Create a card attached to a panel overlay.
 *
 * @param overlay the overlay
 * @param desc card descriptor, or NULL for defaults
 * @return the card, or NULL on error
 */
DVZ_EXPORT DvzOverlayCard* dvz_overlay_card(
    DvzOverlay* overlay, const DvzOverlayCardDesc* desc);


/**
 * Destroy an overlay card.
 *
 * @param card the card
 */
DVZ_EXPORT void dvz_overlay_card_destroy(DvzOverlayCard* card);


/**
 * Set an overlay card style.
 *
 * @param card the card
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_overlay_card_set_style(
    DvzOverlayCard* card, const DvzOverlayCardStyle* style);


/**
 * Set the text displayed in an overlay card.
 *
 * @param card the card
 * @param text the text, or NULL to clear it
 */
DVZ_EXPORT void dvz_overlay_card_set_text(DvzOverlayCard* card, const char* text);


/**
 * Set rich text displayed in an overlay card.
 *
 * @param card the card
 * @param desc rich text descriptor
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_overlay_card_set_rich_text(
    DvzOverlayCard* card, const DvzOverlayRichTextDesc* desc);


/**
 * Clear rich text content and return the card to the plain GPU text path.
 *
 * @param card the card
 */
DVZ_EXPORT void dvz_overlay_card_clear_rich_text(DvzOverlayCard* card);


/**
 * Set the panel-local pixel layout of an overlay card.
 *
 * @param card the card
 * @param anchor_px panel-local anchor in logical pixels, or NULL to keep it unchanged
 * @param offset_px offset from the anchor in logical pixels, or NULL to keep it unchanged
 */
DVZ_EXPORT void dvz_overlay_card_set_layout(
    DvzOverlayCard* card, const float anchor_px[2], const float offset_px[2]);


/**
 * Set semantic placement for an overlay card.
 *
 * @param card the card
 * @param placement semantic placement mode
 * @param offset_px inward/relative offset in logical pixels, or NULL to keep it unchanged
 */
DVZ_EXPORT void dvz_overlay_card_set_placement(
    DvzOverlayCard* card, DvzOverlayCardPlacement placement, const float offset_px[2]);


/**
 * Show or hide an overlay card.
 *
 * @param card the card
 * @param visible whether the card should be visible
 */
DVZ_EXPORT void dvz_overlay_card_set_visible(DvzOverlayCard* card, bool visible);

EXTERN_C_OFF
