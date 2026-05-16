/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_LABEL_SIZE 128
#define DVZ_SCENE_MAX_NODE_RESOURCES 8
#define DVZ_SCENE_MAX_RENDER_VISUALS 8
#define DVZ_SCENE_MAX_DIAGNOSTICS 16
#define DVZ_SCENE_DIAGNOSTIC_SIZE 256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCapabilitySnapshot DvzCapabilitySnapshot;
typedef struct DvzDiagnosticReport DvzDiagnosticReport;
typedef struct DvzFramePlanEmitter DvzFramePlanEmitter;
typedef struct DvzFramePlanEmitConfig DvzFramePlanEmitConfig;
typedef struct DvzFramePlan DvzFramePlan;
typedef struct DvzFramePlanNode DvzFramePlanNode;

/* Scene graph objects (opaque handles) */
typedef struct DvzScene             DvzScene;
typedef struct DvzFigure            DvzFigure;
typedef struct DvzPanel             DvzPanel;
typedef struct DvzVisual            DvzVisual;
typedef struct DvzCamera            DvzCamera;
typedef struct DvzSampledField      DvzSampledField;
typedef struct DvzSceneBuffer       DvzSceneBuffer;
typedef struct DvzInteractionPolicy DvzInteractionPolicy;
typedef struct DvzSelection         DvzSelection;
typedef struct DvzLinkChannel       DvzLinkChannel;
typedef struct DvzPinnedReadout     DvzPinnedReadout;
typedef struct DvzScale             DvzScale;
typedef struct DvzColormap          DvzColormap;
typedef struct DvzColorbar          DvzColorbar;
typedef struct DvzFont              DvzFont;
typedef struct DvzText              DvzText;
typedef struct DvzAnnotation        DvzAnnotation;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCapabilitySnapshot
{
    uint64_t max_buffer_size;
    uint32_t max_texture_dimension_2d;
    uint32_t max_bind_groups;
    uint32_t max_vertex_buffers;
    uint32_t max_color_attachments;
    bool shader_format_wgsl;
    bool shader_format_glsl;
    bool render_target_format_rgba16float;
    bool render_target_format_r16float;
    bool supports_render_target_sampling;
    bool supports_color_blending;
};



struct DvzDiagnosticReport
{
    uint32_t count;
    char messages[DVZ_SCENE_MAX_DIAGNOSTICS][DVZ_SCENE_DIAGNOSTIC_SIZE];
};


struct DvzFramePlanEmitConfig
{
    DvzSceneShaderFormat shader_format;
    bool external_color_target;
    uint64_t color_target_id;
    uint32_t target_width;
    uint32_t target_height;
    bool fullscreen_triangle;
    uint64_t runtime_resource_scope_id; /* Optional scope for mutable runtime intermediates. */
    float clear_color[4]; /* RGBA clear color for the render pass [0,1]; default opaque black */
};



struct DvzPanelDesc
{
    float x, y;           /* top-left in normalized figure coords [0, 1] */
    float width, height;  /* extent in normalized figure coords */
};
typedef struct DvzPanelDesc DvzPanelDesc;


struct DvzEdlDesc
{
    float radius;
    float strength;
    float depth_scale;
};
typedef struct DvzEdlDesc DvzEdlDesc;


struct DvzMsaaDesc
{
    bool enabled;
    uint32_t sample_count;
    bool alpha_to_coverage;
};
typedef struct DvzMsaaDesc DvzMsaaDesc;


struct DvzSsaoDesc
{
    float radius;
    float strength;
    float bias;
    float power;
    float min_visibility;
    float blur_radius;
    float blur_depth_sigma;
    float blur_normal_sigma;
    uint32_t sample_count;
    bool blur_enabled;
    bool debug_view;
};
typedef struct DvzSsaoDesc DvzSsaoDesc;


struct DvzSceneBufferDesc
{
    uint32_t usage;
    uint32_t stride;
    uint64_t byte_size;
};
typedef struct DvzSceneBufferDesc DvzSceneBufferDesc;


struct DvzPrimitiveShadingDesc
{
    float light_direction[3];
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};
typedef struct DvzPrimitiveShadingDesc DvzPrimitiveShadingDesc;


struct DvzPhongMaterial
{
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};
typedef struct DvzPhongMaterial DvzPhongMaterial;


struct DvzStandardMaterial
{
    float roughness;
    float specular;
    float metallic;
    float emissive[3];
    float rim_strength;
};
typedef struct DvzStandardMaterial DvzStandardMaterial;


struct DvzMaterialDesc
{
    DvzMaterialModel model;
    DvzAlphaMode alpha_mode;
    float opacity;
    float base_color_factor[4];
    float light_direction[3];
    DvzPhongMaterial phong;
    DvzStandardMaterial standard;
};
typedef struct DvzMaterialDesc DvzMaterialDesc;


struct DvzDepthCueDesc
{
    DvzDepthCueMode mode;
    DvzDepthCueMetric metric;
    DvzDepthCueFalloff falloff;
    float near_depth;
    float far_depth;
    float strength;
    float density;
    float background_color[4];
};
typedef struct DvzDepthCueDesc DvzDepthCueDesc;


struct DvzVolumeState
{
    float opacity;
    DvzVolumeSamplingMode sampling;
    DvzVolumeRenderMode render_mode;
    DvzVolumeAxis slice_axis;
    double slice_position;
    bool clipping_enabled;
    double clip_min[3];
    double clip_max[3];
    uint32_t step_count;
    uint64_t version;
};
typedef struct DvzVolumeState DvzVolumeState;



/* Per-visual attachment options.
 *
 * Passed to dvz_panel_add_visual() to control draw order and controller behavior
 * for a visual within a panel. Pass NULL to use defaults (z_layer=0, controller_mode=APPLY).
 *
 * Spec: spec/scene/pipeline/TRANSFORM_PIPELINE.md "Visual Attachment And Coordinate Space".
 */
struct DvzVisualAttachDesc
{
    int32_t           z_layer;          /* signed; lower draws behind, higher in front; default 0 */
    DvzControllerMode controller_mode;  /* APPLY (default) or FIXED (ignore panzoom/arcball) */
};
typedef struct DvzVisualAttachDesc DvzVisualAttachDesc;


struct DvzVisualDataUpdate
{
    const char* attr_name;
    const void* data;
    uint32_t item_count;
};
typedef struct DvzVisualDataUpdate DvzVisualDataUpdate;



struct DvzFormatDesc
{
    int32_t precision;
    bool scientific;
    bool trim_trailing_zeros;
    bool show_unit;
    const char* unit;
    const char* prefix;
    const char* suffix;
};
typedef struct DvzFormatDesc DvzFormatDesc;


struct DvzSelectionDesc
{
    DvzSelectMode mode;
    DvzSceneTargetKind target;
    uint32_t flags;
};
typedef struct DvzSelectionDesc DvzSelectionDesc;


struct DvzSelectionItem
{
    uint64_t visual_id;
    DvzSceneTargetKind target;
    uint64_t target_id;
    uint64_t link_key;
};
typedef struct DvzSelectionItem DvzSelectionItem;


struct DvzPickRequest
{
    uint64_t request_id;
    DvzSceneTargetKind target;
    DvzPickHitPolicy hit_policy;
    uint32_t flags;
};
typedef struct DvzPickRequest DvzPickRequest;


struct DvzPickResult
{
    uint64_t request_id;
    bool hit;
    uint64_t panel_id;
    uint64_t visual_id;
    DvzSceneTargetKind raw_parent_target;
    uint64_t raw_parent_id;
    DvzSceneTargetKind raw_target;
    uint64_t raw_id;
    DvzSceneTargetKind resolved_parent_target;
    uint64_t resolved_parent_id;
    DvzSceneTargetKind resolved_target;
    uint64_t resolved_id;
    uint64_t instance_id;
    double panel_position[2];
    bool has_data_position;
    double data_position[3];
};
typedef struct DvzPickResult DvzPickResult;


struct DvzProbeRequest
{
    uint64_t request_id;
    DvzSceneTargetKind target;
    uint32_t flags;
};
typedef struct DvzProbeRequest DvzProbeRequest;


struct DvzProbeResult
{
    uint64_t request_id;
    bool hit;
    uint64_t panel_id;
    uint64_t visual_id;
    DvzSceneTargetKind target;
    uint64_t target_id;
    bool has_coordinate;
    double coordinate[3];
    DvzProbeValueKind value_kind;
    double scalar;
    double vector[4];
    uint64_t category_id;
    char label[DVZ_SCENE_LABEL_SIZE];
    char unit[32];
    DvzScale* scale;
    uint64_t source_request_id;
};
typedef struct DvzProbeResult DvzProbeResult;


struct DvzHoverState
{
    bool active;
    DvzLinkChannel* link_channel;
    DvzPickResult pick;
    DvzProbeResult probe;
};
typedef struct DvzHoverState DvzHoverState;


struct DvzScaleDesc
{
    DvzScaleKind kind;
    const char* label;
    const char* unit;
    DvzFormatDesc format;
};
typedef struct DvzScaleDesc DvzScaleDesc;


struct DvzColormapStop
{
    double position;
    uint8_t rgba[4];
};
typedef struct DvzColormapStop DvzColormapStop;


struct DvzColormapDesc
{
    DvzColormapKind kind;
    DvzBuiltinColormap builtin;
    double center;
    const char* label;
};
typedef struct DvzColormapDesc DvzColormapDesc;


struct DvzColorbarDesc
{
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    const char* title;
    uint32_t flags;
};
typedef struct DvzColorbarDesc DvzColorbarDesc;


struct DvzFontDesc
{
    const char* path;
    float size_pts;
    uint32_t flags;
};
typedef struct DvzFontDesc DvzFontDesc;


struct DvzTextStyle
{
    DvzFont* font;
    float size_pts;
    uint8_t color[4];
    uint32_t flags;
    bool bold;
    bool italic;
    bool underline;
};
typedef struct DvzTextStyle DvzTextStyle;


struct DvzTextPlacement
{
    DvzTextPlacementMode mode;
    DvzSceneAnchor anchor;
    double position[3];
    float offset[2];
};
typedef struct DvzTextPlacement DvzTextPlacement;


struct DvzTextDesc
{
    const char* string;
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
};
typedef struct DvzTextDesc DvzTextDesc;


struct DvzAnnotationDesc
{
    DvzAnnotationKind kind;
    const char* text;
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
};
typedef struct DvzAnnotationDesc DvzAnnotationDesc;


struct DvzLabelDesc
{
    const char* text;
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
};
typedef struct DvzLabelDesc DvzLabelDesc;
