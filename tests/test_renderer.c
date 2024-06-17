/*************************************************************************************************/
/*  Testing renderer                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_renderer.h"
#include "_map.h"
#include "board.h"
#include "fileio.h"
#include "renderer.h"
#include "scene/colormaps.h"
#include "scene/graphics.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void
_load_shader(DvzBatch* batch, DvzId graphics_id, DvzShaderType shader_type, const char* name)
{
    ANN(batch);
    ASSERT(graphics_id != 0);

    unsigned long shader_size = 0;
    unsigned char* shader_buffer = dvz_resource_shader(name, &shader_size);
    ASSERT(shader_size > 0);
    // // NOTE: we have to make a copy to ignore a clang alignment warning on macOS, unless there
    // // is a better solution?
    // uint32_t* shader_buffer_uint32 = (uint32_t*)malloc(shader_size);
    // memcpy(shader_buffer_uint32, shader_buffer, shader_size);

    DvzRequest req = dvz_create_spirv(batch, shader_type, shader_size, shader_buffer);
    ASSERT(req.id != DVZ_ID_NONE);

    dvz_set_shader(batch, graphics_id, req.id);
    // FREE(shader_buffer_uint32);
}



/*************************************************************************************************/
/*  Renderer tests                                                                               */
/*************************************************************************************************/

int test_renderer_1(TstSuite* suite)
{
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create a board.
    req = dvz_create_board(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;

    // Board clear color.
    req = dvz_set_background(batch, board_id, (cvec4){32, 64, 128, 255});

    // Create a graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_TRIANGLE, DVZ_REQUEST_FLAGS_OFFSCREEN);
    DvzId graphics_id = req.id;

    // Create the vertex buffer dat.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    DvzId dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_bind_vertex(batch, graphics_id, 0, dat_id, 0);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);

    // Binding #0: MVP.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    DvzId mvp_id = req.id;

    req = dvz_bind_dat(batch, graphics_id, 0, mvp_id, 0);

    DvzMVP mvp = dvz_mvp_default();
    // dvz_show_base64(sizeof(mvp), &mvp);
    req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

    // Binding #1: viewport.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzId viewport_id = req.id;

    req = dvz_bind_dat(batch, graphics_id, 1, viewport_id, 0);

    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    // dvz_show_base64(sizeof(viewport), &viewport);
    req = dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);

    // Commands.
    dvz_record_begin(batch, board_id);
    dvz_record_viewport(batch, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, board_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, board_id);

    // Render.
    req = dvz_update_board(batch, board_id);

    // Submit the batch requests to the renderer.
    uint32_t count = dvz_batch_size(batch);
    AT(count > 0);
    DvzRequest* reqs = dvz_batch_requests(batch);
    AT(reqs != NULL);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, board_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_1.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Create a board deletion request.
    req = dvz_delete_board(batch, board_id);
    dvz_renderer_request(rd, req);

    // Create a dat deletion request.
    req = dvz_delete_dat(batch, dat_id);
    dvz_renderer_request(rd, req);

    // Create a graphics deletion request.
    req = dvz_delete_graphics(batch, graphics_id);
    dvz_renderer_request(rd, req);

    // Destroy the requester and renderer.
    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    return 0;
}



int test_renderer_graphics(TstSuite* suite)
{
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create a boards.
    req = dvz_create_board(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;

    // Board clear color.
    req = dvz_set_background(batch, board_id, (cvec4){32, 64, 128, 255});


    // Create a custom graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_CUSTOM, DVZ_REQUEST_FLAGS_OFFSCREEN);
    DvzId graphics_id = req.id;

    // Load shaders.
    _load_shader(batch, graphics_id, DVZ_SHADER_VERTEX, "graphics_basic_vert");
    _load_shader(batch, graphics_id, DVZ_SHADER_FRAGMENT, "graphics_basic_frag");

    // Primitive topology.
    dvz_set_primitive(batch, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Polygon mode.
    dvz_set_polygon(batch, graphics_id, DVZ_POLYGON_MODE_FILL);

    // Vertex binding.
    dvz_set_vertex(batch, graphics_id, 0, sizeof(DvzVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(batch, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));
    dvz_set_attr(batch, graphics_id, 0, 1, DVZ_FORMAT_R8G8B8A8_UNORM, offsetof(DvzVertex, color));

    // Descriptor slots.
    dvz_set_slot(batch, graphics_id, 0, DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_set_slot(batch, graphics_id, 1, DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Create the vertex buffer dat.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    DvzId dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_bind_vertex(batch, graphics_id, 0, dat_id, 0);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);

    // Binding #0: MVP.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    DvzId mvp_id = req.id;
    req = dvz_bind_dat(batch, graphics_id, 0, mvp_id, 0);

    DvzMVP mvp = dvz_mvp_default();
    // dvz_show_base64(sizeof(mvp), &mvp);
    req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

    // Binding #1: viewport.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzId viewport_id = req.id;
    req = dvz_bind_dat(batch, graphics_id, 1, viewport_id, 0);

    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    // dvz_show_base64(sizeof(viewport), &viewport);
    req = dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);

    // Commands.
    dvz_record_begin(batch, board_id);
    dvz_record_viewport(batch, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, board_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, board_id);

    // Render.
    req = dvz_update_board(batch, board_id);


    // Submit the batch requests to the renderer.
    uint32_t count = dvz_batch_size(batch);
    AT(count > 0);
    DvzRequest* reqs = dvz_batch_requests(batch);
    AT(reqs != NULL);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, board_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_graphics.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Create a board deletion request.
    req = dvz_delete_board(batch, board_id);

    // Destroy the requester and renderer.
    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    return 0;
}



int test_renderer_resize(TstSuite* suite)
{
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create a board.
    req = dvz_create_board(batch, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    dvz_renderer_request(rd, req);

    // Resize the board.
    req = dvz_resize_board(batch, board_id, WIDTH, HEIGHT);
    dvz_renderer_request(rd, req);

    // Check board resizing.
    DvzBoard* board = dvz_renderer_board(rd, board_id);
    AT(board->width == WIDTH);
    AT(board->height == HEIGHT);

    // Create a dat.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    DvzId dat_id = req.id;
    dvz_renderer_request(rd, req);

    DvzSize size = 1024;
    uint8_t* data = (uint8_t*)calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i % 256;
    // NOTE: upload a buffer larger than the dat, checking that automatic resize will work.
    req = dvz_upload_dat(batch, dat_id, 0, size, data, 0);
    dvz_renderer_request(rd, req);
    FREE(data);

    // Check dat resizing.
    DvzDat* dat = dvz_renderer_dat(rd, dat_id);
    AT(dat->br.size == size);

    // Resize the dat.
    req = dvz_resize_dat(batch, dat_id, 2 * size);
    dvz_renderer_request(rd, req);

    // Check dat resizing.
    AT(dat->br.size == 2 * size);

    // Create a tex.
    req = dvz_create_tex(batch, DVZ_TEX_3D, DVZ_FORMAT_R32_UINT, (uvec3){2, 3, 4}, 0);
    dvz_renderer_request(rd, req);
    DvzId tex_id = req.id;

    // Create a sampler.
    req = dvz_create_sampler(batch, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    dvz_renderer_request(rd, req);
    DvzId sampler_id = req.id;

    // Resize the tex.
    req = dvz_resize_tex(batch, tex_id, (uvec3){20, 30, 40});
    dvz_renderer_request(rd, req);

    // Check tex resizing.
    DvzTex* tex = dvz_renderer_tex(rd, tex_id);
    AT(tex->shape[0] == 20);
    AT(tex->shape[1] == 30);
    AT(tex->shape[2] == 40);

    // NOTE: the board should be automatically destroyed when destroying the renderer.

    // Create a tex deletion request.
    req = dvz_delete_tex(batch, tex_id);
    dvz_renderer_request(rd, req);

    // Create a sampler deletion request.
    req = dvz_delete_sampler(batch, sampler_id);
    dvz_renderer_request(rd, req);

    // Destroy the requester and renderer.
    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    return 0;
}
