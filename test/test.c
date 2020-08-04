#include <stb_image.h>
#include <unistd.h>
#include <visky/visky.h>

#include "blank.h"
#include "compute.h"
#include "custom.h"
#include "plot2d.h"
#include "plot3d.h"

#include "test_vklite.h"

#define WIDTH  1024
#define HEIGHT 768

// Pass N = FPS * DURATION frames before taking a test screenshot
#define FPS      60
#define DURATION 1



/*************************************************************************************************/
/*  Test macros                                                                                  */
/*************************************************************************************************/

#define LAUNCH(layout, frame_count)                                                               \
    if (argc == 1 || strcmp(argv[1], "all") == 0 || strcmp(argv[1], test_name) == 0)              \
    {                                                                                             \
        launch_test(canvas, test_name, layout, cases, frame_count);                               \
    }

#define TEST_11(func, controller, controller_params, frame_count)                                 \
    {                                                                                             \
        TestCase cases[1] = {{#func, func, controller, controller_params}};                       \
        char test_name[256];                                                                      \
        sprintf(test_name, "test_11_%s", cases[0].case_name);                                     \
        LAUNCH(TEST_LAYOUT_11, frame_count);                                                      \
    }

#define TEST_12(func0, func1, c0, c0_params, c1, c1_params, frame_count)                          \
    {                                                                                             \
        TestCase cases[2] = {{#func0, func0, c0, c0_params}, {#func1, func1, c1, c1_params}};     \
        char test_name[256];                                                                      \
        sprintf(test_name, "test_12_%s_%s", cases[0].case_name, cases[1].case_name);              \
        LAUNCH(TEST_LAYOUT_12, frame_count);                                                      \
    }

#define VKLITE_TEST_NOCANVAS(func)                                                                \
    res = func();                                                                                 \
    show_single_test(#func, res);                                                                 \
    res_tot += res;

// TODO:
// test_canvas(canvas, #func, 1);
#define VKLITE_TEST(func)                                                                         \
    res = func(canvas);                                                                           \
    show_single_test(#func, res);                                                                 \
    res_tot += res;                                                                               \
    vky_reset_canvas(canvas);                                                                     \
    vky_clear_all_buffers(canvas->gpu);                                                           \
    vky_reset_all_constants();



/*************************************************************************************************/
/*  Test constants                                                                               */
/*************************************************************************************************/

static char SCREENSHOT_DIR[512];
static const double NORM3_255 = 1. / (3 * 255.0 * 255.0);
static const double NORM3_THRESHOLD = 1e-5;

static int32_t TEST_INDEX = 0;
static int32_t TEST_FAILED = 0;

static VkyBuffer* vertex_buffer;
static VkyBuffer* index_buffer;

typedef void (*TestFunction)(VkyPanel*);

typedef struct TestCase TestCase;
struct TestCase
{
    const char* case_name;
    TestFunction func;
    VkyControllerType controller;
    void* controller_params;
};

typedef enum
{
    TEST_LAYOUT_11,
    TEST_LAYOUT_12,
} TestLayout;



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/

static void show_single_test(const char* test_name, int res)
{
    printf("- Test #%03d %28s : ", TEST_INDEX, test_name);
    printf("\x1b[%dm%s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed" : "FAILED");
    TEST_INDEX++;
}



static bool file_exists(const char* path) { return access(path, F_OK) != -1; }



static int image_diff(const uint8_t* image_0, const char* path)
{
    int w = 0, h = 0;
    uint8_t* image_1 = read_ppm(path, &w, &h);
    ASSERT(w == WIDTH && h == HEIGHT);

    // Make sure the image is not all black.
    void* black = calloc(WIDTH * HEIGHT * 3, sizeof(uint8_t));
    if (memcmp(image_0, black, (size_t)(WIDTH * HEIGHT * 3 * sizeof(uint8_t))) == 0)
    {
        free(black);
        return 1;
    }

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
    free(image_1);
    return err < NORM3_THRESHOLD ? 0 : 1;
}



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

static int test_canvas(VkyCanvas* canvas, const char* test_name, uint32_t frame_count)
{
    log_debug("Starting test %s", test_name);

    srand(0);
    int diff = 0;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", SCREENSHOT_DIR, test_name);

    // Run one frame of the example.
    vky_fill_command_buffers(canvas);

    // Pass several frames before making a screenshot.
    // vky_offscreen_frame(canvas, 0);
    for (double t = 0; t < frame_count / (float)FPS; t += (1. / FPS))
    {
        vky_offscreen_frame(canvas, t);
    }

    // Make a screenshot.
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    vky_begin_screenshot(screenshot);
    uint8_t* image = vky_screenshot_to_rgb(screenshot, false);

    // If there is no existing saved screenshot, save it and skip the test.
    if (!file_exists(path))
    {
        log_debug("file %s didn't exist, creating it and skipping test", path);
        if (write_ppm(path, screenshot->width, screenshot->height, image) != 0)
            log_error("failed creating %s", path);
        else
            printf("  | created %s\n", path);
    }
    // Otherwise, compare the images.
    else
    {
        diff = image_diff(image, path);
    }

    // If the test failed, save the discrepant image.
    char path_failed[1024];
    snprintf(path_failed, sizeof(path_failed), "%s/%s_fail.ppm", SCREENSHOT_DIR, test_name);
    if (diff != 0)
    {
        log_debug("test %s failed", test_name);
        if (write_ppm(path_failed, screenshot->width, screenshot->height, image) != 0)
        {
            log_error("failed creating %s", path_failed);
        }
    }
    else
    {
        log_debug("test %s passed", test_name);
        if (file_exists(path_failed))
        {
            remove(path_failed);
        }
    }

    free(image);
    vky_end_screenshot(screenshot);
    vky_destroy_screenshot(screenshot);

    // Report.
    printf("- Test #%03d %28s : ", TEST_INDEX, test_name);
    printf("\x1b[%dm%s\x1b[0m\n", diff == 0 ? 32 : 31, diff == 0 ? "passed" : "FAILED");

    return diff;
}



static void set_test_case(VkyCanvas* canvas, uint32_t row, uint32_t col, TestCase test_case)
{
    VkyPanel* panel = vky_get_panel(canvas->scene, row, col);
    test_case.func(panel);
    vky_set_controller(panel, test_case.controller, test_case.controller_params);
}



static void launch_test(
    VkyCanvas* canvas, const char* test_name, TestLayout layout, TestCase* test_cases,
    uint32_t frame_count)
{
    VkyScene* scene = NULL;
    uint32_t cols = 1, rows = 1;

    if (layout == TEST_LAYOUT_12)
    {
        cols = 2;
    }

    // Create a new scene in the same canvas.
    scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, rows, cols);

    switch (layout)
    {

    case TEST_LAYOUT_11:
        set_test_case(canvas, 0, 0, test_cases[0]);
        break;

    case TEST_LAYOUT_12:
        set_test_case(canvas, 0, 0, test_cases[0]);
        set_test_case(canvas, 0, 1, test_cases[1]);
        break;

    default:
        log_error("Unknown test layout");
        break;
    }

    // Test the canvas.
    if (VKY_DEBUG_TEST)
    {
        vky_run_app(canvas->app);
    }
    else
    {
        TEST_FAILED += test_canvas(canvas, test_name, frame_count);
        TEST_INDEX++;
    }

    // Cleanup for the next test.
    vky_destroy_scene(scene);
    vky_reset_canvas(canvas);
    vky_clear_all_buffers(canvas->gpu);
    vky_reset_all_constants();
}



static int test_images(int argc, char* argv[])
{
    log_set_level_env();

    VkyBackendType backend = VKY_DEBUG_TEST ? VKY_BACKEND_GLFW : VKY_BACKEND_OFFSCREEN;
    VkyApp* app = vky_create_app(backend, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, WIDTH, HEIGHT);
    VkyGpu* gpu = canvas->gpu;

    // Create large GPU buffers that will be cleared after each test.
    vertex_buffer = vky_add_vertex_buffer(gpu, 1e6);
    index_buffer = vky_add_index_buffer(gpu, 1e6);

    printf("--- Image tests ----------------------------------\n");

#include "test_images.h"

    printf("--------------------------------------------------\n");
    if (TEST_INDEX > 0 && TEST_FAILED == 0)
        printf("\x1b[32m%d/%d tests PASSED.\x1b[0m\n", TEST_INDEX, TEST_INDEX);
    else if (TEST_INDEX > 0)
        printf("\x1b[31m%d/%d tests FAILED.\x1b[0m\n", TEST_FAILED, TEST_INDEX);
    else
        printf("\x1b[31mThere were no tests.\x1b[0m\n");

    vky_destroy_app(app);
    return TEST_FAILED;
}



/*************************************************************************************************/
/*  vklitetests                                                                                  */
/*************************************************************************************************/

static int test_vklite()
{
    log_set_level_env();
    int res = 0, res_tot = 0;

    printf("--- vklite tests ----------------------------------\n");

    // Tests without canvas.
    VKLITE_TEST_NOCANVAS(test_vklite_compute);

    // Tests with canvas.
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, WIDTH, HEIGHT);

    VKLITE_TEST(test_vklite_blank);

    vky_destroy_app(app);

    return res_tot;
}



int main(int argc, char* argv[])
{
    snprintf(SCREENSHOT_DIR, sizeof(SCREENSHOT_DIR), "%s/test/screenshots", ROOT_DIR);

    int res = 0;

    TEST_INDEX = 0;
    // Only run vklite tests when no arguments are given.
    if (argc == 1)
    {
        res += test_vklite();
    }
    printf("\n");

    TEST_INDEX = 0;
    res += test_images(argc, argv);

    return res;
}
