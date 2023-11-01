/*************************************************************************************************/
/*  Font                                                                                         */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/font.h"
#include "../_pointer.h"
#include "_macros.h"
#include "fileio.h"
#include "request.h"

#include <ft2build.h>
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
    int pen_y = 0;
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

        // HACK: ensure the position is (0, 0) for the first glyph.
        if (i == 0)
        {
            pen_x = -face->glyph->bitmap_left;
            pen_y = +face->glyph->bitmap_top - (int)h;
        }

        x = pen_x + face->glyph->bitmap_left;
        y = pen_y - face->glyph->bitmap_top + (int)h;

        xywh[i][0] = (float)x;
        xywh[i][1] = -(float)y;
        xywh[i][2] = (float)w;
        xywh[i][3] = (float)h;

        // Update the pen position based on the glyph's advance width
        pen_x += (face->glyph->advance.x >> 6); // 1/64 pixel units
    }

    return xywh;
}



uint8_t* dvz_font_draw(DvzFont* font, uint32_t length, const uint32_t* codepoints, uvec2 out_size)
{
    ANN(font);
    return NULL;
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
