/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene internal types                                                                         */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_frame_plan.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/types.h"
#include "datoviz/geom/types.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/animation.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/controller/camera.h"
#include "datoviz/scene/enums.h"
#include "datoviz/controller/fly.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/scene/overlay.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/controller/turntable.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_MAX_FIGURES    16
#define DVZ_SCENE_MAX_GRIDS      16
#define DVZ_SCENE_MAX_GRID_ROWS  32
#define DVZ_SCENE_MAX_GRID_COLS  32
#define DVZ_SCENE_MAX_PANELS     64
#define DVZ_SCENE_MAX_VISUALS    256
#define DVZ_SCENE_MAX_COMPOSITES 128
#define DVZ_SCENE_MAX_POLYGONS   64
#define DVZ_SCENE_MAX_POLYGON_SETS 32
#define DVZ_SCENE_MAX_FIELDS     128
#define DVZ_SCENE_MAX_BUFFERS    128
#define DVZ_SCENE_MAX_SCALES     64
#define DVZ_SCENE_MAX_COLORMAPS  64
#define DVZ_SCENE_MAX_COLORBARS  64
#define DVZ_SCENE_MAX_LEGENDS    64
#define DVZ_SCENE_MAX_SCALE_CATEGORIES 4096
#define DVZ_SCENE_MAX_LEGEND_TEXTS (DVZ_SCENE_MAX_SCALE_CATEGORIES + 1)
#define DVZ_SCENE_MAX_INTERACTIONS 64
#define DVZ_SCENE_MAX_SELECTIONS 64
#define DVZ_SCENE_MAX_LINK_CHANNELS 64
#define DVZ_SCENE_MAX_PINNED_READOUTS 128
#define DVZ_SCENE_MAX_OVERLAYS 64
#define DVZ_SCENE_MAX_OVERLAY_CARDS 128
#define DVZ_SCENE_MAX_FONTS 64
#define DVZ_SCENE_MAX_TEXTS 128
#define DVZ_SCENE_MAX_ANNOTATIONS 128
#define DVZ_SCENE_MAX_PANEL_COLORBARS 16
#define DVZ_SCENE_MAX_PANEL_LEGENDS 16
#define DVZ_SCENE_MAX_COLORBAR_TICKS 16
#define DVZ_SCENE_MAX_COLORBAR_TEXTS (DVZ_SCENE_MAX_COLORBAR_TICKS + 1)
#define DVZ_SCENE_MAX_COLOR_STOPS 32
#define DVZ_SCENE_MAX_ITEM_ATTRS 8
#define DVZ_SCENE_MAX_VISUAL_BINDINGS 3
#define DVZ_SCENE_MAX_SELECTION_ITEMS 1024
#define DVZ_SCENE_MAX_PICK_RESULTS 128
#define DVZ_SCENE_MAX_PROBE_RESULTS 128
#define DVZ_SCENE_MAX_QUERY_RESULTS 128
#define DVZ_SCENE_MAX_PENDING_REQUESTS 128
#define DVZ_SCENE_MAX_REQUEST_SCOPES 256
#define DVZ_SCENE_MAX_ANIMATIONS 128
#define DVZ_SCENE_MAX_AXIS_TICKS 64
#define DVZ_SCENE_MAX_AXIS_MINOR_TICKS 8
#define DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS 16
#define DVZ_SCENE_MAX_CONTROLLERS 128
#define DVZ_SCENE_MAX_CONTROLLER_LINKS 128
#define DVZ_SCENE_MAX_AXIS_LINES                                                                  \
    ((2 + DVZ_SCENE_MAX_AXIS_MINOR_TICKS) * DVZ_SCENE_MAX_AXIS_TICKS + 1)
#define DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS 256
#define DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT 32
#define DVZ_SCENE_TEXT_BLOCK_SOURCE_SIZE 1024
#define DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE   1024
#define DVZ_SCENE_TEXT_BLOCK_MAX_RUNS    64
#define DVZ_COMPOSITE_MAX_VISUALS 8
#define DVZ_COMPOSITE_ROLE_SIZE   32



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_VISUAL_TYPE_NONE      = 0,
    DVZ_VISUAL_TYPE_POINT     = 1,
    DVZ_VISUAL_TYPE_PIXEL     = 2,
    DVZ_VISUAL_TYPE_MARKER    = 3,
    DVZ_VISUAL_TYPE_SEGMENT   = 4,
    DVZ_VISUAL_TYPE_PATH      = 5,
    DVZ_VISUAL_TYPE_IMAGE     = 6,
    DVZ_VISUAL_TYPE_MESH      = 7,
    DVZ_VISUAL_TYPE_VOLUME    = 8,
    DVZ_VISUAL_TYPE_PRIMITIVE = 9,
    DVZ_VISUAL_TYPE_SPHERE    = 10,
    DVZ_VISUAL_TYPE_GLYPH     = 11,
    DVZ_VISUAL_TYPE_TEXT      = 12,
    DVZ_VISUAL_TYPE_LABELS    = 13,
} DvzVisualType;



typedef enum
{
    DVZ_COMPOSITE_TYPE_NONE,
    DVZ_COMPOSITE_TYPE_POLYGON,
    DVZ_COMPOSITE_TYPE_POLYGON_SET,
} DvzCompositeType;



typedef struct DvzPolygonStoredRing DvzPolygonStoredRing;
typedef struct DvzPolygonSetItem DvzPolygonSetItem;
typedef struct DvzCompositeVisual DvzCompositeVisual;



struct DvzPolygonStoredRing
{
    dvec2* xy;
    uint32_t count;
};



struct DvzPolygon
{
    DvzScene* scene;
    uint32_t flags;
    bool active;
    DvzPolygonStoredRing outer;
    DvzPolygonStoredRing* holes;
    uint32_t hole_count;
    DvzColor fill_color;
    DvzColor stroke_color;
    float stroke_width;
    uint64_t version;
};



struct DvzPolygonSetItem
{
    bool active;
    DvzPolygonStoredRing outer;
    DvzPolygonStoredRing* holes;
    uint32_t hole_count;
    DvzColor fill_color;
    DvzColor stroke_color;
    float stroke_width;
    uint64_t version;
};



struct DvzPolygonSet
{
    DvzScene* scene;
    uint32_t flags;
    bool active;
    DvzPolygonSetItem* polygons;
    uint32_t polygon_count;
    uint32_t polygon_capacity;
    uint64_t version;
};



struct DvzCompositeVisual
{
    char role[DVZ_COMPOSITE_ROLE_SIZE];
    DvzVisual* visual;
    int32_t z_offset;
};



struct DvzComposite
{
    DvzScene* scene;
    DvzCompositeType type;
    uint32_t flags;
    bool active;
    bool dirty;
    bool fill_dirty;
    bool stroke_dirty;
    void* source;
    uint64_t source_version_seen;
    uint32_t visual_count;
    DvzCompositeVisual visuals[DVZ_COMPOSITE_MAX_VISUALS];
};



/* Image texture payload cache (field -> runtime texture realization). */
typedef struct DvzVisualTexture DvzVisualTexture;
struct DvzVisualTexture
{
    void* rgba;                 /* owned RGBA8 staging for scalar textures */
    uint64_t rgba_size;         /* bytes */
    void* label_lookup;         /* owned sparse label lookup staging buffer */
    uint64_t label_lookup_size; /* bytes */
    void* upload;               /* owned tightly-packed upload scratch for partial texture writes */
    uint64_t upload_size;       /* bytes */
    uint32_t width;             /* pixels */
    uint32_t height;            /* pixels */
    bool dirty;                 /* needs upload on next emit */
    bool field_dirty;
    bool field_dirty_full;
    DvzFieldRegion field_dirty_region;
    uint64_t version;           /* increments when texture payload semantics change */
};



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzScene   DvzScene;
typedef struct DvzFigure  DvzFigure;
typedef struct DvzGrid    DvzGrid;
typedef struct DvzPanel   DvzPanel;
typedef struct DvzVisual  DvzVisual;
typedef struct DvzSampledField DvzSampledField;
typedef struct DvzSceneBuffer DvzSceneBuffer;
typedef struct DvzScale   DvzScale;
typedef struct DvzColormap DvzColormap;
typedef struct DvzColorbar DvzColorbar;
typedef struct DvzLegend DvzLegend;
typedef struct DvzInteractionPolicy DvzInteractionPolicy;
typedef struct DvzSelection DvzSelection;
typedef struct DvzLinkChannel DvzLinkChannel;
typedef struct DvzPinnedReadout DvzPinnedReadout;
typedef struct DvzFont DvzFont;
typedef struct DvzText DvzText;
typedef struct DvzAnnotation DvzAnnotation;
typedef struct DvzAxis DvzAxis;
typedef struct DvzAnimation DvzAnimation;
typedef struct DvzTextShapedGlyph DvzTextShapedGlyph;
typedef struct DvzTextLayoutMetrics DvzTextLayoutMetrics;
typedef struct DvzTextGlyphInstance DvzTextGlyphInstance;
typedef struct DvzTextAtlasGlyph DvzTextAtlasGlyph;
typedef struct DvzTextAtlasSpec DvzTextAtlasSpec;
typedef struct DvzTextAtlas DvzTextAtlas;
typedef struct DvzTextBlockRun DvzTextBlockRun;
typedef struct DvzTextBlockLayout DvzTextBlockLayout;
typedef struct DvzTextBlockRasterDesc DvzTextBlockRasterDesc;
typedef struct DvzTextBlockImageDesc DvzTextBlockImageDesc;
typedef struct DvzTextBlock DvzTextBlock;

typedef void (*DvzSceneRequestFrameCallback)(DvzFigure* figure, void* user_data);

typedef struct DvzSceneRequestFrameSubscription DvzSceneRequestFrameSubscription;

struct DvzController
{
    DvzScene* scene;
    DvzControllerType type;
    bool active;
    DvzPanzoom* panzoom;
    DvzArcball* arcball;
    DvzFly* fly;
    DvzTurntable* turntable;
};


struct DvzControllerLink
{
    DvzScene* scene;
    DvzController* source;
    DvzController* target;
    uint32_t components;
    DvzControllerLinkMode mode;
    bool active;
};


void _dvz_scene_controller_links_propagate(DvzScene* scene);



/*************************************************************************************************/
/*  Request frame subscriptions                                                                  */
/*************************************************************************************************/

struct DvzSceneRequestFrameSubscription
{
    DvzSceneRequestFrameCallback callback;
    void* user_data;
    bool active;
};



/*************************************************************************************************/
/*  Text atlas                                                                                   */
/*************************************************************************************************/

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
/*  Shared helpers                                                                               */
/*************************************************************************************************/

void _scene_panel_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height);

void _scene_panel_inner_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height);

void _scene_panel_plot_visual_rect(const DvzPanel* panel, float out[4]);

void _scene_panel_plot_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height);

DvzPanelDesc _scene_panel_plot_desc(const DvzPanel* panel);

bool _scene_figure_resolve_layouts(DvzFigure* figure);

void _scene_panel_set_axis_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);

void _scene_panel_set_colorbar_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);

void _scene_panel_set_legend_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);

void _scene_panel_refresh_axis_reserve(DvzPanel* panel);

void _scene_panel_refresh_colorbar_reserve(DvzPanel* panel);

void _scene_panel_refresh_legend_reserve(DvzPanel* panel);



/*************************************************************************************************/
/*  Shared retained-object state                                                                 */
/*************************************************************************************************/

typedef struct DvzSceneFormatState DvzSceneFormatState;
typedef struct DvzSceneCard DvzSceneCard;


typedef enum
{
    DVZ_SCENE_CARD_CONTENT_TEXT = 0,
    DVZ_SCENE_CARD_CONTENT_IMAGE,
} DvzSceneCardContent;


struct DvzSceneFormatState
{
    int32_t precision;
    bool scientific;
    bool trim_trailing_zeros;
    bool show_unit;
    char unit[32];
    char prefix[DVZ_SCENE_LABEL_SIZE];
    char suffix[DVZ_SCENE_LABEL_SIZE];
};


struct DvzSceneCard
{
    DvzPanel* panel;
    DvzVisual* background_visual;
    DvzVisual* text_visual;
    DvzSceneCardContent content;
    char text[DVZ_SCENE_LABEL_SIZE];
    char realized_text[DVZ_SCENE_LABEL_SIZE];
    DvzOverlayCardPlacement placement;
    float anchor_px[2];
    float offset_px[2];
    float padding_px[2];
    float min_width_px;
    float height_px;
    float glyph_advance_px;
    float text_size_px;
    DvzTextRenderer text_renderer;
    uint32_t max_text_chars;
    DvzColor background_color;
    DvzColor text_color;
    float content_size_px[2];
    float realized_rect_px[4];
    uint32_t figure_width;
    uint32_t figure_height;
    bool dirty;
    bool visible;
};


typedef enum
{
    DVZ_TEXT_DIRTY_NONE = 0x00u,
    DVZ_TEXT_DIRTY_STRING = 0x01u,
    DVZ_TEXT_DIRTY_STYLE = 0x02u,
    DVZ_TEXT_DIRTY_PLACEMENT = 0x04u,
    DVZ_TEXT_DIRTY_LAYOUT = 0x08u,
    DVZ_TEXT_DIRTY_RENDER = 0x10u,
    DVZ_TEXT_DIRTY_ALL = 0x1fu,
} DvzTextDirtyFlag;


struct DvzTextShapedGlyph
{
    uint32_t glyph_id;
    uint32_t cluster;
    uint32_t font_index;
    float advance[2];
    float offset[2];
};


struct DvzTextLayoutMetrics
{
    float advance[2];
    float ink_bounds[4];
    float layout_bounds[4];
    float baseline;
    float ascender;
    float descender;
    float line_height;
};


struct DvzTextGlyphInstance
{
    double anchor[3];
    float offset[2];
    float size[2];
    float uv[4];
    float angle;
    uint8_t color[4];
    uint32_t text_index;
    uint32_t glyph_index;
};


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


struct DvzTextAtlasSpec
{
    DvzTextAtlasBackend backend;
    float em_px;
    float distance_range_px;
    uint32_t flags;
};



struct DvzTextAtlas
{
    DvzTextAtlasSpec spec;
    DvzTextAtlasBackend backend;
    DvzTextAtlasEncoding encoding;
    DvzSampledField* field;
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
    DvzTextAtlasGlyph glyphs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS];
};


typedef enum
{
    DVZ_TEXT_BLOCK_STYLE_NONE = 0x00u,
    DVZ_TEXT_BLOCK_STYLE_BOLD = 0x01u,
    DVZ_TEXT_BLOCK_STYLE_ITALIC = 0x02u,
    DVZ_TEXT_BLOCK_STYLE_UNDERLINE = 0x04u,
} DvzTextBlockStyleFlag;


typedef enum
{
    DVZ_TEXT_BLOCK_FACE_REGULAR = 0,
    DVZ_TEXT_BLOCK_FACE_BOLD,
    DVZ_TEXT_BLOCK_FACE_ITALIC,
    DVZ_TEXT_BLOCK_FACE_BOLD_ITALIC,
    DVZ_TEXT_BLOCK_FACE_COUNT,
} DvzTextBlockFaceSlot;


struct DvzTextBlockRun
{
    uint32_t source_start;
    uint32_t source_end;
    uint32_t text_start;
    uint32_t text_end;
    uint32_t style_flags;
    bool has_color;
    DvzColor color;
};


struct DvzTextBlockLayout
{
    DvzScene* scene;
    DvzFont* font;
    DvzFont* bold_font;
    DvzFont* italic_font;
    DvzFont* bold_italic_font;
    float font_size_px;
    float max_width_px;
    float char_width_px;
    float line_height_px;
    float padding_px[2];
};


struct DvzTextBlockRasterDesc
{
    DvzScene* scene;
    DvzFont* font;
    DvzColor text_color;
    DvzColor background_color;
    float font_size_px;
    float scale;
};


struct DvzTextBlockImageDesc
{
    vec3 position;
    vec2 extent;
    vec3 position_px;
    vec2 extent_px;
    vec2 anchor;
    bool pixel_space;
    int32_t z_layer;
    DvzControllerMode controller_mode;
};


struct DvzTextBlock
{
    char source[DVZ_SCENE_TEXT_BLOCK_SOURCE_SIZE];
    char text[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    char diagnostic[DVZ_SCENE_LABEL_SIZE];
    uint32_t source_size;
    uint32_t text_size;
    uint32_t run_count;
    DvzTextBlockRun runs[DVZ_SCENE_TEXT_BLOCK_MAX_RUNS];
    DvzTextBlockLayout layout;
    DvzTextLayoutMetrics metrics;
    uint32_t layout_x[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_y[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_text_start[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_text_end[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_codepoint[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_glyph_index[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_style_flags[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    uint32_t layout_face_slot[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    float layout_pos_x[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    float layout_baseline_y[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    float layout_advance[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    DvzColor layout_color[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    bool layout_visible[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE];
    DvzFont* layout_fonts[DVZ_TEXT_BLOCK_FACE_COUNT];
    uint32_t layout_glyph_count;
    uint32_t layout_line_count;
    uint32_t missing_style_flags;
    uint8_t* rgba;
    uint64_t rgba_size;
    uint32_t raster_width;
    uint32_t raster_height;
    float raster_scale;
    uint64_t raster_version;
    DvzVisual* image_visual;
    DvzSampledField* image_field;
    uint32_t image_width;
    uint32_t image_height;
    uint64_t image_raster_version;
    DvzTextBlockImageDesc image_desc;
    bool image_desc_valid;
    bool image_attached;
    bool valid;
};



/*************************************************************************************************/
/*  Animation                                                                                    */
/*************************************************************************************************/

typedef enum
{
    DVZ_ANIMATION_NONE = 0,
    DVZ_ANIMATION_TIMER,
    DVZ_ANIMATION_PHASE,
    DVZ_ANIMATION_ARCBALL_SPIN,
} DvzAnimationType;



typedef struct DvzSceneClock DvzSceneClock;

struct DvzSceneClock
{
    DvzSceneClockMode mode;
    double t;
    double dt;
    double fps;
    uint64_t last_wall_time_ns;
    bool initialized;
};



struct DvzAnimation
{
    DvzScene* scene;
    DvzAnimationType type;
    bool active;
    double t_start;
    double period_s;
    double last_fire_t;
    DvzAnimTimerCallback timer_callback;
    DvzAnimPhaseCallback phase_callback;
    void* user_data;
    float phase_value;
    float phase_speed;
    float phase_wrap_min;
    float phase_wrap_max;
    DvzArcball* arcball;
    vec3 axis;
    float speed_rad_per_sec;
    uint32_t flags;
};



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

DvzCamera* _dvz_camera(const DvzCameraDesc* desc);

void _scene_panel_apply_mvp(const DvzPanel* panel, DvzMVP* out);
bool _scene_panel_panzoom_extent(const DvzPanel* panel, float out[4]);

bool _dvz_figure_fly_update(DvzFigure* figure, double dt);



/*************************************************************************************************/
/*  Scale / colormap / colorbar                                                                  */
/*************************************************************************************************/

typedef struct DvzScaleCategoryState
{
    DvzCategoryId category_id;
    uint32_t order;
    bool has_label;
    char label[DVZ_SCENE_LABEL_SIZE];
    DvzColor color;
    uint32_t flags;
} DvzScaleCategoryState;


struct DvzScale
{
    DvzScene* scene;
    DvzScaleKind kind;
    char label[DVZ_SCENE_LABEL_SIZE];
    char unit[32];
    DvzSceneFormatState format;
    double domain_min;
    double domain_max;
    double view_min;
    double view_max;
    bool has_domain;
    bool has_view_range;
    DvzColormap* colormap;
    uint32_t category_count;
    uint32_t category_capacity;
    DvzScaleCategoryState* categories;
};


struct DvzColormap
{
    DvzScene* scene;
    DvzColormapKind kind;
    DvzBuiltinColormap builtin;
    double center;
    bool has_center;
    char label[DVZ_SCENE_LABEL_SIZE];
    uint32_t stop_count;
    DvzColormapStop stops[DVZ_SCENE_MAX_COLOR_STOPS];
};


struct DvzColorbar
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzScale* scale;
    DvzColorbarPlacementMode placement_mode;
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    char title[DVZ_SCENE_LABEL_SIZE];
    uint32_t flags;
    bool has_format;
    DvzSceneFormatState format;
    DvzPanelReserve auto_reserve;
    float reserve_px;
    float ramp_width_px;
    float edge_offset_px;
    float plot_gap_px;
    float tick_length_px;
    float label_gap_px;
    DvzTextRenderer text_renderer;
    DvzPlacement placement;
    bool dirty;
    uint64_t version;
    float realized_panel_width;
    float realized_panel_height;
    DvzVisual* ramp_visual;
    DvzVisual* tick_visual;
    DvzVisual* text_visual;
    uint32_t tick_count;
    double ticks[DVZ_SCENE_MAX_COLORBAR_TICKS];
    uint32_t text_count;
    char text_labels[DVZ_SCENE_MAX_COLORBAR_TEXTS][DVZ_SCENE_LABEL_SIZE];
    float text_positions[DVZ_SCENE_MAX_COLORBAR_TEXTS][3];
    float text_anchors[DVZ_SCENE_MAX_COLORBAR_TEXTS][2];
    float text_sizes[DVZ_SCENE_MAX_COLORBAR_TEXTS];
    uint8_t text_colors[DVZ_SCENE_MAX_COLORBAR_TEXTS][4];
    float text_angles[DVZ_SCENE_MAX_COLORBAR_TEXTS];
};


struct DvzLegend
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzScale* scale;
    DvzLegendPlacementMode placement_mode;
    DvzSceneAnchor anchor;
    char title[DVZ_SCENE_LABEL_SIZE];
    uint32_t flags;
    DvzPanelReserve auto_reserve;
    float reserve_px;
    float edge_offset_px;
    float plot_gap_px;
    float entry_gap_px;
    float mark_size_px;
    float mark_label_gap_px;
    DvzTextRenderer text_renderer;
    DvzPlacement placement;
    bool dirty;
    uint64_t version;
    float realized_panel_width;
    float realized_panel_height;
    DvzVisual* mark_visual;
    DvzVisual* text_visual;
    DvzCategoryId highlighted_ids[DVZ_SCENE_MAX_SCALE_CATEGORIES];
    uint32_t highlight_count;
    uint32_t entry_count;
    uint32_t text_count;
    char text_labels[DVZ_SCENE_MAX_LEGEND_TEXTS][DVZ_SCENE_LABEL_SIZE];
    float text_positions[DVZ_SCENE_MAX_LEGEND_TEXTS][3];
    float text_anchors[DVZ_SCENE_MAX_LEGEND_TEXTS][2];
    float text_sizes[DVZ_SCENE_MAX_LEGEND_TEXTS];
    uint8_t text_colors[DVZ_SCENE_MAX_LEGEND_TEXTS][4];
    float text_angles[DVZ_SCENE_MAX_LEGEND_TEXTS];
};



/*************************************************************************************************/
/*  Interaction / selection / readout                                                           */
/*************************************************************************************************/

struct DvzLinkChannel
{
    DvzScene* scene;
    char name[DVZ_SCENE_LABEL_SIZE];
};


struct DvzSelection
{
    DvzScene* scene;
    DvzSelectionDesc desc;
    DvzPanel* card_panel;
    bool card_enabled;
    DvzQueryResult card_query;
    DvzSceneCard card;
    uint32_t item_count;
    DvzSelectionItem items[DVZ_SCENE_MAX_SELECTION_ITEMS];
};


struct DvzInteractionPolicy
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzLinkChannel* link_channel;
    DvzPickHitPolicy pick_hit_policy;
    bool auto_pin_readout;
};


struct DvzPinnedReadout
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzQueryResult query;
    bool has_format;
    DvzSceneFormatState format;
    char text[DVZ_SCENE_LABEL_SIZE];
    DvzSceneCard card;
};


struct DvzOverlay
{
    DvzScene* scene;
    DvzPanel* panel;
    uint32_t flags;
    bool active;
};


struct DvzOverlayCard
{
    DvzScene* scene;
    DvzOverlay* overlay;
    DvzPanel* panel;
    DvzSceneCard card;
    DvzTextBlock rich_block;
    DvzTextBlockLayout rich_layout;
    DvzTextBlockRasterDesc rich_raster;
    uint32_t flags;
    bool rich_enabled;
    bool rich_dirty;
    bool active;
};


struct DvzFont
{
    DvzScene* scene;
    char path[512];
    char family[DVZ_SCENE_LABEL_SIZE];
    char style[DVZ_SCENE_LABEL_SIZE];
    uint32_t face_index;
    uint32_t flags;
    uint64_t version;
    void* ttf_bytes;
    uint64_t ttf_size;
    DvzTextAtlas* atlases[DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT];
    uint32_t atlas_count;
};


struct DvzText
{
    DvzScene* scene;
    DvzPanel* panel;
    char string[DVZ_SCENE_LABEL_SIZE];
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
    uint32_t dirty_flags;
    uint64_t version;
    DvzTextLayoutMetrics metrics;
    DvzVisual* visual;
    uint64_t visual_version;
    uint64_t visual_atlas_generation;
    uint32_t visual_figure_width;
    uint32_t visual_figure_height;
};


typedef struct DvzScaleBarRealization DvzScaleBarRealization;

struct DvzScaleBarRealization
{
    bool valid;
    bool horizontal;
    double units_per_px;
    double length_units;
    float length_px;
    float screen_scale;
    vec3 starts[3];
    vec3 ends[3];
    DvzColor line_colors[3];
    float line_width[3];
    char label[DVZ_SCENE_LABEL_SIZE];
    float label_position[3];
    float label_anchor[2];
    float label_size;
    DvzColor label_color;
    float label_angle;
    DvzTextRenderer renderer;
};


struct DvzAnnotation
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzAnnotationKind kind;
    char text[DVZ_SCENE_LABEL_SIZE];
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
    bool has_format;
    DvzSceneFormatState format;
    uint32_t dirty_flags;
    uint64_t version;
    DvzTextLayoutMetrics metrics;
    DvzVisual* visual;
    uint64_t visual_version;
    uint32_t visual_figure_width;
    uint32_t visual_figure_height;
    DvzScaleBarDesc scalebar;
    DvzVisual* scalebar_visual;
    double scalebar_units;
    float scalebar_px;
    DvzScaleBarRealization scalebar_realization;
};


typedef struct DvzPendingQueryRequest DvzPendingQueryRequest;
typedef struct DvzQueuedQueryResult DvzQueuedQueryResult;
typedef struct DvzRequestFreshnessScope DvzRequestFreshnessScope;
typedef struct DvzSceneQueryScratch DvzSceneQueryScratch;
typedef struct DvzSceneRequestExecutor DvzSceneRequestExecutor;

struct DvzPendingQueryRequest
{
    DvzPanel* panel;
    double x;
    double y;
    uint64_t freshness_serial;
    DvzQueryRequest request;
};


struct DvzQueuedQueryResult
{
    DvzPanel* panel;
    uint64_t freshness_serial;
    DvzQueryResult result;
};


struct DvzRequestFreshnessScope
{
    DvzPanel* panel;
    uint64_t request_id;
    uint64_t freshness_serial;
    uint64_t touched_serial;
};


struct DvzSceneQueryScratch
{
    DvzFramePlan* plan;
    vec3* query_positions;
    vec2* query_texcoords;
    DvzColor* query_colors;
    uint32_t* query_ids;
    float* query_position_start;
    float* query_position_curr;
    float* query_position_end;
    float* query_line_width;
    uint32_t* query_path_flags;
    float* query_path_distance;
    uint32_t* query_indices;
};


struct DvzSceneRequestExecutor
{
    DvzDrp2Runtime* runtime;
    DvzFramePlanEmitter* emitter;
    DvzDrp2RuntimeConfig runtime_cfg;
    DvzVisual* image_query_visual;
    uint64_t image_query_position_version;
    uint64_t image_query_texcoord_version;
    uint64_t image_query_texture_version;
    DvzSceneVisualFamily active_query_family;
    DvzSceneTargetKind active_query_target;
    uint32_t runtime_create_count;
    uint32_t emitter_create_count;
    uint32_t image_query_static_upload_count;
};



/*************************************************************************************************/
/*  Sampled fields                                                                               */
/*************************************************************************************************/

struct DvzSampledField
{
    DvzScene* scene;
    DvzSampledFieldDesc desc;
    DvzFieldGeometry geometry;
    void* data;
    uint64_t data_size;
    void* upload;
    uint64_t upload_size;
    bool dirty;
    bool dirty_full;
    DvzFieldRegion dirty_region;
};


struct DvzSceneBuffer
{
    DvzScene* scene;
    DvzSceneBufferDesc desc;
    void* data;
    bool dirty;
};


typedef struct DvzSceneMaterialParams DvzSceneMaterialParams;

struct DvzSceneMaterialParams
{
    float light_direction[4];
    float params[4];
    float model[4];
    float base_color_factor[4];
    float standard_params[4];
    float emissive_rim[4];
    float depth_cue[4];
    float depth_cue_color[4];
    float depth_cue_extra[4];
};


typedef enum
{
    DVZ_MATERIAL_KIND_UNLIT = 0,
    DVZ_MATERIAL_KIND_LIT,
    DVZ_MATERIAL_KIND_SCIENTIFIC,
    DVZ_MATERIAL_KIND_VOLUME,
} DvzMaterialKind;


typedef struct DvzSceneMaterialState DvzSceneMaterialState;

struct DvzSceneMaterialState
{
    DvzMaterialKind kind;
    DvzMaterialModel model;
    DvzAlphaMode alpha_mode;
    float opacity;
    float base_color_factor[4];
    float light_direction[4];
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float roughness;
    float standard_specular;
    float metallic;
    float emissive[3];
    float rim_strength;
    bool depth_cue_enabled;
    DvzDepthCueMode depth_cue_mode;
    DvzDepthCueMetric depth_cue_metric;
    DvzDepthCueFalloff depth_cue_falloff;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    float depth_cue_density;
    float depth_cue_background[4];
    bool scalar_modulation_enabled;
    char scalar_slot[32];
    float scalar_scale;
    float scalar_bias;
    DvzPointStyleDesc point_style;
    bool point_style_enabled;
    uint64_t version;
};


typedef struct DvzSegmentGpuCache DvzSegmentGpuCache;

struct DvzSegmentGpuCache
{
    float* position_start;
    float* position_end;
    DvzColor* color;
    float* line_width;
    uint32_t* indices;
    uint64_t item_count;
    uint64_t vertex_count;
    uint64_t index_count;
    bool dirty;
};


typedef struct DvzSegmentState DvzSegmentState;

struct DvzSegmentState
{
    DvzSegmentCap start_cap;
    DvzSegmentCap end_cap;
    DvzSegmentGpuCache gpu;
};


typedef struct DvzPathGpuCache DvzPathGpuCache;

struct DvzPathGpuCache
{
    float* position_prev;
    float* position_curr;
    float* position_next;
    DvzColor* color;
    float* line_width;
    uint32_t* path_flags;
    float* path_distance;
    uint32_t* indices;
    uint64_t point_count;
    uint64_t segment_count;
    uint64_t vertex_count;
    uint64_t index_count;
    bool dirty;
};


typedef struct DvzPathState DvzPathState;

struct DvzPathState
{
    uint32_t* subpath_lengths;
    uint32_t subpath_count;
    DvzSegmentCap cap_start;
    DvzSegmentCap cap_end;
    DvzPathJoin join;
    float miter_limit;
    DvzPathGpuCache gpu;
};


typedef struct DvzTextGlyphSpan DvzTextGlyphSpan;

struct DvzTextGlyphSpan
{
    uint32_t first_glyph;
    uint32_t glyph_count;
};


typedef struct DvzTextVisualState DvzTextVisualState;

struct DvzTextVisualState
{
    char** strings;
    uint32_t string_count;
    uint64_t strings_version;
    DvzTextRenderer renderer;
    uint64_t renderer_version;
    DvzTextGlyphSpan* spans;
    uint32_t span_count;
    DvzVisual* glyph_visual;
    uint64_t realized_version;
    uint64_t realized_layout_version;
    uint64_t atlas_generation;
    DvzControllerMode realized_controller_mode;
    float screen_scale;
    uint32_t visual_figure_width;
    uint32_t visual_figure_height;
    uint32_t reserved_glyph_vertices;
};


typedef struct DvzImageGpuCache DvzImageGpuCache;

struct DvzImageGpuCache
{
    float* position;
    float* texcoords;
    uint64_t item_count;
    uint64_t vertex_count;
    bool pixel_space;
    bool dirty;
};



/*************************************************************************************************/
/*  Visual attribute slot                                                                        */
/*************************************************************************************************/

typedef struct DvzVisualAttr DvzVisualAttr;
typedef struct DvzVisualBinding DvzVisualBinding;

typedef enum
{
    DVZ_VISUAL_BINDING_NONE,
    DVZ_VISUAL_BINDING_FIELD,
    DVZ_VISUAL_BINDING_BUFFER,
    DVZ_VISUAL_BINDING_SCALE,
} DvzVisualBindingKind;

struct DvzVisualAttr
{
    char     name[64];
    void*    data;
    DvzSceneBuffer* buffer;
    uint64_t buffer_byte_offset;
    uint64_t item_count;
    uint32_t item_size;         /* bytes per item */
    DvzVisualAttrSource source;
    DvzVisualAttrMutability mutability;
    uint64_t dirty_first_item;  /* first dirty item index */
    uint64_t dirty_item_count;  /* number of dirty items (0 = not dirty) */
    uint64_t version;           /* increments when dense or bound payload changes */
};


struct DvzVisualBinding
{
    DvzVisualBindingKind kind;
    void* resource;
    char slot[32];
    bool owned;
};



/*************************************************************************************************/
/*  DvzVisual                                                                                   */
/*************************************************************************************************/

struct DvzVisual
{
    DvzScene*    scene;
    DvzVisualType type;
    uint32_t     flags;
    bool         visible;
    int32_t      z_layer;
    DvzAlphaMode alpha_mode;
    bool         depth_test_enabled;
    uint32_t     depth_compare_op;
    DvzSceneMaterialState material;

    DvzPrimitiveTopology topology; /* used by DVZ_VISUAL_TYPE_PRIMITIVE */
    DvzVisualBinding bindings[DVZ_SCENE_MAX_VISUAL_BINDINGS];
    DvzSampledField* field;        /* used by DVZ_VISUAL_TYPE_IMAGE */
    char             field_slot[32];
    bool             field_owned;  /* true for legacy wrapper-created fields */
    DvzSceneBuffer*  buffer;       /* current slice: primitive index buffer binding */
    char             buffer_slot[32];
    DvzVisualTexture texture;      /* used by DVZ_VISUAL_TYPE_IMAGE */
    DvzScale*     scale;           /* first slice: image colormap scale */
    char          scale_slot[32];  /* semantic binding slot name */
    uint32_t      query_capabilities;
    DvzLinkChannel* link_channel;
    uint64_t*       link_keys;
    uint32_t        link_key_count;
    DvzSceneMaterialParams material_params;
    bool                   material_params_dirty;
    DvzSegmentState        segment;
    DvzPathState           path;
    DvzTextVisualState     text;
    DvzTextAtlasEncoding   glyph_atlas_encoding;
    float                  glyph_distance_range_px;
    DvzImageGpuCache       image_gpu;
    DvzSphereMode          sphere_mode;
    bool                   mesh_default_color;
    bool                   scene_occluder;
    bool                   scene_occluded;
    bool                   volume_occluded;
    DvzLabelsState         labels;
    DvzVolumeState         volume;
    uint64_t               labels_realized_version;
    uint64_t               volume_realized_version;

    /* Attribute slots — indexed by attr index (type-specific) */
    uint32_t      attr_count;
    DvzVisualAttr attrs[DVZ_SCENE_MAX_ITEM_ATTRS];
};



/*************************************************************************************************/
/*  Scene techniques                                                                            */
/*************************************************************************************************/

typedef struct DvzSceneGBufferTechniqueState
{
    bool enabled;
    bool object_id_enabled;
} DvzSceneGBufferTechniqueState;



typedef struct DvzSceneEdlUniform
{
    float params[4];
} DvzSceneEdlUniform;



typedef struct DvzSceneEdlTechniqueState
{
    bool enabled;
    float radius;
    float strength;
    float depth_scale;
    DvzSceneEdlUniform uniform;
} DvzSceneEdlTechniqueState;


typedef struct DvzSceneSsaoUniform
{
    mat4 inv_proj;
    mat4 view;
    float viewport[4];
    float params[4];
    float params2[4];
    float params3[4];
} DvzSceneSsaoUniform;



typedef struct DvzSceneSsaoTechniqueState
{
    bool enabled;
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
    DvzSceneSsaoUniform uniform;
} DvzSceneSsaoTechniqueState;


typedef struct DvzSceneMsaaTechniqueState
{
    bool enabled;
    uint32_t sample_count;
    bool alpha_to_coverage;
} DvzSceneMsaaTechniqueState;



typedef struct DvzSceneTechniqueState
{
    DvzSceneGBufferTechniqueState gbuffer;
    DvzSceneEdlTechniqueState edl;
    DvzSceneSsaoTechniqueState ssao;
    DvzSceneMsaaTechniqueState msaa;
} DvzSceneTechniqueState;



/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/

struct DvzAxis
{
    DvzPanel* panel;
    DvzDim dim;
    bool enabled;
    bool dirty;
    bool domain_set;
    uint64_t version;
    DvzDataDomain domain;
    DvzAxisTickPolicy tick_policy;
    DvzAxisStyle style;
    char label[DVZ_SCENE_LABEL_SIZE];
    uint32_t tick_count;
    double tick_lmin;
    double tick_lmax;
    double tick_lstep;
    double tick_covered_min;
    double tick_covered_max;
    bool tick_cache_valid;
    double ticks[DVZ_SCENE_MAX_AXIS_TICKS];
    DvzVisual* visual;
    DvzVisual* text_visual;
    uint32_t text_count;
    char text_labels[DVZ_SCENE_MAX_AXIS_TICKS + 1][DVZ_SCENE_LABEL_SIZE];
    float text_positions[DVZ_SCENE_MAX_AXIS_TICKS + 1][3];
    float text_anchors[DVZ_SCENE_MAX_AXIS_TICKS + 1][2];
    float text_sizes[DVZ_SCENE_MAX_AXIS_TICKS + 1];
    uint8_t text_colors[DVZ_SCENE_MAX_AXIS_TICKS + 1][4];
    float text_angles[DVZ_SCENE_MAX_AXIS_TICKS + 1];
};



/*************************************************************************************************/
/*  DvzPanel                                                                                    */
/*************************************************************************************************/

/* Per-visual attachment state on a panel. Stored alongside the visual pointer in the
 * panel's visuals array so the converter can sort by z_layer and choose APPLY vs FIXED MVP. */
typedef struct DvzPanelAttach
{
    DvzVisual*        visual;          /* weak ref — owned by scene */
    int32_t           z_layer;         /* signed; default 0 */
    DvzControllerMode controller_mode; /* default APPLY */
    uint32_t          insertion_index; /* used as stable tie-breaker when z_layer ties */
} DvzPanelAttach;



struct DvzPanel
{
    DvzFigure*  figure;
    DvzPanelDesc desc; /* normalized position and size */
    DvzGrid* grid;     /* optional owner grid for retained layout */
    DvzGridCell grid_cell;
    DvzSceneTechniqueState techniques;

    uint32_t       visual_count;
    DvzPanelAttach visuals[DVZ_SCENE_MAX_VISUALS];
    DvzPanelReserve base_reserve;
    DvzPanelReserve axis_reserve;
    DvzPanelReserve colorbar_reserve;
    DvzPanelReserve legend_reserve;
    DvzPanelReserve reserve;
    DvzPanelReserve padding;

    DvzPanzoom* panzoom; /* optional pan/zoom controller (owned) */
    DvzArcball* arcball; /* optional arcball controller (owned) */
    DvzCamera* camera;   /* optional camera (owned) */
    DvzFly* fly;         /* optional fly camera controller (borrowed from scene-owned handle) */
    DvzController* controllers[3]; /* optional scene-owned spatial controller bindings */
    DvzInputRouter* input_router; /* optional router subscribed through panel-local dispatch */
    DvzVisual* fly_pivot_marker_visual; /* optional navigation overlay marker visual */
    DvzVisual* bounds_visual; /* optional panel-owned visible bounds overlay */
    DvzVisual* bounds_occluded_visual; /* optional panel-owned occluded bounds overlay */
    bool bounds_visible;
    DvzTurntable* turntable; /* optional turntable camera controller (owned) */
    DvzAxis axes[2];
    DvzInteractionPolicy* interaction;
    DvzHoverState hover;
    DvzVisual* volume_occluder_visual;
    DvzVolumeOcclusionDesc volume_occlusion;
    bool volume_occlusion_enabled;
    DvzSceneOcclusionDesc scene_occlusion;
    bool scene_occlusion_enabled;

    /* Optional background visual created by dvz_panel_set_background_*. The visual itself
     * lives in scene->visuals[] (weak ref); this pointer lets repeat calls update the
     * existing visual instead of stacking new ones. */
    DvzVisual* background_visual;
    DvzPanelBackgroundType background_type;

    uint32_t colorbar_count;
    DvzColorbar* colorbars[DVZ_SCENE_MAX_PANEL_COLORBARS];
    uint32_t legend_count;
    DvzLegend* legends[DVZ_SCENE_MAX_PANEL_LEGENDS];
    uint32_t pinned_readout_count;
    DvzPinnedReadout* pinned_readouts[DVZ_SCENE_MAX_PINNED_READOUTS];
};



/*************************************************************************************************/
/*  DvzGrid                                                                                     */
/*************************************************************************************************/

typedef struct DvzGridTrack
{
    DvzGridSizeMode mode;
    float           value;
} DvzGridTrack;


typedef struct DvzGridPanel
{
    DvzPanel*   panel;
    DvzGridCell cell;
} DvzGridPanel;



struct DvzGrid
{
    DvzFigure* figure;
    uint32_t rows;
    uint32_t cols;
    DvzPanelReserve margins;
    float gutter_x_px;
    float gutter_y_px;
    DvzGridTrack row_sizes[DVZ_SCENE_MAX_GRID_ROWS];
    DvzGridTrack col_sizes[DVZ_SCENE_MAX_GRID_COLS];
    uint32_t panel_count;
    DvzGridPanel panels[DVZ_SCENE_MAX_PANELS];
    bool dirty;
};



/*************************************************************************************************/
/*  DvzFigure                                                                                   */
/*************************************************************************************************/

struct DvzFigure
{
    DvzScene*  scene;
    uint32_t   width;
    uint32_t   height;
    uint32_t   flags;
    float      device_scale_x;
    float      device_scale_y;
    float      render_scale;
    float      user_scale;

    uint32_t   panel_count;
    DvzPanel   panels[DVZ_SCENE_MAX_PANELS];
    uint32_t   grid_count;
    DvzGrid    grids[DVZ_SCENE_MAX_GRIDS];
};



/*************************************************************************************************/
/*  DvzScene                                                                                    */
/*************************************************************************************************/

struct DvzScene
{
    DvzCapabilitySnapshot caps;
    DvzSceneTechniqueState techniques;
    DvzFontDefaults font_defaults;

    DvzSceneClock clock;

    uint32_t animation_count;
    DvzAnimation animations[DVZ_SCENE_MAX_ANIMATIONS];

    uint32_t controller_count;
    DvzController controllers[DVZ_SCENE_MAX_CONTROLLERS];

    uint32_t controller_link_count;
    DvzControllerLink controller_links[DVZ_SCENE_MAX_CONTROLLER_LINKS];

    DvzFramePlanEmitter* emitter; /* shared across all figures — owns GPU resource key→ID map */

    uint32_t outstanding_emitted_streams;

    uint32_t  figure_count;
    DvzFigure figures[DVZ_SCENE_MAX_FIGURES];

    uint32_t  visual_count;
    DvzVisual visuals[DVZ_SCENE_MAX_VISUALS]; /* owner of all visual objects */

    uint32_t polygon_count;
    DvzPolygon polygons[DVZ_SCENE_MAX_POLYGONS];

    uint32_t polygon_set_count;
    DvzPolygonSet polygon_sets[DVZ_SCENE_MAX_POLYGON_SETS];

    uint32_t composite_count;
    DvzComposite composites[DVZ_SCENE_MAX_COMPOSITES];

    uint32_t field_count;
    DvzSampledField fields[DVZ_SCENE_MAX_FIELDS];

    uint32_t buffer_count;
    DvzSceneBuffer buffers[DVZ_SCENE_MAX_BUFFERS];

    uint32_t scale_count;
    DvzScale scales[DVZ_SCENE_MAX_SCALES];

    uint32_t colormap_count;
    DvzColormap colormaps[DVZ_SCENE_MAX_COLORMAPS];

    uint32_t colorbar_count;
    DvzColorbar colorbars[DVZ_SCENE_MAX_COLORBARS];

    uint32_t legend_count;
    DvzLegend legends[DVZ_SCENE_MAX_LEGENDS];

    uint32_t interaction_count;
    DvzInteractionPolicy interactions[DVZ_SCENE_MAX_INTERACTIONS];

    uint32_t selection_count;
    DvzSelection selections[DVZ_SCENE_MAX_SELECTIONS];

    uint32_t link_channel_count;
    DvzLinkChannel link_channels[DVZ_SCENE_MAX_LINK_CHANNELS];

    uint32_t pinned_readout_count;
    DvzPinnedReadout pinned_readouts[DVZ_SCENE_MAX_PINNED_READOUTS];

    uint32_t overlay_count;
    DvzOverlay overlays[DVZ_SCENE_MAX_OVERLAYS];

    uint32_t overlay_card_count;
    DvzOverlayCard overlay_cards[DVZ_SCENE_MAX_OVERLAY_CARDS];

    uint32_t font_count;
    DvzFont fonts[DVZ_SCENE_MAX_FONTS];

    uint32_t text_count;
    DvzText texts[DVZ_SCENE_MAX_TEXTS];

    uint32_t annotation_count;
    DvzAnnotation annotations[DVZ_SCENE_MAX_ANNOTATIONS];

    uint32_t pending_query_count;
    DvzPendingQueryRequest pending_queries[DVZ_SCENE_MAX_PENDING_REQUESTS];
    DvzSceneRequestExecutor query_executor;

    uint32_t query_result_count;
    uint32_t query_result_head;
    DvzQueuedQueryResult query_results[DVZ_SCENE_MAX_QUERY_RESULTS];
    uint64_t next_request_serial;
    uint32_t query_scope_count;
    DvzRequestFreshnessScope query_scopes[DVZ_SCENE_MAX_REQUEST_SCOPES];
    DvzSampledField* text_bitmap_atlas;
    DvzSceneRequestFrameSubscription
        request_frame_subscriptions[DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS];

    struct
    {
        bool force_readback_download_failure;
    } test;
};



/*************************************************************************************************/
/*  Internal interaction helpers                                                                */
/*************************************************************************************************/

uint64_t _scene_next_request_serial(DvzScene* scene);
bool _scene_query_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc);
uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel);
void _scene_request_apply_mvp(const DvzPanel* panel, const vec2 request_ndc, DvzMVP* out);
bool _scene_image_query_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, bool include_static_uploads, DvzSceneQueryScratch* out_plan);
void _scene_query_scratch_destroy(DvzSceneQueryScratch* plan);
void _scene_request_executor_init(DvzSceneRequestExecutor* executor);
void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor);
bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime);
bool _scene_figure_has_pending_render_work(const DvzFigure* figure);
bool _scene_add_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data);
void _scene_remove_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data);
void _scene_notify_request_frame(DvzFigure* figure);
void _scene_notify_visual_changed(DvzVisual* visual);
void _scene_notify_buffer_changed(DvzSceneBuffer* buffer);
void _scene_prepare_composite_visuals(DvzFigure* figure);
void _scene_polygon_reset(DvzPolygon* polygon);
void _scene_polygon_set_reset(DvzPolygonSet* set);
void _scene_composite_reset(DvzComposite* composite);



/*************************************************************************************************/
/*  Internal shared scene helpers                                                               */
/*************************************************************************************************/

int _attr_index(const DvzVisual* visual, const char* name);

const char* _visual_type_name(DvzVisualType type);

bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index);

float _scene_screen_scale(const DvzFigure* figure);

bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action);

void _scene_format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src);

const DvzVisualBinding* _visual_binding_const(
    const DvzVisual* visual, DvzVisualBindingKind kind);

void _visual_binding_assign(
    DvzVisual* visual, DvzVisualBindingKind kind, const char* slot_name, void* resource, bool owned);

void _visual_binding_clear(DvzVisual* visual, DvzVisualBindingKind kind);

uint32_t _scene_buffer_index(const DvzScene* scene, const DvzSceneBuffer* buffer);

uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field);

uint32_t _scene_scale_index(const DvzScene* scene, const DvzScale* scale);

bool _field_format_is_scalar(DvzFieldFormat format);

bool _field_format_bytes_per_texel(DvzFieldFormat format, uint32_t* out_bytes);

bool _field_format_texture_format(DvzFieldFormat format, uint32_t* out_format);

bool _field_region_byte_size(
    DvzFieldFormat format, const DvzFieldRegion* region, uint64_t* out_size);

bool _scene_prepare_field_texture(
    DvzSampledField* field, DvzFieldRegion* out_region, const void** out_data);

bool _scene_prepare_volume_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data,
    uint32_t* out_format, uint32_t* out_bytes_per_texel);

bool _scene_prepare_image_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data);

bool _scene_emit_sampled_field_texture_upload(
    DvzFramePlan* plan, const char* resource_id, DvzSampledField* field);

void _scene_text_block_init(DvzTextBlock* block, const char* source);

void _scene_text_block_set_source(DvzTextBlock* block, const char* source);

void _scene_text_block_destroy(DvzTextBlock* block);

int _scene_text_block_parse(DvzTextBlock* block);

int _scene_text_block_measure(DvzTextBlock* block, const DvzTextBlockLayout* layout);

int _scene_text_block_rasterize(DvzTextBlock* block, const DvzTextBlockRasterDesc* desc);

int _scene_text_block_realize_image(
    DvzTextBlock* block, DvzPanel* panel, const DvzTextBlockImageDesc* desc);

bool _scene_visual_frame_plan_metadata(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanVisualMeta* metadata);

bool _scene_color_from_colormap(const DvzColormap* colormap, double t, uint8_t out_rgba[4]);

void _scene_visual_texture_mark_clean(DvzVisual* visual);

void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field);

bool _scene_visual_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size);

bool _scene_visual_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    const char* attr_name, char* out, size_t out_size);

bool _scene_visual_texture_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, char* out,
    size_t out_size);

bool _scene_visual_indexed_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    uint32_t buffer_index, char* out, size_t out_size);

void _scene_prepare_axis_visuals(DvzFigure* figure);

void _scene_prepare_colorbar_visuals(DvzFigure* figure, DvzDiagnosticReport* report);

void _scene_prepare_legend_visuals(DvzFigure* figure, DvzDiagnosticReport* report);

void _scene_prepare_bounds_visuals(DvzFigure* figure);

void _scene_prepare_text_visuals(DvzFigure* figure);

void _scene_prepare_pinned_readout_cards(DvzFigure* figure);

void _scene_prepare_selection_cards(DvzFigure* figure);

void _scene_prepare_overlay_cards(DvzFigure* figure);

DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags);

int _scene_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);

DvzTextRenderer _scene_adornment_text_renderer(DvzTextRenderer renderer);

DvzVisual* _scene_adornment_text_visual(DvzScene* scene, DvzTextRenderer renderer);

int _scene_adornment_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);

EXTERN_C_ON

DvzTextAtlasSpec _scene_text_atlas_spec(DvzTextAtlasBackend backend, float size_px);

DvzTextAtlas* _scene_text_atlas_get(DvzFont* font, const DvzTextAtlasSpec* spec);

bool _scene_text_atlas_ensure(DvzFont* font, const DvzTextAtlasSpec* spec);

bool _scene_text_atlas_ensure_string(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* string);

bool _scene_text_atlas_ensure_strings(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* const* strings, uint32_t count);

DvzTextAtlasGlyph* _scene_text_atlas_glyph(DvzTextAtlas* atlas, uint32_t codepoint);

void _scene_text_atlas_destroy(DvzTextAtlas* atlas);

bool _scene_font_ensure_bytes(DvzFont* font);

void _scene_font_release(DvzFont* font);

EXTERN_C_OFF

void _scene_release_visual_field(DvzVisual* visual);

void _scene_release_visual_buffer(DvzVisual* visual);

void _scene_field_reset(DvzSampledField* field);

void _scene_buffer_reset(DvzSceneBuffer* buffer);

void _scene_visual_reset(DvzVisual* visual, bool release_owned_resources);

uint64_t _scene_visual_public_id(const DvzScene* scene, const DvzVisual* visual);

void _scene_panel_visual_order(const DvzPanel* panel, uint32_t* order);

void _dvz_scene_animations_step(DvzScene* scene, uint64_t wall_time_ns);
