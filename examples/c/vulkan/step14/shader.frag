#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; } material;
layout(set = 0, binding = 1) uniform sampler2D tex;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main()
{
    // Read the texture at this fragment's UV with the sampler's rules, then apply the tint.
    out_color = texture(tex, uv) * material.tint;
}
