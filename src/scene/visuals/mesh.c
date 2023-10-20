/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/mesh.h"
#include "_map.h"
#include "fileio.h"
#include "request.h"
#include "scene/graphics.h"
#include "scene/scene.h"
#include "scene/shape.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);
    // NOTE: here, count is item_count, so index_count/3 (number of faces).
    // We need to multiply by three to retrieve the number of elements to draw using
    // indexing.
    dvz_visual_instance(visual, canvas, first, 0, 3 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_mesh(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Parse flags.
    int textured = (flags & DVZ_MESH_FLAGS_TEXTURED);
    int lighting = (flags & DVZ_MESH_FLAGS_LIGHTING);
    log_trace("create mesh visual, texture: %d, lighting: %d", textured, lighting);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_mesh");

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Specialization constants.
    dvz_visual_specialization(visual, DVZ_SHADER_VERTEX, 0, sizeof(int), &textured);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int), &textured);

    // Textured vertex.
    if (textured)
    {
        // Vertex attributes.
        dvz_visual_attr( //
            visual, 0, FIELD(DvzMeshTexturedVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 1, FIELD(DvzMeshTexturedVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 2, FIELD(DvzMeshTexturedVertex, uva), DVZ_FORMAT_R32G32B32A32_SFLOAT, 0);

        // Vertex stride.
        dvz_visual_stride(visual, 0, sizeof(DvzMeshTexturedVertex));
    }
    // Color vertex.
    else
    {
        // Vertex attributes.
        dvz_visual_attr( //
            visual, 0, FIELD(DvzMeshColorVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 1, FIELD(DvzMeshColorVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 2, FIELD(DvzMeshColorVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

        // Vertex stride.
        dvz_visual_stride(visual, 0, sizeof(DvzMeshColorVertex));
    }

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzMeshParams));

    dvz_params_attr(params, 0, FIELD(DvzMeshParams, light_pos));
    dvz_params_attr(params, 1, FIELD(DvzMeshParams, light_params));

    dvz_visual_tex(
        visual, 3, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID, DVZ_ZERO_OFFSET);

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_mesh_alloc(DvzVisual* visual, uint32_t vertex_count, uint32_t index_count)
{
    ANN(visual);
    ASSERT(vertex_count > 0);
    ASSERT(index_count % 3 == 0);
    log_debug("allocating the mesh visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.

    // NOTE: by convention in this visual, 1 item = 1 triangle.
    // This is why item_count is index_count / 3 below.
    dvz_visual_alloc(visual, index_count / 3, vertex_count, index_count);
}



void dvz_mesh_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* values)
{
    ANN(visual);
    dvz_visual_index(visual, first, count, values);
}



void dvz_mesh_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_mesh_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_mesh_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    if (visual->flags & DVZ_MESH_FLAGS_TEXTURED)
    {
        log_error("cannot use dvz_mesh_color() with a textured mesh");
        return;
    }
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_mesh_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags)
{
    ANN(visual);
    if (!(visual->flags & DVZ_MESH_FLAGS_TEXTURED))
    {
        log_error("cannot use dvz_mesh_texcoords() with a color mesh");
        return;
    }
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



DvzId dvz_mesh_texture(
    DvzVisual* visual, uvec3 shape, DvzFormat format, DvzFilter filter, DvzSize size, void* data)
{
    ANN(visual);

    if (!(visual->flags & DVZ_MESH_FLAGS_TEXTURED))
    {
        log_error("the mesh visual needs to be created with the DVZ_MESH_FLAGS_TEXTURED flag");
        return DVZ_ID_NONE;
    }

    DvzBatch* batch = visual->batch;
    ANN(batch);

    DvzId tex = dvz_create_tex(batch, DVZ_TEX_2D, format, shape, 0).id;
    DvzId sampler = dvz_create_sampler(batch, filter, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 3, tex, sampler, DVZ_ZERO_OFFSET);

    // Upload the texture data.
    if (size > 0 && data != NULL)
        dvz_upload_tex(batch, tex, DVZ_ZERO_OFFSET, shape, size, data);

    return tex;
}



void dvz_mesh_light_pos(DvzVisual* visual, vec4 pos)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, pos);
}



void dvz_mesh_light_params(DvzVisual* visual, vec4 params)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, params);
}



DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags)
{
    ANN(shape);
    ANN(shape->pos);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    // NOTE: set the visual flag to indexed or non-indexed (default) depending on whether the shape
    // has an index buffer or not.
    flags |= (index_count > 0 ? DVZ_VISUALS_FLAGS_INDEXED : DVZ_VISUALS_FLAGS_DEFAULT);
    DvzVisual* visual = dvz_mesh(batch, flags);

    dvz_mesh_alloc(visual, vertex_count, index_count);

    dvz_mesh_position(visual, 0, vertex_count, shape->pos, 0);

    if (shape->normal)
        dvz_mesh_normal(visual, 0, vertex_count, shape->normal, 0);

    if (shape->color && !(flags & DVZ_MESH_FLAGS_TEXTURED))
        dvz_mesh_color(visual, 0, vertex_count, shape->color, 0);

    if (shape->texcoords && (flags & DVZ_MESH_FLAGS_TEXTURED))
        dvz_mesh_texcoords(visual, 0, vertex_count, shape->texcoords, 0);

    if (shape->index_count > 0)
        dvz_mesh_index(visual, 0, index_count, shape->index);

    return visual;
}
