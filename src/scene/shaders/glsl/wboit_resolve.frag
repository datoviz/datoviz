#version 450

layout(set = 0, binding = 0) uniform texture2D accumTex;
layout(set = 0, binding = 1) uniform texture2D weightTex;
layout(set = 0, binding = 2) uniform sampler samp;
layout(location = 0) in vec2 localUv;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 extent = textureSize(sampler2D(accumTex, samp), 0);
    ivec2 texel = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));
    vec4 accum = texelFetch(sampler2D(accumTex, samp), texel, 0);
    float weight = texelFetch(sampler2D(weightTex, samp), texel, 0).r;
    float alpha = clamp(1.0 - exp(-weight), 0.0, 1.0);
    vec3 rgb = accum.a > 1e-5 ? accum.rgb / max(accum.a, 1e-5) : vec3(0.0);
    outColor = vec4(rgb, alpha);
}
