/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ENUMS
#define DVZ_HEADER_ENUMS



/*************************************************************************************************/
/*  Object                                                                                       */
/*************************************************************************************************/

// Object types.
typedef enum
{
    DVZ_OBJECT_TYPE_UNDEFINED,
    DVZ_OBJECT_TYPE_APP,
    DVZ_OBJECT_TYPE_GPU,
    DVZ_OBJECT_TYPE_WINDOW,
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
    DVZ_OBJECT_TYPE_PIPE,
    DVZ_OBJECT_TYPE_BARRIER,
    DVZ_OBJECT_TYPE_FENCES,
    DVZ_OBJECT_TYPE_SEMAPHORES,
    DVZ_OBJECT_TYPE_RENDERPASS,
    DVZ_OBJECT_TYPE_FRAMEBUFFER,
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
/*  Vklite                                                                                       */
/*************************************************************************************************/

// Buffer type.
// NOTE: the enum index should correspond to the buffer index in the context->buffers container
typedef enum
{
    DVZ_BUFFER_TYPE_UNDEFINED,
    DVZ_BUFFER_TYPE_STAGING, // 1
    DVZ_BUFFER_TYPE_VERTEX,  // 2
    DVZ_BUFFER_TYPE_INDEX,   // 3
    DVZ_BUFFER_TYPE_STORAGE, // 4
    DVZ_BUFFER_TYPE_UNIFORM, // 5
} DvzBufferType;



// Texture axis.
typedef enum
{
    DVZ_SAMPLER_AXIS_U,
    DVZ_SAMPLER_AXIS_V,
    DVZ_SAMPLER_AXIS_W,
} DvzSamplerAxis;



// Filter type.
typedef enum
{
    DVZ_SAMPLER_FILTER_MIN,
    DVZ_SAMPLER_FILTER_MAG,
} DvzSamplerFilter;



// VkFormat wrapper, with the most commonly used formats. This is to avoid a dependency to vulkan.h
// WARNING: it is essential that the values match those of vulkan.h!
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8G8B8_UNORM = 23,
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
} DvzFormat;



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
// TODO: not implemented yet, going from these flags to DvzDatOptions
typedef enum
{
    DVZ_DAT_USAGE_FREQUENT_NONE,
    DVZ_DAT_USAGE_FREQUENT_UPLOAD = 0x0001,
    DVZ_DAT_USAGE_FREQUENT_DOWNLOAD = 0x0002,
    DVZ_DAT_USAGE_FREQUENT_RESIZE = 0x0004,
} DvzDatUsage;



// Dat options.
typedef enum
{
    DVZ_DAT_OPTIONS_NONE = 0x0000,               // default: shared, with staging, single copy
    DVZ_DAT_OPTIONS_STANDALONE = 0x0100,         // (or shared)
    DVZ_DAT_OPTIONS_MAPPABLE = 0x0200,           // (or non-mappable = need staging buffer)
    DVZ_DAT_OPTIONS_DUP = 0x0400,                // (or single copy)
    DVZ_DAT_OPTIONS_KEEP_ON_RESIZE = 0x1000,     // (or loose the data when resizing the buffer)
    DVZ_DAT_OPTIONS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzDatOptions;



// Tex dims.
typedef enum
{
    DVZ_TEX_NONE,
    DVZ_TEX_1D,
    DVZ_TEX_2D,
    DVZ_TEX_3D,
} DvzTexDims;



// Tex options.
typedef enum
{
    DVZ_TEX_OPTIONS_NONE = 0x0000,               // default
    DVZ_TEX_OPTIONS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzTexOptions;



#endif
