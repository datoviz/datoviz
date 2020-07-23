
const vec3 light_color = vec3(1.0);

vec4 lighting(vec4 color, vec3 pos, vec3 normal, vec3 view_pos, vec3 light_pos, vec3 light_coeffs) {
    // ambient
    vec3 ambient = color.rgb;

    // diffuse
    vec3 light_dir = normalize(light_pos - pos);
    normal = normalize(normal);
    if (!gl_FrontFacing) normal = -normal;
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * color.rgb;

    // specular
    vec3 view_dir = normalize(view_pos - pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

    vec3 specular = spec * light_color;

    // total
    return vec4(light_coeffs.x * ambient + light_coeffs.y * diffuse + light_coeffs.z * specular, color.a);
}
