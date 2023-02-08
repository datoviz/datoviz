/*************************************************************************************************/
/*  Common enums for the scene module                                                            */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE_ENUMS
#define DVZ_HEADER_SCENE_ENUMS



/*************************************************************************************************/
/*  Vulkan wrapper enums, avoiding dependency to vulkan.h                                        */
/*  WARNING: they must match exactly the corresponding Vulkan enums.                             */
/*************************************************************************************************/

// Vertex input rate (vertex or instance).
typedef enum
{
    DVZ_VERTEX_INPUT_RATE_VERTEX,
    DVZ_VERTEX_INPUT_RATE_INSTANCE,
} DvzVertexInputRate;



// Primitive topology.
// NOTE: matches VkPrimitiveTopology
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,

} DvzPrimitiveTopology;



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



// VkFormat wrapper.
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8G8B8_UNORM = 23,
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R8G8B8A8_UINT = 41,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
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



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

// Graphics flags.
typedef enum
{
    DVZ_GRAPHICS_FLAGS_DEPTH_TEST = 0x0100,
    DVZ_GRAPHICS_FLAGS_PICK = 0x0200,
} DvzGraphicsFlags;



// // Graphics builtins
// typedef enum
// {
//     DVZ_GRAPHICS_NONE,
//     DVZ_GRAPHICS_POINT,

//     DVZ_GRAPHICS_LINE,
//     DVZ_GRAPHICS_LINE_STRIP,
//     DVZ_GRAPHICS_TRIANGLE,
//     DVZ_GRAPHICS_TRIANGLE_STRIP,
//     DVZ_GRAPHICS_TRIANGLE_FAN,

//     DVZ_GRAPHICS_RASTER,
//     DVZ_GRAPHICS_MARKER,

//     DVZ_GRAPHICS_SEGMENT,
//     DVZ_GRAPHICS_ARROW,
//     DVZ_GRAPHICS_PATH,
//     DVZ_GRAPHICS_TEXT,

//     DVZ_GRAPHICS_IMAGE,
//     DVZ_GRAPHICS_IMAGE_CMAP,

//     DVZ_GRAPHICS_VOLUME_SLICE,
//     DVZ_GRAPHICS_MESH,

//     DVZ_GRAPHICS_FAKE_SPHERE,
//     DVZ_GRAPHICS_VOLUME,

//     DVZ_GRAPHICS_COUNT,
//     DVZ_GRAPHICS_CUSTOM,
// } DvzGraphicsType;



#endif
