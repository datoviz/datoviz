// Common MVP uniform.

// NOTE: this structure is passed as a dynamic uniform buffer in Vulkan
// it should be less than 256 bytes in order to be supported on all hardware
layout (std140, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 viewport;          // in pixels: x, y, w, h, top left origin, margins excluded
    vec4 margins;           // top right bottom left (=CSS order), in pixels
    vec4 framebuffer_size;  // in pixels, w, h, top left origin, followed by aspect ratio in z component
    uint cmap_context;      // 4 bytes packed into uint32: texrow, mod, mod_const
} mvp;                      // TODO: rename to common?

layout (binding = 1) uniform sampler2D color_texture;


#define MARGINS mvp.margins
#define VIEWPORT_MARGINS vec4(-MARGINS[3], -MARGINS[0], MARGINS[1] + MARGINS[3], MARGINS[0] + MARGINS[2])
#define VIEWPORT mvp.viewport

#define _CLIP(margin, viewport) \
    vec2 uv = gl_FragCoord.xy - (viewport).xy; \
    if (uv.y < 0 + (margin).x || \
        uv.x > (viewport).z - (margin).y || \
        uv.y > (viewport).w - (margin).z || \
        uv.x < 0 + (margin).w \
    ) \
        discard;

#define CLIP_VIEWPORT _CLIP((MARGINS), (VIEWPORT))

#define TRANSFORM_MODE_NORMAL   0x00
#define TRANSFORM_MODE_X_ONLY   0x01
#define TRANSFORM_MODE_Y_ONLY   0x02
#define TRANSFORM_MODE_STATIC   0x07


mat4 get_ortho_matrix(vec2 size) {
    // The orthographic projection is:
    //    2/w            -1
    //          2/h      -1
    //               1    0
    //                    1
    mat4 ortho = mat4(1.0);

    // WARNING: column-major order (=FORTRAN order, columns first)
    ortho[0][0] = 2. / size.x;
    ortho[1][1] = 2. / size.y;
    ortho[2][2] = 1.;

    ortho[3] = vec4(-1, -1, 0, 1);

    return ortho;
}


vec4 transform_pos(vec3 pos) {
    mat4 proj = mvp.proj;
    return (proj * mvp.view * mvp.model) * vec4(pos, 1.0);
}



vec4 transform_pos(vec3 pos, uint transform_mode) {
    if (transform_mode == TRANSFORM_MODE_NORMAL)
        return transform_pos(pos);
    else if (transform_mode == TRANSFORM_MODE_X_ONLY)
        return vec4(transform_pos(pos).x, pos.yz, 1.0);
    else if (transform_mode == TRANSFORM_MODE_Y_ONLY)
        return vec4(pos.x, transform_pos(pos).y, pos.z, 1.0);
    // else if (transform_mode == TRANSFORM_MODE_STATIC)
    return vec4(pos, 1.0);
}



vec4 transform_pos(vec3 pos, vec2 shift) {
    vec4 pos_tr = transform_pos(pos);
    return pos_tr + vec4(2 * shift / mvp.viewport.zw, 0, 0);
}



vec4 transform_pos(vec3 pos, vec2 shift, uint transform_mode) {
    if (transform_mode == TRANSFORM_MODE_NORMAL)
        return transform_pos(pos, shift);
    else if (transform_mode == TRANSFORM_MODE_X_ONLY)
        return vec4(transform_pos(pos, shift).x, pos.y + 2 * shift.y / mvp.viewport.w, pos.z, 1.0);
    else if (transform_mode == TRANSFORM_MODE_Y_ONLY)
        return vec4(pos.x + 2 * shift.x / mvp.viewport.z, transform_pos(pos, shift).y, pos.z, 1.0);
    // else if (transform_mode == TRANSFORM_MODE_STATIC)
    return vec4(pos, 1.0) + vec4(2 * shift / mvp.viewport.zw, 0, 0);
}



uvec4 unpack_32_8(uint n) {
    /* convert one uint32 into four uint8 */
    return uvec4(
        n & 0xFF,
        (n >> 8) & 0xFF,
        (n >> 16) & 0xFF,
        (n >> 24) & 0xFF
    );
}



uvec2 unpack_32_16(uint n) {
    /* convert one uint32 into two uint16 */
    return uvec2(
        256 * (n         & 0xFF)  + ((n >> 8 ) & 0xFF),
        256 * ((n >> 16) & 0xFF)  + ((n >> 24) & 0xFF)
    );
}



uvec2 unpack_16_8(uint n) {
    /* convert one uint16 into two uint8 */
    return uvec2(
        n & 0xFF,
        (n >> 8) & 0xFF
    );
}


#define VKY_COLOR_MOD_NONE            0
#define VKY_COLOR_MOD_ALPHA           1
#define VKY_COLOR_MOD_HUE             2
#define VKY_COLOR_MOD_SATURATION      3
#define VKY_COLOR_MOD_VALUE           4
#define VKY_COLOR_MOD_LIGHTNESS       5
#define VKY_COLOR_MOD_ALPHA_H         6
#define VKY_COLOR_MOD_ALPHA_S         7
#define VKY_COLOR_MOD_ALPHA_V         8
#define VKY_COLOR_MOD_ALPHA_L         9
#define VKY_COLOR_MOD_HUE_A           10
#define VKY_COLOR_MOD_HUE_S           11
#define VKY_COLOR_MOD_HUE_V           12
#define VKY_COLOR_MOD_HUE_L           13
#define VKY_COLOR_MOD_SATURATION_A    14
#define VKY_COLOR_MOD_SATURATION_H    15
#define VKY_COLOR_MOD_SATURATION_V    16
#define VKY_COLOR_MOD_SATURATION_L    17
#define VKY_COLOR_MOD_VALUE_A         18
#define VKY_COLOR_MOD_VALUE_H         19
#define VKY_COLOR_MOD_VALUE_S         20
#define VKY_COLOR_MOD_VALUE_L         21
#define VKY_COLOR_MOD_LIGHTNESS_A     22
#define VKY_COLOR_MOD_LIGHTNESS_H     23
#define VKY_COLOR_MOD_LIGHTNESS_S     24
#define VKY_COLOR_MOD_LIGHTNESS_V     25


vec4 get_color(uvec2 cmap_bytes) {
    vec4 color = vec4(0, 0, 0, 1);

    // Color context
    uvec4 cmap_ctx = unpack_32_8(mvp.cmap_context);
    float cmap_texrow = float(cmap_ctx.x) / 255.0;  // colormap
    uint cmap_mod = cmap_ctx.y;  // modifier
    float cmap_const = float(cmap_ctx.z) / 255.0;   // constant
    float cunused = float(cmap_ctx.w) / 255.0;      // not yet used

    float cmap_texcol = float(cmap_bytes.x) / 255.0;
    float cmap_mod_var = float(cmap_bytes.y) / 255.0;

    // Fetch the color using the colormap index and the value. The third value in icol is currently unused.
    color = texture(color_texture, vec2(cmap_texcol, cmap_texrow));
    color.a = 1;
    // TODO: try to replace by imageLoad, might have to use SSBO instead of texture for the colormap image
    // color = imageLoad(color_texture, ivec2(cmap_bytes.x, cmap));

    switch (cmap_mod) {
    case VKY_COLOR_MOD_NONE:  // color modifier
        break;
    case VKY_COLOR_MOD_ALPHA:
        color.a = cmap_mod_var;
        break;
    // TODO: implement the other modifiers
    default:
        break;
    }
    return color;
}

vec4 get_color(vec4 color) {
    // no-op when the color is already in RGBA format (4 bytes of uint8 transformed to float by Vulkan via FORMAT_UNORM)
    return color;
}

/*

How to switch from cmap to color system:
1) use VKY_DEFAULT_VERTEX_FORMAT_COLOR_RGBA in the vertex layout.
2) use VkyColorBytes instead of VkyCmapBytes in the vertex struct color component
3) use vec4 in color GLSL attribute
4) use vky_color() instead of vky_cmap() when setting the color

*/
