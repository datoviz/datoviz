/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing font                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_font.h"
#include "_cglm.h"
#include "_string_utils.h"
#include "scene/font.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Font test utils                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Font tests                                                                                   */
/*************************************************************************************************/

int test_font_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Load a font.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);

    // Create the font object.
    DvzFont* font = dvz_font(ttf_size, ttf_bytes);
    if (!font)
        return 1;
    dvz_font_size(font, 64);

    // Compute the layout of the text.
    // const char* text = "dfghijkl!01234";
    const char* text = "Hello world!\nabcdefhijklm";
    uint32_t n = strnlen(text, 1024);
    vec4* xywh = (vec4*)calloc(n, sizeof(vec4));
    dvz_font_ascii(font, text, xywh);
    for (uint32_t i = 0; i < n; i++)
    {
        glm_vec4_print(xywh[i], stdout);
    }

    // Render the text.
    uvec2 out_size;
    uint32_t count = 0;
    uint32_t* codepoints = _ascii_to_utf32(text, &count);
    uint8_t* bitmap = dvz_font_draw(font, n, codepoints, xywh, 0, out_size);

    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/font.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, out_size[0], out_size[1], bitmap);

    // Cleanup.
    FREE(bitmap);
    FREE(xywh);
    dvz_font_destroy(font);
    return 0;
}
