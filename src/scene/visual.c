/*************************************************************************************************/
/*  Visual                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visual.h"
#include "fileio.h"
#include "request.h"
#include "scene/baker.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual*
dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, uint32_t item_count, int flags)
{
    ANN(rqr);
    ASSERT(item_count > 0);

    DvzVisual* visual = (DvzVisual*)calloc(1, sizeof(DvzVisual));

    visual->flags = flags;
    visual->rqr = rqr;

    visual->baker = dvz_baker(rqr, 0);

    // Create the graphics object.
    DvzRequest req = dvz_create_graphics(rqr, DVZ_GRAPHICS_CUSTOM, 0);
    visual->graphics_id = req.id;

    // Default fixed function pipeline states:

    // Primitive topology.
    dvz_set_primitive(rqr, visual->graphics_id, primitive);

    // Polygon mode.
    dvz_set_polygon(rqr, visual->graphics_id, DVZ_POLYGON_MODE_FILL);

    dvz_obj_init(&visual->obj);
    return visual;
}



void dvz_visual_spirv(
    DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer)
{
    ANN(visual);
    ANN(buffer);
    ASSERT(size > 0);

    dvz_set_spirv(visual->rqr, visual->graphics_id, type, size, buffer);
}



void dvz_visual_shader(DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    ASSERT(strlen(name) < 50);

    unsigned long size = 0;
    char rname[64] = {0};

    // Vertex shader.
    snprintf(rname, 60, "%s_%s", name, "vert");
    unsigned char* buffer = dvz_resource_shader(rname, &size);
    dvz_set_spirv(visual->rqr, visual->graphics_id, DVZ_SHADER_VERTEX, size, buffer);

    memset(rname, 0, 64);

    // Fragment shader.
    snprintf(rname, 60, "%s_%s", name, "frag");
    buffer = dvz_resource_shader(rname, &size);
    dvz_set_spirv(visual->rqr, visual->graphics_id, DVZ_SHADER_FRAGMENT, size, buffer);
}



void dvz_visual_count(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    ASSERT(item_count > 0);

    visual->item_count = item_count;
    // TODO: resize the baker, emit the dat resize commands, resize the underlying arrays
}



void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes)
{
    ANN(visual);
    ASSERT(group_count > 0);

    // Allocate the group sizes.
    if (visual->group_sizes == NULL)
    {
        visual->group_sizes = (uint32_t*)calloc(group_count, sizeof(uint32_t));
    }

    // Ensure the group_sizes array is large enough to hold all group sizes.
    if (group_count > visual->group_count)
    {
        REALLOC(visual->group_sizes, group_count * sizeof(uint32_t));
    }
    ASSERT(visual->group_count >= group_count);

    // Copy the group sizes.
    memcpy(visual->group_sizes, group_sizes, group_count * sizeof(uint32_t));
    visual->group_count = group_count;
}



void dvz_visual_attr(DvzVisual* visual, uint32_t attr_idx, DvzFormat format, int flags)
{
    ANN(visual);
    ASSERT(attr_idx < DVZ_MAX_VERTEX_ATTRS);

    // NOTE: lazy spec of vertex bindings and attrs, as this will depend on all attrs.
    // Will be done at create time. Will have to do baker side and GPU request side.
    visual->attrs[attr_idx].format = format;
    visual->attrs[attr_idx].flags = flags;
}



void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzSize size)
{
    ANN(visual);
    ANN(visual->baker);

    // CPU-side: baker.
    dvz_baker_slot(visual->baker, slot_idx, size);

    // GPU-size: slot request.
    dvz_set_slot(visual->rqr, visual->graphics_id, slot_idx, DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}



void dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzTexDims dims, int flags)
{
    ANN(visual);
    ANN(visual->baker);

    // CPU-side: baker.
    // TODO: baker tex slot?
    dvz_baker_slot(visual->baker, slot_idx, 0);

    // GPU-size: slot request.
    dvz_set_slot(visual->rqr, visual->graphics_id, slot_idx, DVZ_DESCRIPTOR_TYPE_SAMPLER);
}



void dvz_visual_create(DvzVisual* visual)
{
    ANN(visual);

    // TODO

    /*
    vertex bindings attributions is done automatically as a function of the flags

    // NOTE: check that props of a given vertex binding idx are either all constant or all not
    constant

    // Determine the vertex bindings as a function of the flags.
    // Assume the highest binding_idx of all props +1 is the number of different bindings
    // Array with the stride of each attribute
        uint32_t binding_count;
        DvzSize strides[DVZ_PROP_MAX_ATTRS]; // for each GLSL attr, the number of bytes per
                                                item
        DvzSize offsets[DVZ_PROP_MAX_BINDINGS]; // for each binding, the offset of the last
                                                    visited prop
        for each prop
            if (binding_count == 0 || binding >= binding_count) // new binding
                dvz_set_vertex() // declare new binding
            dvz_set_attr() // manually called for each prop, with the known GLSL location, and the
                            binding stored in the corresponding DvzProp*

    attrs
        set up the baker, which will create the dats
        also emit the associated graphics commands
        also emit the binding commands for the vertex, index
    descriptors
        set up the baker, which will create the dat
        emit the dat binding


    // Finish setting up the baker.
    dvz_baker_vertex(baker, 0, sizeof(DvzPixelVertex));
    dvz_baker_attr(baker, 0, 0, offsetof(DvzPixelVertex, pos), sizeof(vec3));
    dvz_baker_attr(baker, 1, 0, offsetof(DvzPixelVertex, color), sizeof(cvec4));

    // Create the baker, which will create the arrays and dats.
    dvz_baker_create(baker, 5000); // DEBUG

    // Send the vertex binding commands.
    dvz_set_vertex(rqr, graphics_id, 0, sizeof(DvzPixelVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Send the vertex attribute commands.
    dvz_set_attr(
        rqr, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(DvzPixelVertex, pos));
    dvz_set_attr(
        rqr, graphics_id, 0, 1, DVZ_FORMAT_R8G8B8A8_UNORM, offsetof(DvzPixelVertex, color));

    // Send the vertex binding commands.
    dvz_bind_vertex(rqr, graphics_id, 0, baker->vertex_bindings[0].dual.dat, 0);

    // Send the dat bindings commands.
    dvz_bind_dat(rqr, graphics_id, 0, baker->descriptors[0].dual.dat);

    DvzFormat format = visual->attrs[attr_idx].format;

    */
}



void dvz_visual_mvp(DvzVisual* visual, DvzMVP mvp)
{
    ANN(visual);
    ANN(visual->baker);

    visual->mvp = mvp;

    dvz_baker_uniform(visual->baker, 0, sizeof(DvzMVP), &visual->mvp);
    dvz_baker_update(visual->baker);
}



void dvz_visual_viewport(DvzVisual* visual, DvzViewport viewport)
{
    ANN(visual);
    ANN(visual->baker);

    visual->viewport = viewport;

    dvz_baker_uniform(visual->baker, 0, sizeof(DvzViewport), &visual->viewport);
    dvz_baker_update(visual->baker);
}



void dvz_visual_data(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data)
{
    ANN(visual);
    ASSERT(attr_idx < DVZ_MAX_VERTEX_ATTRS);

    DvzBaker* baker = visual->baker;
    ANN(baker);

    int flags = visual->attrs[attr_idx].flags;

    // Quad.
    if ((flags & DVZ_ATTR_FLAGS_QUAD) != 0)
    {
        log_warn("please use dvz_visual_quads() instead");
        return;
    }

    // Repeats.
    else if ((flags & DVZ_ATTR_FLAGS_REPEAT) != 0)
    {
        // Extract the N in 0xN00 part, that is the number of repeats.
        int reps = (flags & 0x0F00) >> 8;
        ASSERT(reps >= 1);

        log_trace("visual data for attr #%d (%d:%d), repeat %d", attr_idx, first, count, reps);
        dvz_baker_repeat(baker, attr_idx, first, count, (uint32_t)reps, data);
    }

    // Direct copy.
    else
    {
        log_trace("visual data for attr #%d (%d:%d)", attr_idx, first, count);
        dvz_baker_data(baker, attr_idx, first, count, data);
    }
}



void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, //
    vec2 quad_size, vec2* positions)
{
    ANN(visual);
    ASSERT(attr_idx < DVZ_MAX_VERTEX_ATTRS);

    DvzBaker* baker = visual->baker;
    ANN(baker);

    int flags = visual->attrs[attr_idx].flags;
    ASSERT((flags & DVZ_ATTR_FLAGS_QUAD) != 0);

    dvz_baker_quads(baker, attr_idx, quad_size, count, positions);
}



void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t count, uint32_t first_instance,
    uint32_t instance_count)
{
    ANN(visual);

    bool indexed = (visual->flags & DVZ_VISUALS_FLAGS_INDEXED) != 0;
    bool indirect = (visual->flags & DVZ_VISUALS_FLAGS_INDIRECT) != 0;

    if (!indexed && !indirect)
    {
        dvz_record_draw(
            visual->rqr, canvas, visual->graphics_id, first, count, first_instance,
            instance_count);
    }
    else if (indexed && !indirect)
    {
        uint32_t vertex_offset = 0; // TODO
        dvz_record_draw_indexed(
            visual->rqr, canvas, visual->graphics_id, first, vertex_offset, count, first_instance,
            instance_count);
    }
    else if (!indexed && indirect)
    {
        uint32_t draw_count = 1; // TODO
        dvz_record_draw_indirect(visual->rqr, canvas, visual->graphics_id, indirect, draw_count);
    }
    else if (indexed && indirect)
    {
        uint32_t draw_count = 1; // TODO
        dvz_record_draw_indexed_indirect(
            visual->rqr, canvas, visual->graphics_id, indirect, draw_count);
    }
}



void dvz_visual_draw(DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t count)
{
    dvz_visual_instance(visual, canvas, first, count, 0, 1);
}



void dvz_visual_destroy(DvzVisual* visual)
{
    ANN(visual);
    if (visual->group_sizes != NULL)
    {
        FREE(visual->group_sizes);
    }
}


// helper functions that creates the mvp/viewport dat
