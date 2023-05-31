/*************************************************************************************************/
/*  Visual                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visual.h"
#include "_map.h"
#include "fileio.h"
#include "request.h"
#include "scene/baker.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static DvzSize get_attr_size(DvzFormat format)
{
    switch (format)
    {
    case DVZ_FORMAT_R32G32B32_SFLOAT:
        return sizeof(vec3);
    case DVZ_FORMAT_R8G8B8A8_UNORM:
        return sizeof(cvec4);
        // TODO: other formats
    default:
        log_error("DvzFormat %d has not yet been implemented in get_attr_size()", format);
        return 0;
    }
    return 0;
}



/*************************************************************************************************/
/*  Visual lifecycle                                                                             */
/*************************************************************************************************/

DvzVisual* dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, int flags)
{
    ANN(rqr);

    DvzVisual* visual = (DvzVisual*)calloc(1, sizeof(DvzVisual));

    visual->flags = flags;
    visual->rqr = rqr;

    // No callback by default, will just use dvz_visual_instance().
    visual->callback = NULL;

    visual->baker = dvz_baker(rqr, 0);

    // Create the graphics object.
    DvzRequest req = dvz_create_graphics(rqr, DVZ_GRAPHICS_CUSTOM, 0);
    visual->graphics_id = req.id;
    visual->is_visible = true;

    // Default fixed function pipeline states:

    // Primitive topology.
    dvz_set_primitive(rqr, visual->graphics_id, primitive);

    // Polygon mode.
    dvz_set_polygon(rqr, visual->graphics_id, DVZ_POLYGON_MODE_FILL);

    dvz_obj_init(&visual->obj);
    return visual;
}



void dvz_visual_update(DvzVisual* visual)
{
    ANN(visual);
    dvz_baker_update(visual->baker);
}



void dvz_visual_destroy(DvzVisual* visual)
{
    ANN(visual);
    if (visual->group_sizes != NULL)
    {
        FREE(visual->group_sizes);
    }
    FREE(visual);
}



/*************************************************************************************************/
/*  Visual declaration                                                                           */
/*************************************************************************************************/

void dvz_visual_spirv(
    DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer)
{
    ANN(visual);
    ANN(buffer);
    ASSERT(size > 0);

    // First, create a shader object.
    DvzRequest req = dvz_create_spirv(visual->rqr, type, size, buffer);

    // Then associate it to the graphics.
    dvz_set_shader(visual->rqr, visual->graphics_id, req.id);
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
    dvz_visual_spirv(visual, DVZ_SHADER_VERTEX, size, buffer);

    memset(rname, 0, 64);

    // Fragment shader.
    snprintf(rname, 60, "%s_%s", name, "frag");
    buffer = dvz_resource_shader(rname, &size);
    dvz_visual_spirv(visual, DVZ_SHADER_FRAGMENT, size, buffer);
}



void dvz_visual_resize(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count)
{
    ANN(visual);
    ASSERT(item_count > 0);

    // Mark the new item count.
    visual->item_count = item_count;
    visual->vertex_count = vertex_count;

    // Resize the baker, resize the underlying arrays, emit the dat resize commands.
    // TODO: write tests, NOT TESTED YET
    dvz_baker_resize(visual->baker, vertex_count);

    // TODO: resize the groups?
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

    // NOTE: the last significant 4 bits of the flags encode the binding_idx. It's 0 for all flags,
    // except DVZ_ATTR_FLAGS_DYNAMIC where it is 1.
    visual->attrs[attr_idx].binding_idx = (uint32_t)(flags & 0x000F);
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



void dvz_visual_create(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count)
{
    ANN(visual);

    DvzBaker* baker = visual->baker;
    ANN(baker);

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    DvzId graphics_id = visual->graphics_id;
    ASSERT(graphics_id != DVZ_ID_NONE);

    // Compute the offsets of each attribute within their vertex bindings, and the vertex bindings
    // strides.
    DvzSize attr_offsets[DVZ_MAX_VERTEX_BINDINGS] = {0};
    DvzVisualAttr* attr = NULL;
    uint32_t binding_idx = 0;
    uint32_t attr_count = 0;
    uint32_t binding_count = 0;
    for (uint32_t attr_idx = 0; attr_idx < DVZ_MAX_VERTEX_ATTRS; attr_idx++)
    {
        attr = &visual->attrs[attr_idx];
        ANN(attr);

        if (attr->format == DVZ_FORMAT_NONE)
        {
            break;
        }

        // The vertex binding of the current vertex attribute is directly encoded in the flags.
        // It was set in dvz_visual_attr();
        binding_idx = attr->binding_idx;

        // Count the number of vertex bindings.
        // NOTE: we assume that the number of vertex bindings is the maximum binding idx + 1.
        binding_count = MAX(binding_idx + 1, binding_count);

        ASSERT(binding_count <= DVZ_MAX_VERTEX_BINDINGS);

        // Compute the offset and item_size of each attribute.
        attr->offset = attr_offsets[binding_idx];

        // The attribute size depends on its Vulkan format.
        attr->item_size = get_attr_size(attr->format);
        ASSERT(attr->item_size > 0);

        // Keep track of the current offset within each vertex binding.
        attr_offsets[binding_idx] += attr->item_size;

        // Count the number of attributes.
        attr_count++;

        ASSERT(attr_count <= DVZ_MAX_VERTEX_ATTRS);
    }

    log_debug("found %d vertex attributes and %d vertex bindings", attr_count, binding_count);
    ASSERT(attr_count < DVZ_MAX_VERTEX_ATTRS);
    ASSERT(binding_count < DVZ_MAX_VERTEX_BINDINGS);

    // Declare the vertex bindings.
    DvzSize stride = 0;
    for (binding_idx = 0; binding_idx < binding_count; binding_idx++)
    {
        stride = attr_offsets[binding_idx];
        ASSERT(stride > 0);

        // Baker-side.
        dvz_baker_vertex(baker, binding_idx, stride);

        // GPU-side.

        // WARNING TODO NOTE: we ASSUME here that the stride (sum of all attribute sizes)
        // matches the total size of the struct, which is not guaranteed because of alignment
        // issues. To check later!

        // TODO: input rate instance?
        dvz_set_vertex(rqr, graphics_id, binding_idx, stride, DVZ_VERTEX_INPUT_RATE_VERTEX);
    }

    // Declare the vertex attributes.
    for (uint32_t attr_idx = 0; attr_idx < attr_count; attr_idx++)
    {
        attr = &visual->attrs[attr_idx];
        ANN(attr);
        ASSERT(attr->item_size > 0);

        // Baker-side.
        dvz_baker_attr(baker, attr_idx, attr->binding_idx, attr->offset, attr->item_size);

        // GPU-side.
        dvz_set_attr(rqr, graphics_id, attr->binding_idx, attr_idx, attr->format, attr->offset);
    }

    // The baker slots are declared directly in dvz_visual_dat() and dvz_visual_tex().
    // Now, we can create the baker. This will create the arrays and dats.

    visual->item_count = item_count;

    bool indexed = (visual->flags & DVZ_VISUALS_FLAGS_INDEXED) != 0;
    // NOTE: if indexed, item_count is the number of indices.
    uint32_t index_count = indexed ? item_count : 0;
    dvz_baker_create(baker, index_count, vertex_count);

    // We now need to send the vertex/descriptor binding requests to the GPU.

    // Send the vertex binding commands.
    for (binding_idx = 0; binding_idx < binding_count; binding_idx++)
    {
        // TODO: dat offset?
        dvz_bind_vertex(
            rqr, graphics_id, binding_idx, baker->vertex_bindings[binding_idx].dual.dat, 0);
    }

    // Send the dat bindings commands.
    for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    {
        dvz_bind_dat(rqr, graphics_id, slot_idx, baker->descriptors[slot_idx].dual.dat, 0);
    }

    // TODO: same for tex

    dvz_obj_created(&visual->obj);
}



/*************************************************************************************************/
/*  Visual common bindings                                                                       */
/*************************************************************************************************/

void dvz_visual_mvp(DvzVisual* visual, DvzMVP mvp)
{
    // NOTE: the data is immediately copied into the visual's baker's dual

    ANN(visual);
    ANN(visual->baker);

    if (visual->baker->slot_count == 0)
    {
        log_debug("skipping visual_mvp() for visual with no common bindings");
        return;
    }

    dvz_baker_uniform(visual->baker, 0, sizeof(DvzMVP), &mvp);
    dvz_baker_update(visual->baker);
}



void dvz_visual_viewport(DvzVisual* visual, DvzViewport viewport)
{
    // NOTE: the data is immediately copied into the visual's baker's dual

    ANN(visual);
    ANN(visual->baker);

    if (visual->baker->slot_count <= 1)
    {
        log_debug("skipping visual_viewport() for visual with no common bindings");
        return;
    }

    dvz_baker_uniform(visual->baker, 1, sizeof(DvzViewport), &viewport);
    dvz_baker_update(visual->baker);
}



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

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



/*************************************************************************************************/
/*  Visual drawing internal functions                                                            */
/*************************************************************************************************/

void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count,
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);

    bool indexed = (visual->flags & DVZ_VISUALS_FLAGS_INDEXED) != 0;
    bool indirect = (visual->flags & DVZ_VISUALS_FLAGS_INDIRECT) != 0;
    ASSERT(!indirect);

    if (indexed)
    {
        dvz_record_draw_indexed(
            visual->rqr, canvas, visual->graphics_id, first, vertex_offset, count, //
            first_instance, instance_count);
    }
    else
    {
        dvz_record_draw(
            visual->rqr, canvas, visual->graphics_id, first, count, //
            first_instance, instance_count);
    }
}



void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count)
{
    ANN(visual);
    ASSERT((visual->flags & DVZ_VISUALS_FLAGS_INDIRECT) != 0);

    DvzBaker* baker = visual->baker;
    ANN(baker);

    DvzId indirect = baker->indirect.dat;
    ASSERT(indirect != DVZ_ID_NONE);

    bool indexed = (visual->flags & DVZ_VISUALS_FLAGS_INDEXED) != 0;

    if (indexed)
    {
        dvz_record_draw_indexed_indirect(
            visual->rqr, canvas, visual->graphics_id, indirect, draw_count);
    }
    else
    {
        dvz_record_draw_indirect(visual->rqr, canvas, visual->graphics_id, indirect, draw_count);
    }
}



void dvz_visual_record(DvzVisual* visual, DvzId canvas)
{
    ANN(visual);

    // Call the draw callback if there is one.
    if (visual->callback != NULL)
    {
        visual->callback(
            visual, canvas, visual->draw_first, visual->draw_count, visual->first_instance,
            visual->instance_count);
    }

    // Otherwise call the default callback.
    else
    {
        dvz_visual_instance(
            visual, canvas, visual->draw_first, 0, visual->draw_count, visual->first_instance,
            visual->instance_count);
    }
}



void dvz_visual_callback(DvzVisual* visual, DvzVisualCallback callback)
{
    ANN(visual);
    ANN(callback);

    visual->callback = callback;
}



/*************************************************************************************************/
/*  Visual drawing                                                                               */
/*************************************************************************************************/

void dvz_visual_visible(DvzVisual* visual, bool is_visible)
{
    ANN(visual);
    visual->is_visible = is_visible;
}
