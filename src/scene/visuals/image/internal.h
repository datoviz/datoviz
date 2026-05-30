/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image visual internals                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "image/cache.h"
#include "image/upload_payload.h"
#include "scene_emit/visual_lowering.h"
#include "upload.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _image_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr);

bool _image_query_generated_rect_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, bool include_ids,
    bool include_texcoords, uint64_t* out_vertex_count);

bool _scene_image_query_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, bool include_static_uploads, DvzSceneQueryScratch* out_plan);

bool _image_uses_generated_quads(const DvzVisual* visual);

bool _image_generated_quad_cache_rebuild(const DvzFigure* figure, DvzVisual* visual);
bool _image_generated_quad_upload_payloads(
    const DvzFigure* figure, DvzVisual* visual, DvzVisualUploadPayload* out_payloads,
    uint32_t* out_count);

bool _scene_image_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);

bool _scene_image_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d);

bool _scene_image_visual_fill_metadata(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata);

bool _scene_image_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_image_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out);

bool _scene_image_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

bool _scene_image_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out);
