#version 450

layout(set = 0, binding = 0) uniform texture2D occlusionTex;
layout(set = 0, binding = 1) uniform sampler samp;
layout(location = 0) in vec2 localUv;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 extent = textureSize(sampler2D(occlusionTex, samp), 0);
    ivec2 p = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));

    float visibility = clamp(texelFetch(sampler2D(occlusionTex, samp), p, 0).r, 0.0, 1.0);
    outColor = vec4(vec3(visibility), 1.0);
}
