/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan internals                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY             8
#define DVZ_FRAME_PLAN_INITIAL_VISUAL_CAPACITY           4
#define DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY   16
#define DVZ_FRAME_PLAN_INITIAL_GRAPH_PASS_CAPACITY       16
#define DVZ_FRAME_PLAN_INITIAL_PRODUCT_CAPACITY          16
#define DVZ_FRAME_PLAN_INITIAL_PRODUCT_USE_CAPACITY      32
#define DVZ_FRAME_PLAN_INITIAL_COMPOSITION_CAPACITY      4
#define DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES                8
#define DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS       4
#define DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES             (DVZ_SCENE_MAX_RENDER_VISUALS + 16)
#define DVZ_PANEL_COMPOSITION_MAX_PASSES                 (DVZ_SCENE_MAX_RENDER_VISUALS + 64)
#define DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE 16
#define DVZ_PANEL_COMPOSITION_MAX_SCRATCH_RESOURCES      16
#define DVZ_PANEL_COMPOSITION_MAX_WORK_BINDINGS          8
#define DVZ_PANEL_COMPOSITION_MAX_UNREALIZED_PRODUCTS    4
#define DVZ_FRAME_PLAN_ASCII_COMPACT                     0x01u
#define DVZ_FRAME_PLAN_ASCII_VERBOSE                     0x02u
#define DVZ_FRAME_PLAN_ASCII_SHOW_UPLOADS                0x04u
#define DVZ_FRAME_PLAN_ASCII_SHOW_READBACKS              0x08u
#define DVZ_FRAME_PLAN_ASCII_ASCII_ONLY                  0x10u
#define DVZ_FRAME_PLAN_ASCII_MAX_WIDTH_120               0x20u
#define DVZ_SCENE_LABELS_LOOKUP_CAPACITY                 65u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_NEXT,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SHAPE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE_STYLE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA,
} DvzFramePlanResourceRole;



typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_KIND_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
} DvzFramePlanResourceKind;


typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_NONE = 0,
    DVZ_FRAME_GRAPH_RESOURCE_BUFFER,
    DVZ_FRAME_GRAPH_RESOURCE_TEXTURE,
    DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET,
} DvzFrameGraphResourceKind;



typedef enum
{
    DVZ_FRAME_GRAPH_EXTENT_NONE = 0,
    DVZ_FRAME_GRAPH_EXTENT_FIGURE,
    DVZ_FRAME_GRAPH_EXTENT_PANEL,
    DVZ_FRAME_GRAPH_EXTENT_FIXED,
    DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF,
} DvzFrameGraphExtentKind;



typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE = 0,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PERSISTENT,
} DvzFrameGraphResourceLifetime;



typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_NONE = 0x00u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT = 0x01u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT = 0x02u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED = 0x04u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE = 0x08u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC = 0x10u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST = 0x20u,
} DvzFrameGraphResourceUsage;



typedef enum
{
    DVZ_RENDER_PRODUCT_NONE = 0,
    DVZ_RENDER_PRODUCT_SCENE_COLOR,
    DVZ_RENDER_PRODUCT_SURFACE_DEPTH,
    DVZ_RENDER_PRODUCT_SURFACE_NORMAL,
    DVZ_RENDER_PRODUCT_SURFACE_COVERAGE,
    DVZ_RENDER_PRODUCT_OBJECT_ID,
    DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY,
    DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH,
    DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION,
    DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE,
    DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH,
    DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH,
    DVZ_RENDER_PRODUCT_PRESENTATION_COLOR,
} DvzRenderProductKind;



typedef enum
{
    DVZ_RENDER_PRODUCT_DOMAIN_NONE = 0,
    DVZ_RENDER_PRODUCT_DOMAIN_PANEL,
    DVZ_RENDER_PRODUCT_DOMAIN_VIEW,
    DVZ_RENDER_PRODUCT_DOMAIN_SCENE,
    DVZ_RENDER_PRODUCT_DOMAIN_QUERY,
    DVZ_RENDER_PRODUCT_DOMAIN_PRESENTATION,
} DvzRenderProductDomain;



typedef enum
{
    DVZ_RENDER_PRODUCT_EXTENT_NONE = 0,
    DVZ_RENDER_PRODUCT_EXTENT_ABSOLUTE,
    DVZ_RENDER_PRODUCT_EXTENT_TARGET_RELATIVE,
    DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE,
    DVZ_RENDER_PRODUCT_EXTENT_SOURCE_RELATIVE,
} DvzRenderProductExtentPolicy;



typedef enum
{
    DVZ_RENDER_PRODUCT_ROUND_NONE = 0,
    DVZ_RENDER_PRODUCT_ROUND_FLOOR,
    DVZ_RENDER_PRODUCT_ROUND_CEIL,
    DVZ_RENDER_PRODUCT_ROUND_NEAREST,
    DVZ_RENDER_PRODUCT_ROUND_OUTWARD,
} DvzRenderProductRoundingPolicy;



typedef enum
{
    DVZ_RENDER_PRODUCT_FORMAT_NONE = 0,
    DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR,
    DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT,
    DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT,
    DVZ_RENDER_PRODUCT_FORMAT_COVERAGE,
    DVZ_RENDER_PRODUCT_FORMAT_UINT_ID,
    DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT,
    DVZ_RENDER_PRODUCT_FORMAT_VECTOR2_FLOAT,
    DVZ_RENDER_PRODUCT_FORMAT_PRESENTATION_COLOR,
} DvzRenderProductFormatClass;



typedef enum
{
    DVZ_RENDER_PRODUCT_SAMPLES_NONE = 0,
    DVZ_RENDER_PRODUCT_SAMPLES_SINGLE,
    DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE,
    DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED,
} DvzRenderProductSampleDomain;



typedef enum
{
    DVZ_RENDER_PRODUCT_RESOLVE_NONE = 0,
    DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR,
    DVZ_RENDER_PRODUCT_RESOLVE_NEAREST_VALID_DEPTH,
    DVZ_RENDER_PRODUCT_RESOLVE_WINNING_NORMAL,
    DVZ_RENDER_PRODUCT_RESOLVE_COVERAGE_FRACTION,
    DVZ_RENDER_PRODUCT_RESOLVE_WINNING_ID,
    DVZ_RENDER_PRODUCT_RESOLVE_VISIBILITY,
} DvzRenderProductResolvePolicy;



typedef enum
{
    DVZ_RENDER_PRODUCT_COORDINATES_NONE = 0,
    DVZ_RENDER_PRODUCT_COORDINATES_PANEL_LOCAL,
    DVZ_RENDER_PRODUCT_COORDINATES_VIEW,
    DVZ_RENDER_PRODUCT_COORDINATES_SCENE,
    DVZ_RENDER_PRODUCT_COORDINATES_TARGET,
    DVZ_RENDER_PRODUCT_COORDINATES_WORLD,
    DVZ_RENDER_PRODUCT_COORDINATES_CLIP,
    DVZ_RENDER_PRODUCT_COORDINATES_NDC,
    DVZ_RENDER_PRODUCT_COORDINATES_FRAMEBUFFER_PIXEL,
    DVZ_RENDER_PRODUCT_COORDINATES_NOT_APPLICABLE,
} DvzRenderProductCoordinateSpace;



typedef enum
{
    DVZ_RENDER_PRODUCT_ENCODING_NONE = 0,
    DVZ_RENDER_PRODUCT_ENCODING_LINEAR_SCENE_COLOR,
    DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH,
    DVZ_RENDER_PRODUCT_ENCODING_VIEW_NORMAL,
    DVZ_RENDER_PRODUCT_ENCODING_UNIT_VISIBILITY,
    DVZ_RENDER_PRODUCT_ENCODING_INTEGER_ID,
    DVZ_RENDER_PRODUCT_ENCODING_COVERAGE,
    DVZ_RENDER_PRODUCT_ENCODING_PREMULTIPLIED_ACCUMULATION,
    DVZ_RENDER_PRODUCT_ENCODING_UNIT_TRANSMITTANCE,
    DVZ_RENDER_PRODUCT_ENCODING_PRESENTATION_TRANSFER,
} DvzRenderProductEncoding;



typedef enum
{
    DVZ_RENDER_PRODUCT_ALPHA_NONE = 0,
    DVZ_RENDER_PRODUCT_ALPHA_OPAQUE,
    DVZ_RENDER_PRODUCT_ALPHA_STRAIGHT,
    DVZ_RENDER_PRODUCT_ALPHA_PREMULTIPLIED,
} DvzRenderProductAlpha;



typedef enum
{
    DVZ_RENDER_PRODUCT_COVERAGE_NONE = 0,
    DVZ_RENDER_PRODUCT_COVERAGE_BINARY,
    DVZ_RENDER_PRODUCT_COVERAGE_SAMPLE_FRACTION,
    DVZ_RENDER_PRODUCT_COVERAGE_WINNING_SAMPLE,
} DvzRenderProductCoverage;



typedef enum
{
    DVZ_RENDER_PRODUCT_VALIDITY_NONE = 0,
    DVZ_RENDER_PRODUCT_VALIDITY_FULL_EXTENT,
    DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE,
    DVZ_RENDER_PRODUCT_VALIDITY_BACKGROUND_VALUE,
    DVZ_RENDER_PRODUCT_VALIDITY_INTEGER_SENTINEL,
} DvzRenderProductValidity;



typedef enum
{
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_NONE = 0,
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_ANY_DEFINED,
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT,
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_EXPLICIT_COVERAGE,
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_BACKGROUND_VALUE,
    DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_INTEGER_SENTINEL,
} DvzRenderProductValidityRequirement;



typedef struct DvzRenderProductId
{
    uint32_t value;
} DvzRenderProductId;



typedef struct DvzFramePlanPassId
{
    uint32_t value;
} DvzFramePlanPassId;



typedef enum
{
    DVZ_SCENE_TECHNIQUE_NONE = 0,
    DVZ_SCENE_TECHNIQUE_SCENE_OCCLUSION,
    DVZ_SCENE_TECHNIQUE_VOLUME_OCCLUSION,
    DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE,
    DVZ_SCENE_TECHNIQUE_SURFACE_RESOLVE,
    DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY,
    DVZ_SCENE_TECHNIQUE_OPAQUE_SHADING,
    DVZ_SCENE_TECHNIQUE_AMBIENT_COMPOSITE,
    DVZ_SCENE_TECHNIQUE_EDL,
    DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND,
    DVZ_SCENE_TECHNIQUE_WBOIT,
    DVZ_SCENE_TECHNIQUE_DEPTH_PEEL,
    DVZ_SCENE_TECHNIQUE_VOLUME_SHADING,
    DVZ_SCENE_TECHNIQUE_OVERLAY_COMPOSITE,
    DVZ_SCENE_TECHNIQUE_PRESENTATION,
} DvzSceneTechniqueId;



typedef enum
{
    DVZ_SCENE_PHASE_NONE = 0,
    DVZ_SCENE_PHASE_SURFACE_CAPTURE,
    DVZ_SCENE_PHASE_SURFACE_ANALYSIS,
    DVZ_SCENE_PHASE_OPAQUE_SHADING,
    DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
    DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
    DVZ_SCENE_PHASE_VOLUME_SHADING,
    DVZ_SCENE_PHASE_SCENE_POSTPROCESS,
    DVZ_SCENE_PHASE_OVERLAY,
    DVZ_SCENE_PHASE_PRESENTATION,
    DVZ_SCENE_PHASE_QUERY,
} DvzSceneTechniquePhase;



typedef enum
{
    DVZ_SCENE_TECHNIQUE_FALLBACK_NONE = 0,
    DVZ_SCENE_TECHNIQUE_FALLBACK_DISABLE_OPTIONAL,
    DVZ_SCENE_TECHNIQUE_FALLBACK_REDUCE_SAMPLES,
    DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION,
} DvzSceneTechniqueFallback;



typedef DvzFramePlanPassId DvzSceneCompositionPassId;

typedef struct DvzSceneTechniqueInstanceId
{
    uint32_t value;
} DvzSceneTechniqueInstanceId;

typedef struct DvzSceneScratchResourceId
{
    uint32_t value;
} DvzSceneScratchResourceId;

typedef enum
{
    DVZ_SCENE_WORK_NONE = 0,
    DVZ_SCENE_WORK_VISUAL_DRAWS,
    DVZ_SCENE_WORK_FULLSCREEN,
    DVZ_SCENE_WORK_COMPUTE,
} DvzSceneWorkClass;

/* Stable provider identities deliberately replace runtime pipeline/shader names. */
typedef enum
{
    DVZ_SCENE_WORK_PROVIDER_NONE = 0,
    DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION,
    DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION,
    DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE,
    DVZ_SCENE_WORK_PROVIDER_SURFACE_RESOLVE,
    DVZ_SCENE_WORK_PROVIDER_SSAO,
    DVZ_SCENE_WORK_PROVIDER_SSAO_BLUR,
    DVZ_SCENE_WORK_PROVIDER_OPAQUE,
    DVZ_SCENE_WORK_PROVIDER_AMBIENT_COMPOSITE,
    DVZ_SCENE_WORK_PROVIDER_EDL,
    DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND,
    DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION,
    DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE,
    DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT,
    DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION,
    DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE,
    DVZ_SCENE_WORK_PROVIDER_PRESENTATION,
} DvzSceneWorkProviderKey;

typedef enum
{
    DVZ_SCENE_RESOURCE_REF_NONE = 0,
    DVZ_SCENE_RESOURCE_REF_PRODUCT,
    DVZ_SCENE_RESOURCE_REF_SCRATCH,
    DVZ_SCENE_RESOURCE_REF_EXTERNAL,
} DvzSceneResourceRefKind;

typedef enum
{
    DVZ_SCENE_SCRATCH_NONE = 0,
    DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH,
    DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH,
    DVZ_SCENE_SCRATCH_Z_ONLY_DEPTH,
    DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY,
    DVZ_SCENE_SCRATCH_SURFACE_DEPTH,
    DVZ_SCENE_SCRATCH_FORWARD_DEPTH,
    DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH,
    DVZ_SCENE_SCRATCH_SSAO_RAW,
    DVZ_SCENE_SCRATCH_SSAO_DENOISE,
    DVZ_SCENE_SCRATCH_WBOIT_WEIGHT,
    DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM,
    DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING,
    DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG,
    DVZ_SCENE_SCRATCH_EDL_COLOR,
    DVZ_SCENE_SCRATCH_EDL_DEPTH,
} DvzSceneScratchKind;

typedef enum
{
    DVZ_SCENE_WORK_BINDING_NONE = 0,
    DVZ_SCENE_WORK_BINDING_ATTACHMENT,
    DVZ_SCENE_WORK_BINDING_SAMPLED,
    DVZ_SCENE_WORK_BINDING_STORAGE,
} DvzSceneWorkBindingUsage;

typedef enum
{
    DVZ_SCENE_WORK_ACCESS_NONE = 0,
    DVZ_SCENE_WORK_ACCESS_READ,
    DVZ_SCENE_WORK_ACCESS_WRITE,
    DVZ_SCENE_WORK_ACCESS_READ_WRITE,
} DvzSceneWorkAccess;

typedef enum
{
    DVZ_SCENE_AUXILIARY_NONE = 0,
    DVZ_SCENE_AUXILIARY_EDL_PARAMS,
    DVZ_SCENE_AUXILIARY_SSAO_PARAMS,
} DvzSceneAuxiliaryKind;

typedef struct DvzSceneAuxiliaryBinding
{
    uint32_t byte_offset;
    uint32_t byte_size;
    DvzSceneAuxiliaryKind kind;
    uint32_t upload_node_index;
    uint32_t set;
    uint32_t binding;
} DvzSceneAuxiliaryBinding;

typedef enum
{
    DVZ_SCENE_ATTACHMENT_LOAD_NONE = 0,
    DVZ_SCENE_ATTACHMENT_LOAD_CLEAR,
    DVZ_SCENE_ATTACHMENT_LOAD_LOAD,
    DVZ_SCENE_ATTACHMENT_LOAD_DONT_CARE,
} DvzSceneAttachmentLoad;

typedef enum
{
    DVZ_SCENE_ATTACHMENT_STORE_NONE = 0,
    DVZ_SCENE_ATTACHMENT_STORE_STORE,
    DVZ_SCENE_ATTACHMENT_STORE_DONT_CARE,
} DvzSceneAttachmentStore;

typedef enum
{
    DVZ_SCENE_CLEAR_VALUE_NONE = 0,
    DVZ_SCENE_CLEAR_VALUE_LITERAL,
    DVZ_SCENE_CLEAR_VALUE_FRAME,
} DvzSceneClearValueKind;

typedef enum
{
    DVZ_SCENE_SCRATCH_LIFETIME_NONE = 0,
    DVZ_SCENE_SCRATCH_LIFETIME_PASS,
    DVZ_SCENE_SCRATCH_LIFETIME_TECHNIQUE,
    DVZ_SCENE_SCRATCH_LIFETIME_FRAME,
} DvzSceneScratchLifetime;

typedef enum
{
    DVZ_SCENE_SCRATCH_SCOPE_NONE = 0,
    DVZ_SCENE_SCRATCH_SCOPE_PANEL,
    DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE,
    DVZ_SCENE_SCRATCH_SCOPE_PASS,
} DvzSceneScratchScope;

typedef struct DvzSceneScratchResource
{
    DvzSceneScratchResourceId id;
    DvzSceneTechniqueInstanceId technique_instance_id;
    DvzSceneScratchScope scope;
    DvzSceneScratchKind kind;
    uint32_t format;
    DvzRenderProductFormatClass format_class;
    DvzRenderProductExtentPolicy extent_policy;
    DvzRenderProductSampleDomain sample_domain;
    uint32_t sample_count;
    uint32_t usage_mask;
    DvzSceneScratchLifetime lifetime;
} DvzSceneScratchResource;

typedef struct DvzSceneWorkBinding
{
    DvzSceneResourceRefKind ref_kind;
    DvzRenderProductId product_id;
    DvzSceneScratchResourceId scratch_id;
    DvzSceneWorkBindingUsage usage;
    DvzSceneWorkAccess access;
    uint32_t slot;
    uint32_t set;
    uint32_t binding;
    DvzSceneAttachmentLoad load;
    DvzSceneAttachmentStore store;
    bool clear;
    DvzSceneClearValueKind clear_value_kind;
    bool depth_attachment;
    float clear_value[4];
    DvzSceneResourceRefKind load_source_ref_kind;
    DvzRenderProductId load_source_product_id;
    DvzSceneScratchResourceId load_source_scratch_id;
} DvzSceneWorkBinding;



typedef struct DvzSceneGraphRealization
{
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    DvzSceneResourceRefKind ref_kind;
    DvzRenderProductId product_id;
    DvzSceneScratchResourceId scratch_id;
    uint32_t graph_resource_index;
} DvzSceneGraphRealization;



typedef struct DvzSceneResolvedTechnique
{
    DvzSceneTechniqueInstanceId instance_id;
    DvzSceneTechniqueId id;
    uint32_t version;
    DvzSceneTechniquePhase phase;
    uint32_t must_follow_phase_mask;
    uint64_t input_product_mask;
    uint64_t output_product_mask;
    uint32_t input_count;
    DvzRenderProductKind inputs[DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE];
    DvzRenderProductId input_ids[DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE];
    uint32_t output_count;
    DvzRenderProductKind outputs[DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE];
    DvzRenderProductId output_ids[DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE];
    uint32_t participating_layer_mask;
    uint32_t required_capability_mask;
    uint32_t missing_capability_mask;
    uint32_t capability_adaptations;
    DvzSceneTechniqueFallback fallback;
    uint32_t expansion_flags;
    uint32_t authored_order_begin;
    uint32_t authored_order_end;
} DvzSceneResolvedTechnique;



typedef struct DvzSceneResolvedPass
{
    DvzSceneCompositionPassId id;
    DvzSceneTechniqueInstanceId technique_instance_id;
    DvzSceneTechniqueId technique_id;
    DvzSceneTechniquePhase phase;
    DvzFramePlanRenderPassRole role;
    uint32_t ordinal;
    uint32_t authored_order_begin;
    uint32_t authored_order_end;
    uint32_t work_index;
    DvzSceneWorkClass work_class;
    DvzSceneWorkProviderKey provider;
    DvzRenderProductCoordinateSpace coordinate_space;
    bool viewport_panel_local;
    bool scissor_panel_local;
    uint32_t sample_count;
    DvzRenderProductResolvePolicy resolve_policy;
    bool alpha_to_coverage;
    uint32_t visual_layer_filter;
    uint32_t visual_order_begin;
    uint32_t visual_order_end;
    uint32_t dispatch_x;
    uint32_t dispatch_y;
    uint32_t dispatch_z;
    bool legacy_transition;
    uint32_t unrealized_product_count;
    DvzRenderProductId unrealized_product_ids[DVZ_PANEL_COMPOSITION_MAX_UNREALIZED_PRODUCTS];
    uint32_t binding_count;
    DvzSceneWorkBinding bindings[DVZ_PANEL_COMPOSITION_MAX_WORK_BINDINGS];
    uint32_t auxiliary_binding_count;
    DvzSceneAuxiliaryBinding auxiliary_bindings[2];
} DvzSceneResolvedPass;



typedef struct DvzPanelCompositionSnapshot
{
    bool valid;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    float render_scale;
    float local_to_target[4];
    uint64_t required_product_mask;
    DvzCapabilitySnapshot capabilities;
    uint32_t available_capability_mask;
    uint32_t disabled_optional_technique_mask;
    uint32_t requested_sample_count;
    uint32_t effective_sample_count;
    bool alpha_to_coverage;
    uint64_t work_declaration_fingerprint;
    uint32_t technique_count;
    DvzSceneResolvedTechnique techniques[DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES];
    uint32_t scratch_resource_count;
    DvzSceneScratchResource scratch_resources[DVZ_PANEL_COMPOSITION_MAX_SCRATCH_RESOURCES];
    uint32_t pass_count;
    DvzSceneResolvedPass passes[DVZ_PANEL_COMPOSITION_MAX_PASSES];
} DvzPanelCompositionSnapshot;



typedef struct DvzSurfaceRecordId
{
    uint32_t value;
} DvzSurfaceRecordId;



typedef struct DvzRenderProductConsumer
{
    DvzRenderProductId product_id;
    uint32_t pass_index;
    DvzRenderProductValidityRequirement validity_requirement;
} DvzRenderProductConsumer;



typedef enum
{
    DVZ_FRAME_GRAPH_PASS_NONE = 0,
    DVZ_FRAME_GRAPH_PASS_RENDER,
    DVZ_FRAME_GRAPH_PASS_COMPUTE,
    DVZ_FRAME_GRAPH_PASS_COPY,
    DVZ_FRAME_GRAPH_PASS_READBACK,
    DVZ_FRAME_GRAPH_PASS_CLEAR,
} DvzFrameGraphPassKind;



typedef enum
{
    DVZ_FRAME_GRAPH_ACCESS_NONE = 0,
    DVZ_FRAME_GRAPH_ACCESS_SAMPLED,
    DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ,
    DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE,
    DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT,
    DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ,
    DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE,
    DVZ_FRAME_GRAPH_ACCESS_COPY_SRC,
    DVZ_FRAME_GRAPH_ACCESS_COPY_DST,
} DvzFrameGraphAccessUsage;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE,
} DvzFrameGraphAttachmentLoadOp;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE,
} DvzFrameGraphAttachmentStoreOp;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE,
} DvzFrameGraphAttachmentAccess;



typedef struct DvzFrameGraphResource
{
    char id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphResourceKind kind;
    uint32_t format;
    DvzFrameGraphExtentKind extent_kind;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t sample_count;
    char extent_resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t usage_flags;
    DvzFrameGraphResourceLifetime lifetime;
} DvzFrameGraphResource;



typedef struct DvzRenderProductContract
{
    DvzRenderProductId id;
    char diagnostic_label[DVZ_SCENE_LABEL_SIZE];
    uint32_t version;
    DvzRenderProductKind kind;
    DvzRenderProductDomain domain;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    char view_id[DVZ_SCENE_LABEL_SIZE];
    char camera_id[DVZ_SCENE_LABEL_SIZE];
    char projection_id[DVZ_SCENE_LABEL_SIZE];
    DvzRenderProductExtentPolicy extent_policy;
    DvzRenderProductRoundingPolicy rounding_policy;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    float render_scale;
    float local_to_target[4];
    DvzRenderProductFormatClass format_class;
    uint32_t concrete_format;
    DvzRenderProductSampleDomain sample_domain;
    uint32_t sample_count;
    DvzRenderProductResolvePolicy resolve_policy;
    DvzRenderProductCoordinateSpace coordinate_space;
    DvzRenderProductEncoding encoding;
    DvzRenderProductAlpha alpha;
    DvzRenderProductCoverage coverage;
    DvzRenderProductValidity validity;
    DvzRenderProductId validity_product_id;
    bool has_background_value;
    float background_value[4];
    bool has_integer_sentinel;
    uint64_t integer_sentinel;
    uint32_t required_usage_flags;
    DvzFrameGraphResourceLifetime lifetime;
    uint32_t resource_index;
    DvzRenderProductId source_product_id;
    DvzSurfaceRecordId surface_record_id;
    uint32_t producer_pass_index;
    uint32_t capability_adaptations;
} DvzRenderProductContract;



typedef struct DvzFrameGraphAccess
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphAccessUsage usage;
} DvzFrameGraphAccess;



typedef struct DvzFrameGraphAttachment
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t resolve_mode;
    DvzFrameGraphAttachmentLoadOp load_op;
    DvzFrameGraphAttachmentStoreOp store_op;
    DvzFrameGraphAttachmentAccess access;
    float clear_color[4];
    float clear_depth;
    uint32_t clear_stencil;
} DvzFrameGraphAttachment;



typedef struct DvzFrameGraphRect
{
    float x;
    float y;
    float width;
    float height;
} DvzFrameGraphRect;



typedef struct DvzFrameGraphPass
{
    char id[DVZ_SCENE_LABEL_SIZE];
    bool has_composition_pass;
    DvzFramePlanPassId composition_pass_id;
    DvzFrameGraphPassKind kind;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    bool has_viewport;
    DvzFrameGraphRect viewport;
    bool has_scissor;
    DvzFrameGraphRect scissor;
    uint32_t read_count;
    DvzFrameGraphAccess reads[DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES];
    uint32_t write_count;
    DvzFrameGraphAccess writes[DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES];
    uint32_t color_attachment_count;
    DvzFrameGraphAttachment color_attachments[DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS];
    bool has_depth_attachment;
    DvzFrameGraphAttachment depth_attachment;
    bool has_stencil_attachment;
    DvzFrameGraphAttachment stencil_attachment;
    bool alpha_to_coverage;
    char work_label[DVZ_SCENE_LABEL_SIZE];
} DvzFrameGraphPass;



typedef struct DvzFrameGraphDependency
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t producer_pass_index;
    uint32_t consumer_pass_index;
    char producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    char consumer_pass_id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphAccessUsage producer_usage;
    DvzFrameGraphAccessUsage consumer_usage;
} DvzFrameGraphDependency;



typedef struct DvzFramePlanUploadMeta
{
    bool has_metadata;
    DvzFramePlanResourceKind kind;
    DvzFramePlanResourceRole role;
    DvzColorRole color_role;
    uint32_t visual_type;
    uint32_t visual_index;
    uint32_t buffer_index;
    uint64_t logical_item_count;
} DvzFramePlanUploadMeta;



typedef enum DvzFramePlanClipRect
{
    DVZ_FRAME_PLAN_CLIP_RECT_PANEL = 0,
    DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
} DvzFramePlanClipRect;


typedef enum DvzFramePlanViewportRect
{
    DVZ_FRAME_PLAN_VIEWPORT_PANEL = 0,
    DVZ_FRAME_PLAN_VIEWPORT_PLOT,
    DVZ_FRAME_PLAN_VIEWPORT_TARGET,
} DvzFramePlanViewportRect;


typedef enum DvzRenderableKind
{
    DVZ_RENDERABLE_NONE = 0,
    DVZ_RENDERABLE_POINT_LIKE,
    DVZ_RENDERABLE_STROKE_QUAD,
    DVZ_RENDERABLE_PATH_STROKE,
    DVZ_RENDERABLE_INDEXED_MESH,
    DVZ_RENDERABLE_TEXTURED_QUAD,
    DVZ_RENDERABLE_VOLUME_PROXY,
} DvzRenderableKind;



typedef struct DvzFramePlanVisualMeta
{
    bool has_metadata;
    DvzFramePlanClipRect clip_rect;
    DvzFramePlanViewportRect viewport_rect;
    uint32_t visual_type;
    uint32_t renderable_kind;
    uint32_t desc_kind;
    uint32_t point_like_kind;
    uint32_t visual_index;
    uint32_t buffer_index;
    uint32_t topology;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t instance_count;
    bool has_item_range;
    uint32_t item_range_first;
    uint32_t item_range_count;
    DvzAlphaMode alpha_mode;
    bool depth_test_enabled;
    DvzCompareOp depth_compare_op;
    bool depth_cue_enabled;
    bool point_style_enabled;
    bool has_volume;
    bool has_labels;
    bool image_pixel_space;
    bool image_nearest_sampler;
    DvzColorRole image_color_role;
    uint32_t field_format;
    uint32_t field_semantic;
    uint32_t field_width;
    uint32_t field_height;
    uint32_t field_depth;
    uint32_t scale_index;
    bool volume_transfer_rgba;
    DvzColorRole volume_color_role;
    bool scene_occluder;
    bool scene_occluded;
    bool has_scene_occlusion;
    DvzSceneOcclusionDesc scene_occlusion;
    bool volume_occluded;
    bool has_volume_occlusion;
    DvzVolumeOcclusionDesc volume_occlusion;
    DvzLabelsState labels_state;
    uint32_t labels_lookup_count;
    uint32_t labels_lookup[DVZ_SCENE_LABELS_LOOKUP_CAPACITY][4];
    DvzVolumeState volume_state;
    uint32_t glyph_atlas_encoding;
    float glyph_distance_range_px;
    bool has_draw_contract;
    char draw_contract_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_depth_policy;
    uint32_t draw_blend_policy;
    uint32_t draw_blend_mode;
    uint32_t draw_shader_feature_mask;
    uint32_t draw_bind_group_layout_mask;
    bool draw_overlay_composite;
    bool draw_has_raster_state;
    uint32_t draw_cull_mode;
    uint32_t draw_front_face;
    char draw_volume_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char draw_volume_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_volume_occlusion_bind_set;
    uint32_t draw_volume_occlusion_bind_binding;
    char draw_scene_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char draw_scene_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_scene_occlusion_bind_set;
    uint32_t draw_scene_occlusion_bind_binding;
    char position_id[DVZ_SCENE_LABEL_SIZE];
    char position_start_id[DVZ_SCENE_LABEL_SIZE];
    char position_end_id[DVZ_SCENE_LABEL_SIZE];
    char position_next_id[DVZ_SCENE_LABEL_SIZE];
    char color_id[DVZ_SCENE_LABEL_SIZE];
    char size_id[DVZ_SCENE_LABEL_SIZE];
    char sigma_id[DVZ_SCENE_LABEL_SIZE];
    char angle_id[DVZ_SCENE_LABEL_SIZE];
    char bounds_id[DVZ_SCENE_LABEL_SIZE];
    char shape_id[DVZ_SCENE_LABEL_SIZE];
    char line_width_id[DVZ_SCENE_LABEL_SIZE];
    char tex_rect_id[DVZ_SCENE_LABEL_SIZE];
    char texcoords_id[DVZ_SCENE_LABEL_SIZE];
    char instance_transform_id[DVZ_SCENE_LABEL_SIZE];
    char texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_transfer_texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_label_lookup_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char index_id[DVZ_SCENE_LABEL_SIZE];
    char material_id[DVZ_SCENE_LABEL_SIZE];
    char selection_id[DVZ_SCENE_LABEL_SIZE];
    char item_state_style_id[DVZ_SCENE_LABEL_SIZE];
    char path_flags_id[DVZ_SCENE_LABEL_SIZE];
    char path_distance_id[DVZ_SCENE_LABEL_SIZE];
} DvzFramePlanVisualMeta;



typedef struct DvzSceneViewportUniform
{
    float x;
    float y;
    float width;
    float height;
} DvzSceneViewportUniform;


typedef struct DvzFramePlanComputeBinding
{
    uint32_t binding;
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    DvzSceneComputeAccess access;
    uint64_t byte_offset;
    uint64_t byte_size;
} DvzFramePlanComputeBinding;



struct DvzFramePlanNode
{
    DvzFramePlanNodeType type;
    union
    {
        struct
        {
            char resource_id[DVZ_SCENE_LABEL_SIZE];
            uint64_t byte_offset;
            uint64_t byte_size;
            char data_tag[DVZ_SCENE_LABEL_SIZE];
            const void* data; /* optional: if non-NULL, actual bytes to upload */
            void* owned_data;
            bool external;         /* register only; resource is provided by the live runtime */
            uint32_t buffer_usage; /* optional DRP2 buffer-usage mask (0 = vertex default) */
            uint32_t item_stride;  /* optional element stride, used by index buffers */
            /* Optional primitive topology hint, propagated to the converter resource entry.
             * UINT32_MAX = unspecified (default; used by POINT and other typed families). */
            uint32_t topology;
            /* Optional texture write extent. When `texture_width > 0`, the upload targets a
             * texture rather than a vertex buffer. Default 0 = vertex-buffer upload. */
            uint32_t texture_width;
            uint32_t texture_height;
            uint32_t texture_depth;
            uint32_t texture_format;
            uint32_t texture_bytes_per_texel;
            /* Optional full texture allocation extent. Defaults to the write extent when unset. */
            uint32_t texture_alloc_width;
            uint32_t texture_alloc_height;
            uint32_t texture_alloc_depth;
            uint32_t texture_origin_x;
            uint32_t texture_origin_y;
            uint32_t texture_origin_z;
            DvzFramePlanUploadMeta metadata;
        } upload;
        struct
        {
            char shader_key[DVZ_SCENE_LABEL_SIZE];
            DvzSceneShaderFormat shader_format;
            const char* shader_source;
            uint32_t dispatch[3];
            uint32_t binding_count;
            DvzFramePlanComputeBinding bindings[DVZ_SCENE_MAX_NODE_RESOURCES];
            uint32_t read_count;
            char reads[DVZ_SCENE_MAX_NODE_RESOURCES][DVZ_SCENE_LABEL_SIZE];
            uint32_t write_count;
            char writes[DVZ_SCENE_MAX_NODE_RESOURCES][DVZ_SCENE_LABEL_SIZE];
        } compute;
        struct
        {
            char panel_id[DVZ_SCENE_LABEL_SIZE];
            char render_target_id[DVZ_SCENE_LABEL_SIZE];
            uint32_t visual_count;
            uint32_t visual_capacity;
            char (*visuals)[DVZ_SCENE_LABEL_SIZE];
            DvzFramePlanVisualMeta* visual_metadata;
            bool picking;
            DvzFramePlanRenderPassRole pass_role;
            bool has_composition_pass;
            DvzFramePlanPassId composition_pass_id;
            bool has_graph_pass_index;
            uint32_t graph_pass_index;
            bool has_pass_contract;
            char pass_contract_id[DVZ_SCENE_LABEL_SIZE];
            DvzPanelDesc desc;
            bool has_plot_desc;
            DvzPanelDesc plot_desc;
            bool has_viewport;
            DvzSceneViewportUniform viewport;
            bool has_mvp;
            DvzMVP apply_mvp; /* panel APPLY MVP from panzoom/arcball; identity MVP for FIXED
                                 computed by converter */
            DvzControllerMode* controller_modes; /* parallel to visuals[] */
            DvzMVP* visual_mvp;
            bool* visual_has_mvp;
        } render;
        struct
        {
            char panel_id[DVZ_SCENE_LABEL_SIZE];
            char render_target_id[DVZ_SCENE_LABEL_SIZE];
            DvzPanelDesc desc;
        } clear;
        struct
        {
            DvzFramePlanCopyDirection direction;
            char src_resource_id[DVZ_SCENE_LABEL_SIZE];
            char dst_resource_id[DVZ_SCENE_LABEL_SIZE];
            uint32_t src_attachment_index;
            uint32_t src_origin[3];
            uint64_t src_offset;
            uint32_t extent[3];
            uint32_t format;
            uint32_t bytes_per_texel;
            uint64_t bytes_per_row;
            uint32_t rows_per_image;
            uint32_t dst_origin[3];
            uint64_t dst_offset;
            uint64_t byte_size;
            uint64_t request_id;
        } copy;
        struct
        {
            char resource_id[DVZ_SCENE_LABEL_SIZE];
            char request_id[DVZ_SCENE_LABEL_SIZE];
        } readback;
    } u;
};



struct DvzFramePlan
{
    char figure_id[DVZ_SCENE_LABEL_SIZE];
    uint64_t frame_index;
    uint32_t capacity;
    uint32_t count;
    DvzFramePlanNode* nodes;
    uint32_t graph_resource_capacity;
    uint32_t graph_resource_count;
    DvzFrameGraphResource* graph_resources;
    uint32_t graph_pass_capacity;
    uint32_t graph_pass_count;
    DvzFrameGraphPass* graph_passes;
    uint32_t product_capacity;
    uint32_t product_count;
    DvzRenderProductContract* products;
    uint32_t product_use_capacity;
    uint32_t product_use_count;
    DvzRenderProductConsumer* product_uses;
    uint32_t composition_capacity;
    uint32_t composition_count;
    DvzPanelCompositionSnapshot* compositions;
    uint32_t realization_capacity;
    uint32_t realization_count;
    DvzSceneGraphRealization* realizations;
};



/*************************************************************************************************/
/*  Internal helpers                                                                            */
/*************************************************************************************************/

bool dvz_frame_plan_render_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc);

bool dvz_frame_plan_render_panel_role(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc, DvzFramePlanRenderPassRole pass_role);

bool dvz_frame_plan_clear_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc);

DvzFramePlanNode* dvz_frame_plan_last_render_node(DvzFramePlan* plan);

bool dvz_frame_plan_render_visual_metadata(
    DvzFramePlan* plan, const DvzFramePlanVisualMeta* metadata);

bool dvz_frame_plan_render_metadata_complete(const DvzFramePlan* plan);

bool dvz_frame_plan_upload_metadata(DvzFramePlan* plan, const DvzFramePlanUploadMeta* metadata);

bool dvz_frame_plan_graph_resource(DvzFramePlan* plan, const DvzFrameGraphResource* resource);

uint32_t dvz_frame_plan_graph_resource_count(const DvzFramePlan* plan);

const DvzFrameGraphResource*
dvz_frame_plan_graph_resource_get(const DvzFramePlan* plan, uint32_t index);

bool dvz_frame_plan_graph_pass(DvzFramePlan* plan, const DvzFrameGraphPass* pass);

uint32_t dvz_frame_plan_graph_pass_count(const DvzFramePlan* plan);

const DvzFrameGraphPass* dvz_frame_plan_graph_pass_get(const DvzFramePlan* plan, uint32_t index);

uint32_t dvz_frame_plan_graph_dependency_count(const DvzFramePlan* plan);

bool dvz_frame_plan_graph_dependency_get(
    const DvzFramePlan* plan, uint32_t index, DvzFrameGraphDependency* out);

bool dvz_frame_graph_pass_read(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage);

bool dvz_frame_graph_pass_write(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage);

bool dvz_frame_graph_pass_color_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment);

bool dvz_frame_graph_pass_depth_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment);

bool dvz_frame_plan_graph_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report);

bool dvz_frame_plan_product(DvzFramePlan* plan, const DvzRenderProductContract* product);

bool dvz_frame_plan_product_consumer(
    DvzFramePlan* plan, DvzRenderProductId product_id, uint32_t pass_index,
    DvzRenderProductValidityRequirement validity_requirement);

uint32_t dvz_frame_plan_product_count(const DvzFramePlan* plan);

const DvzRenderProductContract*
dvz_frame_plan_product_get(const DvzFramePlan* plan, uint32_t index);

bool dvz_frame_plan_products_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report);

char* dvz_frame_plan_graph_dump(const DvzFramePlan* plan);

char* dvz_frame_plan_graph_ascii(const DvzFramePlan* plan, uint32_t flags);

void dvz_frame_plan_graph_ascii_destroy(char* text);

bool dvz_capability_snapshot_valid(const DvzCapabilitySnapshot* snapshot);
