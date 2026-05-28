#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

vec4 image_pixel_anchor(vec3 position)
{
    vec2 size = max(viewport.rect.zw, vec2(1.0));
    vec2 ndc =
        vec2(-1.0 + 2.0 * position.x / size.x, -1.0 + 2.0 * position.y / size.y);
    return vec4(ndc, position.z, 1.0);
}

void main()
{
    gl_Position = image_pixel_anchor(inPos);
    fragUV = inUV;
}
