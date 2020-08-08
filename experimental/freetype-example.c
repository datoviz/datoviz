/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define WIDTH  640
#define HEIGHT 480

/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH][3];

static int write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image)
{
    // from https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp
    FILE* fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
        return 1;
    // ppm header
    char buffer[100];
    sprintf(buffer, "P6\n%d\n%d\n255\n", width, height);
    fwrite(buffer, strlen(buffer), 1, fp);
    // Write the RGB image.
    fwrite(image, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}

static void draw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
    FT_Int i, j, p, q;
    FT_Int x_max = x + bitmap->width;
    FT_Int y_max = y + bitmap->rows;

    for (i = x, p = 0; i < x_max; i++, p++)
    {
        for (j = y, q = 0; j < y_max; j++, q++)
        {
            if (i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT)
                continue;

            for (uint32_t k = 0; k < 3; k++)
            {
                image[j][i][k] |= bitmap->buffer[q * bitmap->width + p];
            }
        }
    }
}

int main(int argc, char** argv)
{
    FT_Library library;
    FT_Face face;

    FT_GlyphSlot slot;
    FT_Vector pen; /* untransformed origin  */
    FT_Error error;

    char* filename;
    char* text;

    int target_height;
    int n, num_chars;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s font sample-text\n", argv[0]);
        exit(1);
    }

    filename = argv[1]; /* first argument     */
    text = argv[2];     /* second argument    */
    num_chars = strlen(text);
    target_height = HEIGHT;

    error = FT_Init_FreeType(&library);               /* initialize library */
    error = FT_New_Face(library, filename, 0, &face); /* create face object */

    /* use 50pt at 100dpi */
    error = FT_Set_Char_Size(face, 50 * 64, 0, 100, 0); /* set character size */
    slot = face->glyph;

    /* the pen position in 26.6 cartesian space coordinates; */
    pen.x = 100 * 64;
    pen.y = (target_height - 100) * 64;

    for (n = 0; n < num_chars; n++)
    {
        FT_Set_Transform(face, NULL, &pen);

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
        if (error)
            continue; /* ignore errors */

        draw_bitmap(&slot->bitmap, slot->bitmap_left, target_height - slot->bitmap_top);

        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    write_ppm("text.ppm", WIDTH, HEIGHT, image);

    return 0;
}
