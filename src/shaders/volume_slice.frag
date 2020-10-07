#version 450
#include "common.glsl"

layout (binding = 2) uniform sampler3D tex_sampler;

layout (location = 0) in vec3 tex_coords;

layout (location = 0) out vec4 out_color;

void main()
{
    float value = texture(tex_sampler, tex_coords).r * 80; // TODO: gain control
    out_color = vec4(value, value, value, 1.0);
    out_color.a = 1;
}
