#include <visky/mesh_loader.h>
#include <visky/visuals.h>

#if HAS_ASSIMP
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


/*************************************************************************************************/
/*  ASSIMP loader                                                                                */
/*************************************************************************************************/

void vky_mesh_assimp(VkyMesh* mesh, const char* file_path)
{

#if HAS_ASSIMP
    log_trace("loading file %s", file_path);

    const struct aiScene* scene = aiImportFile(
        file_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                       aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (!scene)
    {
        log_error("file %s not found.", file_path);
        return;
    }

    const struct aiNode* node = scene->mRootNode;
    for (uint32_t ch = 0; ch < node->mNumChildren; ch++)
    {
        node = node->mChildren[ch];
        ASSERT(node->mNumMeshes > 0);
        for (uint32_t m = 0; m < node->mNumMeshes; m++)
        {
            struct aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[m]];

            // Get vertices and indices pointers into the mesh arrays.
            uint32_t first_vertex;
            VkyMeshVertex *vertex = NULL, *vertex_orig = NULL;
            VkyIndex* index = NULL;
            vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
            vertex_orig = vertex;
            uint32_t nv;
            ASSERT(vertex != NULL);
            ASSERT(index != NULL);

            nv = ai_mesh->mNumVertices;
            ASSERT(nv > 0);
            log_debug("loading mesh with %d vertices", nv);

            // vertices
            for (uint32_t i = 0; i < ai_mesh->mNumVertices; i++)
            {
                vertex->pos[0] = ai_mesh->mVertices[i].x;
                vertex->pos[1] = ai_mesh->mVertices[i].y;
                vertex->pos[2] = ai_mesh->mVertices[i].z;

                vertex->normal[0] = ai_mesh->mNormals[i].x;
                vertex->normal[1] = ai_mesh->mNormals[i].y;
                vertex->normal[2] = ai_mesh->mNormals[i].z;

                // TODO: color
                vertex->color.rgb[0] = 255;
                vertex->color.rgb[1] = 255;
                vertex->color.rgb[2] = 255;
                vertex->color.alpha = 255;

                vertex++;
            }

            // faces
            uint32_t k = 0;
            for (uint32_t i = 0; i < ai_mesh->mNumFaces; i++)
            {
                struct aiFace face = ai_mesh->mFaces[i];
                for (uint32_t j = 0; j < face.mNumIndices; j++)
                {
                    *index = face.mIndices[j];
                    k++;
                    index++;
                }
            }
            vky_mesh_end(mesh, nv, k);
            vky_normalize_mesh(nv, vertex_orig);
        }
    }

    aiReleaseImport(scene);
#else

    log_error("ASSIMP not available");

#endif
}



/*************************************************************************************************/
/*  Tiny Obj loader                                                                              */
/*************************************************************************************************/

void vky_mesh_obj(VkyMesh* mesh, const char* file_path)
{

    log_trace("loading file %s", file_path);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    // Get vertices and indices pointers into the mesh arrays.
    uint32_t first_vertex;
    VkyMeshVertex *vertex = NULL, *vertex_orig = NULL;
    VkyIndex* index = NULL;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path);
    if (!warn.empty())
        log_warn(warn.c_str());
    if (!err.empty())
        log_warn(err.c_str());
    if (!ret)
        return;

    // Loop over shapes
    uint32_t nv;
    for (size_t s = 0; s < shapes.size(); s++)
    {

        vky_mesh_begin(mesh, &first_vertex, &vertex, &index);
        vertex_orig = vertex;
        ASSERT(vertex != NULL);
        ASSERT(index != NULL);

        ASSERT(attrib.vertices.size() % 3 == 0);
        nv = attrib.vertices.size() / 3;

        log_debug(
            "shape %d, %d vertices, %d faces", s, nv, shapes[s].mesh.num_face_vertices.size());

        // Vertices
        for (uint32_t i = 0; i < nv; i++)
        {
            vertex->pos[0] = attrib.vertices[3 * i + 0];
            vertex->pos[1] = attrib.vertices[3 * i + 1];
            vertex->pos[2] = attrib.vertices[3 * i + 2];

            vertex->normal[0] = attrib.normals[3 * i + 0];
            vertex->normal[1] = attrib.normals[3 * i + 1];
            vertex->normal[2] = attrib.normals[3 * i + 2];

            vertex->color.rgb[0] = TO_BYTE(attrib.colors[3 * i + 0]);
            vertex->color.rgb[1] = TO_BYTE(attrib.colors[3 * i + 1]);
            vertex->color.rgb[2] = TO_BYTE(attrib.colors[3 * i + 2]);
            vertex->color.alpha = 255;

            vertex++;
        }

        // Faces.
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = shapes[s].mesh.num_face_vertices[f];

            // Points in each face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                *index = (VkyIndex)idx.vertex_index;
                index++;
            }
            index_offset += fv;
        }

        vky_mesh_end(mesh, nv, index_offset);
        vky_normalize_mesh(nv, vertex_orig);
    }
}
