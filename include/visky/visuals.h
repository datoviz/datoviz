#ifndef VKL_VISUALS_HEADER
#define VKL_VISUALS_HEADER

#include "array.h"
#include "context.h"
#include "graphics.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_GRAPHICS_PER_VISUAL 16
#define VKL_MAX_COMPUTES_PER_VISUAL 32
#define VKL_MAX_VISUAL_GROUPS       1024
#define VKL_MAX_VISUAL_PRIORITY     4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Pipeline types
typedef enum
{
    VKL_PIPELINE_GRAPHICS,
    VKL_PIPELINE_COMPUTE,
} VklPipelineType;



// Prop types.
typedef enum
{
    VKL_PROP_NONE,
    VKL_PROP_POS,
    VKL_PROP_COLOR,
    VKL_PROP_COLORMAP,
    VKL_PROP_MARKER_SIZE,
    VKL_PROP_TEXT,
    VKL_PROP_TEXT_SIZE,
    VKL_PROP_LINE_WIDTH,
    VKL_PROP_TYPE,
    VKL_PROP_LENGTH,
    VKL_PROP_MARGIN,
    VKL_PROP_NORMAL,
    VKL_PROP_TEXCOORDS,
    VKL_PROP_TEXCOEFS,
    VKL_PROP_IMAGE,
    VKL_PROP_VOLUME,
    VKL_PROP_COLOR_TEXTURE,
    VKL_PROP_TRANSFER_X,
    VKL_PROP_TRANSFER_Y,
    VKL_PROP_LIGHT_POS,
    VKL_PROP_LIGHT_PARAMS,
    VKL_PROP_CLIP,
    VKL_PROP_VIEW_POS,
    VKL_PROP_MODEL,
    VKL_PROP_VIEW,
    VKL_PROP_PROJ,
    VKL_PROP_TIME,
    VKL_PROP_INDEX,
} VklPropType;



// Source kinds.
typedef enum
{
    VKL_SOURCE_KIND_NONE,
    VKL_SOURCE_KIND_VERTEX = 0x0010,
    VKL_SOURCE_KIND_INDEX = 0x0020,
    VKL_SOURCE_KIND_UNIFORM = 0x0030,
    VKL_SOURCE_KIND_STORAGE = 0x0040,
    VKL_SOURCE_KIND_TEXTURE_1D = 0x0050,
    VKL_SOURCE_KIND_TEXTURE_2D = 0x0060,
    VKL_SOURCE_KIND_TEXTURE_3D = 0x0070,
} VklSourceKind;



// Source types.
// NOTE: only 1 source type per pipeline is supported
typedef enum
{
    VKL_SOURCE_TYPE_NONE,
    VKL_SOURCE_TYPE_MVP,      // 1
    VKL_SOURCE_TYPE_VIEWPORT, // 2
    VKL_SOURCE_TYPE_PARAM,    // 3
    VKL_SOURCE_TYPE_VERTEX,   // 4
    VKL_SOURCE_TYPE_INDEX,    // 5
    VKL_SOURCE_TYPE_IMAGE,    // 6
    VKL_SOURCE_TYPE_VOLUME,
    VKL_SOURCE_TYPE_COLOR_TEXTURE,
    VKL_SOURCE_TYPE_FONT_ATLAS,
    VKL_SOURCE_TYPE_OTHER,

    VKL_SOURCE_TYPE_COUNT,
} VklSourceType;



// Data source origin.
typedef enum
{
    VKL_SOURCE_ORIGIN_NONE,   // not set
    VKL_SOURCE_ORIGIN_LIB,    // the GPU buffer or texture is handled by visky's visual module
    VKL_SOURCE_ORIGIN_USER,   // the GPU buffer or texture is handled by the user
    VKL_SOURCE_ORIGIN_NOBAKE, // the GPU buffer or texture is handled by the library, but the user
                              // provides the baked data directly
} VklSourceOrigin;



// Source flags.
typedef enum
{
    VKL_SOURCE_FLAG_MAPPABLE = 0x0001,
} VklSourceFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVisual VklVisual;
typedef struct VklProp VklProp;

// typedef struct VklSourceBuffer VklSourceBuffer;
// typedef struct VklSourceTexture VklSourceTexture;
typedef union VklSourceUnion VklSourceUnion;
typedef struct VklSource VklSource;
typedef struct VklDataCoords VklDataCoords;

typedef struct VklVisualFillEvent VklVisualFillEvent;
typedef struct VklVisualDataEvent VklVisualDataEvent;

typedef uint32_t VklIndex;



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

typedef void (*VklVisualFillCallback)(VklVisual* visual, VklVisualFillEvent ev);
/*
called by the scene event callback in response to a REFILL event
default fill callback: viewport, bind vbuf, ibuf, etc. bind the first graphics only and no
compute...
*/



typedef void (*VklVisualDataCallback)(VklVisual* visual, VklVisualDataEvent ev);
/*
called by the scene event callback in response to a DATA event
baking process
visual data sources, item count, groups ==> bindings, vertex buffer, index buffer
enqueue data transfers
*/



/*************************************************************************************************/
/*  Source structs                                                                               */
/*************************************************************************************************/

struct VklDataCoords
{
    dvec4 data; // (blx, bly, trx, try)
    vec4 gpu;   // (blx, bly, trx, try)
};



union VklSourceUnion
{
    VklBufferRegions br;
    VklTexture* tex;
};



// Within a visual, a source is uniquely identified by (1) its type, (2) the source_idx
struct VklSource
{
    VklObject obj;

    // Identifier of the prop
    VklPipelineType pipeline; // graphics or compute pipeline?
    uint32_t pipeline_idx;    // idx of the pipeline within the graphics or compute pipelines

    uint32_t other_count; // the same source may be shared by multiple pipelines of the same type,
                          // using the same slot_idx
    uint32_t other_idxs[VKL_MAX_GRAPHICS_PER_VISUAL];

    uint32_t source_idx;
    VklSourceType source_type; // Type of the source (MVP, viewport, vertex buffer, etc.)
    VklSourceKind source_kind; // Vertex, index, uniform, storage, or texture
    uint32_t slot_idx;         // Binding slot, or 0 for vertex/index
    int flags;
    VklArray arr; // array to be uploaded to that source

    VklSourceOrigin origin; // whether the underlying GPU object is handled by the user or visky
    VklSourceUnion u;
};



struct VklProp
{
    VklObject obj;

    VklPropType prop_type; // prop type
    uint32_t prop_idx;     // index within all props of that type

    VklSource* source;

    uint32_t field_idx;
    VklDataType dtype;
    VkDeviceSize offset;

    void* default_value;
    VklArray arr_orig;  // original data array
    VklArray arr_trans; // transformed data array
    // VklArray arr_triang; // triangulated data array

    VklArrayCopyType copy_type;
    uint32_t reps; // number of repeats when copying
    // bool is_set; // whether the user has set this prop
};



/*************************************************************************************************/
/*  Visual struct                                                                                */
/*************************************************************************************************/

struct VklVisual
{
    VklObject obj;
    VklCanvas* canvas;
    int flags;
    int priority;
    void* user_data;

    // Graphics.
    uint32_t graphics_count;
    VklGraphics* graphics[VKL_MAX_GRAPHICS_PER_VISUAL];

    // Keep track of the previous number of vertices/indices in each graphics pipeline, so that
    // we can automatically detect changes in vetex_count/index_count and trigger a full REFILL
    // in this case.
    uint32_t prev_vertex_count[VKL_MAX_GRAPHICS_PER_VISUAL];
    uint32_t prev_index_count[VKL_MAX_GRAPHICS_PER_VISUAL];

    // Computes.
    uint32_t compute_count;
    VklCompute* computes[VKL_MAX_COMPUTES_PER_VISUAL];

    // Fill callbacks.
    VklVisualFillCallback callback_fill;

    // Data callbacks.
    VklVisualDataCallback callback_transform;
    VklVisualDataCallback callback_bake;

    // Sources.
    VklContainer sources;

    // Props.
    VklContainer props;

    // User data
    uint32_t group_count;
    uint32_t group_sizes[VKL_MAX_VISUAL_GROUPS];

    // Viewport.
    VklTransformAxis transform[VKL_MAX_GRAPHICS_PER_VISUAL];
    VklViewportClip clip[VKL_MAX_GRAPHICS_PER_VISUAL];
    VklViewport viewport; // usually the visual's panel viewport, but may be customized

    // GPU data
    VklContainer bindings;
    VklContainer bindings_comp;
};



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

// passed to visual callback when it needs to refill the command buffers
struct VklVisualFillEvent
{
    VklCommands* cmds;
    uint32_t cmd_idx;
    VkClearColorValue clear_color;
    VklViewport viewport;
    void* user_data;
};



struct VklVisualDataEvent
{
    VklViewport viewport;
    VklDataCoords coords;
    const void* user_data;
};



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

VKY_EXPORT VklVisual vkl_visual(VklCanvas* canvas);

VKY_EXPORT void vkl_visual_destroy(VklVisual* visual);



// Define a new source. (source_type, pipeline_idx) completely identifies a source within all
// pipelines
VKY_EXPORT void vkl_visual_source(
    VklVisual* visual, VklSourceType type, uint32_t source_idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx,            //
    uint32_t slot_idx, VkDeviceSize item_size, int flags);

VKY_EXPORT void vkl_visual_source_share(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, uint32_t other_idx);

VKY_EXPORT void vkl_visual_prop(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, VklDataType dtype,
    VklSourceType source_type, uint32_t source_idx);

VKY_EXPORT void vkl_visual_prop_default(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, void* default_value);

VKY_EXPORT void vkl_visual_prop_copy(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, //
    uint32_t field_idx, VkDeviceSize offset,                     //
    VklArrayCopyType copy_type, uint32_t reps);

VKY_EXPORT void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics);

VKY_EXPORT void vkl_visual_compute(VklVisual* visual, VklCompute* compute);



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size);

VKY_EXPORT void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t prop_idx, uint32_t count, const void* data);

VKY_EXPORT void vkl_visual_data_partial(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);

VKY_EXPORT void vkl_visual_data_append(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data);

VKY_EXPORT void vkl_visual_data_source(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);

VKY_EXPORT void vkl_visual_buffer(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklBufferRegions br);

VKY_EXPORT void vkl_visual_texture(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklTexture* texture);



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback);

VKY_EXPORT void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data);

VKY_EXPORT void vkl_visual_fill_begin(VklCanvas* canvas, VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_visual_fill_end(VklCanvas* canvas, VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_visual_callback_transform(VklVisual* visual, VklVisualDataCallback callback);

VKY_EXPORT void vkl_visual_callback_bake(VklVisual* visual, VklVisualDataCallback callback);



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

VKY_EXPORT VklSource*
vkl_source_get(VklVisual* visual, VklSourceType source_type, uint32_t source_idx);

VKY_EXPORT VklProp* vkl_prop_get(VklVisual* visual, VklPropType prop_type, uint32_t idx);

VKY_EXPORT VklArray* vkl_prop_array(VklVisual* visual, VklPropType prop_type, uint32_t idx);

VKY_EXPORT void* vkl_prop_item(VklProp* prop, uint32_t idx);



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data);


#endif
