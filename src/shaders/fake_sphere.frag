#version 450
#include "common.glsl"
#include "lighting.glsl"

layout (binding = 2) uniform SphereParams {
    vec4 light_pos;
} params;

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec3 in_pos;
layout (location = 2) in float in_radius;
layout (location = 3) in vec4 in_eye_pos;

layout (location = 0) out vec4 out_color;

void main()
{
    // r^2 = (x - x0)^2 + (y - y0)^2 + (z - z0)^2
    vec2 texcoord = gl_PointCoord *  2.0 - vec2(1.0);
    float x = texcoord.x;
    float y = texcoord.y;
    float d = 1.0 - x*x - y*y;
    if (d <= 0.0) discard;

    float z = sqrt(d);
    vec3 pos = in_pos;
    pos.z += in_radius * z;

    vec3 normal = -vec3(x, y, z);
    vec3 view_pos = mvp.view[3].xyz;
    out_color = lighting(vec4(in_color, 1), pos, normal, view_pos, params.light_pos.xyz, vec3(.3, .4, .5));

    //Set the depth based on the new cameraPos.
    vec4 clipPos = mvp.proj * in_eye_pos - in_radius * z;
    float ndcDepth = clipPos.z / clipPos.w;
    gl_FragDepth = ndcDepth;
}
