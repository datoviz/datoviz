#version 450
#include "constants.glsl"
#include "axes.glsl"
#include "common.glsl"

#define USER_COORD_LEVEL 8

layout (location = 0) in float tick;
layout (location = 1) in uint coord_level; // 0-7 for level 0-3 and coord 0-1, after that = user axes

layout (location = 0) out vec2  out_texcoord;
layout (location = 1) out vec2  out_coord_level;
layout (location = 2) out float out_length;
layout (location = 3) out float out_linewidth;
layout (location = 4) out vec4 out_color;

void main (void)
{
    int index = gl_VertexIndex % 4;

    // Get the level  (0=minor, 1=major, 2=grid, 3=lim) and coord (0=x, 1=y)
    uint level, coord;
    float linewidth;
    if (coord_level < USER_COORD_LEVEL) {
        level = coord_level % 4;
        coord = coord_level <= 3 ? 0 : 1;
        // Compute line width depending on the tick level.
        linewidth = params.linewidths[level];
        out_color = params.colors[level];
    }
    else {
        uint user_level = coord_level - USER_COORD_LEVEL;  // 0 to 7
        // User coords.
        level = 2;  // similar to grid level, just at special tick positions and special line width and color
        coord = user_level <= 3 ? 0 : 1;  // 0 for x, 1 for y
        linewidth = params.user_linewidths[user_level % 4];
        out_color = params.user_colors[user_level % 4];
    }

    out_coord_level = vec2(coord, level);

    out_linewidth = linewidth;

    // Margins and viewport.
    float mt = params.margins[0];
    float mr = params.margins[1];
    float mb = params.margins[2];
    float ml = params.margins[3];
    // NOTE: this is the outer viewport.
    float w = mvp.viewport.z;
    float h = mvp.viewport.w;

    // Viewport ortho matrix.
    mat4 ortho = get_ortho_matrix(vec2(w, h));
    mat4 ortho_inv = inverse(ortho);

    // Axes corner coords in NDC coords.
    float top = 1 - 2 * mt / h;
    float right = 1 - 2 * mr / w;
    float bottom = -1 + 2 * mb / h;
    float left = -1 + 2 * ml / w;

    // Compute the vertex positions.
    vec3 P0, P1;  // positions in NDC coordinates, before MVP transform
    vec4 P0_, P1_;  // positions in NDC coordinates, after MVP transform
    vec2 p0, p1;  // positions in viewport pixel coordinates, origin at top left corner of the viewport
    P0.z = P1.z = P0_.z = P1_.z = 0.0;

    float a, b;

    // ticks and grid
    if (level <= 2) {

        // x ticks
        if (coord == 0) {
            // Take margins into account.
            a = 1 - (ml + mr) / w;
            b = (ml - mr) / w;
            P0.x = P1.x = a * tick + b;
            P0.y = P1.y = bottom;
            // grid
            if (level == 2) {
                P1.y = top;
            }
        }
        // y ticks
        else if (coord == 1) {
            // Take margins into account.
            a = 1 - (mt + mb) / h;
            b = (mb - mt) / h;
            P0.y = P1.y = a * tick + b;
            P0.x = P1.x = left;
            // grid
            if (level == 2) {
                P1.x = right;
            }
        }

        // Apply panzoom.
        P0_ = transform_pos(P0);
        P1_ = transform_pos(P1);
        P0_.z = P1_.z = 0.0;
        P0_.w = P1_.w = 1.0;

        // Fix ticks wrt pan/zoom.
        if (coord == 0) {
            P0_.y = -P0.y;  // NOTE: - because Vulkan inverses the y axis compared to visky/OpenGL convention
            P1_.y = -P1.y;
        }
        else if (coord == 1) {
            P0_.x = P0.x;
            P1_.x = P1.x;
        }

        // Go to pixel coordinates.
        p0 = (ortho_inv * P0_).xy;
        p1 = (ortho_inv * P1_).xy;

        // For ticks, it is sufficient to determine p0. We deduce p1 with the tick length.
        if (level <= 1) {
            if (coord == 0) {
                p0.y += linewidth / 2;
                p1 = p0 + vec2(0, params.tick_lengths[level]);
            }
            else {
                p1 = p0 - vec2(params.tick_lengths[level], 0);
            }
        }

    }
    // xlim and ylim
    else if (level == 3) {
        // Static positions for the lims.
        if (coord == 0) {
            p0 = vec2(ml, h - mb - linewidth / 2);
            p1 = vec2(ml, mt);
        }
        else if (coord == 1) {
            p0 = vec2(ml + linewidth / 2, h - mb);
            p1 = vec2(w - mr, h - mb);
        }
    }


    // From now one, standard segment vertex shader.
    vec2 position;
    vec2 T = p1 - p0;
    out_length = length(T);
    float lw = linewidth / 2.0 + 1.5 * antialias;
    T = lw * normalize(T);
    float z;
    if (index < 0.5) {
       position = vec2(p0.x - T.y - T.x, p0.y + T.x - T.y);
       out_texcoord = vec2(-lw, +lw);
       z = P0.z;
    }
    else if (index < 1.5) {
       position = vec2(p0.x + T.y - T.x, p0.y - T.x - T.y);
       out_texcoord = vec2(-lw, -lw);
       z = P0.z;
    }
    else if (index < 2.5) {
       position = vec2(p1.x + T.y + T.x, p1.y - T.x + T.y);
       out_texcoord = vec2(out_length + lw, -lw);
       z = P1.z;
    }
    else {
       position = vec2(p1.x - T.y + T.x, p1.y + T.x + T.y);
       out_texcoord = vec2(out_length + lw, +lw);
       z = P1.z;
    }

    gl_Position = ortho * vec4(position, z, 1.0);

}
