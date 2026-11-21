#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; } material;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main()
{
    // Diagnostic colors: u as red, v as green, so each face shows its UV gradient.
    out_color = vec4(uv, 0.0, 1.0) * material.tint;
}
