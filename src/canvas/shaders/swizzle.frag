#version 450
layout(set = 0, binding = 0) uniform sampler2D src;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 pixel = texture(src, clamp(uv, 0.0, 1.0));
    out_color = pixel.bgra;
}
