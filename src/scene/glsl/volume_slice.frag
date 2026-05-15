#version 450

layout(set = 1, binding = 0) uniform sampler3D tex;

layout(set = 1, binding = 2) uniform VolumeParams {
    vec4 clip_min;
    vec4 clip_max;
    vec4 params;
} volume;

layout(location = 0) in vec3 fragUVW;
layout(location = 0) out vec4 outColor;

void main()
{
    if (volume.params.y > 0.5) {
        if (any(lessThan(fragUVW, volume.clip_min.xyz)) ||
            any(greaterThan(fragUVW, volume.clip_max.xyz))) {
            discard;
        }
    }

    float value = texture(tex, fragUVW).r;
    outColor = vec4(value, value, value, value * volume.params.x);
}
