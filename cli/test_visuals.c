#include "test_visuals.h"
#include "../include/visky/visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef RANDN_POS
#define RANDN_POS(x)                                                                              \
    x[0] = .25 * randn();                                                                         \
    x[1] = .25 * randn();                                                                         \
    x[2] = .5;
#endif

#ifndef RAND_COLOR
#define RAND_COLOR(x)                                                                             \
    x[0] = rand_byte();                                                                           \
    x[1] = rand_byte();                                                                           \
    x[2] = rand_byte();                                                                           \
    x[3] = 255;
#endif



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_visuals_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    VklBufferRegions br_mvp =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, 1, sizeof(VklMVP));
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    VklBufferRegions br_params =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointsParams));
    VklTexture* tex_color = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Binding data.
    VklMVP mvp = {0};
    float param = 5.0f;
    VklGraphicsPointsParams params = {.point_size = param};
    {
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        vkl_upload_buffers(ctx, br_mvp, 0, sizeof(VklMVP), &mvp);

        // Upload params.
        vkl_upload_buffers(ctx, br_params, 0, sizeof(VklGraphicsPointsParams), &params);
    }

    // Vertex data.
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }

    // Set visual data.
    {
        // Via a GPU buffer.
        // VklBufferRegions br_vert =
        //     vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, N * sizeof(VklVertex));
        // vkl_upload_buffers(ctx, br_vert, 0, N * sizeof(VklVertex), vertices);
        // visual.vertex_count = N;
        // vkl_visual_buffer(&visual, VKL_SOURCE_VERTEX, 0, br_vert);

        // Via user-provided data (underlying vertex buffer created automatically).
        vkl_visual_data_buffer(&visual, VKL_SOURCE_VERTEX, 0, 0, N, N, vertices);
    }

    // Set uniform buffers.
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 0, br_mvp);
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 2, br_params);

    vkl_visual_texture(&visual, VKL_SOURCE_TEXTURE_2D, 0, tex_color);

    // Upload the data to the GPU.
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(&visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(vertices);
    TEST_END
}



int test_visuals_2(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);

    // Vertex data.
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 5.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // Color texture.
    cvec4* colormaps = calloc(16 * 16, sizeof(cvec4));
    for (uint32_t i = 0; i < 16; i++)
        for (uint32_t j = 0; j < 16; j++)
        {
            colormaps[16 * i + j][0] = i * 16;
            colormaps[16 * i + j][1] = j * 16;
            colormaps[16 * i + j][3] = 255;
        }
    vkl_visual_data_texture(&visual, VKL_PROP_COLOR_TEXTURE, 0, 16, 16, 1, colormaps);

    // GPU bindings.
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 1, br_viewport);

    // Upload the data to the GPU..
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(&visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    FREE(colormaps);
    TEST_END
}
