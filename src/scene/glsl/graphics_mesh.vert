#version 450
#include "common.glsl"
#include "params_mesh.glsl"

// Attributes.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 uvcolor; // color or texture, contains either rgba, or uv*a
// distance of the current vertex between the left edge and point A, B, C
layout(location = 3) in vec3 d_left;
// distance of the current vertex between the right edge and point A, B, C
layout(location = 4) in vec3 d_right;
// 0bXY where Y=1 if the opposite edge is a contour, X=1 if vertex is corner
layout(location = 5) in ivec3 contour;

// Varying variables.
layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_uvcolor;
layout(location = 3) out vec3 out_barycentric;
layout(location = 4) out vec3 out_d_left;
layout(location = 5) out vec3 out_d_right;
layout(location = 6) out ivec3 out_contour;


void main()
{
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;
    out_uvcolor = uvcolor;

    // Generate barycentric coordinates.
    out_barycentric = vec3(0);
    out_barycentric[gl_VertexIndex % 3] = 1;

    float z = total_zoom();
    out_contour = contour;
    out_d_left = d_left / z;
    out_d_right = d_right / z;

    // Adjacent vectors.
    // out_adjacent = adjacent;
    // // vec2 u2 = transform(vec3(adjacent.xy, 0)).xy;
    // vec2 u2 = adjacent.xy;
    // u2 = normalize(u2);
    // // vec2 v2 = transform(vec3(adjacent.zw, 0)).xy;
    // // v2 = normalize(v2);
    // out_adjacent.x = dot(pos.xy, vec2(u2.y, -u2.x));

    // TODO: bit unpacking to save memory
    // // Unpack last 3 bits of int as vec3 of boolean.
    // float x = float((edge >> 2) & 1);
    // float y = float((edge >> 1) & 1);
    // float z = float(edge & 1);
    // out_orient = float((edge >> 7) & 1);
    // out_edge = vec3(x, y, z);

    // // Whether each vertex should be a contour corner.
    // float c0 = float((edge >> 3) & 1);
    // float c1 = float((edge >> 4) & 1);
    // float c2 = float((edge >> 5) & 1);
    // out_corner = vec3(c0, c1, c2);
}
