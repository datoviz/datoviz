
vec3 triangle_coords() {
    // Triangle barycentric coordinates available in the fragment shader.
    int id = gl_VertexIndex % 3;
    vec3 triangle_coords = vec3(0);
    if (id == 0) triangle_coords.x = 1;
    else if (id == 1) triangle_coords.y = 1;
    else if (id == 2) triangle_coords.z = 1;
    return triangle_coords;
}
