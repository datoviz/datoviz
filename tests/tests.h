#ifndef DVZ_TEST_HEADER
#define DVZ_TEST_HEADER

#include "../include/datoviz/canvas.h"
#include "proto.h"
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

int test_utils_colormap_idx(TestContext*);
int test_utils_colormap_uv(TestContext*);
int test_utils_colormap_extent(TestContext*);
int test_utils_colormap_default(TestContext*);
int test_utils_colormap_scale(TestContext*);
int test_utils_colormap_packuv(TestContext*);
int test_utils_colormap_array(TestContext*);

int test_utils_ticks_1(TestContext*);
int test_utils_ticks_2(TestContext*);
int test_utils_ticks_duplicate(TestContext*);
int test_utils_ticks_extend(TestContext*);

// Test vklite.
int test_vklite_app(TestContext*);
int test_vklite_commands(TestContext*);
int test_vklite_buffer_1(TestContext*);
int test_vklite_buffer_resize(TestContext*);
int test_vklite_compute(TestContext*);
int test_vklite_push(TestContext*);
int test_vklite_images(TestContext*);
int test_vklite_sampler(TestContext*);
int test_vklite_barrier_buffer(TestContext*);
int test_vklite_barrier_image(TestContext*);
int test_vklite_submit(TestContext*);
int test_vklite_offscreen(TestContext*);
int test_vklite_shader(TestContext*);
int test_vklite_surface(TestContext*);
int test_vklite_window(TestContext*);
int test_vklite_swapchain(TestContext*);
int test_vklite_graphics(TestContext*);
int test_vklite_canvas_blank(TestContext*);
int test_vklite_canvas_triangle(TestContext*);

// Test context.
int test_context_buffer(TestContext*);
int test_context_texture(TestContext*);
int test_context_compute(TestContext*);
int test_context_transfer_buffer(TestContext*);
int test_context_transfer_texture(TestContext*);
int test_context_colormap_custom(TestContext*);

// Test canvas.
int test_canvas_blank(TestContext*);
int test_canvas_multiple(TestContext*);
int test_canvas_events(TestContext*);
int test_canvas_gui(TestContext*);
int test_canvas_screencast(TestContext*);
int test_canvas_video(TestContext*);

int test_canvas_triangle_1(TestContext*);
int test_canvas_triangle_resize(TestContext*);
int test_canvas_triangle_offscreen(TestContext*);
int test_canvas_triangle_push(TestContext*);
int test_canvas_triangle_upload(TestContext*);
int test_canvas_triangle_uniform(TestContext*);
int test_canvas_triangle_compute(TestContext*);
int test_canvas_triangle_pick(TestContext*);
int test_canvas_triangle_append(TestContext*);

// Test interact.
int test_interact_panzoom(TestContext*);
int test_interact_arcball(TestContext*);
int test_interact_camera(TestContext*);

// Test graphics.
int test_graphics_point(TestContext*);
int test_graphics_line_list(TestContext*);
int test_graphics_line_strip(TestContext*);
int test_graphics_triangle_list(TestContext*);
int test_graphics_triangle_strip(TestContext*);
int test_graphics_triangle_fan(TestContext*);
int test_graphics_marker(TestContext*);
int test_graphics_segment(TestContext*);
int test_graphics_path(TestContext*);
int test_graphics_text(TestContext*);
int test_graphics_image_1(TestContext*);
int test_graphics_image_cmap(TestContext*);
int test_graphics_volume_slice(TestContext*);
int test_graphics_volume_1(TestContext*);
int test_graphics_mesh(TestContext*);

// Test visuals.
int test_visuals_sources(TestContext*);
int test_visuals_props(TestContext*);
int test_visuals_update_color(TestContext*);
int test_visuals_update_pos(TestContext*);
int test_visuals_partial(TestContext*);
int test_visuals_append(TestContext*);
int test_visuals_texture(TestContext*);

// Test builtin visuals.
int test_vislib_point(TestContext*);
int test_vislib_line_list(TestContext*);
int test_vislib_line_strip(TestContext*);
int test_vislib_triangle_list(TestContext*);
int test_vislib_triangle_strip(TestContext*);
int test_vislib_triangle_fan(TestContext*);
int test_vislib_marker(TestContext*);
int test_vislib_polygon(TestContext*);
int test_vislib_path(TestContext*);
int test_vislib_image(TestContext*);
int test_vislib_image_cmap(TestContext*);
int test_vislib_axes(TestContext*);
int test_vislib_mesh(TestContext*);
int test_vislib_volume(TestContext*);
int test_vislib_volume_slice(TestContext*);

// Test scene.
int test_scene_1(TestContext*);



/*************************************************************************************************/
/*  List of tests                                                                                */
/*************************************************************************************************/

#define CASE_FIXTURE(fixt, func)                                                                  \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_##fixt                           \
    }

static TestCase TEST_CASES[] = {
    // Utils.
    CASE_FIXTURE(NONE, test_utils_container),        //
    CASE_FIXTURE(NONE, test_utils_fifo_1),           //
    CASE_FIXTURE(NONE, test_utils_fifo_2),           //
    CASE_FIXTURE(NONE, test_utils_fifo_3),           //
    CASE_FIXTURE(NONE, test_utils_array_1),          //
    CASE_FIXTURE(NONE, test_utils_array_2),          //
    CASE_FIXTURE(NONE, test_utils_array_3),          //
    CASE_FIXTURE(NONE, test_utils_array_4),          //
    CASE_FIXTURE(NONE, test_utils_array_5),          //
    CASE_FIXTURE(NONE, test_utils_array_6),          //
    CASE_FIXTURE(NONE, test_utils_array_7),          //
    CASE_FIXTURE(NONE, test_utils_array_cast),       //
    CASE_FIXTURE(NONE, test_utils_array_mvp),        //
    CASE_FIXTURE(NONE, test_utils_array_3D),         //
    CASE_FIXTURE(NONE, test_utils_transforms_1),     //
    CASE_FIXTURE(NONE, test_utils_transforms_2),     //
    CASE_FIXTURE(NONE, test_utils_transforms_3),     //
    CASE_FIXTURE(NONE, test_utils_transforms_4),     //
    CASE_FIXTURE(NONE, test_utils_colormap_idx),     //
    CASE_FIXTURE(NONE, test_utils_colormap_uv),      //
    CASE_FIXTURE(NONE, test_utils_colormap_extent),  //
    CASE_FIXTURE(NONE, test_utils_colormap_default), //
    CASE_FIXTURE(NONE, test_utils_colormap_scale),   //
    CASE_FIXTURE(NONE, test_utils_colormap_packuv),  //
    CASE_FIXTURE(NONE, test_utils_colormap_array),   //
    CASE_FIXTURE(NONE, test_utils_ticks_1),          //
    CASE_FIXTURE(NONE, test_utils_ticks_2),          //
    CASE_FIXTURE(NONE, test_utils_ticks_duplicate),  //
    CASE_FIXTURE(NONE, test_utils_ticks_extend),     //

    // vklite.
    CASE_FIXTURE(NONE, test_vklite_app),             //
    CASE_FIXTURE(NONE, test_vklite_commands),        //
    CASE_FIXTURE(NONE, test_vklite_buffer_1),        //
    CASE_FIXTURE(NONE, test_vklite_buffer_resize),   //
    CASE_FIXTURE(NONE, test_vklite_compute),         //
    CASE_FIXTURE(NONE, test_vklite_push),            //
    CASE_FIXTURE(NONE, test_vklite_images),          //
    CASE_FIXTURE(NONE, test_vklite_sampler),         //
    CASE_FIXTURE(NONE, test_vklite_barrier_buffer),  //
    CASE_FIXTURE(NONE, test_vklite_barrier_image),   //
    CASE_FIXTURE(NONE, test_vklite_submit),          //
    CASE_FIXTURE(NONE, test_vklite_offscreen),       //
    CASE_FIXTURE(NONE, test_vklite_shader),          //
    CASE_FIXTURE(NONE, test_vklite_surface),         //
    CASE_FIXTURE(NONE, test_vklite_window),          //
    CASE_FIXTURE(NONE, test_vklite_swapchain),       //
    CASE_FIXTURE(NONE, test_vklite_graphics),        //
    CASE_FIXTURE(NONE, test_vklite_canvas_blank),    //
    CASE_FIXTURE(NONE, test_vklite_canvas_triangle), //

    // Context.
    CASE_FIXTURE(CONTEXT, test_context_buffer),           //
    CASE_FIXTURE(CONTEXT, test_context_compute),          //
    CASE_FIXTURE(CONTEXT, test_context_texture),          //
    CASE_FIXTURE(CONTEXT, test_context_transfer_buffer),  //
    CASE_FIXTURE(CONTEXT, test_context_transfer_texture), //
    CASE_FIXTURE(CONTEXT, test_context_colormap_custom),  //

    // Canvas.
    CASE_FIXTURE(APP, test_canvas_blank),              //
    CASE_FIXTURE(APP, test_canvas_multiple),           //
    CASE_FIXTURE(APP, test_canvas_events),             //
    CASE_FIXTURE(APP, test_canvas_gui),                //
    CASE_FIXTURE(APP, test_canvas_screencast),         //
    CASE_FIXTURE(APP, test_canvas_video),              //
    CASE_FIXTURE(APP, test_canvas_triangle_1),         //
    CASE_FIXTURE(APP, test_canvas_triangle_resize),    //
    CASE_FIXTURE(APP, test_canvas_triangle_offscreen), //
    CASE_FIXTURE(APP, test_canvas_triangle_push),      //
    CASE_FIXTURE(APP, test_canvas_triangle_upload),    //
    CASE_FIXTURE(APP, test_canvas_triangle_uniform),   //
    CASE_FIXTURE(APP, test_canvas_triangle_compute),   //
    CASE_FIXTURE(APP, test_canvas_triangle_pick),      //
    CASE_FIXTURE(APP, test_canvas_triangle_append),    //

    // Interact.
    CASE_FIXTURE(CANVAS, test_interact_panzoom), //
    CASE_FIXTURE(CANVAS, test_interact_arcball), //
    CASE_FIXTURE(CANVAS, test_interact_camera),  //

    // Graphics.
    CASE_FIXTURE(CANVAS, test_graphics_point),          //
    CASE_FIXTURE(CANVAS, test_graphics_line_list),      //
    CASE_FIXTURE(CANVAS, test_graphics_line_strip),     //
    CASE_FIXTURE(CANVAS, test_graphics_triangle_list),  //
    CASE_FIXTURE(CANVAS, test_graphics_triangle_strip), //
    CASE_FIXTURE(CANVAS, test_graphics_triangle_fan),   //
    CASE_FIXTURE(CANVAS, test_graphics_marker),         //
    CASE_FIXTURE(CANVAS, test_graphics_segment),        //
    CASE_FIXTURE(CANVAS, test_graphics_path),           //
    CASE_FIXTURE(CANVAS, test_graphics_text),           //
    CASE_FIXTURE(CANVAS, test_graphics_image_1),        //
    CASE_FIXTURE(CANVAS, test_graphics_image_cmap),     //
    CASE_FIXTURE(CANVAS, test_graphics_volume_slice),   //
    CASE_FIXTURE(CANVAS, test_graphics_volume_1),       //
    CASE_FIXTURE(CANVAS, test_graphics_mesh),           //

    // Visuals.
    CASE_FIXTURE(CANVAS, test_visuals_sources),      //
    CASE_FIXTURE(CANVAS, test_visuals_props),        //
    CASE_FIXTURE(CANVAS, test_visuals_update_color), //
    CASE_FIXTURE(CANVAS, test_visuals_update_pos),   //
    CASE_FIXTURE(CANVAS, test_visuals_partial),      //
    CASE_FIXTURE(CANVAS, test_visuals_append),       //
    CASE_FIXTURE(CANVAS, test_visuals_texture),      //

    // Builtin visuals.
    CASE_FIXTURE(CANVAS, test_vislib_point), //
    CASE_FIXTURE(CANVAS, test_vislib_line_list), //
    CASE_FIXTURE(CANVAS, test_vislib_line_strip), //
    CASE_FIXTURE(CANVAS, test_vislib_triangle_list), //
    CASE_FIXTURE(CANVAS, test_vislib_triangle_strip), //
    CASE_FIXTURE(CANVAS, test_vislib_triangle_fan), //
    CASE_FIXTURE(CANVAS, test_vislib_marker), //
    CASE_FIXTURE(CANVAS, test_vislib_polygon), //
    CASE_FIXTURE(CANVAS, test_vislib_path), //
    CASE_FIXTURE(CANVAS, test_vislib_image), //
    CASE_FIXTURE(CANVAS, test_vislib_image_cmap), //
    CASE_FIXTURE(CANVAS, test_vislib_axes), //
    CASE_FIXTURE(CANVAS, test_vislib_mesh), //
    CASE_FIXTURE(CANVAS, test_vislib_volume), //
    CASE_FIXTURE(CANVAS, test_vislib_volume_slice), //

    // Scene.
    CASE_FIXTURE(CANVAS, test_scene_1), //

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
    {                                                                                             \
        for (uint32_t k = 0; k < (n); k++)                                                        \
            AT((x)[k] == (y)[k]);                                                                 \
    }


#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN(((x) - (y)), -(eps), +(eps))
#define ACn(n, x, y, eps)                                                                         \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AC((x)[i], (y)[i], (eps));

#define EPS 1e-6



/*************************************************************************************************/
/* Test context                                                                                  */
/*************************************************************************************************/

static TestContext _test_context()
{
    TestContext tc = {0};
    tc.n_tests = N_TESTS;
    tc.cases = TEST_CASES;
    tc.debug = DEBUG_TEST;
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

    // if (tc->context != NULL)
    // {
    //     dvz_context_destroy(tc->context);
    //     tc->context = NULL;
    // }

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



static int64_t file_size(const char* path)
{
    long int sz = 0;
    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return 1;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fclose(fp);
    return sz;
}



static const double NORM3_255 = 1. / (3 * 255.0 * 255.0);
static const double NORM3_THRESHOLD = 1e-5;

static int image_diff(uvec2 size, const uint8_t* image_0, const char* path)
{
    int w = 0, h = 0;
    uint32_t width = size[0];
    uint32_t height = size[1];
    ASSERT(width > 0);
    ASSERT(height > 0);

    uint8_t* image_1 = dvz_read_ppm(path, &w, &h);
    if (w != (int)width || h != (int)height)
    {
        log_error("image size do not match! %dx%d vs %dx%d", width, height, w, h);
        return 1;
    }

    // Fast byte-to-byte comparison of the images.
    if (memcmp(image_0, image_1, (size_t)(width * height * 3 * sizeof(uint8_t))) == 0)
        return 0;
    log_debug("images were not byte-to-byte equivalent: computing the error distance");

    uint8_t rgb0[3], rgb1[3];
    double err = 0.0;
    for (uint32_t i = 0; i < height; i++)
    {
        for (uint32_t j = 0; j < width; j++)
        {
            memcpy(rgb0, &image_0[3 * i * width + 3 * j], sizeof(rgb0));
            memcpy(rgb1, &image_1[3 * i * width + 3 * j], sizeof(rgb1));
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
    err /= (width * height);
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

    char* dest_cpy = calloc(strlen(dest) + 1, 1);
    strncpy(dest_cpy, dest, strlen(dest));
    strncpy(&dest[offset + strlen(ins)], &dest_cpy[offset], strlen(&dest_cpy[offset]));
    strncpy(&dest[offset], ins, strlen(ins));
    dest[n_dest + n_ins] = 0;
    ASSERT(strlen(dest) == strlen(dest_cpy) + strlen(ins));
    FREE(dest_cpy);
}



// Check the image with the reference image, create/delete a fail image if needed.
static int check_image(uvec2 size, const uint8_t* image, const char* path)
{
    uint32_t width = size[0];
    uint32_t height = size[1];
    ASSERT(width > 0);
    ASSERT(height > 0);

    int diff = 0;
    // If there is no existing saved screenshot, save it and skip the test.
    if (!file_exists(path))
    {
        log_debug("file %s didn't exist, creating it and skipping test", path);
        if (dvz_write_ppm(path, width, height, image) != 0)
            log_error("failed creating %s", path);
        else
            log_info("created %s\n", path);
    }
    // Otherwise, compare the images.
    else
    {
        diff = image_diff(size, image, path);
    }

    // If the test failed, save the discrepant image.
    // Construct the failing path.
    char path_failed[1024] = {0};
    ASSERT(strlen(path) < 1000);
    strncpy(path_failed, path, strlen(path) + 1);
    // char* subs = ".fail";
    char* ext = strrchr(path_failed, '.');
    strins(path_failed, ".fail", (uint64_t)ext - (uint64_t)path_failed);

    if (diff != 0)
    {
        log_error("test failed for %s, writing failing image", path);
        if (dvz_write_ppm(path_failed, width, height, image) != 0)
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
    if (DEBUG_TEST)
        return 0;

    ASSERT(canvas != NULL);
    uint8_t* image = dvz_screenshot(canvas, false);
    ASSERT(image != NULL);
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", ARTIFACTS_DIR, test_name);

    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);

    int diff = check_image(size, image, path);
    FREE(image);
    return diff;
}



#endif
