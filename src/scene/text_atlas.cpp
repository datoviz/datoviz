/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text atlas                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <limits.h>
#include <stdint.h>
#include <vector>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"

#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdf-atlas-gen/types.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wundef"
#endif
#define STBTT_STATIC
#define STBTT_malloc(x, u) ((void)(u), dvz_malloc((DvzSize)(x)))
#define STBTT_free(x, u)   ((void)(u), dvz_free(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "imgui/imstb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEXT_SDF_FIRST_CHAR 32u
#define DVZ_TEXT_SDF_LAST_CHAR  126u
#define DVZ_TEXT_SDF_FALLBACK   63u
#define DVZ_TEXT_SDF_COLUMNS    16u
#define DVZ_TEXT_SDF_PADDING    8
#define DVZ_TEXT_SDF_CELL_GAP   2u
#define DVZ_TEXT_SDF_ONEDGE     128u
#define DVZ_TEXT_BITMAP_PADDING 1u



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static bool _text_sdf_codepoint_supported(uint32_t codepoint);
static float _text_sdf_pixel_height(float size_pts);
static bool _text_sdf_font_bytes(DvzFont* font);
static bool _text_atlas_upload_rgba(
    const DvzFont* font, DvzTextAtlas* atlas, uint8_t* rgba, uint32_t width, uint32_t height);


#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
/**
 * Build an RGB MSDF atlas with msdf-atlas-gen.
 *
 * @param font the font
 * @param out_atlas output atlas metadata
 * @return whether atlas creation succeeded
 */
static bool _text_msdf_build_atlas(DvzFont* font, DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(out_atlas);
    *out_atlas = NULL;
    if (!_text_sdf_font_bytes(font))
        return false;

    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (ft == NULL)
    {
        log_error("failed to initialize msdfgen FreeType handle");
        return false;
    }
#if OS_WINDOWS
    msdfgen::FontHandle* msdf_font =
        msdfgen::loadFontData(ft, (const unsigned char*)font->ttf_bytes, (int)font->ttf_size);
#else
    msdfgen::FontHandle* msdf_font =
        msdfgen::loadFontData(ft, (const unsigned char*)font->ttf_bytes, font->ttf_size);
#endif
    if (msdf_font == NULL)
    {
        log_error("failed to load msdfgen font data");
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry font_geometry(&glyphs);
    msdf_atlas::Charset charset;
    const uint32_t glyph_count = DVZ_TEXT_SDF_LAST_CHAR - DVZ_TEXT_SDF_FIRST_CHAR + 1u;
    for (uint32_t i = 0; i < glyph_count; i++)
        charset.add(DVZ_TEXT_SDF_FIRST_CHAR + i);

    if (font_geometry.loadCharset(msdf_font, 1.0, charset) <= 0 || glyphs.empty())
    {
        log_error("failed to load MSDF charset");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    const double max_corner_angle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, max_corner_angle, 0);

    float pixel_height = _text_sdf_pixel_height(font->size_pts);
    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale((double)pixel_height);
    packer.setPixelRange(4.0);
    packer.setMiterLimit(1.0);
    if (packer.pack(glyphs.data(), (int)glyphs.size()) != 0)
    {
        log_error("failed to pack MSDF atlas");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    int width = 0;
    int height = 0;
    packer.getDimensions(width, height);
    if (width <= 0 || height <= 0)
    {
        log_error("MSDF atlas has invalid dimensions");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    msdf_atlas::ImmediateAtlasGenerator<
        float, 4, &msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<uint8_t, 4>>
        generator(width, height);
    msdf_atlas::GeneratorAttributes attributes;
    generator.setAttributes(attributes);
    generator.setThreadCount(8);
    generator.generate(glyphs.data(), glyphs.size());
    msdfgen::BitmapConstRef<uint8_t, 4> bitmap = generator.atlasStorage();

    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)width, (uint64_t)height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || byte_size > SIZE_MAX)
    {
        log_error("MSDF atlas byte size overflow");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("MSDF atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width;
    uint32_t atlas_height = (uint32_t)height;
    for (uint32_t y = 0; y < atlas_height; y++)
    {
        for (uint32_t x = 0; x < atlas_width; x++)
        {
            uint64_t src = ((uint64_t)y * atlas_width + x) * 4u;
            uint64_t dst = ((uint64_t)(atlas_height - 1u - y) * atlas_width + x) * 4u;
            rgba[dst + 0] = bitmap.pixels[src + 0];
            rgba[dst + 1] = bitmap.pixels[src + 1];
            rgba[dst + 2] = bitmap.pixels[src + 2];
            rgba[dst + 3] = bitmap.pixels[src + 3];
        }
    }

    const double scale = packer.getScale();
    const msdfgen::FontMetrics& metrics = font_geometry.getMetrics();
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->pixel_height = (float)scale;
    atlas->pixel_range = 4.0f;
    atlas->ascent = (float)(metrics.ascenderY * scale);
    atlas->descent = (float)(metrics.descenderY * scale);
    atlas->line_gap = (float)((metrics.lineHeight - metrics.ascenderY + metrics.descenderY) * scale);
    atlas->line_height = (float)(metrics.lineHeight * scale);
    if (atlas->line_height <= 0.0f)
        atlas->line_height = (float)scale;

    for (const msdf_atlas::GlyphGeometry& src_glyph : glyphs)
    {
        uint32_t codepoint = (uint32_t)src_glyph.getCodepoint();
        if (!_text_sdf_codepoint_supported(codepoint))
            continue;
        uint32_t index = codepoint - DVZ_TEXT_SDF_FIRST_CHAR;
        if (index >= DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
            continue;
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[index];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)src_glyph.getIndex();
        glyph->advance = (float)(src_glyph.getAdvance() * scale);

        double l = 0.0;
        double b = 0.0;
        double r = 0.0;
        double t = 0.0;
        src_glyph.getQuadPlaneBounds(l, b, r, t);

        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        src_glyph.getBoxRect(x, y, w, h);
        if (w <= 0 || h <= 0)
        {
            glyph->valid = glyph->advance > 0.0f;
            continue;
        }

        const double inset_x = 0.5;
        const double inset_y = 0.5;
        double plane_l = l;
        double plane_b = b;
        double plane_r = r;
        double plane_t = t;
        if (w > 2 && h > 2)
        {
            double fx = inset_x / (double)w;
            double fy = inset_y / (double)h;
            double plane_w = r - l;
            double plane_h = t - b;
            plane_l += fx * plane_w;
            plane_r -= fx * plane_w;
            plane_b += fy * plane_h;
            plane_t -= fy * plane_h;
        }

        glyph->xoff = (float)(plane_l * scale);
        glyph->yoff = (float)(-plane_t * scale);
        glyph->width = (float)((plane_r - plane_l) * scale);
        glyph->height = (float)((plane_t - plane_b) * scale);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;

        uint32_t top_y = atlas_height - (uint32_t)y - (uint32_t)h;
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)top_y;
        glyph->atlas_bounds[2] = (float)(x + w);
        glyph->atlas_bounds[3] = (float)(top_y + (uint32_t)h);
        float pad_x = w > 2 ? (float)inset_x : 0.0f;
        float pad_y = h > 2 ? (float)inset_y : 0.0f;
        glyph->uv[0] = ((float)x + pad_x) / (float)atlas_width;
        glyph->uv[1] = ((float)top_y + pad_y) / (float)atlas_height;
        glyph->uv[2] = ((float)(x + w) - pad_x) / (float)atlas_width;
        glyph->uv[3] = ((float)(top_y + (uint32_t)h) - pad_y) / (float)atlas_height;
        glyph->valid = true;
    }

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    msdfgen::destroyFont(msdf_font);
    msdfgen::deinitializeFreetype(ft);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    return true;
}
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a codepoint belongs to the first atlas charset.
 *
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint is in the printable ASCII atlas range
 */
static bool _text_sdf_codepoint_supported(uint32_t codepoint)
{
    return codepoint >= DVZ_TEXT_SDF_FIRST_CHAR && codepoint <= DVZ_TEXT_SDF_LAST_CHAR;
}



/**
 * Clamp the requested atlas font size to a practical SDF generation range.
 *
 * @param size_pts requested font size in pixels/points
 * @return clamped atlas pixel height
 */
static float _text_sdf_pixel_height(float size_pts)
{
    float pixel_height = size_pts > 0.0f ? size_pts : 32.0f;
    if (pixel_height < 8.0f)
        pixel_height = 8.0f;
    if (pixel_height > 128.0f)
        pixel_height = 128.0f;
    return pixel_height;
}



/**
 * Try to load a font file from a small list of repository-relative defaults.
 *
 * @param out_size output byte size
 * @return owned TTF bytes, or NULL when no default font could be loaded
 */
static void* _text_sdf_load_default_font(DvzSize* out_size)
{
    ANN(out_size);
#if defined(DVZ_HAS_EMBEDDED_FONTS) && DVZ_HAS_EMBEDDED_FONTS
    unsigned long embedded_size = 0;
    const unsigned char* embedded = dvz_resource_font("Roboto_Regular", &embedded_size);
    if (embedded != NULL && embedded_size > 0)
    {
        void* bytes = dvz_malloc((DvzSize)embedded_size);
        if (bytes != NULL)
        {
            dvz_memcpy(bytes, (size_t)embedded_size, embedded, (size_t)embedded_size);
            *out_size = (DvzSize)embedded_size;
            return bytes;
        }
    }
#endif
    const char* paths[] = {
        "data/fonts/Roboto-Regular.ttf",
        "data/fonts/Roboto-Medium.ttf",
        "external/imgui/misc/fonts/Roboto-Medium.ttf",
        "external/cimgui/imgui/misc/fonts/Roboto-Medium.ttf",
    };
    for (uint32_t i = 0; i < (uint32_t)(sizeof(paths) / sizeof(paths[0])); i++)
    {
        DvzSize size = 0;
        void* bytes = dvz_read_file(paths[i], &size);
        if (bytes != NULL && size > 0)
        {
            *out_size = size;
            return bytes;
        }
        if (bytes != NULL)
            dvz_free(bytes);
    }
    return NULL;
}



/**
 * Ensure a font has owned TrueType bytes available.
 *
 * @param font the font
 * @return whether TTF bytes are available
 */
static bool _text_sdf_font_bytes(DvzFont* font)
{
    ANN(font);
    if (font->ttf_bytes != NULL && font->ttf_size > 0)
        return true;

    DvzSize size = 0;
    void* bytes = NULL;
    if (font->path[0] != '\0')
        bytes = dvz_read_file(font->path, &size);
    else
        bytes = _text_sdf_load_default_font(&size);

    if (bytes == NULL || size == 0)
    {
        if (bytes != NULL)
            dvz_free(bytes);
        log_error("failed to load text font bytes");
        return false;
    }
    font->ttf_bytes = bytes;
    font->ttf_size = (uint64_t)size;
    return true;
}



/**
 * Copy one generated SDF glyph into the packed RGBA atlas.
 *
 * @param atlas_rgba destination RGBA atlas bytes
 * @param atlas_width destination atlas width
 * @param dst_x destination glyph x origin
 * @param dst_y destination glyph y origin
 * @param glyph_sdf source SDF alpha bytes
 * @param glyph_width source glyph width
 * @param glyph_height source glyph height
 */
static void _text_sdf_copy_glyph(
    uint8_t* atlas_rgba, uint32_t atlas_width, uint32_t dst_x, uint32_t dst_y,
    const uint8_t* glyph_sdf, uint32_t glyph_width, uint32_t glyph_height)
{
    ANN(atlas_rgba);
    ANN(glyph_sdf);
    for (uint32_t y = 0; y < glyph_height; y++)
    {
        for (uint32_t x = 0; x < glyph_width; x++)
        {
            uint64_t dst = ((uint64_t)(dst_y + y) * atlas_width + dst_x + x) * 4u;
            uint64_t src = (uint64_t)y * glyph_width + x;
            atlas_rgba[dst + 0] = 0;
            atlas_rgba[dst + 1] = 0;
            atlas_rgba[dst + 2] = 0;
            atlas_rgba[dst + 3] = glyph_sdf[src];
        }
    }
}



/**
 * Initialize the stb_truetype font object.
 *
 * @param font the scene font
 * @param out_info output stb font info
 * @return whether initialization succeeded
 */
static bool _text_sdf_init_font(const DvzFont* font, stbtt_fontinfo* out_info)
{
    ANN(font);
    ANN(out_info);
    if (font->ttf_bytes == NULL || font->ttf_size == 0 || font->ttf_size > INT32_MAX)
        return false;
    const unsigned char* bytes = (const unsigned char*)font->ttf_bytes;
    int offset = stbtt_GetFontOffsetForIndex(bytes, (int)font->face_index);
    if (offset < 0)
    {
        log_error("failed to find requested TrueType face index %u", font->face_index);
        return false;
    }
    if (stbtt_InitFont(out_info, bytes, offset) == 0)
    {
        log_error("failed to initialize TrueType font");
        return false;
    }
    return true;
}



/**
 * Convert an SDF value to an atlas threshold scaling constant.
 *
 * @return pixel distance scale passed to stb_truetype SDF generation
 */
static float _text_sdf_pixel_dist_scale(void)
{
    return (float)DVZ_TEXT_SDF_ONEDGE / (float)DVZ_TEXT_SDF_PADDING;
}


/**
 * Return the cache slot used by a text atlas backend.
 *
 * @param font the font
 * @param backend the requested backend
 * @return pointer to the font-owned atlas cache slot
 */
static DvzTextAtlas** _text_atlas_slot(DvzFont* font, DvzTextAtlasBackend backend)
{
    ANN(font);
    switch (backend)
    {
    case DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP:
        return &font->bitmap_atlas;
    case DVZ_TEXT_ATLAS_BACKEND_MSDF:
        return &font->msdf_atlas;
    case DVZ_TEXT_ATLAS_BACKEND_STB_SDF:
    case DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP:
    default:
        return &font->sdf_atlas;
    }
}



/**
 * Upload an RGBA atlas payload into a scene sampled field.
 *
 * @param font the font whose scene owns the field
 * @param atlas the atlas metadata
 * @param rgba the atlas bytes
 * @param width atlas width
 * @param height atlas height
 * @return whether upload succeeded
 */
static bool _text_atlas_upload_rgba(
    const DvzFont* font, DvzTextAtlas* atlas, uint8_t* rgba, uint32_t width,
    uint32_t height)
{
    ANN(font);
    ANN(atlas);
    ANN(rgba);
    DvzSampledFieldDesc desc = {};
    desc.dim = DVZ_FIELD_DIM_2D;
    desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
    desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    DvzSampledField* field = dvz_sampled_field(font->scene, &desc);
    DvzFieldDataView view = {};
    view.data = rgba;
    view.bytes_per_row = (uint64_t)width * 4u;
    view.rows_per_image = height;
    if (field == NULL || !dvz_sampled_field_set_data(field, &view))
        return false;
    atlas->field = field;
    return true;
}



#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
/**
 * Copy one FreeType glyph bitmap into an RGBA alpha atlas.
 *
 * @param atlas_rgba destination RGBA atlas bytes
 * @param atlas_width destination atlas width
 * @param dst_x destination glyph x origin
 * @param dst_y destination glyph y origin
 * @param bitmap source FreeType bitmap
 */
static void _text_ft_copy_bitmap(
    uint8_t* atlas_rgba, uint32_t atlas_width, uint32_t dst_x, uint32_t dst_y,
    const FT_Bitmap* bitmap)
{
    ANN(atlas_rgba);
    ANN(bitmap);
    for (uint32_t y = 0; y < (uint32_t)bitmap->rows; y++)
    {
        for (uint32_t x = 0; x < (uint32_t)bitmap->width; x++)
        {
            uint8_t coverage = 0;
            if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
            {
                const uint8_t* row = bitmap->buffer + (uint64_t)y * (uint32_t)bitmap->pitch;
                coverage = row[x];
            }
            else if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
            {
                const uint8_t* row = bitmap->buffer + (uint64_t)y * (uint32_t)bitmap->pitch;
                coverage = (row[x / 8u] & (uint8_t)(0x80u >> (x % 8u))) != 0 ? 255 : 0;
            }
            uint64_t dst = ((uint64_t)(dst_y + y) * atlas_width + dst_x + x) * 4u;
            atlas_rgba[dst + 0] = 0;
            atlas_rgba[dst + 1] = 0;
            atlas_rgba[dst + 2] = 0;
            atlas_rgba[dst + 3] = coverage;
        }
    }
}



/**
 * Build a hinted FreeType bitmap atlas for printable ASCII glyphs.
 *
 * @param font the font
 * @param out_atlas output atlas metadata
 * @return whether atlas creation succeeded
 */
static bool _text_ft_build_bitmap_atlas(DvzFont* font, DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(out_atlas);
    *out_atlas = NULL;
    if (!_text_sdf_font_bytes(font))
        return false;

    FT_Library library = NULL;
    FT_Face face = NULL;
    if (FT_Init_FreeType(&library) != 0)
    {
        log_error("failed to initialize FreeType");
        return false;
    }
    if (FT_New_Memory_Face(
            library, (const FT_Byte*)font->ttf_bytes, (FT_Long)font->ttf_size,
            (FT_Long)font->face_index, &face) != 0)
    {
        log_error("failed to load FreeType font face");
        FT_Done_FreeType(library);
        return false;
    }

    float pixel_height = _text_sdf_pixel_height(font->size_pts);
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)pixel_height) != 0)
    {
        log_error("failed to set FreeType pixel size");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    const uint32_t glyph_count = DVZ_TEXT_SDF_LAST_CHAR - DVZ_TEXT_SDF_FIRST_CHAR + 1u;
    uint32_t glyph_widths[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_heights[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_lefts[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_tops[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    float glyph_advances[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        if (FT_Load_Char(face, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        glyph_widths[i] = (uint32_t)face->glyph->bitmap.width;
        glyph_heights[i] = (uint32_t)face->glyph->bitmap.rows;
        glyph_lefts[i] = face->glyph->bitmap_left;
        glyph_tops[i] = face->glyph->bitmap_top;
        glyph_advances[i] = (float)face->glyph->advance.x / 64.0f;
        if (glyph_widths[i] > cell_width)
            cell_width = glyph_widths[i];
        if (glyph_heights[i] > cell_height)
            cell_height = glyph_heights[i];
    }

    cell_width += 2u * DVZ_TEXT_BITMAP_PADDING;
    cell_height += 2u * DVZ_TEXT_BITMAP_PADDING;
    uint32_t rows = (glyph_count + DVZ_TEXT_SDF_COLUMNS - 1u) / DVZ_TEXT_SDF_COLUMNS;
    uint64_t width64 = 0;
    uint64_t height64 = 0;
    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(DVZ_TEXT_SDF_COLUMNS, cell_width, &width64) ||
        _dvz_mul_u64_overflows(rows, cell_height, &height64) ||
        _dvz_mul_u64_overflows(width64, height64, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || width64 > UINT32_MAX ||
        height64 > UINT32_MAX || byte_size > SIZE_MAX)
    {
        log_error("text FreeType atlas dimensions overflow");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width64;
    uint32_t atlas_height = (uint32_t)height64;
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("text FreeType atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->pixel_height = pixel_height;
    atlas->ascent = face->size != NULL ? (float)face->size->metrics.ascender / 64.0f : pixel_height;
    atlas->descent = face->size != NULL ? (float)face->size->metrics.descender / 64.0f : 0.0f;
    atlas->line_gap = 0.0f;
    atlas->line_height =
        face->size != NULL ? (float)face->size->metrics.height / 64.0f : pixel_height;
    if (atlas->line_height <= 0.0f)
        atlas->line_height = pixel_height;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)FT_Get_Char_Index(face, (FT_ULong)codepoint);
        glyph->advance = glyph_advances[i];
        glyph->xoff = (float)glyph_lefts[i];
        glyph->yoff = -(float)glyph_tops[i];
        glyph->width = (float)glyph_widths[i];
        glyph->height = (float)glyph_heights[i];
        uint32_t col = i % DVZ_TEXT_SDF_COLUMNS;
        uint32_t row = i / DVZ_TEXT_SDF_COLUMNS;
        uint32_t x = col * cell_width + DVZ_TEXT_BITMAP_PADDING;
        uint32_t y = row * cell_height + DVZ_TEXT_BITMAP_PADDING;
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)y;
        glyph->atlas_bounds[2] = (float)(x + glyph_widths[i]);
        glyph->atlas_bounds[3] = (float)(y + glyph_heights[i]);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;
        glyph->uv[0] = (float)x / (float)atlas_width;
        glyph->uv[1] = (float)y / (float)atlas_height;
        glyph->uv[2] = (float)(x + glyph_widths[i]) / (float)atlas_width;
        glyph->uv[3] = (float)(y + glyph_heights[i]) / (float)atlas_height;
        glyph->valid = glyph->advance > 0.0f || glyph_widths[i] > 0 || glyph_heights[i] > 0;

        if (glyph_widths[i] == 0 || glyph_heights[i] == 0)
            continue;
        if (FT_Load_Char(face, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        _text_ft_copy_bitmap(rgba, atlas_width, x, y, &face->glyph->bitmap);
    }

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    return true;
}
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

extern "C" {

/**
 * Ensure one font has a scene-owned font atlas.
 *
 * @param font the scene font
 * @param backend the requested atlas backend
 * @return whether the atlas is available
 */
bool _scene_text_atlas_ensure(DvzFont* font, DvzTextAtlasBackend backend)
{
    ANN(font);
    DvzTextAtlas** slot = _text_atlas_slot(font, backend);
    if (*slot != NULL && (*slot)->field != NULL)
        return true;
    if (font->scene == NULL)
        return false;
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    if (backend == DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP)
    {
        if (_text_ft_build_bitmap_atlas(font, slot))
        {
            font->version++;
            return true;
        }
        log_debug("FreeType bitmap atlas generation failed; falling back to SDF atlas");
        slot = _text_atlas_slot(font, DVZ_TEXT_ATLAS_BACKEND_STB_SDF);
        if (*slot != NULL && (*slot)->field != NULL)
            return true;
    }
#else
    if (backend == DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP)
    {
        log_debug("FreeType bitmap atlas requested but Datoviz was built without FreeType");
        slot = _text_atlas_slot(font, DVZ_TEXT_ATLAS_BACKEND_STB_SDF);
        if (*slot != NULL && (*slot)->field != NULL)
            return true;
    }
#endif
    if (backend == DVZ_TEXT_ATLAS_BACKEND_MSDF)
    {
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
        if (_text_msdf_build_atlas(font, slot))
        {
            font->version++;
            return true;
        }
        log_debug("MSDF atlas generation failed; falling back to SDF atlas");
#else
        log_debug("MSDF atlas requested but msdf-atlas-gen is unavailable; falling back to SDF atlas");
#endif
        slot = _text_atlas_slot(font, DVZ_TEXT_ATLAS_BACKEND_STB_SDF);
        if (*slot != NULL && (*slot)->field != NULL)
            return true;
    }
    if (!_text_sdf_font_bytes(font))
        return false;

    stbtt_fontinfo info = {};
    if (!_text_sdf_init_font(font, &info))
        return false;

    const float pixel_height = _text_sdf_pixel_height(font->size_pts);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
    const uint32_t glyph_count = DVZ_TEXT_SDF_LAST_CHAR - DVZ_TEXT_SDF_FIRST_CHAR + 1u;

    uint8_t* glyph_sdfs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_widths[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_heights[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_xoffs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_yoffs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        int width = 0;
        int height = 0;
        int xoff = 0;
        int yoff = 0;
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        unsigned char* sdf = stbtt_GetCodepointSDF(
            &info, scale, (int)codepoint, DVZ_TEXT_SDF_PADDING,
            (unsigned char)DVZ_TEXT_SDF_ONEDGE, _text_sdf_pixel_dist_scale(), &width, &height,
            &xoff, &yoff);
        if (sdf == NULL || width <= 0 || height <= 0)
            continue;
        glyph_sdfs[i] = sdf;
        glyph_widths[i] = (uint32_t)width;
        glyph_heights[i] = (uint32_t)height;
        glyph_xoffs[i] = xoff;
        glyph_yoffs[i] = yoff;
        if ((uint32_t)width > cell_width)
            cell_width = (uint32_t)width;
        if ((uint32_t)height > cell_height)
            cell_height = (uint32_t)height;
    }

    cell_width += DVZ_TEXT_SDF_CELL_GAP;
    cell_height += DVZ_TEXT_SDF_CELL_GAP;
    uint32_t rows = (glyph_count + DVZ_TEXT_SDF_COLUMNS - 1u) / DVZ_TEXT_SDF_COLUMNS;
    uint64_t width64 = 0;
    uint64_t height64 = 0;
    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(DVZ_TEXT_SDF_COLUMNS, cell_width, &width64) ||
        _dvz_mul_u64_overflows(rows, cell_height, &height64) ||
        _dvz_mul_u64_overflows(width64, height64, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || width64 > UINT32_MAX ||
        height64 > UINT32_MAX || byte_size > SIZE_MAX)
    {
        log_error("text SDF atlas dimensions overflow");
        for (uint32_t i = 0; i < glyph_count; i++)
            if (glyph_sdfs[i] != NULL)
                stbtt_FreeSDF(glyph_sdfs[i], NULL);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width64;
    uint32_t atlas_height = (uint32_t)height64;
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("text SDF atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        for (uint32_t i = 0; i < glyph_count; i++)
            if (glyph_sdfs[i] != NULL)
                stbtt_FreeSDF(glyph_sdfs[i], NULL);
        return false;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_STB_SDF;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->pixel_height = pixel_height;
    atlas->pixel_range = (float)DVZ_TEXT_SDF_PADDING;
    atlas->ascent = (float)ascent * scale;
    atlas->descent = (float)descent * scale;
    atlas->line_gap = (float)line_gap * scale;
    atlas->line_height = (float)(ascent - descent + line_gap) * scale;
    if (atlas->line_height <= 0.0f)
        atlas->line_height = pixel_height;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)stbtt_FindGlyphIndex(&info, (int)codepoint);
        int advance = 0;
        int left_bearing = 0;
        stbtt_GetCodepointHMetrics(&info, (int)codepoint, &advance, &left_bearing);
        (void)left_bearing;
        glyph->advance = (float)advance * scale;
        if (glyph_sdfs[i] == NULL)
        {
            glyph->valid = glyph->advance > 0.0f;
            continue;
        }

        uint32_t col = i % DVZ_TEXT_SDF_COLUMNS;
        uint32_t row = i / DVZ_TEXT_SDF_COLUMNS;
        uint32_t x = col * cell_width;
        uint32_t y = row * cell_height;
        _text_sdf_copy_glyph(
            rgba, atlas_width, x, y, glyph_sdfs[i], glyph_widths[i], glyph_heights[i]);
        for (uint32_t gy = 0; gy < glyph_heights[i]; gy++)
        {
            for (uint32_t gx = 0; gx < glyph_widths[i]; gx++)
            {
                uint64_t index = ((uint64_t)(y + gy) * atlas_width + x + gx) * 4u;
                rgba[index + 0] = rgba[index + 3];
                rgba[index + 1] = rgba[index + 3];
                rgba[index + 2] = rgba[index + 3];
            }
        }

        glyph->xoff = (float)glyph_xoffs[i];
        glyph->yoff = (float)glyph_yoffs[i];
        glyph->width = (float)glyph_widths[i];
        glyph->height = (float)glyph_heights[i];
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)y;
        glyph->atlas_bounds[2] = (float)(x + glyph_widths[i]);
        glyph->atlas_bounds[3] = (float)(y + glyph_heights[i]);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;
        glyph->uv[0] = (float)x / (float)atlas_width;
        glyph->uv[1] = (float)y / (float)atlas_height;
        glyph->uv[2] = (float)(x + glyph_widths[i]) / (float)atlas_width;
        glyph->uv[3] = (float)(y + glyph_heights[i]) / (float)atlas_height;
        glyph->valid = true;
    }

    for (uint32_t i = 0; i < glyph_count; i++)
        if (glyph_sdfs[i] != NULL)
            stbtt_FreeSDF(glyph_sdfs[i], NULL);

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *slot = atlas;
    font->version++;
    return true;
}



/**
 * Return one atlas glyph, falling back to '?' for unsupported codepoints.
 *
 * @param atlas the text atlas
 * @param codepoint the Unicode codepoint
 * @return the glyph metadata, or NULL when unavailable
 */
DvzTextAtlasGlyph* _scene_text_atlas_glyph(DvzTextAtlas* atlas, uint32_t codepoint)
{
    ANN(atlas);
    if (!_text_sdf_codepoint_supported(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;
    uint32_t index = codepoint - DVZ_TEXT_SDF_FIRST_CHAR;
    if (index >= atlas->glyph_count || index >= DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return NULL;
    DvzTextAtlasGlyph* glyph = &atlas->glyphs[index];
    return glyph->valid ? glyph : NULL;
}



/**
 * Destroy an SDF atlas object.
 *
 * @param atlas the atlas
 */
void _scene_text_atlas_destroy(DvzTextAtlas* atlas)
{
    if (atlas == NULL)
        return;
    if (atlas->field != NULL)
    {
        dvz_sampled_field_destroy(atlas->field);
        atlas->field = NULL;
    }
    dvz_free(atlas);
}



/**
 * Release private font resources.
 *
 * @param font the font
 */
void _scene_font_release(DvzFont* font)
{
    if (font == NULL)
        return;
    if (font->bitmap_atlas != NULL)
    {
        _scene_text_atlas_destroy(font->bitmap_atlas);
        font->bitmap_atlas = NULL;
    }
    if (font->sdf_atlas != NULL)
    {
        _scene_text_atlas_destroy(font->sdf_atlas);
        font->sdf_atlas = NULL;
    }
    if (font->msdf_atlas != NULL)
    {
        _scene_text_atlas_destroy(font->msdf_atlas);
        font->msdf_atlas = NULL;
    }
    if (font->ttf_bytes != NULL)
    {
        dvz_free(font->ttf_bytes);
        font->ttf_bytes = NULL;
        font->ttf_size = 0;
    }
    font->scene = NULL;
}

}
