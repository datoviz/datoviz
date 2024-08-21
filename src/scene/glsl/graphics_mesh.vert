#version 450
#include "common.glsl"
#include "params_mesh.glsl"

// Position.
layout(location = 0) in vec3 pos;

// Normal.
layout(location = 1) in vec3 normal;

// Color or texture.
layout(location = 2) in vec4 uvcolor; // contains either rgba, or uv*a

// Barycentric coordinates.
layout(location = 3) in int edge;

// Varying variables.
layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_uvcolor;
layout(location = 3) out vec3 out_barycentric;
layout(location = 4) out vec3 out_edge;

void main()
{
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;
    out_uvcolor = uvcolor;

    // Generate barycentric coordinates.
    out_barycentric = vec3(0);
    out_barycentric[gl_VertexIndex % 3] = 1;

    // Unpack last 3 bits of int as vec3 of boolean.
    float x = float((edge >> 2) & 1);
    float y = float((edge >> 1) & 1);
    float z = float(edge & 1);
    out_edge = vec3(x, y, z);
}
