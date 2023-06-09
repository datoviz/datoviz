layout(std140, binding = USER_BINDING) uniform MeshParams
{
    mat4 lights_pos_0; // lights 0-3

    mat4 lights_params_0; // for each light, coefs for ambient, diffuse, specular, specular expon

    vec4 tex_coefs; // blending coefficients for the textures

    vec4 clip_coefs;
}
params;
