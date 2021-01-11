#include "mesh_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>



/*************************************************************************************************/
/*  Tiny Obj loader                                                                              */
/*************************************************************************************************/

VklMesh vkl_mesh_obj(const char* file_path)
{
    log_trace("loading file %s", file_path);
    VklMesh mesh = vkl_mesh();

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
    uint32_t s, f, fv, v;
    for (s = 0; s < ns; s++)
    {                                                                 // loop over shapes
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
    log_debug("loaded %s: %d shape(s), %d vertices, %d indices", ns, nv, ni);

    // Create the mesh object and allocate the vertex and index arrays.
    vkl_array_resize(&mesh.vertices, nv);
    vkl_array_resize(&mesh.indices, ni);

    // Pointers to the current vertex and index.
    VklGraphicsMeshVertex* vertex = (VklGraphicsMeshVertex*)mesh.vertices.data;
    VklIndex* index = (VklIndex*)mesh.indices.data;

    ASSERT(vertex != NULL);
    ASSERT(index != NULL);

    // Loop over shapes
    uint32_t i, index_offset, nf;
    tinyobj::index_t idx;
    for (s = 0; s < ns; s++)
    {
        // Vertices
        for (i = 0; i < nv; i++)
        {
            // Vertex position.
            vertex->pos[0] = attrib.vertices[3 * i + 0];
            vertex->pos[1] = attrib.vertices[3 * i + 1];
            vertex->pos[2] = attrib.vertices[3 * i + 2];

            // Vertex normal.
            vertex->normal[0] = attrib.normals[3 * i + 0];
            vertex->normal[1] = attrib.normals[3 * i + 1];
            vertex->normal[2] = attrib.normals[3 * i + 2];

            // Vertex tex coords.
            vertex->uv[0] = attrib.texcoords[2 * i + 0];
            vertex->uv[1] = attrib.texcoords[2 * i + 1];

            // Go to next vertex.
            vertex++;
        }

        // Faces.
        index_offset = 0;
        nf = shapes[s].mesh.num_face_vertices.size(); // number of faces in the current shape
        for (f = 0; f < nf; f++)                      // loop over faces
        {
            fv = shapes[s].mesh.num_face_vertices[f]; // number of vertices in the face

            // HACK: skip non-triangular faces for now (need to be triangulated)
            if (fv != 3)
                continue;

            // Indices in each face.
            for (v = 0; v < fv; v++)
            {
                idx = shapes[s].mesh.indices[index_offset + v];
                *index = (VklIndex)idx.vertex_index;
                index++;
            }
            index_offset += fv;
        }
    }

    // TODO: mesh normalization

    return mesh;
}
