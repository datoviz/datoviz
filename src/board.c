/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "board.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzBoard dvz_board(DvzGpu* gpu, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzBoard board = {0};
    return board;
}



void dvz_board_format(DvzBoard* board, VkFormat format)
{
    ASSERT(board != NULL); //
}



void dvz_board_create(DvzBoard* board)
{
    ASSERT(board != NULL); //
}



void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height)
{
    ASSERT(board != NULL); //
}



void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board != NULL); //
}



void dvz_board_end(DvzBoard* board, DvzCommands* cmds)
{
    ASSERT(board != NULL);
    ASSERT(cmds != NULL);
}



void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgba)
{
    ASSERT(board != NULL);
    ASSERT(size > 0);
    ASSERT(rgba != NULL);
}



void dvz_board_destroy(DvzBoard* board)
{
    ASSERT(board != NULL); //
}
