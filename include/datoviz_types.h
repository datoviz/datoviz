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
    vec3* d_left;    // the distance of each vertex to the left edge adjacent to each face vertex
    vec3* d_right;   // the distance of each vertex to the right edge adjacent to each face vertex
    cvec3* contour;  // in each face, a bit mask with 1 if the opposite edge belongs to the contour
                     // edge, 2 if it is a corner, 4 if it should be oriented differently
    DvzIndex* index; // the index buffer

    // UGLY HACK: this seems to be necessary to ensure struct size equality between C and ctypes
    // (just checkstructs), maybe some alignment issue.
    double _;
};



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



#endif
