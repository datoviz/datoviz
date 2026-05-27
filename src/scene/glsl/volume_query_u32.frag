#version 450

layout(set = 0, binding = 0) uniform MVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

#if defined(DVZ_VOLUME_LABEL_UINT_QUERY)
layout(set = 1, binding = 0) uniform utexture3D tex;
#elif defined(DVZ_VOLUME_LABEL_SINT_QUERY)
layout(set = 1, binding = 0) uniform itexture3D tex;
#else
layout(set = 1, binding = 0) uniform texture3D tex;
#endif
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 3) uniform texture2D depthTex;
layout(set = 1, binding = 4) uniform texture2D transferTex;

layout(set = 1, binding = 2) uniform VolumeParams
{
    vec4 clip_min;
    vec4 clip_max;
    vec4 clip_plane;
    vec4 clip_plane_params;
    vec4 params;
    vec4 slice;
    vec4 bounds_min;
    vec4 bounds_max;
    vec4 axis_order;
    vec4 axis_flip;
    vec4 value_range;
    vec4 occlusion;
} volume;

layout(location = 0) in vec3 fragUVW;
layout(location = 1) in vec3 fragObj;
layout(location = 0) out uint outValue;

float safe_inv(float v)
{
    if (abs(v) < 1e-6)
    {
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

vec3 camera_object()
{
    mat4 inv_model = inverse(mvp.model);
    mat4 inv_view = inverse(mvp.view);
    return (inv_model * inv_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
}

vec3 object_to_uvw(vec3 pos)
{
    vec3 extent = max(volume.bounds_max.xyz - volume.bounds_min.xyz, vec3(1e-6));
    return (pos - volume.bounds_min.xyz) / extent;
}

vec3 object_dir_to_uvw(vec3 dir)
{
    vec3 extent = max(volume.bounds_max.xyz - volume.bounds_min.xyz, vec3(1e-6));
    return dir / extent;
}

float axis_value(int axis, vec3 value)
{
    return axis == 0 ? value.x : (axis == 1 ? value.y : value.z);
}

vec3 texture_uvw(vec3 uvw)
{
    int ax0 = int(clamp(volume.axis_order.x, 0.0, 2.0));
    int ax1 = int(clamp(volume.axis_order.y, 0.0, 2.0));
    int ax2 = int(clamp(volume.axis_order.z, 0.0, 2.0));
    vec3 out_uvw = vec3(axis_value(ax0, uvw), axis_value(ax1, uvw), axis_value(ax2, uvw));
    out_uvw = mix(out_uvw, vec3(1.0) - out_uvw, step(vec3(0.5), volume.axis_flip.xyz));
    return clamp(out_uvw, vec3(0.0), vec3(1.0));
}

#include "volume_label_query.glsl"

bool inside_clip_plane(vec3 uvw)
{
    if (volume.clip_plane_params.x < 0.5)
    {
        return true;
    }
    float side = dot(volume.clip_plane.xyz, uvw) + volume.clip_plane.w;
    return volume.clip_plane_params.y > 0.5 ? side >= -1e-6 : side <= 1e-6;
}

uint encode_scalar(float value)
{
    float denom = max(volume.value_range.y - volume.value_range.x, 1e-12);
    float t = clamp((value - volume.value_range.x) / denom, 0.0, 1.0);
    uint code = uint(floor(t * 16777214.0 + 0.5));
    return code + 1u;
}

void main()
{
    vec3 ro_obj = camera_object();
    vec3 rd_obj = normalize(fragObj - ro_obj);
    vec3 ro = object_to_uvw(ro_obj);
    vec3 rd = object_dir_to_uvw(rd_obj);

    float proxy_t0 = 0.0;
    float proxy_t1 = 0.0;
    if (!ray_box(ro, rd, vec3(0.0), vec3(1.0), proxy_t0, proxy_t1))
    {
        discard;
    }

    vec3 box_min = volume.clip_min.xyz;
    vec3 box_max = volume.clip_max.xyz;

    int axis = int(clamp(volume.slice.x, 0.0, 2.0));
    float axis_pos = clamp(volume.slice.y, 0.0, 1.0);
    float slice_coord = mix(axis_value(axis, box_min), axis_value(axis, box_max), axis_pos);
    float axis_rd = axis == 0 ? rd.x : (axis == 1 ? rd.y : rd.z);
    if (abs(axis_rd) < 1e-6)
    {
        discard;
    }

    float slice_t = (slice_coord - axis_value(axis, ro)) / axis_rd;
    if (slice_t < max(proxy_t0, 0.0) || slice_t > proxy_t1)
    {
        discard;
    }

    vec3 uvw = ro + rd * slice_t;
    if (any(lessThan(uvw, box_min)) || any(greaterThan(uvw, box_max)))
    {
        discard;
    }
    if (!inside_clip_plane(uvw))
    {
        discard;
    }

#if defined(DVZ_VOLUME_LABEL_UINT_QUERY)
    uint id = texelFetch(usampler3D(tex, samp), label_coord(uvw), 0).r;
    if (id == 0u)
    {
        discard;
    }
    outValue = id;
#elif defined(DVZ_VOLUME_LABEL_SINT_QUERY)
    int id = texelFetch(isampler3D(tex, samp), label_coord(uvw), 0).r;
    if (id == 0)
    {
        discard;
    }
    outValue = uint(id);
#else
    float sample_value = texture(sampler3D(tex, samp), texture_uvw(uvw)).r;
    outValue = encode_scalar(sample_value);
#endif
}
