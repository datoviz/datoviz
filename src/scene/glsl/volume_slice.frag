#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(set = 1, binding = 0) uniform sampler3D tex;
layout(set = 1, binding = 3) uniform sampler2D depthTex;

layout(set = 1, binding = 2) uniform VolumeParams {
    vec4 clip_min;
    vec4 clip_max;
    vec4 params;
} volume;

layout(location = 0) in vec3 fragUVW;
layout(location = 0) out vec4 outColor;

float safe_inv(float v)
{
    if (abs(v) < 1e-6) {
        return v < 0.0 ? -1e6 : 1e6;
    }
    return 1.0 / v;
}

bool ray_box(vec3 ro, vec3 rd, vec3 box_min, vec3 box_max, out float t0, out float t1)
{
    vec3 inv_rd = vec3(safe_inv(rd.x), safe_inv(rd.y), safe_inv(rd.z));
    vec3 t_near = (box_min - ro) * inv_rd;
    vec3 t_far = (box_max - ro) * inv_rd;
    vec3 t_min = min(t_near, t_far);
    vec3 t_max = max(t_near, t_far);
    t0 = max(max(t_min.x, t_min.y), t_min.z);
    t1 = min(min(t_max.x, t_max.y), t_max.z);
    return t1 >= max(t0, 0.0);
}

vec3 camera_uvw()
{
    mat4 inv_model = inverse(mvp.model);
    mat4 inv_view = inverse(mvp.view);
    vec3 camera_obj = (inv_model * inv_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    return 0.5 * camera_obj + vec3(0.5);
}

float projected_depth(vec3 uvw)
{
    vec3 pos = uvw * 2.0 - vec3(1.0);
    vec4 clip = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    if (clip.w <= 0.0) {
        return 1.0;
    }
    return clamp(0.5 * (clip.z / clip.w) + 0.5, 0.0, 1.0);
}

bool occluded_by_scene_depth(vec3 uvw)
{
    vec2 size = vec2(textureSize(depthTex, 0));
    vec2 uv = clamp(gl_FragCoord.xy / size, vec2(0.0), vec2(1.0));
    float scene_depth = texture(depthTex, uv).r;
    if (scene_depth >= 0.999999) {
        return false;
    }
    return projected_depth(uvw) > scene_depth + 0.0005;
}

void main()
{
    vec3 ro = camera_uvw();
    vec3 rd = normalize(fragUVW - ro);

    float proxy_t0 = 0.0;
    float proxy_t1 = 0.0;
    if (!ray_box(ro, rd, vec3(0.0), vec3(1.0), proxy_t0, proxy_t1)) {
        discard;
    }

    vec3 box_min = volume.clip_min.xyz;
    vec3 box_max = volume.clip_max.xyz;
    float slice_z = 0.5 * (box_min.z + box_max.z);
    if (abs(rd.z) < 1e-6) {
        discard;
    }

    float slice_t = (slice_z - ro.z) / rd.z;
    if (slice_t < max(proxy_t0, 0.0) || slice_t > proxy_t1) {
        discard;
    }

    vec3 uvw = ro + rd * slice_t;
    if (any(lessThan(uvw, box_min)) || any(greaterThan(uvw, box_max))) {
        discard;
    }
    if (occluded_by_scene_depth(uvw)) {
        discard;
    }

    vec4 sample_value = texture(tex, uvw);
    if (volume.clip_min.w > 0.5) {
        outColor = vec4(sample_value.rgb, sample_value.a * volume.params.x);
    } else {
        float value = sample_value.r;
        outColor = vec4(value, value, value, value * volume.params.x);
    }
}
