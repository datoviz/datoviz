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
typedef struct DvzAtlasFont DvzAtlasFont;

typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef union DvzMouseEventUnion DvzMouseEventUnion;
typedef struct DvzMouseButtonEvent DvzMouseButtonEvent;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;
typedef struct DvzMouseClickEvent DvzMouseClickEvent;

typedef struct DvzWindowEvent DvzWindowEvent;
typedef struct DvzFrameEvent DvzFrameEvent;
typedef struct DvzGuiEvent DvzGuiEvent;
typedef struct DvzTimerEvent DvzTimerEvent;
typedef struct DvzRequestsEvent DvzRequestsEvent;

typedef struct DvzRequest DvzRequest;
typedef union DvzRequestContent DvzRequestContent;
typedef struct DvzRecorderCommand DvzRecorderCommand;

// Forward declarations.
typedef struct DvzTimerItem DvzTimerItem;
typedef struct DvzBatch DvzBatch;
typedef struct DvzApp DvzApp;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzFont DvzFont;

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
    cvec4* color;    // RGBA color of each vertex
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



struct DvzMouseButtonEvent
{
    DvzMouseButton button;
};

struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    DvzMouseButton button;
    vec2 press_pos;
    vec2 shift;
};

struct DvzMouseClickEvent
{
    DvzMouseButton button;
};

union DvzMouseEventUnion
{
    DvzMouseButtonEvent b;
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
    DvzMouseClickEvent c;
};

struct DvzMouseEvent
{
    DvzMouseEventType type;
    DvzMouseEventUnion content;
    vec2 pos; // current position
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

struct DvzRequestsEvent
{
    // uint32_t request_count;
    // void* requests;
    DvzBatch* batch;
    void* user_data;
};



/*************************************************************************************************/
/*  Requests                                                                                     */
/*************************************************************************************************/

struct DvzRecorderCommand
{
    DvzRecorderCommandType type;
    DvzId canvas_or_board_id;
    DvzRequestObject object_type;

    union
    {
        // Viewport.
        struct
        {
            vec2 offset, shape; // in framebuffer pixels
        } v;

        // Direct draw.
        struct
        {
            DvzId pipe_id;
            uint32_t first_vertex, vertex_count;
            uint32_t first_instance, instance_count;
        } draw;

        // Indexed draw.
        struct
        {
            DvzId pipe_id;
            uint32_t first_index, vertex_offset, index_count;
            uint32_t first_instance, instance_count;
        } draw_indexed;

        // Indirect draw.
        struct
        {
            DvzId pipe_id;
            DvzId dat_indirect_id;
            uint32_t draw_count;
        } draw_indirect;

        // Indexed indirect draw.
        struct
        {
            DvzId pipe_id;
            DvzId dat_indirect_id;
            uint32_t draw_count;
        } draw_indexed_indirect;
    } contents;
};



union DvzRequestContent
{
    // Board.
    struct
    {
        uint32_t width, height;
        cvec4 background;
    } board;

    // Canvas.
    struct
    {
        uint32_t framebuffer_width, framebuffer_height;
        uint32_t screen_width, screen_height;
        cvec4 background;
    } canvas;

    // Dat.
    struct
    {
        DvzBufferType type;
        DvzSize size;
    } dat;

    // Tex.
    struct
    {
        DvzTexDims dims;
        uvec3 shape;
        DvzFormat format;
    } tex;

    // Sampler.
    struct
    {
        DvzFilter filter;
        DvzSamplerAddressMode mode;
    } sampler;

    // Shader.
    struct
    {
        DvzShaderFormat format;
        DvzShaderType type;
        DvzSize size;
        char* code;       // For GLSL.
        uint32_t* buffer; // For SPIRV
    } shader;



    // Dat upload.
    struct
    {
        int upload_type; // 0=direct (data pointer), otherwise custom transfer method
        DvzSize offset, size;
        void* data;
    } dat_upload;

    // Tex upload.
    struct
    {
        int upload_type; // 0=direct (data pointer), otherwise custom transfer method
        uvec3 offset, shape;
        DvzSize size;
        void* data;
    } tex_upload;



    // Graphics.
    struct
    {
        DvzGraphicsType type;
    } graphics;

    // Set primitive.
    struct
    {
        DvzPrimitiveTopology primitive;
    } set_primitive;

    // Set blend type.
    struct
    {
        DvzBlendType blend;
    } set_blend;

    // Set depth test.
    struct
    {
        DvzDepthTest depth;
    } set_depth;

    // Set polygon mode.
    struct
    {
        DvzPolygonMode polygon;
    } set_polygon;

    // Set cull mode.
    struct
    {
        DvzCullMode cull;
    } set_cull;

    // Set front face mode.
    struct
    {
        DvzFrontFace front;
    } set_front;

    // Set SPIRV or GLSL shader.
    struct
    {
        DvzId shader;
    } set_shader;

    // Set vertex binding.
    struct
    {
        uint32_t binding_idx;
        DvzSize stride;
        DvzVertexInputRate input_rate;
    } set_vertex;

    // Set vertex attribute.
    struct
    {
        uint32_t binding_idx;
        uint32_t location;
        DvzFormat format;
        DvzSize offset;
    } set_attr;

    // Set descriptor slot.
    struct
    {
        uint32_t slot_idx;
        DvzDescriptorType type;
    } set_slot;

    // Set specialization constant.
    struct
    {
        DvzShaderType shader;
        uint32_t idx;
        DvzSize size;
        void* value;
    } set_specialization;

    // Bind a dat to a vertex binding.
    struct
    {
        uint32_t binding_idx;
        DvzId dat;
        DvzSize offset;
    } bind_vertex;

    // Bind a dat to an index binding.
    struct
    {
        DvzId dat;
        DvzSize offset;
    } bind_index;

    // Bind a dat to a descriptor slot.
    struct
    {
        uint32_t slot_idx;
        DvzId dat;
        DvzSize offset;
    } bind_dat;

    // Bind a tex to a descriptor slot.
    struct
    {
        uint32_t slot_idx;
        DvzId tex;
        DvzId sampler;
        uvec3 offset;
    } bind_tex;



    // Record a command.
    struct
    {
        DvzRecorderCommand command;
    } record;
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



#endif
