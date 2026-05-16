/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene enums                                                                                  */
/*************************************************************************************************/

#pragma once

#include "datoviz/vk/enums.h" /* DvzPrimitiveTopology — shared with vklite/DRP2 */



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel link flags.
typedef enum
{
    DVZ_PANEL_LINK_FLAGS_NONE = 0x00,
    DVZ_PANEL_LINK_FLAGS_MODEL = 0x01,
    DVZ_PANEL_LINK_FLAGS_VIEW = 0x02,
    DVZ_PANEL_LINK_FLAGS_PROJECTION = 0x04,
} DvzPanelLinkFlags;



typedef enum
{
    DVZ_FRAME_PLAN_NODE_NONE,
    DVZ_FRAME_PLAN_NODE_UPLOAD,
    DVZ_FRAME_PLAN_NODE_COMPUTE,
    DVZ_FRAME_PLAN_NODE_RENDER,
    DVZ_FRAME_PLAN_NODE_CLEAR,
    DVZ_FRAME_PLAN_NODE_COPY,
    DVZ_FRAME_PLAN_NODE_READBACK,
} DvzFramePlanNodeType;



typedef enum
{
    DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE = 0,
    DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER,
    DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
    DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
    DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
    DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
    DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
    DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
    DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
    DVZ_FRAME_PLAN_RENDER_PASS_PICKING,
} DvzFramePlanRenderPassRole;



typedef enum
{
    DVZ_SCENE_SHADER_FORMAT_WGSL,
    DVZ_SCENE_SHADER_FORMAT_GLSL,
} DvzSceneShaderFormat;



typedef enum
{
    DVZ_SCENE_BUFFER_USAGE_NONE = 0x00u,
    DVZ_SCENE_BUFFER_USAGE_VERTEX = 0x01u,
    DVZ_SCENE_BUFFER_USAGE_INDEX = 0x02u,
    DVZ_SCENE_BUFFER_USAGE_UNIFORM = 0x04u,
} DvzSceneBufferUsage;



typedef enum
{
    DVZ_VISUAL_ATTR_SOURCE_PER_ITEM = 0,
    DVZ_VISUAL_ATTR_SOURCE_CONSTANT,
    DVZ_VISUAL_ATTR_SOURCE_PER_SPAN,
    DVZ_VISUAL_ATTR_SOURCE_PER_GROUP,
} DvzVisualAttrSource;



typedef enum
{
    DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC = 0,
    DVZ_VISUAL_ATTR_MUTABILITY_STATIC,
    DVZ_VISUAL_ATTR_MUTABILITY_STREAMING,
} DvzVisualAttrMutability;



/* Whether a visual is affected by its panel's controller (panzoom/arcball). */
typedef enum
{
    DVZ_CONTROLLER_APPLY = 0, /* default: panzoom/arcball MVP applies to the visual */
    DVZ_CONTROLLER_FIXED = 1, /* visual is unaffected by navigation; identity MVP */
} DvzControllerMode;



typedef enum
{
    DVZ_ALPHA_OPAQUE = 0,
    DVZ_ALPHA_BLENDED,
    DVZ_ALPHA_WBOIT,
    DVZ_ALPHA_DEPTH_PEEL,
    DVZ_ALPHA_MASK,
} DvzAlphaMode;


typedef enum
{
    DVZ_MATERIAL_MODEL_UNLIT = 0,
    DVZ_MATERIAL_MODEL_PHONG,
    DVZ_MATERIAL_MODEL_STANDARD,
} DvzMaterialModel;


typedef enum
{
    DVZ_SPHERE_FLAGS_NONE = 0x0000,
    DVZ_SPHERE_FLAGS_LIGHTING = 0x0001,
    DVZ_SPHERE_FLAGS_SIZE_PIXELS = 0x0002,
} DvzSphereFlags;


typedef enum
{
    DVZ_SPHERE_MODE_FAST_IMPOSTOR = 0,
    DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR = 1,
} DvzSphereMode;


typedef enum
{
    DVZ_DEPTH_CUE_NONE = 0,
    DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
    DVZ_DEPTH_CUE_DESATURATE,
    DVZ_DEPTH_CUE_DARKEN,
} DvzDepthCueMode;


typedef enum
{
    DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH = 0,
    DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
    DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE,
} DvzDepthCueMetric;


typedef enum
{
    DVZ_DEPTH_CUE_FALLOFF_LINEAR = 0,
    DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL,
} DvzDepthCueFalloff;


typedef enum
{
    DVZ_VOLUME_SAMPLING_LINEAR = 0,
    DVZ_VOLUME_SAMPLING_NEAREST,
} DvzVolumeSamplingMode;


typedef enum
{
    DVZ_VOLUME_RENDER_SLICE = 0,
    DVZ_VOLUME_RENDER_MIP,
    DVZ_VOLUME_RENDER_COMPOSITE,
} DvzVolumeRenderMode;



typedef enum
{
    DVZ_VOLUME_AXIS_X = 0,
    DVZ_VOLUME_AXIS_Y = 1,
    DVZ_VOLUME_AXIS_Z = 2,
} DvzVolumeAxis;



typedef enum
{
    DVZ_SCENE_TARGET_NONE = 0,
    DVZ_SCENE_TARGET_OBJECT,
    DVZ_SCENE_TARGET_ITEM,
    DVZ_SCENE_TARGET_VERTEX,
    DVZ_SCENE_TARGET_FACE,
    DVZ_SCENE_TARGET_PIXEL,
    DVZ_SCENE_TARGET_SAMPLE,
    DVZ_SCENE_TARGET_STRIP,
    DVZ_SCENE_TARGET_SEGMENT,
    DVZ_SCENE_TARGET_TRIANGLE,
    DVZ_SCENE_TARGET_TEXT,
    DVZ_SCENE_TARGET_ANNOTATION,
} DvzSceneTargetKind;



typedef enum
{
    DVZ_PICK_HIT_FRONTMOST = 0,
    DVZ_PICK_HIT_OPAQUE_PREFERRED,
    DVZ_PICK_HIT_ALL,
} DvzPickHitPolicy;



typedef enum
{
    DVZ_PROBE_VALUE_NONE = 0,
    DVZ_PROBE_VALUE_SCALAR,
    DVZ_PROBE_VALUE_VEC2,
    DVZ_PROBE_VALUE_VEC3,
    DVZ_PROBE_VALUE_VEC4,
    DVZ_PROBE_VALUE_LABEL,
} DvzProbeValueKind;



typedef enum
{
    DVZ_SELECT_REPLACE = 0,
    DVZ_SELECT_ADDITIVE,
    DVZ_SELECT_SUBTRACT,
    DVZ_SELECT_TOGGLE,
} DvzSelectMode;



typedef enum
{
    DVZ_SCALE_CONTINUOUS = 0,
    DVZ_SCALE_CATEGORICAL,
} DvzScaleKind;



typedef enum
{
    DVZ_COLORMAP_CONTINUOUS = 0,
    DVZ_COLORMAP_CATEGORICAL,
} DvzColormapKind;



typedef enum
{
    DVZ_BUILTIN_COLORMAP_NONE = 0,
    DVZ_BUILTIN_COLORMAP_VIRIDIS,
    DVZ_BUILTIN_COLORMAP_MAGMA,
    DVZ_BUILTIN_COLORMAP_PLASMA,
    DVZ_BUILTIN_COLORMAP_INFERNO,
    DVZ_BUILTIN_COLORMAP_CIVIDIS,
    DVZ_BUILTIN_COLORMAP_TURBO,
    DVZ_BUILTIN_COLORMAP_GRAY,
} DvzBuiltinColormap;



typedef enum
{
    DVZ_COLORBAR_ORIENTATION_VERTICAL = 0,
    DVZ_COLORBAR_ORIENTATION_HORIZONTAL,
} DvzColorbarOrientation;



typedef enum
{
    DVZ_SCENE_ANCHOR_NONE = 0,
    DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
    DVZ_SCENE_ANCHOR_PANEL_TOP,
    DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT,
    DVZ_SCENE_ANCHOR_PANEL_LEFT,
    DVZ_SCENE_ANCHOR_PANEL_CENTER,
    DVZ_SCENE_ANCHOR_PANEL_RIGHT,
    DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT,
    DVZ_SCENE_ANCHOR_PANEL_BOTTOM,
    DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT,
    DVZ_SCENE_ANCHOR_DATA,
    DVZ_SCENE_ANCHOR_WORLD,
    DVZ_SCENE_ANCHOR_SCREEN,
} DvzSceneAnchor;



typedef enum
{
    DVZ_TEXT_PLACEMENT_SCREEN = 0,
    DVZ_TEXT_PLACEMENT_DATA,
    DVZ_TEXT_PLACEMENT_WORLD,
} DvzTextPlacementMode;



typedef enum
{
    DVZ_ANNOTATION_LABEL = 0,
    DVZ_ANNOTATION_CALLOUT,
    DVZ_ANNOTATION_SCALEBAR,
    DVZ_ANNOTATION_DIMENSION,
    DVZ_ANNOTATION_PINNED_READOUT,
} DvzAnnotationKind;



typedef enum
{
    DVZ_PICK_CAPABILITY_OBJECT = 0x01u,
    DVZ_PICK_CAPABILITY_ITEM = 0x02u,
    DVZ_PICK_CAPABILITY_VERTEX = 0x04u,
    DVZ_PICK_CAPABILITY_FACE = 0x08u,
    DVZ_PICK_CAPABILITY_PIXEL = 0x10u,
    DVZ_PICK_CAPABILITY_SAMPLE = 0x20u,
    DVZ_PICK_CAPABILITY_GROUP = 0x40u,
} DvzPickCapabilityFlag;
