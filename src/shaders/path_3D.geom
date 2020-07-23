#version 450
#include "constants.glsl"
#include "common.glsl"

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

layout (binding = 2) uniform PathParams {
    float linewidth;
} pathParams;


layout (location = 0) in vec4 inColor[];

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outLength;
layout (location = 2) out vec2 outTexcoord;

float compute_u(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    // Then  u *= lenght(p1-p0)
    vec2 v = p1 - p0;
    float l = length(v);
    return ((p.x-p0.x)*v.x + (p.y-p0.y)*v.y) / l;
}

float line_distance(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    vec2 v = p1 - p0;
    float l2 = v.x*v.x + v.y*v.y;
    float u = ((p.x-p0.x)*v.x + (p.y-p0.y)*v.y) / l2;

    // h is the projection of p on (p0,p1)
    vec2 h = p0 + u*v;

    return length(p-h);
}

void main(void)
{
    float linewidth = pathParams.linewidth;
    float miterLimit = 1;

    // Get the four vertices passed to the shader
    vec2 p0 = gl_in[0].gl_Position.xy; // start of previous segment
    vec2 p1 = gl_in[1].gl_Position.xy; // end of previous segment, start of current segment
    vec2 p2 = gl_in[2].gl_Position.xy; // end of current segment, start of next segment
    vec2 p3 = gl_in[3].gl_Position.xy; // end of next segment

    // Path index of the adjacent vertices.
    float i0 = gl_in[0].gl_Position.w;
    float i1 = gl_in[1].gl_Position.w;
    float i2 = gl_in[2].gl_Position.w;
    float i3 = gl_in[3].gl_Position.w;

    // Determine the direction of each of the 3 segments (previous, current, next)
    vec2 v0 = normalize(p1 - p0);
    vec2 v1 = normalize(p2 - p1);
    vec2 v2 = normalize(p3 - p2);

    // Determine the normal of each of the 3 segments (previous, current, next)
    vec2 n0 = vec2(-v0.y, v0.x);
    vec2 n1 = vec2(-v1.y, v1.x);
    vec2 n2 = vec2(-v2.y, v2.x);

    // Determine miter lines by averaging the normals of the 2 segments
    vec2 miter_a = normalize(n0 + n1); // miter at start of current segment
    vec2 miter_b = normalize(n1 + n2); // miter at end of current segment

    // Determine the length of the miter by projecting it onto normal
    vec2 p,v;
    float d;
    float w = linewidth/2.0 + 1.5*antialias;

    float length_a = w / dot(miter_a, n1);
    float length_b = w / dot(miter_b, n1);

    float m = miterLimit*linewidth/2.0;

    // Angle between prev and current segment (sign only)
    float d0 = +1.0;
    if( (v0.x*v1.y - v0.y*v1.x) > 0 ) { d0 = -1.0;}

    // Angle between current and next segment (sign only)
    float d1 = +1.0;
    if( (v1.x*v2.y - v1.y*v2.x) > 0 ) { d1 = -1.0; }


    // Hide boundaries between different paths.
    float alpha = inColor[0].a;
    if (i0 == i1 && i2 == i3 && i1 != i2) alpha = 0;



    // Generate the triangle strip
    mat4 ortho = get_ortho_matrix(mvp.viewport.zw);

    // Cap at start
    outLength = length(p2-p1);
    if( p0 == p1 ) {
        p = p1 - w*v1 + w*n1;
        gl_Position = ortho * vec4(p, 0.0, 1.0);
        outTexcoord = vec2(-w, +w);
    // Regular join
    } else {
        p = p1 + length_a * miter_a;
        gl_Position = ortho * vec4(p, 0.0, 1.0);
        outTexcoord = vec2(compute_u(p1,p2,p), +w);
    }

    outColor = inColor[0];
    outColor.a = alpha;
    EmitVertex();

    // Cap at start
    outLength = length(p2-p1);
    if( p0 == p1 ) {
        p = p1 - w*v1 - w*n1;
        outTexcoord = vec2(-w, -w);
    // Regular join
    } else {
        p = p1 - length_a * miter_a;
        outTexcoord = vec2(compute_u(p1,p2,p), -w);
    }
    gl_Position = ortho * vec4(p, 0.0, 1.0);
    outColor = inColor[0];
    outColor.a = alpha;
    EmitVertex();

    // Cap at end
    outLength = length(p2-p1);
    if( p2 == p3 ) {
        p = p2 + w*v1 + w*n1;
        outTexcoord = vec2(outLength+w, +w);
    // Regular join
    } else {
        p = p2 + length_b * miter_b;
        outTexcoord = vec2(compute_u(p1,p2,p), +w);
    }

    gl_Position = ortho * vec4(p, 0.0, 1.0);
    outColor = inColor[0];
    outColor.a = alpha;
    EmitVertex();

    // Cap at end
    outLength = length(p2-p1);
    if( p2 == p3 ) {
        p = p2 + w*v1 - w*n1;
        outTexcoord = vec2(outLength+w, -w);
    // Regular join
    } else {
        p = p2 - length_b * miter_b;
        outTexcoord = vec2(compute_u(p1,p2,p), -w);
    }
    gl_Position = ortho * vec4(p, 0.0, 1.0);
    outColor = inColor[0];
    outColor.a = alpha;
    EmitVertex();

    EndPrimitive();
}
