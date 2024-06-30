/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_ENUMS
#define DVZ_HEADER_PUBLIC_ENUMS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

// NOTE: we duplicate these common types here for simplicity, best would probably be to
// define them in a common file such as datoviz_enums.h, used both by the public API header file
// include/datoviz.h, and by the common internal file _math.h
typedef uint64_t DvzSize;
typedef uint32_t DvzIndex;
typedef uint64_t DvzId;

typedef uint8_t cvec2[2];
typedef uint8_t cvec3[3];
typedef uint8_t cvec4[4];

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];

typedef double dvec2[2];
typedef double dvec3[3];
typedef double dvec4[4];

typedef vec2 mat2[2];
typedef vec3 mat3[3];
typedef vec4 mat4[4];

typedef dvec2 dmat2[2];
typedef dvec3 dmat3[3];
typedef dvec4 dmat4[4];



/*************************************************************************************************/
/*  Vulkan wrapper enums, avoiding dependency to vulkan.h                                        */
/*  WARNING: they must match exactly the corresponding Vulkan enums.                             */
/*************************************************************************************************/

// VkPrimitiveTopology wrapper.
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,

} DvzPrimitiveTopology;



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



/*************************************************************************************************/
/*  Visual-specific enums                                                                        */
/*************************************************************************************************/

// Marker shape.
// NOTE: the numbers need to correspond to graphics_markers.frag.
typedef enum
{
    DVZ_MARKER_SHAPE_DISC = 0,
    DVZ_MARKER_SHAPE_ASTERISK = 1,
    DVZ_MARKER_SHAPE_CHEVRON = 2,
    DVZ_MARKER_SHAPE_CLOVER = 3,
    DVZ_MARKER_SHAPE_CLUB = 4,
    DVZ_MARKER_SHAPE_CROSS = 5,
    DVZ_MARKER_SHAPE_DIAMOND = 6,
    DVZ_MARKER_SHAPE_ARROW = 7,
    DVZ_MARKER_SHAPE_ELLIPSE = 8,
    DVZ_MARKER_SHAPE_HBAR = 9,
    DVZ_MARKER_SHAPE_HEART = 10,
    DVZ_MARKER_SHAPE_INFINITY = 11,
    DVZ_MARKER_SHAPE_PIN = 12,
    DVZ_MARKER_SHAPE_RING = 13,
    DVZ_MARKER_SHAPE_SPADE = 14,
    DVZ_MARKER_SHAPE_SQUARE = 15,
    DVZ_MARKER_SHAPE_TAG = 16,
    DVZ_MARKER_SHAPE_TRIANGLE = 17,
    DVZ_MARKER_SHAPE_VBAR = 18,
    DVZ_MARKER_SHAPE_COUNT,
} DvzMarkerShape;



// Marker mode.
typedef enum
{
    DVZ_MARKER_MODE_NONE = 0,
    DVZ_MARKER_MODE_CODE = 1,
    DVZ_MARKER_MODE_BITMAP = 2,
    DVZ_MARKER_MODE_SDF = 3,
    DVZ_MARKER_MODE_MSDF = 4,
    DVZ_MARKER_MODE_MTSDF = 5,
} DvzMarkerMode;



// Marker aspect.
typedef enum
{
    DVZ_MARKER_ASPECT_FILLED = 0,
    DVZ_MARKER_ASPECT_STROKE = 1,
    DVZ_MARKER_ASPECT_OUTLINE = 2,
} DvzMarkerAspect;



// Cap type.
typedef enum
{
    DVZ_CAP_TYPE_NONE = 0,
    DVZ_CAP_ROUND = 1,
    DVZ_CAP_TRIANGLE_IN = 2,
    DVZ_CAP_TRIANGLE_OUT = 3,
    DVZ_CAP_SQUARE = 4,
    DVZ_CAP_BUTT = 5,
    DVZ_CAP_COUNT,
} DvzCapType;



// Joint type.
typedef enum
{
    DVZ_JOIN_SQUARE = 0,
    DVZ_JOIN_ROUND = 1,
} DvzJoinType;



// Path topology.
typedef enum
{
    DVZ_PATH_OPEN,
    DVZ_PATH_CLOSED,
} DvzPathTopology;



// Shape type.
typedef enum
{
    DVZ_SHAPE_NONE,
    DVZ_SHAPE_SQUARE,
    DVZ_SHAPE_DISC,
    DVZ_SHAPE_CUBE,
    DVZ_SHAPE_SPHERE,
    DVZ_SHAPE_CYLINDER,
    DVZ_SHAPE_CONE,
    DVZ_SHAPE_OBJ,
    DVZ_SHAPE_OTHER,
} DvzShapeType;



typedef enum
{
    DVZ_EASING_NONE,
    DVZ_EASING_IN_SINE,
    DVZ_EASING_OUT_SINE,
    DVZ_EASING_IN_OUT_SINE,
    DVZ_EASING_IN_QUAD,
    DVZ_EASING_OUT_QUAD,
    DVZ_EASING_IN_OUT_QUAD,
    DVZ_EASING_IN_CUBIC,
    DVZ_EASING_OUT_CUBIC,
    DVZ_EASING_IN_OUT_CUBIC,
    DVZ_EASING_IN_QUART,
    DVZ_EASING_OUT_QUART,
    DVZ_EASING_IN_OUT_QUART,
    DVZ_EASING_IN_QUINT,
    DVZ_EASING_OUT_QUINT,
    DVZ_EASING_IN_OUT_QUINT,
    DVZ_EASING_IN_EXPO,
    DVZ_EASING_OUT_EXPO,
    DVZ_EASING_IN_OUT_EXPO,
    DVZ_EASING_IN_CIRC,
    DVZ_EASING_OUT_CIRC,
    DVZ_EASING_IN_OUT_CIRC,
    DVZ_EASING_IN_BACK,
    DVZ_EASING_OUT_BACK,
    DVZ_EASING_IN_OUT_BACK,
    DVZ_EASING_IN_ELASTIC,
    DVZ_EASING_OUT_ELASTIC,
    DVZ_EASING_IN_OUT_ELASTIC,
    DVZ_EASING_IN_BOUNCE,
    DVZ_EASING_OUT_BOUNCE,
    DVZ_EASING_IN_OUT_BOUNCE,
    DVZ_EASING_COUNT,
} DvzEasing;



#endif
