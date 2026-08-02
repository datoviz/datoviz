#version 450

#define DVZ_VOLUME_LABEL_UINT 1

#include "color.glsl"

#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

#ifndef DVZ_MVP_UNIFORM_GLSL
#define DVZ_MVP_UNIFORM_GLSL
layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;
#endif

#if defined(DVZ_VOLUME_LABEL_UINT)
layout(set = 1, binding = 0) uniform utexture3D tex;
#elif defined(DVZ_VOLUME_LABEL_SINT)
layout(set = 1, binding = 0) uniform itexture3D tex;
#else
layout(set = 1, binding = 0) uniform texture3D tex;
#endif
layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 3) uniform texture2D depthTex;
layout(set = 1, binding = 4) uniform texture2D transferTex;

layout(set = 1, binding = 2) uniform VolumeParams {
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
    vec4 texture_params;
} volume;

layout(location = 0) in vec3 fragUVW;
layout(location = 1) in vec3 fragObj;
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

vec3 uvw_to_object(vec3 uvw)
{
    return mix(volume.bounds_min.xyz, volume.bounds_max.xyz, uvw);
}

float projected_depth(vec3 uvw)
{
    vec3 pos = uvw_to_object(uvw);
    vec4 clip = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    if (clip.w <= 0.0) {
        return 1.0;
    }
    return clamp(0.5 * (clip.z / clip.w) + 0.5, 0.0, 1.0);
}

float view_depth_from_uvw(vec3 uvw)
{
    vec3 pos = uvw_to_object(uvw);
    vec4 view = mvp.view * mvp.model * vec4(pos, 1.0);
    return -view.z;
}

float linearize_depth(float depth)
{
    float z_ndc = depth * 2.0 - 1.0;
    float a = mvp.proj[2][2];
    float b = mvp.proj[3][2];
    float denom = z_ndc + a;
    if (abs(denom) < 1e-6) {
        return 1e6;
    }
    return b / denom;
}

float occlusion_delta_depth(vec3 uvw, float scene_depth, float self_depth)
{
    float delta_depth = self_depth - scene_depth;
    if (delta_depth <= 0.0) {
        return delta_depth;
    }

    if (abs(mvp.proj[3][3]) > 0.5) {
        return delta_depth;
    }

    float delta = view_depth_from_uvw(uvw) - linearize_depth(scene_depth);
    return delta > 0.0 ? delta : delta_depth;
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

vec4 transfer_value(float value)
{
    float denom = max(volume.value_range.y - volume.value_range.x, 1e-12);
    float t = clamp((value - volume.value_range.x) / denom, 0.0, 1.0);
    return semanticColorToLinear(texture(sampler2D(transferTex, samp), vec2(t, 0.5)));
}

#include "volume_labels.glsl"

bool inside_clip_plane(vec3 uvw)
{
    if (volume.clip_plane_params.x < 0.5) {
        return true;
    }
    float side = dot(volume.clip_plane.xyz, uvw) + volume.clip_plane.w;
    return volume.clip_plane_params.y > 0.5 ? side >= -1e-6 : side <= 1e-6;
}

float depth_visibility(vec3 uvw)
{
    vec2 size = vec2(textureSize(sampler2D(depthTex, samp), 0));
    vec2 uv = clamp(
        (gl_FragCoord.xy - volume.texture_params.yz) / size, vec2(0.0), vec2(1.0));
    float scene_depth = texture(sampler2D(depthTex, samp), uv).r;
    if (volume.occlusion.w > 0.5) {
        if (scene_depth <= 0.000001) {
            return 1.0;
        }
        float delta = view_depth_from_uvw(uvw) - scene_depth - 0.0005;
        if (delta <= 0.0) {
            return 1.0;
        }
        float hidden_alpha = clamp(volume.occlusion.z, 0.0, 1.0);
        float fade_distance = max(volume.occlusion.y, 0.000001);
        float fade = smoothstep(0.0, fade_distance, delta);
        return mix(1.0, hidden_alpha, fade);
    }
    if (scene_depth >= 0.999999) {
        return 1.0;
    }
    float self_depth = projected_depth(uvw);
    return self_depth > scene_depth + 0.0005 ? 0.0 : 1.0;
}

#ifdef DVZ_SCENE_OCCLUSION
float scene_occlusion_visibility_linear(vec3 uvw)
{
    vec2 size = vec2(textureSize(sampler2D(sceneOcclusionDepth, sceneOcclusionSamp), 0));
    vec2 uv = clamp(
        (gl_FragCoord.xy - sceneOcclusion.viewport.xy) / size, vec2(0.0), vec2(1.0));
    float scene_depth = texture(sampler2D(sceneOcclusionDepth, sceneOcclusionSamp), uv).r;
    if (scene_depth <= 0.000001) {
        return 1.0;
    }

    float delta = view_depth_from_uvw(uvw) - scene_depth - sceneOcclusion.params.x;
    if (delta <= 0.0) {
        return 1.0;
    }

    float hidden_alpha = clamp(sceneOcclusion.params.z, 0.0, 1.0);
    float soft_edge = max(sceneOcclusion.params.y, 0.000001);
    float fade = smoothstep(0.0, soft_edge, delta);
    return mix(1.0, hidden_alpha, fade);
}
#endif

void main()
{
    vec3 ro_obj = camera_object();
    vec3 rd_obj = normalize(fragObj - ro_obj);
    vec3 ro = object_to_uvw(ro_obj);
    vec3 rd = object_dir_to_uvw(rd_obj);

    float proxy_t0 = 0.0;
    float proxy_t1 = 0.0;
    if (!ray_box(ro, rd, vec3(0.0), vec3(1.0), proxy_t0, proxy_t1)) {
        discard;
    }

    vec3 box_min = volume.clip_min.xyz;
    vec3 box_max = volume.clip_max.xyz;

    int axis = int(clamp(volume.slice.x, 0.0, 2.0));
    float axis_pos = clamp(volume.slice.y, 0.0, 1.0);
    float slice_coord = mix(axis_value(axis, box_min), axis_value(axis, box_max), axis_pos);
    float axis_rd = (axis == 0 ? rd.x : (axis == 1 ? rd.y : rd.z));
    if (abs(axis_rd) < 1e-6) {
        discard;
    }

    float slice_t = (slice_coord - axis_value(axis, ro)) / axis_rd;
    if (slice_t < max(proxy_t0, 0.0) || slice_t > proxy_t1) {
        discard;
    }

    vec3 uvw = ro + rd * slice_t;
    if (any(lessThan(uvw, box_min)) || any(greaterThan(uvw, box_max))) {
        discard;
    }
    if (!inside_clip_plane(uvw)) {
        discard;
    }
    float visibility = depth_visibility(uvw);
    if (visibility <= 0.0) {
        discard;
    }

#if defined(DVZ_VOLUME_LABEL_UINT)
    uint id = texelFetch(usampler3D(tex, samp), label_coord(uvw), 0).r;
    if (id == 0u) {
        discard;
    }
    outColor = label_palette_color(id);
    outColor.a *= volume.params.x * visibility;
#elif defined(DVZ_VOLUME_LABEL_SINT)
    int id = texelFetch(isampler3D(tex, samp), label_coord(uvw), 0).r;
    if (id == 0) {
        discard;
    }
    uint key = uint(id);
    outColor = label_palette_color(key);
    outColor.a *= volume.params.x * visibility;
#else
    vec4 sample_value = texture(sampler3D(tex, samp), texture_uvw(uvw));
    if (volume.clip_min.w > 0.5) {
        vec4 color_value = sampledTextureColorToLinear(sample_value, volume.texture_params);
        outColor = vec4(color_value.rgb, color_value.a * volume.params.x * visibility);
    } else {
        outColor = transfer_value(sample_value.r);
        outColor.a *= volume.params.x * visibility;
    }
#endif
#ifdef DVZ_SCENE_OCCLUSION
    outColor.a *= scene_occlusion_visibility_linear(uvw);
    if (outColor.a <= 0.0) {
        discard;
    }
#endif
}
