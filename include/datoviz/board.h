/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOARD
#define DVZ_HEADER_BOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "context.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBoard DvzBoard;

// Forward declarations.
typedef struct DvzRecorder DvzRecorder;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBoard
{
    DvzObject obj;
    DvzGpu* gpu;
    int flags;

    DvzFormat format;
    // cvec4 clear_color;
    uint32_t width, height;

    DvzSize size; // width*height*3
    uint8_t* rgb; // GPU buffer storing the image

    DvzImages images;
    DvzImages depth;
    DvzImages staging;
    DvzFramebuffers framebuffers;
    DvzRenderpass* renderpass;

    DvzCommands cmds;
    // TODO: picking

    DvzRecorder* recorder; // used to record command buffer when using the presenter
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
DvzBoard
dvz_board(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags);



/**
 * Set the board image format.
 *
 * @param board the board
 * @param format the image format
 */
void dvz_board_format(DvzBoard* board, DvzFormat format);



// /**
//  * Set the board clear color.
//  *
//  * @param board the board
//  * @param color the color as an array of 4 bytes
//  */
// void dvz_board_clear_color(DvzBoard* board, cvec4 color);



/**
 * Create the board after is has been set up.
 *
 * @param board the board
 */
void dvz_board_create(DvzBoard* board);



/**
 * Recreate the board.
 *
 * @param board the board
 */
void dvz_board_recreate(DvzBoard* board);



/**
 * Change the board width and height.
 *
 * @param board the board
 * @param width the width, in framebuffer pixels
 * @param height the height, in framebuffer pixels
 */
void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height);



/**
 * Start rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx);



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
    DvzBoard* board, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size);



/**
 * Stop rendering to the board in a command buffer.
 *
 * @param board the board
 * @param cmds the commands instance
 * @param idx the command buffer index with the commands instance
 */
void dvz_board_end(DvzBoard* board, DvzCommands* cmds, uint32_t idx);



/**
 * Allocate a CPU buffer for downloading the image.
 *
 * @param board the board
 */
uint8_t* dvz_board_alloc(DvzBoard* board);



/**
 * Free the allocated CPU buffer storing the image.
 *
 * @param board the board
 */
void dvz_board_free(DvzBoard* board);



/**
 * Download the current rendered image.
 *
 * @param board the board
 * @param size the image buffer size (should always be equal to width*height*4)
 * @param rgb an alread-allocated buffer that will contain the downloaded image
 */
void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgb);



/**
 * Destroy a board.
 *
 * @param board the board
 */
void dvz_board_destroy(DvzBoard* board);



EXTERN_C_OFF

#endif
