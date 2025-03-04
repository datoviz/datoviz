/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing renderer                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_renderer.h"
#include "_map.h"
#include "canvas.h"
#include "datoviz.h"
#include "fileio.h"
#include "renderer.h"
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

int test_renderer_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create an offscreen canvas.
    req = dvz_create_canvas(
        batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR,
        DVZ_APP_FLAGS_OFFSCREEN | DVZ_CANVAS_FLAGS_PUSH_SCALE);
    DvzId canvas_id = req.id;

    // Canvas clear color.
    req = dvz_set_background(batch, canvas_id, (cvec4){32, 64, 128, 255});

    // Create a graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_TRIANGLE, DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN);
    DvzId graphics_id = req.id;

    // Canvas scale push constant shared by all shaders (common.glsl)
    dvz_set_push(batch, graphics_id, DVZ_SHADER_VERTEX | DVZ_SHADER_FRAGMENT, 0, sizeof(float));

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

    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    // dvz_show_base64(sizeof(mvp), &mvp);
    req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

    // Binding #1: viewport.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzId viewport_id = req.id;

    req = dvz_bind_dat(batch, graphics_id, 1, viewport_id, 0);

    DvzViewport viewport = {0};
    dvz_viewport_default(WIDTH, HEIGHT, &viewport);
    // dvz_show_base64(sizeof(viewport), &viewport);
    req = dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);

    // Commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, canvas_id);

    // Render.
    req = dvz_update_canvas(batch, canvas_id);

    // Submit the batch requests to the renderer.
    uint32_t count = dvz_batch_size(batch);
    AT(count > 0);
    DvzRequest* reqs = dvz_batch_requests(batch);
    AT(reqs != NULL);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, canvas_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_1.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Create a canvas deletion request.
    req = dvz_delete_canvas(batch, canvas_id);
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



int test_renderer_graphics(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create an offscreen canvas.
    req = dvz_create_canvas(
        batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR,
        DVZ_APP_FLAGS_OFFSCREEN | DVZ_CANVAS_FLAGS_PUSH_SCALE);
    DvzId canvas_id = req.id;

    // Canvas clear color.
    req = dvz_set_background(batch, canvas_id, (cvec4){32, 64, 128, 255});


    // Create a custom graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_CUSTOM, DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN);
    DvzId graphics_id = req.id;

    // Canvas scale push constant shared by all shaders (common.glsl)
    dvz_set_push(batch, graphics_id, DVZ_SHADER_VERTEX | DVZ_SHADER_FRAGMENT, 0, sizeof(float));

    // Load shaders.
    _load_shader(batch, graphics_id, DVZ_SHADER_VERTEX, "graphics_trivial_vert");
    _load_shader(batch, graphics_id, DVZ_SHADER_FRAGMENT, "graphics_trivial_frag");

    // Primitive topology.
    dvz_set_primitive(batch, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Polygon mode.
    dvz_set_polygon(batch, graphics_id, DVZ_POLYGON_MODE_FILL);

    // Vertex binding.
    dvz_set_vertex(batch, graphics_id, 0, sizeof(DvzVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(batch, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));
    dvz_set_attr(batch, graphics_id, 0, 1, DVZ_FORMAT_COLOR, offsetof(DvzVertex, color));

    // Descriptor dslots.
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

    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    // dvz_show_base64(sizeof(mvp), &mvp);
    req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

    // Binding #1: viewport.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzId viewport_id = req.id;
    req = dvz_bind_dat(batch, graphics_id, 1, viewport_id, 0);

    DvzViewport viewport = {0};
    dvz_viewport_default(WIDTH, HEIGHT, &viewport);
    // dvz_show_base64(sizeof(viewport), &viewport);
    req = dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);

    // Commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, canvas_id);

    // Render.
    req = dvz_update_canvas(batch, canvas_id);


    // Submit the batch requests to the renderer.
    uint32_t count = dvz_batch_size(batch);
    AT(count > 0);
    DvzRequest* reqs = dvz_batch_requests(batch);
    AT(reqs != NULL);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, canvas_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_graphics.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Create a canvas deletion request.
    req = dvz_delete_canvas(batch, canvas_id);

    // Destroy the requester and renderer.
    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    return 0;
}



int test_renderer_push(TstSuite* suite, TstItem* tstitem)
{
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create an offscreen canvas.
    req =
        dvz_create_canvas(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_APP_FLAGS_OFFSCREEN);
    DvzId canvas_id = req.id;

    // Canvas clear color.
    req = dvz_set_background(batch, canvas_id, (cvec4){32, 64, 128, 255});


    // Create a custom graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_CUSTOM, DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN);
    DvzId graphics_id = req.id;


    // Load shaders.
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/test_triangle_push.vert.spv", SPIRV_DIR);
    DvzSize shader_size = 0;
    uint32_t* shader_code = (uint32_t*)dvz_read_file(path, &shader_size);
    ASSERT(shader_size > 0);
    req =
        dvz_create_spirv(batch, DVZ_SHADER_VERTEX, shader_size, (const unsigned char*)shader_code);
    ASSERT(req.id != DVZ_ID_NONE);
    dvz_set_shader(batch, graphics_id, req.id);
    FREE(shader_code);

    snprintf(path, sizeof(path), "%s/test_triangle.frag.spv", SPIRV_DIR);
    shader_size = 0;
    shader_code = (uint32_t*)dvz_read_file(path, &shader_size);
    ASSERT(shader_size > 0);
    req = dvz_create_spirv(
        batch, DVZ_SHADER_FRAGMENT, shader_size, (const unsigned char*)shader_code);
    ASSERT(req.id != DVZ_ID_NONE);
    dvz_set_shader(batch, graphics_id, req.id);
    FREE(shader_code);


    // Primitive topology.
    dvz_set_primitive(batch, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Polygon mode.
    dvz_set_polygon(batch, graphics_id, DVZ_POLYGON_MODE_FILL);

    // Vertex binding.
    dvz_set_vertex(batch, graphics_id, 0, sizeof(DvzVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(batch, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));
    dvz_set_attr(batch, graphics_id, 0, 1, DVZ_FORMAT_COLOR, offsetof(DvzVertex, color));

    // Push constant.
    dvz_set_push(batch, graphics_id, DVZ_SHADER_VERTEX, 0, sizeof(float) + sizeof(vec4));
    dvz_set_push(batch, graphics_id, DVZ_SHADER_FRAGMENT, 0, sizeof(float));

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

    // Commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    // dvz_record_push(
    //     batch, canvas_id, graphics_id, DVZ_SHADER_VERTEX, 0, sizeof(vec3), (vec3){1, 1, 0});
    dvz_record_push(
        batch, canvas_id, graphics_id, DVZ_SHADER_VERTEX, sizeof(float), sizeof(vec3),
        (vec3){1, 1, 0});
    dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, canvas_id);

    // Render.
    req = dvz_update_canvas(batch, canvas_id);


    // Submit the batch requests to the renderer.
    uint32_t count = dvz_batch_size(batch);
    AT(count > 0);
    DvzRequest* reqs = dvz_batch_requests(batch);
    AT(reqs != NULL);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, canvas_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_push.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Create a canvas deletion request.
    req = dvz_delete_canvas(batch, canvas_id);

    // Destroy the requester and renderer.
    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    return 0;
}



int test_renderer_resize(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Create an offscreen canvas.
    req = dvz_create_canvas(
        batch, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, DVZ_APP_FLAGS_OFFSCREEN);
    DvzId canvas_id = req.id;
    dvz_renderer_request(rd, req);

    // Resize the canvas.
    req = dvz_resize_canvas(batch, canvas_id, WIDTH, HEIGHT);
    dvz_renderer_request(rd, req);

    // Check canvas resizing.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, canvas_id);
    AT(canvas->width == WIDTH);
    AT(canvas->height == HEIGHT);

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

    // NOTE: the canvas should be automatically destroyed when destroying the renderer.

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
