/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Annotation text visual bridge                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags);

int _scene_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);

int _scene_text_visual_set_font(DvzVisual* visual, DvzFont* font);

DvzTextRenderer _scene_adornment_text_renderer(DvzTextRenderer renderer);

DvzVisual* _scene_adornment_text_visual(DvzScene* scene, DvzTextRenderer renderer);

int _scene_adornment_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);
