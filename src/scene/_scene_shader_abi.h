/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene shader ABI constants                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_SHADER_SET_COMMON 0u
#define DVZ_SCENE_SHADER_SET_VISUAL 1u
#define DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION 2u

#define DVZ_SCENE_SHADER_BINDING_COMMON_MVP 0u
#define DVZ_SCENE_SHADER_BINDING_COMMON_VIEWPORT 1u

#define DVZ_SCENE_SHADER_BINDING_MATERIAL_PARAMS 0u

#define DVZ_SCENE_SHADER_BINDING_IMAGE_TEXTURE 0u
#define DVZ_SCENE_SHADER_BINDING_IMAGE_SAMPLER 1u

#define DVZ_SCENE_SHADER_BINDING_LABELS_TEXTURE 0u
#define DVZ_SCENE_SHADER_BINDING_LABELS_SAMPLER 1u
#define DVZ_SCENE_SHADER_BINDING_LABELS_PARAMS 2u

#define DVZ_SCENE_SHADER_BINDING_GLYPH_TEXTURE 0u
#define DVZ_SCENE_SHADER_BINDING_GLYPH_SAMPLER 1u
#define DVZ_SCENE_SHADER_BINDING_GLYPH_PARAMS 2u

#define DVZ_SCENE_SHADER_BINDING_VOLUME_TEXTURE 0u
#define DVZ_SCENE_SHADER_BINDING_VOLUME_SAMPLER 1u
#define DVZ_SCENE_SHADER_BINDING_VOLUME_PARAMS 2u
#define DVZ_SCENE_SHADER_BINDING_VOLUME_DEPTH_TEXTURE 3u
#define DVZ_SCENE_SHADER_BINDING_VOLUME_TRANSFER_TEXTURE 4u
#define DVZ_SCENE_SHADER_BINDING_VOLUME_LABEL_LOOKUP 5u

#define DVZ_SCENE_SHADER_BINDING_OCCLUSION_DEPTH_TEXTURE 0u
#define DVZ_SCENE_SHADER_BINDING_OCCLUSION_SAMPLER 1u
#define DVZ_SCENE_SHADER_BINDING_OCCLUSION_PARAMS 2u
