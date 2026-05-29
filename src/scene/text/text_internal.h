/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text internals                                                                         */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

void _scene_text_block_init(DvzTextBlock* block, const char* source);

void _scene_text_block_set_source(DvzTextBlock* block, const char* source);

void _scene_text_block_destroy(DvzTextBlock* block);

int _scene_text_block_parse(DvzTextBlock* block);

int _scene_text_block_measure(DvzTextBlock* block, const DvzTextBlockLayout* layout);

int _scene_text_block_rasterize(DvzTextBlock* block, const DvzTextBlockRasterDesc* desc);

int _scene_text_block_realize_image(
    DvzTextBlock* block, DvzPanel* panel, const DvzTextBlockImageDesc* desc);

EXTERN_C_ON

DvzTextAtlasSpec _scene_text_atlas_spec(DvzTextAtlasBackend backend, float size_px);

DvzTextAtlas* _scene_text_atlas_get(DvzFont* font, const DvzTextAtlasSpec* spec);

bool _scene_text_atlas_ensure(DvzFont* font, const DvzTextAtlasSpec* spec);

bool _scene_text_atlas_ensure_string(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* string);

bool _scene_text_atlas_ensure_strings(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* const* strings, uint32_t count);

DvzTextAtlasGlyph* _scene_text_atlas_glyph(DvzTextAtlas* atlas, uint32_t codepoint);

void _scene_text_atlas_destroy(DvzTextAtlas* atlas);

bool _scene_font_ensure_bytes(DvzFont* font);

void _scene_font_release(DvzFont* font);

EXTERN_C_OFF
