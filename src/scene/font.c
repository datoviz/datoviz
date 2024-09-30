/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Font                                                                                         */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/font.h"
#include "../_pointer.h"
#include "_macros.h"
#include "_string.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "fileio.h"

#if HAS_MSDF
#include <ft2build.h>
#include <stdio.h>
#include <stdlib.h>
#include FT_FREETYPE_H
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DEFAULT_FONT_SIZE 24
#define MAX_TEXT_LENGTH       65536
#define INTERLINE             1.5



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFont
{
#if HAS_MSDF
    FT_Library library;
    FT_Face face;
#endif
    double size;
};



/*************************************************************************************************/
/*  Font functions                                                                               */
/*************************************************************************************************/

DvzFont* dvz_font(unsigned long ttf_size, unsigned char* ttf_bytes)
{
#if !HAS_MSDF
    return NULL;
#endif

    DvzFont* font = (DvzFont*)calloc(1, sizeof(DvzFont));
    ANN(font);

#if HAS_MSDF
    if (FT_Init_FreeType(&font->library))
    {
        log_error("could not initialize freetype");
    }
    else if (FT_New_Memory_Face(font->library, ttf_bytes, (long int)ttf_size, 0, &font->face))
    {
        log_error("freetype could not load ttf font");
        // FT_Done_FreeType(&font->library);
    }
#else
    log_error("Datoviz has not been compiled with freetype support, ensure it was built with "
              "HAS_MSDF=1");
#endif

    dvz_font_size(font, DVZ_DEFAULT_FONT_SIZE);

    return font;
}



// DvzFont* dvz_font_default()
// {
//     unsigned long ttf_size = 0;
//     unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
//     ASSERT(ttf_size > 0);
//     ANN(ttf_bytes);
//     DvzFont* font = dvz_font(ttf_size, ttf_bytes);
//     return font;
// }



void dvz_font_size(DvzFont* font, double size)
{
    ANN(font);
    font->size = size;

#if HAS_MSDF
    if (!font->face)
    {
        log_error("font was not initialized");
        return;
    }

    if (size <= 0)
    {
        log_error("font size must be >= 0");
        return;
    }

    FT_Set_Pixel_Sizes(font->face, 0, (uint32_t)size);
#endif
}



// The caller must FREE the returned pointer.
vec4* dvz_font_layout(DvzFont* font, uint32_t length, const uint32_t* codepoints)
{
    ANN(font);
    ANN(codepoints);
    ASSERT(length > 0);

#if HAS_MSDF
    FT_Face face = font->face;
    if (!face)
    {
        log_error("font was not initialized");
        return NULL;
    }

    int pen_x = 0;
    int x = 0;
    int y = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    vec4* xywh = (vec4*)calloc(length, sizeof(vec4));
    int x_min = +1000000;
    int y_offset = 0;

    for (int i = 0; i < (int)length; i++)
    {
        if (codepoints[i] == 0x0A)
        {
            pen_x = 0;
            y_offset -= (int)h * INTERLINE;
            continue;
        }

        // Load the glyph for the current character
        if (FT_Load_Char(face, codepoints[i], FT_LOAD_RENDER))
        {
            // Handle glyph loading error
            continue;
        }

        // Glyph size.
        w = face->glyph->bitmap.width;
        h = face->glyph->bitmap.rows;

        // HACK:
        if (i == 0)
        {
            pen_x = 0; // -face->glyph->bitmap_left;
        }

        x = pen_x + face->glyph->bitmap_left;
        y = y_offset + face->glyph->bitmap_top - (int)h;

        x_min = MIN(x_min, x);

        xywh[i][0] = (float)x;
        xywh[i][1] = (float)y;
        xywh[i][2] = (float)w;
        xywh[i][3] = (float)h;

        // Update the pen position based on the glyph's advance width
        pen_x += (face->glyph->advance.x >> 6); // 1/64 pixel units
    }

    // Ensure the minimal x position is 0.
    for (int i = 0; i < (int)length; i++)
    {
        if (codepoints[i] != 0x0A)
        {
            xywh[i][0] -= x_min;
        }
    }

    return xywh;
#else
    return NULL;
#endif
}



vec4* dvz_font_ascii(DvzFont* font, const char* string)
{
    ANN(font);
    ANN(string);

    uint32_t count = 0;
    uint32_t* codepoints = _ascii_to_utf32(string, &count);

    return dvz_font_layout(font, count, codepoints);
}



// The caller must FREE the returned pointer.
uint8_t* dvz_font_draw(
    DvzFont* font, uint32_t length, const uint32_t* codepoints, vec4* xywh, int flags,
    uvec2 out_size)
{
    ANN(font);
    ANN(codepoints);
    ANN(xywh);
    ASSERT(length > 0);
    uint8_t n_channels = (flags & DVZ_FONT_FLAGS_RGBA) > 0 ? 4 : 3;

#if HAS_MSDF
    FT_Face face = font->face;
    if (!face)
    {
        log_error("font was not initialized");
        return NULL;
    }

    // Compute the size of the image.
    int top = -1000000, bottom = +1000000;
    int x = 0, y = 0, w = 0, h = 0;
    int width = 0;
    uint32_t itop = 0;

    for (uint32_t i = 0; i < length; i++)
    {
        x = (int)round(xywh[i][0]);
        y = (int)round(xywh[i][1]);
        w = (int)round(xywh[i][2]);
        h = (int)round(xywh[i][3]);

        width = MAX(width, x + w);

        // Maximum height from the baseline.
        if (y + h > top)
        {
            top = y + h;
            itop = i;
        }

        // Minimum vertical position from the baseline.
        if (y < bottom)
        {
            bottom = y;
        }
    }

    // Total image width.
    int margin = 5;
    width += 2 * margin;

    int ytop = (int)round(xywh[itop][1]);
    int htop = (int)round(xywh[itop][3]);

    int baseline = margin + ytop + htop; // origin at the top row
    ASSERT(baseline > 0);

    int height = baseline + abs(bottom) + margin;
    ASSERT(height > 0);
    ASSERT(height >= 2 * margin);
    ASSERT(baseline < height);

    log_debug("render the text in an image with size %dx%d", width, height);
    out_size[0] = (uint32_t)width;
    out_size[1] = (uint32_t)height;

    uint8_t* bitmap = (uint8_t*)calloc((uint32_t)(width * height * n_channels), sizeof(uint8_t));

    for (int i = 0; i < (int)length; i++)
    {
        // Skip new line.
        if (codepoints[i] == 0x0A)
        {
            continue;
        }

        // Load the glyph for the current character
        if (FT_Load_Char(face, codepoints[i], FT_LOAD_RENDER))
        {
            // Handle glyph loading error
            continue;
        }

        // Determine the top left corner of the glyph.
        x = round(xywh[i][0]);
        y = round(xywh[i][1]);
        w = round(xywh[i][2]);
        h = round(xywh[i][3]);

        // Convert to the coordinate space of the final image.
        x += margin;
        ASSERT(x >= margin);
        ASSERT(x + w <= width - margin);

        y = baseline - y - h;
        ASSERT(y >= margin);
        ASSERT(y + h <= height - margin);

        // Copy the glyph's bitmap into the final bitmap.
        for (int u = 0; u < w; u++)
        {
            for (int v = 0; v < h; v++)
            {
                uint32_t idx = (uint32_t)((y + v) * width + x + u);
                ASSERT((int)idx < width * height * 1);
                for (uint32_t k = 0; k < 3; k++)
                    bitmap[n_channels * idx + k] = face->glyph->bitmap.buffer[w * v + u];
                if (n_channels == 4)
                    bitmap[n_channels * idx + 3] = 255;
                // bitmap[n_channels * idx + 1] += 64; // DEBUG
            }
        }
    }

    return bitmap;
#else
    return NULL;
#endif
}



DvzId dvz_font_texture(
    DvzFont* font, DvzBatch* batch, uint32_t length, uint32_t* codepoints, uvec3 out_size)
{
    ANN(font);

    // Compute the layout of the text.
    vec4* xywh = dvz_font_layout(font, length, codepoints);
    uint8_t* bitmap = dvz_font_draw(font, length, codepoints, xywh, DVZ_FONT_FLAGS_RGBA, out_size);
    out_size[2] = 1;
    DvzId tex = dvz_tex_image(batch, DVZ_FORMAT_R8G8B8A8_UNORM, out_size[0], out_size[1], bitmap);

    // Cleanup.
    FREE(bitmap);
    FREE(xywh);

    return tex;
}



void dvz_font_destroy(DvzFont* font)
{
    ANN(font);

#if HAS_MSDF
    if (font->face)
        FT_Done_Face(font->face);

    if (font->library)
        FT_Done_FreeType(font->library);
#endif

    FREE(font);
}
