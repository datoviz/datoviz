#include <stb_image.h>
#include <unistd.h>
#include <visky/visky.h>
#include <visky/vklite2.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

// Pass N = FPS * DURATION frames before taking a test screenshot
#define FPS         60
#define DURATION    1
#define MAX_RETRIES 10

#define IMAGE_RELPATH "images/tests"

#define NORM3_255       (1. / (3 * 255.0 * 255.0))
#define NORM3_THRESHOLD 1e-5



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VkyTestCase VkyTestCase;
typedef struct VkyTestContext VkyTestContext;

// Test cases callbacks.
typedef int (*VkyTestFunction)(VkyTestContext*);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_TEST_FIXTURE_NONE,
    VKY_TEST_FIXTURE_CANVAS,
    VKY_TEST_FIXTURE_PANEL,
} VkyTestFixture;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VkyTestContext
{
    VkyApp* app;
    VkyCanvas* canvas;
    VkyScene* scene;
    VkyPanel* panel;
    VkyScreenshot* screenshot;
    bool is_live;
};

struct VkyTestCase
{
    const char* name;
    VkyTestFixture fixture;
    VkyTestFunction function;
    VkyTestFunction destroy;
    bool save_screenshot;
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CASE_FIXTURE_NONE(func)                                                                   \
    {                                                                                             \
#func, VKY_TEST_FIXTURE_NONE, func, NULL, false                                           \
    }

#define CASE_FIXTURE_CANVAS(func, func_destroy, screenshot)                                       \
    {                                                                                             \
#func, VKY_TEST_FIXTURE_CANVAS, func, func_destroy, screenshot                            \
    }

#define CASE_FIXTURE_PANEL(func, screenshot)                                                      \
    {                                                                                             \
#func, VKY_TEST_FIXTURE_PANEL, func, NULL, screenshot                                     \
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

#define ABOX(x, a, b, c, d)                                                                       \
    AT(((x).pos_ll[0] == (a)) && ((x).pos_ll[1] == (b)) && ((x).pos_ur[0] == (c)) &&              \
       ((x).pos_ur[1] == (d)))

#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);



/*************************************************************************************************/
/*  Include and define tests                                                                     */
/*************************************************************************************************/

#include "test.h"
#include "test_basic.h"
#include "test_demo.h"
#include "test_visuals.h"
#include "test_vklite2.h"

static VkyTestCase TEST_CASES[] = {

    // vklite2
    CASE_FIXTURE_NONE(vklite2_app),             //
    CASE_FIXTURE_NONE(vklite2_surface),         //
    CASE_FIXTURE_NONE(vklite2_window),          //
    CASE_FIXTURE_NONE(vklite2_swapchain),       //
    CASE_FIXTURE_NONE(vklite2_commands),        //
    CASE_FIXTURE_NONE(vklite2_buffer),          //
    CASE_FIXTURE_NONE(vklite2_compute),         //
    CASE_FIXTURE_NONE(vklite2_push),            //
    CASE_FIXTURE_NONE(vklite2_images),          //
    CASE_FIXTURE_NONE(vklite2_sampler),         //
    CASE_FIXTURE_NONE(vklite2_barrier),         //
    CASE_FIXTURE_NONE(vklite2_submit),          //
    CASE_FIXTURE_NONE(vklite2_blank),           //
    CASE_FIXTURE_NONE(vklite2_graphics),        //
    CASE_FIXTURE_NONE(vklite2_canvas_basic),    //
    CASE_FIXTURE_NONE(vklite2_canvas_triangle), //

    CASE_FIXTURE_NONE(vklite2_context_buffer),  //
    CASE_FIXTURE_NONE(vklite2_context_texture), //
    CASE_FIXTURE_NONE(vklite2_default_app),     //


    // Visual props.
    CASE_FIXTURE_NONE(visuals_props_1), //
    CASE_FIXTURE_NONE(visuals_props_2), //
    CASE_FIXTURE_NONE(visuals_props_3), //
    CASE_FIXTURE_NONE(visuals_props_4), //
    CASE_FIXTURE_NONE(visuals_props_5), //
    CASE_FIXTURE_NONE(visuals_props_6), //

    CASE_FIXTURE_NONE(transform_1), //

    // vklite tests.
    CASE_FIXTURE_CANVAS(vklite_compute, no_destroy, false),               //
    CASE_FIXTURE_CANVAS(vklite_blank, no_destroy, false),                 //
    CASE_FIXTURE_CANVAS(vklite_triangle, vklite_triangle_destroy, false), //
    CASE_FIXTURE_CANVAS(vklite_push, vklite_push_destroy, false),         //

    // Transforms.
    CASE_FIXTURE_PANEL(transform_2, false), //
    CASE_FIXTURE_PANEL(panzoom_1, false),   //
    CASE_FIXTURE_PANEL(axes_1, false),      //
    CASE_FIXTURE_PANEL(axes_2, false),      //

    // Basic tests.
    CASE_FIXTURE_PANEL(red_canvas, true),  //
    CASE_FIXTURE_PANEL(blue_canvas, true), //
    CASE_FIXTURE_PANEL(hello, true),       //
    CASE_FIXTURE_PANEL(triangle, true),    //

    // Visual tests
    CASE_FIXTURE_PANEL(mesh_raw, true),   //
    CASE_FIXTURE_PANEL(scatter, true),    //
    CASE_FIXTURE_PANEL(imshow, true),     //
    CASE_FIXTURE_PANEL(arrows, true),     //
    CASE_FIXTURE_PANEL(paths, true),      //
    CASE_FIXTURE_PANEL(segments, true),   //
    CASE_FIXTURE_PANEL(hist, true),       //
    CASE_FIXTURE_PANEL(area, true),       //
    CASE_FIXTURE_PANEL(axrect, true),     //
    CASE_FIXTURE_PANEL(raster, true),     //
    CASE_FIXTURE_PANEL(graph, true),      //
    CASE_FIXTURE_PANEL(image, true),      //
    CASE_FIXTURE_PANEL(image_cmap, true), //
    CASE_FIXTURE_PANEL(polygon, true),    //
    CASE_FIXTURE_PANEL(pslg_1, true),     //
    CASE_FIXTURE_PANEL(pslg_2, true),     //
    CASE_FIXTURE_PANEL(france, true),     //
    CASE_FIXTURE_PANEL(surface, true),    //
    CASE_FIXTURE_PANEL(spheres, true),    //
    CASE_FIXTURE_PANEL(volume, true),     //

    // Demo tests
    CASE_FIXTURE_PANEL(brain, true),      //
    CASE_FIXTURE_PANEL(axes_3D, true),    //
    CASE_FIXTURE_PANEL(raytracing, true), //
    CASE_FIXTURE_PANEL(mandelbrot, true), //

};
static uint32_t N_TESTS = sizeof(TEST_CASES) / sizeof(VkyTestCase);



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static bool file_exists(const char* path) { return access(path, F_OK) != -1; }

static int image_diff(const uint8_t* image_0, const char* path)
{
    int w = 0, h = 0;
    uint8_t* image_1 = read_ppm(path, &w, &h);
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
    res = write_ppm(path, WIDTH, HEIGHT, rgb);
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

static uint8_t* make_screenshot(VkyTestContext* context)
{
    ASSERT(context != NULL);
    ASSERT(context->canvas != NULL);
    // NOTE: the caller must free the output buffer
    if (context->screenshot == NULL)
        context->screenshot = vky_create_screenshot(context->canvas);
    vky_begin_screenshot(context->screenshot);
    uint8_t* rgb = vky_screenshot_to_rgb(context->screenshot, false);
    vky_end_screenshot(context->screenshot);
    return rgb;
}

static void print_start()
{
    printf("--- Starting tests -------------------------------\n"); //
}

static void print_case(int index, const char* name, int res)
{
    printf("- Test #%03d %28s : ", index, name);
    printf("\x1b[%dm%s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed" : "FAILED");
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

static VkyTestCase get_test_case(const char* name)
{
    for (uint32_t i = 0; i < N_TESTS; i++)
    {
        if (strcmp(TEST_CASES[i].name, name) == 0)
        {
            return TEST_CASES[i];
        }
    }
    log_error("test case %s not found!", name);
    return (VkyTestCase){0};
}

static void run_canvas(VkyCanvas* canvas)
{
    // Run one frame of the example.
    vky_fill_command_buffers(canvas);

    // TODO: multiple frames before screenshot, mock input etc
    if (canvas->is_offscreen)
        vky_offscreen_frame(canvas, 0);
    else
        vky_run_app(canvas->app);

    // for (double t = 0; t < frame_count / (float)FPS; t += (1. / FPS))
    // {
    //     vky_offscreen_frame(canvas, t);
    // }
}


/*************************************************************************************************/
/*  Testing infrastructure                                                                       */
/*************************************************************************************************/

static void _setup(VkyTestContext* context, VkyTestFixture fixture)
{
    ASSERT(context != NULL);

    if (fixture >= VKY_TEST_FIXTURE_CANVAS)
    {
        if (context->app == NULL)
        {
            log_debug("fixture setup: create the app");
            context->app =
                vky_create_app(context->is_live ? VKY_BACKEND_GLFW : VKY_BACKEND_OFFSCREEN, NULL);
        }
        ASSERT(context->app != NULL);
        if (context->canvas == NULL)
        {
            log_debug("fixture setup: create the canvas");
            context->canvas = vky_create_canvas(context->app, WIDTH, HEIGHT);
            // Create large GPU buffers that will be cleared after each test.
            vky_add_vertex_buffer(context->canvas->gpu, 1e6);
            vky_add_index_buffer(context->canvas->gpu, 1e6);
        }

        ASSERT(context->canvas != NULL);
    }

    if (fixture >= VKY_TEST_FIXTURE_PANEL)
    {
        ASSERT(context->canvas != NULL);
        if (context->scene == NULL)
        {
            log_debug("fixture setup: create the scene");
            context->scene = vky_create_scene(context->canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
        }
        ASSERT(context->scene != NULL);
        if (context->panel == NULL)
        {
            log_debug("fixture setup: create the panel");
            context->panel = vky_get_panel(context->scene, 0, 0);
        }

        ASSERT(context->panel != NULL);
    }
}

static void _teardown(VkyTestContext* context, VkyTestFixture fixture)
{
    ASSERT(context != NULL);
    // NOTE: do not try to reset the canvas when is_live is true, because there is
    // only one canvas so it doesn't make sense, and it would cause a segfault
    // as the canvas is destroyed as soon as it is closed.
    if (fixture >= VKY_TEST_FIXTURE_CANVAS && !context->is_live)
    {
        ASSERT(context->canvas != NULL);
        log_debug("fixture teardown: reset the canvas");
        vky_reset_canvas(context->canvas);
        ASSERT(context->canvas->gpu != NULL);
        vky_clear_all_buffers(context->canvas->gpu);
        vky_reset_all_constants();
    }
    if (fixture >= VKY_TEST_FIXTURE_PANEL)
    {
        log_debug("fixture teardown: destroy the scene");
        vky_destroy_scene(context->canvas->scene);
        context->scene = NULL;
        context->panel = NULL;
    }
}

static VkyTestContext _create_context(bool is_live)
{
    VkyTestContext context = {0};
    context.is_live = is_live;
    return context;
}

static void _destroy_context(VkyTestContext* context)
{
    ASSERT(context != NULL);

    if (context->screenshot != NULL)
    {
        vky_destroy_screenshot(context->screenshot);
        context->screenshot = NULL;
    }

    if (context->app != NULL)
    {
        vky_destroy_app(context->app);
        context->app = NULL;
    }
}

static int launcher(VkyTestContext* context, const char* name)
{
    srand(0);

    VkyTestCase test_case = get_test_case(name);
    if (test_case.function == NULL)
        return 1;

    ASSERT(context != NULL);
    ASSERT(name != NULL);

    // Make sure either the canvas or panel is set up if the test case requires it.
    _setup(context, test_case.fixture);

    // Run the test case on the canvas.
    int res = 1;
    ASSERT(test_case.function != NULL);
    res = test_case.function(context);

    // Run the app.
    if (test_case.fixture >= VKY_TEST_FIXTURE_CANVAS)
    {
        ASSERT(context->canvas != NULL);
        run_canvas(context->canvas);
    }

    // Only continue here when offscreen mode.
    if (!context->is_live)
    {
        // If the function passed and needs to be compared with the screenshot, do it.
        if (res == 0 && test_case.save_screenshot)
        {
            // TODO OPTIM: create the screenshot only once, when creating the canvas
            uint8_t* rgb = make_screenshot(context);

            // Test fails if image is blank, not even need to compare with screenshot.
            if (is_blank(rgb))
            {
                log_debug("image was blank, test failed");
                res = 1;
            }
            else
            {
                res = compare_images(name, rgb);
                log_debug("image comparison %s", res == 0 ? "succeeded" : "failed");
            }
            FREE(rgb);
        }
    }

    // Call the test case-specific destruction function if there is one.
    if (test_case.destroy != NULL)
        test_case.destroy(context);

    // Tear down the fixture (mainly resetting the canvas or destroying the scene).
    _teardown(context, test_case.fixture);

    return res;
}



/*************************************************************************************************/
/*  Main functions                                                                               */
/*************************************************************************************************/

static int test(int argc, char** argv)
{
    // argv: test, <name>, --live
    bool is_live = argc >= 3 && strcmp(argv[2], "--live") == 0;
    print_start();

    // Create the test context.
    VkyTestContext context = _create_context(is_live);

    // Start the tests.
    int cur_res = 0;
    int res = 0;
    int index = 0;
    // Loop over all possible tests.
    for (uint32_t i = 0; i < N_TESTS; i++)
    {
        // Run a test only if all tests are requested, or if the requested test matches
        // the current test.
        if (argc == 1 || strstr(TEST_CASES[i].name, argv[1]) != NULL)
        {
            cur_res = launcher(&context, TEST_CASES[i].name);
            print_case(index, TEST_CASES[i].name, cur_res);
            res += cur_res == 0 ? 0 : 1;
            index++;
        }
    }
    print_end(index, res);

    // Destroy the app if needed.
    _destroy_context(&context);

    return res;
}

static int info(int argc, char** argv)
{
    VkyGpu gpu = vky_create_device(0, NULL);
    printf("GPU: %s\n", gpu.device_properties.deviceName);
    vky_destroy_device(&gpu);
    return 0;
}

static int demo(int argc, char** argv)
{
    if (argc <= 1)
    {
        log_error("please specify a demo name");
        return 1;
    }
    VkyTestContext context = _create_context(true);
    launcher(&context, argv[1]);
    _destroy_context(&context);
    return 0;
}

int main(int argc, char** argv)
{
    log_set_level_env();
    if (argc <= 1)
    {
        log_error("specify a command: info, demo, test");
        return 1;
    }
    ASSERT(argc >= 2);
    int res = 0;
    SWITCH_CLI_ARG(info)
    SWITCH_CLI_ARG(test)
    SWITCH_CLI_ARG(demo)
    return res;
}
