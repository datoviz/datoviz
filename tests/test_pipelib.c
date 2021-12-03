/*************************************************************************************************/
/*  Testing pipelib                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_pipelib.h"
#include "board.h"
#include "pipe.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Pipelib tests                                                                                */
/*************************************************************************************************/

int test_pipelib_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    uvec2 size = {WIDTH, HEIGHT};

    // Create the board.
    DvzBoard board = dvz_board(gpu, WIDTH, HEIGHT);
    dvz_board_create(&board);

    // Create the pipelib.
    DvzPipelib lib = dvz_pipelib(gpu);

    // Create a graphics pipe.
    DvzPipe* pipe =
        dvz_pipelib_graphics(&lib, &board.renderpass, 1, size, DVZ_GRAPHICS_TRIANGLE, 0);

    // Destruction.
    // dvz_pipelib_pipe_destroy(&lib, pipe);
    dvz_pipelib_destroy(&lib);
    dvz_board_destroy(&board);
    dvz_context_destroy(ctx);
    return 0;
}
