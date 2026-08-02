#version 450

layout(set = 0, binding = 0) uniform texture2D occlusionTex;
layout(set = 0, binding = 1) uniform sampler samp;
layout(set = 0, binding = 2) uniform SsaoParams {
    mat4 invProj;
    mat4 view;
    vec4 viewport;
    vec4 params;
    vec4 params2;
    vec4 params3;
} ssao;
layout(location = 0) in vec2 localUv;
layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 extent = textureSize(sampler2D(occlusionTex, samp), 0);
    ivec2 p = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));

    float rawVisibility = texelFetch(sampler2D(occlusionTex, samp), p, 0).r;
    float strength = max(ssao.params.y, 0.0);
    float power = max(ssao.params2.x, 0.001);
    float minVisibility = clamp(ssao.params2.y, 0.0, 1.0);
    float debugView = ssao.params2.w;
    float visibility = mix(1.0, rawVisibility, strength);
    visibility = pow(clamp(visibility, minVisibility, 1.0), power);
    float occlusion = clamp(1.0 - visibility, 0.0, 1.0);

    if (debugView > 0.5)
        outColor = vec4(vec3(rawVisibility), 1.0);
    else
        outColor = vec4(0.0, 0.0, 0.0, occlusion);
}
