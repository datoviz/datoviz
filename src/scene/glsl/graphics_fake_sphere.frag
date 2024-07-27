#version 450
#include "common.glsl"
// #include "lighting.glsl"

layout(binding = 2) uniform SphereParams { vec4 light_pos; }
params;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_eye_pos;

layout(location = 0) out vec4 out_color;


// // TODO: put this in lighting.glsl?
// vec4 lighting(vec4 color, vec3 pos, vec3 normal, vec3 view_pos, vec3 light_pos, vec3
// light_coeffs)
// {
//     // ambient
//     vec3 ambient = color.rgb;

//     // diffuse
//     vec3 light_dir = normalize(light_pos - pos);
//     normal = normalize(normal);
//     if (!gl_FrontFacing)
//         normal = -normal;
//     float diff = max(dot(light_dir, normal), 0.0);
//     vec3 diffuse = diff * color.rgb;

//     // specular
//     vec3 view_dir = normalize(view_pos - pos);
//     vec3 halfway_dir = normalize(light_dir + view_dir);
//     float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

//     vec3 specular = spec * light_color;

//     // total
//     return vec4(
//         light_coeffs.x * ambient + light_coeffs.y * diffuse + light_coeffs.z * specular,
//         color.a);
// }

const vec3 light_color = vec3(1.0);
const vec3 ambient_color = vec3(0.1, 0.1, 0.1);
const float shininess = 32.0;

void main()
{
    // Calculate the normalized coordinates of the fragment within the point sprite
    vec2 coord = 2.0 * gl_PointCoord - 1.0;
    float dist_squared = dot(coord, coord);

    if (dist_squared > 1.0)
        discard;

    // Calculate the normal of the fake sphere at this fragment
    vec3 normal = vec3(coord, sqrt(1.0 - dist_squared));

    // Calculate the lighting
    vec3 light_pos = params.light_pos.xyz;
    // light_pos.y = -light_pos.y;
    vec3 light_dir = normalize(light_pos.xyz - in_pos);
    vec3 view_dir = normalize(-in_eye_pos.xyz);
    vec3 reflect_dir = reflect(-light_dir, normal);

    // Color.
    vec3 ambient = ambient_color;
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * light_color;
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    vec3 specular = spec * light_color;
    vec3 final_color = (ambient + diffuse + specular) * in_color.rgb;
    out_color = vec4(final_color, in_color.a);

    // Depth.
    vec4 vm = mvp.view * mvp.model * vec4(in_pos, 1);
    float d = length(vm.xyz);

    // Viewport size.
    float w = viewport.size.x;
    float h = viewport.size.y;
    float a = w / h;
    float v = w;
    // float point_size_clip_space_x = in_point_size / viewport.size.x;
    // float point_size_clip_space_y = in_point_size / viewport.size.y;
    // float point_size_clip_space =
    //     max(point_size_clip_space_x, point_size_clip_space_y * aspect_ratio);

    vm += in_radius / (d * v) * vec4(normal, 1);
    vec4 clipPos = mvp.proj * vm;
    clipPos.z /= clipPos.w;
    gl_FragDepth = clipPos.z;
}
