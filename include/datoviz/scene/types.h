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

#include "datoviz/font.h"
#include "datoviz/math/types.h"
#include "datoviz/scene/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_LABEL_SIZE 128
#define DVZ_SCENE_MAX_NODE_RESOURCES 8
#define DVZ_SCENE_MAX_RENDER_VISUALS 128
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
typedef struct DvzGrid              DvzGrid;
typedef struct DvzPanel             DvzPanel;
typedef struct DvzVisual            DvzVisual;
typedef struct DvzComposite         DvzComposite;
typedef struct DvzPolygon           DvzPolygon;
typedef struct DvzPolygonSet        DvzPolygonSet;
typedef struct DvzCamera            DvzCamera;
typedef struct DvzController        DvzController;
typedef struct DvzControllerLink    DvzControllerLink;
typedef struct DvzSampledField      DvzSampledField;
typedef struct DvzSceneBuffer       DvzSceneBuffer;
typedef struct DvzInteractionPolicy DvzInteractionPolicy;
typedef struct DvzSelection         DvzSelection;
typedef struct DvzLinkChannel       DvzLinkChannel;
typedef struct DvzPinnedReadout     DvzPinnedReadout;
typedef struct DvzScale             DvzScale;
typedef struct DvzColormap          DvzColormap;
typedef struct DvzColorbar          DvzColorbar;
typedef struct DvzLegend            DvzLegend;
typedef struct DvzFont              DvzFont;
typedef struct DvzText              DvzText;
typedef struct DvzAnnotation        DvzAnnotation;
typedef struct DvzAxis              DvzAxis;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_DIM_X = 0,
    DVZ_DIM_Y = 1,
    DVZ_DIM_Z = 2,
} DvzDim;


typedef uint32_t DvzDimMask;


typedef enum
{
    DVZ_DIM_MASK_NONE = 0x00u,
    DVZ_DIM_MASK_X    = 0x01u,
    DVZ_DIM_MASK_Y    = 0x02u,
    DVZ_DIM_MASK_Z    = 0x04u,
    DVZ_DIM_MASK_XY   = DVZ_DIM_MASK_X | DVZ_DIM_MASK_Y,
    DVZ_DIM_MASK_XYZ  = DVZ_DIM_MASK_X | DVZ_DIM_MASK_Y | DVZ_DIM_MASK_Z,
} DvzDimMaskFlag;


typedef enum
{
    DVZ_CONTROLLER_TYPE_NONE      = 0,
    DVZ_CONTROLLER_TYPE_PANZOOM   = 1,
    DVZ_CONTROLLER_TYPE_ARCBALL   = 2,
    DVZ_CONTROLLER_TYPE_FLY       = 3,
    DVZ_CONTROLLER_TYPE_TURNTABLE = 4,
} DvzControllerType;



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
    uint32_t max_color_sample_count;
    uint32_t max_depth_sample_count;
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
    float device_scale_x;
    float device_scale_y;
    float render_scale;
    float user_scale;
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


struct DvzGridCell
{
    uint32_t row;
    uint32_t col;
    uint32_t row_span;
    uint32_t col_span;
};
typedef struct DvzGridCell DvzGridCell;


typedef enum
{
    DVZ_PANEL_BACKGROUND_NONE = 0,
    DVZ_PANEL_BACKGROUND_COLOR,
    DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT,
    DVZ_PANEL_BACKGROUND_IMAGE,
} DvzPanelBackgroundType;


struct DvzPanelBackgroundDesc
{
    DvzPanelBackgroundType type;
    float color[4]; /* RGBA in [0, 1], used by DVZ_PANEL_BACKGROUND_COLOR */

    struct
    {
        float start[2]; /* panel-local coordinates in [0, 1] */
        float end[2];   /* panel-local coordinates in [0, 1] */
        float color0[4];
        float color1[4];
    } gradient;

    struct
    {
        const void* rgba; /* tightly packed RGBA8 pixels */
        uint32_t width;
        uint32_t height;
    } image;
};
typedef struct DvzPanelBackgroundDesc DvzPanelBackgroundDesc;


struct DvzRect
{
    float x;
    float y;
    float width;
    float height;
};
typedef struct DvzRect DvzRect;


struct DvzPanelReserve
{
    float left_px;
    float right_px;
    float top_px;
    float bottom_px;
};
typedef struct DvzPanelReserve DvzPanelReserve;


struct DvzDataDomain
{
    double min;
    double max;
};
typedef struct DvzDataDomain DvzDataDomain;


struct DvzAxisTickPolicy
{
    uint32_t target_count;
    float min_pixel_spacing;
    uint32_t minor_per_interval;
};
typedef struct DvzAxisTickPolicy DvzAxisTickPolicy;


struct DvzAxisStyle
{
    float spine_width;
    float major_tick_width;
    float minor_tick_width;
    float grid_width;
    float major_tick_length;
    float minor_tick_length;
    float reserve_px;
    float tick_gap_px;
    float label_gap_px;
    float tick_size_px;
    float label_size_px;
    DvzTextRenderer text_renderer;
    float plot_margin_left;
    float plot_margin_right;
    float plot_margin_bottom;
    float plot_margin_top;
    uint8_t spine_color[4];
    uint8_t major_tick_color[4];
    uint8_t minor_tick_color[4];
    uint8_t grid_color[4];
    bool show_spine;
    bool show_major_ticks;
    bool show_minor_ticks;
    bool show_grid;
};
typedef struct DvzAxisStyle DvzAxisStyle;


struct DvzPanelLayoutReserve
{
    float left;
    float right;
    float bottom;
    float top;
};
typedef struct DvzPanelLayoutReserve DvzPanelLayoutReserve;


struct DvzPlacement
{
    DvzPlacementSpace space;
    DvzHorizontalAnchor horizontal_anchor;
    DvzVerticalAnchor vertical_anchor;
    float offset_x_px;
    float offset_y_px;
    float width_px;
    float height_px;
};
typedef struct DvzPlacement DvzPlacement;


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


struct DvzVolumeOcclusionDesc
{
    bool enabled;
    float alpha_threshold;
    float fade_distance;
    float occluded_alpha;
};
typedef struct DvzVolumeOcclusionDesc DvzVolumeOcclusionDesc;


struct DvzSceneOcclusionDesc
{
    bool enabled;
    float depth_bias;
    float soft_edge;
    float hidden_alpha;
};
typedef struct DvzSceneOcclusionDesc DvzSceneOcclusionDesc;


struct DvzSceneBufferDesc
{
    uint32_t usage;
    uint32_t stride;
    uint64_t byte_size;
};
typedef struct DvzSceneBufferDesc DvzSceneBufferDesc;


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


struct DvzPointStyleDesc
{
    DvzColor edge_color;
    float stroke_width;
    DvzShapeAspect aspect;
};
typedef struct DvzPointStyleDesc DvzPointStyleDesc;


struct DvzMarkerStyle
{
    DvzColor edge_color;
    float stroke_width;
    DvzShapeAspect aspect;
};
typedef struct DvzMarkerStyle DvzMarkerStyle;


struct DvzVolumeAlphaStop
{
    double position;
    float alpha;
};
typedef struct DvzVolumeAlphaStop DvzVolumeAlphaStop;


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
    bool clip_plane_enabled;
    bool clip_plane_keep_positive;
    double clip_plane_point[3];
    double clip_plane_normal[3];
    double bounds_min[3];
    double bounds_max[3];
    uint32_t axis_order[3];
    bool axis_flip[3];
    double value_min;
    double value_max;
    DvzVolumeAlphaStop alpha_stops[8];
    uint32_t alpha_stop_count;
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
    DvzControllerMode controller_mode;  /* APPLY (default), FIXED, or shader isotropic local */
};
typedef struct DvzVisualAttachDesc DvzVisualAttachDesc;


struct DvzVisualDataUpdate
{
    const char* attr_name;
    const void* data;
    uint32_t item_count;
};
typedef struct DvzVisualDataUpdate DvzVisualDataUpdate;


struct DvzVisualDataView
{
    const void* data;
    uint64_t item_count;
    uint32_t item_size;
    DvzVisualAttrSource source;
    DvzVisualAttrMutability mutability;
    uint64_t version;
};
typedef struct DvzVisualDataView DvzVisualDataView;


typedef enum
{
    DVZ_BOUNDS_SPACE_VISUAL = 0,
    DVZ_BOUNDS_SPACE_SCREEN = 1,
} DvzBoundsSpace;


struct DvzBounds
{
    bool valid;
    uint32_t dims;
    double min[3];
    double max[3];
};
typedef struct DvzBounds DvzBounds;



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
    DvzPickStatus status;
    bool hit;
    uint64_t panel_id;
    uint64_t visual_id;
    DvzSceneVisualFamily visual_family;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
    DvzSceneTargetKind raw_parent_target;
    uint64_t raw_parent_id;
    DvzSceneTargetKind raw_target;
    uint64_t raw_id;
    DvzSceneTargetKind resolved_parent_target;
    uint64_t resolved_parent_id;
    DvzSceneTargetKind resolved_target;
    uint64_t resolved_id;
    uint64_t instance_id;
    uint64_t link_key;
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
    DvzProbeStatus status;
    bool hit;
    uint64_t panel_id;
    uint64_t visual_id;
    DvzSceneVisualFamily visual_family;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
    DvzSceneTargetKind target;
    uint64_t target_id;
    double panel_position[2];
    bool has_coordinate;
    double coordinate[3];
    bool has_uvw;
    double uvw[3];
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


struct DvzScaleCategory
{
    int32_t category_id;
    uint32_t order;
    const char* label;
    DvzColor color;
    uint32_t flags;
};
typedef struct DvzScaleCategory DvzScaleCategory;


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
    DvzColorbarPlacementMode placement_mode;
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    const char* title;
    float reserve_px;
    float ramp_width_px;
    float edge_offset_px;
    float plot_gap_px;
    float tick_length_px;
    float label_gap_px;
    DvzPlacement placement;
    uint32_t flags;
};
typedef struct DvzColorbarDesc DvzColorbarDesc;


struct DvzLegendDesc
{
    DvzLegendPlacementMode placement_mode;
    DvzSceneAnchor anchor;
    const char* title;
    float reserve_px;
    float edge_offset_px;
    float plot_gap_px;
    float entry_gap_px;
    float mark_size_px;
    float mark_label_gap_px;
    DvzPlacement placement;
    uint32_t flags;
};
typedef struct DvzLegendDesc DvzLegendDesc;


struct DvzTextStyle
{
    DvzFont* font;
    float size_px;
    DvzTextRenderer renderer;
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
    float text_anchor[2];
    bool has_text_anchor;
    float angle;
    bool depth_test;
};
typedef struct DvzTextPlacement DvzTextPlacement;


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


struct DvzScaleBarDesc
{
    DvzDim dimension;
    DvzSceneAnchor anchor;
    DvzScaleBarReferenceMode reference_mode;
    DvzScaleBarLabelPosition label_position;
    DvzTextStyle label_style;
    DvzTextPlacement placement;
    DvzFormatDesc format;
    const char* unit;
    double data_to_unit;
    double reference_position[3];
    double reference_direction[3];
    float target_length_px;
    float min_length_px;
    float max_length_px;
    float offset_px[2];
    float tick_length_px;
    float line_width_px;
    uint8_t line_color[4];
    uint8_t background_color[4];
    uint32_t flags;
};
typedef struct DvzScaleBarDesc DvzScaleBarDesc;
