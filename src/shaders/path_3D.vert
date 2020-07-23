#version 450
#include "common.glsl"

// comes from https://github.com/mattdesl/webgl-lines/blob/master/projected/vert.glsl

layout (binding = 2) uniform PathParams {
    float linewidth;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 prev;
layout (location = 2) in vec3 next;
layout (location = 3) in vec4 color;
layout (location = 4) in float path_index;
layout (location = 0) out vec4 out_color;
layout (location = 1) out float out_transverse;

void main() {
    int miter = 0;  // TODO
    float aspect = 1.0;  // TODO
    vec2 aspectVec = vec2(aspect, 1.0);

    float direction = gl_VertexIndex % 2 == 0 ? 1 : -1;
    vec4 previousProjected = transform_pos(prev);
    vec4 currentProjected = transform_pos(pos);
    vec4 nextProjected = transform_pos(next);

    //get 2D screen space with W divide and aspect correction
    vec2 currentScreen = currentProjected.xy / currentProjected.w * aspectVec;
    vec2 previousScreen = previousProjected.xy / previousProjected.w * aspectVec;
    vec2 nextScreen = nextProjected.xy / nextProjected.w * aspectVec;

    float len = params.linewidth;
    float orientation = direction;

    //starting point uses (next - current)
    vec2 dir = vec2(0.0);
    if (currentScreen == previousScreen) {
        dir = normalize(nextScreen - currentScreen);
    }
    //ending point uses (current - previous)
    else if (currentScreen == nextScreen) {
        dir = normalize(currentScreen - previousScreen);
    }
    //somewhere in middle, needs a join
    else {
        //get directions from (C - B) and (B - A)
        vec2 dirA = normalize((currentScreen - previousScreen));
        if (miter == 1) {
            vec2 dirB = normalize((nextScreen - currentScreen));
            //now compute the miter join normal and length
            vec2 tangent = normalize(dirA + dirB);
            vec2 perp = vec2(-dirA.y, dirA.x);
            vec2 miter = vec2(-tangent.y, tangent.x);
            dir = tangent;
            len = params.linewidth / dot(miter, perp);
        } else {
            dir = dirA;
        }
    }
    vec2 normal = vec2(-dir.y, dir.x);
    normal *= len/2.0;
    normal.x /= aspect;

    vec4 offset = vec4(normal * orientation, 0.0, 1.0);

    out_transverse = direction;

    gl_Position = currentProjected + offset;
    out_color = get_color(color);
}
