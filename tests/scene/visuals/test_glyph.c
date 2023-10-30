/*************************************************************************************************/
/*  Testing glyph                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_glyph.h"
#include "renderer.h"
#include "request.h"
#include "scene/atlas.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H



/*************************************************************************************************/
/*  Glyph tests                                                                                  */
/*************************************************************************************************/

static void _on_timer(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev.content.t.time;

    // float x = sin(t);
    // float z = cos(t);
    // dvz_glyph_axis(visual, 0, 1, (vec3[]){{x, 0, z}}, 0);

    dvz_glyph_angle(visual, 0, 1, (float[]){M_PI * t}, 0);
    dvz_visual_update(visual);
}

int test_glyph_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("glyph", VISUAL_TEST_PANZOOM);

    // Number of items.
    const char* text = "HELLO";
    const uint32_t n = strnlen(text, 4096);
    AT(n > 0);

    // Create the visual.
    DvzVisual* visual = dvz_glyph(vt.batch, 0);

    // Visual allocation.
    dvz_glyph_alloc(visual, n);

    // Create the atlas.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);
    DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes);

    // Generate the atlas.
    dvz_atlas_generate(atlas);

    // Upload the atlas texture to the glyph visual.
    dvz_glyph_atlas(visual, atlas);

    vec2* size = (vec2*)calloc(n, sizeof(vec2));
    vec2* shift = (vec2*)calloc(n, sizeof(vec2));
    float font_size = 96;
    {
        FT_Library library;
        FT_Face face;

        if (FT_Init_FreeType(&library))
        {
            // Handle initialization error
            return 1;
        }

        if (FT_New_Face(library, "data/fonts/Roboto-Medium.ttf", 0, &face))
        {
            // Handle font loading error
            FT_Done_FreeType(library);
            return 1;
        }

        // Set the desired font size
        FT_Set_Pixel_Sizes(face, 0, (uint32_t)font_size);

        int pen_x = 0;
        int pen_y = 0;

        for (int i = 0; text[i] != '\0'; i++)
        {
            // Load the glyph for the current character
            if (FT_Load_Char(face, (long unsigned int)text[i], FT_LOAD_RENDER))
            {
                // Handle glyph loading error
                continue;
            }

            // Render the glyph to your target image at the pen position
            // (This part depends on your rendering target)
            int x = pen_x + face->glyph->bitmap_left;
            int y = pen_y - face->glyph->bitmap_top;
            uint32_t w = face->glyph->bitmap.width;
            uint32_t h = face->glyph->bitmap.rows;

            shift[i][0] = (float)x;
            shift[i][1] = (float)font_size - (float)y;
            size[i][0] = (float)w;
            size[i][1] = (float)h;

            // Update the pen position based on the glyph's advance width
            pen_x += (face->glyph->advance.x >> 6); // 1/64 pixel units

            // You can also apply kerning here if desired
            log_error("%d %d", x, y);
        }
    }

    // Set the texture coordinates.
    dvz_glyph_ascii(visual, text);

    // Set the position and color.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    dvz_glyph_position(visual, 0, n, pos, 0);
    FREE(pos);

    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    memset(color, 255, n * sizeof(cvec4));
    dvz_glyph_color(visual, 0, n, color, 0);
    FREE(color);

    // Set the glyph sizes.
    dvz_glyph_size(visual, 0, n, size, 0);

    // Set the glyph shifts.
    dvz_glyph_shift(visual, 0, n, shift, 0);

    // dvz_glyph_axis(visual, 0, n, (vec3[]){{0, 0, 1}}, 0);
    // dvz_glyph_anchor(visual, 0, n, (vec2[]){{0, 0}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // // Animation.
    // vt.visual = visual;
    // dvz_app_timer(vt.app, 0, 1. / 60., 0);
    // dvz_app_ontimer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_atlas_destroy(atlas);
    FREE(size);
    FREE(shift);

    return 0;
}
