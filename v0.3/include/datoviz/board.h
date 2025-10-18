/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOARD
#define DVZ_HEADER_BOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas.h"
#include "context.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzRecorder DvzRecorder;



EXTERN_C_ON

/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

/**
 * Create a board.
 *
 * @param gpu the GPU
 */
DvzCanvas
dvz_board(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags);



/**
 * Set the board image format.
 *
 * @param board the board
 * @param format the image format
 */
void dvz_board_format(DvzCanvas* board, DvzFormat format);



// /**
//  * Set the board clear color.
//  *
//  * @param board the board
//  * @param color the color as an array of 4 bytes
//  */
// void dvz_board_clear_color(DvzCanvas* board, cvec4 color);



/**
 * Create the board after is has been set up.
 *
 * @param board the board
 */
void dvz_board_create(DvzCanvas* board);



/**
 * Recreate the board.
 *
 * @param board the board
 */
void dvz_board_recreate(DvzCanvas* board);



/**
 * Change the board width and height.
 *
 * @param board the board
 * @param width the width, in framebuffer pixels
 * @param height the height, in framebuffer pixels
 */
void dvz_board_resize(DvzCanvas* board, uint32_t width, uint32_t height);



/**
 * Start rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
void dvz_board_begin(DvzCanvas* board, DvzCommands* cmds, uint32_t idx);



/**
 * Set the viewport when filling a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 * @param offset the viewport offset (x, y)
 * @param size the viewport size (w, h)
 */
void dvz_board_viewport( //
    DvzCanvas* board, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size);



/**
 * Stop rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
void dvz_board_end(DvzCanvas* board, DvzCommands* cmds, uint32_t idx);



/**
 * Allocate a CPU buffer for downloading the image.
 *
 * @param board the board
 */
uint8_t* dvz_board_alloc(DvzCanvas* board);



/**
 * Free the allocated CPU buffer storing the image.
 *
 * @param board the board
 */
void dvz_board_free(DvzCanvas* board);



/**
 * Download the current rendered image.
 *
 * @param board the board
 * @param size the image buffer size (should always be equal to width*height*4)
 * @param rgb an alread-allocated buffer that will contain the downloaded image
 */
void dvz_board_download(DvzCanvas* board, DvzSize size, uint8_t* rgb);



/**
 * Destroy a board.
 *
 * @param board the board
 */
void dvz_board_destroy(DvzCanvas* board);



EXTERN_C_OFF

#endif
