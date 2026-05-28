#version 450

layout(set = 0, binding = 0) uniform texture2D tex;
layout(set = 0, binding = 1) uniform sampler samp;
layout(location = 0) out vec4 color;

void main()
{
    color = texture(sampler2D(tex, samp), vec2(0.5));
}
