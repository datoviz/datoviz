/*************************************************************************************************/
/*  Visual system, interfacing user data to graphics pipelines and GPU objects                   */
/*************************************************************************************************/

#ifndef VKL_VISUALS_HEADER
#define VKL_VISUALS_HEADER

#include "array.h"
#include "context.h"
#include "graphics.h"
#include "transforms.h"
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
    VKL_PROP_ALPHA,
    VKL_PROP_COLORMAP,
    VKL_PROP_MARKER_SIZE,
    VKL_PROP_MARKER_TYPE,
    VKL_PROP_ANGLE,
    VKL_PROP_TEXT,
    VKL_PROP_TEXT_SIZE,
    VKL_PROP_LINE_WIDTH,
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
    VKL_PROP_TRANSFORM,
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



// Visual request.
typedef enum
{
    VKL_VISUAL_REQUEST_NOT_SET = 0x0000,       // object has never been set
    VKL_VISUAL_REQUEST_SET = 0x0001,           // object has been set
    VKL_VISUAL_REQUEST_REFILL = 0x0010,        // visual requires a command buffer refill
    VKL_VISUAL_REQUEST_UPLOAD = 0x0020,        // visual requires data GPU upload
    VKL_VISUAL_REQUEST_NORMALIZATION = 0x0040, // visual requires CPU data normalization
    VKL_VISUAL_REQUEST_VIEWPORT = 0x0080,      // visual requires viewport update
} VklVisualRequest;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVisual VklVisual;
typedef struct VklProp VklProp;

typedef union VklSourceUnion VklSourceUnion;
typedef struct VklSource VklSource;

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

union VklSourceUnion
{
    VklBufferRegions br;
    VklTexture* tex;
};



// Within a visual, a source is uniquely identified by (1) its type, (2) the source_idx
struct VklSource
{
    VklObject obj;
    VklVisual* visual;

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

    VklDataType target_dtype; // used for casting during the copy to the vertex array
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
    // VklVisualDataCallback callback_transform;
    VklVisualDataCallback callback_bake;

    // Sources.
    VklContainer sources;

    // Props.
    VklContainer props;

    // User data
    uint32_t group_count;
    uint32_t group_sizes[VKL_MAX_VISUAL_GROUPS];

    // Viewport.
    VklInteractAxis interact_axis[VKL_MAX_GRAPHICS_PER_VISUAL];
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

/**
 * Create a blank visual.
 *
 * @param canvas the canvas
 * @returns a visual object
 */
VKY_EXPORT VklVisual vkl_visual(VklCanvas* canvas);

/**
 * Destroy a visual.
 *
 * This function destroys all GPU objects associated to the visual.
 *
 * @param visual the visual
 */
VKY_EXPORT void vkl_visual_destroy(VklVisual* visual);



/**
 * Define a new source for a visual.
 *
 * Within a given visual, a source is uniquely determined by its type and index. The index is the
 * index of the source among all sources of the same type within the visual.
 *
 * Within a given visual, a pipeline is uniquely determined by its type and index. The index is the
 * index of the pipeline among all pipelines of the same type within the visual.
 *
 * @param visual the visual
 * @param type the source type
 * @param source_idx the index of the source
 * @param pipeline the pipeline type
 * @param pipeline_idx the index of the pipeline
 * @param slot_idx the binding slot of the GPU object associated to the source
 * @param item_size the size of every element in the source data, in bytes
 * @param flags the source creation flags
 */
VKY_EXPORT void vkl_visual_source(
    VklVisual* visual, VklSourceType type, uint32_t source_idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx,            //
    uint32_t slot_idx, VkDeviceSize item_size, int flags);

/**
 * Set up a source share between two sources of the same type, in a given visual.
 *
 * When two sources are shared, they use the same underlying data buffer.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param other_idx the index of the other source
 */
VKY_EXPORT void vkl_visual_source_share(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, uint32_t other_idx);

/**
 * Define a new prop for a visual.
 *
 * Within a given visual, a prop is uniquely determined by its type and index. The index is the
 * index of the prop among all props of the same type within the visual.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @param dtype the data type of the prop
 * @param source_type the type of the source associated to the prop
 * @param source_idx the index of the source associated to the prop
 */
VKY_EXPORT VklProp* vkl_visual_prop(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, VklDataType dtype,
    VklSourceType source_type, uint32_t source_idx);

/**
 * Set up a default value for a prop.
 *
 * @param prop the prop
 * @param default_value a pointer to the default value
 */
VKY_EXPORT void vkl_visual_prop_default(VklProp* prop, void* default_value);

/**
 * Set up how a prop is copied to its associated source.
 *
 * @param prop the prop
 * @param field_idx the index of the corresponding struct field in the source
 * @param offset the offset within each source item
 * @param copy_type the type of copy
 * @param reps the number of repeats for each copied item
 */
VKY_EXPORT void vkl_visual_prop_copy(
    VklProp* prop, uint32_t field_idx, VkDeviceSize offset, //
    VklArrayCopyType copy_type, uint32_t reps);

/**
 * Set up how a prop should be cast when it is copied to its source.
 *
 * @param prop the prop
 * @param field_idx the index of the corresponding struct field in the source
 * @param offset the offset within each source item
 * @param target_dtype the data type to cast to
 * @param copy_type the type of copy
 * @param reps the number of repeats for each copied item
 */
VKY_EXPORT void vkl_visual_prop_cast(
    VklProp* prop, uint32_t field_idx, VkDeviceSize offset, //
    VklDataType target_dtype, VklArrayCopyType copy_type, uint32_t reps);

/**
 * Add a graphics pipeline to a visual.
 *
 * @param visual the visual
 * @param graphics the graphics
 */
VKY_EXPORT void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics);

/**
 * Add a compute pipeline to a visual.
 *
 * @param visual the visual
 * @param compute the compute pipeline
 */
VKY_EXPORT void vkl_visual_compute(VklVisual* visual, VklCompute* compute);



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

/**
 * Define a new data group within a visual.
 *
 * @param visual the visual
 * @param group_idx the group index
 * @param size the number of elements in the group
 */
VKY_EXPORT void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size);

/**
 * Set the data for a given visual prop.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @param count the number of elements to upload
 * @param data the data, that should be in the dtype of the prop
 */
VKY_EXPORT void vkl_visual_data(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data);

/**
 * Set partial data for a given visual prop.
 *
 * If the specified data has less elements than the number of elements to update, the last element
 * will be repeated as many times as necessary.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @param first_item the first item to write to
 * @param item_count the number of elements to update in the source array
 * @param data_item_count the number of elements to copy from `data`
 * @param data the data, that should be in the dtype of the prop
 */
VKY_EXPORT void vkl_visual_data_partial(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);

/**
 * Append elements to the prop.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @param count the number of elements to append to the prop
 * @param data the data, that should be in the dtype of the prop
 */
VKY_EXPORT void vkl_visual_data_append(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data);

/**
 * Set partial data for a given source.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param first_item the first item to write to
 * @param item_count the number of elements to update in the source array
 * @param data_item_count the number of elements to copy from `data`
 * @param data the data, that should be in the dtype of the source
 */
VKY_EXPORT void vkl_visual_data_source(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);

/**
 * Set an existing GPU buffer for a visual source.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param br the buffer regions
 */
VKY_EXPORT void vkl_visual_buffer(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklBufferRegions br);

/**
 * Set an existing GPU texture for a visual source.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param texture the texture
 */
VKY_EXPORT void vkl_visual_texture(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklTexture* texture);



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

/**
 * Set a fill callback for a visual
 *
 * Callback function signature: `void(VklVisual*, VklVisualFillEvent)`
 *
 * @param visual the visual
 * @param callback the fill callback
 */
VKY_EXPORT void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback);

/**
 * Call the visual fill callback.
 *
 * @param visual the visual
 * @param clear_color the clear color
 * @param cmds the command buffers to update
 * @param cmd_idx the index of the command buffer to update
 * @param viewport the viewport
 * @param user_data arbitrary user data pointer
 */
VKY_EXPORT void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data);

/**
 * Begin recording a command buffer and begin the render pass.
 *
 * @param canvas the canvas
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
VKY_EXPORT void vkl_visual_fill_begin(VklCanvas* canvas, VklCommands* cmds, uint32_t idx);

/**
 * Stop recording a command buffer and stop the render pass.
 *
 * @param canvas the canvas
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
VKY_EXPORT void vkl_visual_fill_end(VklCanvas* canvas, VklCommands* cmds, uint32_t idx);

/**
 * Set the visual bake callback function.
 *
 * Callback function signature: `void(VklVisual*, VklVisualDataEvent)`
 *
 * @param visual the visual
 * @param callback the bake callback function
 */
VKY_EXPORT void vkl_visual_callback_bake(VklVisual* visual, VklVisualDataCallback callback);



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

/**
 * Return a source object.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 */
VKY_EXPORT VklSource*
vkl_source_get(VklVisual* visual, VklSourceType source_type, uint32_t source_idx);

/**
 * Return a prop object.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 */
VKY_EXPORT VklProp* vkl_prop_get(VklVisual* visual, VklPropType prop_type, uint32_t idx);

/**
 * Return the size of a prop array.
 *
 * @param prop the prop
 */
VKY_EXPORT uint32_t vkl_prop_size(VklProp* prop);

/**
 * Return an item in a prop array.
 *
 * @param prop the prop
 * @param idx the prop idx
 */
VKY_EXPORT void* vkl_prop_item(VklProp* prop, uint32_t idx);



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

/**
 * Update all GPU buffers and textures from the visual props and sources.
 *
 * @param visual the visual
 * @param viewport the viewport
 * @param coords the data coordinates and transformation
 * @param user_data arbitrary user data pointer
 */
VKY_EXPORT void vkl_visual_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data);



#endif
