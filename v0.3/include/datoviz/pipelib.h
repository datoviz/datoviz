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
typedef struct DvzShader DvzShader;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPipelib
{
    DvzObject obj;
    DvzGpu* gpu;
    DvzContainer graphics;
    DvzContainer computes;
    DvzContainer shaders;
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



/**
 * Create a new shader.
 *
 * @param lib the pipelib instance
 * @param format the shader format (GLSL or SPIRV)
 * @param type the shader type
 * @param size the size of the buffer (0 if code is set)
 * @param code the GLSL code (NULL if buffer is set)
 * @param buffer the SPIR-V bytecode buffer (NULL if code is set)
 * @returns the shader
 */
DvzShader* dvz_pipelib_shader(
    DvzPipelib* lib, DvzShaderFormat format, DvzShaderType type, DvzSize size, char* code,
    uint32_t* buffer);



/**
 * Destroy the pipelib.
 *
 * @param lib the pipelib instance
 */
void dvz_pipelib_destroy(DvzPipelib* lib);


EXTERN_C_OFF

#endif
