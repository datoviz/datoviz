/*************************************************************************************************/
/*  Baker                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/baker.h"
#include "request.h"
#include "scene/array.h"
#include "scene/dual.h"



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _check_sizes(DvzBaker* baker)
{
    DvzSize sizes[DVZ_MAX_VERTEX_BINDINGS] = {0};

    // Compute the sum of the sizes of all attributes in each vertex binding.
    for (uint32_t attr_idx = 0; attr_idx < baker->attr_count; attr_idx++)
    {
        sizes[attr_idx] += baker->vertex_attrs[attr_idx].item_size;
    }

    // The vertex binding stride should be larger than, or equal, to that sum.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        ASSERT(baker->vertex_bindings[binding_idx].stride >= sizes[binding_idx]);
    }
}



static void _create_vertex_binding(DvzBaker* baker, uint32_t binding_idx, uint32_t vertex_count)
{
    ANN(baker);
    ASSERT(binding_idx < baker->binding_count);
    ASSERT(vertex_count > 0);

    DvzBakerVertex* bv = &baker->vertex_bindings[binding_idx];
    ANN(bv);

    log_trace(
        "create baker vertex binding #%d with %d vertices, stride %" PRId64
        ", create dat and array",
        binding_idx, vertex_count, bv->stride);

    if (bv->dual.array != NULL)
    {
        log_trace(
            "skipping creation of dat for vertex binding #%d as a shared dual has already been "
            "set",
            binding_idx);
        return;
    }
    bv->dual = dvz_dual_vertex(baker->rqr, vertex_count, bv->stride);
}



static void _create_index(DvzBaker* baker, uint32_t index_count)
{
    ANN(baker);
    ASSERT(index_count > 0);

    log_trace("create index buffer with %d vertices, create dat and array", index_count);
    if (baker->index.array != NULL)
    {
        log_trace(
            "skipping creation of dat for index buffer as a shared dual has already been set");
        return;
    }
    baker->index = dvz_dual_index(baker->rqr, index_count);
}



static void _create_indirect(DvzBaker* baker, bool indexed)
{
    ANN(baker);

    log_trace("create %sindirect buffer, create dat and array", indexed ? "indexed " : "");
    baker->indirect = dvz_dual_indirect(baker->rqr, indexed);
}



static void _create_descriptor(DvzBaker* baker, uint32_t slot_idx)
{
    ANN(baker);
    ASSERT(slot_idx < baker->slot_count);
    log_trace("create baker descriptor #%d, create dat and array", slot_idx);
    DvzBakerDescriptor* bd = &baker->descriptors[slot_idx];
    ANN(bd);

    if (bd->dual.array != NULL)
    {
        log_trace(
            "skipping creation of dat for descriptor #%d as a shared dual has already been set",
            slot_idx);
        return;
    }
    bd->dual = dvz_dual_dat(baker->rqr, bd->item_size);
}



/*************************************************************************************************/
/*  Baker life cycle                                                                             */
/*************************************************************************************************/

DvzBaker* dvz_baker(DvzRequester* rqr, int flags)
{
    ANN(rqr);
    DvzBaker* baker = (DvzBaker*)calloc(1, sizeof(DvzBaker));
    baker->rqr = rqr;

    // 00xx: which attributes should be in a different buf (8 max)
    // xx00: which attributes should be constants
    baker->flags = flags;

    return baker;
}



void dvz_baker_destroy(DvzBaker* baker)
{
    ANN(baker);

    // Destroy all duals.
    DvzBakerVertex* bv = NULL;
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        bv = &baker->vertex_bindings[binding_idx];
        // NOTE: this will destroy the dual's array only if dual.need_destroy=true (which only
        // occurs if the dual was created with one of the helper functions in dual.c).
        dvz_dual_destroy(&bv->dual);
        // NOTE: the dat is not destroyed at the moment.
    }

    DvzBakerDescriptor* bd = NULL;
    for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    {
        bd = &baker->descriptors[slot_idx];
        // NOTE: same as above.
        dvz_dual_destroy(&bd->dual);
    }

    FREE(baker);
}



/*************************************************************************************************/
/*  Baker sharing                                                                                */
/*************************************************************************************************/

void dvz_baker_share_vertex(DvzBaker* baker, uint32_t binding_idx, DvzDual* dual)
{
    ANN(baker);
    ANN(dual);
    ASSERT(binding_idx < baker->binding_count);

    DvzBakerVertex* bv = &baker->vertex_bindings[binding_idx];
    ANN(bv);

    log_trace("set shared dual for vertex binding #%d", binding_idx);
    bv->dual = *dual;
}



void dvz_baker_share_uniform(DvzBaker* baker, uint32_t binding_idx, DvzDual* dual)
{
    ANN(baker);
    ANN(dual);
    ASSERT(binding_idx < baker->slot_count);

    DvzBakerDescriptor* bd = &baker->descriptors[binding_idx];
    ANN(bd);

    log_trace("set shared dual for descriptor binding #%d", binding_idx);
    bd->dual = *dual;
}



void dvz_baker_share_index(DvzBaker* baker, DvzDual* dual)
{
    ANN(baker);
    ANN(dual);

    log_trace("set shared dual for index buffer");
    baker->index = *dual;
}



/*************************************************************************************************/
/*  Baker specification                                                                          */
/*************************************************************************************************/

// declare a vertex binding
void dvz_baker_vertex(DvzBaker* baker, uint32_t binding_idx, DvzSize stride)
{
    ANN(baker);
    ASSERT(binding_idx < DVZ_MAX_VERTEX_BINDINGS);
    ASSERT(stride > 0);

    baker->vertex_bindings[binding_idx].binding_idx = binding_idx;
    baker->vertex_bindings[binding_idx].stride = stride;
    baker->binding_count = MAX(baker->binding_count, binding_idx + 1);

    log_trace("declare vertex binding #%d with stride %d", binding_idx, stride);
}



// declare a GLSL attribute
void dvz_baker_attr(
    DvzBaker* baker, uint32_t attr_idx, uint32_t binding_idx, DvzSize offset, DvzSize item_size)
{
    ANN(baker);
    ASSERT(attr_idx < DVZ_MAX_VERTEX_ATTRS);

    baker->vertex_attrs[attr_idx].binding_idx = binding_idx;
    baker->vertex_attrs[attr_idx].offset = offset;
    baker->vertex_attrs[attr_idx].item_size = item_size;
    baker->attr_count = MAX(baker->attr_count, attr_idx + 1);

    log_trace(
        "declare vertex attr #%d (binding #%d) with offset %d and size %d", //
        attr_idx, binding_idx, offset, item_size);
}



// declare a descriptor slot
void dvz_baker_slot(DvzBaker* baker, uint32_t slot_idx, DvzSize item_size)
{
    ANN(baker);
    baker->descriptors[slot_idx].slot_idx = slot_idx;
    baker->descriptors[slot_idx].item_size = item_size;
    baker->slot_count = MAX(baker->slot_count, slot_idx + 1);

    log_trace("declare slot #%d with size %d", slot_idx, item_size);
}



void dvz_baker_indirect(DvzBaker* baker)
{
    ANN(baker);
    baker->is_indirect = true;
}



/*************************************************************************************************/
/*  Baker data functions                                                                         */
/*************************************************************************************************/

void dvz_baker_data(DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, void* data)
{
    dvz_baker_repeat(baker, attr_idx, first, count, 1, data);
}



void dvz_baker_resize(DvzBaker* baker, uint32_t vertex_count)
{
    ANN(baker);
    log_trace("resize the baker to %d vertices", vertex_count);

    // Resize the vertex bindings.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        // Resize the underlying dual array.
        dvz_array_resize(baker->vertex_bindings[binding_idx].dual.array, vertex_count);

        // Emit the dual's dat resize commands.
        dvz_dual_resize(&baker->vertex_bindings[binding_idx].dual, vertex_count);
    }
}



void dvz_baker_repeat(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, uint32_t repeats,
    void* data)
{
    ANN(baker);
    if (baker->attr_count == 0)
    {
        log_error("unitialized baker");
        return;
    }
    ASSERT(attr_idx < baker->attr_count);
    ASSERT(count > 0);
    ANN(data);

    DvzBakerAttr* attr = &baker->vertex_attrs[attr_idx];
    uint32_t binding_idx = attr->binding_idx;
    ASSERT(binding_idx < baker->binding_count);

    DvzBakerVertex* vertex = &baker->vertex_bindings[binding_idx];

    DvzDual* dual = &vertex->dual;
    if (dual == NULL)
    {
        log_error("dual is null, please call dvz_baker_create()");
        return;
    }
    ANN(dual);
    if (dual->array == NULL)
    {
        log_error("dual's array is null");
        return;
    }

    if (first + count * repeats > dual->array->item_count)
    {
        log_error("baker array is too small to hold the specified data");
        return;
    }

    DvzSize offset = attr->offset;
    DvzSize item_size = attr->item_size;
    ASSERT(item_size > 0);

    DvzSize col_size = vertex->stride;
    ASSERT(col_size > 0);

    // log_info("%d %d %d %d %d", offset, item_size, first, count, repeats);
    dvz_dual_column(dual, offset, item_size, first, count, repeats, data);
}



void dvz_baker_quads(
    DvzBaker* baker, uint32_t attr_idx, vec2 quad_size, uint32_t count, vec2* positions)
{
    ANN(baker);
    // TODO
}



void dvz_baker_uniform(DvzBaker* baker, uint32_t binding_idx, DvzSize size, void* data)
{
    // NOTE: the data is immediately copied into the baker's dual

    ANN(baker);
    ASSERT(binding_idx < baker->slot_count);
    ASSERT(size > 0);
    ANN(data);

    ASSERT(binding_idx < baker->slot_count);
    DvzBakerDescriptor* descriptor = &baker->descriptors[binding_idx];

    DvzDual* dual = &descriptor->dual;
    if (dual == NULL)
    {
        log_error("dual is null, please call dvz_baker_create()");
        return;
    }
    ANN(dual);

    dvz_dual_data(dual, 0, 1, data);
}



/*************************************************************************************************/
/*  Baker sync functions                                                                         */
/*************************************************************************************************/

void dvz_baker_create(DvzBaker* baker, uint32_t index_count, uint32_t vertex_count)
{
    ANN(baker);
    log_trace(
        "create the dat, arrays, %d bindings, %d descriptors", //
        baker->binding_count, baker->slot_count);

    // Check size consistency.
    _check_sizes(baker);

    // Create the vertex bindings.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        _create_vertex_binding(baker, binding_idx, vertex_count);
    }

    // Create the uniform dats for the descriptors.
    for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    {
        _create_descriptor(baker, slot_idx);
    }

    // Create the index buffer.
    if (index_count > 0)
    {
        _create_index(baker, index_count);
    }

    // Create the indirect buffer.
    if (baker->is_indirect)
    {
        _create_indirect(baker, index_count > 0);
    }
}



// emit the dat update commands to synchronize the dual arrays on the GPU
void dvz_baker_update(DvzBaker* baker)
{
    ANN(baker);

    // Update the vertex bindings duals.
    for (uint32_t binding_idx = 0; binding_idx < baker->binding_count; binding_idx++)
    {
        dvz_dual_update(&baker->vertex_bindings[binding_idx].dual);
    }

    // Update the descriptor duals.
    for (uint32_t slot_idx = 0; slot_idx < baker->slot_count; slot_idx++)
    {
        dvz_dual_update(&baker->descriptors[slot_idx].dual);
    }
}
