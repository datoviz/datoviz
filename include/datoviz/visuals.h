/*************************************************************************************************/
/*  Visual system, interfacing user data to graphics pipelines and GPU objects                   */
/*************************************************************************************************/

#ifndef DVZ_VISUALS_HEADER
#define DVZ_VISUALS_HEADER

#include "array.h"
#include "context.h"
#include "graphics.h"
#include "transforms.h"
#include "vklite.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_GRAPHICS_PER_VISUAL 16
#define DVZ_MAX_COMPUTES_PER_VISUAL 32
#define DVZ_MAX_VISUAL_GROUPS       1024
#define DVZ_MAX_VISUAL_PRIORITY     4
#define DVZ_MAX_UNIFORM_SIZE        65536


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Pipeline types
typedef enum
{
    DVZ_PIPELINE_GRAPHICS,
    DVZ_PIPELINE_COMPUTE,
} DvzPipelineType;



// Prop types.
typedef enum
{
    DVZ_PROP_NONE,
    DVZ_PROP_POS,
    DVZ_PROP_COLOR,
    DVZ_PROP_ALPHA,
    DVZ_PROP_COLORMAP,
    DVZ_PROP_MARKER_SIZE,
    DVZ_PROP_MARKER_TYPE,
    DVZ_PROP_ANGLE,
    DVZ_PROP_TEXT,
    DVZ_PROP_TEXT_SIZE,
    DVZ_PROP_LINE_WIDTH,
    DVZ_PROP_MITER_LIMIT,
    DVZ_PROP_CAP_TYPE,
    DVZ_PROP_JOIN_TYPE,
    DVZ_PROP_TOPOLOGY,
    DVZ_PROP_LENGTH,
    DVZ_PROP_RANGE,
    DVZ_PROP_MARGIN,
    DVZ_PROP_NORMAL,
    DVZ_PROP_TEXCOORDS,
    DVZ_PROP_TEXCOEFS,
    // DVZ_PROP_IMAGE,
    // DVZ_PROP_VOLUME,
    // DVZ_PROP_COLOR_TEXTURE,
    DVZ_PROP_TRANSFER_X,
    DVZ_PROP_TRANSFER_Y,
    DVZ_PROP_LIGHT_POS,
    DVZ_PROP_LIGHT_PARAMS,
    DVZ_PROP_CLIP,
    DVZ_PROP_MODEL,
    DVZ_PROP_VIEW,
    DVZ_PROP_PROJ,
    DVZ_PROP_VIEWPORT,
    DVZ_PROP_TIME,
    DVZ_PROP_INDEX,
    DVZ_PROP_SCALE,
    DVZ_PROP_TRANSFORM,
} DvzPropType;



// Source kinds.
typedef enum
{
    DVZ_SOURCE_KIND_NONE,
    DVZ_SOURCE_KIND_VERTEX = 0x0010,
    DVZ_SOURCE_KIND_INDEX = 0x0020,
    DVZ_SOURCE_KIND_UNIFORM = 0x0030,
    DVZ_SOURCE_KIND_STORAGE = 0x0040,
    DVZ_SOURCE_KIND_TEXTURE_1D = 0x0050,
    DVZ_SOURCE_KIND_TEXTURE_2D = 0x0060,
    DVZ_SOURCE_KIND_TEXTURE_3D = 0x0070,
} DvzSourceKind;



// Source types.
// NOTE: only 1 source type per pipeline is supported
typedef enum
{
    DVZ_SOURCE_TYPE_NONE,
    DVZ_SOURCE_TYPE_MVP,           // 1
    DVZ_SOURCE_TYPE_VIEWPORT,      // 2
    DVZ_SOURCE_TYPE_PARAM,         // 3
    DVZ_SOURCE_TYPE_VERTEX,        // 4
    DVZ_SOURCE_TYPE_INDEX,         // 5
    DVZ_SOURCE_TYPE_IMAGE,         // 6
    DVZ_SOURCE_TYPE_VOLUME,        //
    DVZ_SOURCE_TYPE_TRANSFER,      //
    DVZ_SOURCE_TYPE_COLOR_TEXTURE, //
    DVZ_SOURCE_TYPE_FONT_ATLAS,    //
    DVZ_SOURCE_TYPE_OTHER,         //

    DVZ_SOURCE_TYPE_COUNT,
} DvzSourceType;



// Data source origin.
typedef enum
{
    DVZ_SOURCE_ORIGIN_NONE,   // not set
    DVZ_SOURCE_ORIGIN_LIB,    // the GPU buffer or texture is handled by datoviz's visual module
    DVZ_SOURCE_ORIGIN_USER,   // the GPU buffer or texture is handled by the user
    DVZ_SOURCE_ORIGIN_NOBAKE, // the GPU buffer or texture is handled by the library, but the user
                              // provides the baked data directly
} DvzSourceOrigin;



// Source flags.
typedef enum
{
    DVZ_SOURCE_FLAG_MAPPABLE = 0x0001,
} DvzSourceFlags;



// Visual request.
typedef enum
{
    DVZ_VISUAL_REQUEST_NOT_SET = 0x0000, // object has never been set
    DVZ_VISUAL_REQUEST_SET = 0x0001,     // object has been set
    DVZ_VISUAL_REQUEST_UPLOAD = 0x0002,  // visual requires data GPU upload

    // DVZ_VISUAL_REQUEST_REFILL = 0x0010,  // visual requires a command buffer refill
    // DVZ_VISUAL_REQUEST_NORMALIZATION = 0x0040, // visual requires CPU data normalization
    // DVZ_VISUAL_REQUEST_VIEWPORT = 0x0080,      // visual requires viewport update
} DvzVisualRequest;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVisual DvzVisual;
typedef struct DvzProp DvzProp;

typedef union DvzSourceUnion DvzSourceUnion;
typedef struct DvzSource DvzSource;

typedef struct DvzVisualFillEvent DvzVisualFillEvent;
typedef struct DvzVisualDataEvent DvzVisualDataEvent;

typedef uint32_t DvzIndex;



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

typedef void (*DvzVisualFillCallback)(DvzVisual* visual, DvzVisualFillEvent ev);
/*
called by the scene event callback in response to a REFILL event
default fill callback: viewport, bind vbuf, ibuf, etc. bind the first graphics only and no
compute...
*/



typedef void (*DvzVisualDataCallback)(DvzVisual* visual, DvzVisualDataEvent ev);
/*
called by the scene event callback in response to a DATA event
baking process
visual data sources, item count, groups ==> bindings, vertex buffer, index buffer
enqueue data transfers
*/



/*************************************************************************************************/
/*  Source structs                                                                               */
/*************************************************************************************************/

union DvzSourceUnion
{
    DvzBufferRegions br;
    DvzTexture* tex;
};



// Within a visual, a source is uniquely identified by (1) its type, (2) the source_idx
struct DvzSource
{
    DvzObject obj;
    DvzVisual* visual;

    // Identifier of the prop
    DvzPipelineType pipeline; // graphics or compute pipeline?
    uint32_t pipeline_idx;    // idx of the pipeline within the graphics or compute pipelines

    uint32_t other_count; // the same source may be shared by multiple pipelines of the same type,
                          // using the same slot_idx
    uint32_t other_idxs[DVZ_MAX_GRAPHICS_PER_VISUAL];

    uint32_t source_idx;
    DvzSourceType source_type; // Type of the source (MVP, viewport, vertex buffer, etc.)
    DvzSourceKind source_kind; // Vertex, index, uniform, storage, or texture
    uint32_t slot_idx;         // Binding slot, or 0 for vertex/index
    int flags;
    DvzArray arr; // array to be uploaded to that source

    DvzSourceOrigin origin; // whether the underlying GPU object is handled by the user or datoviz
    DvzSourceUnion u;
};



struct DvzProp
{
    DvzObject obj;

    DvzPropType prop_type; // prop type
    uint32_t prop_idx;     // index within all props of that type

    DvzSource* source;

    uint32_t field_idx;
    DvzDataType dtype;
    VkDeviceSize offset;
    VkDeviceSize item_size;

    float dpi_scaling; // 1 by default, otherwise may be set to canvas->dpi_scaling

    void* default_value;
    DvzArray arr_orig;    // original data array
    DvzArray arr_trans;   // transformed data array
    DvzArray arr_staging; // optional modification made to the prop by the baking function
    // DvzArray arr_triang; // triangulated data array

    DvzDataType target_dtype; // used for casting during the copy to the vertex array
    DvzArrayCopyType copy_type;
    uint32_t reps; // number of repeats when copying
    // bool is_set; // whether the user has set this prop
};



/*************************************************************************************************/
/*  Visual struct                                                                                */
/*************************************************************************************************/

struct DvzVisual
{
    DvzObject obj;
    DvzCanvas* canvas;
    int flags;
    int priority;
    void* user_data;

    // Graphics.
    uint32_t graphics_count;
    DvzGraphics* graphics[DVZ_MAX_GRAPHICS_PER_VISUAL];

    // Keep track of the previous number of vertices/indices in each graphics pipeline, so that
    // we can automatically detect changes in vetex_count/index_count and trigger a full REFILL
    // in this case.
    uint32_t prev_vertex_count[DVZ_MAX_GRAPHICS_PER_VISUAL];
    uint32_t prev_index_count[DVZ_MAX_GRAPHICS_PER_VISUAL];

    // Computes.
    uint32_t compute_count;
    DvzCompute* computes[DVZ_MAX_COMPUTES_PER_VISUAL];

    // Fill callbacks.
    DvzVisualFillCallback callback_fill;

    // Data callbacks.
    // DvzVisualDataCallback callback_transform;
    DvzVisualDataCallback callback_bake;

    // Sources.
    DvzContainer sources;

    // Props.
    DvzContainer props;

    // User data
    uint32_t group_count;
    uint32_t group_sizes[DVZ_MAX_VISUAL_GROUPS];

    // Viewport.
    DvzInteractAxis interact_axis[DVZ_MAX_GRAPHICS_PER_VISUAL];
    DvzViewportClip clip[DVZ_MAX_GRAPHICS_PER_VISUAL];
    DvzViewport viewport; // usually the visual's panel viewport, but may be customized

    // GPU data
    DvzContainer bindings;
    DvzContainer bindings_comp;
};



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

// passed to visual callback when it needs to refill the command buffers
struct DvzVisualFillEvent
{
    DvzCommands* cmds;
    uint32_t cmd_idx;
    VkClearColorValue clear_color;
    DvzViewport viewport;
    void* user_data;
};



struct DvzVisualDataEvent
{
    DvzViewport viewport;
    DvzDataCoords coords;
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
DVZ_EXPORT DvzVisual dvz_visual(DvzCanvas* canvas);

/**
 * Destroy a visual.
 *
 * This function destroys all GPU objects associated to the visual.
 *
 * @param visual the visual
 */
DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);



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
 * @param source_type the source type
 * @param source_idx the index of the source
 * @param pipeline the pipeline type
 * @param pipeline_idx the index of the pipeline
 * @param slot_idx the binding slot of the GPU object associated to the source
 * @param item_size the size of every element in the source data, in bytes
 * @param flags the source creation flags
 */
DVZ_EXPORT void dvz_visual_source(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, //
    DvzPipelineType pipeline, uint32_t pipeline_idx,                   //
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
DVZ_EXPORT void dvz_visual_source_share(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, uint32_t other_idx);

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
 * @returns the prop object
 */
DVZ_EXPORT DvzProp* dvz_visual_prop(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, DvzDataType dtype,
    DvzSourceType source_type, uint32_t source_idx);

/**
 * Set up a default value for a prop.
 *
 * @param prop the prop
 * @param default_value a pointer to the default value
 */
DVZ_EXPORT void dvz_visual_prop_default(DvzProp* prop, void* default_value);

/**
 * Set up how a prop is copied to its associated source.
 *
 * @param prop the prop
 * @param field_idx the index of the corresponding struct field in the source
 * @param offset the offset within each source item
 * @param copy_type the type of copy
 * @param reps the number of repeats for each copied item
 */
DVZ_EXPORT void dvz_visual_prop_copy(
    DvzProp* prop, uint32_t field_idx, VkDeviceSize offset, //
    DvzArrayCopyType copy_type, uint32_t reps);

/**
 * Set the item size, in bytes, of a custom dtype.
 *
 * @param prop the prop
 * @param item_size the size of each prop item, in bytes
 */
DVZ_EXPORT void dvz_visual_prop_size(DvzProp* prop, VkDeviceSize item_size);

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
DVZ_EXPORT void dvz_visual_prop_cast(
    DvzProp* prop, uint32_t field_idx, VkDeviceSize offset, //
    DvzDataType target_dtype, DvzArrayCopyType copy_type, uint32_t reps);

DVZ_EXPORT void dvz_visual_prop_dpi(DvzProp* prop, float dpi_scaling);

/**
 * Add a graphics pipeline to a visual.
 *
 * @param visual the visual
 * @param graphics the graphics
 */
DVZ_EXPORT void dvz_visual_graphics(DvzVisual* visual, DvzGraphics* graphics);

/**
 * Add a compute pipeline to a visual.
 *
 * @param visual the visual
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_visual_compute(DvzVisual* visual, DvzCompute* compute);



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
DVZ_EXPORT void dvz_visual_group(DvzVisual* visual, uint32_t group_idx, uint32_t size);

/**
 * Set the data for a given visual prop.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @param count the number of elements to upload
 * @param data the data, that should be in the dtype of the prop
 */
DVZ_EXPORT void dvz_visual_data(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data);

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
DVZ_EXPORT void dvz_visual_data_partial(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, //
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
DVZ_EXPORT void dvz_visual_data_append(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data);

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
DVZ_EXPORT void dvz_visual_data_source(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);

/**
 * Set an existing GPU buffer for a visual source.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param br the buffer regions
 */
DVZ_EXPORT void dvz_visual_buffer(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, DvzBufferRegions br);

/**
 * Set an existing GPU texture for a visual source.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @param texture the texture
 */
DVZ_EXPORT void dvz_visual_texture(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, DvzTexture* texture);

/**
 * Set visual flags.
 *
 * @param visual the visual
 * @param flags visual flags
 */
DVZ_EXPORT void dvz_visual_flags(DvzVisual* visual, int flags);

/**
 * Return the number of items in a visual.
 *
 * @param visual the visual
 * @returns the number of items in the POS prop
 */

DVZ_EXPORT uint32_t dvz_visual_item_count(DvzVisual* visual);



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

/**
 * Set a fill callback for a visual
 *
 * Callback function signature: `void(DvzVisual*, DvzVisualFillEvent)`
 *
 * @param visual the visual
 * @param callback the fill callback
 */
DVZ_EXPORT void dvz_visual_fill_callback(DvzVisual* visual, DvzVisualFillCallback callback);

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
DVZ_EXPORT void dvz_visual_fill_event(
    DvzVisual* visual, VkClearColorValue clear_color, DvzCommands* cmds, uint32_t cmd_idx,
    DvzViewport viewport, void* user_data);

/**
 * Begin recording a command buffer and begin the render pass.
 *
 * @param canvas the canvas
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
DVZ_EXPORT void dvz_visual_fill_begin(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx);

/**
 * Stop recording a command buffer and stop the render pass.
 *
 * @param canvas the canvas
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
DVZ_EXPORT void dvz_visual_fill_end(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx);

/**
 * Set the visual bake callback function.
 *
 * Callback function signature: `void(DvzVisual*, DvzVisualDataEvent)`
 *
 * @param visual the visual
 * @param callback the bake callback function
 */
DVZ_EXPORT void dvz_visual_callback_bake(DvzVisual* visual, DvzVisualDataCallback callback);



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

/**
 * Return a source object.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @returns the source
 */
DVZ_EXPORT DvzSource*
dvz_source_get(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx);


/**
 * Return a source array.
 *
 * @param visual the visual
 * @param source_type the source type
 * @param source_idx the source index
 * @returns the array
 */
DVZ_EXPORT DvzArray*
dvz_source_array(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx);


/**
 * Return a prop object.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @returns the array
 */
DVZ_EXPORT DvzProp* dvz_prop_get(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx);


/**
 * Return the array of a prop.
 *
 * @param visual the visual
 * @param prop_type the prop type
 * @param prop_idx the prop index
 * @returns the array
 */
DVZ_EXPORT DvzArray* dvz_prop_array(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx);


/**
 * Return the size of a prop array.
 *
 * @param prop the prop
 * @returns the prop size
 */
DVZ_EXPORT uint32_t dvz_prop_size(DvzProp* prop);


/**
 * Return an item in a prop array.
 *
 * @param prop the prop
 * @param prop_idx the prop idx
 * @returns a pointer to the item
 */
DVZ_EXPORT void* dvz_prop_item(DvzProp* prop, uint32_t prop_idx);



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
DVZ_EXPORT void dvz_visual_update(
    DvzVisual* visual, DvzViewport viewport, DvzDataCoords coords, const void* user_data);



#endif
