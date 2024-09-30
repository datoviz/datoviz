/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RENDERER
#define DVZ_HEADER_RENDERER



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "common.h"
#include "datoviz_protocol.h"
#include "pipelib.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRenderer DvzRenderer;
typedef struct DvzRouter DvzRouter;

// Forward declarations.
typedef struct DvzContext DvzContext;
typedef struct DvzPipelib DvzPipelib;
typedef struct DvzWorkspace DvzWorkspace;
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzBoard DvzBoard;
typedef struct DvzMap DvzMap;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRenderer
{
    DvzObject obj;
    DvzGpu* gpu;
    int flags;

    DvzContext* ctx;         // data: the "what"
    DvzPipelib* pipelib;     // GLSL programs: the "how"
    DvzWorkspace* workspace; // boards and canvases: the "where"
    DvzContainer shaders;
    DvzMap* map;       // mapping between uuid and <type, objects>
    DvzRouter* router; // mapping between pairs (action, obj_type) and functions
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a renderer.
 *
 * @param gpu the GPU
 * @param flags renderer creation flags
 * @returns the renderer
 */
DvzRenderer* dvz_renderer(DvzGpu* gpu, int flags);



/**
 * Submit a request to the renderer.
 *
 * @param rd the renderer
 * @param req the request
 */
void dvz_renderer_request(DvzRenderer* rd, DvzRequest req);



/**
 * Submit a batch of requests to the renderer.
 *
 * @param rd the renderer
 * @param count the number of requests to submit
 * @param reqs an array of requests
 */
void dvz_renderer_requests(DvzRenderer* rd, uint32_t count, DvzRequest* reqs);



/**
 * Return a board.
 *
 * @param rd the renderer
 * @param id the board id
 * @returns the board
 */
DvzBoard* dvz_renderer_board(DvzRenderer* rd, DvzId id);



/**
 * Return a canvas.
 *
 * @param rd the renderer
 * @param id the canvas id
 * @returns the canvas
 */
DvzCanvas* dvz_renderer_canvas(DvzRenderer* rd, DvzId id);



/**
 * Return a dat.
 *
 * @param rd the renderer
 * @param id the dat id
 * @returns the dat
 */
DvzDat* dvz_renderer_dat(DvzRenderer* rd, DvzId id);



/**
 * Return a tex.
 *
 * @param rd the renderer
 * @param id the tex id
 * @returns the tex
 */
DvzTex* dvz_renderer_tex(DvzRenderer* rd, DvzId id);



/**
 * Return a pipe.
 *
 * @param rd the renderer
 * @param id the pipe id
 * @returns the pipe
 */
DvzPipe* dvz_renderer_pipe(DvzRenderer* rd, DvzId id);



/**
 * Retrieve the rendered image.
 *
 * @param rd the renderer
 * @param board_id the id of the board
 * @param size a pointer to a variable that will store the size, in bytes, of the downloaded image
 * @param rgb a pointer to the image, or NULL if this array should be handled by datoviz
 * @returns a pointer to the image
 */
uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId bc_id, DvzSize* size, uint8_t* rgb);



/**
 * Destroy a renderer.
 *
 * @param rd the renderer
 */
void dvz_renderer_destroy(DvzRenderer* rd);



EXTERN_C_OFF

#endif
