/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ENUMS
#define DVZ_HEADER_ENUMS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/


#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BINDINGS        16
#define DVZ_MAX_VERTEX_ATTRS    16
#define DVZ_MAX_VERTEX_BINDINGS 8
#define DVZ_MAX_PARAMS          16



/*************************************************************************************************/
/*  Object                                                                                       */
/*************************************************************************************************/

// Object types.
typedef enum
{
    DVZ_OBJECT_TYPE_UNDEFINED,
    DVZ_OBJECT_TYPE_HOST,
    DVZ_OBJECT_TYPE_GPU,
    DVZ_OBJECT_TYPE_WINDOW,
    DVZ_OBJECT_TYPE_GUI_WINDOW,
    DVZ_OBJECT_TYPE_SWAPCHAIN,
    DVZ_OBJECT_TYPE_CANVAS,
    DVZ_OBJECT_TYPE_BOARD,
    DVZ_OBJECT_TYPE_COMMANDS,
    DVZ_OBJECT_TYPE_BUFFER,
    DVZ_OBJECT_TYPE_DAT,
    DVZ_OBJECT_TYPE_TEX,
    DVZ_OBJECT_TYPE_IMAGES,
    DVZ_OBJECT_TYPE_SAMPLER,
    DVZ_OBJECT_TYPE_BINDINGS,
    DVZ_OBJECT_TYPE_COMPUTE,
    DVZ_OBJECT_TYPE_GRAPHICS,
    DVZ_OBJECT_TYPE_SHADER,
    DVZ_OBJECT_TYPE_PIPE,
    DVZ_OBJECT_TYPE_BARRIER,
    DVZ_OBJECT_TYPE_FENCES,
    DVZ_OBJECT_TYPE_SEMAPHORES,
    DVZ_OBJECT_TYPE_RENDERPASS,
    DVZ_OBJECT_TYPE_FRAMEBUFFER,
    DVZ_OBJECT_TYPE_WORKSPACE,
    DVZ_OBJECT_TYPE_PIPELIB,
    DVZ_OBJECT_TYPE_SUBMIT,
    DVZ_OBJECT_TYPE_SCREENCAST,
    DVZ_OBJECT_TYPE_TIMER,
    DVZ_OBJECT_TYPE_ARRAY,
    DVZ_OBJECT_TYPE_CUSTOM,
} DvzObjectType;



// Object status.
// NOTE: the order is important, status >= CREATED means the object has been created
typedef enum
{
    DVZ_OBJECT_STATUS_NONE,          //
    DVZ_OBJECT_STATUS_ALLOC,         // after allocation
    DVZ_OBJECT_STATUS_DESTROYED,     // after destruction
    DVZ_OBJECT_STATUS_INIT,          // after struct initialization but before Vulkan creation
    DVZ_OBJECT_STATUS_CREATED,       // after proper creation on the GPU
    DVZ_OBJECT_STATUS_NEED_RECREATE, // need to be recreated
    DVZ_OBJECT_STATUS_NEED_UPDATE,   // need to be updated
    DVZ_OBJECT_STATUS_NEED_DESTROY,  // need to be destroyed
    DVZ_OBJECT_STATUS_INACTIVE,      // inactive
    DVZ_OBJECT_STATUS_INVALID,       // invalid
} DvzObjectStatus;



/*************************************************************************************************/
/*  Requests                                                                                     */
/*************************************************************************************************/

// Request action.
typedef enum
{
    DVZ_REQUEST_ACTION_NONE,
    DVZ_REQUEST_ACTION_CREATE,
    DVZ_REQUEST_ACTION_DELETE,
    DVZ_REQUEST_ACTION_RESIZE,
    DVZ_REQUEST_ACTION_UPDATE,
    DVZ_REQUEST_ACTION_BIND,
    DVZ_REQUEST_ACTION_RECORD,
    DVZ_REQUEST_ACTION_UPLOAD,
    DVZ_REQUEST_ACTION_UPFILL,
    DVZ_REQUEST_ACTION_DOWNLOAD,
    DVZ_REQUEST_ACTION_SET,
    DVZ_REQUEST_ACTION_GET,
} DvzRequestAction;



// Request object.
typedef enum
{
    DVZ_REQUEST_OBJECT_NONE,
    DVZ_REQUEST_OBJECT_BOARD = 100,
    DVZ_REQUEST_OBJECT_CANVAS,
    DVZ_REQUEST_OBJECT_DAT,
    DVZ_REQUEST_OBJECT_TEX,
    DVZ_REQUEST_OBJECT_SAMPLER,
    DVZ_REQUEST_OBJECT_COMPUTE,
    DVZ_REQUEST_OBJECT_PRIMITIVE,
    DVZ_REQUEST_OBJECT_DEPTH,
    DVZ_REQUEST_OBJECT_BLEND,
    DVZ_REQUEST_OBJECT_POLYGON,
    DVZ_REQUEST_OBJECT_CULL,
    DVZ_REQUEST_OBJECT_FRONT,
    DVZ_REQUEST_OBJECT_SHADER,
    DVZ_REQUEST_OBJECT_VERTEX,
    DVZ_REQUEST_OBJECT_VERTEX_ATTR,
    DVZ_REQUEST_OBJECT_SLOT,
    DVZ_REQUEST_OBJECT_SPECIALIZATION,
    DVZ_REQUEST_OBJECT_GRAPHICS,
    DVZ_REQUEST_OBJECT_INDEX,
    DVZ_REQUEST_OBJECT_BACKGROUND,

    DVZ_REQUEST_OBJECT_RECORD, // use recorder.h
} DvzRequestObject;



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

// Backend.
typedef enum
{
    DVZ_BACKEND_NONE,
    DVZ_BACKEND_GLFW,
    DVZ_BACKEND_OFFSCREEN,
} DvzBackend;



/*************************************************************************************************/
/*  Vulkan wrapper enums, avoiding dependency to vulkan.h                                        */
/*  WARNING: they must match exactly the corresponding Vulkan enums.                             */
/*************************************************************************************************/

// VkFormat wrapper.
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8_UINT = 13,
    DVZ_FORMAT_R8G8B8_UNORM = 23,
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R8G8B8A8_UINT = 41,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
    DVZ_FORMAT_R32G32_SFLOAT = 103,
    DVZ_FORMAT_R32G32B32_SFLOAT = 106,
    DVZ_FORMAT_R32G32B32A32_SFLOAT = 109,
} DvzFormat;



// VkFilter wrapper.
typedef enum
{
    DVZ_FILTER_NEAREST = 0,
    DVZ_FILTER_LINEAR = 1,
    DVZ_FILTER_CUBIC_IMG = 1000015000,
} DvzFilter;



// VkSamplerAddressMode wrapper.
typedef enum
{
    DVZ_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
} DvzSamplerAddressMode;



// VkVertexInputRate wrapper.
typedef enum
{
    DVZ_VERTEX_INPUT_RATE_VERTEX = 0,
    DVZ_VERTEX_INPUT_RATE_INSTANCE = 1,
} DvzVertexInputRate;



// VkPolygonMode wrapper.
typedef enum
{
    DVZ_POLYGON_MODE_FILL = 0,
    DVZ_POLYGON_MODE_LINE = 1,
    DVZ_POLYGON_MODE_POINT = 2,
} DvzPolygonMode;



// VkFrontFace wrapper.
typedef enum
{
    DVZ_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    DVZ_FRONT_FACE_CLOCKWISE = 1,
} DvzFrontFace;



// VkCullModeFlagBits wrapper.
typedef enum
{
    DVZ_CULL_MODE_NONE = 0,
    DVZ_CULL_MODE_FRONT = 0x00000001,
    DVZ_CULL_MODE_BACK = 0x00000002,
} DvzCullMode;



// VkShaderStageFlagBits wrapper.
typedef enum
{
    DVZ_SHADER_VERTEX = 0x00000001,
    DVZ_SHADER_TESSELLATION_CONTROL = 0x00000002,
    DVZ_SHADER_TESSELLATION_EVALUATION = 0x00000004,
    DVZ_SHADER_GEOMETRY = 0x00000008,
    DVZ_SHADER_FRAGMENT = 0x00000010,
    DVZ_SHADER_COMPUTE = 0x00000020,
} DvzShaderType;



// VkDescriptorType wrapper.
typedef enum
{
    DVZ_DESCRIPTOR_TYPE_SAMPLER = 0,
    DVZ_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    DVZ_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    DVZ_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    DVZ_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
} DvzDescriptorType;



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    // NOTE: by convention in vklite, the first queue MUST support transfers
    DVZ_DEFAULT_QUEUE_TRANSFER,
    DVZ_DEFAULT_QUEUE_COMPUTE,
    DVZ_DEFAULT_QUEUE_RENDER,
    DVZ_DEFAULT_QUEUE_PRESENT,
    DVZ_DEFAULT_QUEUE_COUNT,
} DvzDefaultQueue;



// Dat usage.
// TODO: not implemented yet, going from these flags to DvzDatFlags
typedef enum
{
    DVZ_DAT_USAGE_FREQUENT_NONE,
    DVZ_DAT_USAGE_FREQUENT_UPLOAD = 0x0001,
    DVZ_DAT_USAGE_FREQUENT_DOWNLOAD = 0x0002,
    DVZ_DAT_USAGE_FREQUENT_RESIZE = 0x0004,
} DvzDatUsage;



// Dat flags.
typedef enum
{
    DVZ_DAT_FLAGS_NONE = 0x0000,               // default: shared, with staging, single copy
    DVZ_DAT_FLAGS_STANDALONE = 0x0100,         // (or shared)
    DVZ_DAT_FLAGS_MAPPABLE = 0x0200,           // (or non-mappable = need staging buffer)
    DVZ_DAT_FLAGS_DUP = 0x0400,                // (or single copy)
    DVZ_DAT_FLAGS_KEEP_ON_RESIZE = 0x1000,     // (or loose the data when resizing the buffer)
    DVZ_DAT_FLAGS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzDatFlags;



// Dat upload flags.
typedef enum
{
    DVZ_UPLOAD_FLAGS_NOCOPY = 0x0800, // (avoid data copy/free)

} DvzUploadFlags;



// Tex dims.
typedef enum
{
    DVZ_TEX_NONE,
    DVZ_TEX_1D,
    DVZ_TEX_2D,
    DVZ_TEX_3D,
} DvzTexDims;



// Tex flags.
typedef enum
{
    DVZ_TEX_FLAGS_NONE = 0x0000,               // default
    DVZ_TEX_FLAGS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzTexFlags;



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

// Graphics flags.
typedef enum
{
    DVZ_GRAPHICS_FLAGS_DEPTH_TEST = 0x40000,
    DVZ_GRAPHICS_FLAGS_PICK = 0x80000,
} DvzGraphicsFlags;



// Graphics builtins
typedef enum
{
    DVZ_GRAPHICS_NONE,
    DVZ_GRAPHICS_POINT,
    DVZ_GRAPHICS_TRIANGLE,
    DVZ_GRAPHICS_CUSTOM,
} DvzGraphicsType;



/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

// Renderer flags.
typedef enum
{
    DVZ_RENDERER_FLAGS_NONE = 0x000000,
    DVZ_RENDERER_FLAGS_WHITE_BACKGROUND = 0x100000,
} DvzRendererFlags;



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_SLOT_DAT,
    DVZ_SLOT_TEX,
} DvzSlotType;



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static inline DvzSize _format_size(DvzFormat format)
{
    switch (format)
    {
    case DVZ_FORMAT_R8_UNORM:
    case DVZ_FORMAT_R8_SNORM:
        return 1 * sizeof(uint8_t);
        break;

    case DVZ_FORMAT_R16_UNORM:
    case DVZ_FORMAT_R16_SNORM:
        return 1 * sizeof(uint16_t);
        break;

    case DVZ_FORMAT_R32_UINT:
    case DVZ_FORMAT_R32_SINT:
    case DVZ_FORMAT_R32_SFLOAT:
        return 1 * sizeof(uint32_t);
        break;

    case DVZ_FORMAT_R8G8B8_UNORM:
        return 3 * sizeof(uint8_t);
        break;

    case DVZ_FORMAT_R8G8B8A8_UNORM:
    case DVZ_FORMAT_R8G8B8A8_UINT:
    case DVZ_FORMAT_B8G8R8A8_UNORM:
        return 4 * sizeof(uint8_t);
        break;

    case DVZ_FORMAT_R32G32B32_SFLOAT:
        return 3 * sizeof(float);
        break;

    case DVZ_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * sizeof(float);
        break;

    default:
        break;
    }
    log_error("unknown DvzFormat %d", format);
    return 0;
}



static inline uint32_t _format_components(DvzFormat format)
{
    switch (format)
    {
    case DVZ_FORMAT_R8_UNORM:
    case DVZ_FORMAT_R8_SNORM:
    case DVZ_FORMAT_R16_UNORM:
    case DVZ_FORMAT_R16_SNORM:
    case DVZ_FORMAT_R32_UINT:
    case DVZ_FORMAT_R32_SINT:
    case DVZ_FORMAT_R32_SFLOAT:
        return 1;
        break;
        break;

    case DVZ_FORMAT_R8G8B8_UNORM:
    case DVZ_FORMAT_R32G32B32_SFLOAT:
        return 3;
        break;

    case DVZ_FORMAT_R8G8B8A8_UNORM:
    case DVZ_FORMAT_R8G8B8A8_UINT:
    case DVZ_FORMAT_B8G8R8A8_UNORM:
    case DVZ_FORMAT_R32G32B32A32_SFLOAT:
        return 4;
        break;

    default:
        break;
    }
    log_error("unknown DvzFormat %d", format);
    return 0;
}



#endif
