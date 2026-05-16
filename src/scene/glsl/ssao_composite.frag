#version 450

layout(set = 0, binding = 0) uniform texture2D occlusionTex;
layout(set = 0, binding = 1) uniform sampler samp;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 extent = textureSize(sampler2D(occlusionTex, samp), 0);
    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)
        discard;

    float visibility = texelFetch(sampler2D(occlusionTex, samp), p, 0).r;
    outColor = vec4(0.0, 0.0, 0.0, clamp(1.0 - visibility, 0.0, 1.0));
}
