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
    vec2 uv = fragUVW.xy;
    if (volume.params.y > 0.5) {
        if (any(lessThan(uv, volume.clip_min.xy)) ||
            any(greaterThan(uv, volume.clip_max.xy))) {
            discard;
        }
    }

    int steps = int(clamp(volume.params.z, 1.0, 1024.0));
    float z0 = volume.clip_min.z;
    float z1 = volume.clip_max.z;
    float value = 0.0;
    for (int i = 0; i < 1024; i++) {
        if (i >= steps) {
            break;
        }
        float t = (float(i) + 0.5) / float(steps);
        float z = mix(z0, z1, t);
        value = max(value, texture(tex, vec3(uv, z)).r);
    }
    outColor = vec4(value, value, value, value * volume.params.x);
}
