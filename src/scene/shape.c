/*************************************************************************************************/
/*  Shape                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "_macros.h"
#include "datoviz_math.h"

#include "datoviz.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

void dvz_shape_print(DvzShape* shape)
{
    ANN(shape);
    log_info(
        "shape type %d, %d vertices, %d indices", //
        shape->type, shape->vertex_count, shape->index_count);
    // for (uint32_t i = 0; i < shape->vertex_count; i++)
    // {
    //     log_info("%.3f %.3f %.3f", shape->pos[i][0], shape->pos[i][1], shape->pos[i][2]);
    // }
}



void dvz_shape_destroy(DvzShape* shape)
{
    ANN(shape);
    FREE(shape->pos);
    FREE(shape->index);
    FREE(shape->color);
    FREE(shape->texcoords);
    FREE(shape->normal);
}



/*************************************************************************************************/
/*  Shape transforms                                                                             */
/*************************************************************************************************/

static inline void transform_pos(mat4 transform, vec3 pos)
{
    glm_mat4_mulv3(transform, pos, 1, pos);
}



static inline void transform_normal(mat4 transform, vec3 normal)
{
    mat4 tr;
    glm_mat4_copy(transform, tr);
    glm_mat4_inv(tr, tr);
    glm_mat4_transpose(tr);
    glm_mat4_mulv3(tr, normal, 1, normal);
}



void dvz_shape_begin(DvzShape* shape, uint32_t first, uint32_t count)
{
    ANN(shape);
    glm_mat4_identity(shape->transform);

    count = count > 0 ? count : shape->vertex_count;
    first = CLIP(first, 0, shape->vertex_count - 1);
    ASSERT(first < shape->vertex_count);
    count = CLIP(count, 1, shape->vertex_count - first);

    ASSERT(first < shape->vertex_count);
    ASSERT(first + count <= shape->vertex_count);

    shape->first = first;
    shape->count = count;
}



void dvz_shape_scale(DvzShape* shape, vec3 scale)
{
    ANN(shape);
    mat4 tr;
    glm_scale_make(tr, scale);
    dvz_shape_transform(shape, tr);
}



void dvz_shape_translate(DvzShape* shape, vec3 translate)
{
    ANN(shape);
    mat4 tr;
    glm_translate_make(tr, translate);
    dvz_shape_transform(shape, tr);
}



void dvz_shape_rotate(DvzShape* shape, float angle, vec3 axis)
{
    ANN(shape);
    mat4 tr;
    glm_rotate_make(tr, angle, axis);
    dvz_shape_transform(shape, tr);
}



void dvz_shape_transform(DvzShape* shape, mat4 transform)
{
    ANN(shape);
    glm_mat4_mul(transform, shape->transform, shape->transform);
}



float dvz_shape_rescaling(DvzShape* shape, int flags, vec3 out_scale)
{
    ANN(shape);
    // TODO: compute the scaling factors.

    return 1.0;
}



void dvz_shape_normals(DvzShape* shape)
{
    ANN(shape);
    ANN(shape->pos);
    ANN(shape->index);
    ANN(shape->normal);

    DvzIndex i0, i1, i2;
    vec3 u, v, n;
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;

    uint32_t vertex_count = shape->vertex_count;
    uint32_t face_count = shape->index_count / 3;

    // Go through all triangle faces.
    for (uint32_t i = 0; i < face_count; i++)
    {
        i0 = shape->index[3 * i + 0];
        i1 = shape->index[3 * i + 1];
        i2 = shape->index[3 * i + 2];

        glm_vec3_copy(shape->pos[i0], v0);
        glm_vec3_copy(shape->pos[i1], v1);
        glm_vec3_copy(shape->pos[i2], v2);

        // u = v1-v0
        // v = v2-v0
        // n = u^v      normalized vector orthogonal to the current face
        glm_vec3_sub(v1, v0, u);
        glm_vec3_sub(v2, v0, v);
        glm_vec3_crossn(u, v, n);

        // Add the face normal to the current vertex normal.
        glm_vec3_add(shape->normal[i0], n, shape->normal[i0]);
        glm_vec3_add(shape->normal[i1], n, shape->normal[i1]);
        glm_vec3_add(shape->normal[i2], n, shape->normal[i2]);
    }

    // Normalize all normals since every vertex might contain the sum of many normals.
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        glm_vec3_normalize(shape->normal[i]);
    }
}



void dvz_shape_end(DvzShape* shape)
{
    ANN(shape);

    // Apply the transformation to the vertex positions and normals.
    for (uint32_t i = shape->first; i < shape->count; i++)
    {
        ASSERT(i < shape->vertex_count);
        transform_pos(shape->transform, shape->pos[i]);
        transform_normal(shape->transform, shape->normal[i]);
    }

    // Reset the transformation matrix.
    glm_mat4_identity(shape->transform);
}



void dvz_shape_merge(DvzShape* merged, DvzShape* to_merge)
{
    ANN(merged);
    ANN(to_merge);
    // TODO
}



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_square(cvec4 color)
{
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SQUARE;
    shape.vertex_count = 6;

    // Position.
    float x = .5;
    shape.pos = (vec3*)calloc(shape.vertex_count, sizeof(vec3));
    memcpy(
        shape.pos,
        (vec3[]){
            {-x, -x, 0},
            {+x, -x, 0},
            {+x, +x, 0},
            {+x, +x, 0},
            {-x, +x, 0},
            {-x, -x, 0},
        },
        shape.vertex_count * sizeof(vec3));

    // Normal.
    shape.normal = (vec3*)calloc(shape.vertex_count, sizeof(vec3));
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        shape.normal[i][2] = 1;
    }

    // Color.
    shape.color = (cvec4*)calloc(shape.vertex_count, sizeof(cvec4));
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        memcpy(shape.color[i], color, sizeof(cvec4));
    }

    // TODO: texcoords

    return shape;
}



DvzShape dvz_shape_disc(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_DISC;

    const uint32_t triangle_count = count;
    const uint32_t vertex_count = triangle_count + 1;
    const uint32_t index_count = 3 * triangle_count;

    shape.vertex_count = vertex_count;
    shape.index_count = index_count;

    // Position.
    shape.pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    // NOTE: start at i=1 because the first vertex is the origin (0,0)
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        shape.pos[i][0] = .5 * cos(M_2PI * (float)i / triangle_count);
        shape.pos[i][1] = .5 * sin(M_2PI * (float)i / triangle_count);
    }

    // Normal.
    shape.normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        shape.normal[i][2] = 1;
    }

    // Color.
    shape.color = (cvec4*)calloc(vertex_count, sizeof(cvec4));
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        memcpy(shape.color[i], color, sizeof(cvec4));
    }

    // TODO: texcoords

    // Index.
    shape.index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < triangle_count; i++)
    {
        ASSERT(3 * i + 2 < index_count);
        shape.index[3 * i + 0] = 0;
        shape.index[3 * i + 1] = i + 1;
        shape.index[3 * i + 2] = 1 + (i + 1) % triangle_count;
    }

    return shape;
}



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_surface(
    uint32_t row_count, uint32_t col_count, float* heights, //
    vec3 o, vec3 u, vec3 v, cvec4 color, int flags)
{
    // TODO: flag for closed surface on i or j
    // TODO: flag planar or cylindric

    ASSERT(row_count > 0);
    ASSERT(col_count > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SURFACE;

    const uint32_t vertex_count = col_count * row_count;
    const uint32_t index_count = 6 * (col_count - 1) * (row_count - 1);

    shape.vertex_count = vertex_count;
    shape.index_count = index_count;

    shape.pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape.color = (cvec4*)calloc(vertex_count, sizeof(cvec4));

    uint32_t point_idx = 0;
    uint32_t index = 0;

    vec3 normal = {0};
    glm_vec3_crossn(u, v, normal);
    float height = 0;

    for (uint32_t i = 0; i < row_count; i++)
    {
        for (uint32_t j = 0; j < col_count; j++)
        {
            ASSERT(point_idx == col_count * i + j);

            // Position.
            shape.pos[point_idx][0] = o[0] + i * u[0] + j * v[0];
            shape.pos[point_idx][1] = o[1] + i * u[1] + j * v[1];
            shape.pos[point_idx][2] = o[2] + i * u[2] + j * v[2];

            // Height.
            height = heights != NULL ? heights[point_idx] : 0;
            shape.pos[point_idx][0] += height * normal[0];
            shape.pos[point_idx][1] += height * normal[1];
            shape.pos[point_idx][2] += height * normal[2];

            // Color.
            shape.color[point_idx][0] = color[0];
            shape.color[point_idx][1] = color[1];
            shape.color[point_idx][2] = color[2];
            shape.color[point_idx][3] = color[3];

            // Index.
            // TODO: shape topology (flags) to implement here
            if ((i < row_count - 1) && (j < col_count - 1))
            {
                ASSERT(index + 5 < index_count);
                shape.index[index++] = col_count * (i + 0) + (j + 0);
                shape.index[index++] = col_count * (i + 1) + (j + 0);
                shape.index[index++] = col_count * (i + 0) + (j + 1);
                shape.index[index++] = col_count * (i + 1) + (j + 1);
                shape.index[index++] = col_count * (i + 0) + (j + 1);
                shape.index[index++] = col_count * (i + 1) + (j + 0);
            }

            point_idx++;
        }
    }

    dvz_shape_normals(&shape);

    return shape;
}



DvzShape dvz_shape_cube(cvec4* colors)
{
    ANN(colors); // 6 colors, one per face

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CUBE;

    const uint32_t vertex_count = 36;

    shape.vertex_count = vertex_count;

    shape.pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.color = (cvec4*)calloc(vertex_count, sizeof(cvec4));
    shape.texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    float x = .5;

    // Position.
    memcpy(
        shape.pos,
        (vec3[]){
            {-x, -x, +x}, // front
            {+x, -x, +x}, //
            {+x, +x, +x}, //
            {+x, +x, +x}, //
            {-x, +x, +x}, //
            {-x, -x, +x}, //
            {+x, -x, +x}, // right
            {+x, -x, -x}, //
            {+x, +x, -x}, //
            {+x, +x, -x}, //
            {+x, +x, +x}, //
            {+x, -x, +x}, //
            {-x, +x, -x}, // back
            {+x, +x, -x}, //
            {+x, -x, -x}, //
            {+x, -x, -x}, //
            {-x, -x, -x}, //
            {-x, +x, -x}, //
            {-x, -x, -x}, // left
            {-x, -x, +x}, //
            {-x, +x, +x}, //
            {-x, +x, +x}, //
            {-x, +x, -x}, //
            {-x, -x, -x}, //
            {-x, -x, -x}, // bottom
            {+x, -x, -x}, //
            {+x, -x, +x}, //
            {+x, -x, +x}, //
            {-x, -x, +x}, //
            {-x, -x, -x}, //
            {-x, +x, +x}, // top
            {+x, +x, +x}, //
            {+x, +x, -x}, //
            {+x, +x, -x}, //
            {-x, +x, -x}, //
            {-x, +x, +x}, //
        },
        vertex_count * sizeof(vec3));

    // Normal.
    memcpy(
        shape.normal,
        (vec3[]){
            {0, 0, +1}, // front
            {0, 0, +1}, //
            {0, 0, +1}, //
            {0, 0, +1}, //
            {0, 0, +1}, //
            {0, 0, +1}, //
            {+1, 0, 0}, // right
            {+1, 0, 0}, //
            {+1, 0, 0}, //
            {+1, 0, 0}, //
            {+1, 0, 0}, //
            {+1, 0, 0}, //
            {0, 0, -1}, // back
            {0, 0, -1}, //
            {0, 0, -1}, //
            {0, 0, -1}, //
            {0, 0, -1}, //
            {0, 0, -1}, //
            {-1, 0, 0}, // left
            {-1, 0, 0}, //
            {-1, 0, 0}, //
            {-1, 0, 0}, //
            {-1, 0, 0}, //
            {-1, 0, 0}, //
            {0, -1, 0}, // bottom
            {0, -1, 0}, //
            {0, -1, 0}, //
            {0, -1, 0}, //
            {0, -1, 0}, //
            {0, -1, 0}, //
            {0, +1, 0}, // top
            {0, +1, 0}, //
            {0, +1, 0}, //
            {0, +1, 0}, //
            {0, +1, 0}, //
            {0, +1, 0}, //
        },
        vertex_count * sizeof(vec3));

    // Color.
    for (uint32_t i = 0; i < 6; i++)
    {
        for (uint32_t j = 0; j < 6; j++)
        {
            memcpy(shape.color[6 * i + j], colors[i], sizeof(cvec4));
        }
    }

    // Texture coordinates.
    memcpy(
        shape.texcoords,
        (vec4[]){
            // NOTE: u, v, *, a
            {0, 1, 0, 1}, // front
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
            {0, 1, 0, 1}, // right
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
            {1, 0, 0, 1}, // back
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
            {0, 1, 0, 1}, //
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 1, 0, 1}, // left
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
            {0, 1, 0, 1}, // bottom
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
            {0, 1, 0, 1}, // top
            {1, 1, 0, 1}, //
            {1, 0, 0, 1}, //
            {1, 0, 0, 1}, //
            {0, 0, 0, 1}, //
            {0, 1, 0, 1}, //
        },
        vertex_count * sizeof(vec4));

    return shape;
}



DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, cvec4 color)
{
    ASSERT(rows > 0);
    ASSERT(cols > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SPHERE;
    // TODO
    return shape;
}



DvzShape dvz_shape_cone(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CONE;
    // TODO
    return shape;
}



DvzShape dvz_shape_cylinder(uint32_t count, cvec4 color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CYLINDER;
    // TODO
    return shape;
}
