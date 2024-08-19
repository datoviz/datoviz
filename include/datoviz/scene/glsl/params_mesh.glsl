layout(constant_id = 0) const int MESH_TEXTURED = 0; // 1 to enable
layout(constant_id = 1) const int MESH_LIGHTING = 0; // 1 to enable

layout(std140, binding = USER_BINDING) uniform MeshParams
{
    vec4 light_pos;
    vec4 light_params; // ambient, diffuse, specular, specular expon
    vec4 stroke;       // r, g, b, stroke-width
}
params;
