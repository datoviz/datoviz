/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Holds graphics and computes pipes                                                            */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PIPELIB
#define DVZ_HEADER_PIPELIB



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "pipe.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Pipelib flags.
// NOTE: these flags are also passed to graphics flags.
typedef enum
{
    DVZ_PIPELIB_FLAGS_NONE = 0x000000,
    DVZ_PIPELIB_FLAGS_CREATE_MVP = 0x100000,
    DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT = 0x200000,
} DvzPipelibFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPipelib DvzPipelib;

// Forward declarations.
typedef struct DvzContext DvzContext;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPipelib
{
    DvzObject obj;
    DvzGpu* gpu;
    DvzContainer graphics;
    DvzContainer computes;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a library of pipes (computes/graphics).
 *
 * @param ctx the context
 * @returns the pipelib
 */
DvzPipelib* dvz_pipelib(DvzContext* ctx);



/**
 * Create a new builtin graphics pipe.
 *
 * @param lib the pipelib instance
 * @param ctx the context
 * @param renderpass the renderpass
 * @param type the builtin graphics type
 * @param flags the graphics creation flags
 * @returns the pipe
 */
DvzPipe* dvz_pipelib_graphics(
    DvzPipelib* lib, DvzContext* ctx, DvzRenderpass* renderpass, DvzGraphicsType type, int flags);



/**
 * Create a new compute from a compiled SPIRV .spv shader file.
 *
 * @param lib the pipelib instance
 * @param shader_path the path to the .spv shader file
 * @returns the pipe
 */
DvzPipe* dvz_pipelib_compute_file(DvzPipelib* lib, const char* shader_path);



// /**
//  * Destroy a pipe created by the pipelib.
//  *
//  * @param lib the pipelib instance
//  * @param pipe the pipe to destroy
//  */
// void dvz_pipelib_pipe_destroy(DvzPipelib* lib, DvzPipe* pipe);



/**
 * Destroy the pipelib.
 *
 * @param lib the pipelib instance
 */
void dvz_pipelib_destroy(DvzPipelib* lib);


EXTERN_C_OFF

#endif
