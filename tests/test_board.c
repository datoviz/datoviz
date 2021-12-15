/*************************************************************************************************/
/*  Testing board                                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_board.h"
#include "board.h"
#include "fileio.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_board_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Create the board.
    DvzBoard board = dvz_board(gpu, WIDTH, HEIGHT, 0);
    dvz_board_create(&board);

    // Create the graphics.
    DvzGraphics graphics = triangle_graphics(gpu, &board.renderpass, "");

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);

    // Create the buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    DvzSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(
        &buffer,                                 //
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |      //
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_vma_usage(&buffer, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_buffer_create(&buffer);
    DvzBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};

    // Upload the triangle data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Command buffer.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    triangle_commands(&cmds, 0, &board.renderpass, &board.framebuffers, &graphics, &bindings, br);

    // Render.
    dvz_cmd_submit_sync(&cmds, 0);

    // Retrieve the rendered image.
    uint8_t* rgb = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgb);
    // Save it to a file.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/board.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    dvz_board_free(&board);

    // Destruction.
    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);
    dvz_buffer_destroy(&buffer);

    dvz_board_destroy(&board);
    dvz_context_destroy(ctx);

    return 0;
}
