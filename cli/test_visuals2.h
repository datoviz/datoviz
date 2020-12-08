#include "../include/visky/visuals2.h"
#include "utils.h"


#define N_FRAMES 10



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

static void _marker_visual(VklVisual* visual)
{
    VklCanvas* canvas = visual->canvas;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS, 0));

    // Sources.
    {
        vkl_visual_source( //
            visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex));
        vkl_visual_source( //
            visual, VKL_SOURCE_UNIFORM, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP));
        vkl_visual_source(
            visual, VKL_SOURCE_UNIFORM, 1, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport));
        vkl_visual_source( //
            visual, VKL_SOURCE_TEXTURE, 0, VKL_PIPELINE_GRAPHICS, 0, 2, 0);
        vkl_visual_source(
            visual, VKL_SOURCE_UNIFORM, 2, VKL_PIPELINE_GRAPHICS, 0, 3,
            sizeof(VklGraphicsPointsParams));
    }

    // Props.
    {
        vkl_visual_prop(                                     //
            visual, VKL_PROP_POS, 0, VKL_SOURCE_VERTEX, 0,   //
            0, VKL_DTYPE_VEC3, offsetof(VklVertex, pos));    //
        vkl_visual_prop(                                     //
            visual, VKL_PROP_COLOR, 0, VKL_SOURCE_VERTEX, 0, //
            1, VKL_DTYPE_CVEC4, offsetof(VklVertex, color)); //
    }
}

static void _canvas_fill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklVisual* visual = (VklVisual*)ev.user_data;
    VklViewport viewport = vkl_viewport_full(canvas);

    // TODO: choose which of all canvas command buffers need to be filled with the visual
    // For now, update all of them.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        vkl_visual_fill_event(
            visual, ev.u.rf.clear_color, ev.u.rf.cmds[i], ev.u.rf.img_idx, viewport, NULL);
    }
}

static int vklite2_visuals_1(VkyTestContext* context)
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
    VklBufferRegions br_vert =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, N * sizeof(VklVertex));

    // Binding resources.
    VklBufferRegions br_mvp = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklMVP));
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

        // Upload MVP.
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        vkl_upload_buffers(ctx, br_mvp, 0, sizeof(VklMVP), &mvp);

        // Upload params.
        vkl_upload_buffers(ctx, br_params, 0, sizeof(VklGraphicsPointsParams), &params);
    }

    // Vertex data.
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    // TODO: remove below
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])

        memcpy(vertices[i].pos, pos[i], sizeof(pos[i]));
        memcpy(vertices[i].color, color[i], sizeof(color[i]));
    }

    // Set visual data.
    // TODO
    // vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    // vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    vkl_upload_buffers(ctx, br_vert, 0, N * sizeof(VklVertex), vertices);
    // HACK:
    visual.vertex_count = N;

    vkl_visual_buffer(&visual, VKL_SOURCE_VERTEX, 0, br_vert);

    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 0, br_mvp);
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 2, br_params);

    vkl_visual_texture(&visual, VKL_SOURCE_TEXTURE, 0, tex_color);

    // Upload the data to the GPU..
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(&visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, 0);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    FREE(vertices);
    TEST_END
}
