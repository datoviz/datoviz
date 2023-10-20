#version 450
#include "common.glsl"
#include "params_mesh.glsl"

// Position.
layout(location = 0) in vec3 pos;

// Normal.
layout(location = 1) in vec3 normal;

// Color or texture.
layout(location = 2) in vec4 uvcolor; // contains either rgba, or uv*a

// Varying variables.
layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_uvcolor;

void main()
{
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;
    out_uvcolor = uvcolor;
}
