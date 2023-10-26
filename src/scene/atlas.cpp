/*************************************************************************************************/
/*  Atlas                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/atlas.h"
#include "_macros.h"

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
    FreetypeHandle* ft;
    FontHandle* font;
    std::vector<GlyphGeometry> glyphs;
    // FontGeometry fontGeometry;
    TightAtlasPacker packer;
};



/*************************************************************************************************/
/*  Atlas functions                                                                              */
/*************************************************************************************************/

static bool generateAtlas(const char* fontFilename)
{
    bool success = false;

    // Initialize instance of FreeType library
    if (FreetypeHandle* ft = initializeFreetype())
    {
        // Load font file
        if (FontHandle* font = loadFont(ft, fontFilename))
        {
            // Storage for glyph geometry and their coordinates in the atlas
            std::vector<GlyphGeometry> glyphs;

            // FontGeometry is a helper class that loads a set of glyphs from a single font.
            // It can also be used to get additional font metrics, kerning information, etc.
            FontGeometry fontGeometry(&glyphs);

            // Load a set of character glyphs:
            // The second argument can be ignored unless you mix different font sizes in one atlas.
            // In the last argument, you can specify a charset other than ASCII.
            // To load specific glyph indices, use loadGlyphs instead.

            // NOTE: to specify a range of codepoints, create a manual charset

            fontGeometry.loadCharset(font, 1.0, Charset::ASCII);

            // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
            const double maxCornerAngle = 3.0;
            for (GlyphGeometry& glyph : glyphs)
                glyph.edgeColoring(&edgeColoringInkTrap, maxCornerAngle, 0);

            // TightAtlasPacker class computes the layout of the atlas.
            TightAtlasPacker packer;

            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);

            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(24.0);

            // setPixelRange or setUnitRange
            packer.setPixelRange(2.0);
            packer.setMiterLimit(1.0);

            // Compute atlas layout - pack glyphs
            packer.pack(glyphs.data(), glyphs.size());

            // Get final atlas dimensions
            int width = 0, height = 0;
            packer.getDimensions(width, height);

            // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
            ImmediateAtlasGenerator<
                float, // pixel type of buffer for individual glyphs depends on generator function
                3,     // number of atlas color channels
                &msdfGenerator,             // function to generate bitmaps for individual glyphs
                BitmapAtlasStorage<byte, 3> // class that stores the atlas bitmap
                // For example, a custom atlas storage class that stores it in VRAM can be used.
                >
                generator(width, height);

            // GeneratorAttributes can be modified to change the generator's default settings.
            GeneratorAttributes attributes;
            generator.setAttributes(attributes);
            generator.setThreadCount(4);

            // Generate atlas bitmap
            generator.generate(glyphs.data(), glyphs.size());

            // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
            // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
            auto bitmap = generator.atlasStorage();

            // Cleanup
            destroyFont(font);
        }
        deinitializeFreetype(ft);
    }
    return success;
}



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



void dvz_atlas_set(DvzAtlas* atlas, uint32_t count, const uint32_t* codepoints)
{
    ANN(atlas);

    // TODO
}



void dvz_atlas_string(DvzAtlas* atlas, const char* string)
{
    ANN(atlas);

    // TODO
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
    out_coords[1] = (float)y;
    out_coords[2] = (float)w;
    out_coords[3] = (float)h;

    return 0;
}



int dvz_atlas_glyphs(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints, vec4* out_coords)
{
    ANN(atlas); //
    return 0;
}



int dvz_atlas_generate(DvzAtlas* atlas)
{
    ANN(atlas);

    // FontGeometry is a helper class that loads a set of glyphs from a single font.
    // It can also be used to get additional font metrics, kerning information, etc.
    FontGeometry fontGeometry(&atlas->glyphs);

    // Load a set of character glyphs:
    // The second argument can be ignored unless you mix different font sizes in one atlas.
    // In the last argument, you can specify a charset other than ASCII.
    // To load specific glyph indices, use loadGlyphs instead.

    // NOTE: to specify a range of codepoints, create a manual charset
    // TODO
    fontGeometry.loadCharset(atlas->font, 1.0, Charset::ASCII);

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
    packer.setMinimumScale(24.0);

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
    auto bitmap = generator.atlasStorage();

    return 0;
}



void dvz_atlas_destroy(DvzAtlas* atlas)
{
    ANN(atlas);
    destroyFont(atlas->font);
    deinitializeFreetype(atlas->ft);
    FREE(atlas);
}
