/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing image                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_string_utils.h"
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if OS_MACOS
#include <stdint.h>
typedef uint16_t char16_t;
typedef uint32_t char32_t;
#else
#include <uchar.h>
#endif

#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/image.h"
#include "scene/visuals/test_image.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

int test_image_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("image", VISUAL_TEST_PANZOOM, 0);

    // Create and upload the texture.
    uvec3 tex_shape = {0};
    DvzTexture* texture = load_crate_texture(vt.batch, tex_shape);
    float w = tex_shape[0], h = tex_shape[1];

    // Create the visual.
    DvzVisual* visual = dvz_image(vt.batch, DVZ_IMAGE_FLAGS_BORDER);

    // Visual allocation.
    dvz_image_alloc(visual, 1);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);

    // Image size.
    ASSERT(w > 0);
    ASSERT(h > 0);
    dvz_image_size(visual, 0, 1, (vec2[]){{w, h}}, 0);

    // Image anchor.
    dvz_image_anchor(visual, 0, 1, (vec2[]){{0, 0}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Image face colors.
    dvz_image_facecolor(visual, 0, 1, (DvzColor[]){{BLUE}}, 0);

    // Image parameters.
    dvz_image_radius(visual, 100.0);                // rounded corners
    dvz_image_linewidth(visual, 10.0);              // border width
    dvz_image_edgecolor(visual, (DvzColor){GREEN}); // border color

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Attach the texture to the image visual.
    dvz_image_texture(visual, texture);

    // Run the test.
    visual_test_end(vt);

    return 0;
}



static uint32_t* utf32_codepoints(const char* text, uint32_t* out_length)
{

#if OS_MACOS
    log_error("test_image_2 currently not supported on macOS");
    return NULL;
#else
    ANN(text);
    ANN(out_length);

    uint32_t length = 0;
    const char* ptr = text;
    mbstate_t state = {0};
    char32_t codepoint;
    uint32_t* codepoints = (uint32_t*)calloc(strnlen(ptr, 1024), sizeof(uint32_t));
    uint32_t k = 0;
    size_t len = 0;

    while ((len = mbrtoc32(&codepoint, ptr, (size_t)MB_CUR_MAX, &state)) > 0)
    {
        if (len == (size_t)-1 || len == (size_t)-2)
        {
            fprintf(stderr, "Invalid UTF-8 sequence\n");
            return NULL;
        }

        codepoints[k++] = (uint32_t)codepoint;
        length++;
        ptr += len;
    }
    ASSERT(length > 0);

    *out_length = length;
    return codepoints;
#endif
}

int test_image_2(TstSuite* suite, TstItem* tstitem)
{

#if OS_MACOS
    log_error("test_image_2 currently not supported on macOS");
    return 0;
#else
    VisualTest vt = visual_test_start("image", VISUAL_TEST_PANZOOM, 0);

    // NOTE: quick test with font texture image, need to find a way to define the image size
    // independently from the viewport size.
    // Generate font texture.

    // Load font.
    DvzSize ttf_size = 0;
    unsigned char* ttf_bytes = dvz_read_file("data/fonts/Arial-Unicode-Regular.ttf", &ttf_size);
    if (ttf_bytes == NULL)
    {
        return 0;
    }
    DvzFont* font = dvz_font(ttf_size, ttf_bytes);
    dvz_font_size(font, 64);

    // Font texture.
    const char* text = "Hello world!\n语言处理";
    uint32_t length = 0;
    uint32_t* codepoints = utf32_codepoints(text, &length);
    uvec3 tex_size = {0};
    DvzTexture* texture = dvz_font_texture(font, vt.batch, length, codepoints, tex_size);
    FREE(codepoints);
    dvz_font_destroy(font);
    float w = tex_size[0];
    float h = tex_size[1];



    // Create the visual.
    DvzVisual* visual = dvz_image(vt.batch, 0);

    // Visual allocation.
    dvz_image_alloc(visual, 1);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);

    // Image size.
    ASSERT(w > 0);
    ASSERT(h > 0);
    dvz_image_size(visual, 0, 1, (vec2[]){{w, h}}, 0);

    // Image anchor.
    dvz_image_anchor(visual, 0, 1, (vec2[]){{0, 0}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Attach the texture to the image visual.
    dvz_image_texture(visual, texture);

    // Run the test.
    visual_test_end(vt);

    return 0;
#endif
}
