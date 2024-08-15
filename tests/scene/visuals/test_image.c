/*************************************************************************************************/
/*  Testing image                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include <locale.h>
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <uchar.h>
// #include "_string.h"

#include "scene/visuals/test_image.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/image.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

int test_image_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("image", VISUAL_TEST_PANZOOM, 0);

    // NOTE: quick test with font texture image, need to find a way to define the image size
    // independently from the viewport size.
    // // Generate font texture.
    // uint32_t length = 0;
    // const char* ptr = "Hello world! 语言处理";
    // mbstate_t state = {0};
    // char32_t codepoint;
    // uint32_t* codepoints = (uint32_t*)calloc(strnlen(ptr, 1024), sizeof(uint32_t));
    // uint32_t k = 0;
    // size_t len = 0;

    // while ((len = mbrtoc32(&codepoint, ptr, MB_CUR_MAX, &state)) > 0)
    // {
    //     if (len == (size_t)-1 || len == (size_t)-2)
    //     {
    //         fprintf(stderr, "Invalid UTF-8 sequence\n");
    //         return 1;
    //     }

    //     codepoints[k++] = (uint32_t)codepoint;
    //     length++;
    //     ptr += len;
    // }

    // DvzSize ttf_size = 0;
    // unsigned char* ttf_bytes = dvz_read_file("data/fonts/Arial-Unicode-Regular.ttf", &ttf_size);
    // DvzFont* font = dvz_font(ttf_size, ttf_bytes);
    // dvz_font_size(font, 64);
    // uvec3 tex_size = {0};
    // DvzId tex = dvz_font_texture(font, vt.batch, length, codepoints, tex_size);
    // FREE(codepoints);
    // dvz_font_destroy(font);
    // float w = tex_size[0] / (float)WIDTH;
    // float h = tex_size[1] / (float)HEIGHT;

    float w = 1, h = 1;

    // Create the visual.
    DvzVisual* visual = dvz_image(vt.batch, 0);

    // Visual allocation.
    dvz_image_alloc(visual, 1);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec4[]){{-w, +h, +w, -h}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Create and upload the texture.
    uvec3 tex_shape = {0};
    DvzId tex = load_crate_texture(vt.batch, tex_shape);

    dvz_image_texture(visual, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
