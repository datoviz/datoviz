// -----------------------------------------------------------------------------
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
// -----------------------------------------------------------------------------

#include "antialias.glsl"


/* ---------------------------------------------------------
   Hyperbolic cosine
   --------------------------------------------------------- */
// float cosh(float x)
// {
//     return 0.5 * (exp(x)+exp(-x));
// }

/* ---------------------------------------------------------
   Hyperbolic sine
   --------------------------------------------------------- */
// float sinh(float x)
// {
//     return 0.5 * (exp(x)-exp(-x));
// }

/* ---------------------------------------------------------
   Compute distance from a point to a line (2d)

   Parameters:
   -----------

   p0, p1: Points describing the line
   p: Point to computed distance to

   Return:
   -------
   Distance of p to (p0,p1)

   --------------------------------------------------------- */
float point_to_line_distance(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    vec2 v = p1 - p0;
    float l2 = v.x*v.x + v.y*v.y;
    float u = ((p.x-p0.x)*v.x + (p.y-p0.y)*v.y) / l2;

    // h is the projection of p on (p0,p1)
    vec2 h = p0 + u*v;

    return length(p-h);
}

/* ---------------------------------------------------------

   Project a point p onto a line (p0,p1) and return linear position u such that
   p' = p0 + u*(p1-p0)

   Parameters:
   -----------

   p0, p1: Points describing the line
   p: Point to be projected

   Return:
   -------
   Linear position of p onto (p0,p1)

   --------------------------------------------------------- */
float point_to_line_projection(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    // Then  u *= lenght(p1-p0)
    vec2 v = p1 - p0;
    float l = length(v);
    return ((p.x-p0.x)*v.x + (p.y-p0.y)*v.y) / l;
}

/* ---------------------------------------------------------

   Computes the signed distance from a line

   Parameters:
   -----------

   p0, p1: Points describing the line
   p: Point to measure distance from

   Return:
   -------
   Signed distance

   --------------------------------------------------------- */
float line_distance(vec2 p, vec2 p1, vec2 p2) {
    vec2 center = (p1 + p2) * 0.5;
    float len = length(p2 - p1);
    vec2 dir = (p2 - p1) / len;
    vec2 rel_p = p - center;
    return dot(rel_p, vec2(dir.y, -dir.x));
}

/* ---------------------------------------------------------

   Computes the signed distance from a line segment

   Parameters:
   -----------

   p0, p1: Points describing the line segment
   p: Point to measure distance from

   Return:
   -------
   Signed distance

   --------------------------------------------------------- */

float segment_distance(vec2 p, vec2 p1, vec2 p2) {
    vec2 center = (p1 + p2) * 0.5;
    float len = length(p2 - p1);
    vec2 dir = (p2 - p1) / len;
    vec2 rel_p = p - center;
    float dist1 = abs(dot(rel_p, vec2(dir.y, -dir.x)));
    float dist2 = abs(dot(rel_p, dir)) - 0.5*len;
    return max(dist1, dist2);
}

/* ---------------------------------------------------------

   Computes the center of the 2 circles with given radius passing through
   p1 & p2

   Parameters:
   -----------

   p0, p1: Points ascribed in the circles
   radius: Radius of the circle

   Return:
   -------
   Centers of the two circles with specified radius

   --------------------------------------------------------- */

vec4 circle_from_2_points(vec2 p1, vec2 p2, float radius)
{
    float q = length(p2-p1);
    vec2 m = (p1+p2)/2.0;
    vec2 d = vec2( sqrt(radius*radius - (q*q/4.0)) * (p1.y-p2.y)/q,
                   sqrt(radius*radius - (q*q/4.0)) * (p2.x-p1.x)/q);
    return  vec4(m+d, m-d);
}

/* ---------------------------------------------------------

   Computes the signed distance to a triangle arrow

   Parameters:
   -----------

   texcoord : Point to compute distance to
   body :     Total length of the arrow (pixels, body+head)
   head :     Length of the head (pixels)
   height :   Height of the head (pixel)
   linewidth: Stroke line width (in pixels)
   antialias: Stroke antialiased area (in pixels)

   Return:
   -------
   Signed distance to the arrow

   --------------------------------------------------------- */

float arrow_triangle(vec2 texcoord,
                     float body, float head, float height,
                     float linewidth)
{
    float w = linewidth/2.0 + antialias;
    vec2 start = -vec2(body/2.0, 0.0);
    vec2 end   = +vec2(body/2.0, 0.0);

    // Head : 3 lines
    float d1 = line_distance(texcoord, end, end - head*vec2(+1.0,-height));
    float d2 = line_distance(texcoord, end - head*vec2(+1.0,+height), end);
    float d3 = texcoord.x - end.x + head;

    // Body : 1 segment
    float d4 = segment_distance(texcoord, start, end - vec2(linewidth,0.0));

    float d = min(max(max(d1, d2), -d3), d4);
    return d;
}

/* ---------------------------------------------------------

   Computes the signed distance to an angle arrow

   Parameters:
   -----------

   texcoord : Point to compute distance to
   body :     Total length of the arrow (pixels, body+head)
   head :     Length of the head (pixels)
   height :   Height of the head (pixel)
   linewidth: Stroke line width (in pixels)
   antialias: Stroke antialiased area (in pixels)

   Return:
   -------
   Signed distance to the arrow

   --------------------------------------------------------- */
float arrow_angle(vec2 texcoord,
                  float body, float head, float height,
                  float linewidth)
{
    float d;
    float w = linewidth/2.0 + antialias;
    vec2 start = -vec2(body/2.0, 0.0);
    vec2 end   = +vec2(body/2.0, 0.0);

    // Arrow tip (beyond segment end)
    if( texcoord.x > body/2.0) {
        // Head : 2 segments
        float d1 = line_distance(texcoord, end, end - head*vec2(+1.0,-height));
        float d2 = line_distance(texcoord, end - head*vec2(+1.0,+height), end);
        // Body : 1 segment
        float d3 = end.x - texcoord.x;
        d = max(max(d1,d2), d3);
    } else {
        // Head : 2 segments
        float d1 = segment_distance(texcoord, end - head*vec2(+1.0,-height), end);
        float d2 = segment_distance(texcoord, end - head*vec2(+1.0,+height), end);
        // Body : 1 segment
        float d3 = segment_distance(texcoord, start, end - vec2(linewidth,0.0));
        d = min(min(d1,d2), d3);
    }
    return d;
}

// Computes the centers of a circle with
// given radius passing through p1 & p2
vec4 inscribed_circle(vec2 p1, vec2 p2, float radius)
{
    float q = length(p2-p1);
    vec2 m = (p1+p2)/2.0;
    vec2 d = vec2( sqrt(radius*radius - (q*q/4.0)) * (p1.y-p2.y)/q,
    sqrt(radius*radius - (q*q/4.0)) * (p2.x-p1.x)/q);
    return vec4(m+d, m-d);
}

// Computes the signed distance from a line
// float line_distance(vec2 p, vec2 p1, vec2 p2) {
//     vec2 center = (p1 + p2) * 0.5;
//     float len = length(p2 - p1);
//     vec2 dir = (p2 - p1) / len;
//     vec2 rel_p = p - center;
//     return dot(rel_p, vec2(dir.y, -dir.x));
// }

// float segment_distance(vec2 p, vec2 p1, vec2 p2) {
//     vec2 center = (p1 + p2) * 0.5;
//     float len = length(p2 - p1);
//     vec2 dir = (p2 - p1) / len;
//     vec2 rel_p = p - center;
//     float dist1 = abs(dot(rel_p, vec2(dir.y, -dir.x)));
//     float dist2 = abs(dot(rel_p, dir)) - 0.5*len;
//     return max(dist1, dist2);
// }

/* ---------------------------------------------------------
// Arrows
*/

float arrow_angle_30(vec2 texcoord,
                     float body, float head,
                     float linewidth)
{
    return arrow_angle(texcoord, body, head, 0.25, linewidth);
}

float arrow_angle_60(vec2 texcoord,
                     float body, float head,
                     float linewidth)
{
    return arrow_angle(texcoord, body, head, 0.5, linewidth);
}

float arrow_angle_90(vec2 texcoord,
                     float body, float head,
                     float linewidth)
{
    return arrow_angle(texcoord, body, head, 1.0, linewidth);
}

float arrow_triangle_30(vec2 texcoord,
                        float body, float head,
                        float linewidth)
{
    return arrow_triangle(texcoord, body, head, 0.25, linewidth);
}

float arrow_triangle_60(vec2 texcoord,
                        float body, float head,
                        float linewidth)
{
    return arrow_triangle(texcoord, body, head, 0.5, linewidth);
}

float arrow_triangle_90(vec2 texcoord,
                        float body, float head,
                        float linewidth)
{
    return arrow_triangle(texcoord, body, head, 1.0, linewidth);
}

float arrow_curved(vec2 texcoord,
                   float body, float head,
                   float linewidth)
{
    float w = linewidth/2.0 + antialias;
    vec2 start = -vec2(body/2.0, 0.0);
    vec2 end   = +vec2(body/2.0, 0.0);
    float height = 0.5;

    vec2 p1 = end - head*vec2(+1.0,+height);
    vec2 p2 = end - head*vec2(+1.0,-height);
    vec2 p3 = end;

    // Head : 3 circles
    vec2 c1  = circle_from_2_points(p1, p3, 1.25*body).zw;
    float d1 = length(texcoord - c1) - 1.25*body;
    vec2 c2  = circle_from_2_points(p2, p3, 1.25*body).xy;
    float d2 = length(texcoord - c2) - 1.25*body;
    vec2 c3  = circle_from_2_points(p1, p2, max(body-head, 1.0*body)).xy;
    float d3 = length(texcoord - c3) - max(body-head, 1.0*body);

    // Body : 1 segment
    float d4 = segment_distance(texcoord, start, end - vec2(linewidth,0.0));

    // Outside (because of circles)
    if( texcoord.y > +(2.0*head + antialias) )
         return 1000.0;
    if( texcoord.y < -(2.0*head + antialias) )
         return 1000.0;
    if( texcoord.x < -(body/2.0 + antialias) )
         return 1000.0;
    if( texcoord.x > c1.x ) //(body + antialias) )
         return 1000.0;

    return min( d4, -min(d3,min(d1,d2)));
}

float arrow_stealth(vec2 texcoord,
                    float body, float head,
                    float linewidth)
{
    float w = linewidth/2.0 + antialias;
    vec2 start = -vec2(body/2.0, 0.0);
    vec2 end   = +vec2(body/2.0, 0.0);
    float height = 0.5;

    // Head : 4 lines
    float d1 = line_distance(texcoord, end-head*vec2(+1.0,-height),
                                       end);
    float d2 = line_distance(texcoord, end-head*vec2(+1.0,-height),
                                       end-vec2(3.0*head/4.0,0.0));
    float d3 = line_distance(texcoord, end-head*vec2(+1.0,+height), end);
    float d4 = line_distance(texcoord, end-head*vec2(+1.0,+0.5),
                                       end-vec2(3.0*head/4.0,0.0));

    // Body : 1 segment
    float d5 = segment_distance(texcoord, start, end - vec2(linewidth,0.0));

    return min(d5, max( max(-d1, d3), - max(-d2,d4)));
}

float select_arrow(vec2 texcoord, float body, float head, float linewidth, float arrow_type) {
    // NOTE: the numbers need to correspond to VkyArrowType enum in visuals.h
    if (arrow_type < 0.5) return arrow_curved(texcoord, body, head, linewidth);
    else if (arrow_type < 1.5) return arrow_stealth(texcoord, body, head, linewidth);
    else if (arrow_type < 2.5) return arrow_angle_30(texcoord, body, head, linewidth);
    else if (arrow_type < 3.5) return arrow_angle_60(texcoord, body, head, linewidth);
    else if (arrow_type < 4.5) return arrow_angle_90(texcoord, body, head, linewidth);
    else if (arrow_type < 5.5) return arrow_triangle_30(texcoord, body, head, linewidth);
    else if (arrow_type < 6.5) return arrow_triangle_60(texcoord, body, head, linewidth);
    else if (arrow_type < 7.5) return arrow_triangle_90(texcoord, body, head, linewidth);
    return 0;
}
