layout (std140, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout (std140, binding = 1) uniform Viewport {
    vec4 viewport;          // in pixels: x, y, w, h, top left origin, margins excluded
    vec4 margins;           // top right bottom left (=CSS order), in pixels
    vec4 framebuffer_size;  // in pixels, w, h, top left origin, followed by aspect ratio in z component
} viewport;

// layout (binding = 2) uniform sampler2D color_tex;
