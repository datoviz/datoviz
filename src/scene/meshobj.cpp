/*************************************************************************************************/
/*  MeshObj                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "_macros.h"
#include "datoviz.h"

// #define MUTE_MISSING_PROTOTYPES _Pragma("GCC diagnostic ignored \"-Wmissing-prototypes\"")

MUTE_ON
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
MUTE_OFF



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  MeshObj functions                                                                            */
/*************************************************************************************************/

void dvz_shape_normalize(DvzShape* shape)
{
    ANN(shape);

    const float INF = 1000000;
    vec3 min = {+INF, +INF, +INF}, max = {-INF, -INF, -INF};
    vec3 center = {0};
    vec3 pos = {0};

    uint32_t nv = shape->vertex_count;
    for (uint32_t i = 0; i < nv; i++)
    {
        _vec3_copy(shape->pos[i], pos);
        glm_vec3_minv(min, pos, min);
        glm_vec3_maxv(max, pos, max);

        glm_vec3_add(center, pos, center);
    }
    glm_vec3_scale(center, 1. / nv, center);

    // a * (pos - center) \in (-1, 1)
    // a * (min - center)
    // a = min(1/(max-center), 1/(center-xmin))
    vec3 u = {0}, v = {0};
    vec3 ones = {1, 1, 1};
    glm_vec3_sub(max, center, u);
    glm_vec3_sub(center, min, v);
    glm_vec3_div(ones, u, u);
    glm_vec3_div(ones, v, v);
    // for (uint32_t k = 0; k < 3; k++)
    // {
    //     u[k] = 1 / u[k];
    //     v[k] = 1 / v[k];
    // }
    float a = fmin(glm_vec3_min(u), glm_vec3_min(v));
    ASSERT(a > 0);

    for (uint32_t i = 0; i < nv; i++)
    {
        glm_vec3_sub(shape->pos[i], center, shape->pos[i]);
        glm_vec3_scale(shape->pos[i], a, shape->pos[i]);
    }
}



void dvz_shape_obj(DvzShape* shape, const char* file_path)
{
    ANN(file_path);
    ANN(shape);

    shape->type = DVZ_SHAPE_OBJ;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    // Load the file.
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path);
    if (!warn.empty())
        log_warn(warn.c_str());
    if (!err.empty())
        log_warn(err.c_str());
    if (!ret)
    {
        log_error("error loading obj file %s", file_path);
        return;
    }

    // Number of vertices.
    uint32_t nv = attrib.vertices.size() / 3; // 3 floats per vertex
    ASSERT(nv > 0);

    // Number of indices.
    uint32_t ni = 0;

    // Number of shapes in the OBJ file.
    uint32_t ns = shapes.size();

    // Count the number of indices.
    // TODO: do not count in a first pass, use C++ resizable containers directly.
    uint32_t s, f, fv, v;
    for (s = 0; s < ns; s++) // loop over shapes
    {
        for (f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) // loop over faces
        {
            fv = shapes[s].mesh.num_face_vertices[f]; // number of indices per face
            // NOTE: Only triangle faces for now.
            if (fv != 3)
            {
                log_error("non-triangular OBJ faces (%d vertices) not yet implemented", fv);
                continue;
            }
            ni += fv;
        }
    }
    ASSERT(ni > 0);
    log_debug("loading shape %d: %d shape(s), %d vertices, %d indices", s, ns, nv, ni);

    // Create the DvzShape structure.

    uint32_t vertex_count = nv;
    uint32_t index_count = ni;

    shape->vertex_count = vertex_count;
    shape->index_count = index_count;

    shape->pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    shape->color = (DvzColor*)calloc(vertex_count, sizeof(DvzColor));
    shape->texcoords = (vec4*)calloc(vertex_count, sizeof(vec4));
    shape->index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));

    // Loop over shapes
    uint32_t i, index_offset, nf;
    tinyobj::index_t idx;

    // Vertices
    log_debug("loading %d vertices", nv);
    size_t nv_obj = attrib.vertices.size();
    size_t nn_obj = attrib.normals.size();
    size_t nt_obj = attrib.texcoords.size();
    size_t nc_obj = attrib.colors.size();

    for (i = 0; i < nv; i++)
    {
        // Vertex position.
        ASSERT(3 * i + 2 < nv_obj);
        shape->pos[i][0] = attrib.vertices[3 * i + 0];
        shape->pos[i][1] = attrib.vertices[3 * i + 1];
        shape->pos[i][2] = attrib.vertices[3 * i + 2];

        // Vertex normal.
        ASSERT(3 * i + 2 < nn_obj);
        shape->normal[i][0] = attrib.normals[3 * i + 0];
        shape->normal[i][1] = attrib.normals[3 * i + 1];
        shape->normal[i][2] = attrib.normals[3 * i + 2];

        // Vertex tex coords.
        if (nt_obj > 0)
        {
            ASSERT(2 * i + 1 < nt_obj);
            shape->texcoords[i][0] = attrib.texcoords[2 * i + 0];
            shape->texcoords[i][1] = attrib.texcoords[2 * i + 1];
        }
        else if (nc_obj > 0)
        {
            shape->color[i][0] = TO_BYTE(attrib.colors[3 * i + 0]);
            shape->color[i][1] = TO_BYTE(attrib.colors[3 * i + 1]);
            shape->color[i][2] = TO_BYTE(attrib.colors[3 * i + 2]);
            shape->color[i][3] = 255;
        }
    }

    // Shapes.
    i = 0;
    for (s = 0; s < ns; s++)
    {
        // Faces.
        index_offset = 0;
        nf = shapes[s].mesh.num_face_vertices.size(); // number of faces in the current shape
        log_debug("shape #%d: loading %d indices", s, nf);

        for (f = 0; f < nf; f++) // loop over faces
        {
            fv = shapes[s].mesh.num_face_vertices[f]; // number of vertices in the face

            // HACK: skip non-triangular faces for now (need to be triangulated)
            if (fv != 3)
                continue;

            // Indices in each face.
            for (v = 0; v < fv; v++)
            {
                idx = shapes[s].mesh.indices[index_offset + v];
                shape->index[i++] = (DvzIndex)idx.vertex_index;
            }
            index_offset += fv;
        }
    }
    ASSERT(i == ni);

    dvz_shape_normalize(shape);
}
