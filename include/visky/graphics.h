#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

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
#define VKL_USER_BINDING 3



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
    // uint8_t transform_mode;
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
    // uint8_t transform_mode;
};



/*************************************************************************************************/
/*  Segment utils                                                                                */
/*************************************************************************************************/

static void _graphics_segment_add(
    VklGraphicsSegmentVertex* data, VklIndex* indices, uint32_t i, //
    vec3 P0, vec3 P1, cvec4 color, float linewidth, vec4 shift, VklCapType cap0, VklCapType cap1)
{
    ASSERT(data != NULL);
    ASSERT(indices != NULL);

    for (uint32_t j = 0; j < 4; j++)
    {
        glm_vec3_copy(P0, data[4 * i + j].P0);
        glm_vec3_copy(P1, data[4 * i + j].P1);
        memcpy(data[4 * i + j].color, color, sizeof(cvec4));
        glm_vec4_copy(shift, data[4 * i + j].shift);

        data[4 * i + j].linewidth = linewidth;
        data[4 * i + j].cap0 = cap0;
        data[4 * i + j].cap1 = cap1;
    }

    indices[6 * i + 0] = 4 * i + 0;
    indices[6 * i + 1] = 4 * i + 1;
    indices[6 * i + 2] = 4 * i + 2;
    indices[6 * i + 3] = 4 * i + 0;
    indices[6 * i + 4] = 4 * i + 2;
    indices[6 * i + 5] = 4 * i + 3;
}



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

#define VKL_FONT_ATLAS_STRING                                                                     \
    " !\"#$%&'()*+,-./"                                                                           \
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f"

typedef struct VklFontAtlas VklFontAtlas;
struct VklFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
};

static size_t _font_atlas_glyph(VklFontAtlas* atlas, const char* str, uint32_t idx)
{
    ASSERT(atlas != NULL);
    ASSERT(str != NULL);
    ASSERT(idx < strlen(str));
    ASSERT(atlas->font_str != NULL);

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

static void _graphics_text_string(
    VklFontAtlas* atlas, uint32_t str_idx, const char* str,                                    //
    vec3 pos, vec2 shift, vec2 anchor, float angle, float font_size, const cvec4* char_colors, //
    VklGraphicsTextVertex* vertices)                                                           //
{
    // Append a string to a vertex array.

    // vertices must point to an array of at least 4*strlen Vertex structs.
    ASSERT(vertices != NULL);
    ASSERT(str != NULL);
    uint32_t n = strlen(str);
    ASSERT(n > 0);
    for (uint32_t i = 0; i < n; i++)
    {
        size_t g = _font_atlas_glyph(atlas, str, i);
        for (uint32_t j = 0; j < 4; j++)
        {
            glm_vec3_copy(pos, vertices[4 * i + j].pos);
            glm_vec3_copy(shift, vertices[4 * i + j].shift);
            glm_vec3_copy(anchor, vertices[4 * i + j].anchor);
            vertices[4 * i + j].angle = angle;

            // Color.
            if (char_colors != NULL)
                memcpy(vertices[4 * i + j].color, char_colors[i], sizeof(cvec4));

            // Glyph size.
            _font_atlas_glyph_size(atlas, font_size, vertices[4 * i + j].glyph_size);

            // Glyph.
            vertices[4 * i + j].glyph[0] = g;       // char
            vertices[4 * i + j].glyph[1] = i;       // char idx
            vertices[4 * i + j].glyph[2] = n;       // str len
            vertices[4 * i + j].glyph[3] = str_idx; // str idx
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklGraphics*
vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type, int flags);

VKY_EXPORT VklViewport vkl_viewport_full(VklCanvas* canvas);



#endif
