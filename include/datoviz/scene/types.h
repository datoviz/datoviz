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
#include "datoviz/controller/camera.h"
#include "datoviz/geom/types.h"
#include "datoviz/math/dim.h"
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
typedef struct DvzSceneFrameArtifact DvzSceneFrameArtifact;
typedef struct DvzVisualTransformDesc DvzVisualTransformDesc;
typedef struct DvzVisualShaderDesc DvzVisualShaderDesc;

/* Scene graph objects (opaque handles) */
typedef struct DvzScene             DvzScene;
typedef struct DvzFigure            DvzFigure;
typedef struct DvzGrid              DvzGrid;
typedef struct DvzPanel             DvzPanel;
typedef struct DvzPanelFrameSnapshot DvzPanelFrameSnapshot;
typedef struct DvzVisual            DvzVisual;
typedef struct DvzSceneCompute      DvzSceneCompute;
typedef struct DvzComposite         DvzComposite;
typedef struct DvzPolygon           DvzPolygon;
typedef struct DvzPolygonSet        DvzPolygonSet;
typedef struct DvzGraph             DvzGraph;
typedef struct DvzCamera            DvzCamera;
typedef struct DvzController        DvzController;
typedef struct DvzControllerLink    DvzControllerLink;
typedef struct DvzSampledField      DvzSampledField;
typedef struct DvzSceneBuffer       DvzSceneBuffer;
typedef struct DvzInteractionPolicy DvzInteractionPolicy;
typedef struct DvzSelection         DvzSelection;
typedef struct DvzHover             DvzHover;
typedef struct DvzItemInteraction   DvzItemInteraction;
typedef struct DvzLinkChannel       DvzLinkChannel;
typedef struct DvzPinnedReadout     DvzPinnedReadout;
typedef struct DvzOverlay           DvzOverlay;
typedef struct DvzOverlayCard       DvzOverlayCard;
typedef struct DvzScale             DvzScale;
typedef struct DvzColormap          DvzColormap;
typedef struct DvzColorbar          DvzColorbar;
typedef struct DvzLegend            DvzLegend;
typedef struct DvzOrientationGizmo  DvzOrientationGizmo;
typedef struct DvzReferenceGrid     DvzReferenceGrid;
typedef struct DvzFont              DvzFont;
typedef struct DvzText              DvzText;
typedef struct DvzTextAtlas         DvzTextAtlas;
typedef struct DvzSymbolSet         DvzSymbolSet;
typedef struct DvzAnnotation        DvzAnnotation;
typedef struct DvzAnnotation        DvzScaleBar;
typedef struct DvzGuideLine         DvzGuideLine;
typedef struct DvzGuideSpan         DvzGuideSpan;
typedef struct DvzBars              DvzBars;
typedef struct DvzBand              DvzBand;
typedef struct DvzAxis              DvzAxis;
typedef struct DvzUnitLadder        DvzUnitLadder;
typedef struct DvzUnits             DvzUnits;
typedef struct DvzDateTimeFormat    DvzDateTimeFormat;


typedef int64_t DvzCategoryId;
typedef int64_t DvzTimestamp; /* microseconds since Unix epoch UTC */
typedef uint32_t DvzSymbolId;

#define DVZ_SYMBOL_ID_INVALID UINT32_MAX



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_UNIT_LADDER_METRIC_LENGTH = 0,
    DVZ_UNIT_LADDER_DURATION      = 1,
    DVZ_UNIT_LADDER_RAW           = 2,
} DvzUnitLadderBuiltin;


typedef enum
{
    DVZ_UNIT_DISPLAY_AUTO        = 0,
    DVZ_UNIT_DISPLAY_AXIS_STABLE = 1,
    DVZ_UNIT_DISPLAY_FIXED       = 2,
} DvzUnitDisplayMode;


typedef enum
{
    DVZ_TIME_INTERVAL_NANOSECOND  = 0,
    DVZ_TIME_INTERVAL_MICROSECOND = 1,
    DVZ_TIME_INTERVAL_MILLISECOND = 2,
    DVZ_TIME_INTERVAL_SECOND      = 3,
    DVZ_TIME_INTERVAL_MINUTE      = 4,
    DVZ_TIME_INTERVAL_HOUR        = 5,
    DVZ_TIME_INTERVAL_DAY         = 6,
    DVZ_TIME_INTERVAL_MONTH       = 7,
    DVZ_TIME_INTERVAL_YEAR        = 8,
} DvzTimeInterval;


typedef enum
{
    DVZ_DATETIME_FORMAT_CONCISE_UTC = 0,
    DVZ_DATETIME_FORMAT_ISO_UTC     = 1,
} DvzDateTimeBuiltin;


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


typedef enum
{
    DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK = 0,
    DVZ_SCENE_FRAME_ARTIFACT_STATUS_ENCODE_ERROR = 1,
} DvzSceneFrameArtifactStatus;


typedef enum
{
    DVZ_PANEL_VIEW_KIND_NONE = 0,
    DVZ_PANEL_VIEW_KIND_2D   = 1,
    DVZ_PANEL_VIEW_KIND_3D   = 2,
} DvzPanelViewKind;


typedef enum
{
    DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP = 0,
    DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP,
    DVZ_TEXT_ATLAS_BACKEND_STB_SDF,
    DVZ_TEXT_ATLAS_BACKEND_MSDF,
} DvzTextAtlasBackend;


typedef enum
{
    DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA = 0,
    DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA,
    DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB,
} DvzTextAtlasEncoding;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCapabilitySnapshot
{
    uint32_t struct_size;
    uint32_t flags;
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
    bool supports_readback;
    uint32_t min_texture_copy_bytes_per_row_alignment;
    uint64_t max_readback_size;
    bool texture_format_r32uint;
    bool texture_format_rg32uint;
    bool render_target_format_r32uint;
    bool render_target_format_rg32uint;
    bool query_profile_u32_r32;
    bool query_profile_u64_rg32;
    bool query_profile_u64_2xr32;
};



struct DvzDiagnosticReport
{
    uint32_t count;
    char messages[DVZ_SCENE_MAX_DIAGNOSTICS][DVZ_SCENE_DIAGNOSTIC_SIZE];
};


struct DvzFramePlanEmitConfig
{
    uint32_t struct_size;
    uint32_t flags;
    DvzSceneShaderFormat shader_format;
    DvzColorPipeline color_pipeline;
    bool external_color_target;
    uint64_t color_target_id;
    DvzFormat color_target_format;
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


struct DvzVisualTransformDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzVisualTransformKind kind;
    DvzVisualTransformSpace input_space;
    DvzVisualTransformSpace output_space;
    uint64_t transform_id;
    const char* label;
    mat4 matrix;
};


struct DvzVisualShaderDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzVisualShaderKind kind;
    DvzVisualShaderSource vertex_source;
    DvzVisualShaderSource fragment_source;
    uint64_t shader_id;
    const char* family;
    const char* variant;
    const char* label;
    const void* vertex_code;
    uint64_t vertex_code_size;
    const void* fragment_code;
    uint64_t fragment_code_size;
};


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
    uint32_t struct_size;
    uint32_t flags;
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


typedef enum
{
    DVZ_GUIDE_ORIENTATION_HORIZONTAL = 0,
    DVZ_GUIDE_ORIENTATION_VERTICAL   = 1,
} DvzGuideOrientation;


typedef enum
{
    DVZ_PLOT_ROLE_FILL = 0,
    DVZ_PLOT_ROLE_LINE,
    DVZ_PLOT_ROLE_OUTLINE,
    DVZ_PLOT_ROLE_BOUNDS,
} DvzPlotRole;


struct DvzGuideLineDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzGuideOrientation orientation;
    double value;
    float stroke_width_px;
    DvzSegmentCap cap_start;
    DvzSegmentCap cap_end;
    DvzColor color;
    int32_t z_layer;
    const char* label;
};
typedef struct DvzGuideLineDesc DvzGuideLineDesc;


struct DvzGuideSpanDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzGuideOrientation orientation;
    double min_value;
    double max_value;
    DvzColor fill_color;
    DvzColor outline_color;
    float outline_width_px;
    int32_t z_layer;
    const char* label;
};
typedef struct DvzGuideSpanDesc DvzGuideSpanDesc;


typedef enum
{
    DVZ_BARS_ORIENTATION_VERTICAL = 0,
    DVZ_BARS_ORIENTATION_HORIZONTAL = 1,
} DvzBarsOrientation;


struct DvzBarsDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzBarsOrientation orientation;
    double baseline;
    float gap_fraction;
    DvzColor fill_color;
    DvzColor outline_color;
    float outline_width_px;
    int32_t z_layer;
};
typedef struct DvzBarsDesc DvzBarsDesc;


struct DvzBandDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzColor fill_color;
    DvzColor line_color;
    float line_width_px;
    bool show_line;
    bool show_bounds;
    DvzColor bound_color;
    float bound_width_px;
    int32_t z_layer;
};
typedef struct DvzBandDesc DvzBandDesc;


struct DvzPanelBorderDesc
{
    uint32_t struct_size;
    uint32_t flags;
    bool visible;
    DvzColor color;
    float width_px;
    float inset_px;
};
typedef struct DvzPanelBorderDesc DvzPanelBorderDesc;


struct DvzRect
{
    float x;
    float y;
    float width;
    float height;
};
typedef struct DvzRect DvzRect;


struct DvzPanelFrameInfo
{
    uint32_t struct_size;
    uint32_t flags;
    DvzId snapshot_id;
    DvzId figure_id;
    DvzId panel_id;
    DvzId view_id;
    DvzPanelViewKind view_kind;
    uint64_t panel_revision;
    uint64_t layout_revision;
    uint64_t view_revision;
    uint64_t guide_revision;
    uint64_t visual_revision;
    uint32_t logical_width_px;
    uint32_t logical_height_px;
    float framebuffer_width_px;
    float framebuffer_height_px;
    float device_scale_x;
    float device_scale_y;
    float user_scale;
    DvzRect panel_rect_px;
    DvzRect inner_rect_px;
    DvzRect plot_rect_px;
    DvzRect grid_clip_rect_px;
    float plot_view[4];
    float view_extent[4];
    float controller_extent[4];
    double source_data_x[2];
    double source_data_y[2];
    double visible_data_x[2];
    double visible_data_y[2];
    uint64_t data_to_view_padding;
    mat4 data_to_view;
    bool has_view2d;
    bool has_valid_source_x;
    bool has_valid_source_y;
    bool has_valid_visible_x;
    bool has_valid_visible_y;
    DvzDiagnosticReport diagnostics;
};
typedef struct DvzPanelFrameInfo DvzPanelFrameInfo;


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


typedef enum
{
    DVZ_PANEL_VIEW2D_NONE = 0,
    DVZ_PANEL_VIEW2D_CONTAIN,
} DvzPanelView2DMode;


typedef enum
{
    DVZ_PANEL_VIEW2D_ASPECT_FREE = 0,
    DVZ_PANEL_VIEW2D_ASPECT_EQUAL,
} DvzPanelView2DAspect;


struct DvzPanelView2D
{
    uint32_t struct_size;
    uint32_t flags;
    DvzPanelView2DMode mode;
    DvzPanelView2DAspect aspect;
    double padding;
};
typedef struct DvzPanelView2D DvzPanelView2D;


struct DvzPanelView2DDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzPanelView2DMode mode;
    DvzPanelView2DAspect aspect;
    double padding;
    double domain_x[2];
    double domain_y[2];
    bool has_domain_x;
    bool has_domain_y;
};
typedef struct DvzPanelView2DDesc DvzPanelView2DDesc;


struct DvzPanelView2DState
{
    uint32_t struct_size;
    uint32_t flags;
    DvzId view_id;
    uint64_t revision;
    bool enabled;
    DvzPanelView2DMode mode;
    DvzPanelView2DAspect aspect;
    double padding;
    double domain_x[2];
    double domain_y[2];
    bool has_domain_x;
    bool has_domain_y;
    float view_extent[4];
    uint32_t data_to_view_padding[3];
    mat4 data_to_view;
    bool has_valid_source_x;
    bool has_valid_source_y;
};
typedef struct DvzPanelView2DState DvzPanelView2DState;


struct DvzPanelView3DDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzCameraDesc camera;
};
typedef struct DvzPanelView3DDesc DvzPanelView3DDesc;


struct DvzPanelView3DState
{
    uint32_t struct_size;
    uint32_t flags;
    DvzId view_id;
    uint64_t revision;
    bool enabled;
    DvzCameraView view;
    DvzCameraProjection projection;
    bool has_explicit_orthographic_bounds;
    float orthographic_bounds[6]; /* left, right, bottom, top, near, far */
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};
typedef struct DvzPanelView3DState DvzPanelView3DState;


struct DvzAxisTickPolicy
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t target_count;
    float min_pixel_spacing;
    uint32_t minor_per_interval;
};
typedef struct DvzAxisTickPolicy DvzAxisTickPolicy;


struct DvzAxisTicks
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t count;
    const double* values;
    const char* const* labels;
};
typedef struct DvzAxisTicks DvzAxisTicks;


struct DvzAxisStyle
{
    uint32_t struct_size;
    uint32_t flags;
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


struct DvzPanelAxes2DDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const char* x_label;
    const char* y_label;
    DvzAxisTickPolicy tick_policy;
    DvzAxisStyle x_style;
    DvzAxisStyle y_style;
};
typedef struct DvzPanelAxes2DDesc DvzPanelAxes2DDesc;


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


struct DvzOrientationGizmoDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzPlacement placement;
    bool show_axes;
    float axis_length;
    float axis_width_px;
    DvzColor x_color;
    DvzColor y_color;
    DvzColor z_color;
};
typedef struct DvzOrientationGizmoDesc DvzOrientationGizmoDesc;


struct DvzReferenceGridDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzReferenceGridPlane plane;
    vec3 origin;
    vec3 axis_u;
    vec3 axis_v;
    float size[2];
    float spacing;
    uint32_t major_every;
    DvzColor minor_color;
    DvzColor major_color;
    DvzColor axis_color;
    float minor_width_px;
    float major_width_px;
    float axis_width_px;
    bool show_minor;
    bool show_major;
    bool show_axes;
    bool depth_test;
};
typedef struct DvzReferenceGridDesc DvzReferenceGridDesc;


struct DvzEdlDesc
{
    uint32_t struct_size;
    uint32_t flags;
    float radius;
    float strength;
    float depth_scale;
};
typedef struct DvzEdlDesc DvzEdlDesc;


struct DvzMsaaDesc
{
    uint32_t struct_size;
    uint32_t flags;
    bool enabled;
    uint32_t sample_count;
    bool alpha_to_coverage;
};
typedef struct DvzMsaaDesc DvzMsaaDesc;


struct DvzSsaoDesc
{
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t struct_size;
    uint32_t flags;
    bool enabled;
    float alpha_threshold;
    float fade_distance;
    float occluded_alpha;
};
typedef struct DvzVolumeOcclusionDesc DvzVolumeOcclusionDesc;


struct DvzSceneOcclusionDesc
{
    uint32_t struct_size;
    uint32_t flags;
    bool enabled;
    float depth_bias;
    float soft_edge;
    float hidden_alpha;
};
typedef struct DvzSceneOcclusionDesc DvzSceneOcclusionDesc;


struct DvzVectorStyle
{
    uint32_t struct_size;
    uint32_t flags;
    float scale;
    DvzVectorAnchor anchor;
    DvzSegmentCap start_cap;
    DvzSegmentCap end_cap;
    DvzPathJoin join;
    float miter_limit;
};
typedef struct DvzVectorStyle DvzVectorStyle;


struct DvzPolygonStyle
{
    uint32_t struct_size;
    uint32_t flags;
    bool visible;
    DvzColor fill_color;
    DvzColor stroke_color;
    float stroke_width_px;
    DvzSegmentCap stroke_start_cap;
    DvzSegmentCap stroke_end_cap;
    DvzPathJoin stroke_join;
    float stroke_miter_limit;
};
typedef struct DvzPolygonStyle DvzPolygonStyle;


struct DvzGraphEdgeStyle
{
    uint32_t struct_size;
    uint32_t flags;
    DvzGraphEdgeMode mode;
    DvzBezierTessellationDesc tessellation;
    DvzSegmentCap start_cap;
    DvzSegmentCap end_cap;
    DvzPathJoin join;
    float miter_limit;
};
typedef struct DvzGraphEdgeStyle DvzGraphEdgeStyle;


struct DvzSceneBufferDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t usage;
    uint32_t stride;
    uint64_t byte_size;
};
typedef struct DvzSceneBufferDesc DvzSceneBufferDesc;


typedef enum
{
    DVZ_SCENE_COMPUTE_ACCESS_READ = 0,
    DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE,
} DvzSceneComputeAccess;


struct DvzSceneComputeDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const char* label;
    DvzSceneShaderFormat shader_format;
    const char* shader_source;
    const char* entry_point;
    uint32_t dispatch[3];
};
typedef struct DvzSceneComputeDesc DvzSceneComputeDesc;


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
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t struct_size;
    uint32_t flags;
    DvzColor edge_color;
    float stroke_width_px;
    DvzShapeAspect aspect;
};
typedef struct DvzPointStyleDesc DvzPointStyleDesc;


struct DvzMarkerStyle
{
    uint32_t struct_size;
    uint32_t flags;
    DvzColor edge_color;
    float stroke_width_px;
    DvzShapeAspect aspect;
};
typedef struct DvzMarkerStyle DvzMarkerStyle;


struct DvzSymbolImageDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t row_stride;
    float distance_range_px;
};
typedef struct DvzSymbolImageDesc DvzSymbolImageDesc;


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


#define DVZ_LABELS_MAX_HIDDEN 256


struct DvzLabelsState
{
    float opacity;
    DvzCategoryId background_id;
    bool selected_enabled;
    DvzCategoryId selected_id;
    DvzCategoryId hidden_ids[DVZ_LABELS_MAX_HIDDEN];
    uint32_t hidden_count;
    bool boundary_enabled;
    float boundary_width_px;
    DvzColor boundary_color;
    uint32_t fallback_seed;
    DvzVolumeAxis slice_axis;
    double slice_position;
    uint64_t version;
};
typedef struct DvzLabelsState DvzLabelsState;



/* Per-visual attachment options.
 *
 * Passed to dvz_panel_add_visual() to control draw order, controller behavior, coordinate
 * interpretation, and optional scissor/viewport selection for a visual within a panel. Pass NULL
 * to use defaults (z_layer=0, controller_mode=APPLY, coord_space=DATA, clip_rect=AUTO,
 * viewport_rect=AUTO).
 *
 * Spec: spec/scene/pipeline/TRANSFORM_PIPELINE.md "Visual Attachment And Coordinate Space".
 */
struct DvzVisualAttachDesc
{
    uint32_t struct_size;
    uint32_t flags;
    int32_t           z_layer;          /* signed; lower draws behind, higher in front; default 0 */
    DvzControllerMode controller_mode;  /* APPLY (default), FIXED, VIEW_PROJ, or isotropic */
    DvzVisualCoordSpace coord_space;    /* DATA/domain (default), VIEW, or PANEL coordinates */
    DvzVisualClipRect clip_rect;        /* AUTO (default), PANEL, or PLOT scissor */
    DvzVisualViewportRect viewport_rect; /* AUTO (default), PANEL, PLOT, or TARGET viewport */
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


struct DvzItemRange
{
    uint32_t first_item;
    uint32_t item_count;
};
typedef struct DvzItemRange DvzItemRange;


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
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t struct_size;
    uint32_t flags;
    DvzSelectMode mode;
    DvzSceneTargetKind target;
    uint32_t selection_flags;
};
typedef struct DvzSelectionDesc DvzSelectionDesc;


struct DvzItemStateVisualStyle
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t visual_flags;
    float alpha;
    DvzColor tint;
    float tint_mix;
    float scale;
};
typedef struct DvzItemStateVisualStyle DvzItemStateVisualStyle;


struct DvzSelectionVisualStyle
{
    uint32_t struct_size;
    uint32_t flags;
    DvzItemStateVisualStyle selected;
    DvzItemStateVisualStyle unselected;
};
typedef struct DvzSelectionVisualStyle DvzSelectionVisualStyle;


struct DvzHoverDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzSceneTargetKind target;
    DvzQueryHitPolicy hit_policy;
    uint32_t hover_flags;
};
typedef struct DvzHoverDesc DvzHoverDesc;


struct DvzItemInteractionDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzHover* hover;
    DvzSelection* selection;
    bool hover_enabled;
    bool selection_enabled;
    DvzSelectMode select_mode;
    DvzSceneTargetKind target;
    DvzQueryHitPolicy hit_policy;
    bool clear_hover_on_miss;
    bool clear_selection_on_miss;
};
typedef struct DvzItemInteractionDesc DvzItemInteractionDesc;


struct DvzSelectionItem
{
    uint64_t visual_id;
    DvzSceneTargetKind target;
    uint64_t target_id;
    uint64_t link_key;
};
typedef struct DvzSelectionItem DvzSelectionItem;


struct DvzQueryRequest
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t request_id;
    DvzSceneTargetKind target;
    DvzQueryHitPolicy hit_policy;
    DvzQueryProfile profile;
};
typedef struct DvzQueryRequest DvzQueryRequest;


struct DvzQueryResult
{
    uint64_t request_id;
    uint64_t freshness_serial;
    DvzQueryStatus status;
    bool hit;
    DvzId scene_id;
    DvzId figure_id;
    uint64_t panel_id;
    /* Panel-local logical pixels, origin at the outer panel rectangle. */
    double panel_position[2];
    uint32_t framebuffer_position[2];
    uint64_t visual_id;
    DvzSceneVisualFamily visual_family;
    DvzQueryProfile profile;
    uint32_t payload_version;
    DvzSceneTargetKind raw_parent_target;
    uint64_t raw_parent_id;
    DvzSceneTargetKind raw_target;
    uint64_t raw_id;
    DvzSceneTargetKind resolved_parent_target;
    uint64_t resolved_parent_id;
    DvzSceneTargetKind resolved_target;
    uint64_t resolved_id;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
    uint64_t instance_id;
    uint64_t face_id;
    uint64_t primitive_id;
    uint64_t vertex_id;
    uint64_t voxel_id;
    uint64_t texel_id;
    uint64_t link_key;
    uint32_t link_channel;
    bool has_visual_position;
    double visual_position[3];
    bool has_data_position;
    double data_position[3];
    bool has_uvw;
    double uvw[3];
    bool has_depth;
    double depth;
    bool has_display_rgba;
    double display_rgba[4];
    DvzQueryValueKind value_kind;
    double scalar;
    double vector[4];
    DvzCategoryId category_id;
    char label[DVZ_SCENE_LABEL_SIZE];
    char unit[32];
    /* Borrowed scene-owned scale associated with the result, or NULL. Valid only while the source
     * scene and scale remain alive; do not use across scenes or after scale destruction. */
    DvzScale* scale;
};
typedef struct DvzQueryResult DvzQueryResult;


struct DvzHoverState
{
    bool active;
    DvzLinkChannel* link_channel;
    DvzQueryResult query;
};
typedef struct DvzHoverState DvzHoverState;


struct DvzScaleDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzScaleKind kind;
    const char* label;
    const char* unit;
    DvzFormatDesc format;
};
typedef struct DvzScaleDesc DvzScaleDesc;


struct DvzScaleCategory
{
    DvzCategoryId category_id;
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
    uint32_t struct_size;
    uint32_t flags;
    DvzColormapKind kind;
    DvzBuiltinColormap builtin;
    double center;
    const char* label;
};
typedef struct DvzColormapDesc DvzColormapDesc;


struct DvzColorbarDesc
{
    uint32_t struct_size;
    uint32_t flags;
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
    DvzTextRenderer text_renderer;
    DvzPlacement placement;
    uint32_t colorbar_flags;
};
typedef struct DvzColorbarDesc DvzColorbarDesc;


struct DvzColorbarTicks
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t count;
    const double* values;
    const char* const* labels;
};
typedef struct DvzColorbarTicks DvzColorbarTicks;


struct DvzLegendDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzLegendPlacementMode placement_mode;
    DvzSceneAnchor anchor;
    const char* title;
    float reserve_px;
    float edge_offset_px;
    float plot_gap_px;
    float entry_gap_px;
    float mark_size_px;
    float mark_label_gap_px;
    DvzTextRenderer text_renderer;
    DvzPlacement placement;
    uint32_t legend_flags;
};
typedef struct DvzLegendDesc DvzLegendDesc;


struct DvzTextStyle
{
    uint32_t struct_size;
    uint32_t flags;
    DvzFont* font;
    float size_px;
    DvzTextRenderer renderer;
    uint8_t color[4];
    uint32_t style_flags;
    bool bold;
    bool italic;
    bool underline;
};
typedef struct DvzTextStyle DvzTextStyle;


struct DvzTextAtlasSpec
{
    DvzTextAtlasBackend backend;
    float em_px;
    float distance_range_px;
    uint32_t flags;
};
typedef struct DvzTextAtlasSpec DvzTextAtlasSpec;


struct DvzTextAtlasInfo
{
    DvzTextAtlasSpec spec;
    DvzTextAtlasBackend backend;
    DvzTextAtlasEncoding encoding;
    uint32_t width;
    uint32_t height;
    uint32_t glyph_count;
    uint32_t channels;
    float em_px;
    float distance_range_px;
    float ascent;
    float descent;
    float line_gap;
    float line_height;
    uint32_t missing_glyph_count;
    uint64_t generation;
};
typedef struct DvzTextAtlasInfo DvzTextAtlasInfo;


struct DvzTextAtlasGlyph
{
    uint32_t codepoint;
    uint32_t glyph_id;
    float advance;
    float xoff;
    float yoff;
    float width;
    float height;
    float plane_bounds[4];
    float atlas_bounds[4];
    float uv[4];
    bool valid;
};
typedef struct DvzTextAtlasGlyph DvzTextAtlasGlyph;


struct DvzTextPlacement
{
    uint32_t struct_size;
    uint32_t flags;
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


struct DvzTextItem
{
    uint32_t struct_size;
    uint32_t flags;
    const char* string;
    double position[3];
    float offset[2];
    float anchor[2];
    float size_px;
    DvzColor color;
    float angle;
};
typedef struct DvzTextItem DvzTextItem;


struct DvzTextLayout
{
    uint32_t struct_size;
    uint32_t flags;
    float line_height;
    float line_gap_px;
    float wrap_width_px;
    DvzTextAlign align;
};
typedef struct DvzTextLayout DvzTextLayout;


struct DvzAnnotationDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzAnnotationKind kind;
    const char* text;
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t annotation_flags;
};
typedef struct DvzAnnotationDesc DvzAnnotationDesc;


struct DvzLabelDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const char* text;
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t label_flags;
};
typedef struct DvzLabelDesc DvzLabelDesc;


struct DvzScaleBarDesc
{
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t scalebar_flags;
};
typedef struct DvzScaleBarDesc DvzScaleBarDesc;
