#include "../include/datoviz/mesh.h"
#include "../include/datoviz/common.h"



/*************************************************************************************************/
/*  Mesh transforms                                                                              */
/*************************************************************************************************/

DVZ_INLINE void texture_color(usvec2 ij, usvec2 nm, void* color)
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

DVZ_INLINE void transform_pos(DvzMesh* mesh, vec3 pos)
{
    glm_mat4_mulv3(mesh->transform, pos, 1, pos);
}

DVZ_INLINE void transform_normal(DvzMesh* mesh, vec3 normal)
{
    mat4 tr;
    glm_mat4_copy(mesh->transform, tr);
    glm_mat4_inv(tr, tr);
    glm_mat4_transpose(tr);
    glm_mat4_mulv3(tr, normal, 1, normal);
}

void dvz_mesh_transform_reset(DvzMesh* mesh) { glm_mat4_identity(mesh->transform); }

void dvz_mesh_transform_add(DvzMesh* mesh, mat4 transform)
{
    glm_mat4_mul(transform, mesh->transform, mesh->transform);
}

void dvz_mesh_translate(DvzMesh* mesh, vec3 translate)
{
    mat4 tr;
    glm_translate_make(tr, translate);
    dvz_mesh_transform_add(mesh, tr);
}

void dvz_mesh_scale(DvzMesh* mesh, vec3 scale)
{
    mat4 tr;
    glm_scale_make(tr, scale);
    dvz_mesh_transform_add(mesh, tr);
}

void dvz_mesh_rotate(DvzMesh* mesh, float angle, vec3 axis)
{
    mat4 tr;
    glm_rotate_make(tr, angle, axis);
    dvz_mesh_transform_add(mesh, tr);
}

void dvz_mesh_transform(DvzMesh* mesh)
{
    ASSERT(mesh != NULL);
    DvzGraphicsMeshVertex* vertex = NULL;
    for (uint32_t i = 0; i < mesh->vertices.item_count; i++)
    {
        vertex = dvz_array_item(&mesh->vertices, i);
        transform_pos(mesh, vertex->pos);
        transform_normal(mesh, vertex->normal);
    }
}

void dvz_mesh_normalize(DvzMesh* mesh)
{
    const float INF = 1000000;
    vec3 min = {+INF, +INF, +INF}, max = {-INF, -INF, -INF};
    vec3 center = {0};
    vec3 pos = {0};

    uint32_t nv = mesh->vertices.item_count;
    DvzGraphicsMeshVertex* vertex = NULL;
    for (uint32_t i = 0; i < nv; i++)
    {
        vertex = dvz_array_item(&mesh->vertices, i);
        ASSERT(vertex != NULL);
        _vec3_copy(vertex->pos, pos);
        glm_vec3_minv(min, pos, min);
        glm_vec3_maxv(max, pos, max);

        glm_vec3_add(center, pos, center);
    }
    glm_vec3_scale(center, 1. / nv, center);

    // a * (pos - center) \in (-1, 1)
    // a * (min - center)
    // a = min(1/(max-center), 1/(center-xmin))
    vec3 u = {0}, v = {0};
    glm_vec3_sub(max, center, u);
    glm_vec3_sub(center, min, v);
    glm_vec3_div((vec3){1, 1, 1}, u, u);
    glm_vec3_div((vec3){1, 1, 1}, v, v);
    // for (uint32_t k = 0; k < 3; k++)
    // {
    //     u[k] = 1 / u[k];
    //     v[k] = 1 / v[k];
    // }
    float a = fmin(glm_vec3_min(u), glm_vec3_min(v));
    ASSERT(a > 0);

    for (uint32_t i = 0; i < nv; i++)
    {
        vertex = dvz_array_item(&mesh->vertices, i);
        ASSERT(vertex != NULL);
        glm_vec3_sub(vertex->pos, center, vertex->pos);
        glm_vec3_scale(vertex->pos, a, vertex->pos);
    }
}



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

DvzMesh dvz_mesh()
{
    DvzMesh mesh = {0};
    mesh.vertices = dvz_array_struct(0, sizeof(DvzGraphicsMeshVertex));
    mesh.indices = dvz_array_struct(0, sizeof(DvzIndex));
    dvz_mesh_transform_reset(&mesh);
    return mesh;
}

DvzMesh dvz_mesh_grid(uint32_t row_count, uint32_t col_count, const vec3* positions)
{
    DvzMesh mesh = dvz_mesh();
    const uint32_t nv = col_count * row_count;
    // 2 triangles = 6 vertices per point:
    const uint32_t ni = 6 * (col_count - 1) * (row_count - 1);

    dvz_array_resize(&mesh.vertices, nv);
    dvz_array_resize(&mesh.indices, ni);

    // Get vertices and indices pointers into the mesh arrays.
    vec3 cur, next_j, next_i, u, v;
    DvzGraphicsMeshVertex* vertices = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    DvzGraphicsMeshVertex* vertex = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    DvzIndex* index = (DvzIndex*)mesh.indices.data;
    vec2 uv = {0};
    uint32_t point_idx = 0;
    uint32_t first_vertex = 0;
    for (uint32_t i = 0; i < row_count; i++)
    {
        for (uint32_t j = 0; j < col_count; j++)
        {
            ASSERT(point_idx == col_count * i + j);

            // Position.
            ASSERT(point_idx < nv);
            _vec3_copy(positions[point_idx], vertex->pos);
            uv[1] = i / (float)(row_count - 1);
            uv[0] = j / (float)(col_count - 1);

            _vec3_copy(uv, vertex->uv);
            vertex->alpha = 255;

            // Normals.
            _vec3_copy(positions[point_idx], cur);
            _vec3_copy(positions[col_count * i + (j + 1) % col_count], next_j);
            _vec3_copy(positions[col_count * ((i + 1) % row_count) + j], next_i);
            glm_vec3_sub(next_i, cur, u);
            glm_vec3_sub(next_j, cur, v);
            glm_vec3_crossn(u, v, vertex->normal);

            // Normal vector on edges.
            if (i == row_count - 1)
            {
                _vec3_copy(
                    vertices[first_vertex + col_count * (i - 1) + j].normal, vertex->normal);
            }
            else if (j == col_count - 1)
            {
                _vec3_copy(vertices[first_vertex + col_count * i + j - 1].normal, vertex->normal);
            }

            // Vertex topology.
            if ((i < row_count - 1) && (j < col_count - 1))
            {
                memcpy(
                    index,
                    (DvzIndex[]){
                        first_vertex + col_count * (i + 0) + (j + 0),
                        first_vertex + col_count * (i + 1) + (j + 0),
                        first_vertex + col_count * (i + 0) + (j + 1),
                        first_vertex + col_count * (i + 1) + (j + 1),
                        first_vertex + col_count * (i + 0) + (j + 1),
                        first_vertex + col_count * (i + 1) + (j + 0),
                    },
                    6 * sizeof(DvzIndex));
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
        vertex = &vertices[first_vertex + i];
        transform_pos(&mesh, vertex->pos);
        transform_normal(&mesh, vertex->normal);
    }

    return mesh;
}

DvzMesh dvz_mesh_surface(uint32_t row_count, uint32_t col_count, const float* heights)
{
    ASSERT(row_count > 0);
    ASSERT(col_count > 0);

    vec3* positions = calloc(col_count * row_count, sizeof(vec3));
    vec3 p00 = {-1, 0, -1}, p10 = {+1, 0, -1}, p01 = {-1, 0, +1};
    vec3 p = {0}, q = {0}, r = {0};

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

    DvzMesh mesh = dvz_mesh_grid(row_count, col_count, (const vec3*)positions);
    FREE(positions);
    return mesh;
}

DvzMesh dvz_mesh_cube()
{
    DvzMesh mesh = dvz_mesh();
    const uint32_t nv = 36;
    dvz_array_resize(&mesh.vertices, nv);
    DvzGraphicsMeshVertex* vertex = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    ASSERT(vertex != NULL);

    float x = .5;
    DvzGraphicsMeshVertex vertices[] = {
        {{-x, -x, +x}, {0, 0, +1}, {0, 1}, 255}, // front
        {{+x, -x, +x}, {0, 0, +1}, {1, 1}, 255}, //
        {{+x, +x, +x}, {0, 0, +1}, {1, 0}, 255}, //
        {{+x, +x, +x}, {0, 0, +1}, {1, 0}, 255}, //
        {{-x, +x, +x}, {0, 0, +1}, {0, 0}, 255}, //
        {{-x, -x, +x}, {0, 0, +1}, {0, 1}, 255}, //

        {{+x, -x, +x}, {+1, 0, 0}, {0, 1}, 255}, // right
        {{+x, -x, -x}, {+1, 0, 0}, {1, 1}, 255}, //
        {{+x, +x, -x}, {+1, 0, 0}, {1, 0}, 255}, //
        {{+x, +x, -x}, {+1, 0, 0}, {1, 0}, 255}, //
        {{+x, +x, +x}, {+1, 0, 0}, {0, 0}, 255}, //
        {{+x, -x, +x}, {+1, 0, 0}, {0, 1}, 255}, //

        {{-x, +x, -x}, {0, 0, -1}, {1, 0}, 255}, // back
        {{+x, +x, -x}, {0, 0, -1}, {0, 0}, 255}, //
        {{+x, -x, -x}, {0, 0, -1}, {0, 1}, 255}, //
        {{+x, -x, -x}, {0, 0, -1}, {0, 1}, 255}, //
        {{-x, -x, -x}, {0, 0, -1}, {1, 1}, 255}, //
        {{-x, +x, -x}, {0, 0, -1}, {1, 0}, 255}, //

        {{-x, -x, -x}, {-1, 0, 0}, {0, 1}, 255}, // left
        {{-x, -x, +x}, {-1, 0, 0}, {1, 1}, 255}, //
        {{-x, +x, +x}, {-1, 0, 0}, {1, 0}, 255}, //
        {{-x, +x, +x}, {-1, 0, 0}, {1, 0}, 255}, //
        {{-x, +x, -x}, {-1, 0, 0}, {0, 0}, 255}, //
        {{-x, -x, -x}, {-1, 0, 0}, {0, 1}, 255}, //

        {{-x, -x, -x}, {0, -1, 0}, {0, 1}, 255}, // bottom
        {{+x, -x, -x}, {0, -1, 0}, {1, 1}, 255}, //
        {{+x, -x, +x}, {0, -1, 0}, {1, 0}, 255}, //
        {{+x, -x, +x}, {0, -1, 0}, {1, 0}, 255}, //
        {{-x, -x, +x}, {0, -1, 0}, {0, 0}, 255}, //
        {{-x, -x, -x}, {0, -1, 0}, {0, 1}, 255}, //

        {{-x, +x, +x}, {0, +1, 0}, {0, 1}, 255}, // top
        {{+x, +x, +x}, {0, +1, 0}, {1, 1}, 255}, //
        {{+x, +x, -x}, {0, +1, 0}, {1, 0}, 255}, //
        {{+x, +x, -x}, {0, +1, 0}, {1, 0}, 255}, //
        {{-x, +x, -x}, {0, +1, 0}, {0, 0}, 255}, //
        {{-x, +x, +x}, {0, +1, 0}, {0, 1}, 255}, //
    };
    for (uint32_t i = 0; i < nv; i++)
    {
        transform_pos(&mesh, vertices[i].pos);
        transform_normal(&mesh, vertices[i].normal);
    }
    memcpy(vertex, vertices, sizeof(vertices));
    return mesh;
}

DvzMesh dvz_mesh_sphere(uint32_t row_count, uint32_t col_count)
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
            _vec3_copy((vec3){x, y, z}, positions[col_count * i + j]);
        }
    }
    DvzMesh mesh = dvz_mesh_grid(row_count, col_count, (const vec3*)positions);
    FREE(positions);
    return mesh;
}

DvzMesh dvz_mesh_cylinder(uint32_t count)
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
            _vec3_copy((vec3){x, .5 - i, z}, positions[k]);
            k++;
        }
    }
    DvzMesh mesh = dvz_mesh_grid(2, count, (const vec3*)positions);
    FREE(positions);
    return mesh;
}

DvzMesh dvz_mesh_cone(uint32_t count)
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
            _vec3_copy((vec3){x, -.5 + i, z}, positions[k]);
            k++;
        }
    }
    DvzMesh mesh = dvz_mesh_grid(2, count, (const vec3*)positions);
    FREE(positions);
    return mesh;
}

DvzMesh dvz_mesh_square()
{
    DvzMesh mesh = dvz_mesh();
    const uint32_t nv = 6;
    dvz_array_resize(&mesh.vertices, nv);
    DvzGraphicsMeshVertex* vertex = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    float x = .5;
    DvzGraphicsMeshVertex vertices[] = {
        {{-x, -x, 0}, {0, 0, +1}, {0, 1}, 255}, //
        {{+x, -x, 0}, {0, 0, +1}, {1, 1}, 255}, //
        {{+x, +x, 0}, {0, 0, +1}, {1, 0}, 255}, //
        {{+x, +x, 0}, {0, 0, +1}, {1, 0}, 255}, //
        {{-x, +x, 0}, {0, 0, +1}, {0, 0}, 255}, //
        {{-x, -x, 0}, {0, 0, +1}, {0, 1}, 255}, //
    };
    for (uint32_t i = 0; i < nv; i++)
    {
        transform_pos(&mesh, vertices[i].pos);
        transform_normal(&mesh, vertices[i].normal);
    }
    memcpy(vertex, vertices, sizeof(vertices));
    return mesh;
}

DvzMesh dvz_mesh_disc(uint32_t count)
{
    DvzMesh mesh = dvz_mesh();
    uint32_t nv = count + 1;
    uint32_t ni = 3 * count;
    dvz_array_resize(&mesh.vertices, nv);
    dvz_array_resize(&mesh.indices, ni);

    // Get vertices and indices pointers into the mesh arrays.
    DvzGraphicsMeshVertex* vertex = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    DvzIndex* index = (DvzIndex*)mesh.indices.data;
    uint32_t first_vertex = 0;

    // Variables.
    float r = .5;
    float dphi = M_2PI / count;
    float phi = 0;
    float x = 0, y = 0;

    // Center point.
    vec3 normal = {0, 0, -1};
    _vec3_copy((vec3){0, 0, 0}, vertex->pos);
    _vec3_copy(normal, vertex->normal);
    vertex->uv[0] = 0.5;
    vertex->uv[1] = 0;
    transform_pos(&mesh, vertex->pos);
    transform_normal(&mesh, vertex->normal);

    // Transform, colors, indices.
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
        _vec3_copy(normal, vertex->normal);

        vertex->uv[0] = i / (float)(count - 1);
        vertex->uv[1] = 1;

        vertex->alpha = 255;

        // Transform.
        transform_pos(&mesh, vertex->pos);
        transform_normal(&mesh, vertex->normal);

        // Indices.
        memcpy(
            index,
            (DvzIndex[]){
                first_vertex, first_vertex + i + 1, first_vertex + ((i + 1) % (count)) + 1},
            3 * sizeof(DvzIndex));
        index += 3;
    }
    return mesh;
}

void dvz_mesh_destroy(DvzMesh* mesh)
{
    ASSERT(mesh != NULL);
    dvz_array_destroy(&mesh->vertices);
    dvz_array_destroy(&mesh->indices);
}
