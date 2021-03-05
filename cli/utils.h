#ifndef DVZ_TEST_UTILS_HEADER
#define DVZ_TEST_UTILS_HEADER

#include "../include/datoviz/datoviz.h"
#include "../src/vklite_utils.h"

BEGIN_INCL_NO_WARN
#include "../external/stb_image.h"
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// #if !OS_MACOS
// #define WIDTH  1280
// #define HEIGHT 960
// #else
#define WIDTH  800
#define HEIGHT 600
// #endif

// Pass N = FPS * DURATION frames before taking a test screenshot
#define FPS          60
#define DURATION     1
#define MAX_RETRIES  10
#define CANVAS_FLAGS (getenv("DVZ_FPS") != 0 ? DVZ_CANVAS_FLAGS_FPS : 0)

#define IMAGE_RELPATH "images/tests"

#define NORM3_255       (1. / (3 * 255.0 * 255.0))
#define NORM3_THRESHOLD 1e-5

#define TEST_WIDTH  WIDTH
#define TEST_HEIGHT HEIGHT

static const VkClearColorValue bgcolor = {{.4f, .6f, .8f, 1.0f}};
#define TEST_FORMAT       VK_FORMAT_B8G8R8A8_UNORM
#define TEST_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
// #define TEST_PRESENT_MODE VK_PRESENT_MODE_IMMEDIATE_KHR

#define MOUSE_VOLUME_WIDTH  320
#define MOUSE_VOLUME_HEIGHT 456
#define MOUSE_VOLUME_DEPTH  528

#define N_FRAMES (getenv("DVZ_INTERACT") != NULL ? 0 : 10)



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCase TestCase;
typedef struct TestContext TestContext;

typedef struct TestVertex TestVertex;
typedef struct TestCanvas TestCanvas;
typedef struct TestVisual TestVisual;
typedef struct TestScene TestScene;

// Test cases callbacks.
typedef int (*TestFunction)(TestContext*);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// TestFixture
typedef enum
{
    DVZ_TEST_FIXTURE_NONE,
    DVZ_TEST_FIXTURE_CANVAS,
    DVZ_TEST_FIXTURE_PANEL,
} TestFixture;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestVertex
{
    vec3 pos;
    vec4 color;
};



struct TestCanvas
{
    DvzGpu* gpu;
    bool is_offscreen;

    DvzWindow* window;

    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
    DvzSwapchain swapchain;

    DvzImages* images;
    DvzImages* depth;

    DvzCompute* compute;
    DvzBindings* bindings;
    DvzGraphics* graphics;
    DvzBufferRegions br;

    void* data;
};



struct TestVisual
{
    DvzGpu* gpu;
    DvzRenderpass* renderpass;
    DvzFramebuffers* framebuffers;
    DvzGraphics graphics;
    DvzCompute* compute;
    DvzBindings bindings;
    DvzBuffer buffer;
    DvzBufferRegions br;
    DvzBufferRegions br_u;
    uint32_t n_vertices;
    float dt;
    void* data;
    void* data_u;
    void* user_data;
};



struct TestScene
{
    DvzMouse mouse;
    DvzKeyboard keyboard;
    DvzInteract interact;
    DvzVisual visual;
    DvzGrid* grid;
};



struct TestContext
{
    DvzApp* app;
    // DvzCanvas* canvas;
    // DvzScene* scene;
    // DvzPanel* panel;
    // DvzScreenshot* screenshot;
    // bool is_live;
};



struct TestCase
{
    const char* name;
    TestFixture fixture;
    TestFunction function;
    TestFunction destroy;
    bool save_screenshot;
};



typedef void (*FillCallback)(TestCanvas*, DvzCommands*, uint32_t);



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

#if 0
static bool file_exists(const char* path) { return access(path, F_OK) != -1; }

static int image_diff(const uint8_t* image_0, const char* path)
{
    int w = 0, h = 0;
    uint8_t* image_1 = dvz_read_ppm(path, &w, &h);
    ASSERT(w == WIDTH && h == HEIGHT);

    // Fast byte-to-byte comparison of the images.
    if (memcmp(image_0, image_1, (size_t)(WIDTH * HEIGHT * 3 * sizeof(uint8_t))) == 0)
        return 0;
    log_debug("images were not byte-to-byte equivalent: computing the error distance");

    uint8_t rgb0[3], rgb1[3];
    double err = 0.0;
    for (uint32_t i = 0; i < HEIGHT; i++)
    {
        for (uint32_t j = 0; j < WIDTH; j++)
        {
            memcpy(rgb0, &image_0[3 * i * WIDTH + 3 * j], sizeof(rgb0));
            memcpy(rgb1, &image_1[3 * i * WIDTH + 3 * j], sizeof(rgb1));
            // Fast byte-to-byte comparison of the RGB values for that pixel.
            if (memcmp(rgb0, rgb1, sizeof(rgb0)) != 0)
            {
                err += ((rgb1[0] - rgb0[0]) * (rgb1[0] - rgb0[0]) +
                        (rgb1[1] - rgb0[1]) * (rgb1[1] - rgb0[1]) +
                        (rgb1[2] - rgb0[2]) * (rgb1[2] - rgb0[2])) *
                       NORM3_255;
            }
        }
    }
    err /= (WIDTH * HEIGHT);
    log_debug("image diff was %.20f", err);
    FREE(image_1);
    return err < NORM3_THRESHOLD ? 0 : 1;
}

static int write_image(const char* path, const uint8_t* rgb)
{
    int res = 0;
    res = dvz_write_ppm(path, WIDTH, HEIGHT, rgb);
    if (res != 0)
    {
        log_error("failed writing to %s", path);
    }
    return res;
}

static void get_image_path(const char* name, const char* ext, char* out)
{
    snprintf(out, 1024, "%s/%s/%s%s", ROOT_DIR, IMAGE_RELPATH, name, ext);
}

static int compare_images(const char* name, const uint8_t* rgb)
{
    // Get the paths to the screenshots, depending on the test case name.
    char path[1024];
    char path_failed[1024];
    get_image_path(name, ".ppm", path);
    get_image_path(name, ".failed.ppm", path_failed);

    // If the file doesn't exist, return 0.
    if (!file_exists(path))
    {
        log_debug("file %s didn't exist, so create it and mark the test as passing", path);
        write_image(path, rgb);
        return 0;
    }

    // Compare the saved file with the screenshot buffer.
    int res = image_diff(rgb, path);

    // Test failed: we write the failed screenshot with a different filename.
    if (res != 0)
    {
        log_debug("image comparison failed for %s, writing the failed output", name);
        write_image(path_failed, rgb);
    }
    else if (file_exists(path_failed))
    {
        log_debug("image comparison succeeded, deleting old failed image %s", path_failed);
        remove(path_failed);
    }

    return res;
}

static bool is_blank(uint8_t* image)
{
    // Make sure the image is not all black.
    void* black = calloc(WIDTH * HEIGHT * 3, sizeof(uint8_t));
    if (memcmp(image, black, (size_t)(WIDTH * HEIGHT * 3 * sizeof(uint8_t))) == 0)
    {
        FREE(black);
        return true;
    }
    return false;
}

static uint8_t* make_screenshot(TestContext* context)
{
    ASSERT(context != NULL);
    // ASSERT(context->canvas != NULL);
    // // NOTE: the caller must free the output buffer
    // if (context->screenshot == NULL)
    //     context->screenshot = dvz_create_screenshot(context->canvas);
    // dvz_begin_screenshot(context->screenshot);
    // uint8_t* rgb = dvz_screenshot_to_rgb(context->screenshot, false);
    // dvz_end_screenshot(context->screenshot);
    // return rgb;
    return NULL;
}

static void run_canvas(DvzCanvas* canvas)
{
    // Run one frame of the example.
    dvz_fill_command_buffers(canvas);

    // TODO: multiple frames before screenshot, mock input etc
    if (canvas->is_offscreen)
        dvz_offscreen_frame(canvas, 0);
    else
        dvz_run_app(canvas->app);

    // for (double t = 0; t < frame_count / (float)FPS; t += (1. / FPS))
    // {
    //     dvz_offscreen_frame(canvas, t);
    // }
}

#endif

static void print_start()
{
    printf("--- Starting tests -------------------------------\n"); //
}

static void print_case(int index, const char* name)
{
    printf("- Launching test #%03d %28s\n", index, name);
}

static void print_res(int index, const char* name, int res)
{
    printf("%50s", name);
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}

static void print_end(int index, int res)
{
    printf("--------------------------------------------------\n");
    if (index > 0 && res == 0)
        printf("\x1b[32m%d/%d tests PASSED.\x1b[0m\n", index, index);
    else if (index > 0)
        printf("\x1b[31m%d/%d tests FAILED.\x1b[0m\n", res, index);
    else
        printf("\x1b[31mThere were no tests.\x1b[0m\n");
}



/*************************************************************************************************/
/*  Testing infrastructure                                                                       */
/*************************************************************************************************/

#if 0
static void _setup(TestContext* context, TestFixture fixture)
{
    ASSERT(context != NULL);

    if (fixture >= DVZ_TEST_FIXTURE_CANVAS)
    {
        if (context->app == NULL)
        {
            log_debug("fixture setup: create the app");
            context->app =
                dvz_create_app(context->is_live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN, NULL);
        }
        ASSERT(context->app != NULL);
        if (context->canvas == NULL)
        {
            log_debug("fixture setup: create the canvas");
            context->canvas = dvz_create_canvas(context->app, WIDTH, HEIGHT);
            // Create large GPU buffers that will be cleared after each test.
            dvz_add_vertex_buffer(context->canvas->gpu, 1e6);
            dvz_add_index_buffer(context->canvas->gpu, 1e6);
        }

        ASSERT(context->canvas != NULL);
    }

    if (fixture >= DVZ_TEST_FIXTURE_PANEL)
    {
        ASSERT(context->canvas != NULL);
        if (context->scene == NULL)
        {
            log_debug("fixture setup: create the scene");
            context->scene = dvz_create_scene(context->canvas, DVZ_CLEAR_COLOR_WHITE, 1, 1);
        }
        ASSERT(context->scene != NULL);
        if (context->panel == NULL)
        {
            log_debug("fixture setup: create the panel");
            context->panel = dvz_get_panel(context->scene, 0, 0);
        }

        ASSERT(context->panel != NULL);
    }
}

static void _teardown(TestContext* context, TestFixture fixture)
{
    ASSERT(context != NULL);
    // NOTE: do not try to reset the canvas when is_live is true, because there is
    // only one canvas so it doesn't make sense, and it would cause a segfault
    // as the canvas is destroyed as soon as it is closed.
    if (fixture >= DVZ_TEST_FIXTURE_CANVAS && !context->is_live)
    {
        ASSERT(context->canvas != NULL);
        log_debug("fixture teardown: reset the canvas");
        dvz_reset_canvas(context->canvas);
        ASSERT(context->canvas->gpu != NULL);
        dvz_clear_all_buffers(context->canvas->gpu);
        dvz_reset_all_constants();
    }
    if (fixture >= DVZ_TEST_FIXTURE_PANEL)
    {
        log_debug("fixture teardown: destroy the scene");
        dvz_destroy_scene(context->canvas->scene);
        context->scene = NULL;
        context->panel = NULL;
    }
}

static TestContext _create_context(bool is_live)
{
    TestContext context = {0};
    context.is_live = is_live;
    return context;
}

static void _destroy_context(TestContext* context)
{
    ASSERT(context != NULL);

    if (context->screenshot != NULL)
    {
        dvz_destroy_screenshot(context->screenshot);
        context->screenshot = NULL;
    }

    if (context->app != NULL)
    {
        dvz_destroy_app(context->app);
        context->app = NULL;
    }
}
#endif



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CASE_FIXTURE_NONE(func)                                                                   \
    {                                                                                             \
#func, DVZ_TEST_FIXTURE_NONE, func, NULL, false                                           \
    }

#define CASE_FIXTURE_CANVAS(func, func_destroy, screenshot)                                       \
    {                                                                                             \
#func, DVZ_TEST_FIXTURE_CANVAS, func, func_destroy, screenshot                            \
    }

#define CASE_FIXTURE_PANEL(func, screenshot)                                                      \
    {                                                                                             \
#func, DVZ_TEST_FIXTURE_PANEL, func, NULL, screenshot                                     \
    }

// #define CASE(func, fixture, save_screenshot)
//     {
// #func, fixture, func, NULL, save_screenshot
//     }

// #define CASE_DESTROY(func, func_destroy, save_screenshot)
//     {
// #func, fixture, func, func_destroy, save_screenshot
//     }

#define SWITCH_CLI_ARG(arg)                                                                       \
    if (argc >= 1 && strcmp(argv[1], #arg) == 0)                                                  \
        res = arg(argc - 1, &argv[1]);

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        log_error("assertion '%s' failed", #x);                                                   \
        return 1;                                                                                 \
    }

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN((x - y), -eps, +eps)

#define ABOX(x, a, b, c, d)                                                                       \
    AT(((x).pos_ll[0] == (a)) && ((x).pos_ll[1] == (b)) && ((x).pos_ur[0] == (c)) &&              \
       ((x).pos_ur[1] == (d)))

#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);

#define TEST_END return dvz_app_destroy(app);

#define RANDN_POS(x)                                                                              \
    x[0] = .25 * dvz_rand_normal();                                                               \
    x[1] = .25 * dvz_rand_normal();                                                               \
    x[2] = .25 * dvz_rand_normal();

#define RAND_COLOR(x)                                                                             \
    x[0] = dvz_rand_byte();                                                                       \
    x[1] = dvz_rand_byte();                                                                       \
    x[2] = dvz_rand_byte();                                                                       \
    x[3] = 255;



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzRenderpass _renderpass(
    DvzGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, VkImageLayout layout)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    dvz_renderpass_clear(&renderpass, clear_color);
    dvz_renderpass_clear(&renderpass, clear_depth);

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(&renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    dvz_renderpass_attachment(
        &renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);
    dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    dvz_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_renderpass_subpass_dependency_access(
        &renderpass, 0, 0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
depth_image(DvzImages* depth_images, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    dvz_images_format(depth_images, renderpass->attachments[1].format);
    dvz_images_size(depth_images, width, height, 1);
    dvz_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth_images, 0);
    dvz_images_create(depth_images);
}



static TestCanvas offscreen(DvzGpu* gpu)
{
    TestCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.is_offscreen = true;

    canvas.renderpass =
        _renderpass(gpu, bgcolor, TEST_FORMAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Color attachment
    DvzImages images_struct = dvz_images(canvas.renderpass.gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* images = (DvzImages*)calloc(1, sizeof(DvzImages));
    *images = images_struct;
    dvz_images_format(images, canvas.renderpass.attachments[0].format);
    dvz_images_size(images, TEST_WIDTH, TEST_HEIGHT, 1);
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_queue_access(images, 0);
    dvz_images_create(images);
    canvas.images = images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, TEST_WIDTH, TEST_HEIGHT);
    canvas.depth = depth;

    // Create renderpass.
    dvz_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static TestCanvas glfw_canvas(DvzGpu* gpu, DvzWindow* window)
{
    TestCanvas canvas = {0};
    canvas.is_offscreen = false;
    canvas.gpu = gpu;
    canvas.window = window;

    uint32_t framebuffer_width, framebuffer_height;
    dvz_window_get_size(window, &framebuffer_width, &framebuffer_height);
    ASSERT(framebuffer_width > 0);
    ASSERT(framebuffer_height > 0);

    DvzRenderpass renderpass =
        _renderpass(gpu, bgcolor, TEST_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    canvas.renderpass = renderpass;

    canvas.swapchain = dvz_swapchain(canvas.renderpass.gpu, window, 3);
    dvz_swapchain_format(&canvas.swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    dvz_swapchain_present_mode(&canvas.swapchain, TEST_PRESENT_MODE);
    dvz_swapchain_create(&canvas.swapchain);
    canvas.images = canvas.swapchain.images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, canvas.images->width, canvas.images->height);
    canvas.depth = depth;

    // Create renderpass.
    dvz_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, canvas.swapchain.images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static void* screenshot(DvzImages* images, VkDeviceSize bytes_per_component)
{
    // NOTE: the caller must free the output

    DvzGpu* gpu = images->gpu;

    // Create the staging image.
    log_debug("starting creation of staging image");
    DvzImages staging_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* staging = (DvzImages*)calloc(1, sizeof(DvzImages));
    *staging = staging_struct;
    dvz_images_format(staging, images->format);
    dvz_images_size(staging, images->width, images->height, images->depth);
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_images_memory(
        staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_create(staging);

    // Start the image transition command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);

    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, staging);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, images, staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    void* rgb = calloc(images->width * images->height, 3 * bytes_per_component);
    dvz_images_download(staging, 0, bytes_per_component, true, false, rgb);

    dvz_images_destroy(staging);

    return rgb;
}



static void save_screenshot(DvzFramebuffers* framebuffers, const char* path)
{
    log_debug("saving screenshot to %s", path);
    // Make a screenshot of the color attachment.
    DvzImages* images = framebuffers->attachments[0];
    uint8_t* rgba = (uint8_t*)screenshot(images, 1);
    dvz_write_ppm(path, images->width, images->height, rgba);
    FREE(rgba);
}



static void show_canvas(TestCanvas canvas, FillCallback fill_commands, uint32_t n_frames)
{
    DvzGpu* gpu = canvas.gpu;
    DvzWindow* window = canvas.window;
    DvzRenderpass* renderpass = &canvas.renderpass;
    DvzFramebuffers* framebuffers = &canvas.framebuffers;
    DvzSwapchain* swapchain = &canvas.swapchain;

    ASSERT(swapchain != NULL);
    ASSERT(swapchain->img_count > 0);

    DvzCommands cmds = dvz_commands(gpu, 0, swapchain->img_count);
    for (uint32_t i = 0; i < cmds.count; i++)
        fill_commands(&canvas, &cmds, i);

    // Sync objects.
    DvzSemaphores sem_img_available = dvz_semaphores(gpu, DVZ_MAX_FRAMES_IN_FLIGHT);
    DvzSemaphores sem_render_finished = dvz_semaphores(gpu, DVZ_MAX_FRAMES_IN_FLIGHT);
    DvzFences fences = dvz_fences(gpu, DVZ_MAX_FRAMES_IN_FLIGHT, true);
    DvzFences bak_fences = {0};
    bak_fences.gpu = gpu;
    bak_fences.count = swapchain->img_count;
    uint32_t cur_frame = 0;
    DvzBackend backend = DVZ_BACKEND_GLFW;

    for (uint32_t frame = 0; frame < n_frames; frame++)
    {
        log_debug("iteration %d", frame);

        glfwPollEvents();

        if (backend_window_should_close(backend, window->backend_window) ||
            window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
            break;

        // Wait for fence.
        dvz_fences_wait(&fences, cur_frame);

        // We acquire the next swapchain image.
        dvz_swapchain_acquire(swapchain, &sem_img_available, cur_frame, NULL, 0);
        if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
        {
            dvz_gpu_wait(gpu);
            break;
        }
        // Handle resizing.
        else if (swapchain->obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
        {
            log_trace("recreating the swapchain");

            // Wait until the device is ready and the window fully resized.
            // Framebuffer new size.
            uint32_t width, height;
            backend_window_get_size(
                backend, window->backend_window, //
                &window->width, &window->height, //
                &width, &height);
            dvz_gpu_wait(gpu);

            // Destroy swapchain resources.
            dvz_framebuffers_destroy(framebuffers);
            dvz_images_destroy(canvas.depth);
            dvz_images_destroy(canvas.images);
            dvz_swapchain_destroy(swapchain);

            // Recreate the swapchain. This will automatically set the swapchain->images new
            // size.
            dvz_swapchain_create(swapchain);
            // Find the new framebuffer size as determined by the swapchain recreation.
            width = swapchain->images->width;
            height = swapchain->images->height;

            // The instance should be the same.
            ASSERT(swapchain->images == canvas.images);

            // Need to recreate the depth image with the new size.
            dvz_images_size(canvas.depth, width, height, 1);
            dvz_images_create(canvas.depth);

            // Recreate the framebuffers with the new size.
            ASSERT(framebuffers->attachments[0]->width == width);
            ASSERT(framebuffers->attachments[0]->height == height);
            dvz_framebuffers_create(framebuffers, renderpass);

            // Need to refill the command buffers.
            for (uint32_t i = 0; i < cmds.count; i++)
            {
                dvz_cmd_reset(&cmds, i);
                fill_commands(&canvas, &cmds, i);
            }
        }
        else
        {
            dvz_fences_copy(&fences, cur_frame, &bak_fences, swapchain->img_idx);

            // Then, we submit the cmds on that image
            DvzSubmit submit = dvz_submit(gpu);
            dvz_submit_commands(&submit, &cmds);
            dvz_submit_wait_semaphores(
                &submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, &sem_img_available,
                cur_frame);
            // Once the render is finished, we signal another semaphore.
            dvz_submit_signal_semaphores(&submit, &sem_render_finished, cur_frame);
            dvz_submit_send(&submit, swapchain->img_idx, &fences, cur_frame);

            // Once the image is rendered, we present the swapchain image.
            dvz_swapchain_present(swapchain, 1, &sem_render_finished, cur_frame);

            cur_frame = (cur_frame + 1) % DVZ_MAX_FRAMES_IN_FLIGHT;
        }

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present.
        dvz_queue_wait(gpu, 1);
    }
    log_trace("end of main loop");
    dvz_gpu_wait(gpu);

    dvz_semaphores_destroy(&sem_img_available);
    dvz_semaphores_destroy(&sem_render_finished);
    dvz_fences_destroy(&fences);
}



static void destroy_canvas(TestCanvas* canvas)
{
    log_trace("destroy canvas");
    if (canvas->is_offscreen)
    {
        dvz_images_destroy(canvas->images);
    }
    dvz_images_destroy(canvas->depth);

    dvz_renderpass_destroy(&canvas->renderpass);
    dvz_swapchain_destroy(&canvas->swapchain);
    dvz_framebuffers_destroy(&canvas->framebuffers);
    dvz_window_destroy(canvas->window);
}



static void _triangle_graphics(TestVisual* visual, const char* suffix)
{
    DvzGpu* gpu = visual->gpu;
    visual->graphics = dvz_graphics(gpu);
    ASSERT(visual->renderpass != NULL);
    DvzGraphics* graphics = &visual->graphics;
    visual->n_vertices = 3;

    dvz_graphics_renderpass(graphics, visual->renderpass, 0);
    dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle%s.vert.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle%s.frag.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(graphics, 0, sizeof(TestVertex));
    dvz_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));
}



static void _depth_vertices(uint32_t N, DvzGraphicsMeshVertex* vertices, bool vulkan_transform)
{
    float x = 0;
    float y = 0;
    float l = .075;
    float z = 0;
    DvzGraphicsMeshVertex *v0, *v1, *v2;
    uint32_t j = 0;
    float col = (0.5) / 256.0;
    vec2 z_values = {.75, .25};
    if (vulkan_transform)
    {
        z_values[0] = -.25;
        z_values[1] = +.25;
    }
    for (uint32_t i = 0; i < N; i++)
    {
        v0 = &vertices[3 * i + 0];
        v1 = &vertices[3 * i + 1];
        v2 = &vertices[3 * i + 2];

        x = .75 * (-1 + 2 * dvz_rand_float());
        y = .75 * (-1 + 2 * dvz_rand_float());

        // The following should work even if the depth buffer is not working.
        // j = i < N / 6 ? 0 : 1;

        // The following checks the depth buffer.
        j = i % 2;

        // grey background, color foreground
        // j == 0 : background, j == 1 : foreground
        // NOTE: no Vulkan transformation, use native Vulkan z coordinate, 0 = front, 1 = back
        z = z_values[j % 2]; // j == 0, .75 = background, .25 = foreground (if no vulkan_transform)
        z += .01 * dvz_rand_normal();

        v0->pos[0] = x - l;
        v0->pos[1] = y - l;
        v0->pos[2] = z;
        v0->uv[0] = 0.00;
        v0->uv[1] = col + 1 * j / 256.0;
        v0->normal[2] = 1;
        v0->alpha = 255;

        v1->pos[0] = x + l;
        v1->pos[1] = y - l;
        v1->pos[2] = z;
        v1->uv[0] = 0.50;
        v1->uv[1] = col + 1 * j / 256.0;
        v1->normal[2] = 1;
        v1->alpha = 255;

        v2->pos[0] = x + 0;
        v2->pos[1] = y + l;
        v2->pos[2] = z;
        v2->uv[0] = 1.00;
        v2->uv[1] = col + 1 * j / 256.0;
        v2->normal[2] = 1;
        v2->alpha = 255;
    }
}



static void _triangle_buffer(TestVisual* visual)
{
    DvzGpu* gpu = visual->gpu;

    // Create the buffer.
    visual->buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&visual->buffer, size);
    dvz_buffer_usage(
        &visual->buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_memory(
        &visual->buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_create(&visual->buffer);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    dvz_buffer_upload(&visual->buffer, 0, size, data);

    visual->br.buffer = &visual->buffer;
    visual->br.size = size;
    visual->br.count = 1;
}



static void test_triangle(TestVisual* visual, const char* suffix)
{
    _triangle_graphics(visual, suffix);

    // Create the bindings.
    visual->bindings = dvz_bindings(&visual->graphics.slots, 1);
    dvz_bindings_update(&visual->bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual->graphics);

    _triangle_buffer(visual);
}



static void destroy_visual(TestVisual* visual)
{
    dvz_graphics_destroy(&visual->graphics);
    dvz_bindings_destroy(&visual->bindings);
    dvz_buffer_destroy(&visual->buffer);
}



static DvzTexture* _mouse_volume(DvzCanvas* canvas)
{
    DvzGpu* gpu = canvas->gpu;

    const uint32_t ni = MOUSE_VOLUME_WIDTH;
    const uint32_t nj = MOUSE_VOLUME_HEIGHT;
    const uint32_t nk = MOUSE_VOLUME_DEPTH;

    // Texture.
    char path[1024];
    snprintf(path, sizeof(path), "%s/volume/%s", DATA_DIR, "atlas_25.img");
    DvzTexture* texture =
        dvz_ctx_texture(gpu->context, 3, (uvec3){ni, nj, nk}, VK_FORMAT_R16_UNORM);
    // WARNING: nearest filter causes visual artifacts when sampling from a 3D texture close to the
    // boundaries between different values
    dvz_texture_filter(texture, DVZ_FILTER_MAG, VK_FILTER_LINEAR);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    uint16_t* tex_data = (uint16_t*)dvz_read_file(path, NULL);
    for (uint32_t i = 0; i < (ni * nj * nk); i++)
        tex_data[i] *= 10;
    dvz_texture_upload(
        texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, //
        ni * nj * nk * sizeof(uint16_t), tex_data);
    FREE(tex_data);
    return texture;
}



static DvzTexture* _mouse_label(DvzCanvas* canvas)
{
    DvzGpu* gpu = canvas->gpu;

    const uint32_t ni = MOUSE_VOLUME_WIDTH;
    const uint32_t nj = MOUSE_VOLUME_HEIGHT;
    const uint32_t nk = MOUSE_VOLUME_DEPTH;

    // Texture.
    char path[1024];
    snprintf(path, sizeof(path), "%s/volume/%s", DATA_DIR, "atlas_25_label.img");
    DvzTexture* texture =
        dvz_ctx_texture(gpu->context, 3, (uvec3){ni, nj, nk}, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_texture_filter(texture, DVZ_FILTER_MAG, VK_FILTER_NEAREST);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    VkDeviceSize size = 0;
    uint8_t* tex_data = (uint8_t*)dvz_read_file(path, &size);
    ASSERT(size == ni * nj * nk * 4 * sizeof(uint8_t));
    dvz_texture_upload(texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size, tex_data);
    FREE(tex_data);
    return texture;
}



static DvzTexture* _earth_texture(DvzCanvas* canvas)
{
    DvzGpu* gpu = canvas->gpu;
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/earth.jpg", DATA_DIR);
    int width, height, depth;
    uint8_t* tex_data = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
    uint32_t tex_size = (uint32_t)(width * height);
    DvzTexture* texture = dvz_ctx_texture(
        gpu->context, 2, (uvec3){(uint32_t)width, (uint32_t)height, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_upload_texture(
        canvas, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, tex_size * sizeof(cvec4), tex_data);
    FREE(tex_data)
    return texture;
}



/*************************************************************************************************/
/*  Commands filling                                                                             */
/*************************************************************************************************/

static void empty_commands(TestCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



static void triangle_commands(TestCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas->br.buffer != NULL);
    ASSERT(canvas->graphics != NULL);
    ASSERT(canvas->bindings != NULL);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, canvas->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, canvas->graphics, canvas->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, 3);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



#endif
