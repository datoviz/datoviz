// -----------------------------------------------------------------------------
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
// -----------------------------------------------------------------------------

#include "constants.glsl"


float marker_arrow(vec2 P, float size)
{
    float r1 = abs(P.x) + abs(P.y) - size / 2;
    float r2 = max(abs(P.x + size / 2), abs(P.y)) - size / 2;
    float r3 = max(abs(P.x - size / 6) - size / 4, abs(P.y) - size / 4);
    return min(r3, max(.75 * r1, r2));
}



float marker_asterisk(vec2 P, float size)
{
    float x = M_SQRT2 / 2 * (P.x - P.y);
    float y = M_SQRT2 / 2 * (P.x + P.y);
    float r1 = max(abs(x) - size / 2, abs(y) - size / 10);
    float r2 = max(abs(y) - size / 2, abs(x) - size / 10);
    float r3 = max(abs(P.x) - size / 2, abs(P.y) - size / 10);
    float r4 = max(abs(P.y) - size / 2, abs(P.x) - size / 10);
    return min(min(r1, r2), min(r3, r4));
}



float marker_chevron(vec2 P, float size)
{
    float x = 1.0 / M_SQRT2 * ((P.x - size / 6) - P.y);
    float y = 1.0 / M_SQRT2 * ((P.x - size / 6) + P.y);
    float r1 = max(abs(x), abs(y)) - size / 3.0;
    float r2 = max(abs(x - size / 3.0), abs(y - size / 3.0)) - size / 3.0;
    return max(r1, -r2);
}



float marker_clover(vec2 P, float size)
{
    const float t1 = -M_PI / 2;
    const vec2 c1 = 0.25 * vec2(cos(t1), sin(t1));
    const float t2 = t1 + 2 * M_PI / 3;
    const vec2 c2 = 0.25 * vec2(cos(t2), sin(t2));
    const float t3 = t2 + 2 * M_PI / 3;
    const vec2 c3 = 0.25 * vec2(cos(t3), sin(t3));

    float r1 = length(P - c1 * size) - size / 3.5;
    float r2 = length(P - c2 * size) - size / 3.5;
    float r3 = length(P - c3 * size) - size / 3.5;
    return min(min(r1, r2), r3);
}



float marker_club(vec2 P, float size)
{
    // clover (3 discs)
    const float t1 = -M_PI / 2.0;
    const vec2 c1 = 0.225 * vec2(cos(t1), sin(t1));
    const float t2 = t1 + 2 * M_PI / 3.0;
    const vec2 c2 = 0.225 * vec2(cos(t2), sin(t2));
    const float t3 = t2 + 2 * M_PI / 3.0;
    const vec2 c3 = 0.225 * vec2(cos(t3), sin(t3));
    float r1 = length(P - c1 * size) - size / 4.25;
    float r2 = length(P - c2 * size) - size / 4.25;
    float r3 = length(P - c3 * size) - size / 4.25;
    float r4 = min(min(r1, r2), r3);

    // Root (2 circles and 2 planes)
    const vec2 c4 = vec2(+0.65, 0.125);
    const vec2 c5 = vec2(-0.65, 0.125);
    float r5 = length(P - c4 * size) - size / 1.6;
    float r6 = length(P - c5 * size) - size / 1.6;
    float r7 = P.y - 0.5 * size;
    float r8 = 0.2 * size - P.y;
    float r9 = max(-min(r5, r6), max(r7, r8));

    return min(r4, r9);
}



float marker_cross(vec2 P, float size)
{
    float x = M_SQRT2 / 2.0 * (P.x - P.y);
    float y = M_SQRT2 / 2.0 * (P.x + P.y);
    float r1 = max(abs(x - size / 3.0), abs(x + size / 3.0));
    float r2 = max(abs(y - size / 3.0), abs(y + size / 3.0));
    float r3 = max(abs(x), abs(y));
    float r = max(min(r1, r2), r3);
    r -= size / 2;
    return r;
}



float marker_diamond(vec2 P, float size)
{
    float x = M_SQRT2 / 2.0 * (P.x - P.y);
    float y = M_SQRT2 / 2.0 * (P.x + P.y);
    return max(abs(x), abs(y)) - size / (2.0 * M_SQRT2);
}



float marker_disc(vec2 P, float size) { return length(P) - size / 2; }



float marker_ellipse(vec2 P, float size)
{
    // --- ellipse
    // Created by Inigo Quilez - iq/2013
    // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
    // Alternate version (approximation)
    float a = 1.0;
    float b = 2.0;
    float r = 0.5 * size;
    float f = length(P * vec2(a, b));
    f = length(P * vec2(a, b));
    f = f * (f - r) / length(P * vec2(a * a, b * b));
    return f;
}



float marker_hbar(vec2 P, float size) { return max(abs(P.x) - size / 6.0, abs(P.y) - size / 2.0); }



float marker_heart(vec2 P, float size)
{
    float x = M_SQRT2 / 2.0 * (P.x - P.y);
    float y = M_SQRT2 / 2.0 * (P.x + P.y);
    float r1 = max(abs(x), abs(y)) - size / 3.5;
    float r2 = length(P - M_SQRT2 / 2.0 * vec2(+1.0, -1.0) * size / 3.5) - size / 3.5;
    float r3 = length(P - M_SQRT2 / 2.0 * vec2(-1.0, -1.0) * size / 3.5) - size / 3.5;
    return min(min(r1, r2), r3);
}



float marker_infinity(vec2 P, float size)
{
    const vec2 c1 = vec2(+0.2125, 0.00);
    const vec2 c2 = vec2(-0.2125, 0.00);
    float r1 = length(P - c1 * size) - size / 3.5;
    float r2 = length(P - c1 * size) - size / 7.5;
    float r3 = length(P - c2 * size) - size / 3.5;
    float r4 = length(P - c2 * size) - size / 7.5;
    return min(max(r1, -r2), max(r3, -r4));
}



float marker_pin(vec2 P, float size)
{
    size *= .9;

    vec2 c1 = vec2(0.0, -0.15) * size;
    float r1 = length(P - c1) - size / 2.675;
    vec2 c2 = vec2(+1.49, -0.80) * size;
    float r2 = length(P - c2) - 2. * size;
    vec2 c3 = vec2(-1.49, -0.80) * size;
    float r3 = length(P - c3) - 2. * size;
    float r4 = length(P - c1) - size / 5;
    return max(min(r1, max(max(r2, r3), -P.y)), -r4);
}



float marker_ring(vec2 P, float size)
{
    float r1 = length(P) - size / 2;
    float r2 = length(P) - size / 4;
    return max(r1, -r2);
}



float marker_spade(vec2 P, float size)
{
    // Reversed heart (diamond + 2 circles)
    float s = size * 0.85 / 3.5;
    float x = M_SQRT2 / 2.0 * (P.x + P.y) + 0.4 * s;
    float y = M_SQRT2 / 2.0 * (P.x - P.y) - 0.4 * s;
    float r1 = max(abs(x), abs(y)) - s;
    float r2 = length(P - M_SQRT2 / 2.0 * vec2(+1.0, +0.2) * s) - s;
    float r3 = length(P - M_SQRT2 / 2.0 * vec2(-1.0, +0.2) * s) - s;
    float r4 = min(min(r1, r2), r3);

    // Root (2 circles and 2 planes)
    const vec2 c1 = vec2(+0.65, 0.125);
    const vec2 c2 = vec2(-0.65, 0.125);
    float r5 = length(P - c1 * size) - size / 1.6;
    float r6 = length(P - c2 * size) - size / 1.6;
    float r7 = P.y - 0.5 * size;
    float r8 = 0.1 * size - P.y;
    float r9 = max(-min(r5, r6), max(r7, r8));

    return min(r4, r9);
}



float marker_square(vec2 P, float size) { return max(abs(P.x), abs(P.y)) - size / 2.0; }



float marker_rounded_rect(vec2 P, vec2 size, float radius)
{
    vec2 d = abs(P) - size / 2.0 + radius;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;
}



float marker_rounded_rect(vec2 P, float size, float radius)
{
    return marker_rounded_rect(P, vec2(size, size), radius);
}



float marker_tag(vec2 P, float size)
{
    float r1 = max(abs(P.x) - size / 2.0, abs(P.y) - size / 6.0);
    float r2 = abs(P.x - size / 2.0) + abs(P.y) - size;
    return max(r1, .75 * r2);
}



float marker_triangle(vec2 P, float size)
{
    float x = M_SQRT2 / 2.0 * (P.x - (P.y - size / 6));
    float y = M_SQRT2 / 2.0 * (P.x + (P.y - size / 6));
    float r1 = max(abs(x), abs(y)) - size / (2.0 * M_SQRT2);
    float r2 = P.y - size / 6;
    return max(r1, r2);
}



float marker_vbar(vec2 P, float size) { return max(abs(P.y) - size / 2.0, abs(P.x) - size / 6.0); }
