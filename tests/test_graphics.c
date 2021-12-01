/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "mesh.h"
// #include "../src/interact_utils.h"
// #include "proto.h"

#include "test_graphics.h"
#include "context.h"
#include "fileio.h"
#include "graphics.h"
#include "host.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestGraphics
{
    // DvzCanvas* canvas;
    DvzGraphics* graphics;
    DvzCompute* compute;

    DvzBufferRegions br_vert;
    DvzBufferRegions br_vert_comp;
    DvzBufferRegions br_index;
    DvzBufferRegions br_mvp;
    DvzBufferRegions br_viewport;
    DvzBufferRegions br_params;
    DvzBufferRegions br_comp;

    // DvzTexture* texture;
    DvzBindings bindings;
    DvzBindings bindings_comp;

    // DvzInteract interact;
    // DvzGraphicsData graphics_data;

    uint32_t item_count;
    uvec3 n_vert_comp;
    DvzArray vertices;
    DvzArray indices;

    // float param;
    void* data;
    void* params_data;
};



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TstSuite* suite)
{
    ASSERT(suite != NULL);

    // Host.
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);

    // GPU.
    DvzGpu* gpu = dvz_gpu_best(host);
    // Specify the default queues.
    _default_queues(gpu, false);

    // Default features
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});

    // dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);


    // // Context.
    // DvzContext* ctx = dvz_context(gpu);
    // ASSERT(ctx != NULL);


    // TestCanvas canvas = offscreen(gpu);
    // TestVisual visual = triangle_visual(gpu, &canvas.renderpass, &canvas.framebuffers, "");
    // visual.br.buffer = &visual.buffer;
    // visual.br.size = visual.buffer.size;
    // visual.br.count = 1;
    // canvas.data = &visual;
    // canvas.br = visual.br;
    // ASSERT(canvas.br.buffer->buffer != VK_NULL_HANDLE);
    // canvas.graphics = &visual.graphics;
    // canvas.bindings = &visual.bindings;

    // DvzCommands cmds = dvz_commands(gpu, 0, 1);
    // triangle_commands(
    //     &cmds, 0, &canvas.renderpass, &canvas.framebuffers, //
    //     canvas.graphics, canvas.bindings, canvas.br);
    // dvz_cmd_submit_sync(&cmds, 0);

    // char path[1024];
    // snprintf(path, sizeof(path), "%s/screenshot.ppm", ARTIFACTS_DIR);

    // log_debug("saving screenshot to %s", path);
    // // Make a screenshot of the color attachment.
    // DvzImages* images = visual.framebuffers->attachments[0];
    // uint8_t* rgba = (uint8_t*)screenshot(images, 1);
    // dvz_write_ppm(path, images->shape[0], images->shape[1], rgba);
    // FREE(rgba);

    // destroy_visual(&visual);
    // test_canvas_destroy(&canvas);


    // dvz_context_destroy(ctx);

    dvz_gpu_destroy(gpu);
    dvz_host_destroy(host);

    return 0;
}
