#version 450
#include "common.glsl"
#include "colormaps.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    int cmap;
} params;

layout(binding = (USER_BINDING + 1)) uniform sampler3D tex;

layout (location = 0) in vec3 in_uvw;

layout (location = 0) out vec4 out_color;

void main() {
    float value = texture(tex, in_uvw).r;
    out_color = colormap(params.cmap, value);



}
