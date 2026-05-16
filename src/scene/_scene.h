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
#include <stdint.h>

#include "_frame_plan.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/types.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/animation.h"
#include "datoviz/scene/arcball.h"
#include "datoviz/scene/camera.h"
#include "datoviz/scene/enums.h"
#include "datoviz/scene/fly.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/scene/panzoom.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_MAX_FIGURES    16
#define DVZ_SCENE_MAX_PANELS     64
#define DVZ_SCENE_MAX_VISUALS    256
#define DVZ_SCENE_MAX_FIELDS     128
#define DVZ_SCENE_MAX_BUFFERS    128
#define DVZ_SCENE_MAX_SCALES     64
#define DVZ_SCENE_MAX_COLORMAPS  64
#define DVZ_SCENE_MAX_COLORBARS  64
#define DVZ_SCENE_MAX_INTERACTIONS 64
#define DVZ_SCENE_MAX_SELECTIONS 64
#define DVZ_SCENE_MAX_LINK_CHANNELS 64
#define DVZ_SCENE_MAX_PINNED_READOUTS 128
#define DVZ_SCENE_MAX_FONTS 64
#define DVZ_SCENE_MAX_TEXTS 128
#define DVZ_SCENE_MAX_ANNOTATIONS 128
#define DVZ_SCENE_MAX_PANEL_COLORBARS 16
#define DVZ_SCENE_MAX_COLOR_STOPS 32
#define DVZ_SCENE_MAX_ITEM_ATTRS 8
#define DVZ_SCENE_MAX_VISUAL_BINDINGS 3
#define DVZ_SCENE_MAX_SELECTION_ITEMS 1024
#define DVZ_SCENE_MAX_PICK_RESULTS 128
#define DVZ_SCENE_MAX_PROBE_RESULTS 128
#define DVZ_SCENE_MAX_PENDING_REQUESTS 128
#define DVZ_SCENE_MAX_REQUEST_SCOPES 256
#define DVZ_SCENE_MAX_ANIMATIONS 128



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
} DvzVisualType;



/* Image texture payload cache (field -> runtime texture realization). */
typedef struct DvzVisualTexture DvzVisualTexture;
struct DvzVisualTexture
{
    void* rgba;                 /* owned RGBA8 staging for scalar textures */
    uint64_t rgba_size;         /* bytes */
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
typedef struct DvzPanel   DvzPanel;
typedef struct DvzVisual  DvzVisual;
typedef struct DvzSampledField DvzSampledField;
typedef struct DvzSceneBuffer DvzSceneBuffer;
typedef struct DvzScale   DvzScale;
typedef struct DvzColormap DvzColormap;
typedef struct DvzColorbar DvzColorbar;
typedef struct DvzInteractionPolicy DvzInteractionPolicy;
typedef struct DvzSelection DvzSelection;
typedef struct DvzLinkChannel DvzLinkChannel;
typedef struct DvzPinnedReadout DvzPinnedReadout;
typedef struct DvzFont DvzFont;
typedef struct DvzText DvzText;
typedef struct DvzAnnotation DvzAnnotation;
typedef struct DvzAnimation DvzAnimation;



/*************************************************************************************************/
/*  Shared helpers                                                                               */
/*************************************************************************************************/

void _scene_panel_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height);



/*************************************************************************************************/
/*  Shared retained-object state                                                                 */
/*************************************************************************************************/

typedef struct DvzSceneFormatState DvzSceneFormatState;

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



/*************************************************************************************************/
/*  Animation                                                                                    */
/*************************************************************************************************/

typedef enum
{
    DVZ_ANIMATION_NONE = 0,
    DVZ_ANIMATION_TIMER,
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
    void* user_data;
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

bool _dvz_figure_fly_update(DvzFigure* figure, double dt);



/*************************************************************************************************/
/*  Scale / colormap / colorbar                                                                  */
/*************************************************************************************************/

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
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    char title[DVZ_SCENE_LABEL_SIZE];
    uint32_t flags;
    bool has_format;
    DvzSceneFormatState format;
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
    DvzProbeResult probe;
    bool has_format;
    DvzSceneFormatState format;
};


struct DvzFont
{
    DvzScene* scene;
    char path[512];
    float size_pts;
    uint32_t flags;
};


struct DvzText
{
    DvzScene* scene;
    DvzPanel* panel;
    char string[DVZ_SCENE_LABEL_SIZE];
    DvzTextStyle style;
    DvzTextPlacement placement;
    uint32_t flags;
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
};


typedef struct DvzPendingPickRequest DvzPendingPickRequest;
typedef struct DvzPendingProbeRequest DvzPendingProbeRequest;
typedef struct DvzQueuedPickResult DvzQueuedPickResult;
typedef struct DvzQueuedProbeResult DvzQueuedProbeResult;
typedef struct DvzRequestFreshnessScope DvzRequestFreshnessScope;
typedef struct DvzSceneProbePlan DvzSceneProbePlan;
typedef struct DvzSceneRequestExecutor DvzSceneRequestExecutor;

struct DvzPendingPickRequest
{
    DvzPanel* panel;
    double x;
    double y;
    uint64_t freshness_serial;
    DvzPickRequest request;
};


struct DvzPendingProbeRequest
{
    DvzPanel* panel;
    double x;
    double y;
    uint64_t freshness_serial;
    DvzProbeRequest request;
};


struct DvzQueuedPickResult
{
    DvzPanel* panel;
    uint64_t freshness_serial;
    DvzPickResult result;
};


struct DvzQueuedProbeResult
{
    DvzPanel* panel;
    uint64_t freshness_serial;
    DvzProbeResult result;
};


struct DvzRequestFreshnessScope
{
    DvzPanel* panel;
    uint64_t request_id;
    uint64_t freshness_serial;
    uint64_t touched_serial;
};


struct DvzSceneProbePlan
{
    DvzFramePlan* plan;
    vec3* probe_positions;
    vec2* probe_texcoords;
};


struct DvzSceneRequestExecutor
{
    DvzDrp2Runtime* runtime;
    DvzFramePlanEmitter* emitter;
    DvzDrp2RuntimeConfig runtime_cfg;
    DvzVisual* image_probe_visual;
    uint64_t image_probe_position_version;
    uint64_t image_probe_texcoord_version;
    uint64_t image_probe_texture_version;
    uint32_t runtime_create_count;
    uint32_t emitter_create_count;
    uint32_t image_probe_static_upload_count;
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
    DvzAlphaMode alpha_mode;
    float opacity;
    float light_direction[4];
    float ambient;
    float diffuse;
    float specular;
    float shininess;
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
    uint64_t version;
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
    uint32_t      pick_capabilities;
    DvzLinkChannel* link_channel;
    uint64_t*       link_keys;
    uint32_t        link_key_count;
    DvzSceneMaterialParams material_params;
    bool                   material_params_dirty;
    bool                   mesh_default_color;
    DvzVolumeState         volume;

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
    float params[4];
} DvzSceneSsaoUniform;



typedef struct DvzSceneSsaoTechniqueState
{
    bool enabled;
    float radius;
    float strength;
    float bias;
    uint32_t sample_count;
    DvzSceneSsaoUniform uniform;
} DvzSceneSsaoTechniqueState;



typedef struct DvzSceneTechniqueState
{
    DvzSceneGBufferTechniqueState gbuffer;
    DvzSceneEdlTechniqueState edl;
    DvzSceneSsaoTechniqueState ssao;
} DvzSceneTechniqueState;



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
    DvzSceneTechniqueState techniques;

    uint32_t       visual_count;
    DvzPanelAttach visuals[DVZ_SCENE_MAX_VISUALS];

    DvzPanzoom* panzoom; /* optional pan/zoom controller (owned) */
    DvzArcball* arcball; /* optional arcball controller (owned) */
    DvzCamera* camera;   /* optional camera (owned) */
    DvzFly* fly;         /* optional fly camera controller (owned) */
    DvzInteractionPolicy* interaction;
    DvzHoverState hover;

    /* Optional background visual created by dvz_panel_set_background_*. The visual itself
     * lives in scene->visuals[] (weak ref); this pointer lets repeat calls update the
     * existing visual instead of stacking new ones. */
    DvzVisual* background_visual;

    uint32_t colorbar_count;
    DvzColorbar* colorbars[DVZ_SCENE_MAX_PANEL_COLORBARS];
    uint32_t pinned_readout_count;
    DvzPinnedReadout* pinned_readouts[DVZ_SCENE_MAX_PINNED_READOUTS];
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

    uint32_t   panel_count;
    DvzPanel   panels[DVZ_SCENE_MAX_PANELS];
};



/*************************************************************************************************/
/*  DvzScene                                                                                    */
/*************************************************************************************************/

struct DvzScene
{
    DvzCapabilitySnapshot caps;
    DvzSceneTechniqueState techniques;

    DvzSceneClock clock;

    uint32_t animation_count;
    DvzAnimation animations[DVZ_SCENE_MAX_ANIMATIONS];

    DvzFramePlanEmitter* emitter; /* shared across all figures — owns GPU resource key→ID map */

    uint32_t outstanding_emitted_streams;

    uint32_t  figure_count;
    DvzFigure figures[DVZ_SCENE_MAX_FIGURES];

    uint32_t  visual_count;
    DvzVisual visuals[DVZ_SCENE_MAX_VISUALS]; /* owner of all visual objects */

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

    uint32_t interaction_count;
    DvzInteractionPolicy interactions[DVZ_SCENE_MAX_INTERACTIONS];

    uint32_t selection_count;
    DvzSelection selections[DVZ_SCENE_MAX_SELECTIONS];

    uint32_t link_channel_count;
    DvzLinkChannel link_channels[DVZ_SCENE_MAX_LINK_CHANNELS];

    uint32_t pinned_readout_count;
    DvzPinnedReadout pinned_readouts[DVZ_SCENE_MAX_PINNED_READOUTS];

    uint32_t font_count;
    DvzFont fonts[DVZ_SCENE_MAX_FONTS];

    uint32_t text_count;
    DvzText texts[DVZ_SCENE_MAX_TEXTS];

    uint32_t annotation_count;
    DvzAnnotation annotations[DVZ_SCENE_MAX_ANNOTATIONS];

    uint32_t pending_pick_count;
    DvzPendingPickRequest pending_picks[DVZ_SCENE_MAX_PENDING_REQUESTS];

    uint32_t pending_probe_count;
    DvzPendingProbeRequest pending_probes[DVZ_SCENE_MAX_PENDING_REQUESTS];

    uint32_t pick_result_count;
    uint32_t pick_result_head;
    DvzQueuedPickResult pick_results[DVZ_SCENE_MAX_PICK_RESULTS];

    uint32_t probe_result_count;
    uint32_t probe_result_head;
    DvzQueuedProbeResult probe_results[DVZ_SCENE_MAX_PROBE_RESULTS];
    uint64_t next_request_serial;
    uint32_t pick_scope_count;
    DvzRequestFreshnessScope pick_scopes[DVZ_SCENE_MAX_REQUEST_SCOPES];
    uint32_t probe_scope_count;
    DvzRequestFreshnessScope probe_scopes[DVZ_SCENE_MAX_REQUEST_SCOPES];

    struct
    {
        bool force_readback_download_failure;
    } test;
};



/*************************************************************************************************/
/*  Internal interaction helpers                                                                */
/*************************************************************************************************/

bool _dvz_scene_enqueue_pick_result(DvzScene* scene, const DvzPickResult* result);
bool _dvz_scene_enqueue_probe_result(DvzScene* scene, const DvzProbeResult* result);
bool _dvz_scene_enqueue_pick_result_scoped(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzPickResult* result);
bool _dvz_scene_enqueue_probe_result_scoped(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzProbeResult* result);
uint64_t _scene_next_request_serial(DvzScene* scene);
void _scene_track_pick_request_serial(
    DvzScene* scene, DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial);
void _scene_track_probe_request_serial(
    DvzScene* scene, DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial);
bool _scene_pick_request_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial);
bool _scene_probe_request_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial);
bool _scene_push_pick_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzPickResult* result);
bool _scene_push_probe_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzProbeResult* result);
void _scene_coalesce_pending_pick_requests(DvzScene* scene, const DvzFigure* figure);
void _scene_coalesce_pending_probe_requests(DvzScene* scene, const DvzFigure* figure);
void _scene_remove_pending_pick_at(DvzScene* scene, uint32_t index);
void _scene_remove_pending_probe_at(DvzScene* scene, uint32_t index);
bool _scene_pick_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc);
uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel);
void _scene_request_apply_mvp(const DvzPanel* panel, const vec2 request_ndc, DvzMVP* out);
bool _scene_point_pick_cpu(
    const DvzFigure* figure, const DvzPanel* panel, const DvzVisual* visual, double x, double y,
    uint64_t* out_item_id);
void _scene_pick_trace(const char* format, ...);
bool _scene_image_probe_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingProbeRequest* pending,
    const vec2 request_ndc, bool include_static_uploads, DvzSceneProbePlan* out_plan);
void _scene_probe_plan_destroy(DvzSceneProbePlan* plan);
void _scene_request_executor_init(DvzSceneRequestExecutor* executor);
void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor);
uint32_t _dvz_figure_process_requests_with_executor(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps);



/*************************************************************************************************/
/*  Internal shared scene helpers                                                               */
/*************************************************************************************************/

int _attr_index(const DvzVisual* visual, const char* name);

const char* _visual_type_name(DvzVisualType type);

bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index);

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

bool _scene_visual_frame_plan_metadata(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanVisualMeta* metadata);

bool _scene_color_from_colormap(const DvzColormap* colormap, double t, uint8_t out_rgba[4]);

void _scene_visual_texture_mark_clean(DvzVisual* visual);

void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field);

void _scene_release_visual_field(DvzVisual* visual);

void _scene_release_visual_buffer(DvzVisual* visual);

void _scene_field_reset(DvzSampledField* field);

void _scene_buffer_reset(DvzSceneBuffer* buffer);

void _scene_visual_reset(DvzVisual* visual, bool release_owned_resources);

uint64_t _scene_visual_public_id(const DvzScene* scene, const DvzVisual* visual);

void _scene_panel_visual_order(const DvzPanel* panel, uint32_t* order);

void _dvz_scene_animations_step(DvzScene* scene, uint64_t wall_time_ns);
