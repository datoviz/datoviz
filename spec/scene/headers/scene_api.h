/*
 * Draft header sketch derived from spec/scene/.
 * This file is the authoritative draft C spelling for the current scene API direction.
 * It is not part of the installed public API yet.
 */

/*************************************************************************************************/
/*  Scene API sketch                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/common/macros.h"

#include "diagnostics.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzCapabilitySnapshot DvzCapabilitySnapshot;

typedef struct DvzScene     DvzScene;      /* logical scene graph */
typedef struct DvzFigure    DvzFigure;     /* per-output layout container (one per window/target) */
typedef struct DvzPanel     DvzPanel;      /* viewport inside a figure */
typedef struct DvzVisual    DvzVisual;     /* one visual instance */
typedef struct DvzResource  DvzResource;   /* GPU resource (buffer, texture, …) */
typedef struct DvzScale     DvzScale;      /* mapping / colormap */
typedef struct DvzStyle     DvzStyle;      /* reusable visual defaults */
typedef struct DvzFont      DvzFont;       /* loaded typeface */
typedef struct DvzSelection DvzSelection;  /* GPU selection + highlight state */
typedef struct DvzFramePlan DvzFramePlan;  /* ordered frame execution plan */
typedef struct DvzTexture   DvzTexture;    /* 2-D or 3-D GPU texture */
typedef struct DvzRenderTarget DvzRenderTarget; /* logical output target */
typedef struct DvzRuntime  DvzRuntime;    /* runtime boundary (wraps DRP2 session) */
typedef struct DvzDrp2CommandStream DvzDrp2CommandStream; /* emitted DRP2 command stream */



/*************************************************************************************************/
/*  Visual family flags                                                                          */
/*                                                                                               */
/*  Each family constructor takes a uint32_t flags encoding variant axes.                        */
/*  Bits 0-3   render / texture mode axis (family-specific)                                      */
/*  Bits 4-7   color mode axis                                                                   */
/*  Bits 8-11  size / width mode axis                                                            */
/*  Bits 16+   feature enable bits                                                               */
/*************************************************************************************************/

/* Marker render mode (bits 0-1) */
#define DVZ_MARKER_CODE     0x00u
#define DVZ_MARKER_BITMAP   0x01u
#define DVZ_MARKER_SDF      0x02u
#define DVZ_MARKER_MSDF     0x03u

/* Color mode — shared across families (bits 4-5) */
#define DVZ_COLOR_RGBA      0x00u
#define DVZ_COLOR_SCALAR    0x10u

/* Size mode (bits 8-9) */
#define DVZ_SIZE_DIRECT     0x000u
#define DVZ_SIZE_SCALAR     0x100u

/* Image texture mode (bits 0-1) */
#define DVZ_IMAGE_RGBA      0x00u
#define DVZ_IMAGE_SCALAR    0x01u
#define DVZ_IMAGE_HEATMAP   0x02u
#define DVZ_IMAGE_NONE      0x03u

/* Volume texture mode (bits 0-1) */
#define DVZ_VOLUME_SCALAR   0x00u
#define DVZ_VOLUME_RGBA     0x01u

/* Volume color mode (bits 4-5) */
#define DVZ_VOLUME_DENSITY  0x00u
#define DVZ_VOLUME_COLORMAP 0x10u

/* Volume render mode (bits 8-9) */
#define DVZ_VOLUME_DVR      0x000u
#define DVZ_VOLUME_MIP      0x100u
#define DVZ_VOLUME_SLICE    0x200u



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

/* --- Marker shapes -------------------------------------------------------- */

typedef enum
{
    DVZ_MARKER_SHAPE_DISC         = 0,
    DVZ_MARKER_SHAPE_CIRCLE       = 1,
    DVZ_MARKER_SHAPE_SQUARE       = 2,
    DVZ_MARKER_SHAPE_DIAMOND      = 3,
    DVZ_MARKER_SHAPE_TRIANGLE     = 4,
    DVZ_MARKER_SHAPE_CROSS        = 5,
    DVZ_MARKER_SHAPE_ASTERISK     = 6,
    DVZ_MARKER_SHAPE_CHEVRON      = 7,
    DVZ_MARKER_SHAPE_CLOVER       = 8,
    DVZ_MARKER_SHAPE_CLUB         = 9,
    DVZ_MARKER_SHAPE_SPADE        = 10,
    DVZ_MARKER_SHAPE_HEART        = 11,
    DVZ_MARKER_SHAPE_ARROW        = 12,
    DVZ_MARKER_SHAPE_ELLIPSE      = 13,
    DVZ_MARKER_SHAPE_HBAR         = 14,
    DVZ_MARKER_SHAPE_VBAR         = 15,
    DVZ_MARKER_SHAPE_RING         = 16,
    DVZ_MARKER_SHAPE_PIN          = 17,
    DVZ_MARKER_SHAPE_TAG          = 18,
    DVZ_MARKER_SHAPE_ROUNDED_RECT = 19,
} DvzMarkerShape;


/* --- Marker aspect -------------------------------------------------------- */

typedef enum
{
    DVZ_MARKER_ASPECT_FILLED  = 0, /* solid fill, no edge */
    DVZ_MARKER_ASPECT_STROKE  = 1, /* edge only */
    DVZ_MARKER_ASPECT_OUTLINE = 2, /* fill + edge */
} DvzMarkerAspect;


/* --- Arrow style (shared: marker arrow_style, segment/path caps) ---------- */

typedef enum
{
    DVZ_ARROW_FILLED  = 0,
    DVZ_ARROW_OPEN    = 1,
    DVZ_ARROW_STEALTH = 2,
    DVZ_ARROW_CIRCLE  = 3,
} DvzArrowStyle;


/* --- Cap type (path cap_start/cap_end, segment end caps) ------------------ */

typedef enum
{
    DVZ_CAP_NONE          = 0,
    DVZ_CAP_ROUND         = 1,
    DVZ_CAP_BUTT          = 2,
    DVZ_CAP_SQUARE        = 3,
    DVZ_CAP_TRIANGLE      = 4,
    DVZ_CAP_ARROW_FILLED  = 5,
    DVZ_CAP_ARROW_OPEN    = 6,
    DVZ_CAP_ARROW_STEALTH = 7,
    DVZ_CAP_ARROW_CIRCLE  = 8,
} DvzCapType;


/* --- Path join style ------------------------------------------------------ */

typedef enum
{
    DVZ_JOIN_MITER = 0,
    DVZ_JOIN_ROUND = 1,
    DVZ_JOIN_BEVEL = 2,
} DvzJoinType;


/* --- Primitive topology --------------------------------------------------- */

typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST          = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST           = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP          = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST       = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP      = 4,
} DvzPrimitiveTopology;


/* --- Boxplot style -------------------------------------------------------- */

typedef enum
{
    DVZ_BOXPLOT_UNIFORM     = 0, /* classical: whiskers + box + median */
    DVZ_BOXPLOT_DIRECTIONAL = 1, /* candlestick / OHLC */
    DVZ_BOXPLOT_NOTCHED     = 2, /* confidence-interval notch at median */
} DvzBoxplotStyle;


/* --- Volume slice axis ---------------------------------------------------- */

typedef enum
{
    DVZ_VOLUME_SLICE_X = 0,
    DVZ_VOLUME_SLICE_Y = 1,
    DVZ_VOLUME_SLICE_Z = 2,
} DvzVolumeSliceAxis;


/* --- Volume ray direction ------------------------------------------------- */

typedef enum
{
    DVZ_VOLUME_FRONT_BACK = 0,
    DVZ_VOLUME_BACK_FRONT = 1,
} DvzVolumeDirection;


/* --- Size space ----------------------------------------------------------- */

typedef enum
{
    DVZ_SIZE_SPACE_SCREEN = 0,
    DVZ_SIZE_SPACE_DATA   = 1,
} DvzSizeSpace;


/* --- Alpha / blending mode ------------------------------------------------ */

typedef enum
{
    DVZ_ALPHA_OPAQUE       = 0, /* all items fully opaque, no blending */
    DVZ_ALPHA_BLENDED      = 1, /* weighted blended OIT (default for transparent visuals) */
    DVZ_ALPHA_BLENDED_EXACT = 2, /* per-pixel linked list OIT (deferred to v0.4+) */
    DVZ_ALPHA_MASK         = 3, /* binary alpha cut-off (aliased) */
} DvzAlphaMode;


/* --- Clip mode ------------------------------------------------------------ */

typedef enum
{
    DVZ_CLIP_DATA_AREA = 0, /* clip to inner data area (inside axes margins) — default */
    DVZ_CLIP_PANEL     = 1, /* clip to full panel extent */
    DVZ_CLIP_NONE      = 2, /* no clipping (default for axes/annotation visuals) */
} DvzClipMode;


/* --- Selection mode ------------------------------------------------------- */

typedef enum
{
    DVZ_SELECT_REPLACE  = 0,
    DVZ_SELECT_ADDITIVE = 1,
    DVZ_SELECT_SUBTRACT = 2,
    DVZ_SELECT_TOGGLE   = 3,
} DvzSelectMode;


/* --- Built-in non-linear coordinate transform types ----------------------- */

typedef enum
{
    DVZ_COORD_TRANSFORM_NONE             = 0, /* identity (default) */
    DVZ_COORD_TRANSFORM_MERCATOR         = 1,
    DVZ_COORD_TRANSFORM_EQUIRECTANGULAR  = 2,
    DVZ_COORD_TRANSFORM_ORTHOGRAPHIC_GEO = 3,
    DVZ_COORD_TRANSFORM_POLAR            = 4,
    DVZ_COORD_TRANSFORM_CUSTOM           = 5,
} DvzCoordTransformType;


/* --- Event types ---------------------------------------------------------- */

typedef enum
{
    DVZ_EVENT_SELECTION_CHANGED = 0,
    DVZ_EVENT_PICK_RESULT       = 1,
    DVZ_EVENT_HOVER             = 2,
    DVZ_EVENT_ANIM_STEP         = 3,
    DVZ_EVENT_ANIM_COMPLETE     = 4,
    DVZ_EVENT_RESIZE            = 5,
    DVZ_EVENT_DPI_CHANGED       = 6,
} DvzEventType;


/* --- Transfer types ------------------------------------------------------- */

typedef enum
{
    DVZ_TRANSFER_DATA     = 0, /* upload bytes to a resource attribute */
    DVZ_TRANSFER_UNIFORM  = 1, /* write a visual-wide parameter value */
    DVZ_TRANSFER_CALLBACK = 2, /* run a closure on the render thread */
} DvzTransferType;


/* --- Panel controller type ------------------------------------------------ */

typedef enum
{
    DVZ_CONTROLLER_NONE   = 0,
    DVZ_CONTROLLER_PANZOOM = 1,
    DVZ_CONTROLLER_ARCBALL = 2,
    DVZ_CONTROLLER_FLY    = 3,
} DvzControllerType;


/* --- Redraw scope --------------------------------------------------------- */

typedef enum
{
    DVZ_REDRAW_SCENE = 0,
    DVZ_REDRAW_PANEL = 1,
} DvzRedrawScope;


/* --- Pick kind ------------------------------------------------------------ */

typedef enum
{
    DVZ_PICK_HOVER = 0,
    DVZ_PICK_CLICK = 1,
    DVZ_PICK_QUERY = 2,
} DvzPickKind;


/* --- Attribute mutability hint -------------------------------------------- */

typedef enum
{
    DVZ_MUTABILITY_DYNAMIC   = 0, /* default: scene copies data on write */
    DVZ_MUTABILITY_STATIC    = 1, /* upload once; no further writes expected */
    DVZ_MUTABILITY_STREAMING = 2, /* updated every frame; scene uses mapped memory */
} DvzMutability;


/* --- Panel side (for colorbar attachment) --------------------------------- */

typedef enum
{
    DVZ_PANEL_SIDE_TOP    = 0,
    DVZ_PANEL_SIDE_RIGHT  = 1,
    DVZ_PANEL_SIDE_BOTTOM = 2,
    DVZ_PANEL_SIDE_LEFT   = 3,
} DvzPanelSide;


/* --- Offscreen panel flags ------------------------------------------------ */

#define DVZ_PANEL_OFFSCREEN_DEFAULT 0x00u /* render to texture only */
#define DVZ_PANEL_OFFSCREEN_PIP     0x01u /* also composite into main framebuffer */


/* --- Frame-build flags ---------------------------------------------------- */

typedef enum
{
    DVZ_BUILD_FLAGS_NONE            = 0x00,
    DVZ_BUILD_FLAGS_OFFSCREEN_ONLY  = 0x01,
    DVZ_BUILD_FLAGS_INCLUDE_PICKING = 0x02,
} DvzBuildFlags;


/* --- Datoviz-owned texture / target formats -------------------------------- */

typedef enum
{
    DVZ_FORMAT_UNDEFINED  = 0,
    DVZ_FORMAT_RGBA8_UNORM = 1,
    DVZ_FORMAT_BGRA8_UNORM = 2,
    DVZ_FORMAT_R32_UINT    = 3,
    DVZ_FORMAT_R32_FLOAT   = 4,
    DVZ_FORMAT_RGBA32_FLOAT = 5,
    DVZ_FORMAT_DEPTH24_STENCIL8 = 6,
    DVZ_FORMAT_DEPTH32_FLOAT = 7,
} DvzFormat;


/* --- Shader source language ------------------------------------------------ */

typedef enum
{
    DVZ_SHADER_SOURCE_WGSL  = 0, /* portable DRP2 contract language */
    DVZ_SHADER_SOURCE_GLSL  = 1, /* accepted only through explicit runtime capability */
    DVZ_SHADER_SOURCE_SPIRV = 2, /* native-only precompiled path */
} DvzShaderSourceLanguage;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/* --- Core creation descriptors ------------------------------------------- */

typedef struct
{
    uint32_t initial_panel_capacity;
    uint32_t initial_visual_capacity;
    uint32_t initial_resource_capacity;
} DvzSceneCreateDesc;


typedef struct
{
    float x, y;           /* top-left corner in normalized figure coordinates [0,1] */
    float width, height;  /* extent in normalized figure coordinates */
} DvzPanelDesc;


/* --- Pick request / result ------------------------------------------------ */

typedef struct
{
    DvzPickKind kind;
    uint64_t    request_id;
    double      x, y;          /* figure-space coordinates */
} DvzPickRequest;


typedef struct
{
    uint64_t request_id;
    uint64_t panel_id;
    uint64_t visual_id;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id; /* level index for isosurface, face index for mesh, … */
    bool     valid;
} DvzPickResult;


/* --- Highlight descriptor (used by selection) ----------------------------- */

typedef struct
{
    float    alpha_scale;         /* multiplier on per-item alpha; 1 = unchanged */
    uint8_t  color_tint[4];       /* additive RGBA tint; (0,0,0,0) = none */
    float    size_scale;          /* multiplier on size; 1 = unchanged */
    int32_t  z_layer_offset;      /* z_layer delta for selected items */
    uint8_t  edgecolor[4];        /* edge color override; alpha=0 disables */
    float    edgewidth;           /* edge width override; 0 = unchanged */
} DvzHighlightDesc;


/* --- Selection input mapping ---------------------------------------------- */

typedef struct
{
    bool     click_select;        /* single click → replace selection */
    bool     shift_additive;      /* shift+click → additive */
    bool     ctrl_subtract;       /* ctrl+click → subtract */
    bool     box_select;          /* click-drag → rectangular box select */
    bool     lasso_select;        /* shift+drag → lasso select */
} DvzSelectionInputMap;


/* --- Event callback payloads ---------------------------------------------- */

typedef struct
{
    DvzSelection* selection;
    uint64_t      visual_id;
    uint64_t      item_count;
} DvzEventSelectionChanged;


typedef struct
{
    DvzPickResult result;
} DvzEventPickResult;


typedef struct
{
    uint64_t panel_id;
    double   x, y;          /* data-space coordinates */
    uint64_t visual_id;
    uint64_t item_id;
    bool     over_item;
} DvzEventHover;


typedef struct
{
    double t;               /* animation time, seconds from start */
    double dt;              /* delta from previous step */
} DvzEventAnimStep;


typedef struct
{
    uint32_t width, height; /* new figure size in logical pixels */
    float    dpi_scale;
} DvzEventResize;


typedef struct
{
    float dpi_scale_old;
    float dpi_scale_new;
} DvzEventDpiChanged;


typedef union
{
    DvzEventSelectionChanged selection_changed;
    DvzEventPickResult       pick_result;
    DvzEventHover            hover;
    DvzEventAnimStep         anim_step;
    DvzEventResize           resize;
    DvzEventDpiChanged       dpi_changed;
} DvzEventPayload;


/* Callback signature for dvz_scene_on */
typedef void (*DvzEventCallback)(DvzScene*, DvzEventType, DvzEventPayload*, void* user_data);


/* --- Transfer descriptor -------------------------------------------------- */

typedef void (*DvzTransferCallback)(DvzScene* scene, void* user_data);

typedef struct
{
    DvzTransferType type;
    DvzVisual*      visual;       /* target visual (DATA, UNIFORM) */
    const char*     attr_name;    /* attribute name (DATA) or param name (UNIFORM) */
    uint64_t        byte_offset;  /* byte range start (DATA) */
    uint64_t        byte_size;    /* byte range length (DATA) */
    const void*     data;         /* pointer to source bytes (DATA, UNIFORM) */
    DvzTransferCallback callback; /* closure to run on render thread (CALLBACK) */
    void*           user_data;
    bool            zero_copy;    /* keep data pointer alive until consumed (DATA) */
} DvzTransferDesc;


/* Completion callback: fired after the frame in which the transfer was applied */
typedef void (*DvzTransferDoneCallback)(DvzScene* scene, const DvzTransferDesc* transfer,
                                        void* user_data);


/* --- Export options ------------------------------------------------------- */

typedef struct
{
    float    render_scale;   /* >1 for higher-resolution export */
    bool     embed_fonts;    /* embed font data in SVG output */
    float    dpi;            /* target DPI for raster embeds; 0 = screen DPI */
} DvzSVGExportOptions;


/* --- Non-linear coordinate transform descriptor --------------------------- */

typedef struct
{
    DvzCoordTransformType type;
    double                params[8];  /* type-specific parameters */
    /* custom compute shader source, used when type = DVZ_COORD_TRANSFORM_CUSTOM */
    DvzShaderSourceLanguage shader_language;
    const char*             shader_source;
} DvzCoordTransformDesc;


/* --- Custom visual descriptor --------------------------------------------- */

typedef struct
{
    const char* name;         /* attribute name as used in dvz_visual_set_data */
    uint32_t    binding;      /* vertex buffer binding slot */
    uint32_t    location;     /* shader input location */
    DvzFormat   format;       /* Datoviz-owned vertex format; UNDEFINED = auto-infer */
    const char* type_name;    /* shader type string, e.g. "vec3f" or "vec3" */
    bool        per_item;     /* true = per-vertex/item; false = per-instance/group */
} DvzVisualAttributeDesc;


typedef struct
{
    /* shader sources. WGSL is the portable DRP2 contract language; GLSL/SPIR-V require capability. */
    DvzShaderSourceLanguage shader_language;
    const char* vertex_source;
    const char* fragment_source;
    const char* compute_source; /* optional pre-pass; NULL if not used */

    /* attribute schema */
    const DvzVisualAttributeDesc* attributes;
    uint32_t                      attribute_count;

    /* uniform parameters (struct layout copied verbatim to UBO) */
    const char* params_struct;      /* shader-language-specific parameter struct text */
    uint32_t    params_size;        /* sizeof the params struct */

    /* texture slots */
    uint32_t texture_slot_count;    /* how many sampler bindings the shader expects */

    /* feature opt-ins */
    bool supports_picking;
    bool supports_selection;
    bool supports_clipping;
} DvzCustomVisualDesc;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/* ========================================================================= */
/* Scene lifecycle                                                            */
/* ========================================================================= */

DVZ_EXPORT DvzScene* dvz_scene_create(const DvzSceneCreateDesc* desc);

DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);

/* Inject the runtime capability snapshot used by validate / adapt. */
DVZ_EXPORT int dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps);

DVZ_EXPORT int dvz_scene_validate(DvzScene* scene, DvzDiagnosticReport* out_report);

DVZ_EXPORT int dvz_scene_adapt(DvzScene* scene, DvzDiagnosticReport* out_report);

DVZ_EXPORT float dvz_scene_dpi_scale(DvzScene* scene);


/* ========================================================================= */
/* Figure                                                                      */
/* ========================================================================= */

/* Create a figure owned by the scene.
 * width/height are in logical pixels; pass 0 to inherit from the window. */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height,
                                  uint32_t flags);

DVZ_EXPORT void dvz_figure_destroy(DvzFigure* fig);

/* Scale rendering resolution relative to the logical pixel size (e.g. 2.0 for 2× export).
 * Stacks multiplicatively with dpi_scale. */
DVZ_EXPORT void dvz_figure_set_render_scale(DvzFigure* fig, float scale);

/* Bind the logical runtime-resolved render target used by frame planning. */
DVZ_EXPORT void dvz_figure_set_target(DvzFigure* fig, DvzRenderTarget* target);

/* Lay out n_rows × n_cols equal panels in a grid, returning an array of panel handles.
 * Caller must free the returned array. */
DVZ_EXPORT DvzPanel** dvz_figure_grid(DvzFigure* fig, uint32_t n_rows, uint32_t n_cols);

/* Export the figure to a PNG file.
 * Drives one offline render at the current render_scale * dpi_scale. */
DVZ_EXPORT int dvz_figure_export_png(DvzFigure* fig, const char* path);

/* Export a structural SVG: axes / annotations / colorbars as SVG elements,
 * GPU visual content embedded as raster PNG. */
DVZ_EXPORT int dvz_figure_export_svg(DvzFigure* fig, const char* path,
                                      const DvzSVGExportOptions* opts);

/* Build and return the ordered frame execution plan. Caller drives frame execution. */
DVZ_EXPORT DvzFramePlan* dvz_figure_build_frame(DvzFigure* fig, DvzBuildFlags flags,
                                                  DvzDiagnosticReport* out_report);

DVZ_EXPORT void dvz_frame_plan_destroy(DvzFramePlan* fp);

/* Request a redraw at the scene or panel scope. */
DVZ_EXPORT int dvz_figure_request_redraw(DvzFigure* fig, DvzRedrawScope scope, DvzPanel* panel);


/* ========================================================================= */
/* Panel                                                                      */
/* ========================================================================= */

/* Create a panel with an explicit normalized position and size. */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* fig, const DvzPanelDesc* desc);

DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);

/* Assign an interactive controller to the panel. */
DVZ_EXPORT void dvz_panel_set_controller(DvzPanel* panel, DvzControllerType type);

/* Set a non-linear coordinate transform for the panel.
 * Positions are transformed CPU-side (v0.4); GPU compute pre-pass is a v0.4+ feature. */
DVZ_EXPORT void dvz_panel_set_coord_transform(DvzPanel* panel, const DvzCoordTransformDesc* transform);

/* Add a visual to the panel.
 * Insertion order determines draw order among visuals sharing the same z_layer. */
DVZ_EXPORT int dvz_panel_add_visual(DvzPanel* panel, DvzVisual* visual);

/* Mark the panel as offscreen.
 * Returns a DvzTexture* the caller can bind to another visual (e.g. image).
 * The scene ensures the offscreen RenderNode runs before any node that samples the texture.
 * flags: DVZ_PANEL_OFFSCREEN_DEFAULT (exclusive) or DVZ_PANEL_OFFSCREEN_PIP (also composites
 * into the main framebuffer). */
DVZ_EXPORT DvzTexture* dvz_panel_set_offscreen(DvzPanel* panel, uint32_t flags);

/* Create a fixed-width adjacent panel for a colorbar and return its handle.
 * Wraps the grid setup + margin boilerplate for the common colorbar-beside-panel layout. */
DVZ_EXPORT DvzPanel* dvz_panel_attach_colorbar(DvzPanel* panel, DvzPanelSide side,
                                                float width_px);


/* ========================================================================= */
/* Resources                                                                  */
/* ========================================================================= */

/* Create a 2-D GPU texture. */
DVZ_EXPORT DvzTexture* dvz_texture_2d(DvzScene* scene, uint32_t width, uint32_t height,
                                       DvzFormat format);

/* Create a 3-D GPU texture. */
DVZ_EXPORT DvzTexture* dvz_texture_3d(DvzScene* scene, uint32_t width, uint32_t height,
                                       uint32_t depth, DvzFormat format);

/* Upload bytes to a texture sub-region. */
DVZ_EXPORT int dvz_texture_upload(DvzTexture* tex, uint32_t x, uint32_t y, uint32_t z,
                                   uint32_t w, uint32_t h, uint32_t d, const void* data);

DVZ_EXPORT void dvz_texture_destroy(DvzTexture* tex);

/* Create a reusable group of visual defaults. Individual visual parameters remain settable with
 * dvz_visual_set_param(). */
DVZ_EXPORT DvzStyle* dvz_style(DvzScene* scene);

DVZ_EXPORT void dvz_style_destroy(DvzStyle* style);

DVZ_EXPORT int dvz_style_set_param(DvzStyle* style, const char* name, const void* value);

DVZ_EXPORT int dvz_visual_set_style(DvzVisual* visual, DvzStyle* style);

/* Create a DvzScale (colormap / size mapping) — generic form. */
DVZ_EXPORT DvzScale* dvz_scale(DvzScene* scene, uint32_t kind_flags);

/* Typed scale constructors — preferred over the generic form. */
DVZ_EXPORT DvzScale* dvz_scale_color(DvzScene* scene, const char* colormap_name,
                                      double domain_min, double domain_max);
DVZ_EXPORT DvzScale* dvz_scale_size(DvzScene* scene, float px_min, float px_max,
                                     double domain_min, double domain_max);
DVZ_EXPORT DvzScale* dvz_scale_opacity(DvzScene* scene, double domain_min, double domain_max);

/* Update an existing scale in place. Marks all referencing visuals dirty. */
DVZ_EXPORT void dvz_scale_set_domain(DvzScale* scale, double min, double max);
DVZ_EXPORT void dvz_scale_set_colormap(DvzScale* scale, const char* colormap_name);
DVZ_EXPORT void dvz_scale_set_stops(DvzScale* scale, const uint8_t* rgba_stops, uint32_t count);
DVZ_EXPORT void dvz_scale_destroy(DvzScale* scale);

/* Register a custom colormap by name (256 RGBA entries). */
DVZ_EXPORT int dvz_colormap_register(DvzScene* scene, const char* name,
                                      const uint8_t* rgba256);

/* Load a font from a TTF/OTF file, returning a font handle for use with glyph visuals. */
DVZ_EXPORT DvzFont* dvz_font_load(DvzScene* scene, const char* path, float size_pts);

DVZ_EXPORT void dvz_font_destroy(DvzFont* font);


/* ========================================================================= */
/* Visual family constructors                                                 */
/*                                                                            */
/* Each returns a DvzVisual* owned by the scene.                              */
/* flags encodes variant axes; see the DVZ_* flag definitions above.          */
/* ========================================================================= */

DVZ_EXPORT DvzVisual* dvz_pixel(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_point(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_marker(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_segment(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_path(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_glyph(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_image(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_primitive(DvzScene* scene, DvzPrimitiveTopology topology,
                                     uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_sphere(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_volume(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_errorbar(DvzScene* scene, uint32_t flags);

DVZ_EXPORT DvzVisual* dvz_boxplot(DvzScene* scene, DvzBoxplotStyle style, uint32_t flags);

DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);


/* ========================================================================= */
/* Visual — attribute and parameter data                                      */
/* ========================================================================= */

/* Hint the expected item count for pre-allocation. */
DVZ_EXPORT void dvz_visual_alloc(DvzVisual* visual, uint32_t item_count);

/* Write attribute data by name.
 * attr_name: e.g. "position", "color", "size" — matches spec attribute names.
 * data:      packed array with n items (or groups when attr is PER_GROUP).
 * n:         item count.
 * The scene deduces element size from the attribute schema. */
DVZ_EXPORT int dvz_visual_set_data(DvzVisual* visual, const char* attr_name,
                                    const void* data, uint32_t n);

/* Partial upload: replace a byte range inside an existing attribute buffer. */
DVZ_EXPORT int dvz_visual_set_data_range(DvzVisual* visual, const char* attr_name,
                                          const void* data, uint64_t byte_offset,
                                          uint64_t byte_count);

/* Write a visual-wide parameter (uniform) by name.
 * Matches the parameter names listed in each visual family spec.
 * value points to exactly the expected struct / scalar for that parameter. */
DVZ_EXPORT int dvz_visual_set_param(DvzVisual* visual, const char* param_name,
                                     const void* value);

/* Bind a texture resource to a named parameter slot (e.g. "texture", "colormap"). */
DVZ_EXPORT int dvz_visual_set_texture(DvzVisual* visual, const char* slot_name,
                                       DvzTexture* tex);

/* Bind a Scale to a named mapping slot. */
DVZ_EXPORT int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name,
                                     DvzScale* scale);


/* ========================================================================= */
/* Visual — display properties                                                */
/* ========================================================================= */

DVZ_EXPORT void dvz_visual_set_visible(DvzVisual* visual, bool visible);

/* z_layer: draw order relative to other visuals in the same panel.
 * Ties broken by insertion order (FIFO). */
DVZ_EXPORT void dvz_visual_set_z_layer(DvzVisual* visual, int32_t z_layer);

/* Per-visual alpha / blending mode — see DvzAlphaMode. */
DVZ_EXPORT void dvz_visual_set_alpha_mode(DvzVisual* visual, DvzAlphaMode mode);

/* Clipping region — see DvzClipMode. */
DVZ_EXPORT void dvz_visual_set_clip(DvzVisual* visual, DvzClipMode mode);

/* Set the mutability hint for one named attribute.
 * Default is DVZ_MUTABILITY_DYNAMIC; call before the first dvz_visual_set_data for best effect. */
DVZ_EXPORT void dvz_visual_set_mutability(DvzVisual* visual, const char* attr_name,
                                           DvzMutability hint);

/* Link a font to a glyph visual. */
DVZ_EXPORT void dvz_visual_set_font(DvzVisual* visual, DvzFont* font);


/* ========================================================================= */
/* Selection                                                                  */
/* ========================================================================= */

/* Create a selection object. Owns a GPU uint8 mask buffer. */
DVZ_EXPORT DvzSelection* dvz_selection(DvzScene* scene);

DVZ_EXPORT void dvz_selection_destroy(DvzSelection* sel);

/* Connect a visual to the selection so its mask buffer is updated on selection change. */
DVZ_EXPORT void dvz_selection_link_visual(DvzSelection* sel, DvzVisual* visual);

/* Configure how input events drive selection operations. */
DVZ_EXPORT void dvz_selection_set_input_map(DvzSelection* sel,
                                             const DvzSelectionInputMap* map);

/* Set the visual highlight applied to selected items. */
DVZ_EXPORT void dvz_selection_set_highlight(DvzSelection* sel,
                                             const DvzHighlightDesc* desc);

/* Programmatically replace the selection with an explicit item index list. */
DVZ_EXPORT int dvz_selection_set(DvzSelection* sel, const uint64_t* item_ids,
                                  uint32_t count, DvzSelectMode mode);

/* Remove all selected items from the selection. */
DVZ_EXPORT void dvz_selection_clear(DvzSelection* sel);

/* Return the current selection count. */
DVZ_EXPORT uint32_t dvz_selection_count(const DvzSelection* sel);

/* Copy the current selected item indices into out_ids (caller allocates). */
DVZ_EXPORT void dvz_selection_get(const DvzSelection* sel, uint64_t* out_ids,
                                   uint32_t max_count);

/* Trigger a one-shot GPU→CPU readback of the selection mask. The result is available on the
 * next rendered frame via dvz_selection_count / dvz_selection_get. Opt-in: automatic
 * readback is not performed every frame. */
DVZ_EXPORT void dvz_selection_sync(DvzSelection* sel);


/* ========================================================================= */
/* Event callbacks                                                            */
/* ========================================================================= */

/* Register a callback for an event type.
 * Returns an opaque handle that can be passed to dvz_scene_off to unregister. */
DVZ_EXPORT uint64_t dvz_scene_on(DvzScene* scene, DvzEventType event,
                                  DvzEventCallback callback, void* user_data);

/* Unregister a previously registered callback by handle. */
DVZ_EXPORT void dvz_scene_off(DvzScene* scene, uint64_t callback_handle);


/* ========================================================================= */
/* Thread-safe transfer queue                                                 */
/* ========================================================================= */

/* Enqueue a transfer from any thread.
 * The transfer is consumed during stage 2 of the render loop (before invalidation).
 * DVZ_TRANSFER_CALLBACK may perform structural mutations since it runs on the render thread. */
DVZ_EXPORT int dvz_scene_submit_transfer(DvzScene* scene, const DvzTransferDesc* transfer);

/* Register a one-shot callback fired after the frame in which transfer is applied. */
DVZ_EXPORT void dvz_transfer_on_rendered(DvzScene* scene, uint64_t transfer_id,
                                          DvzTransferDoneCallback cb, void* user_data);


/* ========================================================================= */
/* Custom visuals                                                             */
/* ========================================================================= */

/* Create a custom visual from a descriptor containing shader sources, attribute schema, and uniform
 * layout. WGSL is the portable path; GLSL/SPIR-V require explicit runtime capability. The visual
 * participates in the standard frame lifecycle when the corresponding flags are set. */
DVZ_EXPORT DvzVisual* dvz_visual_custom(DvzScene* scene, const DvzCustomVisualDesc* desc,
                                         uint32_t flags);


/* ========================================================================= */
/* Picking                                                                    */
/* ========================================================================= */

/* Enqueue an asynchronous pick request. Result is delivered via DVZ_EVENT_PICK_RESULT
 * callback or polled with dvz_scene_poll_pick_result. */
DVZ_EXPORT int dvz_scene_request_pick(DvzScene* scene, DvzPanel* panel,
                                       const DvzPickRequest* req);

/* Poll one interpreted pick result. Returns true when a result was written. */
DVZ_EXPORT bool dvz_scene_poll_pick_result(DvzScene* scene, DvzPickResult* out_result);


/* ========================================================================= */
/* Runtime boundary                                                           */
/* ========================================================================= */

/* Create the runtime. Init params are backend-specific (e.g. window handle, Vulkan instance).
 * The runtime wraps the DRP2 session and hides encoder/queue lifecycle from the scene layer. */
DVZ_EXPORT DvzRuntime* dvz_runtime_create(void* backend_init_params);

DVZ_EXPORT void dvz_runtime_destroy(DvzRuntime* rt);

/* Create a runtime-resolved target backed by application/canvas presentation resources.
 * The token type is intentionally opaque in the draft API. */
DVZ_EXPORT DvzRenderTarget* dvz_runtime_target_canvas(DvzRuntime* rt, void* app_canvas_token);

/* Create a runtime-resolved offscreen/readback-capable target. */
DVZ_EXPORT DvzRenderTarget* dvz_runtime_target_offscreen(DvzRuntime* rt, uint32_t width,
                                                          uint32_t height, DvzFormat format);

DVZ_EXPORT void dvz_render_target_destroy(DvzRenderTarget* target);

/* Submit an already emitted DRP2 command stream. This is the primary runtime boundary. */
DVZ_EXPORT int dvz_runtime_submit_commands(DvzRuntime* rt, DvzDrp2CommandStream* commands);

/* Convenience helper. Translates the plan to DRP2 commands and submits them.
 * Returns 0 on success; negative on validation or execution failure. */
DVZ_EXPORT int dvz_runtime_submit_frame_plan(DvzRuntime* rt, DvzFramePlan* frame_plan);

/* Query the runtime capability snapshot. Used by dvz_scene_set_capabilities. */
DVZ_EXPORT int dvz_runtime_get_capabilities(DvzRuntime* rt, DvzCapabilitySnapshot* out_caps);


EXTERN_C_OFF
