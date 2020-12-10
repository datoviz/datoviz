layout (std140, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

struct VkViewport {
    float x, y, w, h, dmin, dmax;
};

layout (std140, binding = 1) uniform Viewport {
    VkViewport viewport; // Vulkan viewport
    vec4 margins;
    uvec4 screen; // (tlx, tly, w, h)
    uvec4 framebuffer; // (tlx, tly, w, h)
    float dpi_scaling; // DPI  scaling
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
