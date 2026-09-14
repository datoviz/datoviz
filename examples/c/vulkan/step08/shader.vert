#version 450

layout(push_constant) uniform Push { float time; float pulse; } push_data;
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 color;

void main()
{
    float cosine = cos(push_data.time);
    float sine = sin(push_data.time);
    vec2 position = in_position * push_data.pulse;
    gl_Position = vec4(
        cosine * position.x - sine * position.y,
        sine * position.x + cosine * position.y, 0.0, 1.0);
    color = in_color;
}
