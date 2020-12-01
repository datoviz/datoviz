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
#define VKL_MAX_VISUAL_GROUPS       1024
#define VKL_MAX_VISUAL_SOURCES      256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_PROP_NONE,
    VKL_PROP_POS,
    VKL_PROP_COLOR,
    VKL_PROP_TYPE,
} VklPropType;


typedef enum
{
    VKL_DTYPE_NONE,

    VKL_DTYPE_FLOAT, // 32 bits
    VKL_DTYPE_VEC2,
    VKL_DTYPE_VEC3,
    VKL_DTYPE_VEC4,

    VKL_DTYPE_DOUBLE, // 64 bits
    VKL_DTYPE_DVEC2,
    VKL_DTYPE_DVEC3,
    VKL_DTYPE_DVEC4,

    VKL_DTYPE_UINT, // 32 bits, unsigned
    VKL_DTYPE_UVEC2,
    VKL_DTYPE_UVEC3,
    VKL_DTYPE_UVEC4,

    VKL_DTYPE_CHAR, // 8 bits, unsigned
    VKL_DTYPE_CVEC2,
    VKL_DTYPE_CVEC3,
    VKL_DTYPE_CVEC4,

    VKL_DTYPE_INT, // 32 bits, signed
    VKL_DTYPE_IVEC2,
    VKL_DTYPE_IVEC3,
    VKL_DTYPE_IVEC4,
} VklDataType;


typedef enum
{
    VKL_PROP_LOC_NONE,

    VKL_PROP_LOC_ATTRIBUTE,
    VKL_PROP_LOC_UNIFORM,
    VKL_PROP_LOC_STORAGE,
    VKL_PROP_LOC_SAMPLER,
    VKL_PROP_LOC_PUSH,
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
    VKL_VISUAL_VARIANT_NONE,
    VKL_VISUAL_VARIANT_RAW,
    VKL_VISUAL_VARIANT_AGG,
    VKL_VISUAL_VARIANT_TEXTURED,
    VKL_VISUAL_VARIANT_TEXTURED_MULTI,
    VKL_VISUAL_VARIANT_SHADED,
} VklVisualVariant;


typedef enum
{
    VKL_VISUAL_NONE,

    VKL_VISUAL_SCATTER, // raw, agg
    VKL_VISUAL_SEGMENT, // raw, agg
    VKL_VISUAL_ARROW,
    VKL_VISUAL_PATH, // raw, agg
    VKL_VISUAL_TEXT, // raw, agg
    VKL_VISUAL_TRIANGLE,
    VKL_VISUAL_RECTANGLE,
    VKL_VISUAL_IMAGE, // single, multi
    VKL_VISUAL_DISC,
    VKL_VISUAL_SECTOR,
    VKL_VISUAL_MESH, // raw, textured, textured_multi, shaded
    VKL_VISUAL_POLYGON,
    VKL_VISUAL_PSLG,
    VKL_VISUAL_HISTOGRAM,
    VKL_VISUAL_AREA,
    VKL_VISUAL_CANDLE,
    VKL_VISUAL_GRAPH,
    VKL_VISUAL_SURFACE,
    VKL_VISUAL_VOLUME,
    VKL_VISUAL_FAKE_SPHERE,
} VklVisualBuiltin;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVisual VklVisual;
typedef struct VklSource VklSource;

typedef struct VklVisualDataArray VklVisualDataArray;
typedef struct VklVisualDataBuffer VklVisualDataBuffer;
typedef struct VklVisualDataTexture VklVisualDataTexture;
typedef union VklVisualData VklVisualData;

typedef struct VklVisualFillEvent VklVisualFillEvent;
typedef struct VklVisualDataEvent VklVisualDataEvent;



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

struct VklVisualDataArray
{
    VkDeviceSize size;
    const void* data;
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

    // Visual characteristics of the prop
    VklDataType dtype;
    VklPropLoc loc;
    uint32_t binding_idx;
    uint32_t field_idx;
    VkDeviceSize offset;

    // Specified by the user
    VklPropBinding binding; // initially, NONE, filled when the user specifies the visual's data
    VklVisualData u;
};



/*************************************************************************************************/
/*  Visual struct                                                                                */
/*************************************************************************************************/

struct VklVisual
{
    VklCanvas* canvas;

    uint32_t graphics_count;
    VklGraphics* graphics[VKL_MAX_GRAPHICS_PER_VISUAL];

    uint32_t compute_count;
    VklCompute* computes[VKL_MAX_COMPUTES_PER_VISUAL];

    VklVisualFillCallback fill_callback;
    VklVisualDataCallback data_callback;

    // User data
    uint32_t item_count;
    uint32_t group_count;
    uint32_t group_sizes[VKL_MAX_VISUAL_GROUPS];
    uint32_t source_count;
    VklSource sources[VKL_MAX_VISUAL_SOURCES];

    // GPU data
    uint32_t vertex_count, index_count;
    VklBufferRegions vertex_buf;
    VklBufferRegions index_buf;
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
    uint32_t idx;
    VkClearColorValue clear_color;
    VklViewport viewport;
    void* user_data;
};



struct VklVisualDataEvent
{ // passed to visual callback when it needs to update its data
    VklViewport viewport;
    // const void* data; // only used with CPU prop binding
    const void* user_data;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklVisual vkl_visual(VklCanvas* canvas);

VKY_EXPORT void vkl_visual_create(VklVisual* visual);

VKY_EXPORT void vkl_visual_destroy(VklVisual* visual);

// Builtin visuals
VKY_EXPORT void vkl_visual_builtin(VklVisual* visual, VklVisualBuiltin builtin);

VKY_EXPORT void vkl_visual_variant(VklVisual* visual, VklVisualVariant variant);

VKY_EXPORT void vkl_visual_transform(VklVisual* visual, VklTransformAxis transform_axis);

// Custom visuals
VKY_EXPORT void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset);

VKY_EXPORT void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics);

VKY_EXPORT void vkl_visual_compute(VklVisual* visual, VklCompute* compute);

VKY_EXPORT void vkl_visual_bake(VklVisual* visual, VklVisualDataCallback callback);

VKY_EXPORT void vkl_visual_fill(VklVisual* visual, VklVisualFillCallback callback);

VKY_EXPORT void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count);

VKY_EXPORT void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size);

VKY_EXPORT void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t idx, VkDeviceSize size, const void* data);

VKY_EXPORT void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, VklBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size);

VKY_EXPORT void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, VklTexture* texture, uvec2 offset,
    uvec2 shape);



#endif
