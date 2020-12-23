#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

#include "array.h"
#include "canvas.h"
#include "vklite.h"

// #define STB_IMAGE_IMPLEMENTATION
BEGIN_INCL_NO_WARN
#include "../external/stb_image.h"
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common bindings
// NOTE: must correspond to the same constant in common.glsl
#define VKL_USER_BINDING 2



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics builtins
typedef enum
{
    VKL_GRAPHICS_NONE,
    VKL_GRAPHICS_POINTS,

    VKL_GRAPHICS_LINES,
    VKL_GRAPHICS_LINE_STRIP,
    VKL_GRAPHICS_TRIANGLES,
    VKL_GRAPHICS_TRIANGLE_STRIP,
    VKL_GRAPHICS_TRIANGLE_FAN,

    VKL_GRAPHICS_MARKER_RAW,
    VKL_GRAPHICS_MARKER,

    VKL_GRAPHICS_SEGMENT,
    VKL_GRAPHICS_ARROW,
    VKL_GRAPHICS_PATH,
    VKL_GRAPHICS_TEXT,

    VKL_GRAPHICS_MESH_RAW,
    VKL_GRAPHICS_MESH_TEXTURED,
    VKL_GRAPHICS_MESH_MULTI_TEXTURED,
    VKL_GRAPHICS_MESH_SHADED,

    VKL_GRAPHICS_FAKE_SPHERE,
    VKL_GRAPHICS_VOLUME,

    VKL_GRAPHICS_COUNT,
} VklGraphicsBuiltin;



// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKL_MARKER_DISC = 0,
    VKL_MARKER_ASTERISK = 1,
    VKL_MARKER_CHEVRON = 2,
    VKL_MARKER_CLOVER = 3,
    VKL_MARKER_CLUB = 4,
    VKL_MARKER_CROSS = 5,
    VKL_MARKER_DIAMOND = 6,
    VKL_MARKER_ARROW = 7,
    VKL_MARKER_ELLIPSE = 8,
    VKL_MARKER_HBAR = 9,
    VKL_MARKER_HEART = 10,
    VKL_MARKER_INFINITY = 11,
    VKL_MARKER_PIN = 12,
    VKL_MARKER_RING = 13,
    VKL_MARKER_SPADE = 14,
    VKL_MARKER_SQUARE = 15,
    VKL_MARKER_TAG = 16,
    VKL_MARKER_TRIANGLE = 17,
    VKL_MARKER_VBAR = 18,
} VkyMarkerType;



typedef enum
{
    VKL_CAP_TYPE_NONE = 0,
    VKL_CAP_ROUND = 1,
    VKL_CAP_TRIANGLE_IN = 2,
    VKL_CAP_TRIANGLE_OUT = 3,
    VKL_CAP_SQUARE = 4,
    VKL_CAP_BUTT = 5,
    VKL_CAP_COUNT,
} VklCapType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;
typedef struct VklMVP VklMVP;

typedef struct VklGraphicsPointParams VklGraphicsPointParams;
typedef struct VklGraphicsMarkerVertex VklGraphicsMarkerVertex;
typedef struct VklGraphicsMarkerParams VklGraphicsMarkerParams;
typedef struct VklGraphicsSegmentVertex VklGraphicsSegmentVertex;

typedef struct VklGraphicsTextParams VklGraphicsTextParams;
typedef struct VklGraphicsTextVertex VklGraphicsTextVertex;
typedef struct VklGraphicsTextItem VklGraphicsTextItem;

typedef struct VklGraphicsData VklGraphicsData;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;
    cvec4 color;
};



struct VklMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};



struct VklGraphicsData
{
    VklGraphics* graphics;
    VklArray* vertices;
    VklArray* indices;
    uint32_t item_count;
    uint32_t current_idx;
    void* user_data;
};



/*************************************************************************************************/
/*  Graphics points                                                                              */
/*************************************************************************************************/

struct VklGraphicsPointParams
{
    float point_size;
};



/*************************************************************************************************/
/*  Graphics marker                                                                              */
/*************************************************************************************************/

struct VklGraphicsMarkerVertex
{
    vec3 pos;
    cvec4 color;
    float size;
    // in fact a VklMarkerType but we should control the exact data type for the GPU
    uint8_t marker;
    uint8_t angle;
    uint8_t transform;
};

struct VklGraphicsMarkerParams
{
    vec4 edge_color;
    float edge_width;
    int32_t enable_depth;
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct VklGraphicsSegmentVertex
{
    vec3 P0;
    vec3 P1;
    vec4 shift;
    cvec4 color;
    float linewidth;
    VklCapType cap0;
    VklCapType cap1;
    uint8_t transform;
};



/*************************************************************************************************/
/*  Graphics text                                                                                */
/*************************************************************************************************/

struct VklGraphicsTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};

struct VklGraphicsTextVertex
{
    vec3 pos;
    vec2 shift;
    cvec4 color;
    vec2 glyph_size;
    vec2 anchor;
    float angle;
    usvec4 glyph; // char, char_index, str_len, str_index
    uint8_t transform;
};

struct VklGraphicsTextItem
{
    VklGraphicsTextVertex vertex;
    float font_size;
    const char* string;
};



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

static const char VKL_FONT_ATLAS_STRING[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";

typedef struct VklFontAtlas VklFontAtlas;
struct VklFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
    VklTexture* texture;
};

static size_t _font_atlas_glyph(VklFontAtlas* atlas, const char* str, uint32_t idx)
{
    ASSERT(atlas != NULL);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    ASSERT(idx < strlen(str));
    ASSERT(atlas->font_str != NULL);
    ASSERT(strlen(atlas->font_str) > 0);

    char c[2] = {str[idx], 0};
    return strcspn(atlas->font_str, c);
}

static void _font_atlas_glyph_size(VklFontAtlas* atlas, float size, vec2 glyph_size)
{
    ASSERT(atlas != NULL);
    glyph_size[0] = size * atlas->glyph_width / atlas->glyph_height;
    glyph_size[1] = size;
}

static VklFontAtlas vkl_font_atlas(const char* img_path)
{
    ASSERT(img_path != NULL);

    int width, height, depth;

    VklFontAtlas atlas = {0};
    atlas.font_texture = stbi_load(img_path, &width, &height, &depth, STBI_rgb_alpha);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    // TODO: parameters
    atlas.font_str = VKL_FONT_ATLAS_STRING;
    ASSERT(strlen(atlas.font_str) > 0);
    atlas.cols = 16;
    atlas.rows = 6;

    atlas.width = (uint32_t)width;
    atlas.height = (uint32_t)height;
    atlas.glyph_width = atlas.width / (float)atlas.cols;
    atlas.glyph_height = atlas.height / (float)atlas.rows;

    return atlas;
}

static void vkl_font_atlas_destroy(VklFontAtlas* atlas)
{
    ASSERT(atlas != NULL);
    stbi_image_free(atlas->font_texture);
}

static VklFontAtlas _font_texture(VklContext* ctx)
{
    ASSERT(ctx != NULL);

    // Font texture
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "font_inconsolata.png");
    VklFontAtlas atlas = vkl_font_atlas(path);

    VklTexture* texture = vkl_ctx_texture(
        ctx, 2, (uvec3){(uint32_t)atlas.width, (uint32_t)atlas.height, 1},
        VK_FORMAT_R8G8B8A8_UNORM);
    // NOTE: the font texture must have LINEAR filter! otherwise no antialiasing
    vkl_texture_filter(texture, VKL_FILTER_MAX, VK_FILTER_LINEAR);
    vkl_texture_filter(texture, VKL_FILTER_MIN, VK_FILTER_LINEAR);
    vkl_upload_texture(
        ctx, texture, (uint32_t)(atlas.width * atlas.height * 4), atlas.font_texture);
    atlas.texture = texture;

    return atlas;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vkl_graphics_callback(VklGraphics* graphics, VklGraphicsCallback callback);

// Used in visual bake
VKY_EXPORT VklGraphicsData
vkl_graphics_data(VklGraphics* graphics, VklArray* vertices, VklArray* indices, void* user_data);

VKY_EXPORT void vkl_graphics_alloc(VklGraphicsData* data, uint32_t item_count);

VKY_EXPORT void vkl_graphics_append(VklGraphicsData* data, const void* item);



VKY_EXPORT VklGraphics*
vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type, int flags);

VKY_EXPORT VklViewport vkl_viewport_full(VklCanvas* canvas);



#endif
