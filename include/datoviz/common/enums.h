/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "macros.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// App creation flags.
typedef enum
{
    DVZ_APP_FLAGS_NONE = 0x000000,
    DVZ_APP_FLAGS_OFFSCREEN = 0x008000, // INTERNAL: also passed as CanvasFlags in visual_test.h

    // NOTE: must match DVZ_RENDERER_FLAGS_WHITE_BACKGROUND
    DVZ_APP_FLAGS_EXTERNAL = 0x080000,
    DVZ_APP_FLAGS_WHITE_BACKGROUND = 0x100000,

} DvzAppFlags;



// Backend.
typedef enum
{
    DVZ_BACKEND_NONE,
    DVZ_BACKEND_GLFW,
    DVZ_BACKEND_QT,
    DVZ_BACKEND_OFFSCREEN,
    DVZ_BACKEND_WRAP,
} DvzBackend;



// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003,     // NOTE: 1 bit for ImGUI, 1 bit for FPS
    DVZ_CANVAS_FLAGS_MONITOR = 0x0005, // NOTE: 1 bit for ImGUI, 1 bit for Monitor
    DVZ_CANVAS_FLAGS_FULLSCREEN = 0x0008,
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010,
    DVZ_CANVAS_FLAGS_PICK = 0x0020,
    DVZ_CANVAS_FLAGS_PUSH_SCALE = 0x0040, // HACK: shaders expect a push constant with scaling
} DvzCanvasFlags;



// Dimension.
typedef enum
{
    DVZ_DIM_X = 0x0000,
    DVZ_DIM_Y = 0x0001,
    DVZ_DIM_Z = 0x0002,
    DVZ_DIM_COUNT,
} DvzDim;



// Ref flags.
typedef enum
{
    DVZ_REF_FLAGS_NONE = 0x00,
    DVZ_REF_FLAGS_EQUAL = 0x01,
} DvzRefFlags;



// Axis flags.
typedef enum
{
    DVZ_AXIS_FLAGS_NONE = 0x00,
    DVZ_AXIS_FLAGS_DARK = 0x01,
} DvzAxisFlags;



// Visual flags.
// NOTE: these flags are also passed to BakerFlags
typedef enum
{
    DVZ_VISUAL_FLAGS_DEFAULT = 0x000000,
    DVZ_VISUAL_FLAGS_INDEXED = 0x010000,
    DVZ_VISUAL_FLAGS_INDIRECT = 0x020000,

    DVZ_VISUAL_FLAGS_FIXED_X = 0x001000,
    DVZ_VISUAL_FLAGS_FIXED_Y = 0x002000,
    DVZ_VISUAL_FLAGS_FIXED_Z = 0x004000,
    DVZ_VISUAL_FLAGS_FIXED_ALL = 0x007000,

    DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000,
    DVZ_VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000,
} DvzVisualFlags;



// View flags (panel_visual).
typedef enum
{
    DVZ_VIEW_FLAGS_NONE = 0x0000,
    DVZ_VIEW_FLAGS_STATIC = 0x0010,
    DVZ_VIEW_FLAGS_NOCLIP = 0x0020,
} DvzViewFlags;



// Panel link flags.
typedef enum
{
    DVZ_PANEL_LINK_FLAGS_NONE = 0x00,
    DVZ_PANEL_LINK_FLAGS_MODEL = 0x01,
    DVZ_PANEL_LINK_FLAGS_VIEW = 0x02,
    DVZ_PANEL_LINK_FLAGS_PROJECTION = 0x04,
} DvzPanelLinkFlags;



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



// Tex flags.
typedef enum
{
    DVZ_TEX_FLAGS_NONE = 0x0000,               // default
    DVZ_TEX_FLAGS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzTexFlags;



// Mock flags.
typedef enum
{
    DVZ_MOCK_FLAGS_NONE = 0x00,
    DVZ_MOCK_FLAGS_CLOSED = 0x01,
} DvzMockFlags;



// Tex dims.
typedef enum
{
    DVZ_TEX_NONE,
    DVZ_TEX_1D,
    DVZ_TEX_2D,
    DVZ_TEX_3D,
} DvzTexDims;



// Graphics builtins
// TODO: remove?
typedef enum
{
    DVZ_GRAPHICS_NONE,
    DVZ_GRAPHICS_POINT,
    DVZ_GRAPHICS_TRIANGLE,
    DVZ_GRAPHICS_CUSTOM,
} DvzGraphicsType;



/*************************************************************************************************/
/*  Requests                                                                                     */
/*************************************************************************************************/



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
// NOTE: we only included the most common ones, this list can be completed as needed.
// IMPORTANT: the original Vulkan enum values need to be used:
// see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
//
// NOTE: see https://vulkan.gpuinfo.org/listbufferformats.php for hardware support
// Avoid using poorly-supported formats.
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8_UINT = 13,
    DVZ_FORMAT_R8_SINT = 14,
    DVZ_FORMAT_R8G8_UNORM = 16,
    DVZ_FORMAT_R8G8_SNORM = 17,
    DVZ_FORMAT_R8G8_UINT = 20,
    DVZ_FORMAT_R8G8_SINT = 21,
    DVZ_FORMAT_R8G8B8_UNORM = 23, // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_SNORM = 24, // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_UINT = 27,  // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8_SINT = 28,  // NOTE: poor GPU hardware support
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R8G8B8A8_SNORM = 38,
    DVZ_FORMAT_R8G8B8A8_UINT = 41,
    DVZ_FORMAT_R8G8B8A8_SINT = 42,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
    DVZ_FORMAT_R32G32_UINT = 101,
    DVZ_FORMAT_R32G32_SINT = 102,
    DVZ_FORMAT_R32G32_SFLOAT = 103,
    DVZ_FORMAT_R32G32B32_UINT = 104,   // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SINT = 105,   // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SFLOAT = 106, // NOTE: poor GPU hardware support for textures
    DVZ_FORMAT_R32G32B32A32_UINT = 107,
    DVZ_FORMAT_R32G32B32A32_SINT = 108,
    DVZ_FORMAT_R32G32B32A32_SFLOAT = 109,

    // NOTE: poor GPU hardware support
    DVZ_FORMAT_R64_UINT = 110,
    DVZ_FORMAT_R64_SINT = 111,
    DVZ_FORMAT_R64_SFLOAT = 112,
    DVZ_FORMAT_R64G64_UINT = 113,
    DVZ_FORMAT_R64G64_SINT = 114,
    DVZ_FORMAT_R64G64_SFLOAT = 115,
    DVZ_FORMAT_R64G64B64_UINT = 116,
    DVZ_FORMAT_R64G64B64_SINT = 117,
    DVZ_FORMAT_R64G64B64_SFLOAT = 118,
    DVZ_FORMAT_R64G64B64A64_UINT = 119,
    DVZ_FORMAT_R64G64B64A64_SINT = 120,
    DVZ_FORMAT_R64G64B64A64_SFLOAT = 121,
} DvzFormat;



// Color mask.
// VkColorComponentFlagBits wrapper
typedef enum
{
    DVZ_MASK_COLOR_R = 0x00000001,
    DVZ_MASK_COLOR_G = 0x00000002,
    DVZ_MASK_COLOR_B = 0x00000004,
    DVZ_MASK_COLOR_A = 0x00000008,
    DVZ_MASK_COLOR_ALL = 0x0000000F,
} DvzColorMask;



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



typedef int32_t DvzShaderStageFlags;



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
/*  Visual enums                                                                                 */
/*************************************************************************************************/

// Buffer type.
// NOTE: the enum index should correspond to the buffer index in the context->buffers container
typedef enum
{
    DVZ_BUFFER_TYPE_UNDEFINED,
    DVZ_BUFFER_TYPE_STAGING,  // 1
    DVZ_BUFFER_TYPE_VERTEX,   // 2
    DVZ_BUFFER_TYPE_INDEX,    // 3
    DVZ_BUFFER_TYPE_STORAGE,  // 4
    DVZ_BUFFER_TYPE_UNIFORM,  // 5
    DVZ_BUFFER_TYPE_INDIRECT, // 6
} DvzBufferType;

#define DVZ_BUFFER_TYPE_COUNT 6



// Shader format.
typedef enum
{
    DVZ_SHADER_NONE,
    DVZ_SHADER_SPIRV,
    DVZ_SHADER_GLSL,
} DvzShaderFormat;



// Texture axis.
typedef enum
{
    DVZ_SAMPLER_AXIS_U,
    DVZ_SAMPLER_AXIS_V,
    DVZ_SAMPLER_AXIS_W,
} DvzSamplerAxis;



// Blend type.
typedef enum
{
    DVZ_BLEND_DISABLE,
    DVZ_BLEND_STANDARD,
    DVZ_BLEND_DESTINATION,
    DVZ_BLEND_OIT,
} DvzBlendType;



// Slot type.
typedef enum
{
    DVZ_SLOT_DAT,
    DVZ_SLOT_TEX,
    DVZ_SLOT_COUNT,
} DvzSlotType;



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
    DVZ_MARKER_SHAPE_ROUNDED_RECT = 19,
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
    DVZ_CAP_NONE = 0,
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



// Path flags.
typedef enum
{
    DVZ_PATH_FLAGS_OPEN,
    DVZ_PATH_FLAGS_CLOSED,
} DvzPathFlags;



// Image flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_IMAGE_FLAGS_SIZE_PIXELS = 0x0000, // image size is specified in pixels
    DVZ_IMAGE_FLAGS_SIZE_NDC = 0x0001, // image size is specified in normalized device coordinates
    DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO = 0x0004, // image size ~ to total zoom level
    DVZ_IMAGE_FLAGS_RESCALE = 0x0008,            // image size ~ to axis zoom level
    DVZ_IMAGE_FLAGS_MODE_RGBA = 0x0000,          // image mode: RGBA texture
    DVZ_IMAGE_FLAGS_MODE_COLORMAP = 0x0010, // image mode: single-channel texture with colormap
    DVZ_IMAGE_FLAGS_MODE_FILL = 0x0020,     // image mode: fill color
    DVZ_IMAGE_FLAGS_BORDER = 0x0080,        // square or rounded border around the image
} DvzImageFlags;



// Shape type.
typedef enum
{
    DVZ_SHAPE_NONE,
    DVZ_SHAPE_SQUARE,
    DVZ_SHAPE_DISC,
    DVZ_SHAPE_SECTOR,
    DVZ_SHAPE_POLYGON,
    DVZ_SHAPE_HISTOGRAM,
    DVZ_SHAPE_CUBE,
    DVZ_SHAPE_SPHERE,
    DVZ_SHAPE_CYLINDER,
    DVZ_SHAPE_CONE,
    DVZ_SHAPE_TORUS,
    DVZ_SHAPE_ARROW,
    DVZ_SHAPE_TETRAHEDRON,
    DVZ_SHAPE_HEXAHEDRON,
    DVZ_SHAPE_OCTAHEDRON,
    DVZ_SHAPE_DODECAHEDRON,
    DVZ_SHAPE_ICOSAHEDRON,
    DVZ_SHAPE_SURFACE,
    DVZ_SHAPE_OBJ,
    DVZ_SHAPE_OTHER,
} DvzShapeType;



// Contour flags.
typedef enum
{
    DVZ_CONTOUR_NONE = 0x00,   // no contours
    DVZ_CONTOUR_EDGES = 0x01,  // set edge on some vertices (those on the contour)
    DVZ_CONTOUR_JOINTS = 0x02, // set joints on some vertices (those with 1 exterior adjacent edge)
    DVZ_CONTOUR_FULL = 0x04,   // set edge on all vertices
} DvzContourFlags;



// Indexing flags.
// This indicates how a mesh is being triangulated. This is used to specify predefine contours
// in the mesh visual, when using a DvzShape.
typedef enum
{
    DVZ_INDEXING_NONE = 0x00,    // no indexing
    DVZ_INDEXING_EARCUT = 0x10,  // polygon contour = consecutive indices i..(i+1)
    DVZ_INDEXING_SURFACE = 0x20, // indexing of mesh grid for surface
} DvzShapeIndexingFlags;



// Sphere flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_SPHERE_FLAGS_NONE = 0x0000,
    DVZ_SPHERE_FLAGS_TEXTURED = 0x0001,
    DVZ_SPHERE_FLAGS_LIGHTING = 0x0002,
    DVZ_SPHERE_FLAGS_SIZE_PIXELS = 0x0004,
    DVZ_SPHERE_FLAGS_EQUAL_RECTANGULAR = 0x0008,
} DvzSphereFlags;



// Mesh flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_MESH_FLAGS_NONE = 0x0000,
    DVZ_MESH_FLAGS_TEXTURED = 0x0001,
    DVZ_MESH_FLAGS_LIGHTING = 0x0002,
    DVZ_MESH_FLAGS_CONTOUR = 0x0004,
    DVZ_MESH_FLAGS_ISOLINE = 0x0008,
} DvzMeshFlags;



// Volume flags.
typedef enum
{
    DVZ_VOLUME_FLAGS_NONE = 0x0000,
    DVZ_VOLUME_FLAGS_RGBA = 0x0001,
    DVZ_VOLUME_FLAGS_COLORMAP = 0x0002,
    DVZ_VOLUME_FLAGS_BACK_FRONT = 0x0004,
} DvzVolumeFlags;



// Easing.
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



// Box flags.
typedef enum
{
    DVZ_BOX_EXTENT_DEFAULT = 0,               // no fixed aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND = 1,   // expand the box to match the aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_CONTRACT = 2, // contract the box to match the aspect ratio
} DvzBoxExtentStrategy;



// Box merge flags.
typedef enum
{
    DVZ_BOX_MERGE_DEFAULT = 0, // take extrema of input boxes
    DVZ_BOX_MERGE_CENTER = 1,  // merged is centered around 0 and encompasses all input boxes
    DVZ_BOX_MERGE_CORNER = 2,  // merged has (0,0,0) in its lower left corner
} DvzBoxMergeStrategy;



// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_VIEWPORT_CLIP_INNER = 0x0001,
    DVZ_VIEWPORT_CLIP_OUTER = 0x0002,
    DVZ_VIEWPORT_CLIP_BOTTOM = 0x0004,
    DVZ_VIEWPORT_CLIP_LEFT = 0x0008,
} DvzViewportClip;



// Depth test.
typedef enum
{
    DVZ_DEPTH_TEST_DISABLE,
    DVZ_DEPTH_TEST_ENABLE,
} DvzDepthTest;



// Alignment.
typedef enum
{
    DVZ_ALIGN_NONE,
    DVZ_ALIGN_LOW,
    DVZ_ALIGN_MIDDLE,
    DVZ_ALIGN_HIGH,
} DvzAlign;



// Orientation.
typedef enum
{
    DVZ_ORIENTATION_DEFAULT,
    DVZ_ORIENTATION_UP,
    DVZ_ORIENTATION_REVERSE,
    DVZ_ORIENTATION_DOWN,
} DvzOrientation;



// Predefined font for scene module.
typedef enum
{
    DVZ_SCENE_FONT_MONO,
    DVZ_SCENE_FONT_LABEL,
    DVZ_SCENE_FONT_COUNT,
} DvzSceneFont;
