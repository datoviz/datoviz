/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

#define VKL_VIEWPORT_NONE           0
#define VKL_VIEWPORT_INNER          1
#define VKL_VIEWPORT_OUTER          2
#define VKL_VIEWPORT_OUTER_BOTTOM   3
#define VKL_VIEWPORT_OUTER_LEFT     4

#define VKL_TRANSFORM_AXIS_DEFAULT  0
#define VKL_TRANSFORM_AXIS_ALL      1
#define VKL_TRANSFORM_AXIS_X        2
#define VKL_TRANSFORM_AXIS_Y        3
#define VKL_TRANSFORM_AXIS_NONE     4

#define USER_BINDING 2

// NOTE:needs to be a macro and not a function so that it can be safely included in both
// vertex and fragment shaders (discard is forbidden in the vertex shader)
#define CLIP \
    switch (viewport.clip)                                                                        \
    {                                                                                             \
        case VKL_VIEWPORT_NONE:                                                                   \
            break;                                                                                \
                                                                                                  \
        case VKL_VIEWPORT_INNER:                                                                  \
        case VKL_VIEWPORT_OUTER:                                                                  \
            if(clip_viewport(gl_FragCoord.xy))                                                    \
                discard;                                                                          \
            break;                                                                                \
                                                                                                  \
        case VKL_VIEWPORT_OUTER_BOTTOM:                                                           \
            if(clip_viewport(gl_FragCoord.xy, 0))                                                 \
                discard;                                                                          \
            break;                                                                                \
                                                                                                  \
        case VKL_VIEWPORT_OUTER_LEFT:                                                             \
            if(clip_viewport(gl_FragCoord.xy, 1))                                                 \
                discard;                                                                          \
            break;                                                                                \
                                                                                                  \
        default:                                                                                  \
            break;                                                                                \
    }



/*************************************************************************************************/
/*  Common bindings                                                                              */
/*************************************************************************************************/

layout (std140, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
} mvp;

struct VkViewport {
    float x, y, w, h, dmin, dmax;
};

layout (std140, binding = 1) uniform Viewport {
    VkViewport viewport;    // Vulkan viewport
    vec4 margins;           // margins

    uvec2 offset_screen;    // offset
    uvec2 size_screen;      // size

    uvec2 offset;           // framebuffer coordinates
    uvec2 size;             // framebuffer coordinates

    // Options
    int clip;               // viewport clipping
    int transform;
    float dpi_scaling;      // DPI scaling
} viewport;



/*************************************************************************************************/
/*  Viewport and transform functions                                                             */
/*************************************************************************************************/

vec4 to_vulkan(vec4 tr) {
    // HACK: we transform from OpenGL conventional coordinate system to Vulkan
    // This allows us to use MVP matrices in OpenGL conventions.
    tr.y = -tr.y; // Vulkan swaps top and bottom in its device coordinate system.
    tr.z = .5 * (1.0 - tr.z); // depth is [-1, 1] in OpenGL but [0, 1] in Vulkan
    return tr;
}

vec4 transform(vec3 pos, vec2 shift, uint transform_mode) {
    mat4 mvp = mvp.proj * mvp.view * mvp.model;
    vec4 tr = vec4(pos, 1.0);

    // By default, take the viewport transform.
    if (transform_mode == 0)
        transform_mode = uint(viewport.transform);
    // Default: transform all
    if (transform_mode == 0)
        transform_mode = VKL_TRANSFORM_AXIS_ALL;

    // Transform.
    switch (transform_mode) {
        case VKL_TRANSFORM_AXIS_NONE:
            break;
        case VKL_TRANSFORM_AXIS_ALL:
            tr = mvp * tr;
            break;
        case VKL_TRANSFORM_AXIS_X:
            tr = mvp * tr;
            tr.y = pos.y;
            break;
        case VKL_TRANSFORM_AXIS_Y:
            tr = mvp * tr;
            tr.x = pos.x;
            break;
        default:
            break;
    }

    // Margins
    float w = viewport.size.x;
    float h = viewport.size.y;
    float mt = viewport.margins.x;
    float mr = viewport.margins.y;
    float mb = viewport.margins.z;
    float ml = viewport.margins.w;
    float a = 1;
    float b = 0;

    // horizontal margins
    if (w > 0) {
        a = 1 - (ml + mr) / w;
        b = (ml - mr) / w;
        tr.x = a * tr.x + b;
    }

    // vertical margins
    if (h > 0) {
        a = 1 - (mb + mt) / h;
        b = (mb - mt) / h;
        tr.y = a * tr.y + b;
    }

    // pixel shift.
    if (w > 0 && h > 0)
        tr.xy += (2 * shift / viewport.size);

    tr = to_vulkan(tr);
    return tr;
}



vec4 transform(vec3 pos, vec2 shift) {
    return transform(pos, shift, 0);
}



vec4 transform(vec3 pos, uint transform_mode) {
    return transform(pos, vec2(0, 0), transform_mode);
}



vec4 transform(vec3 pos) {
    return transform(pos, vec2(0, 0), 0);
}



bool clip_viewport(vec2 frag_coords) {
    vec2 uv = frag_coords - viewport.offset;
    return (uv.y < 0 + viewport.margins.x ||
        uv.x > viewport.size.x - viewport.margins.y ||
        uv.y > viewport.size.y - viewport.margins.z ||
        uv.x < 0 + viewport.margins.w
    );
}



bool clip_viewport(vec2 frag_coords, int coord) {
    vec2 uv = frag_coords - viewport.offset;

    float w = viewport.size.x;
    float h = viewport.size.y;

    float top = viewport.margins.x;
    float right = viewport.margins.y;
    float bottom = viewport.margins.z;
    float left = viewport.margins.w;

    // Discard top and right edges.
    // NOTE: extra margin: 10, 30px
    // if (uv.y < top - 10.0 || uv.x > w - right + 30.0)
    //     discard;

    // Bottom-left corner: discard along the diagonal
    if (uv.x < left && uv.y > h - bottom) {
        vec2 q0 = vec2(left, bottom);
        vec2 q1 = vec2(uv.x, h - uv.y);
        float s = (q0.x * q1.y - q0.y * q1.x);
        float eps = left * 5;
        if ((s > -eps && coord < .5) || (s < eps && coord > .5))
            return true;
    }
    return false;
}



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
