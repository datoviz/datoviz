#ifndef VKL_VISUALS_HEADER
#define VKL_VISUALS_HEADER

#include "context.h"
#include "graphics.h"
#include "vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_GRAPHICS_PER_VISUAL 256
#define VKL_MAX_COMPUTES_PER_VISUAL 256
#define VKL_MAX_VISUAL_GROUPS       16384
#define VKL_MAX_VISUAL_SOURCES      256
#define VKL_MAX_VISUAL_RESOURCES    256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_PROP_NONE,

    VKL_PROP_VERTEX,
    VKL_PROP_UNIFORM,
    VKL_PROP_TEXTURE,

    VKL_PROP_POS,
    VKL_PROP_COLOR,
    VKL_PROP_TYPE,
} VklPropType;


typedef enum
{
    VKL_DTYPE_NONE,

    VKL_DTYPE_CHAR, // 8 bits, unsigned
    VKL_DTYPE_CVEC2,
    VKL_DTYPE_CVEC3,
    VKL_DTYPE_CVEC4,

    VKL_DTYPE_UINT, // 32 bits, unsigned
    VKL_DTYPE_UVEC2,
    VKL_DTYPE_UVEC3,
    VKL_DTYPE_UVEC4,

    VKL_DTYPE_INT, // 32 bits, signed
    VKL_DTYPE_IVEC2,
    VKL_DTYPE_IVEC3,
    VKL_DTYPE_IVEC4,

    VKL_DTYPE_FLOAT, // 32 bits
    VKL_DTYPE_VEC2,
    VKL_DTYPE_VEC3,
    VKL_DTYPE_VEC4,

    VKL_DTYPE_DOUBLE, // 64 bits
    VKL_DTYPE_DVEC2,
    VKL_DTYPE_DVEC3,
    VKL_DTYPE_DVEC4,
} VklDataType;


typedef enum
{
    VKL_PROP_LOC_NONE,
    VKL_PROP_LOC_VERTEX_ATTR,  // only compatible with CPU binding
    VKL_PROP_LOC_VERTEX,       // only compatible with BUFFER binding
    VKL_PROP_LOC_INDEX,        // only compatible with CPU and BUFFER binding
    VKL_PROP_LOC_UNIFORM,      // only compatible with BUFFER binding
    VKL_PROP_LOC_UNIFORM_ATTR, // only compatible with CPU binding
    VKL_PROP_LOC_STORAGE,      // only compatible with BUFFER binding
    VKL_PROP_LOC_SAMPLER,      // only compatible with CPU and TEXTURE binding
    VKL_PROP_LOC_PUSH,         // only compatible with CPU binding
} VklPropLoc;


typedef enum
{
    VKL_PROP_BINDING_NONE,
    VKL_PROP_BINDING_CPU,
    VKL_PROP_BINDING_BUFFER,  // only compatible with prop loc attribute, uniform, storage
    VKL_PROP_BINDING_TEXTURE, // only compatible with prop loc sampler
} VklPropBinding;


typedef enum
{
    VKL_PIPELINE_GRAPHICS,
    VKL_PIPELINE_COMPUTE,
} VklPipelineType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVisual VklVisual;
typedef struct VklSource VklSource;
typedef struct VklDataCoords VklDataCoords;

typedef struct VklVisualDataArray VklVisualDataArray;
typedef struct VklVisualDataBuffer VklVisualDataBuffer;
typedef struct VklVisualDataTexture VklVisualDataTexture;
typedef union VklVisualData VklVisualData;

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



struct VklVisualDataArray
{
    VkDeviceSize offset;
    VkDeviceSize size;

    void* data_original;
    void* data_transformed;
    void* data_triangulated;
};

struct VklVisualDataBuffer
{
    VklBufferRegions br;
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct VklVisualDataTexture
{
    VklTexture* texture;
    uvec2 offset;
    uvec2 shape;
};

union VklVisualData
{
    VklVisualDataArray a;
    VklVisualDataBuffer b;
    VklVisualDataTexture t;
};



struct VklSource
{ // Identifier of the prop
    VklPropType prop;
    uint32_t prop_idx;

    // Characteristics of the prop
    VklPipelineType pipeline_type; // graphics or compute pipeline?
    uint32_t pipeline_idx;         // idx of the pipeline within the graphics or compute pipelines
    VklDataType dtype;             // data type of the prop
    VklPropLoc loc;                // prop location
    uint32_t binding_idx;          // binding idx
    uint32_t field_idx;            // field index within the binding (ATTR and uniform struct)
    VkDeviceSize offset;           // used by vertex attributes
    VkDeviceSize dtype_size;       // rename into size

    // Specified by the user
    VklPropBinding binding; // initially, NONE, filled when the user specifies the visual's data
    VklVisualData u;
};



/*************************************************************************************************/
/*  Visual struct                                                                                */
/*************************************************************************************************/

struct VklVisual
{
    VklObject obj;
    VklCanvas* canvas;

    uint32_t graphics_count;
    VklGraphics* graphics[VKL_MAX_GRAPHICS_PER_VISUAL];

    uint32_t compute_count;
    VklCompute* computes[VKL_MAX_COMPUTES_PER_VISUAL];

    VklVisualFillCallback fill_callback;

    // Data callbacks.
    VklVisualDataCallback transform_callback;
    VklVisualDataCallback triangulation_callback;
    VklVisualDataCallback bake_callback;

    // User data
    uint32_t item_count;
    uint32_t item_count_triangulated; // set by the triangulation callback
    uint32_t group_count;
    uint32_t group_sizes[VKL_MAX_VISUAL_GROUPS];
    uint32_t source_count;
    VklSource sources[VKL_MAX_VISUAL_SOURCES];

    // GPU data
    uint32_t vertex_count;
    uint32_t index_count;
    VkDeviceSize vertex_size;
    void* vertex_data;
    void* index_data;

    // GPU objects
    VklBufferRegions vertex_buf;
    VklBufferRegions index_buf;
    VklBufferRegions buffers[VKL_MAX_VISUAL_RESOURCES];
    VklTexture* textures[VKL_MAX_VISUAL_RESOURCES];
    VklBindings gbindings[VKL_MAX_GRAPHICS_PER_VISUAL];
    VklBindings cbindings[VKL_MAX_GRAPHICS_PER_VISUAL];
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

VKY_EXPORT void vkl_visual_vertex(VklVisual* visual, VkDeviceSize vertex_size);

VKY_EXPORT void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset);

VKY_EXPORT void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics);

VKY_EXPORT void vkl_visual_compute(VklVisual* visual, VklCompute* compute);



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count);

VKY_EXPORT void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size);

VKY_EXPORT void
vkl_visual_data(VklVisual* visual, VklPropType type, uint32_t idx, const void* data);

VKY_EXPORT void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t first_item, uint32_t item_count,
    const void* data);

VKY_EXPORT void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size);

VKY_EXPORT void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklTexture* texture, uvec3 offset, uvec3 shape);



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback);

VKY_EXPORT void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data);



VKY_EXPORT void vkl_visual_transform_callback(VklVisual* visual, VklVisualDataCallback callback);

VKY_EXPORT void
vkl_visual_triangulation_callback(VklVisual* visual, VklVisualDataCallback callback);

VKY_EXPORT void vkl_visual_bake_callback(VklVisual* visual, VklVisualDataCallback callback);



/*************************************************************************************************/
/*  Data update and baking                                                                       */
/*************************************************************************************************/

VKY_EXPORT void vkl_bake_alloc(VklVisual* visual, uint32_t vertex_count, uint32_t index_count);

VKY_EXPORT void vkl_bake_vertex_attr(VklVisual* visual);

VKY_EXPORT void vkl_visual_data_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data);



#endif
