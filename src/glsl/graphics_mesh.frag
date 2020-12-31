#version 450
#include "common.glsl"
#include "lighting.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    mat4 lights_pos_0; // lights 0-3
    mat4 lights_pos_1; // lights 4-7
    mat4 lights_params_0;
    mat4 lights_params_1;

    vec4 view_pos;

    ivec2 tex_size_0;
    ivec2 tex_size_1;
    ivec2 tex_size_2;
    ivec2 tex_size_3;
} params;

layout(binding = (USER_BINDING+1)) uniform sampler2D tex_0;
layout(binding = (USER_BINDING+2)) uniform sampler2D tex_1;
layout(binding = (USER_BINDING+3)) uniform sampler2D tex_2;
layout(binding = (USER_BINDING+4)) uniform sampler2D tex_3;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_triangle;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 normal = normalize(in_normal);
    vec3 light_pos = params.lights_pos_0[0].xyz;
    vec3 light_dir = normalize(light_pos - in_pos);

    // ambient
    vec3 ambient = vec3(.1);

    // diffuse
    // if (!gl_FrontFacing) normal = -normal;
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * vec3(1); // TODO: customizable light color, here white only

    // specular
    vec3 view_dir = normalize(params.view_pos.xyz - in_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
    vec3 specular = spec * vec3(1);

    // total
    vec3 color = texture(tex_0, in_uv).xyz;
    out_color = vec4((ambient + diffuse + specular) * color, 1);
}
