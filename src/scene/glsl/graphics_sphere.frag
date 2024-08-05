#version 450
#include "antialias.glsl"
#include "common.glsl"

layout(binding = 2) uniform SphereParams
{
    vec4 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_eye_pos;

layout(location = 0) out vec4 out_color;


const vec3 light_color = vec3(1.0);

#define EPS 0


void main()
{
    // Calculate the normalized coordinates of the fragment within the point sprite
    vec2 coord = 2.0 * gl_PointCoord - 1.0;
    float dist_squared = dot(coord, coord);

    if (dist_squared > 1.0 + EPS)
        discard;

    // Calculate the normal of the sphere at this fragment
    vec3 normal = vec3(coord, sqrt(1.0 - dist_squared));

    // Calculate the lighting
    vec3 light_pos = params.light_pos.xyz;
    // light_pos.y = -light_pos.y;
    vec3 light_dir = normalize(light_pos.xyz - in_pos);
    vec3 view_dir = normalize(-in_eye_pos.xyz);
    vec3 reflect_dir = reflect(-light_dir, normal);

    // Color.
    vec3 ambient = params.light_params.x * vec3(1.0);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = params.light_params.y * diff * light_color;
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), params.light_params.w);
    vec3 specular = params.light_params.z * spec * light_color;
    vec3 final_color = (ambient + diffuse + specular) * in_color.rgb;

    // TODO: border antialias.
    float alpha = in_color.a;
    // if (dist_squared > 1.0)
    //     alpha *= compute_distance(dist_squared - 1, 1.0).z;
    out_color = vec4(final_color, alpha);

    // Depth.
    vec4 vm = mvp.view * mvp.model * vec4(in_pos, 1);
    float d = length(vm.xyz);

    // Viewport size.
    float w = viewport.size.x;
    float h = viewport.size.y;
    float a = w / h;
    float v = w;

    vm += in_radius / (d * v) * vec4(normal, 1);
    vec4 clipPos = mvp.proj * vm;
    clipPos.z /= clipPos.w;
    gl_FragDepth = clipPos.z;
}
