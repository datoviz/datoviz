layout(std140, binding = USER_BINDING) uniform MeshParams
{
    vec4 light_pos;
    vec4 light_params; // ambient, diffuse, specular, specular expon
}
params;
