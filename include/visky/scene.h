#ifndef VKY_SCENE_HEADER
#define VKY_SCENE_HEADER

#include "app.h"
#include "colormaps.h"
#include "constants.h"
#include "ticks.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef enum
{
    VKY_VIEWPORT_INNER = 0,
    VKY_VIEWPORT_OUTER = 1,
} VkyViewportType;

typedef enum
{
    VKY_VISUAL_PRIORITY_NONE = 0,
    VKY_VISUAL_PRIORITY_FIRST = 1,
    VKY_VISUAL_PRIORITY_LAST = 2,
} VkyVisualPriority;

typedef enum
{
    VKY_VISUAL_UNDEFINED = 0,

    VKY_VISUAL_RECTANGLE = 10,
    VKY_VISUAL_RECTANGLE_AXIS = 11,
    VKY_VISUAL_AREA = 12,

    VKY_VISUAL_MESH = 20,
    VKY_VISUAL_MESH_RAW = 21,

    VKY_VISUAL_MARKER = 30,
    VKY_VISUAL_MARKER_RAW = 31,
    VKY_VISUAL_SEGMENT = 32,
    VKY_VISUAL_ARROW = 33,

    VKY_VISUAL_PATH = 40,
    VKY_VISUAL_PATH_RAW = 41,
    VKY_VISUAL_PATH_RAW_MULTI = 42,

    VKY_VISUAL_FAKE_PATH = 50,
    VKY_VISUAL_FAKE_SPHERE = 51,

    VKY_VISUAL_AXES_TEXT = 60,
    VKY_VISUAL_AXES_TICK = 61,

    VKY_VISUAL_AXES_3D = 70,
    VKY_VISUAL_AXES_3D_TEXT = 71,

    VKY_VISUAL_IMAGE = 80,
    VKY_VISUAL_VOLUME = 81,

    VKY_VISUAL_COLORBAR = 90,
    VKY_VISUAL_TEXT = 100,


} VkyVisualType;

typedef struct VkyAxes VkyAxes;
typedef struct VkyPanel VkyPanel;
typedef struct VkyVisualPanel VkyVisualPanel;
typedef struct VkyData VkyData;
typedef struct VkyGrid VkyGrid;
typedef struct VkyMVP VkyMVP;
typedef struct VkyVertex VkyVertex;
typedef struct VkyTextureVertex VkyTextureVertex;
typedef struct VkyVisual VkyVisual;
typedef struct VkyVisualBundle VkyVisualBundle;
typedef struct VkyPanzoom VkyPanzoom;
typedef struct VkyPick VkyPick;

typedef struct VkyPanzoom VkyPanzoom;
typedef struct VkyArcball VkyArcball;
typedef struct VkyCamera VkyCamera;

typedef struct VkyAxesScale VkyAxesScale;
typedef struct VkyAxesDyad VkyAxesDyad;
typedef struct VkyAxesTickVertex VkyAxesTickVertex;
typedef struct VkyAxesTextVertex VkyAxesTextVertex;
typedef struct VkyAxesTextData VkyAxesTextData;
typedef struct VkyAxesTextParams VkyAxesTextParams;
typedef struct VkyAxes VkyAxes;
typedef struct VkyAxesTickParams VkyAxesTickParams;
typedef struct VkyAxesBox VkyAxesBox;
typedef struct VkyAxesUser VkyAxesUser;
typedef struct VkyAxesLabel VkyAxesLabel;

typedef struct VkyColorbarParams VkyColorbarParams;

typedef struct VkyControllerAxes2D VkyControllerAxes2D;
typedef struct VkyAxes2DParams VkyAxes2DParams;
typedef struct VkyControllerAutorotate VkyControllerAutorotate;

// Callbacks.
// typedef VkyScene* (*VkyInitCallback)(VkyCanvas*);
// typedef void (*VkyCallback)(VkyScene*);
typedef void (*VkyVisualCallback)(VkyVisual*);
typedef VkyData (*VkyVisualBakeCallback)(VkyVisual*, VkyData);
typedef VkyData (*VkyVisualBundleBakeCallback)(VkyVisualBundle*, VkyData);
typedef void (*VkyVisualResizeCallback)(VkyVisual*);

typedef void (*VkyAxesTickFormatter)(VkyAxes*, double value, char* out_text);


/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

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



struct VkyMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float aspect_ratio; // if not 0, the aspect ratio is forced to be 1 in the common.glsl shader
};



struct VkyControllerAxes2D
{
    VkyAxes* axes; // comes with its own panzoom
    VkyPanzoom* panzoom;
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
    uint32_t row, col;
    VkyViewportType viewport_type;
    VkyVisualPriority priority;
};



struct VkyScene
{
    VkyCanvas* canvas;
    VkyColorBytes clear_color;

    VkyGrid* grid;

    uint32_t visual_count;
    VkyVisual* visuals;

    uint32_t visual_bundle_count;
    VkyVisualBundle* visual_bundles;
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

    bool no_vertices_alloc;
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

    void** resources;
    uint16_t resource_count;

    VkyVisualBakeCallback cb_bake_data;
    VkyVisualResizeCallback cb_resize; // NOTE: unused yet
};



struct VkyVisualBundle
{
    VkyScene* scene;
    void* params;
    VkyData data;
    uint32_t visual_count;
    VkyVisual** visuals; // array of pointers to VkyVisual

    VkyVisualBundleBakeCallback cb_bake_data;
};



// Default vertex with vec3 pos and usvec4 color.
struct VkyVertex
{
    vec3 pos;
    VkyColorBytes color; // 4 bytes for RGBA
};



struct VkyTextureVertex
{
    vec3 pos;
    vec2 uv;
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
};



struct VkyPick
{
    VkyPanel* panel; // pointer to the picked panel
    dvec2 pos_canvas_px, pos_canvas_ndc, pos_panel, pos_panzoom, pos_gpu, pos_data;
};



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

typedef enum
{
    VKY_AXIS_X = 0,
    VKY_AXIS_Y = 1
} VkyAxis;



typedef enum
{
    VKY_COLORBAR_NONE = 0,
    VKY_COLORBAR_RIGHT,
    VKY_COLORBAR_BOTTOM,
    VKY_COLORBAR_LEFT,
    VKY_COLORBAR_TOP,
} VkyColorbarPosition;



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



struct VkyAxesBox
{
    double xmin, xmax;
    double ymin, ymax;
};



struct VkyAxesUser
{
    vec4 linewidths;
    mat4 colors;

    uint32_t tick_count;
    uint8_t
        levels[VKY_AXES_MAX_USER_TICKS]; // 0 to 7, 0-3=x, 4-7=y, %4=index in linewidths and colors
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
    VkyColorBytes color;
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
};



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
        {VKY_AXIS_X, {0}, 0, {0}},
        {VKY_AXIS_X, {0}, 0, {0}},
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
    };
    return params;
}



struct VkyAxes
{
    VkyAxesScale xscale_orig, yscale_orig; // corresponds to dyad 0, 0
    VkyAxesScale xscale, yscale;           // (corresponds exactly to initial mvp pan zoom)
    VkyAxesScale xscale_cache, yscale_cache;

    VkyAxesDyad xdyad, ydyad; // 0, 0 at the beginning. RELATIVE to the multiscale
    VkyAxesDyad xdyad_multiscale, ydyad_multiscale; // 0, 0 at the beginning

    VkyAxesTickRange xticks, yticks; // min, step, max, format (decimal/scientific, precision)

    VkyAxesBox panzoom_box; // used to make smooth panzoom transitions while recomputing new ticks

    VkyVisual* tick_visual;
    VkyVisual* text_visual;
    VkyPanzoom* panzoom;

    VkyAxesTickFormatter xtick_fmt, ytick_fmt;

    VkyAxesTickVertex* tick_data;
    VkyAxesTextData* text_data;
    char* str_buffer;

    VkyAxesUser user;
    VkyPanel* panel;
};



/*************************************************************************************************/
/*  Picking                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyPick vky_pick(VkyScene* scene, vec2 canvas_coords);



/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/

VKY_EXPORT VkyMVP vky_create_mvp(void);
// TODO: VkyMVP first argument
VKY_EXPORT void vky_mvp_set_model(mat4 model, VkyMVP* mvp); // TODO: replace by VkyMVP ?
VKY_EXPORT void vky_mvp_set_view(mat4 view, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_view_3D(vec3 eye, vec3 center, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_proj(mat4 proj, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_set_proj_3D(VkyPanel*, VkyViewportType, VkyMVP* mvp);
VKY_EXPORT void vky_mvp_normal_matrix(VkyMVP* mvp, mat4 normal);
VKY_EXPORT void vky_mvp_upload(VkyPanel*, VkyViewportType viewport_type, VkyMVP* mvp);



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
VKY_EXPORT void vky_add_visual_to_panel(VkyVisual*, VkyPanel*, VkyViewportType, VkyVisualPriority);
VKY_EXPORT VkyVisual*
vky_visual(VkyScene* scene, VkyVisualType visual_type, const void* params, const void* obj);

VKY_EXPORT VkyResourceLayout vky_common_resource_layout(VkyVisual*);
VKY_EXPORT void
vky_set_color_context(VkyPanel* panel, VkyColormap cmap, VkyColorMod modifier, uint8_t constant);
VKY_EXPORT void vky_allocate_vertex_buffer(VkyVisual* visual, VkDeviceSize size);
VKY_EXPORT void vky_allocate_index_buffer(VkyVisual* visual, VkDeviceSize size);
VKY_EXPORT void vky_add_common_resources(VkyVisual*);
VKY_EXPORT void vky_add_uniform_buffer_resource(VkyVisual* visual, VkyUniformBuffer* ubo);
VKY_EXPORT void vky_add_texture_resource(VkyVisual* visual, VkyTexture* texture);
VKY_EXPORT void vky_visual_upload(VkyVisual* visual, VkyData data);
VKY_EXPORT void vky_visual_upload_partial(VkyVisual* visual, uint32_t item_offset, VkyData data);
VKY_EXPORT void vky_draw_visual(VkyVisual* visual, VkyPanel*, VkyViewportType viewport_type);
VKY_EXPORT void vky_draw_all_visuals(VkyScene* scene);
VKY_EXPORT void vky_toggle_visual_visibility(VkyVisual* visual, bool is_visible);
VKY_EXPORT void vky_destroy_visual(VkyVisual* visual);
VKY_EXPORT void vky_free_data(VkyData data);



/*************************************************************************************************/
/*  Visual bundles                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyVisualBundle* vky_create_visual_bundle(VkyScene* scene);
VKY_EXPORT void vky_add_visual_to_bundle(VkyVisualBundle* vb, VkyVisual* visual);
VKY_EXPORT void vky_add_visual_bundle_to_panel(
    VkyVisualBundle* visual_bundle, VkyPanel* panel, VkyViewportType viewport_type,
    VkyVisualPriority priority);
VKY_EXPORT void vky_destroy_visual_bundle(VkyVisualBundle* visual_bundle);



/*************************************************************************************************/
/*  Panels                                                                                       */
/*************************************************************************************************/

VKY_EXPORT void vky_set_full_viewport(VkyCanvas* canvas); // TODO: scene
VKY_EXPORT VkyPanel* vky_get_panel(VkyScene* scene, uint32_t row, uint32_t col);
VKY_EXPORT void* vky_get_axes(VkyPanel* panel);
VKY_EXPORT VkyViewport vky_get_viewport(VkyPanel*, VkyViewportType viewport_type);
VKY_EXPORT void vky_mvp_finalize(VkyScene* scene);
VKY_EXPORT void vky_reset_all_mvp(VkyScene* scene);
VKY_EXPORT uint32_t vky_get_panel_buffer_index(VkyPanel* panel, VkyViewportType viewport_type);
VKY_EXPORT void vky_set_panel_aspect_ratio(VkyPanel*, float aspect_ratio);
VKY_EXPORT void
vky_set_controller(VkyPanel* panel, VkyControllerType controller_type, const void*);
VKY_EXPORT VkyPanel* vky_panel_from_mouse(VkyScene* scene, vec2 pos);



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

VKY_EXPORT VkyGrid* vky_create_grid(VkyScene* scene, uint32_t row_count, uint32_t col_count);
VKY_EXPORT VkyScene* vky_create_scene(
    VkyCanvas* canvas, VkyColorBytes clear_color, uint32_t row_count, uint32_t col_count);
VKY_EXPORT void vky_clear_color(VkyScene* scene, VkyColorBytes clear_color);
VKY_EXPORT void vky_set_regular_grid(VkyScene* scene, vec4 margins);
VKY_EXPORT void vky_set_grid_widths(VkyScene* scene, const float* widths);
VKY_EXPORT void vky_set_grid_heights(VkyScene* scene, const float* heights);

VKY_EXPORT void vky_add_panel(
    VkyScene* scene, uint32_t i, uint32_t j, uint32_t vspan, uint32_t hspan, vec4 margins,
    VkyControllerType);
VKY_EXPORT VkyViewport vky_add_viewport_margins(VkyViewport viewport, vec4 margins);
VKY_EXPORT VkyViewport vky_remove_viewport_margins(VkyViewport viewport, vec4 margins);

VKY_EXPORT void vky_destroy_axes(VkyAxes* axes);
VKY_EXPORT void vky_destroy_controller(VkyPanel* panel);



#endif
