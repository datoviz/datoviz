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
#include "scene_emit/visual_lowering.h"
#include "upload.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzImageTextureUploadPayload
{
    DvzFieldRegion region;
    const void* data;
    uint64_t byte_size;
    uint32_t allocation_width;
    uint32_t allocation_height;
} DvzImageTextureUploadPayload;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _image_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr);

bool _image_query_generated_rect_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, bool include_ids,
    bool include_texcoords, uint64_t* out_vertex_count);

bool _image_uses_generated_quads(const DvzVisual* visual);

bool _image_generated_quad_cache_rebuild(const DvzFigure* figure, DvzVisual* visual);
bool _image_generated_quad_upload_payloads(
    const DvzFigure* figure, DvzVisual* visual, DvzVisualUploadPayload* out_payloads,
    uint32_t* out_count);

bool _image_texture_upload_payload(DvzVisual* visual, DvzImageTextureUploadPayload* out);

void _image_gpu_cache_free(DvzImageGpuCache* cache);

bool _scene_image_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);
