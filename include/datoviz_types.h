/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_TYPES
#define DVZ_HEADER_PUBLIC_TYPES



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz_enums.h"
#include "datoviz_keycodes.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzShape DvzShape;
typedef struct DvzMVP DvzMVP;
typedef struct DvzViewport DvzViewport;
typedef struct _VkViewport _VkViewport;
typedef struct DvzAtlasFont DvzAtlasFont;
typedef struct DvzTime DvzTime;

typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef union DvzMouseEventUnion DvzMouseEventUnion;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;

typedef struct DvzWindowEvent DvzWindowEvent;
typedef struct DvzFrameEvent DvzFrameEvent;
typedef struct DvzGuiEvent DvzGuiEvent;
typedef struct DvzTimerEvent DvzTimerEvent;
typedef struct DvzRequestsEvent DvzRequestsEvent;

// Requests.
typedef struct DvzRequestBoard DvzRequestBoard;
typedef struct DvzRequestCanvas DvzRequestCanvas;
typedef struct DvzRequestDat DvzRequestDat;
typedef struct DvzRequestTex DvzRequestTex;
typedef struct DvzRequestSampler DvzRequestSampler;
typedef struct DvzRequestShader DvzRequestShader;
typedef struct DvzRequestDatUpload DvzRequestDatUpload;
typedef struct DvzRequestTexUpload DvzRequestTexUpload;
typedef struct DvzRequestGraphics DvzRequestGraphics;
typedef struct DvzRequestPrimitive DvzRequestPrimitive;
typedef struct DvzRequestBlend DvzRequestBlend;
typedef struct DvzRequestMask DvzRequestMask;
typedef struct DvzRequestDepth DvzRequestDepth;
typedef struct DvzRequestPolygon DvzRequestPolygon;
typedef struct DvzRequestCull DvzRequestCull;
typedef struct DvzRequestFront DvzRequestFront;
typedef struct DvzRequestShaderSet DvzRequestShaderSet;
typedef struct DvzRequestVertex DvzRequestVertex;
typedef struct DvzRequestAttr DvzRequestAttr;
typedef struct DvzRequestSlot DvzRequestSlot;
typedef struct DvzRequestPush DvzRequestPush;
typedef struct DvzRequestSpecialization DvzRequestSpecialization;
typedef struct DvzRequestBindVertex DvzRequestBindVertex;
typedef struct DvzRequestBindIndex DvzRequestBindIndex;
typedef struct DvzRequestBindDat DvzRequestBindDat;
typedef struct DvzRequestBindTex DvzRequestBindTex;
typedef struct DvzRequestRecord DvzRequestRecord;
typedef struct DvzRequest DvzRequest;
typedef union DvzRequestContent DvzRequestContent;

typedef struct DvzRequester DvzRequester;
typedef struct DvzBatch DvzBatch;

// Qt.
typedef struct DvzQtApp DvzQtApp;
typedef struct QApplication QApplication;
typedef struct DvzQtWindow DvzQtWindow;

// Recorder.
typedef struct DvzRecorderViewport DvzRecorderViewport;
typedef struct DvzRecorderPush DvzRecorderPush;
typedef struct DvzRecorderDraw DvzRecorderDraw;
typedef struct DvzRecorderDrawIndexed DvzRecorderDrawIndexed;
typedef struct DvzRecorderDrawIndirect DvzRecorderDrawIndirect;
typedef struct DvzRecorderDrawIndexedIndirect DvzRecorderDrawIndexedIndirect;
typedef union DvzRecorderUnion DvzRecorderUnion;
typedef struct DvzRecorderCommand DvzRecorderCommand;

// Forward declarations.
typedef struct DvzTimerItem DvzTimerItem;
typedef struct DvzGuiWindow DvzGuiWindow;
typedef struct DvzApp DvzApp;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzFont DvzFont;
typedef struct DvzList DvzList;
typedef struct DvzFifo DvzFifo;

// Callback types.
typedef void (*DvzAppGuiCallback)(DvzApp* app, DvzId canvas_id, DvzGuiEvent ev);
typedef void (*DvzAppMouseCallback)(DvzApp* app, DvzId window_id, DvzMouseEvent ev);
typedef void (*DvzAppKeyboardCallback)(DvzApp* app, DvzId window_id, DvzKeyboardEvent ev);
typedef void (*DvzAppFrameCallback)(DvzApp* app, DvzId window_id, DvzFrameEvent ev);
typedef void (*DvzAppTimerCallback)(DvzApp* app, DvzId window_id, DvzTimerEvent ev);
typedef void (*DvzAppResizeCallback)(DvzApp* app, DvzId window_id, DvzWindowEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAtlasFont
{
    unsigned long ttf_size;
    unsigned char* ttf_bytes;
    DvzAtlas* atlas;
    DvzFont* font;
};



struct DvzMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;

    // float time;
};



// NOTE: this corresponds to VkViewport, but we want to avoid the inclusion of vklite.h here.
struct _VkViewport
{
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

// NOTE: must correspond to the shader structure in common.glsl
struct DvzViewport
{
    _VkViewport viewport; // Vulkan viewport
    vec4 margins;

    // Position and size of the viewport in screen coordinates.
    uvec2 offset_screen;
    uvec2 size_screen;

    // Position and size of the viewport in framebuffer coordinates.
    uvec2 offset_framebuffer;
    uvec2 size_framebuffer;

    // NOTE: obsolete?
    int flags;
    // TODO: aspect ratio
};



struct DvzShape
{
    // Transform variables during transform begin/end.
    mat4 transform; // transformation matrix
    uint32_t first; // first vertex to transform
    uint32_t count; // number of vertices to transform

    DvzShapeType type;     // shape type
    uint32_t vertex_count; // number of vertices
    uint32_t index_count;  // number of indices (three times the number of triangle faces)

    vec3* pos;       // 3D positions of each vertex
    vec3* normal;    // 3D normal vector at each vertex
    DvzColor* color; // RGBA color of each vertex
    vec4* texcoords; // texture coordinates as u, v, (unused), alpha
    float* isoline;  // scalar field for isolines
    vec3* d_left;    // the distance of each vertex to the left edge adjacent to each face vertex
    vec3* d_right;   // the distance of each vertex to the right edge adjacent to each face vertex
    cvec4* contour;  // in each face, a bit mask with 1 if the opposite edge belongs to the contour
                     // edge, 2 if it is a corner, 4 if it should be oriented differently
    DvzIndex* index; // the index buffer

    // UGLY HACK: this seems to be necessary to ensure struct size equality between C and ctypes
    // (just checkstructs), maybe some alignment issue.
    // double _;
};



struct DvzTime
{
    uint64_t seconds;
    uint64_t nanoseconds;
};



/*************************************************************************************************/
/*  Events                                                                                       */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
    void* user_data;
};



struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    vec2 press_pos;
    vec2 shift;
    bool is_press_valid; // whether the press event was valid
};

union DvzMouseEventUnion
{
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
};

struct DvzMouseEvent
{
    DvzMouseEventType type;
    DvzMouseEventUnion content;
    vec2 pos; // current position
    DvzMouseButton button;
    int mods;
    float content_scale;
    void* user_data;
};



struct DvzWindowEvent
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t screen_width;
    uint32_t screen_height;
    int flags;
    void* user_data;
};

struct DvzFrameEvent
{
    uint64_t frame_idx;
    double time;
    double interval;
    void* user_data;
};

struct DvzGuiEvent
{
    DvzGuiWindow* gui_window;
    void* user_data;
};

struct DvzTimerEvent
{
    uint32_t timer_idx;
    DvzTimerItem* timer_item;
    uint64_t step_idx;
    double time;
    void* user_data;
};



/*************************************************************************************************/
/*  Requests                                                                                     */
/*************************************************************************************************/



struct DvzRecorderViewport
{
    vec2 offset;
    vec2 shape; // in framebuffer pixels
};

struct DvzRecorderPush
{
    DvzId pipe_id;
    DvzShaderStageFlags shader_stages;
    DvzSize offset;
    DvzSize size;
    void* data;
};

struct DvzRecorderDraw
{
    DvzId pipe_id;
    uint32_t first_vertex;
    uint32_t vertex_count;
    uint32_t first_instance;
    uint32_t instance_count;
};

struct DvzRecorderDrawIndexed
{
    DvzId pipe_id;
    uint32_t first_index;
    uint32_t vertex_offset;
    uint32_t index_count;
    uint32_t first_instance;
    uint32_t instance_count;
};

struct DvzRecorderDrawIndirect
{
    DvzId pipe_id;
    DvzId dat_indirect_id;
    uint32_t draw_count;
};

struct DvzRecorderDrawIndexedIndirect
{
    DvzId pipe_id;
    DvzId dat_indirect_id;
    uint32_t draw_count;
};

union DvzRecorderUnion
{
    // Viewport.
    DvzRecorderViewport v;

    // Push constant.
    DvzRecorderPush p;

    // Direct draw.
    DvzRecorderDraw draw;

    // Indexed draw.
    DvzRecorderDrawIndexed draw_indexed;

    // Indirect draw.
    DvzRecorderDrawIndirect draw_indirect;

    // Indexed indirect draw.
    DvzRecorderDrawIndexedIndirect draw_indexed_indirect;
};

struct DvzRecorderCommand
{
    DvzRecorderCommandType type;
    DvzId canvas_id;
    DvzRequestObject object_type;

    DvzRecorderUnion contents;
};



struct DvzRequestBoard
{
    uint32_t width;
    uint32_t height;
    cvec4 background;
};

struct DvzRequestCanvas
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t screen_width;
    uint32_t screen_height;
    bool is_offscreen;
    cvec4 background;
};

struct DvzRequestDat
{
    DvzBufferType type;
    DvzSize size;
};

struct DvzRequestTex
{
    DvzTexDims dims;
    uvec3 shape;
    DvzFormat format;
};

struct DvzRequestSampler
{
    DvzFilter filter;
    DvzSamplerAddressMode mode;
};

struct DvzRequestShader
{
    DvzShaderFormat format;
    DvzShaderType type;
    DvzSize size;
    char* code;       // For GLSL.
    uint32_t* buffer; // For SPIRV
};

struct DvzRequestDatUpload
{
    int upload_type; // 0=direct (data pointer), otherwise custom transfer method
    DvzSize offset;
    DvzSize size;
    void* data;
};

struct DvzRequestTexUpload
{
    int upload_type; // 0=direct (data pointer), otherwise custom transfer method
    uvec3 offset;
    uvec3 shape;
    DvzSize size;
    void* data;
};

struct DvzRequestGraphics
{
    DvzGraphicsType type;
};

struct DvzRequestPrimitive
{
    DvzPrimitiveTopology primitive;
};

struct DvzRequestBlend
{
    DvzBlendType blend;
};

struct DvzRequestMask
{
    int32_t mask;
};

struct DvzRequestDepth
{
    DvzDepthTest depth;
};

struct DvzRequestPolygon
{
    DvzPolygonMode polygon;
};

struct DvzRequestCull
{
    DvzCullMode cull;
};

struct DvzRequestFront
{
    DvzFrontFace front;
};

struct DvzRequestShaderSet
{
    DvzId shader;
};

struct DvzRequestVertex
{
    uint32_t binding_idx;
    DvzSize stride;
    DvzVertexInputRate input_rate;
};

struct DvzRequestAttr
{
    uint32_t binding_idx;
    uint32_t location;
    DvzFormat format;
    DvzSize offset;
};

struct DvzRequestSlot
{
    uint32_t slot_idx;
    DvzDescriptorType type;
};

struct DvzRequestPush
{
    DvzShaderStageFlags shader_stages;
    DvzSize offset;
    DvzSize size;
};

struct DvzRequestSpecialization
{
    DvzShaderType shader;
    uint32_t idx;
    DvzSize size;
    void* value;
};

struct DvzRequestBindVertex
{
    uint32_t binding_idx;
    DvzId dat;
    DvzSize offset;
};

struct DvzRequestBindIndex
{
    DvzId dat;
    DvzSize offset;
};

struct DvzRequestBindDat
{
    uint32_t slot_idx;
    DvzId dat;
    DvzSize offset;
};

struct DvzRequestBindTex
{
    uint32_t slot_idx;
    DvzId tex;
    DvzId sampler;
    uvec3 offset;
};

struct DvzRequestRecord
{
    DvzRecorderCommand command;
};



union DvzRequestContent
{
    // Canvas.
    DvzRequestCanvas canvas;

    // Dat.
    DvzRequestDat dat;

    // Tex.
    DvzRequestTex tex;

    // Sampler.
    DvzRequestSampler sampler;

    // Shader.
    DvzRequestShader shader;

    // Dat upload.
    DvzRequestDatUpload dat_upload;

    // Tex upload.
    DvzRequestTexUpload tex_upload;

    // Graphics.
    DvzRequestGraphics graphics;

    // Set primitive.
    DvzRequestPrimitive set_primitive;

    // Set blend type.
    DvzRequestBlend set_blend;

    // Set color mask.
    DvzRequestMask set_mask;

    // Set depth test.
    DvzRequestDepth set_depth;

    // Set polygon mode.
    DvzRequestPolygon set_polygon;

    // Set cull mode.
    DvzRequestCull set_cull;

    // Set front face mode.
    DvzRequestFront set_front;

    // Set SPIRV or GLSL shader.
    DvzRequestShaderSet set_shader;

    // Set vertex binding.
    DvzRequestVertex set_vertex;

    // Set vertex attribute.
    DvzRequestAttr set_attr;

    // Set descriptor slot.
    DvzRequestSlot set_slot;

    // Set push layout.
    DvzRequestPush set_push;

    // Set specialization constant.
    DvzRequestSpecialization set_specialization;

    // Bind a dat to a vertex binding.
    DvzRequestBindVertex bind_vertex;

    // Bind a dat to an index binding.
    DvzRequestBindIndex bind_index;

    // Bind a dat to a descriptor slot.
    DvzRequestBindDat bind_dat;

    // Bind a tex to a descriptor slot.
    DvzRequestBindTex bind_tex;

    // Record a command.
    DvzRequestRecord record;
};



struct DvzRequest
{
    uint32_t version;          // request version
    DvzRequestAction action;   // type of action
    DvzRequestObject type;     // type of the object targetted by the action
    DvzId id;                  // id of the object
    DvzRequestContent content; // details on the action
    int tag;                   // optional tag
    int flags;                 // custom flags
    const char* desc;          // optional string
};



struct DvzBatch
{
    uint32_t capacity;
    uint32_t count;
    DvzRequest* requests;

    DvzList* pointers_to_free; // HACK: list of pointers created when loading requests dumps
    int flags;
};



struct DvzRequestsEvent
{
    DvzBatch* batch;
    void* user_data;
};



struct DvzRequester
{
    DvzFifo* fifo;
};



#endif
