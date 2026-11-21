#version 450

layout(push_constant) uniform Push { mat4 projection; mat4 model_view; } push_data;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 view_position;

void main()
{
    // View space: the camera sits at the origin, and the lighting happens here.
    vec4 position = push_data.model_view * vec4(in_position, 1.0);
    view_position = position.xyz;

    // Rotate the normal by the upper-left 3 x 3 part; no translation for a direction.
    normal = mat3(push_data.model_view) * in_normal;
    uv = in_uv;

    // Clip space, for the rasterizer.
    gl_Position = push_data.projection * position;
}
