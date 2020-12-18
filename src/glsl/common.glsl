layout (std140, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
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
    float dpi_scaling;      // DPI scaling
} viewport;

layout (binding = 2) uniform sampler2D color_tex;

#define USER_BINDING 3

vec4 transform(vec3 pos) {
    vec4 tr = (mvp.proj * mvp.view * mvp.model) * vec4(pos, 1.0);
    // HACK: we transform from OpenGL conventional coordinate system to Vulkan
    // This allows us to use MVP matrices in OpenGL conventions.
    tr.y = -tr.y; // Vulkan swaps top and bottom in its device coordinate system.
    tr.z = .5 * (1.0 - tr.z); // depth is [-1, 1] in OpenGL but [0, 1] in Vulkan
    return tr;
}

vec4 transform(vec3 pos, vec2 shift) {
    vec4 pos_tr = transform(pos);
    return pos_tr + vec4(2 * shift / viewport.size, 0, 0);
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
