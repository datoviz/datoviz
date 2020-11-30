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

// layout (binding = 2) uniform sampler2D color_tex;

#define USER_BINDING 2
