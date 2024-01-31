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
#include "fileio.h"
#include "request.h"

#include <ft2build.h>
#include <stdio.h>
#include <stdlib.h>
#include FT_FREETYPE_H



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFont
{
    FT_Library library;
    FT_Face face;
    double size;
};



/*************************************************************************************************/
/*  Font functions                                                                               */
/*************************************************************************************************/

DvzFont* dvz_font(unsigned long ttf_size, unsigned char* ttf_bytes)
{
    DvzFont* font = (DvzFont*)calloc(1, sizeof(DvzFont));
    ANN(font);

    if (FT_Init_FreeType(&font->library))
    {
        log_error("could not initialize freetype");
    }
    else if (FT_New_Memory_Face(font->library, ttf_bytes, (long int)ttf_size, 0, &font->face))
    {
        log_error("freetype could not load ttf font");
        // FT_Done_FreeType(&font->library);
    }

    dvz_font_size(font, DVZ_DEFAULT_FONT_SIZE);

    return font;
}



void dvz_font_size(DvzFont* font, double size)
{
    ANN(font);
    font->size = size;

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
}



// The caller must FREE the returned pointer.
vec4* dvz_font_layout(DvzFont* font, uint32_t length, const uint32_t* codepoints)
{
    ANN(font);
    ANN(codepoints);
    ASSERT(length > 0);

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

    for (int i = 0; i < (int)length; i++)
    {
        // Load the glyph for the current character
        if (FT_Load_Char(face, codepoints[i], FT_LOAD_RENDER))
        {
            // Handle glyph loading error
            continue;
        }

        // Glyph size.
        w = face->glyph->bitmap.width;
        h = face->glyph->bitmap.rows;

        // HACK: ensure the x position is 0 for the first glyph.
        if (i == 0)
        {
            pen_x = -face->glyph->bitmap_left;
        }

        x = pen_x + face->glyph->bitmap_left;
        y = face->glyph->bitmap_top - (int)h;

        xywh[i][0] = (float)x;
        xywh[i][1] = (float)y;
        xywh[i][2] = (float)w;
        xywh[i][3] = (float)h;

        // Update the pen position based on the glyph's advance width
        pen_x += (face->glyph->advance.x >> 6); // 1/64 pixel units
    }

    return xywh;
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
    DvzFont* font, uint32_t length, const uint32_t* codepoints, vec4* xywh, uvec2 out_size)
{
    ANN(font);
    ANN(codepoints);
    ANN(xywh);
    ASSERT(length > 0);

    FT_Face face = font->face;
    if (!face)
    {
        log_error("font was not initialized");
        return NULL;
    }

    // Compute the size of the image.
    int margin = 5;
    int width = 2 * margin + xywh[length - 1][0] + xywh[length - 1][2]; // x_last + w_last
    ASSERT(width > 0);

    int top = -1000000, bottom = +1000000;
    int x = 0, y = 0, w = 0, h = 0;
    uint32_t itop = 0;

    for (uint32_t i = 0; i < length; i++)
    {
        x = (int)round(xywh[i][0]);
        y = (int)round(xywh[i][1]);
        w = (int)round(xywh[i][2]);
        h = (int)round(xywh[i][3]);

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

    uint8_t* bitmap = (uint8_t*)calloc((uint32_t)(width * height * 3), sizeof(uint8_t));

    for (int i = 0; i < (int)length; i++)
    {
        // Load the glyph for the current character
        if (FT_Load_Char(face, codepoints[i], FT_LOAD_RENDER))
        {
            // Handle glyph loading error
            continue;
        }

        // Determine the upper-left corner of the glyph.
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
                // NOTE: in red for now.
                bitmap[3 * idx + 0] = face->glyph->bitmap.buffer[w * v + u];
                // bitmap[3 * idx + 1] = 255; // DEBUG
            }
        }
    }

    return bitmap;
}



void dvz_font_destroy(DvzFont* font)
{
    ANN(font);

    if (font->face)
        FT_Done_Face(font->face);

    if (font->library)
        FT_Done_FreeType(font->library);

    FREE(font);
}
