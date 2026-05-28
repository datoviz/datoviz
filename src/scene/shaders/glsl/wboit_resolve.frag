#version 450

layout(set = 0, binding = 0) uniform texture2D accumTex;
layout(set = 0, binding = 1) uniform texture2D weightTex;
layout(set = 0, binding = 2) uniform sampler samp;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(sampler2D(accumTex, samp), 0));
    vec4 accum = texture(sampler2D(accumTex, samp), uv);
    float weight = texture(sampler2D(weightTex, samp), uv).r;
    float alpha = clamp(1.0 - exp(-weight), 0.0, 1.0);
    vec3 rgb = accum.a > 1e-5 ? accum.rgb / max(accum.a, 1e-5) : vec3(0.0);
    outColor = vec4(rgb, alpha);
}
