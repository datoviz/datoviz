const int DVZ_STROKE_CAP_NONE = 0;
const int DVZ_STROKE_CAP_TRIANGLE_OUT = 3;
const int DVZ_STROKE_CAP_BUTT = 5;


float dvz_stroke_alpha(float distance, float lineWidth)
{
    float aa = 1.0;
    float halfWidth = max(lineWidth, 0.0) * 0.5;
    return 1.0 - smoothstep(halfWidth - aa, halfWidth + aa, abs(distance));
}


float dvz_stroke_outer_half_width(float lineWidth)
{
    float aa = 1.0;
    return max(lineWidth, 0.0) * 0.5 + 1.5 * aa;
}


float dvz_stroke_triangle_head_length(float lineWidth)
{
    return max(max(lineWidth, 0.0) * 3.0, dvz_stroke_outer_half_width(lineWidth));
}


float dvz_stroke_triangle_head_half_width(float lineWidth)
{
    return max(max(lineWidth, 0.0) * 1.5, dvz_stroke_outer_half_width(lineWidth));
}


float dvz_stroke_cap_extension(int capType, float lineWidth)
{
    if (capType == DVZ_STROKE_CAP_NONE || capType == DVZ_STROKE_CAP_BUTT)
        return 0.0;
    if (capType == DVZ_STROKE_CAP_TRIANGLE_OUT)
        return dvz_stroke_triangle_head_length(lineWidth) + 1.0;
    return dvz_stroke_outer_half_width(lineWidth);
}


float dvz_stroke_cap_half_width(int capType, float lineWidth)
{
    if (capType == DVZ_STROKE_CAP_TRIANGLE_OUT)
        return dvz_stroke_triangle_head_half_width(lineWidth) + 1.0;
    return dvz_stroke_outer_half_width(lineWidth);
}


float dvz_stroke_cap_distance(int capType, float dx, float dy, float lineWidth)
{
    float aa = 1.0;
    float halfWidth = max(lineWidth, 0.0) * 0.5;
    float t = max(halfWidth - aa, 0.0);
    float x = abs(dx);
    float y = abs(dy);

    if (capType == 0)
        return 1e6;
    if (capType == 1)
        return length(vec2(x, y));
    if (capType == 2)
        return max(y, t + x - y);
    if (capType == 4)
        return max(x, y);
    if (capType == 5)
        return max(x + t, y);
    return max(x + t, y);
}


float dvz_stroke_triangle_out_alpha(float dx, float dy, float lineWidth)
{
    float aa = 1.0;
    float x = max(dx, 0.0);
    float y = abs(dy);
    float lengthPx = dvz_stroke_triangle_head_length(lineWidth);
    float halfWidth = dvz_stroke_triangle_head_half_width(lineWidth);
    float scale = min(lengthPx, halfWidth);
    float edgeDistance = (x / max(lengthPx, 1e-6) + y / max(halfWidth, 1e-6) - 1.0) * scale;
    float tipDistance = x - lengthPx;
    float distance = max(edgeDistance, tipDistance);
    return 1.0 - smoothstep(-aa, aa, distance);
}


float dvz_stroke_cap_alpha(int capType, float dx, float dy, float lineWidth)
{
    if (capType == DVZ_STROKE_CAP_TRIANGLE_OUT)
        return dvz_stroke_triangle_out_alpha(dx, dy, lineWidth);
    return dvz_stroke_alpha(dvz_stroke_cap_distance(capType, dx, dy, lineWidth), lineWidth);
}
