#include "../include/datoviz/mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>



/*************************************************************************************************/
/*  Tiny Obj loader                                                                              */
/*************************************************************************************************/

DvzMesh dvz_mesh_obj(const char* file_path)
{
    log_trace("loading file %s", file_path);
    DvzMesh mesh = dvz_mesh();

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
        return mesh;
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

    // Create the mesh object and allocate the vertex and index arrays.
    dvz_array_resize(&mesh.vertices, nv);
    dvz_array_resize(&mesh.indices, ni);

    // Pointers to the current vertex and index.
    DvzGraphicsMeshVertex* vertex = (DvzGraphicsMeshVertex*)mesh.vertices.data;
    DvzIndex* index = (DvzIndex*)mesh.indices.data;

    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    // Loop over shapes
    uint32_t i, index_offset, nf;
    tinyobj::index_t idx;
    cvec3 color;

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
        vertex->pos[0] = attrib.vertices[3 * i + 0];
        vertex->pos[1] = attrib.vertices[3 * i + 1];
        vertex->pos[2] = attrib.vertices[3 * i + 2];

        // Vertex normal.
        ASSERT(3 * i + 2 < nn_obj);
        vertex->normal[0] = attrib.normals[3 * i + 0];
        vertex->normal[1] = attrib.normals[3 * i + 1];
        vertex->normal[2] = attrib.normals[3 * i + 2];

        // Vertex tex coords.
        if (nt_obj > 0)
        {
            ASSERT(2 * i + 1 < nt_obj);
            vertex->uv[0] = attrib.texcoords[2 * i + 0];
            vertex->uv[1] = attrib.texcoords[2 * i + 1];
        }
        else if (nc_obj > 0)
        {
            color[0] = TO_BYTE(attrib.colors[3 * i + 0]);
            color[1] = TO_BYTE(attrib.colors[3 * i + 1]);
            color[2] = TO_BYTE(attrib.colors[3 * i + 2]);
            dvz_colormap_packuv(color, vertex->uv);
        }

        // Alpha value.
        vertex->alpha = 255;

        // Go to next vertex.
        vertex++;
    }
    ASSERT(
        (int64_t)vertex - (int64_t)mesh.vertices.data ==
        (int64_t)(nv * sizeof(DvzGraphicsMeshVertex)));

    // Shapes.
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
                *index = (DvzIndex)idx.vertex_index;
                index++;
            }
            index_offset += fv;
        }
    }
    ASSERT((int64_t)index - (int64_t)mesh.indices.data == (int64_t)(ni * sizeof(DvzIndex)));

    // Mesh normalization.
    dvz_mesh_normalize(&mesh);

    return mesh;
}
