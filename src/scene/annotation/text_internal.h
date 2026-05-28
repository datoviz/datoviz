/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text annotation internals                                                              */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

#define DVZ_TEXT_BITMAP_GLYPH_WIDTH  6u
#define DVZ_TEXT_BITMAP_GLYPH_HEIGHT 8u
#define DVZ_TEXT_BITMAP_LINE_HEIGHT  9u
#define DVZ_TEXT_BITMAP_FIRST_CHAR   32u
#define DVZ_TEXT_BITMAP_GLYPH_COUNT  96u
#define DVZ_TEXT_BITMAP_FALLBACK     63u
#define DVZ_TEXT_BITMAP_ATLAS_COLS   16u
#define DVZ_TEXT_BITMAP_ATLAS_ROWS   6u
#define DVZ_TEXT_BITMAP_ATLAS_PAD    1u
#define DVZ_TEXT_BITMAP_ATLAS_CELL_W (DVZ_TEXT_BITMAP_GLYPH_WIDTH + 2u * DVZ_TEXT_BITMAP_ATLAS_PAD)
#define DVZ_TEXT_BITMAP_ATLAS_CELL_H (DVZ_TEXT_BITMAP_GLYPH_HEIGHT + 2u * DVZ_TEXT_BITMAP_ATLAS_PAD)

float _text_style_size_px(const DvzTextStyle* style, float fallback_size_px);

DvzTextAtlasBackend _text_renderer_backend(
    DvzTextRenderer renderer, const DvzTextStyle* style, float fallback_size_px);

void _text_style_color(const DvzTextStyle* style, uint8_t out_color[4]);

bool _text_utf8_next(const char* string, uint32_t* inout_index, uint32_t* out_codepoint);

void _text_measure_cells(
    const char* string, uint32_t* out_columns, uint32_t* out_lines, uint32_t* out_visible);

float _text_bitmap_layout_scale(const DvzTextStyle* style);

DvzSampledField* _text_bitmap_atlas_field(DvzScene* scene);

void _text_bitmap_atlas_uv(uint32_t ascii, float out[4]);

float _text_sdf_layout_scale(const DvzTextStyle* style, const DvzTextAtlas* atlas);

void _text_sdf_measure(
    const char* string, DvzTextAtlas* atlas, float scale, float* out_width, float* out_height,
    uint32_t* out_visible);

void _text_pixel_to_clip(const DvzFigure* figure, float x, float y, float z, float out[3]);

void _text_panel_pixel_to_clip(
    const DvzPanel* panel, float x, float y, float z, float out[3]);

void _text_placement_alignment(
    const DvzTextPlacement* placement, float width, float height, float* out_x, float* out_y);

bool _text_visual_prepare(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual);

bool _scalebar_prepare_visual(DvzFigure* figure, DvzAnnotation* annotation);

DvzFont* _text_sdf_font(DvzScene* scene, const DvzTextStyle* style);

DvzTextAtlas* _text_font_atlas(DvzFont* font, const DvzTextAtlasSpec* spec);

uint64_t _text_scene_font_version_sum(const DvzScene* scene);
