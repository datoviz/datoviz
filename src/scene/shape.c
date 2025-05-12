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
#include "_pointer.h"
#include "datoviz_math.h"

#include "datoviz.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Macros and utils                                                                             */
/*************************************************************************************************/

#define ARROW_SIDE_COUNT 32

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

static inline void normalize_vec3(float* v)
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 0.0f)
    {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}



/*************************************************************************************************/
/*  Polyhedrons                                                                                  */
/*************************************************************************************************/

static void generate_tetrahedron(DvzShape* shape)
{
    float v[4][3] = {
        {-1, -1, -1},
        {-1, 1, 1},
        {1, -1, 1},
        {1, 1, -1},
    };
    DvzIndex idx[] = {
        1, 0, 2, 0, 1, 3, 3, 1, 2, 0, 3, 2,
    };

    shape->vertex_count = 4;
    shape->index_count = 12;
    shape->pos = calloc(4, sizeof(vec3));
    shape->texcoords = calloc(4, sizeof(vec4));
    shape->index = calloc(12, sizeof(DvzIndex));

    for (uint32_t i = 0; i < 4; i++)
    {
        memcpy(shape->pos[i], v[i], sizeof(vec3));
        normalize_vec3(shape->pos[i]);
        float x = shape->pos[i][0], y = shape->pos[i][1], z = shape->pos[i][2];
        shape->texcoords[i][0] = 0.5f + atan2f(z, x) / (2 * M_PI);
        shape->texcoords[i][1] = 0.5f - asinf(y) / M_PI;
    }

    memcpy(shape->index, idx, sizeof(idx));
}



static void generate_hexahedron(DvzShape* shape)
{
    float v[8][3] = {
        {1, 1, -1},   {1, -1, 1},  {1, -1, -1}, {1, 1, 1},
        {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1},  {-1, -1, 1},
    };
    DvzIndex idx[] = {
        0, 1, 2, 1, 0, 3, 0, 4, 5, 4, 0, 2, 6, 0, 5, 0, 6, 3,
        1, 6, 7, 6, 1, 3, 1, 4, 2, 4, 1, 7, 6, 4, 7, 4, 6, 5,
    };

    shape->vertex_count = 8;
    shape->index_count = 36;
    shape->pos = calloc(8, sizeof(vec3));
    shape->texcoords = calloc(8, sizeof(vec4));
    shape->index = calloc(36, sizeof(DvzIndex));

    for (uint32_t i = 0; i < 8; i++)
    {
        memcpy(shape->pos[i], v[i], sizeof(vec3));
        normalize_vec3(shape->pos[i]);
        float x = shape->pos[i][0], y = shape->pos[i][1], z = shape->pos[i][2];
        shape->texcoords[i][0] = 0.5f + atan2f(z, x) / (2 * M_PI);
        shape->texcoords[i][1] = 0.5f - asinf(y) / M_PI;
    }

    memcpy(shape->index, idx, sizeof(idx));
}



static void generate_octahedron(DvzShape* shape)
{
    float v[6][3] = {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {1, 0, 0}, {0, -1, 0}, {0, 0, 1}};
    DvzIndex idx[] = {0, 1, 2, 3, 0, 2, 1, 0, 4, 2, 1, 5, 3, 2, 5, 4, 0, 3, 5, 1, 4, 5, 4, 3};

    shape->vertex_count = 6;
    shape->index_count = 24;
    shape->pos = calloc(6, sizeof(vec3));
    shape->texcoords = calloc(6, sizeof(vec4));
    shape->index = calloc(24, sizeof(DvzIndex));

    for (uint32_t i = 0; i < 6; i++)
    {
        memcpy(shape->pos[i], v[i], sizeof(vec3));
        normalize_vec3(shape->pos[i]);
        float x = shape->pos[i][0], y = shape->pos[i][1], z = shape->pos[i][2];
        shape->texcoords[i][0] = 0.5f + atan2f(z, x) / (2 * M_PI);
        shape->texcoords[i][1] = 0.5f - asinf(y) / M_PI;
    }

    memcpy(shape->index, idx, sizeof(idx));
}



static void generate_dodecahedron(DvzShape* shape)
{
    float phi = 1.618033988749895f;
    float iphi = 1.0f / phi;

    float v[20][3] = {{-1, 1, -1},      {-phi, 0, iphi}, {-phi, 0, -iphi}, {-1, 1, 1},
                      {-iphi, phi, 0},  {1, 1, 1},       {iphi, phi, 0},   {0, iphi, phi},
                      {-1, -1, 1},      {0, -iphi, phi}, {-1, -1, -1},     {-iphi, -phi, 0},
                      {0, -iphi, -phi}, {0, iphi, -phi}, {1, 1, -1},       {phi, 0, -iphi},
                      {phi, 0, iphi},   {1, -1, 1},      {iphi, -phi, 0},  {1, -1, -1}};

    DvzIndex idx[] = {1,  0,  2,  0,  1,  3,  0,  3,  4,  4,  5,  6,  5,  4,  3,  5,  3,  7,
                      8,  3,  1,  3,  8,  7,  7,  8,  9,  8,  10, 11, 10, 8,  2,  2,  8,  1,
                      0,  10, 2,  10, 0,  12, 12, 0,  13, 0,  14, 13, 14, 0,  6,  6,  0,  4,
                      15, 5,  16, 5,  15, 14, 5,  14, 6,  9,  5,  7,  5,  9,  17, 5,  17, 16,
                      18, 8,  11, 8,  18, 17, 8,  17, 9,  19, 10, 12, 10, 19, 11, 11, 19, 18,
                      13, 19, 12, 19, 13, 14, 19, 14, 15, 19, 17, 18, 17, 19, 16, 16, 19, 15};

    shape->vertex_count = 20;
    shape->index_count = sizeof(idx) / sizeof(DvzIndex);
    shape->pos = calloc(shape->vertex_count, sizeof(vec3));
    shape->texcoords = calloc(shape->vertex_count, sizeof(vec4));
    shape->index = calloc(shape->index_count, sizeof(DvzIndex));

    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        memcpy(shape->pos[i], v[i], sizeof(vec3));
        normalize_vec3(shape->pos[i]);
        float x = shape->pos[i][0], y = shape->pos[i][1], z = shape->pos[i][2];
        shape->texcoords[i][0] = 0.5f + atan2f(z, x) / (2 * M_PI);
        shape->texcoords[i][1] = 0.5f - asinf(y) / M_PI;
    }

    memcpy(shape->index, idx, sizeof(idx));
}



static void generate_icosahedron(DvzShape* shape)
{
    float phi = 1.618033988749895f;

    float v[12][3] = {{-1, 0, -phi}, {0, phi, -1}, {1, 0, -phi}, {0, phi, 1},
                      {phi, 1, 0},   {1, 0, phi},  {-phi, 1, 0}, {-phi, -1, 0},
                      {-1, 0, phi},  {0, -phi, 1}, {phi, -1, 0}, {0, -phi, -1}};

    DvzIndex idx[] = {1, 6, 3, 0, 6, 1, 3, 4, 1, 3,  6, 8,  6, 0,  7,  2, 0, 1,  4,  3,
                      5, 4, 2, 1, 7, 8, 6, 5, 3, 8,  0, 11, 7, 11, 0,  2, 4, 5,  10, 10,
                      2, 4, 8, 7, 9, 8, 9, 5, 7, 11, 9, 11, 2, 10, 10, 5, 9, 11, 10, 9};

    shape->vertex_count = 12;
    shape->index_count = sizeof(idx) / sizeof(DvzIndex);
    shape->pos = calloc(shape->vertex_count, sizeof(vec3));
    shape->texcoords = calloc(shape->vertex_count, sizeof(vec4));
    shape->index = calloc(shape->index_count, sizeof(DvzIndex));

    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        memcpy(shape->pos[i], v[i], sizeof(vec3));
        normalize_vec3(shape->pos[i]);
        float x = shape->pos[i][0], y = shape->pos[i][1], z = shape->pos[i][2];
        shape->texcoords[i][0] = 0.5f + atan2f(z, x) / (2 * M_PI);
        shape->texcoords[i][1] = 0.5f - asinf(y) / M_PI;
    }

    memcpy(shape->index, idx, sizeof(idx));
}



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

void dvz_compute_normals(
    uint32_t vertex_count, uint32_t index_count, vec3* pos, DvzIndex* index, vec3* normal)
{
    ANN(pos);
    ANN(normal);

    DvzIndex i0, i1, i2;
    vec3 u, v, n;
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;

    // Default indices.
    if (index_count == 0)
    {
        ASSERT(index == NULL);
        // if (index != NULL)
        // {
        //     FREE(index)
        // };
        index_count = vertex_count;
        index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
        for (uint32_t i = 0; i < index_count; i++)
        {
            index[i] = i;
        }
    }
    ASSERT(index_count % 3 == 0);

    uint32_t face_count = index_count / 3;

    log_trace("starting to compute shape normals");

    // BUG: this doesn't work if HAS_OPENMP is true
#if HAS_OPENMP
#pragma omp parallel for
#endif
    // Go through all triangle faces.
    for (uint32_t i = 0; i < face_count; i++)
    {
        ASSERT(3 * i + 2 < index_count);

        i0 = index[3 * i + 0];
        i1 = index[3 * i + 1];
        i2 = index[3 * i + 2];

        ASSERT(i0 < vertex_count);
        ASSERT(i1 < vertex_count);
        ASSERT(i2 < vertex_count);

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

    log_trace("starting normal normalization");
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



DvzShape* dvz_shape(void)
{
    DvzShape* shape = (DvzShape*)calloc(1, sizeof(DvzShape));
    return shape;
}



void dvz_shape_custom(
    DvzShape* shape, uint32_t vertex_count, vec3* positions, vec3* normals, DvzColor* colors,
    vec4* texcoords, uint32_t index_count, DvzIndex* indices)
{
    ANN(positions);
    ANN(indices);
    ANN(shape);
    ASSERT(vertex_count > 0);
    ASSERT(index_count > 0);

    shape->type = DVZ_SHAPE_OTHER;
    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)_cpy(vertex_count * sizeof(vec3), positions);

    shape->index = (DvzIndex*)_cpy(index_count * sizeof(DvzIndex), indices);

    if (colors != NULL)
    {
        shape->color = (DvzColor*)_cpy(vertex_count * sizeof(DvzColor), colors);
    }

    if (texcoords != NULL)
    {
        shape->texcoords = (vec4*)_cpy(vertex_count * sizeof(vec4), texcoords);
    }

    if (normals != NULL)
    {
        shape->normal = (vec3*)_cpy(vertex_count * sizeof(vec3), normals);
    }

    log_trace("shape created with %d vertices and %d indices", vertex_count, index_count);
}



void dvz_shape_normals(DvzShape* shape)
{
    ANN(shape);
    ANN(shape->pos);
    // ANN(shape->index);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    if (shape->normal == NULL)
    {
        shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    }
    ANN(shape->normal);

    dvz_compute_normals(vertex_count, index_count, shape->pos, shape->index, shape->normal);
}



void dvz_shape_merge(DvzShape* merged, uint32_t count, DvzShape** shapes)
{
    ASSERT(count > 0);
    ANN(shapes);
    ANN(merged);

    glm_mat4_identity(merged->transform);
    merged->first = 0;
    merged->count = 0;
    merged->type = DVZ_SHAPE_OTHER;

    // Initialize vertex and index counts.
    merged->vertex_count = 0;
    merged->index_count = 0;

    // Calculate total vertex and index counts.
    bool has_normal = false, has_color = false, has_texcoords = false, has_isoline = false,
         has_d_left = false, has_d_right = false, has_contour = false, has_index = false;
    for (uint32_t i = 0; i < count; i++)
    {
        ANN(shapes[i]);

        // Ensures all transformations are applied before merging the shapes.
        dvz_shape_end(shapes[i]);

        merged->vertex_count += shapes[i]->vertex_count;
        merged->index_count += shapes[i]->index_count;

        has_normal |= shapes[i]->normal != NULL;
        has_color |= shapes[i]->color != NULL;
        has_texcoords |= shapes[i]->texcoords != NULL;
        has_isoline |= shapes[i]->isoline != NULL;
        has_d_left |= shapes[i]->d_left != NULL;
        has_d_right |= shapes[i]->d_right != NULL;
        has_contour |= shapes[i]->contour != NULL;
        has_index |= shapes[i]->index_count > 0;
    }

    ASSERT(merged->vertex_count > 0);

    // Allocate memory for the merged shape's data.
    merged->pos = (vec3*)calloc(merged->vertex_count, sizeof(vec3));

    if (has_normal)
    {
        merged->normal = (vec3*)calloc(merged->vertex_count, sizeof(vec3));
    }

    if (has_color)
    {
        merged->color = (DvzColor*)calloc(merged->vertex_count, sizeof(DvzColor));
    }

    if (has_texcoords)
    {
        merged->texcoords = (vec4*)calloc(merged->vertex_count, sizeof(vec4));
    }

    if (has_isoline)
    {
        merged->isoline = (float*)calloc(merged->vertex_count, sizeof(float));
    }

    if (has_d_left)
    {
        merged->d_left = (vec3*)calloc(merged->vertex_count, sizeof(vec3));
    }

    if (has_d_right)
    {
        merged->d_right = (vec3*)calloc(merged->vertex_count, sizeof(vec3));
    }

    if (has_contour)
    {
        merged->contour = (cvec4*)calloc(merged->vertex_count, sizeof(cvec4));
    }

    if (has_index)
    {
        ASSERT(merged->index_count > 0);
        merged->index = (DvzIndex*)calloc(merged->index_count, sizeof(DvzIndex));
    }

    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;

    // Copy the data from each shape into the merged shape.
    for (uint32_t i = 0; i < count; i++)
    {
        DvzShape* shape = shapes[i];

        memcpy(merged->pos + vertex_offset, shape->pos, shape->vertex_count * sizeof(vec3));

        if (shape->normal != NULL)
            memcpy(
                merged->normal + vertex_offset, shape->normal, shape->vertex_count * sizeof(vec3));

        if (shape->color != NULL)
            memcpy(
                merged->color + vertex_offset, shape->color,
                shape->vertex_count * sizeof(DvzColor));

        if (shape->texcoords != NULL)
            memcpy(
                merged->texcoords + vertex_offset, shape->texcoords,
                shape->vertex_count * sizeof(vec4));

        if (shape->isoline != NULL)
            memcpy(
                merged->isoline + vertex_offset, shape->isoline,
                shape->vertex_count * sizeof(float));

        if (shape->d_left != NULL)
            memcpy(
                merged->d_left + vertex_offset, shape->d_left, shape->vertex_count * sizeof(vec3));

        if (shape->d_right != NULL)
            memcpy(
                merged->d_right + vertex_offset, shape->d_right,
                shape->vertex_count * sizeof(vec3));

        if (shape->contour != NULL)
            memcpy(
                merged->contour + vertex_offset, shape->contour,
                shape->vertex_count * sizeof(cvec4));

        // Copy and reindex the indices.
        for (uint32_t j = 0; j < shape->index_count; j++)
        {
            merged->index[index_offset + j] = shape->index[j] + vertex_offset;
        }

        // Update the offsets.
        vertex_offset += shape->vertex_count;
        index_offset += shape->index_count;
    }
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



uint32_t dvz_shape_vertex_count(DvzShape* shape)
{
    ANN(shape);
    return shape->vertex_count;
}



uint32_t dvz_shape_index_count(DvzShape* shape)
{
    ANN(shape);
    return shape->index_count;
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

        // NOTE: the logic below that determines which edge of each face should have a border,
        // depends on the specific indexing method. The current logic is meant for the earcut
        // triangulation algorithm that only uses existing points for the triangulation,
        // using only indexing to triangulate the polygon.

        // TODO: the user should be able to provide (e0, e1, e2) for each face (1 byte per face
        // sufficient).

        if ((flags & DVZ_INDEXING_EARCUT) > 0)
        {
            // Whether there should be an edge on the other side of the v0 vertex.
            e0 = ((abs(v1 - v2) % vertex_count) == 1) ||
                 ((abs(v1 - v2) % vertex_count) == vertex_count - 1);
            // Whether there should be an edge on the other side of the v1 vertex.
            e1 = ((abs(v0 - v2) % vertex_count) == 1) ||
                 ((abs(v0 - v2) % vertex_count) == vertex_count - 1);
            // Whether there should be an edge on the other side of the v2 vertex.
            e2 = ((abs(v0 - v1) % vertex_count) == 1) ||
                 ((abs(v0 - v1) % vertex_count) == vertex_count - 1);
        }

        // When using a surface mesh triangulated by Datoviz, this scheme ensures that all
        // quads have a contour.
        else if ((flags & DVZ_INDEXING_SURFACE) > 0)
        {
            e0 = 0;
            e1 = 1;
            e2 = 1;
        }

        else
        {
            e0 = e1 = e2 = 0;
        }

        // Other indexing strategies, defining (e0, e1, e2) as a function of a face's indices,
        // could be implemented here.


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
    FREE(shape->index);

    FREE(left);
    FREE(right);
}



void dvz_shape_destroy(DvzShape* shape)
{
    log_trace("destroy shape");

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

    FREE(shape);
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
        if (shape->normal != NULL)
        {
            transform_normal(shape->transform, shape->normal[i]);
        }
    }

    // Reset the transformation matrix.
    glm_mat4_identity(shape->transform);
}



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

void dvz_shape_square(DvzShape* shape, DvzColor color)
{
    ANN(shape);

    shape->type = DVZ_SHAPE_SQUARE;
    shape->vertex_count = 6;

    // Position.
    float x = .5;
    shape->pos = (vec3*)calloc(shape->vertex_count, sizeof(vec3));
    memcpy(
        shape->pos,
        (vec3[]){
            {-x, -x, 0},
            {+x, -x, 0},
            {+x, +x, 0},
            {+x, +x, 0},
            {-x, +x, 0},
            {-x, -x, 0},
        },
        shape->vertex_count * sizeof(vec3));

    // Normal.
    shape->normal = (vec3*)calloc(shape->vertex_count, sizeof(vec3));
    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        shape->normal[i][2] = 1;
    }

    // Color.
    shape->color = (DvzColor*)calloc(shape->vertex_count, sizeof(DvzColor));
    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        memcpy(shape->color[i], color, sizeof(DvzColor));
    }

    // Texcoords.
    shape->texcoords = (vec4*)calloc(shape->vertex_count, sizeof(vec4));
    memcpy(
        shape->texcoords,
        (vec4[]){
            {0, 0, 0, 1},
            {1, 0, 0, 1},
            {1, 1, 0, 1},
            {1, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 0, 0, 1},
        },
        shape->vertex_count * sizeof(vec4));
}



void dvz_shape_disc(DvzShape* shape, uint32_t count, DvzColor color)
{
    ASSERT(count > 0);
    ANN(shape);

    shape->type = DVZ_SHAPE_DISC;

    const uint32_t triangle_count = count;
    const uint32_t vertex_count = triangle_count + 1;
    const uint32_t index_count = 3 * triangle_count;

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    // Position.
    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    // NOTE: start at i=1 because the first vertex is the origin (0,0)
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        shape->pos[i][0] = .5 * cos(M_2PI * (float)i / triangle_count);
        shape->pos[i][1] = .5 * sin(M_2PI * (float)i / triangle_count);
    }

    // Normal.
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        shape->normal[i][2] = 1;
    }

    // Color.
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        memcpy(shape->color[i], color, sizeof(DvzColor));
    }

    // Texcoords.
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));
    shape->texcoords[0][0] = 0.5f;
    shape->texcoords[0][1] = 0.5f;
    shape->texcoords[0][2] = 0.0f;
    shape->texcoords[0][3] = 1.0f;
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        float x = shape->pos[i][0];
        float y = shape->pos[i][1];
        shape->texcoords[i][0] = 0.5f + x;
        shape->texcoords[i][1] = 0.5f + y;
        shape->texcoords[i][2] = 0.0f;
        shape->texcoords[i][3] = 1.0f;
    }

    // Index.
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < triangle_count; i++)
    {
        ASSERT(3 * i + 2 < index_count);
        shape->index[3 * i + 0] = 0;
        shape->index[3 * i + 1] = i + 1;
        shape->index[3 * i + 2] = 1 + (i + 1) % triangle_count;
    }
}



void dvz_shape_polygon(DvzShape* shape, uint32_t count, const dvec2* points, DvzColor color)
{
    ASSERT(count > 2);
    ANN(points);
    ANN(shape);

    shape->type = DVZ_SHAPE_POLYGON;
    uint32_t index_count = 0;

    // Run earcut.
    DvzIndex* indices = dvz_earcut(count, points, &index_count);

    if (indices == NULL)
    {
        log_error("Polygon triangulation failed");
        return;
    }
    ASSERT(index_count > 0);
    ANN(indices);

    shape->vertex_count = count;
    shape->index_count = index_count;
    shape->index = indices;

    // Position.
    shape->pos = (vec3*)calloc(count, sizeof(vec3));
    for (uint32_t i = 0; i < count; i++)
    {
        shape->pos[i][0] = (float)points[i][0];
        shape->pos[i][1] = (float)points[i][1];
    }

    // Color.
    shape->color = (DvzColor*)calloc(count, sizeof(DvzColor));
    for (uint32_t i = 0; i < count; i++)
    {
        shape->color[i][0] = color[0];
        shape->color[i][1] = color[1];
        shape->color[i][2] = color[2];
        shape->color[i][3] = color[3];
    }
}



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

void dvz_shape_surface(
    DvzShape* shape, uint32_t row_count, uint32_t col_count, //
    float* heights, DvzColor* colors,                        //
    vec3 o, vec3 u, vec3 v, int flags)
{
    // TODO: flag for closed surface on i or j
    // TODO: flag planar or cylindric

    ASSERT(row_count > 1);
    ASSERT(col_count > 1);

    shape->type = DVZ_SHAPE_SURFACE;

    const uint32_t vertex_count = col_count * row_count;
    const uint32_t index_count = 6 * (col_count - 1) * (row_count - 1);

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

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
            shape->pos[point_idx][0] = o[0] + i * u[0] + j * v[0];
            shape->pos[point_idx][1] = o[1] + i * u[1] + j * v[1];
            shape->pos[point_idx][2] = o[2] + i * u[2] + j * v[2];

            // Height.
            height = heights != NULL ? heights[point_idx] : 0;
            shape->pos[point_idx][0] += height * normal[0];
            shape->pos[point_idx][1] += height * normal[1];
            shape->pos[point_idx][2] += height * normal[2];

            // Color.
            shape->color[point_idx][0] = colors != NULL ? colors[point_idx][0] : DVZ_ALPHA_MAX;
            shape->color[point_idx][1] = colors != NULL ? colors[point_idx][1] : DVZ_ALPHA_MAX;
            shape->color[point_idx][2] = colors != NULL ? colors[point_idx][2] : DVZ_ALPHA_MAX;
            shape->color[point_idx][3] = colors != NULL ? colors[point_idx][3] : DVZ_ALPHA_MAX;

            shape->texcoords[point_idx][0] = i / (float)(row_count - 1); // in [0, 1] along i axis
            shape->texcoords[point_idx][1] = j / (float)(col_count - 1); // in [0, 1] along j axis
            // shape->texcoords[point_idx][2];     // unused for now
            shape->texcoords[point_idx][3] = 1; // alpha

            // Index.
            // TODO: shape topology (flags) to implement here
            if ((i < row_count - 1) && (j < col_count - 1))
            {
                ASSERT(index + 5 < index_count);
                shape->index[index++] = col_count * (i + 0) + (j + 0);
                shape->index[index++] = col_count * (i + 1) + (j + 0);
                shape->index[index++] = col_count * (i + 0) + (j + 1);
                shape->index[index++] = col_count * (i + 1) + (j + 1);
                shape->index[index++] = col_count * (i + 0) + (j + 1);
                shape->index[index++] = col_count * (i + 1) + (j + 0);
            }

            point_idx++;
        }
    }

    dvz_shape_normals(shape);
}



void dvz_shape_cube(DvzShape* shape, DvzColor* colors)
{
    ANN(colors); // 6 colors, one per face
    ANN(shape);

    shape->type = DVZ_SHAPE_CUBE;

    const uint32_t vertex_count = 36;

    shape->vertex_count = vertex_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    float x = .5;

    // Position.
    memcpy(
        shape->pos,
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
        shape->normal,
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
            memcpy(shape->color[6 * i + j], colors[i], sizeof(DvzColor));
        }
    }

    // Texture coordinates.
    memcpy(
        shape->texcoords,
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
}



void dvz_shape_sphere(DvzShape* shape, uint32_t rows, uint32_t cols, DvzColor color)
{
    ASSERT(rows > 0);
    ASSERT(cols > 2);
    ANN(shape);

    shape->type = DVZ_SHAPE_SPHERE;

    const float radius = 0.5f;
    const uint32_t vertex_count = (rows + 1) * (cols + 1);
    const uint32_t index_count = 6 * rows * cols;

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    uint32_t point_idx = 0;
    for (uint32_t i = 0; i <= rows; i++)
    {
        float theta = M_PI * i / rows; // [0, PI]
        float y = cosf(theta);         // vertical axis
        float r = sinf(theta);

        for (uint32_t j = 0; j <= cols; j++)
        {
            float phi = 2.0f * M_PI * j / cols; // [0, 2PI]
            float x = r * cosf(phi);
            float z = r * sinf(phi);

            // Position.
            shape->pos[point_idx][0] = radius * x;
            shape->pos[point_idx][1] = radius * y;
            shape->pos[point_idx][2] = radius * z;

            // Normal.
            glm_vec3_copy((vec3){x, y, z}, shape->normal[point_idx]);
            glm_vec3_normalize(shape->normal[point_idx]);

            // Color.
            memcpy(shape->color[point_idx], color, sizeof(DvzColor));

            // Texcoords.
            shape->texcoords[point_idx][0] = j / (float)cols;
            shape->texcoords[point_idx][1] = i / (float)rows;
            shape->texcoords[point_idx][3] = 1; // alpha

            point_idx++;
        }
    }

    // Indices.
    uint32_t index = 0;
    for (uint32_t i = 0; i < rows; i++)
    {
        for (uint32_t j = 0; j < cols; j++)
        {
            uint32_t i0 = i * (cols + 1) + j;
            uint32_t i1 = (i + 1) * (cols + 1) + j;
            uint32_t i2 = i * (cols + 1) + (j + 1);
            uint32_t i3 = (i + 1) * (cols + 1) + (j + 1);

            shape->index[index++] = i0;
            shape->index[index++] = i1;
            shape->index[index++] = i2;

            shape->index[index++] = i1;
            shape->index[index++] = i3;
            shape->index[index++] = i2;
        }
    }
}



void dvz_shape_cylinder(DvzShape* shape, uint32_t count, DvzColor color)
{
    ASSERT(count > 2);
    ANN(shape);
    shape->type = DVZ_SHAPE_CYLINDER;

    const float radius = 0.5f;
    const float half_height = 0.5f;

    const uint32_t vertex_count = 2 * count    // side vertices
                                  + 2          // center top/bottom
                                  + 2 * count; // cap ring vertices
    const uint32_t index_count = 12 * count;

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    uint32_t vi = 0, ii = 0;

    // --- SIDE VERTICES (bottom and top rings)
    for (uint32_t i = 0; i < count; i++)
    {
        float angle = 2.0f * M_PI * i / count;
        float x = cosf(angle), z = sinf(angle);

        // Bottom ring
        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = -half_height;
        shape->pos[vi][2] = radius * z;
        glm_vec3_copy((vec3){x, 0, z}, shape->normal[vi]);
        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = i / (float)count;
        shape->texcoords[vi][1] = 0;
        shape->texcoords[vi][3] = 1;
        vi++;

        // Top ring
        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = +half_height;
        shape->pos[vi][2] = radius * z;
        glm_vec3_copy((vec3){x, 0, z}, shape->normal[vi]);
        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = i / (float)count;
        shape->texcoords[vi][1] = 1;
        shape->texcoords[vi][3] = 1;
        vi++;
    }

    // --- SIDE INDICES
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t i_bot_0 = (2 * i + 0) % (2 * count);
        uint32_t i_top_0 = (2 * i + 1) % (2 * count);
        uint32_t i_bot_1 = (2 * ((i + 1) % count) + 0);
        uint32_t i_top_1 = (2 * ((i + 1) % count) + 1);

        shape->index[ii++] = i_bot_0;
        shape->index[ii++] = i_bot_1;
        shape->index[ii++] = i_top_0;

        shape->index[ii++] = i_top_0;
        shape->index[ii++] = i_bot_1;
        shape->index[ii++] = i_top_1;
    }

    // --- CAP CENTERS
    uint32_t center_bottom = vi++;
    shape->pos[center_bottom][0] = 0;
    shape->pos[center_bottom][1] = -half_height;
    shape->pos[center_bottom][2] = 0;
    glm_vec3_copy((vec3){0, -1, 0}, shape->normal[center_bottom]);
    memcpy(shape->color[center_bottom], color, sizeof(DvzColor));
    shape->texcoords[center_bottom][0] = 0.5f;
    shape->texcoords[center_bottom][1] = 0.5f;
    shape->texcoords[center_bottom][3] = 1;

    uint32_t center_top = vi++;
    shape->pos[center_top][0] = 0;
    shape->pos[center_top][1] = +half_height;
    shape->pos[center_top][2] = 0;
    glm_vec3_copy((vec3){0, +1, 0}, shape->normal[center_top]);
    memcpy(shape->color[center_top], color, sizeof(DvzColor));
    shape->texcoords[center_top][0] = 0.5f;
    shape->texcoords[center_top][1] = 0.5f;
    shape->texcoords[center_top][3] = 1;

    // --- CAP RING VERTICES (flat normals)
    uint32_t base_ring_bottom = vi;
    for (uint32_t i = 0; i < count; i++)
    {
        float angle = 2.0f * M_PI * i / count;
        float x = cosf(angle), z = sinf(angle);
        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = -half_height;
        shape->pos[vi][2] = radius * z;
        glm_vec3_copy((vec3){0, -1, 0}, shape->normal[vi]);
        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = 0.5f + 0.5f * x;
        shape->texcoords[vi][1] = 0.5f + 0.5f * z;
        shape->texcoords[vi][3] = 1;
        vi++;
    }

    uint32_t base_ring_top = vi;
    for (uint32_t i = 0; i < count; i++)
    {
        float angle = 2.0f * M_PI * i / count;
        float x = cosf(angle), z = sinf(angle);
        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = +half_height;
        shape->pos[vi][2] = radius * z;
        glm_vec3_copy((vec3){0, +1, 0}, shape->normal[vi]);
        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = 0.5f + 0.5f * x;
        shape->texcoords[vi][1] = 0.5f + 0.5f * z;
        shape->texcoords[vi][3] = 1;
        vi++;
    }

    // --- CAP INDICES
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t i1 = base_ring_bottom + i;
        uint32_t i2 = base_ring_bottom + (i + 1) % count;
        shape->index[ii++] = center_bottom;
        shape->index[ii++] = i2;
        shape->index[ii++] = i1;

        i1 = base_ring_top + i;
        i2 = base_ring_top + (i + 1) % count;
        shape->index[ii++] = center_top;
        shape->index[ii++] = i1;
        shape->index[ii++] = i2;
    }
}



void dvz_shape_cone(DvzShape* shape, uint32_t count, DvzColor color)
{
    ASSERT(count > 2); // At least 3 segments for a valid cone base
    ANN(shape);
    shape->type = DVZ_SHAPE_CONE;

    const float radius = 0.5f;
    const float half_height = 0.5f;

    const uint32_t verts_side = count + 1; // base ring + apex
    const uint32_t verts_base = count + 1; // base ring + center
    const uint32_t vertex_count = verts_side + verts_base;

    const uint32_t tris_side = count;
    const uint32_t tris_base = count;
    const uint32_t index_count = 3 * (tris_side + tris_base);

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    uint32_t vi = 0;
    uint32_t ii = 0;

    // --- Side vertices (base ring)
    for (uint32_t i = 0; i < count; i++)
    {
        float angle = 2.0f * M_PI * i / count;
        float x = cosf(angle);
        float z = sinf(angle);

        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = -half_height;
        shape->pos[vi][2] = radius * z;

        // Approximate normal using the cone slope
        vec3 n = {x, radius / 1.0f, z}; // dy = height, dx = radius
        glm_vec3_normalize_to(n, shape->normal[vi]);

        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = i / (float)count;
        shape->texcoords[vi][1] = 0;
        shape->texcoords[vi][3] = 1;
        vi++;
    }

    // Apex vertex.
    uint32_t apex_index = vi;
    shape->pos[vi][0] = 0;
    shape->pos[vi][1] = +half_height;
    shape->pos[vi][2] = 0;
    glm_vec3_copy((vec3){0, 1, 0}, shape->normal[vi]);
    memcpy(shape->color[vi], color, sizeof(DvzColor));
    shape->texcoords[vi][0] = 0.5;
    shape->texcoords[vi][1] = 1.0;
    shape->texcoords[vi][3] = 1;
    vi++;

    // Side indices.
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t i0 = i;
        uint32_t i1 = (i + 1) % count;
        shape->index[ii++] = apex_index;
        shape->index[ii++] = i1;
        shape->index[ii++] = i0;
    }

    // --- Base center
    uint32_t base_center_index = vi;
    shape->pos[vi][0] = 0;
    shape->pos[vi][1] = -half_height;
    shape->pos[vi][2] = 0;
    glm_vec3_copy((vec3){0, -1, 0}, shape->normal[vi]);
    memcpy(shape->color[vi], color, sizeof(DvzColor));
    shape->texcoords[vi][0] = 0.5;
    shape->texcoords[vi][1] = 0.5;
    shape->texcoords[vi][3] = 1;
    vi++;

    // Base ring vertices (again, for flat normals)
    for (uint32_t i = 0; i < count; i++)
    {
        float angle = 2.0f * M_PI * i / count;
        float x = cosf(angle);
        float z = sinf(angle);

        shape->pos[vi][0] = radius * x;
        shape->pos[vi][1] = -half_height;
        shape->pos[vi][2] = radius * z;

        glm_vec3_copy((vec3){0, -1, 0}, shape->normal[vi]);
        memcpy(shape->color[vi], color, sizeof(DvzColor));
        shape->texcoords[vi][0] = 0.5f + 0.5f * x;
        shape->texcoords[vi][1] = 0.5f + 0.5f * z;
        shape->texcoords[vi][3] = 1;
        vi++;
    }

    // Base indices.
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t i0 = base_center_index;
        uint32_t i1 = base_center_index + 1 + i;
        uint32_t i2 = base_center_index + 1 + ((i + 1) % count);
        shape->index[ii++] = i0;
        shape->index[ii++] = i1;
        shape->index[ii++] = i2;
    }
}



void dvz_shape_arrow(
    DvzShape* shape, float head_length, float head_radius, float shaft_radius, DvzColor color)
{
    ANN(shape);
    ASSERT(head_length > 0);
    ASSERT(head_radius > 0);
    ASSERT(shaft_radius > 0);
    shape->type = DVZ_SHAPE_ARROW;

    const float total_height = 1.0f;
    ASSERT(head_length < total_height);
    float shaft_length = total_height - head_length;

    // Create shaft.
    DvzShape* shaft = dvz_shape();
    dvz_shape_cylinder(shaft, ARROW_SIDE_COUNT, color);
    dvz_shape_begin(shaft, 0, shaft->vertex_count);
    vec3 scale_shaft = {shaft_radius, shaft_length, shaft_radius};
    dvz_shape_scale(shaft, scale_shaft);
    vec3 trans_shaft = {0, -0.5f + shaft_length / 2.0f, 0};
    dvz_shape_translate(shaft, trans_shaft);
    dvz_shape_end(shaft);

    // Create head.
    DvzShape* head = dvz_shape();
    dvz_shape_cone(head, ARROW_SIDE_COUNT, color);
    dvz_shape_begin(head, 0, head->vertex_count);
    vec3 scale_head = {head_radius, head_length, head_radius};
    dvz_shape_scale(head, scale_head);
    vec3 trans_head = {0, 0.5f - head_length / 2.0f, 0};
    dvz_shape_translate(head, trans_head);
    dvz_shape_end(head);

    // Merge both parts.
    DvzShape* parts[] = {shaft, head};
    dvz_shape_merge(shape, 2, parts);
    dvz_shape_normals(shape);

    dvz_shape_destroy(shaft);
    dvz_shape_destroy(head);
}



void dvz_shape_torus(
    DvzShape* shape, uint32_t count_radial, uint32_t count_tubular, float tube_radius,
    DvzColor color)
{
    ANN(shape);
    ASSERT(count_radial > 2);
    ASSERT(count_tubular > 2);
    ASSERT(tube_radius > 0);

    shape->type = DVZ_SHAPE_TORUS;

    const float R = 0.5f; // Major radius of the torus (center of tube path)
    const float r = tube_radius;

    const uint32_t vertex_count = (count_radial + 1) * (count_tubular + 1);
    const uint32_t index_count = 6 * count_radial * count_tubular;

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));

    uint32_t vi = 0;
    for (uint32_t i = 0; i <= count_radial; i++)
    {
        float u = (float)i / count_radial;
        float theta = u * 2.0f * M_PI;

        float cos_theta = cosf(theta);
        float sin_theta = sinf(theta);

        for (uint32_t j = 0; j <= count_tubular; j++)
        {
            float v = (float)j / count_tubular;
            float phi = v * 2.0f * M_PI;

            float cos_phi = cosf(phi);
            float sin_phi = sinf(phi);

            float x = (R + r * cos_phi) * cos_theta;
            float y = r * sin_phi;
            float z = (R + r * cos_phi) * sin_theta;

            shape->pos[vi][0] = x;
            shape->pos[vi][1] = y;
            shape->pos[vi][2] = z;

            // Normal: from torus center ring to surface
            shape->normal[vi][0] = cos_theta * cos_phi;
            shape->normal[vi][1] = sin_phi;
            shape->normal[vi][2] = sin_theta * cos_phi;

            glm_vec3_normalize(shape->normal[vi]);

            // Texcoords
            shape->texcoords[vi][0] = u;
            shape->texcoords[vi][1] = v;
            shape->texcoords[vi][3] = 1;

            // Color
            memcpy(shape->color[vi], color, sizeof(DvzColor));

            vi++;
        }
    }

    // Indices
    uint32_t ii = 0;
    for (uint32_t i = 0; i < count_radial; i++)
    {
        for (uint32_t j = 0; j < count_tubular; j++)
        {
            uint32_t a = (i * (count_tubular + 1)) + j;
            uint32_t b = ((i + 1) * (count_tubular + 1)) + j;
            uint32_t c = ((i + 1) * (count_tubular + 1)) + (j + 1);
            uint32_t d = (i * (count_tubular + 1)) + (j + 1);

            shape->index[ii++] = a;
            shape->index[ii++] = b;
            shape->index[ii++] = d;

            shape->index[ii++] = b;
            shape->index[ii++] = c;
            shape->index[ii++] = d;
        }
    }
}



/*************************************************************************************************/
/*  Platonic solids                                                                              */
/*************************************************************************************************/

void dvz_shape_tetrahedron(DvzShape* shape, DvzColor color)
{
    ANN(shape);
    shape->type = DVZ_SHAPE_TETRAHEDRON;
    generate_tetrahedron(shape);
    shape->color = dvz_mock_monochrome(shape->vertex_count, color);
}



void dvz_shape_hexahedron(DvzShape* shape, DvzColor color)
{
    ANN(shape);
    shape->type = DVZ_SHAPE_HEXAHEDRON;
    generate_hexahedron(shape);
    shape->color = dvz_mock_monochrome(shape->vertex_count, color);
}



void dvz_shape_octahedron(DvzShape* shape, DvzColor color)
{
    ANN(shape);
    shape->type = DVZ_SHAPE_OCTAHEDRON;
    generate_octahedron(shape);
    shape->color = dvz_mock_monochrome(shape->vertex_count, color);
}



void dvz_shape_dodecahedron(DvzShape* shape, DvzColor color)
{
    ANN(shape);
    shape->type = DVZ_SHAPE_DODECAHEDRON;
    generate_dodecahedron(shape);
    shape->color = dvz_mock_monochrome(shape->vertex_count, color);
}



void dvz_shape_icosahedron(DvzShape* shape, DvzColor color)
{
    ANN(shape);
    shape->type = DVZ_SHAPE_ICOSAHEDRON;
    generate_icosahedron(shape);
    shape->color = dvz_mock_monochrome(shape->vertex_count, color);
}
