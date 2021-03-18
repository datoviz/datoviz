#ifndef DVZ_TEST_HEADER
#define DVZ_TEST_HEADER

#include "../include/datoviz/canvas.h"
#include "runner.h"



/*************************************************************************************************/
/*  Test declarations                                                                            */
/*************************************************************************************************/

// Test utils.
int test_utils_container(TestContext*);
int test_utils_fifo_1(TestContext*);
int test_utils_fifo_2(TestContext*);
int test_utils_fifo_3(TestContext*);
int test_utils_array_1(TestContext*);
int test_utils_array_2(TestContext*);
int test_utils_array_3(TestContext*);
int test_utils_array_4(TestContext*);
int test_utils_array_5(TestContext*);
int test_utils_array_6(TestContext*);
int test_utils_array_7(TestContext*);
int test_utils_array_cast(TestContext*);
int test_utils_array_mvp(TestContext*);
int test_utils_array_3D(TestContext*);
int test_utils_transforms_1(TestContext*);
int test_utils_transforms_2(TestContext*);
int test_utils_transforms_3(TestContext*);
int test_utils_transforms_4(TestContext*);
// int test_utils_transforms_5(TestContext*);

// Test vklite.
int test_vklite_app(TestContext*);
int test_vklite_commands(TestContext*);
int test_vklite_buffer_1(TestContext*);
int test_vklite_buffer_resize(TestContext*);
int test_vklite_compute(TestContext*);
int test_vklite_push(TestContext*);
int test_vklite_images(TestContext*);
int test_vklite_sampler(TestContext*);
int test_vklite_barrier(TestContext*);
int test_vklite_submit(TestContext*);
int test_vklite_offscreen(TestContext*);
int test_vklite_shader(TestContext*);
int test_vklite_surface(TestContext*);
int test_vklite_window(TestContext*);
int test_vklite_swapchain(TestContext*);
int test_vklite_graphics(TestContext*);
int test_vklite_canvas_blank(TestContext*);
int test_vklite_canvas_triangle(TestContext*);

// Test canvas.
int test_canvas_blank(TestContext*);
int test_canvas_multiple(TestContext*);
int test_canvas_events(TestContext*);
int test_canvas_screencast(TestContext*);
int test_canvas_video(TestContext*);

int test_canvas_triangle_1(TestContext*);
int test_canvas_triangle_offscreen(TestContext*);
int test_canvas_triangle_push(TestContext*);
int test_canvas_triangle_upload(TestContext*);
int test_canvas_triangle_uniform(TestContext*);
int test_canvas_triangle_compute(TestContext*);
int test_canvas_triangle_pick(TestContext*);
int test_canvas_triangle_append(TestContext*);



/*************************************************************************************************/
/*  List of tests                                                                                */
/*************************************************************************************************/

#define CASE_FIXTURE_NONE(func)                                                                   \
    {                                                                                             \
        .name = #func, .function = func,                                                          \
    }

#define CASE_FIXTURE_APP(func)                                                                    \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_APP                              \
    }

#define CASE_FIXTURE_CANVAS(func)                                                                 \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_CANVAS                           \
    }

static TestCase TEST_CASES[] = {
    // Utils.
    CASE_FIXTURE_NONE(test_utils_container),    //
    CASE_FIXTURE_NONE(test_utils_fifo_1),       //
    CASE_FIXTURE_NONE(test_utils_fifo_2),       //
    CASE_FIXTURE_NONE(test_utils_fifo_3),       //
    CASE_FIXTURE_NONE(test_utils_array_1),      //
    CASE_FIXTURE_NONE(test_utils_array_2),      //
    CASE_FIXTURE_NONE(test_utils_array_3),      //
    CASE_FIXTURE_NONE(test_utils_array_4),      //
    CASE_FIXTURE_NONE(test_utils_array_5),      //
    CASE_FIXTURE_NONE(test_utils_array_6),      //
    CASE_FIXTURE_NONE(test_utils_array_7),      //
    CASE_FIXTURE_NONE(test_utils_array_cast),   //
    CASE_FIXTURE_NONE(test_utils_array_mvp),    //
    CASE_FIXTURE_NONE(test_utils_array_3D),     //
    CASE_FIXTURE_NONE(test_utils_transforms_1), //
    CASE_FIXTURE_NONE(test_utils_transforms_2), //
    CASE_FIXTURE_NONE(test_utils_transforms_3), //
    CASE_FIXTURE_NONE(test_utils_transforms_4), //
    // CASE_FIXTURE_NONE(test_utils_transforms_5), //

    // vklite.
    CASE_FIXTURE_NONE(test_vklite_app),             //
    CASE_FIXTURE_NONE(test_vklite_commands),        //
    CASE_FIXTURE_NONE(test_vklite_buffer_1),        //
    CASE_FIXTURE_NONE(test_vklite_buffer_resize),   //
    CASE_FIXTURE_NONE(test_vklite_compute),         //
    CASE_FIXTURE_NONE(test_vklite_push),            //
    CASE_FIXTURE_NONE(test_vklite_images),          //
    CASE_FIXTURE_NONE(test_vklite_sampler),         //
    CASE_FIXTURE_NONE(test_vklite_barrier),         //
    CASE_FIXTURE_NONE(test_vklite_submit),          //
    CASE_FIXTURE_NONE(test_vklite_offscreen),       //
    CASE_FIXTURE_NONE(test_vklite_shader),          //
    CASE_FIXTURE_NONE(test_vklite_surface),         //
    CASE_FIXTURE_NONE(test_vklite_window),          //
    CASE_FIXTURE_NONE(test_vklite_swapchain),       //
    CASE_FIXTURE_NONE(test_vklite_graphics),        //
    CASE_FIXTURE_NONE(test_vklite_canvas_blank),    //
    CASE_FIXTURE_NONE(test_vklite_canvas_triangle), //

    // Canvas.
    CASE_FIXTURE_APP(test_canvas_blank),      //
    CASE_FIXTURE_APP(test_canvas_multiple),   //
    CASE_FIXTURE_APP(test_canvas_events),     //
    CASE_FIXTURE_APP(test_canvas_screencast), //
    CASE_FIXTURE_APP(test_canvas_video),      //

    CASE_FIXTURE_APP(test_canvas_triangle_1),         //
    CASE_FIXTURE_APP(test_canvas_triangle_offscreen), //
    CASE_FIXTURE_APP(test_canvas_triangle_push),      //
    CASE_FIXTURE_APP(test_canvas_triangle_upload),    //
    CASE_FIXTURE_APP(test_canvas_triangle_uniform),   //
    CASE_FIXTURE_APP(test_canvas_triangle_compute),   //
    CASE_FIXTURE_APP(test_canvas_triangle_pick),      //
    CASE_FIXTURE_APP(test_canvas_triangle_append),    //

};

static uint32_t N_TESTS = sizeof(TEST_CASES) / sizeof(TestCase);



/*************************************************************************************************/
/*  Test macros                                                                                  */
/*************************************************************************************************/

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        log_error("assertion '%s' failed", #x);                                                   \
        return 1;                                                                                 \
    }
#define AEn(n, x, y)                                                                              \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AT((x)[i] == (y)[i]);


#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN(((x) - (y)), -(eps), +(eps))
#define ACn(n, x, y, eps)                                                                         \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AC((x)[i], (y)[i], (eps));

#define WIDTH  800
#define HEIGHT 600
#define EPS    1e-6



/*************************************************************************************************/
/* Test context                                                                                  */
/*************************************************************************************************/

static TestContext _test_context()
{
    TestContext tc = {0};
    tc.n_tests = N_TESTS;
    tc.cases = TEST_CASES;
    return tc;
}

static void _test_context_destroy(TestContext* tc)
{
    ASSERT(tc != NULL);
    if (tc->canvas != NULL)
    {
        dvz_canvas_destroy(tc->canvas);
        tc->canvas = NULL;
    }
    if (tc->app != NULL)
    {
        dvz_app_destroy(tc->app);
        tc->app = NULL;
    }
}



/*************************************************************************************************/
/* Image comparison                                                                              */
/*************************************************************************************************/

static bool file_exists(const char* path) { return access(path, F_OK) != -1; }



static const double NORM3_255 = 1. / (3 * 255.0 * 255.0);
static const double NORM3_THRESHOLD = 1e-5;

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
    free(image_1);
    return err < NORM3_THRESHOLD ? 0 : 1;
}

// Insert a substring in a larger string.
static void strins(char* dest, char* ins, size_t offset)
{
    ASSERT(dest != NULL);
    ASSERT(ins != NULL);

    uint32_t n_ins = strlen(ins);
    uint32_t n_dest = strlen(dest);

    ASSERT(n_ins > 0);
    ASSERT(n_dest > 0);
    ASSERT(offset <= n_dest);

    char* dest_cpy = malloc(strlen(dest));
    strncpy(dest_cpy, dest, strlen(dest) + 1);
    strncpy(&dest[offset + strlen(ins)], &dest_cpy[offset], strlen(&dest_cpy[offset]) + 1);
    strncpy(&dest[offset], ins, strlen(ins));
    ASSERT(strlen(dest) == strlen(dest_cpy) + strlen(ins));
    FREE(dest_cpy);
}



// Check the image with the reference image, create/delete a fail image if needed.
static int check_image(const uint8_t* image, const char* path)
{
    int diff = 0;
    // If there is no existing saved screenshot, save it and skip the test.
    if (!file_exists(path))
    {
        log_debug("file %s didn't exist, creating it and skipping test", path);
        if (dvz_write_ppm(path, WIDTH, HEIGHT, image) != 0)
            log_error("failed creating %s", path);
        else
            log_info("created %s\n", path);
    }
    // Otherwise, compare the images.
    else
    {
        diff = image_diff(image, path);
    }

    // If the test failed, save the discrepant image.
    // Construct the failing path.
    char path_failed[1024];
    ASSERT(strlen(path) < 1000);
    strncpy(path_failed, path, strlen(path));
    char* subs = ".fail";
    char* ext = strrchr(path_failed, '.');
    strins(path_failed, subs, (uint64_t)ext - (uint64_t)path_failed);

    if (diff != 0)
    {
        log_error("test failed for %s, writing failing image", path);
        if (dvz_write_ppm(path_failed, WIDTH, HEIGHT, image) != 0)
            log_error("failed creating %s", path_failed);
        else
            log_debug("created %s", path_failed);
    }
    else
    {
        log_debug("test passed for %s", path);
        if (file_exists(path_failed))
            remove(path_failed);
    }
    return diff;
}



static int check_canvas(DvzCanvas* canvas, const char* test_name)
{
    ASSERT(canvas != NULL);
    uint8_t* image = dvz_screenshot(canvas, false);
    ASSERT(image != NULL);
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", ARTIFACTS_DIR, test_name);
    int diff = check_image(image, path);
    FREE(image);
    return diff;
}



#endif
