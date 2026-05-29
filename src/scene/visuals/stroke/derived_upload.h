/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Stroke visual derived upload payloads                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "upload.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _stroke_quad_segment_derived_upload_payloads(
    DvzVisual* visual, bool vector_params_sync, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count);

bool _path_stroke_derived_upload_payloads(
    DvzVisual* visual, bool vector_params_sync, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count);
