#version 450
#include "common.glsl"

// layout (std140, binding = USER_BINDING) uniform Params {
// } params;

layout(binding = (USER_BINDING)) uniform sampler3D tex;

layout (location = 0) in vec3 in_uvw;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = texture(tex, in_uvw);
}
