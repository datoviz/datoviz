#version 450

layout(set = 0, binding = 0) uniform sampler2D depthTex;

layout(location = 0) out vec4 outColor;

// 8 sample offsets in a small disk around the pixel
const vec2 OFFSETS[8] = vec2[](
    vec2( 1.0,  0.0), vec2(-1.0,  0.0),
    vec2( 0.0,  1.0), vec2( 0.0, -1.0),
    vec2( 1.0,  1.0), vec2(-1.0,  1.0),
    vec2( 1.0, -1.0), vec2(-1.0, -1.0)
);

void main()
{
    vec2 resolution = vec2(800.0, 600.0);
    vec2 uv = gl_FragCoord.xy / resolution;

    float depth0 = texture(depthTex, uv).r;

    float occlusion = 0.0;
    float radius = 4.0;     // in pixels
    float bias   = 0.02;    // depth bias

    for (int i = 0; i < 8; ++i)
    {
        vec2 ofs = OFFSETS[i] * radius / resolution;
        vec2 uvo = uv + ofs;

        float d = texture(depthTex, uvo).r;

        if (d < depth0 - bias)
            occlusion += 1.0;
    }

    occlusion /= 8.0;
    float ao = 1.0 - occlusion;

    outColor = vec4(vec3(ao), 1.0);
}
