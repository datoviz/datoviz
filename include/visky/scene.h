#ifndef VKY_SCENE_HEADER
#define VKY_SCENE_HEADER

#include "app.h"
#include "colormaps.h"
#include "constants.h"
#include "ticks.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_VISUAL_EMPTY = 0,

    VKY_VISUAL_RECTANGLE = 10,
    VKY_VISUAL_RECTANGLE_AXIS = 11,
    VKY_VISUAL_AREA = 12,

    VKY_VISUAL_MESH = 20,
    VKY_VISUAL_MESH_RAW = 21,

    VKY_VISUAL_MARKER = 30,
    VKY_VISUAL_MARKER_RAW = 31,
    VKY_VISUAL_MARKER_TRANSIENT = 32,

    VKY_VISUAL_SEGMENT = 35,
    VKY_VISUAL_ARROW = 36,
    VKY_VISUAL_GRAPH = 37,

    VKY_VISUAL_PATH = 40,
    VKY_VISUAL_PATH_RAW = 41,
    VKY_VISUAL_PATH_RAW_MULTI = 42,

    VKY_VISUAL_POLYGON = 50,
    VKY_VISUAL_PSLG = 51,
    VKY_VISUAL_TRIANGULATION = 52,

    VKY_VISUAL_FAKE_SPHERE = 60,

    VKY_VISUAL_AXES_TEXT = 70,
    VKY_VISUAL_AXES_TICK = 71,

    VKY_VISUAL_AXES_3D = 72,
    VKY_VISUAL_AXES_3D_SEGMENTS = 73,
    VKY_VISUAL_AXES_3D_TEXT = 74,

    VKY_VISUAL_IMAGE = 80,
    VKY_VISUAL_IMAGE_CMAP = 81,
    VKY_VISUAL_VOLUME = 85,
    VKY_VISUAL_VOLUME_SLICER = 86,

    VKY_VISUAL_COLORBAR = 90,
    VKY_VISUAL_TEXT = 100,
    VKY_VISUAL_CUSTOM = 255,

} VkyVisualType;


typedef enum
{
    VKY_VISUAL_PROP_NONE = 0,
    VKY_VISUAL_PROP_POS = 1,
    VKY_VISUAL_PROP_POS_GPU = 2,
    VKY_VISUAL_PROP_TEXTURE_COORDS = 3,
    VKY_VISUAL_PROP_NORMAL = 4,
    VKY_VISUAL_PROP_COLOR = 5,
    VKY_VISUAL_PROP_SIZE = 10,
    VKY_VISUAL_PROP_SHAPE = 11,
    VKY_VISUAL_PROP_SHIFT = 12,
    VKY_VISUAL_PROP_LENGTH = 13,
    VKY_VISUAL_PROP_AXIS = 14,
    VKY_VISUAL_PROP_TEXT = 15,
    VKY_VISUAL_PROP_ANGLE = 16,
    VKY_VISUAL_PROP_TIME = 17,
    VKY_VISUAL_PROP_LINEWIDTH = 20,
    VKY_VISUAL_PROP_EDGE_COLOR = 21,
    VKY_VISUAL_PROP_EDGE_ALPHA = 22,
    VKY_VISUAL_PROP_IMAGE = 24,
    VKY_VISUAL_PROP_VOLUME = 25,
    VKY_VISUAL_PROP_BUFFER = 26,
    VKY_VISUAL_PROP_TRANSFORM = 31,
} VkyVisualPropType;


typedef enum
{
    VKY_CONTROLLER_NONE = 0, // NOTE: need to the the first so that it is 0 = default

    VKY_CONTROLLER_PANZOOM = 10,
    VKY_CONTROLLER_AXES_2D = 11,

    VKY_CONTROLLER_ARCBALL = 20,
    VKY_CONTROLLER_TURNTABLE = 21,
    VKY_CONTROLLER_AUTOROTATE = 22,
    VKY_CONTROLLER_AXES_3D = 23,

    VKY_CONTROLLER_FLY = 30,
    VKY_CONTROLLER_FPS = 31,

    VKY_CONTROLLER_IMAGE = 40,
    VKY_CONTROLLER_VOLUME = 41,
} VkyControllerType;


typedef enum
{
    VKY_MVP_MODEL = 1,
    VKY_MVP_VIEW = 2,
    VKY_MVP_PROJ = 3,
    VKY_MVP_ORTHO = 4,
} VkyMVPMatrix;


typedef enum
{
    VKY_VIEWPORT_INNER = 0,
    VKY_VIEWPORT_OUTER = 1,
} VkyViewportType;


typedef enum
{
    VKY_ASPECT_UNCONSTRAINED = 0,
    VKY_ASPECT_SQUARE = 1,
} VkyAspect;


typedef enum
{
    VKY_VISUAL_PRIORITY_NONE = 0,
    VKY_VISUAL_PRIORITY_FIRST = 1,
    VKY_VISUAL_PRIORITY_LAST = 2,
} VkyVisualPriority;


typedef enum
{
    VKY_PANEL_LINK_NONE = 0x0,
    VKY_PANEL_LINK_X = 0x1,
    VKY_PANEL_LINK_Y = 0x2,
    VKY_PANEL_LINK_Z = 0x4,
    VKY_PANEL_LINK_ALL = 0x7,
} VkyPanelLinkMode;


typedef enum
{
    VKY_TRANSFORM_MODE_NORMAL = 0x0,
    VKY_TRANSFORM_MODE_X_ONLY = 0x1,
    VKY_TRANSFORM_MODE_Y_ONLY = 0x2,
    VKY_TRANSFORM_MODE_STATIC = 0x7,
} VkyTransformMode;


typedef enum
{
    VKY_CONTROLLER_SOURCE_NONE = 0,
    VKY_CONTROLLER_SOURCE_HUMAN = 1,
    VKY_CONTROLLER_SOURCE_MOCK = 2,
    VKY_CONTROLLER_SOURCE_LINK = 3,
} VkyControllerSource;


typedef enum
{
    VKY_PANEL_STATUS_NONE = 0,
    VKY_PANEL_STATUS_ACTIVE = 1,
    VKY_PANEL_STATUS_LINKED = 2,
    VKY_PANEL_STATUS_RESET = 7,
} VkyPanelStatus;


typedef enum
{
    VKY_AXIS_X = 0,
    VKY_AXIS_Y = 1
} VkyAxis;


typedef enum
{
    VKY_COLORBAR_NONE = 0,
    VKY_COLORBAR_TOP = 1,
    VKY_COLORBAR_RIGHT = 2,
    VKY_COLORBAR_BOTTOM = 3,
    VKY_COLORBAR_LEFT = 4,
} VkyColorbarPosition;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VkyArcball VkyArcball;
typedef struct VkyAxes VkyAxes;
typedef struct VkyAxes2DParams VkyAxes2DParams;
typedef struct VkyAxesDyad VkyAxesDyad;
typedef struct VkyAxesLabel VkyAxesLabel;
typedef struct VkyAxesScale VkyAxesScale;
typedef struct VkyAxesTextData VkyAxesTextData;
typedef struct VkyAxesTextParams VkyAxesTextParams;
typedef struct VkyAxesTextVertex VkyAxesTextVertex;
typedef struct VkyAxesTickParams VkyAxesTickParams;
typedef struct VkyAxesTickVertex VkyAxesTickVertex;
typedef struct VkyAxesUser VkyAxesUser;
typedef struct VkyBox2D VkyBox2D;
typedef struct VkyCamera VkyCamera;
typedef struct VkyColorbarParams VkyColorbarParams;
typedef struct VkyControllerAutorotate VkyControllerAutorotate;
typedef struct VkyData VkyData;
typedef struct VkyGrid VkyGrid;
typedef struct VkyMVP VkyMVP;
typedef struct VkyPanel VkyPanel;
typedef struct VkyPanelIndex VkyPanelIndex;
typedef struct VkyPanelLink VkyPanelLink;
typedef struct VkyPanzoom VkyPanzoom;
typedef struct VkyPick VkyPick;
typedef struct VkyVisual VkyVisual;
typedef struct VkyVisualPanel VkyVisualPanel;
typedef struct VkyVisualProp VkyVisualProp;

// Callbacks.
typedef void (*VkyAxesTickFormatter)(VkyAxes*, double value, char* out_text);
typedef void (*VkyVisualBakeCallback)(VkyVisual* visual);
typedef void (*VkyVisualCallback)(VkyVisual* visual);
typedef void (*VkyVisualPropCallback)(VkyVisualProp*, VkyVisual* visual, VkyPanel* panel);
typedef void (*VkyVisualResizeCallback)(VkyVisual* visual);



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

struct VkyMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float aspect_ratio; // if not 0, the aspect ratio is forced to be 1 in the common.glsl shader
};



struct VkyControllerAutorotate
{
    double speed;
    vec3 axis, eye, center;
    VkyMVP mvp;
};



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

struct VkyVisualPanel
{
    VkyVisual* visual;
    VkyPanel* panel;
    VkyViewportType viewport_type;
    VkyVisualPriority priority;
};



struct VkyScene
{
    VkyCanvas* canvas;
    VkyColor clear_color;

    VkyGrid* grid;

    uint32_t visual_count;
    VkyVisual* visuals;
};



struct VkyData
{
    // User-provided data, in a format specified by the visual.
    uint32_t item_count;
    void* items;

    // Visual data with the vertices and indices.
    uint32_t vertex_count;
    void* vertices;

    uint32_t index_count;
    VkyIndex* indices;

    // Groups are used in visuals such as text, where each text item has multiple glyphs,
    // or paths, where each path has multiple data points.
    uint32_t group_count;
    uint32_t* group_starts;
    uint32_t* group_lengths;
    void* group_params;

    bool need_free_items;
    bool need_free_vertices;
    bool need_free_indices;
};



struct VkyVisualProp
{
    VkyVisualPropType type;
    size_t field_offset;
    size_t field_size;

    void* values; // only used by POS prop, as we need dvec2* array for renormalization

    void* resource; // (only for the _RESOURCE prop types)
    VkyVisualPropCallback callback;

    bool is_set;
    bool need_free;
};



struct VkyVisual
{
    VkyVisualType visual_type;

    VkyScene* scene;

    void* params;
    VkyUniformBuffer params_buffer;

    VkyGraphicsPipeline pipeline;

    VkyData data;

    uint32_t indirect_item_count; // Number of vertices or indices to draw with indirect rendering.
    VkyBufferRegion indirect_buffer;
    VkyBufferRegion vertex_buffer;
    VkyBufferRegion index_buffer;

    size_t item_size;    // size in bytes of the prop struct (corresponds to VkyData.items)
    uint32_t prop_count; // number of props/fields in the struct
    VkyVisualProp* props;
    uint32_t pos_prop_count;
    size_t group_param_size; // size in bytes of the group parameters

    uint32_t resource_count;
    void** resources;

    uint32_t children_count;
    VkyVisual** children;
    VkyVisual* parent;

    VkyVisualBakeCallback cb_bake_data;
    VkyVisualResizeCallback cb_resize; // NOTE: unused yet

    bool need_data_upload;
    bool need_pos_rescale;
};



struct VkyGrid
{
    uint32_t panel_count, row_count, col_count;
    float *xs, *ys, *widths, *heights;
    VkyPanel* panels;

    // Keeps track of where each panel each visual should be shown.
    uint32_t visual_panel_count;
    VkyVisualPanel* visual_panels;

    VkyUniformBuffer dynamic_buffer;
};



struct VkyPanelLink
{
    VkyPanel* p0;
    VkyPanel* p1;
    VkyPanelLinkMode mode;
};



struct VkyPanelIndex
{
    uint32_t row, col;
};



struct VkyPanel
{
    VkyScene* scene;
    VkyControllerType controller_type;
    void* controller;

    cvec4 color_ctx; // 4 bytes for the color context. colormap, color modifier, color constant,
                     // unused

    uint32_t row, col, vspan, hspan;

    VkyViewport viewport; // outer viewport, remove the margins to get the inner viewport
    vec4 margins;

    // indices in the dynamic uniform buffer
    uint32_t inner_uniform_index;
    uint32_t outer_uniform_index;
    float aspect_ratio;

    VkyPanelStatus status;
};



struct VkyPick
{
    VkyPanel* panel; // pointer to the picked panel
    dvec2 pos_canvas_px;
    dvec2 pos_canvas_ndc;
    dvec2 pos_panel;
    dvec2 pos_panzoom;
    dvec2 pos_gpu;
    dvec2 pos_data;
};



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

struct VkyAxesScale
{
    double vmin, vmax;
    bool log_scale;
};



struct VkyAxesDyad
{
    int32_t level;  // default=0, zoom = 2^scale
    int32_t offset; // default=0
};



struct VkyAxesTickVertex
{
    float tick;
    /*
    x  <==>  coord_level <= 3
    level = coord_level % 4

    0   x0
    1   x1
    2   x2
    3   x3
    4   y0
    5   y1
    6   y2
    7   y3
    */
    // uint32_t coord;  // TODO: byte
    // uint32_t level;  // (0=minor, 1=major, 2=grid, 3=lim)
    uint8_t coord_level;
};



struct VkyAxesTextVertex
{
    float tick;
    uint32_t coord;
    vec2 anchor;
    usvec4 glyph; // char, char_index, str_len, str_index
};



struct VkyAxesTextData
{
    float tick;
    uint32_t coord;
    uint32_t string_len;
    char* string;
};



struct VkyAxesTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
    vec2 glyph_size;
    vec4 color;
};



struct VkyAxesTickParams
{
    vec4 margins; // in pixels, top right bottom left
    vec4 linewidths;
    mat4 colors;
    vec4 user_linewidths;
    mat4 user_colors;
    vec2 tick_lengths;
    int cap;
};



struct VkyBox2D
{
    dvec2 pos_ll, pos_ur;
};



struct VkyAxesUser
{
    vec4 linewidths;
    mat4 colors;

    uint32_t tick_count;
    // 0 to 7, 0-3=x, 4-7=y, %4=index in linewidths and colors
    uint8_t levels[VKY_AXES_MAX_USER_TICKS];
    double ticks[VKY_AXES_MAX_USER_TICKS];
    float ticks_ndc[VKY_AXES_MAX_USER_TICKS];
};



struct VkyColorbarParams
{
    VkyColorbarPosition position;
    VkyColormap cmap;
    VkyAxesScale scale;

    // top-left corner of the colorbar
    vec2 pos_tl; // in relative NDC coordinates
    vec2 pad_tl; // in pixels, relative to the closest corner or mid-edge

    // bottom-right corner of the colorbar
    vec2 pos_br; // in relative NDC coordinates
    vec2 pad_br; // in pixels, relative to the closest corner or mid-edge

    float z; // depth
};



struct VkyAxesLabel
{
    VkyAxis axis;
    char label[VKY_AXES_MAX_LABEL_LENGTH];
    // TODO: label text alignment
    float font_size;
    VkyColor color;
};



struct VkyAxes2DParams
{
    VkyAxesScale xscale, yscale;
    VkyAxesTickFormatter xtick_fmt, ytick_fmt;
    vec4 margins; // in pixels: top right bottom left
    VkyAxesLabel xlabel, ylabel;
    // TODO: add VkyAxesTextParams and VkyAxesTickParams
    VkyColorbarParams colorbar;
    VkyAxesUser user;
    bool enable_panzoom;
};



struct VkyAxes
{
    VkyAxesScale xscale_orig, yscale_orig; // corresponds to dyad 0, 0
    VkyAxesScale xscale, yscale;           // (corresponds exactly to initial mvp pan zoom)
    VkyAxesScale xscale_cache, yscale_cache;

    VkyAxesDyad xdyad, ydyad; // 0, 0 at the beginning. RELATIVE to the multiscale
    VkyAxesDyad xdyad_multiscale, ydyad_multiscale; // 0, 0 at the beginning

    VkyAxesTickRange xticks, yticks; // min, step, max, format (decimal/scientific, precision)

    VkyBox2D panzoom_box; // used to make smooth panzoom transitions while recomputing new ticks

    VkyVisual* tick_visual;
    VkyVisual* text_visual;
    VkyPanzoom* panzoom_outer;
    VkyPanzoom* panzoom_inner;

    VkyAxesTickFormatter xtick_fmt, ytick_fmt;

    VkyAxesTickVertex* tick_data;
    VkyAxesTextData* text_data;
    char* str_buffer;

    long unsigned int visibility_flags;

    VkyAxesUser user;
    VkyPanel* panel;
    bool enable_panzoom;
};



/*************************************************************************************************/
/*  Picking                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyPick vky_pick(VkyScene* scene, vec2 canvas_coords, VkyPanel* panel);



/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/

VKY_EXPORT VkyMVP vky_create_mvp(void);
// TODO: VkyMVP first argument
VKY_EXPORT void vky_mvp_set_model(mat4 model, VkyMVP* mvp); // TODO: replace by VkyMVP ?
VKY_EXPORT void vky_mvp_set_view(mat4 view, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_view_3D(vec3 eye, vec3 center, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_proj(mat4 proj, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_proj_3D(VkyPanel* panel, VkyViewportType viewport_type, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_normal_matrix(VkyMVP* mvp, mat4 normal);
VKY_EXPORT void vky_mvp_upload(VkyPanel* panel, VkyViewportType viewport_type, VkyMVP* mvp);

static VkyAxes2DParams vky_default_axes_2D_params()
{
    VkyAxes2DParams params = {
        // Default axes scale.
        VKY_AXES_DEFAULT_SCALE,
        VKY_AXES_DEFAULT_SCALE,
        // Default tick formatters will be set automatically in axes.c
        NULL,
        NULL,
        // Default axis argins
        {
            (float)VKY_AXES_MARGIN_TOP,
            (float)VKY_AXES_MARGIN_RIGHT,
            (float)VKY_AXES_MARGIN_BOTTOM,
            (float)VKY_AXES_MARGIN_LEFT * ((float)VKY_AXES_FONT_SIZE / 8.0f),
        },
        // Labels
        {VKY_AXIS_X, {0}, 0, {{0}}},
        {VKY_AXIS_X, {0}, 0, {{0}}},
        // Default colorbar (the position must be set in order to activate it)
        {
            VKY_COLORBAR_NONE,
            VKY_DEFAULT_COLORMAP,
            {0, 1, false}, // default colormap scale
            {0, 0},
            {0, 0},
            {0, 0},
            {0, 0},
            0,
        },

        // User axes
        {
            {1, 5, 0, 0}, // four possible slots for custom line widths, we only use the first one
            {             // four possible slots for custom line colors
             {.5, .5, .5, 1},
             {0, 0, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            2, // number of ticks
            {0, 4},
            {0, 0},
            {0, 0},
        },
        true,
    };
    return params;
}



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

VKY_EXPORT void vky_begin_commands(VkyScene* scene, uint32_t image_index);
VKY_EXPORT void vky_submit_commands(VkyScene* scene);
VKY_EXPORT void vky_end_commands(VkyScene* Scene);



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_create_visual(VkyScene* scene, VkyVisualType);
VKY_EXPORT void vky_visual_params(VkyVisual* visual, size_t params_size, const void* params);
VKY_EXPORT void vky_add_visual_to_panel(
    VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type, VkyVisualPriority priority);
VKY_EXPORT VkyVisual*
vky_visual(VkyScene* scene, VkyVisualType visual_type, const void* params, const void* obj);

VKY_EXPORT VkyResourceLayout vky_common_resource_layout(VkyVisual* visual);
VKY_EXPORT void vky_set_color_context(VkyPanel* panel, VkyColormap cmap, uint8_t constant);
VKY_EXPORT void vky_allocate_vertex_buffer(VkyVisual* visual, VkDeviceSize size);
VKY_EXPORT void vky_allocate_index_buffer(VkyVisual* visual, VkDeviceSize size);
VKY_EXPORT void vky_add_common_resources(VkyVisual* visual);
VKY_EXPORT void vky_add_uniform_buffer_resource(VkyVisual* visual, VkyUniformBuffer* ubo);
VKY_EXPORT void vky_add_texture_resource(VkyVisual* visual, VkyTexture* texture);
VKY_EXPORT void vky_visual_data_raw(VkyVisual* visual);
VKY_EXPORT void vky_visual_data_raw_old(VkyVisual* visual, VkyData* data);
VKY_EXPORT void vky_draw_visual(VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type);
VKY_EXPORT void vky_draw_all_visuals(VkyScene* scene);
VKY_EXPORT void vky_toggle_visual_visibility(VkyVisual* visual, bool is_visible);
VKY_EXPORT void vky_destroy_visual(VkyVisual* visual);
VKY_EXPORT void vky_free_data(VkyData data);
VKY_EXPORT void vky_visual_add_child(VkyVisual* parent, VkyVisual* child);



/*************************************************************************************************/
/*  Visual props */
/*************************************************************************************************/

VKY_EXPORT void vky_visual_prop_spec(VkyVisual* visual, size_t item_size, size_t group_param_size);

VKY_EXPORT void vky_visual_data_set_size(
    VkyVisual* visual, uint32_t item_count, uint32_t group_count, const uint32_t* group_lengths,
    const void* group_params);

VKY_EXPORT VkyVisualProp*
vky_visual_prop_add(VkyVisual* visual, VkyVisualPropType prop, size_t offset);

VKY_EXPORT VkyVisualProp*
vky_visual_prop_get(VkyVisual* visual, VkyVisualPropType prop, uint32_t prop_index);

VKY_EXPORT void vky_visual_data(
    VkyVisual* visual, VkyVisualPropType prop, uint32_t prop_index, uint32_t value_count,
    const void* values);

VKY_EXPORT void vky_visual_data_partial(
    VkyVisual* visual, VkyVisualPropType prop, uint32_t prop_index, //
    uint32_t first_item, uint32_t item_count,                       //
    uint32_t value_count, const void* values);

VKY_EXPORT void vky_visual_data_group(
    VkyVisual* visual, VkyVisualPropType prop, uint32_t prop_index, uint32_t group_idx, //
    uint32_t value_count, const void* value);

VKY_EXPORT void
vky_visual_data_resource(VkyVisual* visual, VkyVisualPropType prop, uint32_t, void*);

VKY_EXPORT void vky_visual_data_callback(
    VkyVisual* visual, VkyVisualPropType prop, uint32_t prop_index, VkyVisualPropCallback);

void vky_visual_data_upload(VkyVisual* visual, VkyPanel* panel);



/*************************************************************************************************/
/*  Panels */
/*************************************************************************************************/

VKY_EXPORT void vky_set_full_viewport(VkyCanvas* canvas); // TODO: scene
VKY_EXPORT VkyPanel* vky_get_panel(VkyScene* scene, uint32_t row, uint32_t col);
VKY_EXPORT VkyPanelIndex vky_get_panel_index(VkyPanel* panel);
VKY_EXPORT VkyAxes* vky_get_axes(VkyPanel* panel);
VKY_EXPORT VkyViewport vky_get_viewport(VkyPanel* panel, VkyViewportType viewport_type);
VKY_EXPORT void vky_mvp_finalize(VkyScene* scene);
VKY_EXPORT void vky_reset_all_mvp(VkyScene* scene);
VKY_EXPORT uint32_t vky_get_panel_buffer_index(VkyPanel* panel, VkyViewportType viewport_type);
VKY_EXPORT void vky_set_panel_aspect_ratio(VkyPanel* panel, float aspect_ratio);
VKY_EXPORT void vky_set_controller(
    VkyPanel* panel, VkyControllerType controller_type, const void* controller_params);
VKY_EXPORT VkyPanel* vky_panel_from_mouse(VkyScene* scene, vec2 pos);
VKY_EXPORT void vky_link_panels(VkyPanel* panel0, VkyPanel* panel1, VkyPanelLinkMode link);


/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

VKY_EXPORT VkyGrid* vky_create_grid(VkyScene* scene, uint32_t row_count, uint32_t col_count);
VKY_EXPORT VkyScene*
vky_create_scene(VkyCanvas* canvas, VkyColor clear_color, uint32_t row_count, uint32_t col_count);
VKY_EXPORT void vky_clear_color(VkyScene* scene, VkyColor clear_color);
VKY_EXPORT void vky_set_regular_grid(VkyScene* scene, vec4 margins);
VKY_EXPORT void vky_set_grid_widths(VkyScene* scene, const float* widths);
VKY_EXPORT void vky_set_grid_heights(VkyScene* scene, const float* heights);
VKY_EXPORT void vky_set_panel_span(VkyPanel* panel, uint32_t hspan, uint32_t vspan);

VKY_EXPORT void vky_add_panel(
    VkyScene* scene, uint32_t i, uint32_t j, uint32_t vspan, uint32_t hspan, vec4 margins,
    VkyControllerType);
VKY_EXPORT VkyViewport vky_add_viewport_margins(VkyViewport viewport, vec4 margins);
VKY_EXPORT VkyViewport vky_remove_viewport_margins(VkyViewport viewport, vec4 margins);

VKY_EXPORT void vky_destroy_axes(VkyAxes* axes);
VKY_EXPORT void vky_destroy_controller(VkyPanel* panel);



#endif
