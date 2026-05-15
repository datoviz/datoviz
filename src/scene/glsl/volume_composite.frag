#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(set = 1, binding = 0) uniform sampler3D tex;

layout(set = 1, binding = 2) uniform VolumeParams {
    vec4 clip_min;
    vec4 clip_max;
    vec4 params;
} volume;

layout(location = 0) in vec3 fragUVW;
layout(location = 0) out vec4 outColor;

const float ENTRY_FACE_EPSILON = 0.004;
const float EXTINCTION_SCALE = 4.0;

float safe_inv(float v)
{
    if (abs(v) < 1e-6) {
        return v < 0.0 ? -1e6 : 1e6;
    }
    return 1.0 / v;
}

bool inside_box(vec3 p, vec3 box_min, vec3 box_max)
{
    return all(greaterThanEqual(p, box_min)) && all(lessThanEqual(p, box_max));
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

void main()
{
    vec3 ro = camera_uvw();
    vec3 rd = normalize(fragUVW - ro);

    float proxy_t0 = 0.0;
    float proxy_t1 = 0.0;
    if (!ray_box(ro, rd, vec3(0.0), vec3(1.0), proxy_t0, proxy_t1)) {
        discard;
    }

    if (!inside_box(ro, vec3(0.0), vec3(1.0))) {
        vec3 proxy_entry = ro + rd * max(proxy_t0, 0.0);
        if (distance(proxy_entry, fragUVW) > ENTRY_FACE_EPSILON) {
            discard;
        }
    }

    vec3 box_min = volume.clip_min.xyz;
    vec3 box_max = volume.clip_max.xyz;
    float t0 = 0.0;
    float t1 = 0.0;
    if (!ray_box(ro, rd, box_min, box_max, t0, t1)) {
        discard;
    }

    int steps = int(clamp(volume.params.z, 1.0, 1024.0));
    float start_t = max(t0, 0.0);
    float end_t = t1;
    if (end_t <= start_t) {
        discard;
    }

    float ray_length = end_t - start_t;
    float step_len = ray_length / float(steps);
    bool transfer = volume.clip_min.w > 0.5;
    vec4 accum = vec4(0.0);
    for (int i = 0; i < 1024; i++) {
        if (i >= steps || accum.a > 0.985) {
            break;
        }
        float t = (float(i) + 0.5) / float(steps);
        vec3 uvw = ro + rd * mix(start_t, end_t, t);
        vec4 sample_value = texture(tex, uvw);
        float density = clamp(transfer ? sample_value.a : sample_value.r, 0.0, 1.0);
        vec3 color = transfer ? sample_value.rgb : vec3(sample_value.r);
        float sample_alpha =
            1.0 - exp(-density * volume.params.x * EXTINCTION_SCALE * step_len);
        sample_alpha = clamp(sample_alpha, 0.0, 1.0);
        accum.rgb += (1.0 - accum.a) * color * sample_alpha;
        accum.a += (1.0 - accum.a) * sample_alpha;
    }
    outColor = accum;
}
