#include <unistd.h>
#include <visky/visky.h>



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
typedef int (*VkyTestFunction)(VkyCanvas*);


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VkyTestCase
{
    const char* name;
    VkyTestFunction function;
    bool save_screenshot;
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CASE(name, save_screenshot)                                                               \
    {                                                                                             \
#name, name, save_screenshot                                                              \
    }

#define SWITCH_CLI_ARG(arg)                                                                       \
    if (argc >= 1 && strcmp(argv[1], #arg) == 0)                                                  \
        res = arg(argc - 1, &argv[1]);

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
        return 1;

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define ABOX(x, a, b, c, d)                                                                       \
    AT(((x).pos_ll[0] == (a)) && ((x).pos_ll[1] == (b)) && ((x).pos_ur[0] == (c)) &&              \
       ((x).pos_ur[1] == (d)))

#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);



/*************************************************************************************************/
/*  Include and define tests                                                                     */
/*************************************************************************************************/

#include "demo.h"
#include "test.h"
#include "test_basic.h"
#include "test_visuals.h"

static VkyTestCase TEST_CASES[] = {

    // Basic tests.
    CASE(red_canvas, true),
    CASE(blue_canvas, true),
    CASE(hello, true),
    CASE(triangle, true),

    // Visual props.
    CASE(visuals_props_1, false),
    CASE(visuals_props_2, false),
    CASE(visuals_props_3, false),
    CASE(visuals_props_4, false),
    CASE(visuals_props_5, false),
    CASE(visuals_props_6, false),

    // Transforms.
    CASE(transform_1, false),
    CASE(transform_2, false),
    CASE(panzoom_1, false),
    CASE(axes_1, false),
    CASE(axes_2, false),

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

static uint8_t* make_screenshot(VkyCanvas* canvas)
{
    // NOTE: the caller must free the output buffer
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    vky_begin_screenshot(screenshot);
    uint8_t* rgb = vky_screenshot_to_rgb(screenshot, false);
    vky_end_screenshot(screenshot);
    vky_destroy_screenshot(screenshot);
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
    log_error("Test case %s not found!", name);
    return (VkyTestCase){0};
}

static void reset_canvas(VkyCanvas* canvas)
{
    ASSERT(canvas != NULL);
    vky_reset_canvas(canvas);
    vky_clear_all_buffers(canvas->gpu);
    vky_reset_all_constants();
    vky_clear_color(canvas->scene, VKY_CLEAR_COLOR_BLACK);
}

static void run_app(VkyCanvas* canvas)
{
    // Run one frame of the example.
    vky_fill_command_buffers(canvas);

    // TODO: multiple frames before screenshot, mock input etc
    vky_offscreen_frame(canvas, 0);

    // for (double t = 0; t < frame_count / (float)FPS; t += (1. / FPS))
    // {
    //     vky_offscreen_frame(canvas, t);
    // }
}

static int launcher(VkyCanvas* canvas, const char* name)
{
    VkyTestCase test_case = get_test_case(name);
    ASSERT(test_case.function != NULL);

    // Run the test case on the canvas.
    int res = test_case.function(canvas);

    run_app(canvas);

    // If the function passed and needs to be compared with the screenshot, do it.
    if (res == 0 && test_case.save_screenshot)
    {
        // TODO OPTIM: create the screenshot only once, when creating the canvas
        uint8_t* rgb = make_screenshot(canvas);

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

    // Reset canvas for the next test.
    reset_canvas(canvas);

    return res;
}



/*************************************************************************************************/
/*  Main functions                                                                               */
/*************************************************************************************************/

static int test(int argc, char** argv)
{
    // argv: test, <name>, --live
    bool is_live = argc >= 3 && strcmp(argv[2], "--live") == 0;

    int res = 0;
    int index = 0;

    VkyBackendType backend = is_live ? VKY_BACKEND_GLFW : VKY_BACKEND_OFFSCREEN;
    VkyApp* app = vky_create_app(backend, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, WIDTH, HEIGHT);
    vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyGpu* gpu = canvas->gpu;
    // Create large GPU buffers that will be cleared after each test.
    vky_add_vertex_buffer(gpu, 1e6);
    vky_add_index_buffer(gpu, 1e6);

    // Start the tests.
    print_start();
    // Loop over all possible tests.
    for (uint32_t i = 0; i < N_TESTS; i++)
    {
        // Run a test only if all tests are requested, or if the requested test matches
        // the current test.
        if (argc == 1 || strcmp(TEST_CASES[i].name, argv[1]) == 0)
        {
            res += launcher(canvas, TEST_CASES[i].name);
            print_case(index, TEST_CASES[i].name, res);
            index++;
        }
    }
    print_end(index, res);

    vky_destroy_app(app);
    return res;
}

static int info(int argc, char** argv) { return 0; }

static int demo(int argc, char** argv) { return 0; }

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
