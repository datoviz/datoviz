// layout(constant_id = 0) const int MESH_TEXTURED = 0;

layout(std140, binding = USER_BINDING) uniform ImageParams
{
    float radius;     // rounded rectangle radius, 0 for sharp corners
    float edge_width; // width of the border, 0 for no border
    vec4 edge_color;  // color of the border
}
params;
