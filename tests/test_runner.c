/*************************************************************************************************/
/*  Testing runner                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_runner.h"
#include "_glfw.h"
#include "context.h"
#include "map.h"
#include "renderer.h"
#include "request.h"
#include "runner.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Runner tests                                                                                 */
/*************************************************************************************************/

int test_runner_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Renderer.
    DvzRenderer* rd = dvz_renderer(gpu);

    // Runner.
    DvzRunner* runner = dvz_runner(rd);

    // Requester.
    DvzRequester* rqr = runner->requester;
    DvzRequest req = {0};

    // Create a request.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, 0);
    ASSERT(req.id != DVZ_ID_NONE);
    dvz_runner_request(runner, req);

    // Runner loop.
    dvz_runner_loop(runner, N_FRAMES);

    // Destruction
    dvz_runner_destroy(runner);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);
    return 0;
}



int test_runner_2(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Renderer.
    DvzRenderer* rd = dvz_renderer(gpu);

    // Runner.
    DvzRunner* runner = dvz_runner(rd);

    // Requester.
    DvzRequester* rqr = runner->requester;
    DvzRequest req = {0};

    // Create a canvas.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, 0);
    DvzId canvas_0 = req.id;
    dvz_runner_request(runner, req);

    dvz_runner_loop(runner, 5);

    // Create another canvas.
    req = dvz_create_canvas(rqr, WIDTH / 2, HEIGHT / 2, 0);
    dvz_runner_request(runner, req);

    dvz_runner_loop(runner, 5);

    // Delete the first canvas.
    req = dvz_delete_canvas(rqr, canvas_0);
    dvz_runner_request(runner, req);

    dvz_runner_loop(runner, 5);

    // Destruction
    dvz_runner_destroy(runner);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);
    return 0;
}



int test_runner_triangle(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Renderer.
    DvzRenderer* rd = dvz_renderer(gpu);

    // Runner.
    DvzRunner* runner = dvz_runner(rd);

    // Requester.
    DvzRequester* rqr = runner->requester;
    DvzRequest req = {0};

    // Create a canvas.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, 0);
    DvzId canvas_id = req.id;
    dvz_runner_request(runner, req);

    // Create a graphics.
    req = dvz_create_graphics(
        rqr, canvas_id, DVZ_GRAPHICS_TRIANGLE,
        DVZ_PIPELIB_FLAGS_CREATE_MVP | DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);
    DvzId graphics_id = req.id;
    dvz_runner_request(runner, req);

    // Create the vertex buffer dat.
    req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    DvzId dat_id = req.id;
    dvz_runner_request(runner, req);

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_set_vertex(rqr, graphics_id, dat_id);
    dvz_runner_request(runner, req);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(rqr, dat_id, 0, sizeof(data), data);
    dvz_runner_request(runner, req);

    // Commands.
    dvz_requester_begin(rqr);
    dvz_requester_add(rqr, dvz_set_begin(rqr, canvas_id));
    dvz_requester_add(
        rqr, dvz_set_viewport(rqr, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT));
    dvz_requester_add(rqr, dvz_set_draw(rqr, canvas_id, graphics_id, 0, 3));
    dvz_requester_add(rqr, dvz_set_end(rqr, canvas_id));
    uint32_t count = 0;
    DvzRequest* reqs = dvz_requester_end(rqr, &count);
    AT(reqs != NULL);
    AT(count > 0);
    dvz_runner_request(runner, req);

    // Event loop.
    dvz_runner_loop(runner, N_FRAMES);

    // Destruction
    dvz_runner_destroy(runner);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);
    return 0;
}
