#version 450
#include "../src/glsl/common.glsl"
#include "../src/glsl/colormaps.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in float size; // between 0.0 and 255.0

layout (location = 0) out vec4 out_color;

void main() {
    gl_Position = transform(pos);
    gl_PointSize = size;
    out_color = colormap(DVZ_CMAP_HSV, size / 255.0);
    out_color.a = .25;
}
