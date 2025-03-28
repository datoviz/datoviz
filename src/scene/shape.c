/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

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
/*  Macros and utils                                                                             */
/*************************************************************************************************/

#define COPY_SCALAR(x, idx, y) x[3 * i + idx] = shape->x[y];

#define COPY_VEC3(x, idx, y)                                                                      \
    x[3 * i + (idx)][0] = shape->x[y][0];                                                         \
    x[3 * i + (idx)][1] = shape->x[y][1];                                                         \
    x[3 * i + (idx)][2] = shape->x[y][2];

#define COPY_VEC4(x, idx, y)                                                                      \
    x[3 * i + (idx)][0] = shape->x[y][0];                                                         \
    x[3 * i + (idx)][1] = shape->x[y][1];                                                         \
    x[3 * i + (idx)][2] = shape->x[y][2];                                                         \
    x[3 * i + (idx)][3] = shape->x[y][3];

static inline void direction_vector(vec3 a, vec3 b, vec2 u)
{
    u[0] = b[0] - a[0];
    u[1] = b[1] - a[1];
    glm_vec2_normalize(u);
}

static inline float line_distance(vec3 p, vec3 q, vec2 u)
{
    float d = -(q[0] - p[0]) * u[1] + (q[1] - p[1]) * u[0];
    return d;
}



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

void dvz_compute_normals(
    uint32_t vertex_count, uint32_t index_count, vec3* pos, DvzIndex* index, vec3* normal)
{
    ANN(pos);
    ANN(index);
    ANN(normal);

    DvzIndex i0, i1, i2;
    vec3 u, v, n;
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;

    uint32_t face_count = index_count / 3;

    // BUG: this doesn't work if HAS_OPENMP is true
#if HAS_OPENMP
#pragma omp parallel for
#endif
    // Go through all triangle faces.
    for (uint32_t i = 0; i < face_count; i++)
    {
        i0 = index[3 * i + 0];
        i1 = index[3 * i + 1];
        i2 = index[3 * i + 2];

        glm_vec3_copy(pos[i0], v0);
        glm_vec3_copy(pos[i1], v1);
        glm_vec3_copy(pos[i2], v2);

        // u = v1-v0
        // v = v2-v0
        // n = u^v      normalized vector orthogonal to the current face
        glm_vec3_sub(v1, v0, u);
        glm_vec3_sub(v2, v0, v);
        glm_vec3_crossn(u, v, n);

        // Add the face normal to the current vertex normal.
        glm_vec3_add(normal[i0], n, normal[i0]);
        glm_vec3_add(normal[i1], n, normal[i1]);
        glm_vec3_add(normal[i2], n, normal[i2]);
    }

#if HAS_OPENMP
#pragma omp parallel for
#endif
    // Normalize all normals since every vertex might contain the sum of many normals.
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        glm_vec3_normalize(normal[i]);
        // glm_vec3_print(normal[i], stdout);
    }
}



void dvz_shape_normals(DvzShape* shape)
{
    ANN(shape);
    ANN(shape->pos);
    ANN(shape->index);
    ANN(shape->normal);
    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    dvz_compute_normals(vertex_count, index_count, shape->pos, shape->index, shape->normal);
}



DvzShape dvz_shape_merge(uint32_t count, DvzShape* shapes)
{
    DvzShape merged_shape = {0};

    glm_mat4_identity(merged_shape.transform);
    merged_shape.first = 0;
    merged_shape.count = 0;
    merged_shape.type = DVZ_SHAPE_OTHER;

    // Initialize vertex and index counts.
    merged_shape.vertex_count = 0;
    merged_shape.index_count = 0;

    // Calculate total vertex and index counts.
    bool has_normal = false, has_color = false, has_texcoords = false, has_isoline = false,
         has_d_left = false, has_d_right = false, has_contour = false, has_index = false;
    for (uint32_t i = 0; i < count; i++)
    {
        // Ensures all transformations are applied before merging the shapes.
        dvz_shape_end(&shapes[i]);

        merged_shape.vertex_count += shapes[i].vertex_count;
        merged_shape.index_count += shapes[i].index_count;

        has_normal |= shapes[i].normal != NULL;
        has_color |= shapes[i].color != NULL;
        has_texcoords |= shapes[i].texcoords != NULL;
        has_isoline |= shapes[i].isoline != NULL;
        has_d_left |= shapes[i].d_left != NULL;
        has_d_right |= shapes[i].d_right != NULL;
        has_contour |= shapes[i].contour != NULL;
        has_index |= shapes[i].index != NULL;
    }

    // Allocate memory for the merged shape's data.
    merged_shape.pos = (vec3*)malloc(merged_shape.vertex_count * sizeof(vec3));
    if (has_normal)
        merged_shape.normal = (vec3*)malloc(merged_shape.vertex_count * sizeof(vec3));
    if (has_color)
        merged_shape.color = (DvzColor*)malloc(merged_shape.vertex_count * sizeof(DvzColor));
    if (has_texcoords)
        merged_shape.texcoords = (vec4*)malloc(merged_shape.vertex_count * sizeof(vec4));
    if (has_isoline)
        merged_shape.isoline = (float*)malloc(merged_shape.vertex_count * sizeof(float));
    if (has_d_left)
        merged_shape.d_left = (vec3*)malloc(merged_shape.vertex_count * sizeof(vec3));
    if (has_d_right)
        merged_shape.d_right = (vec3*)malloc(merged_shape.vertex_count * sizeof(vec3));
    if (has_contour)
        merged_shape.contour = (cvec4*)malloc(merged_shape.vertex_count * sizeof(cvec4));
    if (has_index)
        merged_shape.index = (DvzIndex*)malloc(merged_shape.index_count * sizeof(DvzIndex));

    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;

    // Copy the data from each shape into the merged shape.
    for (uint32_t i = 0; i < count; i++)
    {
        DvzShape* shape = &shapes[i];

        memcpy(merged_shape.pos + vertex_offset, shape->pos, shape->vertex_count * sizeof(vec3));

        if (shape->normal != NULL)
            memcpy(
                merged_shape.normal + vertex_offset, shape->normal,
                shape->vertex_count * sizeof(vec3));

        if (shape->color != NULL)
            memcpy(
                merged_shape.color + vertex_offset, shape->color,
                shape->vertex_count * sizeof(DvzColor));

        if (shape->texcoords != NULL)
            memcpy(
                merged_shape.texcoords + vertex_offset, shape->texcoords,
                shape->vertex_count * sizeof(vec4));

        if (shape->isoline != NULL)
            memcpy(
                merged_shape.isoline + vertex_offset, shape->isoline,
                shape->vertex_count * sizeof(float));

        if (shape->d_left != NULL)
            memcpy(
                merged_shape.d_left + vertex_offset, shape->d_left,
                shape->vertex_count * sizeof(vec3));

        if (shape->d_right != NULL)
            memcpy(
                merged_shape.d_right + vertex_offset, shape->d_right,
                shape->vertex_count * sizeof(vec3));

        if (shape->contour != NULL)
            memcpy(
                merged_shape.contour + vertex_offset, shape->contour,
                shape->vertex_count * sizeof(cvec4));

        // Copy and reindex the indices.
        for (uint32_t j = 0; j < shape->index_count; j++)
        {
            merged_shape.index[index_offset + j] = shape->index[j] + vertex_offset;
        }

        // Update the offsets.
        vertex_offset += shape->vertex_count;
        index_offset += shape->index_count;
    }

    return merged_shape;
}



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



void dvz_shape_unindex(DvzShape* shape, int flags)
{
    ANN(shape);

    if (shape->index_count == 0)
    {
        log_warn("the shape is already non-indexed, skipping unindexing");
        return;
    }

    int32_t vertex_count = (int32_t)shape->vertex_count;
    ASSERT(vertex_count > 0);

    uint32_t index_count = shape->index_count;
    ASSERT(index_count > 0);

    uint32_t face_count = index_count / 3;
    ASSERT(face_count > 0);

    // Reindex positions.
    log_trace("reindex positions (%d vertices, %d indices)", vertex_count, index_count);
    vec3* pos = (vec3*)calloc(index_count, sizeof(vec3));
    DvzColor* color = NULL;
    vec3* normal = NULL;
    vec4* texcoords = NULL;
    float* isoline = NULL;
    vec3* d_left = NULL;
    vec3* d_right = NULL;
    cvec4* contour = NULL;

    if (shape->color != NULL)
        color = (DvzColor*)calloc(index_count, sizeof(DvzColor));
    if (shape->normal != NULL)
        normal = (vec3*)calloc(index_count, sizeof(vec3));
    if (shape->texcoords != NULL)
        texcoords = (vec4*)calloc(index_count, sizeof(vec4));
    if (shape->isoline != NULL)
        isoline = (float*)calloc(index_count, sizeof(float));

    d_left = (vec3*)calloc(index_count, sizeof(vec3));
    d_right = (vec3*)calloc(index_count, sizeof(vec3));
    contour = (cvec4*)calloc(index_count, sizeof(cvec4));
    int32_t v0, v1, v2;
    int32_t v0r, v1r, v2r;

    vec2* left = (vec2*)calloc((uint32_t)vertex_count, sizeof(vec2));
    vec2* right = (vec2*)calloc((uint32_t)vertex_count, sizeof(vec2));

    // Compute direction vectors of contours.
    // NOTE: assume clockwise orientation of the polygon contour.
    int32_t im, iM;
    for (int32_t i = 0; i < vertex_count; i++)
    {
        im = i > 0 ? i - 1 : i - 1 + vertex_count;
        iM = (i + 1) % vertex_count;

        // d_left[i] = P[i] - P[i-1] (2D)
        direction_vector(shape->pos[i], shape->pos[im], left[i]);

        // d_right[i] = P[i+1] - P[i] (2D)
        direction_vector(shape->pos[i], shape->pos[iM], right[i]);
    }

    // By default, contour on all triangles.
    bool e0 = false, e1 = false, e2 = false;
    float cross = 0;

    vec3 face[3] = {0};
    vec2 face_left[3] = {0};
    vec2 face_right[3] = {0};
    DvzIndex vertex_idx = 0;
    for (uint32_t i = 0; i < face_count; i++)
    {
        v0 = (int32_t)shape->index[3 * i + 0];
        v1 = (int32_t)shape->index[3 * i + 1];
        v2 = (int32_t)shape->index[3 * i + 2];
        ASSERT(v2 < vertex_count);

        v0r = v0;
        v1r = v1;
        v2r = v2;

        // Whether there should be an edge on the other side of the v0 vertex.
        e0 = ((abs(v1 - v2) % vertex_count) == 1) ||
             ((abs(v1 - v2) % vertex_count) == vertex_count - 1);
        // Whether there should be an edge on the other side of the v1 vertex.
        e1 = ((abs(v0 - v2) % vertex_count) == 1) ||
             ((abs(v0 - v2) % vertex_count) == vertex_count - 1);
        // Whether there should be an edge on the other side of the v2 vertex.
        e2 = ((abs(v0 - v1) % vertex_count) == 1) ||
             ((abs(v0 - v1) % vertex_count) == vertex_count - 1);

        glm_vec3_copy(shape->pos[v0r], face[0]);
        glm_vec3_copy(shape->pos[v1r], face[1]);
        glm_vec3_copy(shape->pos[v2r], face[2]);

        glm_vec2_copy(left[v0r], face_left[0]);
        glm_vec2_copy(left[v1r], face_left[1]);
        glm_vec2_copy(left[v2r], face_left[2]);

        glm_vec2_copy(right[v0r], face_right[0]);
        glm_vec2_copy(right[v1r], face_right[1]);
        glm_vec2_copy(right[v2r], face_right[2]);

        // Set edge bit mask on all vertices.
        if ((flags & DVZ_CONTOUR_FULL) > 0)
        {
            for (uint8_t k = 0; k < 3; k++)
                for (uint8_t l = 0; l < 3; l++)
                    contour[3 * i + k][l] |= 1;
        }

        // Set corner and edge bit mask depending on topology.
        else if ((flags & DVZ_CONTOUR_JOINTS) > 0)
        {
            // Compute d_left and d_right.
            for (uint8_t l = 0; l < 3; l++)
            {
                for (uint8_t k = 0; k < 3; k++)
                {
                    d_left[3 * i + k][l] = line_distance(face[l], face[k], face_left[l]);
                    d_right[3 * i + k][l] = -line_distance(face[l], face[k], face_right[l]);
                }
            }

            // if 3 edges, set contour=1 on the 3 vertices
            if (e0 && e1 && e2)
            {
                for (uint8_t k = 0; k < 3; k++)
                    for (uint8_t l = 0; l < 3; l++)
                        contour[3 * i + k][l] |= 1;
            }

            // 2 edges, the two opposite ends are corners
            else if (e1 && e2)
            {
                for (uint8_t k = 0; k < 3; k++)
                {
                    contour[3 * i + k][1] |= 2;
                    contour[3 * i + k][2] |= 2;
                }
            }
            else if (e0 && e2)
            {
                for (uint8_t k = 0; k < 3; k++)
                {
                    contour[3 * i + k][0] |= 2;
                    contour[3 * i + k][2] |= 2;
                }
            }
            else if (e0 && e1)
            {
                for (uint8_t k = 0; k < 3; k++)
                {
                    contour[3 * i + k][0] |= 2;
                    contour[3 * i + k][1] |= 2;
                }
            }

            // 1 or 0 edge: all corners
            else
            {
                for (uint8_t k = 0; k < 3; k++)
                    for (uint8_t l = 0; l < 3; l++)
                        contour[3 * i + k][l] |= 2;
            }

            // Orientation.
            //     if left[i_k] ^ right[i_k] < 0, then |4 for [k] on the 3 vertices
            for (uint8_t k = 0; k < 3; k++)
            {
                cross = glm_vec2_cross(face_left[k], face_right[k]);
                if (cross < 0)
                {
                    for (uint8_t l = 0; l < 3; l++)
                    {
                        contour[3 * i + l][k] |= 4;
                    }
                }
            }
        }

        // Only set edges on contour.
        else if ((flags & DVZ_CONTOUR_EDGES) > 0)
        {
            if (e0)
            {
                for (uint8_t l = 0; l < 3; l++)
                    contour[3 * i + l][0] |= 1;
            }
            if (e1)
            {
                for (uint8_t l = 0; l < 3; l++)
                    contour[3 * i + l][1] |= 1;
            }
            if (e2)
            {
                for (uint8_t l = 0; l < 3; l++)
                    contour[3 * i + l][2] |= 1;
            }
        }

        // Now we copy the unindexed pos, color, normal, texcoords values.
        COPY_VEC3(pos, 0, v0r)
        COPY_VEC3(pos, 1, v1r)
        COPY_VEC3(pos, 2, v2r)

        if (shape->color != NULL)
        {
            COPY_VEC4(color, 0, v0r)
            COPY_VEC4(color, 1, v1r)
            COPY_VEC4(color, 2, v2r)

            // DEBUG: random color on each triangle for debugging purposes
            // color[3 * i + 0][0] = color[3 * i + 1][0] = color[3 * i + 2][0] = dvz_rand_byte();
            // color[3 * i + 0][1] = color[3 * i + 1][1] = color[3 * i + 2][1] = dvz_rand_byte();
        }

        if (shape->normal != NULL)
        {
            COPY_VEC3(normal, 0, v0r)
            COPY_VEC3(normal, 1, v1r)
            COPY_VEC3(normal, 2, v2r)
        }

        if (shape->texcoords != NULL)
        {
            COPY_VEC4(texcoords, 0, v0r)
            COPY_VEC4(texcoords, 1, v1r)
            COPY_VEC4(texcoords, 2, v2r)
        }

        if (shape->isoline != NULL)
        {
            COPY_SCALAR(isoline, 0, v0r)
            COPY_SCALAR(isoline, 1, v1r)
            COPY_SCALAR(isoline, 2, v2r)
        }
    }
    FREE(shape->pos);
    shape->pos = pos;

    shape->d_left = d_left;
    shape->d_right = d_right;
    shape->contour = contour;

    if (shape->isoline != NULL)
    {
        FREE(shape->isoline);
        shape->isoline = isoline;
    }

    if (shape->color != NULL)
    {
        FREE(shape->color);
        shape->color = color;
    }

    if (shape->normal != NULL)
    {
        FREE(shape->normal);
        shape->normal = normal;
    }

    if (shape->texcoords != NULL)
    {
        FREE(shape->texcoords);
        shape->texcoords = texcoords;
    }

    shape->vertex_count = index_count;
    shape->index_count = 0;

    FREE(left);
    FREE(right);

    // NOTE: we keep shape->index around although we disable indexed rendering with index_count=0.
    // This is why this is commented.
    // FREE(shape->index);
}



void dvz_shape_destroy(DvzShape* shape)
{
    ANN(shape);
    FREE(shape->pos);
    FREE(shape->color);
    FREE(shape->texcoords);
    FREE(shape->normal);
    FREE(shape->d_left);
    FREE(shape->d_right);
    FREE(shape->contour);
    FREE(shape->isoline);

    FREE(shape->index);
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



void dvz_shape_end(DvzShape* shape)
{
    // Apply the transformation matrix.
    ANN(shape);

    if (shape->count == 0)
        return;

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



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_square(DvzColor color)
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
    shape.color = (DvzColor*)calloc(shape.vertex_count, sizeof(DvzColor));
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        memcpy(shape.color[i], color, sizeof(DvzColor));
    }

    // TODO: texcoords

    return shape;
}



DvzShape dvz_shape_disc(uint32_t count, DvzColor color)
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
    shape.color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        memcpy(shape.color[i], color, sizeof(DvzColor));
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



DvzShape dvz_shape_polygon(uint32_t count, const dvec2* points, DvzColor color)
{
    ASSERT(count > 2);
    ANN(points);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_POLYGON;
    uint32_t index_count = 0;

    // Run earcut.
    DvzIndex* indices = dvz_earcut(count, points, &index_count);

    if (indices == NULL)
    {
        log_error("Polygon triangulation failed");
        return shape;
    }
    ASSERT(index_count > 0);
    ANN(indices);

    shape.vertex_count = count;
    shape.index_count = index_count;
    shape.index = indices;

    // Position.
    shape.pos = (vec3*)calloc(count, sizeof(vec3));
    for (uint32_t i = 0; i < count; i++)
    {
        shape.pos[i][0] = (float)points[i][0];
        shape.pos[i][1] = (float)points[i][1];
    }

    // Color.
    shape.color = (DvzColor*)calloc(count, sizeof(DvzColor));
    for (uint32_t i = 0; i < count; i++)
    {
        shape.color[i][0] = color[0];
        shape.color[i][1] = color[1];
        shape.color[i][2] = color[2];
        shape.color[i][3] = color[3];
    }

    return shape;
}



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

DvzShape dvz_shape_surface(
    uint32_t row_count, uint32_t col_count, //
    float* heights, DvzColor* colors,       //
    vec3 o, vec3 u, vec3 v, int flags)
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
    shape.color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));

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
            shape.color[point_idx][0] = colors != NULL ? colors[point_idx][0] : DVZ_ALPHA_MAX;
            shape.color[point_idx][1] = colors != NULL ? colors[point_idx][1] : DVZ_ALPHA_MAX;
            shape.color[point_idx][2] = colors != NULL ? colors[point_idx][2] : DVZ_ALPHA_MAX;
            shape.color[point_idx][3] = colors != NULL ? colors[point_idx][3] : DVZ_ALPHA_MAX;

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



DvzShape dvz_shape_cube(DvzColor* colors)
{
    ANN(colors); // 6 colors, one per face

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CUBE;

    const uint32_t vertex_count = 36;

    shape.vertex_count = vertex_count;

    shape.pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape.color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
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
            ASSERT(i < 6);
            ASSERT(6 * i + j < vertex_count);
            memcpy(shape.color[6 * i + j], colors[i], sizeof(DvzColor));
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



DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, DvzColor color)
{
    ASSERT(rows > 0);
    ASSERT(cols > 0);

    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_SPHERE;
    // TODO
    log_error("dvz_shape_sphere() not yet implemented");
    return shape;
}



DvzShape dvz_shape_cone(uint32_t count, DvzColor color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CONE;
    // TODO
    log_error("dvz_shape_cone() not yet implemented");
    return shape;
}



DvzShape dvz_shape_cylinder(uint32_t count, DvzColor color)
{
    ASSERT(count > 0);
    DvzShape shape = {0};
    shape.type = DVZ_SHAPE_CYLINDER;
    // TODO
    log_error("dvz_shape_cylinder() not yet implemented");
    return shape;
}
