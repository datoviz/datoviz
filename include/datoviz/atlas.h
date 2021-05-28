/*************************************************************************************************/
/*  Simple monospace font atlas                                                                  */
/*************************************************************************************************/

#ifndef DVZ_FONT_ATLAS_HEADER
#define DVZ_FONT_ATLAS_HEADER

#include "common.h"
#include "context.h"

// #define STB_IMAGE_IMPLEMENTATION
BEGIN_INCL_NO_WARN
#include "../external/stb_image.h"
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Font atlas                                                                                   */
/*************************************************************************************************/

static const char DVZ_FONT_ATLAS_STRING[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";



static size_t _font_atlas_glyph(DvzFontAtlas* atlas, const char* str, uint32_t idx)
{
    ASSERT(atlas != NULL);
    ASSERT(atlas->rows > 0);
    ASSERT(atlas->cols > 0);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    ASSERT(idx < strlen(str));
    ASSERT(atlas->font_str != NULL);
    ASSERT(strlen(atlas->font_str) > 0);

    char c[2] = {str[idx], 0};
    return strcspn(atlas->font_str, c);
}



static void _font_atlas_glyph_size(DvzFontAtlas* atlas, float size, vec2 glyph_size)
{
    ASSERT(atlas != NULL);
    glyph_size[0] = size * atlas->glyph_width / atlas->glyph_height;
    glyph_size[1] = size;
}



static DvzTexture* _font_texture(DvzContext* ctx, DvzFontAtlas* atlas)
{
    ASSERT(ctx != NULL);
    ASSERT(atlas != NULL);
    ASSERT(atlas->font_texture != NULL);

    uvec3 shape = {(uint32_t)atlas->width, (uint32_t)atlas->height, 1};
    DvzTexture* texture = dvz_ctx_texture(ctx, 2, shape, VK_FORMAT_R8G8B8A8_UNORM);
    // NOTE: the font texture must have LINEAR filter! otherwise no antialiasing
    dvz_texture_filter(texture, DVZ_FILTER_MAG, VK_FILTER_LINEAR);
    dvz_texture_filter(texture, DVZ_FILTER_MIN, VK_FILTER_LINEAR);

    dvz_texture_upload(
        texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, (uint32_t)(atlas->width * atlas->height * 4),
        atlas->font_texture);
    return texture;
}



static DvzFontAtlas dvz_font_atlas(DvzContext* ctx)
{
    // Font texture
    ASSERT(DATA_DIR);
    ASSERT(strlen(DATA_DIR) > 0);
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "font_inconsolata.png");
    log_trace("loading %s", path);

    int width, height, depth;

    DvzFontAtlas atlas = {0};
    atlas.font_texture = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    // TODO: parameters
    atlas.font_str = DVZ_FONT_ATLAS_STRING;
    ASSERT(strlen(atlas.font_str) > 0);
    atlas.cols = 16;
    atlas.rows = 6;

    atlas.width = (uint32_t)width;
    atlas.height = (uint32_t)height;
    atlas.glyph_width = atlas.width / (float)atlas.cols;
    atlas.glyph_height = atlas.height / (float)atlas.rows;

    atlas.texture = _font_texture(ctx, &atlas);

    return atlas;
}



static void dvz_font_atlas_destroy(DvzFontAtlas* atlas)
{
    ASSERT(atlas != NULL);
    ASSERT(atlas->font_texture != NULL);
    stbi_image_free(atlas->font_texture);
}



#endif
