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

#include "color.glsl"


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
