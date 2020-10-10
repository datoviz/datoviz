#include "../include/visky/mesh.h"


/*************************************************************************************************/
/*  Mesh creation                                                                                */
/*************************************************************************************************/

VkyMesh vky_create_mesh(uint32_t total_vertex_count, uint32_t total_index_count)
{
    VkyMesh mesh = {0};
    glm_mat4_identity(mesh.current_transform);

    mesh.object_count = 0;
    mesh.vertex_offsets = calloc(VKY_MAX_MESH_OBJECTS, sizeof(uint32_t));
    mesh.index_offsets = calloc(VKY_MAX_MESH_OBJECTS, sizeof(uint32_t));

    mesh.vertex_size = sizeof(VkyMeshVertex);

    mesh.max_vertex_count = total_vertex_count;
    mesh.vertices = calloc(total_vertex_count, mesh.vertex_size);

    mesh.max_index_count = total_index_count;
    mesh.indices = calloc(total_index_count, sizeof(VkyIndex));

    return mesh;
}

void vky_mesh_set_transform(VkyMesh* mesh, mat4 transform)
{
    glm_mat4_copy(transform, mesh->current_transform);
}

VkyData vky_mesh_data(VkyMesh* mesh)
{
    VkyData data = {0};
    data.vertex_count = mesh->vertex_offsets[mesh->object_count];
    data.index_count = mesh->index_offsets[mesh->object_count];
    data.vertices = mesh->vertices;
    data.indices = mesh->indices;
    data.no_vertices_alloc = true; // vky_mesh_destroy() will take care of the freeing instead
    return data;
}

void vky_mesh_destroy(VkyMesh* mesh)
{
    free(mesh->vertices);
    free(mesh->indices);
    free(mesh->vertex_offsets);
    free(mesh->index_offsets);
}

void vky_mesh_begin(
    VkyMesh* mesh, uint32_t* first_vertex, VkyMeshVertex** vertices, VkyIndex** indices)
{
    ASSERT(mesh->object_count < VKY_MAX_MESH_OBJECTS - 1);

    uint32_t vertex_offset = mesh->vertex_offsets[mesh->object_count];
    uint32_t index_offset = mesh->index_offsets[mesh->object_count];

    *first_vertex = vertex_offset;
    *vertices = &mesh->vertices[vertex_offset];
    *indices = &mesh->indices[index_offset];
}

void vky_mesh_end(VkyMesh* mesh, uint32_t vertex_count, uint32_t index_count)
{
    mesh->vertex_offsets[mesh->object_count + 1] =
        mesh->vertex_offsets[mesh->object_count] + vertex_count;
    mesh->index_offsets[mesh->object_count + 1] =
        mesh->index_offsets[mesh->object_count] + index_count;

    mesh->object_count++;
    ASSERT(mesh->vertex_offsets[mesh->object_count] < mesh->max_vertex_count);
    ASSERT(mesh->index_offsets[mesh->object_count] < mesh->max_index_count);
}

void vky_normalize_mesh(uint32_t vertex_count, VkyMeshVertex* vertices)
{
    const float INF = 1000000;
    vec3 min = {+INF, +INF, +INF}, max = {-INF, -INF, -INF};
    vec3 center = {0};
    vec3 pos = {0};

    for (uint32_t i = 0; i < vertex_count; i++)
    {
        glm_vec3_copy(vertices[i].pos, pos);
        glm_vec3_minv(min, pos, min);
        glm_vec3_maxv(max, pos, max);

        glm_vec3_add(center, pos, center);
    }
    glm_vec3_scale(center, 1. / vertex_count, center);

    // a * (pos - center) \in (-1, 1)
    // a * (min - center)
    // a = min(1/(max-center), 1/(center-xmin))
    vec3 u = {0}, v = {0};
    glm_vec3_sub(max, center, u);
    glm_vec3_sub(center, min, v);
    for (uint32_t k = 0; k < 3; k++)
    {
        u[k] = 1 / u[k];
        v[k] = 1 / v[k];
    }
    float a = fmin(glm_vec3_min(u), glm_vec3_min(v));
    ASSERT(a > 0);

    for (uint32_t i = 0; i < vertex_count; i++)
    {
        glm_vec3_sub(vertices[i].pos, center, vertices[i].pos);
        glm_vec3_scale(vertices[i].pos, a, vertices[i].pos);
    }
}



/*************************************************************************************************/
/*  Mesh transforms                                                                              */
/*************************************************************************************************/

VKY_INLINE void texture_color(usvec2 ij, usvec2 nm, void* color)
{
    // Put two uint16 numbers with the UV tex coordinates in color
    uint16_t i = ij[0];
    uint16_t j = ij[1];

    double n = (double)nm[0];
    double m = (double)nm[1];

    double u = i / n;
    double v = j / m;

    uint16_t x = (uint16_t)round(u * UINT16_MAX);
    uint16_t y = (uint16_t)round(v * UINT16_MAX);

    memcpy(color, (cvec4){x & 0xFF, (x >> 8) & 0xFF, y & 0xFF, (y >> 8) & 0xFF}, sizeof(cvec4));
}

VKY_INLINE void transform_pos(VkyMesh* mesh, vec3 pos)
{
    glm_mat4_mulv3(mesh->current_transform, pos, 1, pos);
}

VKY_INLINE void transform_normal(VkyMesh* mesh, vec3 normal)
{
    mat4 tr;
    glm_mat4_copy(mesh->current_transform, tr);
    glm_mat4_inv(tr, tr);
    glm_mat4_transpose(tr);
    glm_mat4_mulv3(tr, normal, 1, normal);
}

void vky_mesh_transform_reset(VkyMesh* mesh) { glm_mat4_identity(mesh->current_transform); }

void vky_mesh_transform_add(VkyMesh* mesh, mat4 transform)
{
    glm_mat4_mul(transform, mesh->current_transform, mesh->current_transform);
}

void vky_mesh_translate(VkyMesh* mesh, vec3 translate)
{
    mat4 tr;
    glm_translate_make(tr, translate);
    vky_mesh_transform_add(mesh, tr);
}

void vky_mesh_scale(VkyMesh* mesh, vec3 scale)
{
    mat4 tr;
    glm_scale_make(tr, scale);
    vky_mesh_transform_add(mesh, tr);
}

void vky_mesh_rotate(VkyMesh* mesh, float angle, vec3 axis)
{
    mat4 tr;
    glm_rotate_make(tr, angle, axis);
    vky_mesh_transform_add(mesh, tr);
}



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

void vky_mesh_grid(
    VkyMesh* mesh, uint32_t row_count, uint32_t col_count, const vec3* positions,
    const void* color)
{
    const uint32_t nv = col_count * row_count;
    const uint32_t ni =
        6 * (col_count - 1) * (row_count - 1); // 2 triangles = 6 vertices per point

    // Get vertices and indices pointers into the mesh arrays.
    uint32_t first_vertex;
    VkyMeshVertex* vertex = NULL;
    VkyIndex* index = NULL;
    vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    vec3 cur, next_j, next_i, u, v;

    uint32_t point_idx = 0;
    for (uint32_t i = 0; i < row_count; i++)
    {
        for (uint32_t j = 0; j < col_count; j++)
        {
            ASSERT(point_idx == col_count * i + j);

            // Position.
            ASSERT(point_idx < nv);
            vec3_copy(positions[point_idx], vertex->pos);

            // Color/texture coordinates.
            if (color != NULL)
            {
                memcpy(&vertex->color, &((const VkyColor*)color)[point_idx], sizeof(VkyColor));
            }
            else
            {
                // Texture coordinates.
                texture_color((usvec2){i, j}, (usvec2){row_count, col_count}, &vertex->color);
            }

            // Normals.
            vec3_copy(positions[point_idx], cur);
            vec3_copy(positions[col_count * i + (j + 1) % col_count], next_j);
            vec3_copy(positions[col_count * ((i + 1) % row_count) + j], next_i);
            glm_vec3_sub(next_i, cur, u);
            glm_vec3_sub(next_j, cur, v);
            glm_vec3_crossn(u, v, vertex->normal);

            // Normal vector on edges.
            if (i == row_count - 1)
            {
                vec3_copy(
                    mesh->vertices[first_vertex + col_count * (i - 1) + j].normal, vertex->normal);
            }
            else if (j == col_count - 1)
            {
                vec3_copy(
                    mesh->vertices[first_vertex + col_count * i + j - 1].normal, vertex->normal);
            }

            // Vertex topology.
            if ((i < row_count - 1) && (j < col_count - 1))
            {
                memcpy(
                    index,
                    (VkyIndex[]){
                        first_vertex + col_count * (i + 0) + (j + 0),
                        first_vertex + col_count * (i + 1) + (j + 0),
                        first_vertex + col_count * (i + 0) + (j + 1),
                        first_vertex + col_count * (i + 1) + (j + 1),
                        first_vertex + col_count * (i + 0) + (j + 1),
                        first_vertex + col_count * (i + 1) + (j + 0),
                    },
                    6 * sizeof(VkyIndex));
                index += 6;
            }

            // Go to next vertex.
            point_idx++;
            vertex++; // remember this is a pointer to the mesh vertices array.
        }
    }

    // Second pass for the transformation.
    for (uint32_t i = 0; i < nv; i++)
    {
        vertex = &mesh->vertices[first_vertex + i];
        transform_pos(mesh, vertex->pos);
        transform_normal(mesh, vertex->normal);
    }

    vky_mesh_end(mesh, nv, ni);
}

void vky_mesh_grid_surface(
    VkyMesh* mesh, uint32_t row_count, uint32_t col_count, vec3 p00, vec3 p01, vec3 p10,
    const float* heights, const void* color)
{
    ASSERT(row_count > 0);
    ASSERT(col_count > 0);

    vec3* positions = calloc(col_count * row_count, sizeof(vec3));

    vec3 p, q, r;
    glm_vec3_sub(p01, p00, p);
    glm_vec3_sub(p10, p00, q);
    glm_vec3_crossn(p, q, r);

    float h, x, y, z, u, v;
    for (uint32_t i = 0; i < row_count; i++)
    {
        u = (float)i / (row_count - 1);
        for (uint32_t j = 0; j < col_count; j++)
        {
            v = (float)j / (col_count - 1);
            h = heights[col_count * i + j];

            x = p00[0] + p[0] * u + q[0] * v + r[0] * h;
            y = p00[1] + p[1] * u + q[1] * v + r[1] * h;
            z = p00[2] + p[2] * u + q[2] * v + r[2] * h;

            positions[col_count * i + j][0] = x;
            positions[col_count * i + j][1] = y;
            positions[col_count * i + j][2] = z;
        }
    }

    vky_mesh_grid(mesh, row_count, col_count, (const vec3*)positions, color);
}

void vky_mesh_cube(VkyMesh* mesh, const void* color)
{
    const uint32_t nv = 36;

    // Get vertices and indices pointers into the mesh arrays.
    uint32_t first_vertex;
    VkyMeshVertex* vertex = NULL;
    VkyIndex* index = NULL;
    vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    float x = .5;

    VkyMeshVertex vertices[] = {

        {{-x, -x, +x}, {0, 0, +1}}, // back
        {{+x, -x, +x}, {0, 0, +1}}, //
        {{+x, +x, +x}, {0, 0, +1}}, //
        {{+x, +x, +x}, {0, 0, +1}}, //
        {{-x, +x, +x}, {0, 0, +1}}, //
        {{-x, -x, +x}, {0, 0, +1}}, //

        {{+x, -x, +x}, {+1, 0, 0}}, // right
        {{+x, -x, -x}, {+1, 0, 0}}, //
        {{+x, +x, -x}, {+1, 0, 0}}, //
        {{+x, +x, -x}, {+1, 0, 0}}, //
        {{+x, +x, +x}, {+1, 0, 0}}, //
        {{+x, -x, +x}, {+1, 0, 0}}, //

        {{-x, +x, -x}, {0, 0, -1}}, // front
        {{+x, +x, -x}, {0, 0, -1}}, //
        {{+x, -x, -x}, {0, 0, -1}}, //
        {{+x, -x, -x}, {0, 0, -1}}, //
        {{-x, -x, -x}, {0, 0, -1}}, //
        {{-x, +x, -x}, {0, 0, -1}}, //

        {{-x, -x, -x}, {-1, 0, 0}}, // left
        {{-x, -x, +x}, {-1, 0, 0}}, //
        {{-x, +x, +x}, {-1, 0, 0}}, //
        {{-x, +x, +x}, {-1, 0, 0}}, //
        {{-x, +x, -x}, {-1, 0, 0}}, //
        {{-x, -x, -x}, {-1, 0, 0}}, //

        {{-x, -x, -x}, {0, -1, 0}}, // bottom
        {{+x, -x, -x}, {0, -1, 0}}, //
        {{+x, -x, +x}, {0, -1, 0}}, //
        {{+x, -x, +x}, {0, -1, 0}}, //
        {{-x, -x, +x}, {0, -1, 0}}, //
        {{-x, -x, -x}, {0, -1, 0}}, //

        {{-x, +x, +x}, {0, +1, 0}}, // top
        {{+x, +x, +x}, {0, +1, 0}}, //
        {{+x, +x, -x}, {0, +1, 0}}, //
        {{+x, +x, -x}, {0, +1, 0}}, //
        {{-x, +x, -x}, {0, +1, 0}}, //
        {{-x, +x, +x}, {0, +1, 0}}, //

    };

    // Transform, colors, indices.
    const VkyColor* p_color = (const VkyColor*)color;
    for (uint32_t i = 0; i < nv; i++)
    {
        transform_pos(mesh, vertices[i].pos);
        transform_normal(mesh, vertices[i].normal);
        if (color != NULL)
        {
            memcpy(&vertices[i].color, &p_color[i], sizeof(VkyColor));
        }
        *index = first_vertex + i;
        index++;
    }

    // Copy the vertices to the mesh vertices.
    memcpy(vertex, vertices, sizeof(vertices));
    vky_mesh_end(mesh, nv, nv);
}

void vky_mesh_sphere(VkyMesh* mesh, uint32_t row_count, uint32_t col_count, const void* color)
{
    float dphi, dtheta;
    dphi = M_2PI / (col_count - 1);
    dtheta = M_PI / (row_count - 1);
    float r, phi, theta, x, y, z;
    r = .5;
    uint32_t point_count = row_count * col_count;
    vec3* positions = calloc(point_count, sizeof(vec3));
    for (uint32_t i = 0; i < row_count; i++)
    {
        theta = dtheta * i;
        for (uint32_t j = 0; j < col_count; j++)
        {
            phi = M_2PI - dphi * j;
            x = r * sin(theta) * cos(phi);
            y = r * cos(theta);
            z = r * sin(theta) * sin(phi);
            vec3_copy((vec3){x, y, z}, positions[col_count * i + j]);
        }
    }
    vky_mesh_grid(mesh, row_count, col_count, (const vec3*)positions, color);
}

void vky_mesh_cylinder(VkyMesh* mesh, uint32_t count, const void* color)
{
    float dphi;
    dphi = M_2PI / (count - 1);
    float r, phi, x, z;
    r = .5;
    uint32_t k = 0;
    uint32_t point_count = 2 * count;
    vec3* positions = calloc(point_count, sizeof(vec3));
    for (uint32_t i = 0; i < 2; i++)
    {
        for (uint32_t j = 0; j < count; j++)
        {
            phi = dphi * j;
            x = r * cos(phi);
            z = r * sin(phi);
            ASSERT(k < point_count);
            vec3_copy((vec3){x, -.5 + i, z}, positions[k]);
            k++;
        }
    }
    vky_mesh_grid(mesh, 2, count, (const vec3*)positions, color);
}

void vky_mesh_cone(VkyMesh* mesh, uint32_t count, const void* color)
{
    float dphi;
    dphi = M_2PI / (count - 1);
    float r0, r, phi, x, z;
    r0 = .5;
    uint32_t k = 0;
    uint32_t point_count = 2 * count;
    vec3* positions = calloc(point_count, sizeof(vec3));
    for (uint32_t i = 0; i < 2; i++)
    {
        for (uint32_t j = 0; j < count; j++)
        {
            phi = dphi * j;
            r = r0 * (1 - i);
            x = r * 1 * cos(phi);
            z = r * 1 * sin(phi);
            ASSERT(k < point_count);
            vec3_copy((vec3){x, -.5 + i, z}, positions[k]);
            k++;
        }
    }
    vky_mesh_grid(mesh, 2, count, (const vec3*)positions, color);
}

void vky_mesh_square(VkyMesh* mesh, const void* color)
{
    const uint32_t nv = 6;

    // Get vertices and indices pointers into the mesh arrays.
    uint32_t first_vertex;
    VkyMeshVertex* vertex = NULL;
    VkyIndex* index = NULL;
    vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    float x = .5;

    VkyMeshVertex vertices[] = {
        {{-x, -x, 0}, {0, 0, +1}}, {{+x, -x, 0}, {0, 0, +1}}, {{+x, x, 0}, {0, 0, +1}},
        {{+x, x, 0}, {0, 0, +1}},  {{-x, x, 0}, {0, 0, +1}},  {{-x, -x, 0}, {0, 0, +1}},
    };

    // Transform, colors, indices.
    const VkyColor* p_color = (const VkyColor*)color;
    for (uint32_t i = 0; i < nv; i++)
    {
        // Transform.
        transform_pos(mesh, vertices[i].pos);
        transform_normal(mesh, vertices[i].normal);
        if (color != NULL)
        {
            memcpy(&vertices[i].color, &p_color[i], sizeof(VkyColor));
        }
        *index = (first_vertex + i);
        index++;
    }

    // Copy the vertices to the mesh vertices.
    memcpy(vertex, vertices, sizeof(vertices));
    vky_mesh_end(mesh, nv, nv);
}

void vky_mesh_disc(VkyMesh* mesh, uint32_t count, const void* color)
{
    uint32_t nv = count + 1;
    uint32_t ni = 3 * count;

    // Get vertices and indices pointers into the mesh arrays.
    uint32_t first_vertex;
    VkyMeshVertex* vertex = NULL;
    VkyIndex* index = NULL;
    vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    // Variables.
    float r = .5;
    float dphi = M_2PI / count;
    float phi = 0;
    float x = 0, y = 0;

    // Center point.
    vec3 normal = {0, 0, -1};
    vec3_copy((vec3){0, 0, 0}, vertex->pos);
    vec3_copy(normal, vertex->normal);
    transform_pos(mesh, vertex->pos);
    transform_normal(mesh, vertex->normal);
    if (color != NULL)
    {
        memcpy(&vertex->color, color, sizeof(VkyColor));
    }

    // Transform, colors, indices.
    const VkyColor* p_color = (const VkyColor*)color;
    for (uint32_t i = 0; i < count; i++)
    {
        vertex++; // Go to next vertex in the mesh vertices array.
        phi = M_2PI - dphi * i;
        x = r * cos(phi);
        y = r * sin(phi);

        // Position.
        vertex->pos[0] = x;
        vertex->pos[1] = y;
        vertex->pos[2] = 0;

        // Normal.
        vec3_copy(normal, vertex->normal);

        // Transform.
        transform_pos(mesh, vertex->pos);
        transform_normal(mesh, vertex->normal);

        // Color.
        if (p_color != NULL)
        {
            memcpy(&vertex->color, &p_color[i], sizeof(VkyColor));
        }

        // Indices.
        memcpy(
            index,
            (VkyIndex[]){
                first_vertex, first_vertex + i + 1, first_vertex + ((i + 1) % (count)) + 1},
            3 * sizeof(VkyIndex));
        index += 3;
    }
    vky_mesh_end(mesh, nv, ni);
}
