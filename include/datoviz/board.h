/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOARD
#define DVZ_HEADER_BOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "context.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBoard DvzBoard;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBoard
{
    DvzObject obj;
    DvzGpu* gpu;

    VkFormat format;
    cvec4 clear_color;
    uint32_t width, height;

    DvzSize size; // width*height*4
    cvec4* rgba;  // GPU buffer storing the image

    DvzImages images;
    DvzImages depth;
    DvzImages staging;
    // TODO: picking
    // TODO: overlay imgui support
    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

/**
 * Create a board.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT DvzBoard dvz_board(DvzGpu* gpu, uint32_t width, uint32_t height);



/**
 * Set the board image format.
 *
 * @param board the board
 * @param format the image format
 */
DVZ_EXPORT void dvz_board_format(DvzBoard* board, VkFormat format);



/**
 * Set the board clear color.
 *
 * @param board the board
 * @param color the color as an array of 4 bytes
 */
DVZ_EXPORT void dvz_board_clear_color(DvzBoard* board, cvec4 color);



/**
 * Create the board after is has been set up.
 *
 * @param board the board
 */
DVZ_EXPORT void dvz_board_create(DvzBoard* board);



/**
 * Change the board width and height.
 *
 * @param board the board
 * @param width the width, in framebuffer pixels
 * @param height the height, in framebuffer pixels
 */
DVZ_EXPORT void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height);



/**
 * Start rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
DVZ_EXPORT void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx);



/**
 * Stop rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
DVZ_EXPORT void dvz_board_end(DvzBoard* board, DvzCommands* cmds, uint32_t idx);


/**
 * Allocate a CPU buffer for downloading the image.
 *
 * @param board the board
 */
DVZ_EXPORT uint8_t* dvz_board_alloc(DvzBoard* board);



/**
 * Free the allocated CPU buffer storing the image.
 *
 * @param board the board
 */
DVZ_EXPORT void dvz_board_free(DvzBoard* board);



/**
 * Download the current rendered image.
 *
 * @param board the board
 * @param size the image buffer size (should always be equal to width*height*4)
 * @param rgba an alread-allocated buffer that will contain the downloaded image
 */
DVZ_EXPORT void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgba);



/**
 * Destroy a board.
 *
 * @param board the board
 */
DVZ_EXPORT void dvz_board_destroy(DvzBoard* board);



EXTERN_C_OFF

#endif