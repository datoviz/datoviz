/*************************************************************************************************/
/*  Atlas                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/atlas.h"
#include "../_pointer.h"
#include "_macros.h"
#include "fileio.h"
#include "request.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" //
#pragma GCC diagnostic ignored "-Wsign-conversion"
// #include "msdfgen.h"
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#pragma GCC diagnostic pop

using namespace msdfgen;
using namespace msdf_atlas;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzAtlas
{
    // Specified charset.
    uint32_t codepoints_count;
    uint32_t* codepoints;

    // Information about the glyph positions in the atlas.
    std::vector<GlyphGeometry> glyphs;

    // Internal objects.
    FreetypeHandle* ft;
    FontHandle* font;
    TightAtlasPacker packer;

    // Atlas bitmap.
    uint32_t width;
    uint32_t height;
    uint8_t* rgb;
};



/*************************************************************************************************/
/*  Atlas functions                                                                              */
/*************************************************************************************************/

DvzAtlas* dvz_atlas(unsigned long ttf_size, unsigned char* ttf_bytes)
{
    DvzAtlas* atlas = (DvzAtlas*)calloc(1, sizeof(DvzAtlas));
    ANN(atlas);

    // Initialize instance of FreeType library
    atlas->ft = initializeFreetype();

    // Load font file
    atlas->font = loadFontData(atlas->ft, ttf_bytes, ttf_size);

    return atlas;
}



void dvz_atlas_clear(DvzAtlas* atlas)
{
    ANN(atlas);
    auto glyphs = atlas->glyphs;
    glyphs.clear();
    // atlas->packer;
}



// This function makes a copy of the codepoints array. The atlas will destroy the copy upon atlas
// destruction.
void dvz_atlas_codepoints(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints)
{
    ANN(atlas);
    ASSERT(count > 0);
    ANN(codepoints);

    atlas->codepoints_count = count;
    atlas->codepoints = (uint32_t*)_cpy((DvzSize)(count * sizeof(uint32_t)), (void*)codepoints);
}



void dvz_atlas_string(DvzAtlas* atlas, const char* string)
{
    ANN(atlas);

    atlas->codepoints_count = strnlen(string, 4096);
    ASSERT(atlas->codepoints_count > 0);
    ASSERT(atlas->codepoints_count < 4096);
    uint32_t* codepoints = (uint32_t*)calloc(atlas->codepoints_count, sizeof(uint32_t));
    for (uint32_t i = 0; i < atlas->codepoints_count; i++)
    {
        codepoints[i] = (uint32_t)string[i];
    }
    atlas->codepoints = codepoints;
}



int dvz_atlas_glyph(DvzAtlas* atlas, uint32_t codepoint, vec4 out_coords)
{
    ANN(atlas);
    int x, y, w, h;
    bool found = false;
    for (const GlyphGeometry& glyph : atlas->glyphs)
    {
        if ((uint32_t)glyph.getCodepoint() == codepoint)
        {
            glyph.getBoxRect(x, y, w, h);
            found = true;
            break;
        }
    }
    if (!found)
        return 1;

    out_coords[0] = (float)x;
    out_coords[1] = (float)((int)atlas->height - h - y);
    out_coords[2] = (float)w;
    out_coords[3] = (float)h;

    return 0;
}



// NOTE: the caller must FREE the returned pointer.
vec4* dvz_atlas_glyphs(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints)
{
    ANN(atlas);
    ASSERT(count > 0);
    ANN(codepoints);

    vec4* out_coords = (vec4*)calloc(count, sizeof(vec4));
    for (uint32_t i = 0; i < count; i++)
    {
        int result = dvz_atlas_glyph(atlas, codepoints[i], out_coords[i]);
        if (result != 0)
        {
            log_warn("code point %d not found in the font atlas", codepoints[i]);
        }
    }
    return out_coords;
}



int dvz_atlas_generate(DvzAtlas* atlas)
{
    ANN(atlas);

    // FontGeometry is a helper class that loads a set of glyphs from a single font.
    // It can also be used to get additional font metrics, kerning information, etc.
    FontGeometry fontGeometry(&atlas->glyphs);

    // Charset.
    Charset charset;
    if (atlas->codepoints_count == 0)
    {
        // By default, use the ASCII charset.
        charset = Charset::ASCII;
    }
    else
    {
        // Create a manual charset with the specified Unicode codepoints.
        for (uint32_t i = 0; i < atlas->codepoints_count; i++)
        {
            charset.add(atlas->codepoints[i]);
        }
    }

    // The second argument can be ignored unless you mix different font sizes in one atlas.
    // In the last argument, you can specify a charset other than ASCII.
    // To load specific glyph indices, use loadGlyphs instead.
    fontGeometry.loadCharset(atlas->font, 1.0, charset);

    // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
    const double maxCornerAngle = 3.0;
    for (GlyphGeometry& glyph : atlas->glyphs)
        glyph.edgeColoring(&edgeColoringInkTrap, maxCornerAngle, 0);

    // TightAtlasPacker class computes the layout of the atlas.
    TightAtlasPacker packer;

    // Set atlas parameters:
    // setDimensions or setDimensionsConstraint to find the best value
    packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);

    // setScale for a fixed size or setMinimumScale to use the largest that fits
    packer.setMinimumScale(48.0);

    // setPixelRange or setUnitRange
    packer.setPixelRange(2.0);
    packer.setMiterLimit(1.0);

    // Compute atlas layout - pack glyphs
    packer.pack(atlas->glyphs.data(), atlas->glyphs.size());

    // Get final atlas dimensions
    int width = 0, height = 0;
    packer.getDimensions(width, height);

    // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
    ImmediateAtlasGenerator<
        float,          // pixel type of buffer for individual glyphs depends on generator function
        3,              // number of atlas color channels
        &msdfGenerator, // function to generate bitmaps for individual glyphs
        BitmapAtlasStorage<byte, 3> // class that stores the atlas bitmap
        // For example, a custom atlas storage class that stores it in VRAM can be used.
        >
        generator(width, height);

    // GeneratorAttributes can be modified to change the generator's default settings.
    GeneratorAttributes attributes;
    generator.setAttributes(attributes);
    generator.setThreadCount(4);

    // Generate atlas bitmap
    generator.generate(atlas->glyphs.data(), atlas->glyphs.size());

    // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
    // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
    BitmapConstRef<unsigned char, 3> bitmap = generator.atlasStorage();

    uint32_t w = atlas->width = (uint32_t)bitmap.width;
    uint32_t h = atlas->height = (uint32_t)bitmap.height;
    DvzSize size = atlas->width * atlas->height * 3;

    // Make a copy of the buffer in the DvzAtlas structure.
    if (atlas->rgb != NULL)
    {
        FREE(atlas->rgb);
    }
    atlas->rgb = (uint8_t*)malloc(size);

    uint32_t x, y, u, i, j;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            i = 3 * (y * w + x);
            j = 3 * ((h - 1 - y) * w + (x));

            for (u = 0; u < 3; u++)
            {
                atlas->rgb[j + u] = bitmap.pixels[i + u];
            }
        }
    }
    return 0;
}



void dvz_atlas_shape(DvzAtlas* atlas, uvec3 shape)
{
    ANN(atlas);
    shape[0] = atlas->width;
    shape[1] = atlas->height;
    shape[2] = 1;
}



bool dvz_atlas_valid(DvzAtlas* atlas)
{
    ANN(atlas);
    return (atlas->width > 0) && (atlas->height > 0) && (atlas->rgb != NULL);
}



// The caller MUST NOT free the output, it is owned by the atlas and the pointer will be freed on
// atlas destruction.
uint8_t* dvz_atlas_rgb(DvzAtlas* atlas)
{
    ANN(atlas);
    return atlas->rgb;
}



void dvz_atlas_png(DvzAtlas* atlas, const char* png_filename)
{
    ANN(atlas);
    ASSERT(dvz_atlas_valid(atlas));
    dvz_write_png(png_filename, atlas->width, atlas->height, atlas->rgb);
}



DvzId dvz_atlas_texture(DvzAtlas* atlas, DvzBatch* batch)
{
    ANN(atlas);

    if (atlas->rgb == NULL)
    {
        log_error("unable to create the atlas texture, the atlas has not been created yet");
        return DVZ_ID_NONE;
    }

    uvec3 shape = {0};
    dvz_atlas_shape(atlas, shape);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] == 1);

    // TODO: mtsdf with 4 channels
    DvzId tex = dvz_create_tex(batch, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, shape, 0).id;

    DvzSize size = atlas->width * atlas->height * 4;

    ANN(atlas->rgb);

    // HACK: Vulkan does not support RGB textures so we create a RGBA texture with a full alpha
    // channel.
    cvec4* rgba = (cvec4*)calloc(atlas->width * atlas->height, sizeof(cvec4));
    for (uint32_t i = 0; i < atlas->width * atlas->height; i++)
    {
        rgba[i][0] = atlas->rgb[3 * i + 0];
        rgba[i][1] = atlas->rgb[3 * i + 1];
        rgba[i][2] = atlas->rgb[3 * i + 2];
        rgba[i][3] = 255;
    }
    dvz_upload_tex(batch, tex, DVZ_ZERO_OFFSET, shape, size, rgba);
    FREE(rgba);

    return tex;
}



void dvz_atlas_destroy(DvzAtlas* atlas)
{
    ANN(atlas);

    if (atlas->codepoints != NULL)
    {
        FREE(atlas->codepoints);
    }
    if (atlas->rgb != NULL)
    {
        FREE(atlas->rgb);
    }

    destroyFont(atlas->font);
    deinitializeFreetype(atlas->ft);
    FREE(atlas);
}
