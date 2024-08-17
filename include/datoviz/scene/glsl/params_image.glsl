layout(constant_id = 0) const int SIZE_NDC = 0;
layout(constant_id = 1) const int RESCALE = 0; // 1 = rescale_keep_ratio, 2 = rescale
layout(constant_id = 2) const int FILL = 0;    // 0=textured, 1=fill color

layout(std140, binding = USER_BINDING) uniform ImageParams
{
    float radius;     // rounded rectangle radius, 0 for sharp corners
    float edge_width; // width of the border, 0 for no border
    vec4 edge_color;  // color of the border
}
params;
