#version 450
#include "common.glsl"

layout (binding = 2) uniform sampler2D texture_sampler;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;


void main() {
    out_color = texture(texture_sampler, in_uv);
}
