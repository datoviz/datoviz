/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_ENUMS
#define DVZ_HEADER_PUBLIC_ENUMS



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// App creation flags.
typedef enum
{
    DVZ_APP_FLAGS_NONE = 0x000000,
    DVZ_APP_FLAGS_OFFSCREEN = 0x008000, // INTERNAL: also passed as CanvasFlags in visual_test.h

    // NOTE: must match DVZ_RENDERER_FLAGS_WHITE_BACKGROUND
    DVZ_APP_FLAGS_WHITE_BACKGROUND = 0x100000,

} DvzAppFlags;



// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003,     // NOTE: 1 bit for ImGUI, 1 bit for FPS
    DVZ_CANVAS_FLAGS_MONITOR = 0x0005, // NOTE: 1 bit for ImGUI, 1 bit for Monitor
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010,
    DVZ_CANVAS_FLAGS_PICK = 0x0020,
} DvzCanvasFlags;



// Keyboard mods
// NOTE: must match GLFW values! no mapping is done as of now
typedef enum
{
    DVZ_KEY_MODIFIER_NONE = 0x00000000,
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001,
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002,
    DVZ_KEY_MODIFIER_ALT = 0x00000004,
    DVZ_KEY_MODIFIER_SUPER = 0x00000008,
} DvzKeyboardModifiers;



// Keyboard event type (press or release)
typedef enum
{
    DVZ_KEYBOARD_EVENT_NONE,
    DVZ_KEYBOARD_EVENT_PRESS,
    DVZ_KEYBOARD_EVENT_REPEAT,
    DVZ_KEYBOARD_EVENT_RELEASE,
} DvzKeyboardEventType;



// Mouse buttons
typedef enum
{
    DVZ_MOUSE_BUTTON_NONE = 0,
    DVZ_MOUSE_BUTTON_LEFT = 1,
    DVZ_MOUSE_BUTTON_MIDDLE = 2,
    DVZ_MOUSE_BUTTON_RIGHT = 3,
} DvzMouseButton;



// Mouse states.
typedef enum
{
    DVZ_MOUSE_STATE_RELEASE = 0,
    DVZ_MOUSE_STATE_PRESS = 1,
    DVZ_MOUSE_STATE_CLICK = 3,
    DVZ_MOUSE_STATE_CLICK_PRESS = 4,
    DVZ_MOUSE_STATE_DOUBLE_CLICK = 5,
    DVZ_MOUSE_STATE_DRAGGING = 11,
} DvzMouseState;



// Mouse events.
typedef enum
{
    DVZ_MOUSE_EVENT_RELEASE = 0,
    DVZ_MOUSE_EVENT_PRESS = 1,
    DVZ_MOUSE_EVENT_MOVE = 2,
    DVZ_MOUSE_EVENT_CLICK = 3,
    DVZ_MOUSE_EVENT_DOUBLE_CLICK = 5,
    DVZ_MOUSE_EVENT_DRAG_START = 10,
    DVZ_MOUSE_EVENT_DRAG = 11,
    DVZ_MOUSE_EVENT_DRAG_STOP = 12,
    DVZ_MOUSE_EVENT_WHEEL = 20,
    DVZ_MOUSE_EVENT_ALL = 255,
} DvzMouseEventType;



// GUI flags.
typedef enum
{
    DVZ_GUI_FLAGS_NONE = 0x0000,
    DVZ_GUI_FLAGS_OFFSCREEN = 0x0001,
    DVZ_GUI_FLAGS_DOCKING = 0x0010,
} DvzGuiFlags;



// GUI dialog flags.
typedef enum
{
    DVZ_DIALOG_FLAGS_NONE = 0x0000,
    DVZ_DIALOG_FLAGS_OVERLAY = 0x0001,
    DVZ_DIALOG_FLAGS_BLANK = 0x0004,
    DVZ_DIALOG_FLAGS_PANEL = 0x0008,
} DvzDialogFlags;



// GUI corner.
typedef enum
{
    DVZ_DIALOG_CORNER_TOP_LEFT = 0,
    DVZ_DIALOG_CORNER_TOP_RIGHT = 1,
    DVZ_DIALOG_CORNER_BOTTOM_LEFT = 2,
    DVZ_DIALOG_CORNER_BOTTOM_RIGHT = 3,
} DvzCorner;



// Arcball flags.
typedef enum
{
    DVZ_ARCBALL_FLAGS_NONE,
    DVZ_ARCBALL_FLAGS_CONSTRAIN,
} DvzArcballFlags;



// Panzoom flags.
typedef enum
{
    DVZ_PANZOOM_FLAGS_NONE = 0x00,
    DVZ_PANZOOM_FLAGS_KEEP_ASPECT = 0x01,
    DVZ_PANZOOM_FLAGS_FIXED_X = 0x10,
    DVZ_PANZOOM_FLAGS_FIXED_Y = 0x20,
} DvzPanzoomFlags;



// Camera flags.
typedef enum
{
    DVZ_CAMERA_FLAGS_PERSPECTIVE = 0x00,
    DVZ_CAMERA_FLAGS_ORTHO = 0x01,
} DvzCameraFlags;



// Visual flags.
// NOTE: these flags are also passed to BakerFlags
typedef enum
{
    DVZ_VISUAL_FLAGS_DEFAULT = 0x000000,
    DVZ_VISUAL_FLAGS_INDEXED = 0x010000,
    DVZ_VISUAL_FLAGS_INDIRECT = 0x020000,

    DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000,
    DVZ_VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000,
} DvzVisualFlags;



// View flags (panel_visual).
typedef enum
{
    DVZ_VIEW_FLAGS_NONE = 0x0000,
    DVZ_VIEW_FLAGS_STATIC = 0x0001,
} DvzViewFlags;



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



// Font flags.
typedef enum
{
    DVZ_FONT_FLAGS_RGB,
    DVZ_FONT_FLAGS_RGBA,
} DvzFontFlags;



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

// Recorder command type.
typedef enum
{
    DVZ_RECORDER_NONE,
    DVZ_RECORDER_BEGIN,
    DVZ_RECORDER_DRAW,
    DVZ_RECORDER_DRAW_INDEXED,
    DVZ_RECORDER_DRAW_INDIRECT,
    DVZ_RECORDER_DRAW_INDEXED_INDIRECT,
    DVZ_RECORDER_VIEWPORT,
    DVZ_RECORDER_END,
} DvzRecorderCommandType;



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
    DVZ_FORMAT_R32G32B32_UINT = 104,   // NOTE: weak GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SINT = 105,   // NOTE: weak GPU hardware support for textures
    DVZ_FORMAT_R32G32B32_SFLOAT = 106, // NOTE: weak GPU hardware support for textures
    DVZ_FORMAT_R32G32B32A32_UINT = 107,
    DVZ_FORMAT_R32G32B32A32_SINT = 108,
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
    DVZ_BLEND_OIT,
} DvzBlendType;



// Slot type.
typedef enum
{
    DVZ_SLOT_DAT,
    DVZ_SLOT_TEX,
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
    DVZ_IMAGE_FLAGS_FILL = 0x0010, // image does not show a texture but a uniform color
} DvzImageFlags;



// Shape type.
typedef enum
{
    DVZ_SHAPE_NONE,
    DVZ_SHAPE_SQUARE,
    DVZ_SHAPE_DISC,
    DVZ_SHAPE_POLYGON,
    DVZ_SHAPE_CUBE,
    DVZ_SHAPE_SPHERE,
    DVZ_SHAPE_CYLINDER,
    DVZ_SHAPE_CONE,
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
    DVZ_CONTOUR_FULL = 0xF0,   // set edge on all vertices
} DvzContourFlags;



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



/*************************************************************************************************/
/*  Defaults                                                                                     */
/*************************************************************************************************/

#define DVZ_DEFAULT_FORMAT DVZ_FORMAT_B8G8R8A8_UNORM

#define DVZ_DEFAULT_VIEWPORT                                                                      \
    (vec2) { 0, 0 }

#define DVZ_DEFAULT_CLEAR_COLOR                                                                   \
    (cvec4) { 0, 0, 0, 0 }



/*************************************************************************************************/
/*  Colormap enums                                                                               */
/*************************************************************************************************/

// Colormaps: native, user-defined, total.
#define CMAP_OFS     0                     //   0
#define CMAP_NAT     144                   // 144
#define CMAP_USR_OFS CMAP_NAT              // 144
#define CMAP_USR     32                    //  32
#define CMAP_TOT     (CMAP_NAT + CMAP_USR) // 176

// Color palettes with 256 colors each: native, user-defined, total.
#define CPAL256_OFS     CMAP_TOT                    // 176
#define CPAL256_NAT     32                          //  32
#define CPAL256_USR_OFS (CPAL256_OFS + CPAL256_NAT) // 208
#define CPAL256_USR     32                          //  32
#define CPAL256_TOT     (CPAL256_NAT + CPAL256_USR) //  64

// Color palettes with 32 colors each: native, user-defined, total.
// There are 8 palettes per row in the texture (8*32=256)
#define CPAL032_OFS     (CPAL256_OFS + CPAL256_TOT) // 240
#define CPAL032_NAT     8                           //   8
#define CPAL032_USR_OFS (CPAL032_OFS + CPAL032_NAT) // 248
#define CPAL032_USR     8                           //   8
#define CPAL032_PER_ROW 8                           //   8
#define CPAL032_SIZ     32                          //  32
#define CPAL032_TOT     (CPAL032_NAT + CPAL032_USR) //  16

#define CMAP_COUNT 256

// Custom colormaps.
#define CMAP_CUSTOM_COUNT 16
#define CMAP_CUSTOM       (CMAP_TOT - CMAP_CUSTOM_COUNT)    // 160
#define CPAL256_CUSTOM    (CPAL032_OFS - CMAP_CUSTOM_COUNT) // 224
// TODO: CPAL032 custom

// Colormaps.
typedef enum
{
    // Standard colormaps
    DVZ_CMAP_BINARY, // grey level
    DVZ_CMAP_HSV,    // all HSV hues

    // matplotlib perceptually uniform

    DVZ_CMAP_CIVIDIS,
    DVZ_CMAP_INFERNO,
    DVZ_CMAP_MAGMA,
    DVZ_CMAP_PLASMA,
    DVZ_CMAP_VIRIDIS,

    // matplotlib sequential

    DVZ_CMAP_BLUES,
    DVZ_CMAP_BUGN,
    DVZ_CMAP_BUPU,
    DVZ_CMAP_GNBU,
    DVZ_CMAP_GREENS,
    DVZ_CMAP_GREYS,
    DVZ_CMAP_ORANGES,
    DVZ_CMAP_ORRD,
    DVZ_CMAP_PUBU,
    DVZ_CMAP_PUBUGN,
    DVZ_CMAP_PURPLES,
    DVZ_CMAP_RDPU,
    DVZ_CMAP_REDS,
    DVZ_CMAP_YLGN,
    DVZ_CMAP_YLGNBU,
    DVZ_CMAP_YLORBR,
    DVZ_CMAP_YLORRD,

    // matplotlib sequential 2

    DVZ_CMAP_AFMHOT,
    DVZ_CMAP_AUTUMN,
    DVZ_CMAP_BONE,
    DVZ_CMAP_COOL,
    DVZ_CMAP_COPPER,
    DVZ_CMAP_GIST_HEAT,
    DVZ_CMAP_GRAY,
    DVZ_CMAP_HOT,
    DVZ_CMAP_PINK,
    DVZ_CMAP_SPRING,
    DVZ_CMAP_SUMMER,
    DVZ_CMAP_WINTER,
    DVZ_CMAP_WISTIA,

    // matplotlib diverging

    DVZ_CMAP_BRBG,
    DVZ_CMAP_BWR,
    DVZ_CMAP_COOLWARM,
    DVZ_CMAP_PIYG,
    DVZ_CMAP_PRGN,
    DVZ_CMAP_PUOR,
    DVZ_CMAP_RDBU,
    DVZ_CMAP_RDGY,
    DVZ_CMAP_RDYLBU,
    DVZ_CMAP_RDYLGN,
    DVZ_CMAP_SEISMIC,
    DVZ_CMAP_SPECTRAL,

    // matplotlib cyclic

    DVZ_CMAP_TWILIGHT_SHIFTED,
    DVZ_CMAP_TWILIGHT,

    // matplotlib misc

    DVZ_CMAP_BRG,
    DVZ_CMAP_CMRMAP,
    DVZ_CMAP_CUBEHELIX,
    DVZ_CMAP_FLAG,
    DVZ_CMAP_GIST_EARTH,
    DVZ_CMAP_GIST_NCAR,
    DVZ_CMAP_GIST_RAINBOW,
    DVZ_CMAP_GIST_STERN,
    DVZ_CMAP_GNUPLOT2,
    DVZ_CMAP_GNUPLOT,
    DVZ_CMAP_JET,
    DVZ_CMAP_NIPY_SPECTRAL,
    DVZ_CMAP_OCEAN,
    DVZ_CMAP_PRISM,
    DVZ_CMAP_RAINBOW,
    DVZ_CMAP_TERRAIN,

    // colorcet diverging

    DVZ_CMAP_BKR,
    DVZ_CMAP_BKY,
    DVZ_CMAP_CET_D10,
    DVZ_CMAP_CET_D11,
    DVZ_CMAP_CET_D8,
    DVZ_CMAP_CET_D13,
    DVZ_CMAP_CET_D3,
    DVZ_CMAP_CET_D1A,
    DVZ_CMAP_BJY,
    DVZ_CMAP_GWV,
    DVZ_CMAP_BWY,
    DVZ_CMAP_CET_D12,
    DVZ_CMAP_CET_R3,
    DVZ_CMAP_CET_D9,
    DVZ_CMAP_CWR,

    // colorcet colorblind

    DVZ_CMAP_CET_CBC1,
    DVZ_CMAP_CET_CBC2,
    DVZ_CMAP_CET_CBL1,
    DVZ_CMAP_CET_CBL2,
    DVZ_CMAP_CET_CBTC1,
    DVZ_CMAP_CET_CBTC2,
    DVZ_CMAP_CET_CBTL1,

    // colorcet others

    DVZ_CMAP_BGY,
    DVZ_CMAP_BGYW,
    DVZ_CMAP_BMW,
    DVZ_CMAP_CET_C1,
    DVZ_CMAP_CET_C1S,
    DVZ_CMAP_CET_C2,
    DVZ_CMAP_CET_C4,
    DVZ_CMAP_CET_C4S,
    DVZ_CMAP_CET_C5,
    DVZ_CMAP_CET_I1,
    DVZ_CMAP_CET_I3,
    DVZ_CMAP_CET_L10,
    DVZ_CMAP_CET_L11,
    DVZ_CMAP_CET_L12,
    DVZ_CMAP_CET_L16,
    DVZ_CMAP_CET_L17,
    DVZ_CMAP_CET_L18,
    DVZ_CMAP_CET_L19,
    DVZ_CMAP_CET_L4,
    DVZ_CMAP_CET_L7,
    DVZ_CMAP_CET_L8,
    DVZ_CMAP_CET_L9,
    DVZ_CMAP_CET_R1,
    DVZ_CMAP_CET_R2,
    DVZ_CMAP_COLORWHEEL,
    DVZ_CMAP_FIRE,
    DVZ_CMAP_ISOLUM,
    DVZ_CMAP_KB,
    DVZ_CMAP_KBC,
    DVZ_CMAP_KG,
    DVZ_CMAP_KGY,
    DVZ_CMAP_KR,

    // Moreland colormaps

    DVZ_CMAP_BLACK_BODY,
    DVZ_CMAP_KINDLMANN,
    DVZ_CMAP_EXTENDED_KINDLMANN,

    // colorcet palettes with 256 colors

    DVZ_CPAL256_GLASBEY = CPAL256_OFS,
    DVZ_CPAL256_GLASBEY_COOL,
    DVZ_CPAL256_GLASBEY_DARK,
    DVZ_CPAL256_GLASBEY_HV,
    DVZ_CPAL256_GLASBEY_LIGHT,
    DVZ_CPAL256_GLASBEY_WARM,

    // matplotlib palettes with <= 32 colors

    DVZ_CPAL032_ACCENT = CPAL032_OFS,
    DVZ_CPAL032_DARK2,
    DVZ_CPAL032_PAIRED,
    DVZ_CPAL032_PASTEL1,
    DVZ_CPAL032_PASTEL2,
    DVZ_CPAL032_SET1,
    DVZ_CPAL032_SET2,
    DVZ_CPAL032_SET3,

    // (new row in the texture after 8 palettes)

    DVZ_CPAL032_TAB10,
    DVZ_CPAL032_TAB20,
    DVZ_CPAL032_TAB20B,
    DVZ_CPAL032_TAB20C,

    // bokeh palettes with <= 32 colors

    DVZ_CPAL032_CATEGORY10_10,
    DVZ_CPAL032_CATEGORY20_20,
    DVZ_CPAL032_CATEGORY20B_20,
    DVZ_CPAL032_CATEGORY20C_20,

    // (new row in the texture after 8 palettes)

    // BUG: this is 256, =0 with uint8, so this colormap is not working atm
    DVZ_CPAL032_COLORBLIND8,

    // OS palettes

    // DVZ_CPAL032_WINDOWS_16,
    // DVZ_CPAL032_WINDOWS_20,
    // DVZ_CPAL032_APPLE_16,
    // DVZ_CPAL032_RISC_16,
    // DVZ_CPAL032_WEB_16,

} DvzColormap;



#endif
