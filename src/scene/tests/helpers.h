/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene test helpers                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "../_frame_plan_emit.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SceneCanvasDrawState SceneCanvasDrawState;

struct SceneCanvasDrawState
{
    DvzFramePlanEmitter* emitter;
    DvzDrp2Runtime* runtime;
    DvzCapabilitySnapshot caps;
    DvzFramePlanEmitConfig emit_cfg;
    uint32_t callback_count;
    bool attach_ok;
    bool emit_ok;
    bool direct_target_ok;
    bool execute_ok;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _captured_log_contains(const TstContext* suite, const char* needle);

const uint8_t* _pixel_at(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t x, uint32_t y);

bool _scene_vklite_runtime_available(void);

void _scene_canvas_drp2_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data);

char* _read_text_fixture(const char* path);

int _assert_stream_matches_fixture(DvzDrp2CommandStream* stream, const char* name, const char* path);

uint32_t _stream_write_buffer_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_visual_write_buffer_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_write_buffer_range_count(
    const DvzDrp2CommandStream* stream, uint64_t offset, uint64_t size);

uint32_t _stream_draw_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_set_vertex_buffer_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_set_index_buffer_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_draw_indexed_count(const DvzDrp2CommandStream* stream);

uint32_t _stream_create_buffer_size_count(const DvzDrp2CommandStream* stream, uint64_t size);
